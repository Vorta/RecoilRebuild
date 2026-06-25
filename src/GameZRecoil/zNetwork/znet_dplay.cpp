#include "GameZRecoil/zNetwork/zNetwork.h"

#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zReader/zReader.h"

#include <dplay.h>
#include <dplobby.h>
#include <objbase.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" HWND g_RecoilApp_hWndMain;
extern "C" char g_Player_MasterTypeName_Unknown[0x08];

extern "C" {
/**
 * Reimplements data 0x4db5b0: g_zNetwork_ProviderName_Modem.
 * Data owner: network_online.znetwork_provider_session_literals.
 * Purpose: provide the DirectPlay provider-name token used to identify modem providers.
 */
char g_zNetwork_ProviderName_Modem[0x6] = "Modem";
/**
 * Reimplements data 0x4db5b8: g_zNetwork_ProviderName_TcpIp.
 * Data owner: network_online.znetwork_provider_session_literals.
 * Purpose: provide the DirectPlay provider-name token used to identify TCP/IP providers.
 */
char g_zNetwork_ProviderName_TcpIp[0x7] = "TCP/IP";
/**
 * Reimplements data 0x4db5c0: g_zNetwork_ProviderName_Ipx.
 * Data owner: network_online.znetwork_provider_session_literals.
 * Purpose: provide the DirectPlay provider-name token used to identify IPX providers.
 */
char g_zNetwork_ProviderName_Ipx[0x4] = "IPX";
/**
 * Reimplements data 0x4db5c4: g_zNetwork_ModemSessionName.
 * Data owner: network_online.znetwork_provider_session_literals.
 * Purpose: provide the default DirectPlay session name for immediate modem-host creation.
 */
char g_zNetwork_ModemSessionName[0xd] = "ModemSession";
/**
 * Reimplements data 0x56aaf0: Symbol.
 * Data owner: engine.znetwork.directplay_runtime_globals.
 * Purpose: cache the active IDirectPlay4A interface for zNetwork session calls.
 */
zNetwork_DPlay4 *g_zNetwork_pDirectPlay4 = 0;
/**
 * Reimplements data 0x56aaf8: Symbol.
 * Data owner: engine.znetwork.directplay_runtime_globals.
 * Purpose: hold the local DirectPlay player record while joined to a session.
 */
zNetwork_PlayerRecord *g_zNetwork_LocalPlayerRecord = 0;
/**
 * Reimplements data 0x56aa30: Symbol.
 * Data owner: engine.znetwork.directplay_runtime_globals.
 * Purpose: cache whether the local player is the session host.
 */
int g_zNetwork_IsHostFlag = 0;
/**
 * Reimplements data 0x56aa44: Symbol.
 * Data owner: engine.znetwork.dplay_player_runtime_lifecycle.
 * Purpose: cache the local DirectPlay player identifier.
 */
int g_zNetwork_LocalPlayerKey = 0;
/**
 * Reimplements data 0x56aa50: Symbol.
 * Data owner: engine.znetwork.dplay_player_runtime_lifecycle.
 * Purpose: provide the fixed local player-name buffer used by DirectPlay.
 */
char g_zNetwork_LocalPlayerNameScratch[0x50] = {0};
/**
 * Reimplements data 0x56aa3c: Symbol.
 * Data owner: engine.znetwork.directplay_runtime_globals.
 * Purpose: select asynchronous TCP/IP sends when provider caps allow it.
 */
int g_zNetwork_TcpIpAsyncSendEnabled = 0;
/**
 * Reimplements data 0x56aa34: Symbol.
 * Data owner: engine.znetwork.service_provider_list.
 * Purpose: cache whether the selected DirectPlay service-provider record is modem-backed.
 */
int g_zNetwork_ActiveProviderIsModem = 0;
/**
 * Reimplements data 0x56aa38: Symbol.
 * Data owner: engine.znetwork.directplay_runtime_globals.
 * Purpose: cache whether the current DirectPlay provider is TCP/IP.
 */
int g_zNetwork_ActiveProviderIsTcpIp = 0;
/**
 * Reimplements data 0x56ab08: Symbol.
 * Data owner: engine.znetwork.directplay_runtime_globals.
 * Purpose: store DirectPlay capability bits queried for send-mode selection.
 */
zNetworkDPlayCaps g_zNetwork_DPlayCaps = {0};
/**
 * Reimplements data 0x56aa48: Symbol.
 * Data owner: engine.znetwork.directplay_runtime_globals.
 * Purpose: point to the application GUID used for session enumeration.
 */
GUID *g_zNetwork_AppGuid = 0;
/**
 * Reimplements data 0x4ccd78: Symbol.
 * Data owner: engine.znetwork.directplay_runtime_globals.
 * Purpose: hold Recoil's DirectPlay application GUID.
 */
GUID g_zNetwork_RecoilAppGuid = {
    0xc94ebca1,
    0x95b7,
    0x11d2,
    {0xa7, 0x7c, 0x00, 0x60, 0x08, 0x98, 0x77, 0x43}
};
/**
 * Reimplements data 0x56ab00: Symbol.
 * Data owner: engine.znetwork.session_runtime_lifecycle.
 * Purpose: track the last asynchronous DirectPlay SendEx handle.
 */
unsigned int g_zNetwork_LastSendExHandle = 0;
/**
 * Reimplements data 0x56ab04: Symbol.
 * Data owner: engine.znetwork.session_runtime_lifecycle.
 * Purpose: track completion state for the last asynchronous SendEx packet.
 */
int g_zNetwork_LastSendExCompleted = 0;
/**
 * Reimplements data 0x56aa28: Symbol.
 * Data owner: engine.znetwork.session_runtime_lifecycle.
 * Purpose: indicate that session runtime globals have been initialized.
 */
int g_zNetwork_SessionRuntimeInitialized = 0;
/**
 * Reimplements data 0x56aaf4: Symbol.
 * Data owner: engine.znetwork.directplay_runtime_globals.
 * Purpose: cache the currently selected DirectPlay session descriptor.
 */
zNetworkDPlaySessionDescCache *g_zNetwork_CurrentSessionDescCache = 0;
/**
 * Reimplements data 0x56aafc: Symbol.
 * Data owner: engine.znetwork.session_runtime_lifecycle.
 * Purpose: store the callback invoked on fatal DirectPlay disconnect.
 */
zNetworkFatalDisconnectCallback g_zNetwork_FatalDisconnectCallback = 0;
/**
 * Reimplements data 0x56aa40: Symbol.
 * Data owner: engine.znetwork.session_runtime_lifecycle.
 * Purpose: prevent repeated fatal-disconnect callback dispatch.
 */
int g_zNetwork_FatalDisconnectTriggered = 0;
/**
 * Reimplements data 0x4e185c: Symbol.
 * Data owner: engine.znetwork.dplay_player_runtime_lifecycle.
 * Purpose: mirror the cached current-player count used by system messages.
 */
int g_zNetworkCurrentPlayerCountCached = 1;
/**
 * Reimplements data 0x56aaa0: Symbol.
 * Data owner: engine.znetwork.directplay_runtime_globals.
 * Purpose: cache the current session name buffer for descriptor updates.
 */
char g_zNetwork_SessionNameCache[kZNetworkSessionNameCacheBytes] = {0};
/**
 * Reimplements data 0x56ab30: Symbol.
 * Data owner: engine.znetwork.session_enumeration_list.
 * Purpose: own the archive list of enumerated session descriptors.
 */
zArchiveList *g_zNetwork_EnumeratedSessionList = 0;
/**
 * Reimplements data 0x56ab38: Symbol.
 * Data owner: engine.znetwork.service_provider_list.
 * Purpose: own the recovered vector of DirectPlay service-provider records.
 */
zNetworkServiceProviderListVec *g_zNetwork_ServiceProviderList = 0;
/**
 * Reimplements data 0x56ab34: Symbol.
 * Data owner: engine.znetwork.directplay_runtime_globals.
 * Purpose: own the recovered intrusive list of player records.
 */
zNetworkPlayerRecordList *g_zNetwork_PlayerRecordList = 0;
/**
 * Reimplements data 0x56ab3c: Symbol.
 * Data owner: engine.znetwork.session_runtime_lifecycle.
 * Purpose: hold the reusable DirectPlay receive buffer.
 */
void *g_zNetwork_ReceiveBuffer = 0;
/**
 * Reimplements data 0x56add4: Symbol.
 * Data owner: engine.znetwork.session_runtime_lifecycle.
 * Purpose: record the allocated size of the reusable receive buffer.
 */
unsigned int g_zNetwork_ReceiveBufferCapacity = 0;
/**
 * Reimplements data 0x56ad50: Symbol.
 * Data owner: engine.znetwork.dplay_player_runtime_lifecycle.
 * Purpose: track player-color slots currently in use by the session.
 */
int g_zNetwork_PlayerColorInUseFlags[16] = {0};
/**
 * Reimplements data 0x56addc: Symbol.
 * Data owner: engine.znetwork.directplay_runtime_globals.
 * Purpose: point to the packet-dispatch handler list sentinel.
 */
zNetworkDispatchHandlerListNode *g_zNetwork_DispatchHandlerListSentinel = 0;
/**
 * Reimplements data 0x56ade0: Symbol.
 * Data owner: engine.znetwork.directplay_runtime_globals.
 * Purpose: count packet-dispatch handler list nodes.
 */
int g_zNetwork_DispatchHandlerListCount = 0;
/**
 * Reimplements data 0x56add8: Symbol.
 * Data owner: engine.znetwork.directplay_runtime_globals.
 * Purpose: preserve the recovered dispatch-handler list allocator flag byte.
 */
unsigned char g_zNetwork_DispatchHandlerListFlags = 0;
/**
 * Reimplements data 0x4e1860..0x4e18ff:
 * network_online.znetwork_dplay_literal_pool diagnostics header.
 * Purpose: provide writable znet_dplay.cpp source/capability literals used by
 * DirectPlay reporting and TCP/IP send-mode diagnostics.
 */
/**
 * Reimplements data 0x4e1860: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_SourceFile_ZnetDplayCpp[0x2c] =
    "D:\\Proj\\GameZRecoil\\zNetwork\\znet_dplay.cpp";
/**
 * Reimplements data 0x4e188c: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_UsingTcpIpFmt[0x19] =
    "Network using TCP/IP %s\n";
/**
 * Reimplements data 0x4e18a8: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_SyncModeName[0x6] = "SYNCH";
/**
 * Reimplements data 0x4e18b0: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_AsyncModeName[0x7] = "ASYNCH";
/**
 * Reimplements data 0x4e18b8: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_GuaranteedTcpIpNotSupportedMsg[0x22] =
    "Guaranteed TCP/IP not supported n";
/**
 * Reimplements data 0x4e18dc: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_GetCapabilitiesFailedMsg[0x24] =
    "Failed to get network capabilities\n";
/**
 * Reimplements data 0x4e1900..0x4e1ad8:
 * network_online.znetwork_dplay_literal_pool DirectPlay open-failure text.
 * Purpose: provide the writable UI strings selected by the recovered
 * ReportDPlayOpenFailure helper.
 */
/**
 * Reimplements data 0x4e1900: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_SignatureFailure[0x12] = "Signature Failure";
/**
 * Reimplements data 0x4e1914: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_LogonDenied[0xd] = "Logon Denied";
/**
 * Reimplements data 0x4e1924: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_EncryptionNotSupported[0x19] =
    "Encryption Not Supported";
/**
 * Reimplements data 0x4e1940: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_EncryptionFailed[0x12] = "Encryption Failed";
/**
 * Reimplements data 0x4e1954: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_SecuritySupportProviderError[0x20] =
    "Security Support Provider Error";
/**
 * Reimplements data 0x4e1974: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_CannotLoadSecurityPackage[0x1d] =
    "Cannot Load Security Package";
/**
 * Reimplements data 0x4e1994: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_CryptographyServicesError[0x1c] =
    "Cryptography Services Error";
/**
 * Reimplements data 0x4e19b0: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_AuthenticationFailed[0x16] =
    "Authentication Failed";
/**
 * Reimplements data 0x4e19c8: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_ConnectionLost[0x10] = "Connection Lost";
/**
 * Reimplements data 0x4e19d8: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_ErrorConnecting[0x11] = "Error Connecting";
/**
 * Reimplements data 0x4e19ec: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_InvalidPassword[0x11] = "Invalid Password";
/**
 * Reimplements data 0x4e1a00: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_NoNewPlayersAllowed[0x17] =
    "No New Players Allowed";
/**
 * Reimplements data 0x4e1a18: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_InitializationError[0x15] =
    "Initialization Error";
/**
 * Reimplements data 0x4e1a30: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_CannotCreateServer[0x15] =
    "Cannot Create Server";
/**
 * Reimplements data 0x4e1a48: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_TimeoutError[0xe] = "Timeout Error";
/**
 * Reimplements data 0x4e1a58: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_NoExistingSessions[0x15] =
    "No Existing Sessions";
/**
 * Reimplements data 0x4e1a70: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_NoConnection[0xe] = "No Connection";
/**
 * Reimplements data 0x4e1a80: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_InvalidFlags[0x15] =
    "Sorry, Invalid Flags";
/**
 * Reimplements data 0x4e1a98: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_AccessDenied[0xe] = "Access Denied";
/**
 * Reimplements data 0x4e1aa8: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_AlreadyInitialized[0x14] =
    "Already Initialized";
/**
 * Reimplements data 0x4e1abc: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorText_InvalidParameters[0x1a] =
    "Sorry, Invalid Parameters";
/**
 * Reimplements data 0x4e1ad8: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_RecoilNetworkErrorMsg[0x15] = "Recoil Network Error";
/**
 * Reimplements data 0x4e1af0..0x4e1c48:
 * network_online.znetwork_dplay_literal_pool runtime diagnostics.
 * Purpose: provide writable player/session, receive-buffer, COM, and
 * DirectPlay report-format literals used by znet_dplay.cpp.
 */
/**
 * Reimplements data 0x4e1af0: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_NoNetworkConnectionMsg[0x16] = "No Network Connection";
/**
 * Reimplements data 0x4e1b08: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_NetworkConnectionLostMsg[0x26] =
    "Your Network Connection Has Been Lost";
/**
 * Reimplements data 0x4e1b30: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_InvalidPlayerParametersMsg[0x21] =
    "Sorry, Invalid Player Parameters";
/**
 * Reimplements data 0x4e1b54: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_CannotCreateAnotherPlayerMsg[0x1d] =
    "Cannot Create Another Player";
/**
 * Reimplements data 0x4e1b74: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_CannotAddAnotherPlayerMsg[0x1a] =
    "Cannot Add Another Player";
/**
 * Reimplements data 0x4e1b90: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DefaultPlayerName[0x7] = "noname";
/**
 * Reimplements data 0x4e1b98: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_ReceiveBufferIncreasedFmt[0x2e] =
    "Receiving buffer size increased from %d to %d";
/**
 * Reimplements data 0x4e1bc8: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_UnhandledDirectPlaySystemMessageMsg[0x24] =
    "Unhandled DirectPlay system message";
/**
 * Reimplements data 0x4e1bec: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_CoCreateNotInitializedMsg[0x19] =
    "CoCreate not initialized";
/**
 * Reimplements data 0x4e1c08: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_ClassCannotBeCreatedMsg[0x18] =
    "Class cannot be created";
/**
 * Reimplements data 0x4e1c20: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_ClassNotRegisteredMsg[0x15] = "Class not registered";
/**
 * Reimplements data 0x4e1c38: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_ForcedTcpIpModeName[0xe] = "forced TCP/IP";
/**
 * Reimplements data 0x4e1c48: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DirectPlayErrorFmt[0x1e] =
    "DirectPlay Error (0x%08x)[%s]";
/**
 * Reimplements data 0x4e1c68..0x4e20f8:
 * network_online.znetwork_dplay_literal_pool DirectPlay HRESULT names.
 * Purpose: provide the writable strings selected by zNetwork_DPlay_ReportError.
 */
/**
 * Reimplements data 0x4e1c68: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_LogonDenied[0x13] = "DPERR_LOGONDENIED ";
/**
 * Reimplements data 0x4e1c7c: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_NotLoggedIn[0x12] = "DPERR_NOTLOGGEDIN";
/**
 * Reimplements data 0x4e1c90: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_CantLoadCapi[0x13] = "DPERR_CANTLOADCAPI";
/**
 * Reimplements data 0x4e1ca4: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_EncryptionNotSupported[0x1d] =
    "DPERR_ENCRYPTIONNOTSUPPORTED";
/**
 * Reimplements data 0x4e1cc4: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_CantLoadSecurityPackage[0x1e] =
    "DPERR_CANTLOADSECURITYPACKAGE";
/**
 * Reimplements data 0x4e1ce4: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_SignFailed[0x11] = "DPERR_SIGNFAILED";
/**
 * Reimplements data 0x4e1cf8: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_EncryptionFailed[0x17] =
    "DPERR_ENCRYPTIONFAILED";
/**
 * Reimplements data 0x4e1d10: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_CantLoadSspi[0x13] = "DPERR_CANTLOADSSPI";
/**
 * Reimplements data 0x4e1d24: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_AuthenticationFailed[0x1b] =
    "DPERR_AUTHENTICATIONFAILED";
/**
 * Reimplements data 0x4e1d40: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_NotLobbied[0x11] = "DPERR_NOTLOBBIED";
/**
 * Reimplements data 0x4e1d54: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_UnknownApplication[0x19] =
    "DPERR_UNKNOWNAPPLICATION";
/**
 * Reimplements data 0x4e1d70: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_InvalidInterface[0x18] =
    "DPERR_INVALIDINTERFACE ";
/**
 * Reimplements data 0x4e1d88: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_AppNotStarted[0x14] = "DPERR_APPNOTSTARTED";
/**
 * Reimplements data 0x4e1d9c: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_CantCreateProcess[0x18] =
    "DPERR_CANTCREATEPROCESS";
/**
 * Reimplements data 0x4e1db4: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_BufferTooLarge[0x15] =
    "DPERR_BUFFERTOOLARGE";
/**
 * Reimplements data 0x4e1dcc: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_InvalidPriority[0x16] =
    "DPERR_INVALIDPRIORITY";
/**
 * Reimplements data 0x4e1de4: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_CancelFailed[0x13] = "DPERR_CANCELFAILED";
/**
 * Reimplements data 0x4e1df8: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_UnknownMessage[0x15] =
    "DPERR_UNKNOWNMESSAGE";
/**
 * Reimplements data 0x4e1e10: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_ConnectionLost[0x15] =
    "DPERR_CONNECTIONLOST";
/**
 * Reimplements data 0x4e1e28: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_Connecting[0x11] = "DPERR_CONNECTING";
/**
 * Reimplements data 0x4e1e3c: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_NoNewPlayers[0x13] = "DPERR_NONEWPLAYERS";
/**
 * Reimplements data 0x4e1e50: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_Uninitialized[0x15] =
    "DPERR_UNINITIALIZED ";
/**
 * Reimplements data 0x4e1e68: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_SessionLost[0x12] = "DPERR_SESSIONLOST";
/**
 * Reimplements data 0x4e1e7c: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_PlayerLost[0x12] = "DPERR_PLAYERLOST ";
/**
 * Reimplements data 0x4e1e90: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_CannotCreateServer[0x19] =
    "DPERR_CANNOTCREATESERVER";
/**
 * Reimplements data 0x4e1eac: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_UserCancel[0x11] = "DPERR_USERCANCEL";
/**
 * Reimplements data 0x4e1ec0: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_Busy[0xb] = "DPERR_BUSY";
/**
 * Reimplements data 0x4e1ecc: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_Unavailable[0x12] = "DPERR_UNAVAILABLE";
/**
 * Reimplements data 0x4e1ee0: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_Timeout[0xe] = "DPERR_TIMEOUT";
/**
 * Reimplements data 0x4e1ef0: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_SendTooBig[0x11] = "DPERR_SENDTOOBIG";
/**
 * Reimplements data 0x4e1f04: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_NoSessions[0x11] = "DPERR_NOSESSIONS";
/**
 * Reimplements data 0x4e1f18: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_NoPlayers[0x10] = "DPERR_NOPLAYERS";
/**
 * Reimplements data 0x4e1f28: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_NoNameServerFound[0x18] =
    "DPERR_NONAMESERVERFOUND";
/**
 * Reimplements data 0x4e1f40: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_NoMessages[0x12] = "DPERR_NOMESSAGES ";
/**
 * Reimplements data 0x4e1f54: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_NoConnection[0x13] = "DPERR_NOCONNECTION";
/**
 * Reimplements data 0x4e1f68: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_NoCaps[0xd] = "DPERR_NOCAPS";
/**
 * Reimplements data 0x4e1f78: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_InvalidGroup[0x13] = "DPERR_INVALIDGROUP";
/**
 * Reimplements data 0x4e1f8c: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_InvalidPlayer[0x14] = "DPERR_INVALIDPLAYER";
/**
 * Reimplements data 0x4e1fa0: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_InvalidObject[0x14] = "DPERR_INVALIDOBJECT";
/**
 * Reimplements data 0x4e1fb4: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_InvalidFlags[0x13] = "DPERR_INVALIDFLAGS";
/**
 * Reimplements data 0x4e1fc8: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_Exception[0x10] = "DPERR_EXCEPTION";
/**
 * Reimplements data 0x4e1fd8: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_CapsNotAvailableYet[0x1a] =
    "DPERR_CAPSNOTAVAILABLEYET";
/**
 * Reimplements data 0x4e1ff4: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_CantCreateSession[0x18] =
    "DPERR_CANTCREATESESSION";
/**
 * Reimplements data 0x4e200c: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_CantCreatePlayer[0x17] =
    "DPERR_CANTCREATEPLAYER";
/**
 * Reimplements data 0x4e2024: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_CantCreateGroup[0x17] =
    "DPERR_CANTCREATEGROUP:";
/**
 * Reimplements data 0x4e203c: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_CantAddPlayer[0x14] = "DPERR_CANTADDPLAYER";
/**
 * Reimplements data 0x4e2050: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_BufferTooSmall[0x15] =
    "DPERR_BUFFERTOOSMALL";
/**
 * Reimplements data 0x4e2068: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_ActivePlayers[0x14] = "DPERR_ACTIVEPLAYERS";
/**
 * Reimplements data 0x4e207c: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_AccessDenied[0x13] = "DPERR_ACCESSDENIED";
/**
 * Reimplements data 0x4e2090: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_AlreadyInitialized[0x19] =
    "DPERR_ALREADYINITIALIZED";
/**
 * Reimplements data 0x4e20ac: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_InvalidParams[0x14] = "DPERR_INVALIDPARAMS";
/**
 * Reimplements data 0x4e20c0: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_OutOfMemory[0x12] = "DPERR_OUTOFMEMORY";
/**
 * Reimplements data 0x4e20d4: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_Generic[0xe] = "DPERR_GENERIC";
/**
 * Reimplements data 0x4e20e4: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_Unsupported[0x12] = "DPERR_UNSUPPORTED";
/**
 * Reimplements data 0x4e20f8: Symbol.
 * Data owner: network_online.znetwork_dplay_literal_pool.
 * Purpose: provide a writable DirectPlay diagnostic/reporting literal.
 */
char g_zNetwork_DpErrorName_Pending[0xe] = "DPERR_PENDING";
}

RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_SourceFile_ZnetDplayCpp) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_UsingTcpIpFmt) == 0x19);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_SyncModeName) == 0x6);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_AsyncModeName) == 0x7);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_GuaranteedTcpIpNotSupportedMsg) == 0x22);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_GetCapabilitiesFailedMsg) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_SignatureFailure) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_LogonDenied) == 0xd);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_EncryptionNotSupported) == 0x19);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_EncryptionFailed) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_SecuritySupportProviderError) == 0x20);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_CannotLoadSecurityPackage) == 0x1d);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_CryptographyServicesError) == 0x1c);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_AuthenticationFailed) == 0x16);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_ConnectionLost) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_ErrorConnecting) == 0x11);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_InvalidPassword) == 0x11);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_NoNewPlayersAllowed) == 0x17);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_InitializationError) == 0x15);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_CannotCreateServer) == 0x15);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_TimeoutError) == 0xe);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_NoExistingSessions) == 0x15);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_NoConnection) == 0xe);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_InvalidFlags) == 0x15);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_AccessDenied) == 0xe);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_AlreadyInitialized) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorText_InvalidParameters) == 0x1a);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_RecoilNetworkErrorMsg) == 0x15);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_NoNetworkConnectionMsg) == 0x16);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_NetworkConnectionLostMsg) == 0x26);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_InvalidPlayerParametersMsg) == 0x21);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_CannotCreateAnotherPlayerMsg) == 0x1d);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_CannotAddAnotherPlayerMsg) == 0x1a);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DefaultPlayerName) == 0x7);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_ReceiveBufferIncreasedFmt) == 0x2e);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_UnhandledDirectPlaySystemMessageMsg) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_CoCreateNotInitializedMsg) == 0x19);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_ClassCannotBeCreatedMsg) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_ClassNotRegisteredMsg) == 0x15);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_ForcedTcpIpModeName) == 0xe);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DirectPlayErrorFmt) == 0x1e);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_LogonDenied) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_NotLoggedIn) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_CantLoadCapi) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_EncryptionNotSupported) == 0x1d);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_CantLoadSecurityPackage) == 0x1e);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_SignFailed) == 0x11);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_EncryptionFailed) == 0x17);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_CantLoadSspi) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_AuthenticationFailed) == 0x1b);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_NotLobbied) == 0x11);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_UnknownApplication) == 0x19);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_InvalidInterface) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_AppNotStarted) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_CantCreateProcess) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_BufferTooLarge) == 0x15);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_InvalidPriority) == 0x16);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_CancelFailed) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_UnknownMessage) == 0x15);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_ConnectionLost) == 0x15);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_Connecting) == 0x11);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_NoNewPlayers) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_Uninitialized) == 0x15);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_SessionLost) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_PlayerLost) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_CannotCreateServer) == 0x19);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_UserCancel) == 0x11);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_Busy) == 0xb);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_Unavailable) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_Timeout) == 0xe);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_SendTooBig) == 0x11);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_NoSessions) == 0x11);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_NoPlayers) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_NoNameServerFound) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_NoMessages) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_NoConnection) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_NoCaps) == 0xd);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_InvalidGroup) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_InvalidPlayer) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_InvalidObject) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_InvalidFlags) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_Exception) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_CapsNotAvailableYet) == 0x1a);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_CantCreateSession) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_CantCreatePlayer) == 0x17);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_CantCreateGroup) == 0x17);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_CantAddPlayer) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_BufferTooSmall) == 0x15);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_ActivePlayers) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_AccessDenied) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_AlreadyInitialized) == 0x19);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_InvalidParams) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_OutOfMemory) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_Generic) == 0xe);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_Unsupported) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(g_zNetwork_DpErrorName_Pending) == 0xe);

namespace {
const int kDPlayPending = (int)(0x8000000a);
const int kDPlayBufferTooSmall = (int)(0x8877001e);
const int kDPlayConnecting = (int)(0x8877015e);

/**
 * Recovered local helper: ReportDPlayOpenFailure.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp.
 * Original helper evidence: no standalone retail address; used by address-backed caller
 * zNetworkDPlay::OpenSelectedSessionAndReadStatusFields at 0x48a520.
 * Purpose: display the DirectPlay Open failure message for selected HRESULTs.
 */
int ReportDPlayOpenFailure(
    int hresult
) {
    const char *message = 0;
    switch (hresult) {
    case (int)(0x80070057) :
        message = g_zNetwork_DpErrorText_InvalidParameters;
        break;
    case (int)(0x88770005) :
        message = g_zNetwork_DpErrorText_AlreadyInitialized;
        break;
    case (int)(0x8877000a) :
        message = g_zNetwork_DpErrorText_AccessDenied;
        break;
    case (int)(0x88770078) :
        message = g_zNetwork_DpErrorText_InvalidFlags;
        break;
    case (int)(0x887700aa) :
        message = g_zNetwork_DpErrorText_NoConnection;
        break;
    case (int)(0x887700dc) :
        message = g_zNetwork_DpErrorText_NoExistingSessions;
        break;
    case (int)(0x887700f0) :
        message = g_zNetwork_DpErrorText_TimeoutError;
        break;
    case (int)(0x88770122) :
        message = g_zNetwork_DpErrorText_CannotCreateServer;
        break;
    case (int)(0x88770140) :
        message = g_zNetwork_DpErrorText_InitializationError;
        break;
    case (int)(0x8877014a) :
        message = g_zNetwork_DpErrorText_NoNewPlayersAllowed;
        break;
    case (int)(0x88770154) :
        message = g_zNetwork_DpErrorText_InvalidPassword;
        break;
    case (int)(0x8877015e) :
        message = g_zNetwork_DpErrorText_ErrorConnecting;
        break;
    case (int)(0x88770168) :
        message = g_zNetwork_DpErrorText_ConnectionLost;
        break;
    case (int)(0x887707d0) :
        message = g_zNetwork_DpErrorText_AuthenticationFailed;
        break;
    case (int)(0x887707da) :
        message = g_zNetwork_DpErrorText_SecuritySupportProviderError;
        break;
    case (int)(0x887707e4) :
        message = g_zNetwork_DpErrorText_EncryptionFailed;
        break;
    case (int)(0x887707ee) :
        message = g_zNetwork_DpErrorText_SignatureFailure;
        break;
    case (int)(0x887707f8) :
        message = g_zNetwork_DpErrorText_CannotLoadSecurityPackage;
        break;
    case (int)(0x88770802) :
        message = g_zNetwork_DpErrorText_EncryptionNotSupported;
        break;
    case (int)(0x8877080c) :
        message = g_zNetwork_DpErrorText_CryptographyServicesError;
        break;
    case (int)(0x88770820) :
        message = g_zNetwork_DpErrorText_LogonDenied;
        break;
    }

    if (message != 0) {
        MessageBoxA(
            g_RecoilApp_hWndMain,
            message,
            g_zNetwork_RecoilNetworkErrorMsg,
            MB_OK
        );
    }

    return 0;
}

/**
 * Recovered local helper: AppendServiceProviderInfo.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp.
 * Original helper evidence: no standalone retail address; fully inlined in
 * zNetworkDPlay::EnumConnectionsCallback_AddServiceProviderInfo at 0x48b3a0, where BN
 * shows the service-provider vector insertion, capacity growth, copied pointer range,
 * and MSVC EH setup around the callback body.
 * Purpose: append a DirectPlay service-provider record to the recovered vector.
 */
inline void AppendServiceProviderInfo(
    zNetworkDPlayServiceProviderInfo *info
) {
    zNetworkServiceProviderListVec *const list = g_zNetwork_ServiceProviderList;
    if (list->end == list->cap) {
        const int count = list->begin != 0 ? (int)(list->end - list->begin) : 0;
        const int newCapacity = count <= 1 ? count + 1 : count * 2;
        zNetworkDPlayServiceProviderInfo **const newBegin =
            (zNetworkDPlayServiceProviderInfo **)(::operator new(
                sizeof(zNetworkDPlayServiceProviderInfo *) * newCapacity
            ));

        int index;
        for (index = 0; index < count; ++index) {
            newBegin[index] = list->begin[index];
        }

        newBegin[count] = info;
        ::operator delete(list->begin);
        list->begin = newBegin;
        list->end = newBegin + count + 1;
        list->cap = newBegin + newCapacity;
        return;
    }

    *list->end = info;
    ++list->end;
}
} // namespace

/**
 * Reimplements 0x489f70: zNetwork_GetLocalPlayerKey.
 * Original source path: D:\Proj\Battlesport\zNetwork\zNetwork.cpp.
 * Purpose: Return the cached DirectPlay local-player key.
 */
extern "C" int zNetwork_GetLocalPlayerKey() {
    return g_zNetwork_LocalPlayerKey;
}

/**
 * Reimplements 0x48b980: zNetwork_GetLocalPlayerColorIndex.
 * Original source path: D:\Proj\GameZRecoil\zNetwork.cpp.
 * Purpose: return the local player record's assigned color index.
 */
extern "C" int zNetwork_GetLocalPlayerColorIndex() {
    if (g_zNetwork_LocalPlayerRecord == 0) {
        return 0;
    }

    return g_zNetwork_LocalPlayerRecord->colorIndex;
}

/**
 * Reimplements 0x48b9a0: zNetwork_GetPlayerColorIndexByKey.
 * Original source path: D:\Proj\GameZRecoil\zNetwork.cpp.
 * Purpose: look up a player color index and reject values outside the session
 * player range.
 */
extern "C" int __fastcall zNetwork_GetPlayerColorIndexByKey(
    int playerKey
) {
    zNetwork_PlayerRecord *const playerRecord = zNetwork_FindPlayerRecordByKey(playerKey);
    if (playerRecord == 0 || g_zNetwork_CurrentSessionDescCache == 0) {
        return 0;
    }

    const int colorIndex = playerRecord->colorIndex;
    if (colorIndex < 1 || (unsigned int)(colorIndex) >
                              (unsigned int)(g_zNetwork_CurrentSessionDescCache->desc.dwMaxPlayers)) {
        return 0;
    }

    return colorIndex;
}

/**
 * Reimplements 0x48b9d0: zNetwork_GetPlayerRecordCount.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\zNetwork.cpp.
 * Purpose: return the current DirectPlay player-record list count.
 */
extern "C" int zNetwork_GetPlayerRecordCount() {
    return g_zNetwork_PlayerRecordList->count;
}

/**
 * Reimplements 0x48bab0: zNetwork_ExtractStatusFieldsFromSessionDesc.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\zNetwork.cpp.
 * Purpose: copy session status fields from the current DirectPlay descriptor.
 */
extern "C" int __fastcall zNetwork_ExtractStatusFieldsFromSessionDesc(
    zNetworkSessionDescStatusFields *outFields
) {
    const zNetworkDPlaySessionDesc sessionDesc = g_zNetwork_CurrentSessionDescCache->desc;

    outFields->eventCode = sessionDesc.dwUser1;
    outFields->statusFlags = sessionDesc.dwUser2;
    outFields->valueOrTime = sessionDesc.dwUser3;
    outFields->auxParam = sessionDesc.dwUser4;
    outFields->maxPlayers = sessionDesc.dwMaxPlayers;
    outFields->selectedSessionIndex = -1;

    const size_t sessionNameBytes = strlen(sessionDesc.lpszSessionNameA) + 1;
    memcpy(
        outFields->sessionNameBuf,
        sessionDesc.lpszSessionNameA,
        sessionNameBytes
    );
    return 1;
}

/**
 * Reimplements 0x48bb20: zNetwork_ApplyStatusFieldsToSessionDesc.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\zNetwork.cpp.
 * Purpose: apply status fields to the current DirectPlay session descriptor.
 */
extern "C" int __fastcall zNetwork_ApplyStatusFieldsToSessionDesc(
    zNetworkSessionDescStatusFields *statusFields
) {
    zNetworkDPlaySessionDesc *const sessionDesc = &g_zNetwork_CurrentSessionDescCache->desc;

    sessionDesc->dwUser1 = statusFields->eventCode;
    sessionDesc->dwUser2 = statusFields->statusFlags;
    sessionDesc->dwUser3 = statusFields->valueOrTime;
    sessionDesc->dwUser4 = statusFields->auxParam;
    sessionDesc->dwMaxPlayers = statusFields->maxPlayers;

    memcpy(
        sessionDesc->lpszSessionNameA,
        statusFields->sessionNameBuf,
        strlen(statusFields->sessionNameBuf) + 1
    );

    const int hresult =
        g_zNetwork_pDirectPlay4->SetSessionDesc((LPDPSESSIONDESC2)sessionDesc, 0);
    if (hresult < 0) {
        return 0;
    }

    memcpy(
        g_zNetwork_SessionNameCache,
        sessionDesc->lpszSessionNameA,
        strlen(sessionDesc->lpszSessionNameA) + 1
    );
    return 1;
}

namespace zNetwork {
/**
 * Reimplements 0x489f80: zNetwork::IsHost.
 * Original source path: D:\Proj\Battlesport\zNetwork\zNetwork.cpp.
 * Purpose: return the cached local-host flag.
 */
int IsHost() {
    return g_zNetwork_IsHostFlag;
}

/**
 * Reimplements 0x48afa0: zNetwork::GetPlayerNameByKey.
 * Purpose: copy a player name from the player-record list by DirectPlay key.
 */
int __fastcall GetPlayerNameByKey(
    int playerKey,
    char *destination,
    unsigned int maxCount
) {
    zNetwork_PlayerRecord *const playerRecord = zNetwork_FindPlayerRecordByKey(playerKey);
    if (playerRecord == 0 || playerRecord->playerName == 0) {
        return 0;
    }

    strncpy(
        destination,
        playerRecord->playerName,
        maxCount
    );
    return 1;
}
} // namespace zNetwork

/**
 * Reimplements 0x48acf0: zNetwork_DPlay_SendUnreliable.
 * Original source path: GameZRecoil/zNetwork/znet_dplay.cpp.
 * Purpose: send a packet through DirectPlay without reliable delivery flags.
 */
extern "C" int __fastcall zNetwork_DPlay_SendUnreliable(
    zNetworkPacketHeader *packet,
    unsigned int packetSizeBytes
) {
    const int hresult = g_zNetwork_pDirectPlay4->Send(
        g_zNetwork_LocalPlayerRecord->playerKey,
        0,
        0,
        packet,
        packetSizeBytes
    );
    if (hresult != kDPlayPending && hresult < 0) {
        return zNetwork_DPlay_ReportError(
            hresult,
            g_zNetwork_SourceFile_ZnetDplayCpp,
            0x226
        );
    }

    return 0;
}

/**
 * Reimplements 0x48ad30: zNetwork_DPlay_SendReliable.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp.
 * Purpose: send a packet through DirectPlay with reliable delivery.
 */
extern "C" int __fastcall zNetwork_DPlay_SendReliable(
    zNetworkPacketHeader *packet,
    unsigned int packetSizeBytes
) {
    const int hresult = g_zNetwork_pDirectPlay4->Send(
        g_zNetwork_LocalPlayerRecord->playerKey,
        0,
        1,
        packet,
        packetSizeBytes
    );
    if (hresult != 0 && hresult < 0) {
        return zNetwork_DPlay_ReportError(
            hresult,
            g_zNetwork_SourceFile_ZnetDplayCpp,
            0x234
        );
    }

    return 0;
}

/**
 * Reimplements 0x48ad70: zNetwork_DPlay_SendExUnreliableTracked.
 * Original source path: GameZRecoil/zNetwork/znet_dplay.cpp.
 * Purpose: send an asynchronous unreliable packet and track the DirectPlay
 * message handle for packet type 6.
 */
extern "C" int __fastcall zNetwork_DPlay_SendExUnreliableTracked(
    zNetworkPacketHeader *packet,
    unsigned int packetSizeBytes
) {
    unsigned int flags = 0x600;
    if (packet->packetType == 6) {
        flags = 0x200;
        if (g_zNetwork_LastSendExCompleted == 0) {
            g_zNetwork_pDirectPlay4->CancelMessage(
                g_zNetwork_LastSendExHandle,
                0
            );
        } else {
            g_zNetwork_LastSendExCompleted = 0;
        }
    }

    const int hresult = g_zNetwork_pDirectPlay4->SendEx(
        g_zNetwork_LocalPlayerRecord->playerKey,
        0,
        flags,
        packet,
        packetSizeBytes,
        0,
        0,
        0,
        (LPDWORD)&g_zNetwork_LastSendExHandle
    );
    if (hresult != kDPlayPending && hresult < 0) {
        return zNetwork_DPlay_ReportError(
            hresult,
            g_zNetwork_SourceFile_ZnetDplayCpp,
            0x25a
        );
    }

    return 0;
}

/**
 * Reimplements 0x48ae10: zNetwork_DPlay_SendExReliable.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp.
 * Purpose: send an asynchronous reliable packet through DirectPlay.
 */
extern "C" int __fastcall zNetwork_DPlay_SendExReliable(
    zNetworkPacketHeader *packet,
    unsigned int packetSizeBytes
) {
    unsigned int asyncHandle = 0;
    const int hresult = g_zNetwork_pDirectPlay4->SendEx(
        g_zNetwork_LocalPlayerRecord->playerKey,
        0,
        0x601,
        packet,
        packetSizeBytes,
        0,
        0,
        0,
        (LPDWORD)&asyncHandle
    );
    if (hresult != kDPlayPending && hresult < 0) {
        return zNetwork_DPlay_ReportError(
            hresult,
            g_zNetwork_SourceFile_ZnetDplayCpp,
            0x26f
        );
    }

    return 0;
}

/**
 * Reimplements 0x48c060: zNetwork_SendPacketUnreliable.
 * Original source path: GameZRecoil/zNetwork/znet_dplay.cpp.
 * Purpose: route an unreliable packet to the sync or async DirectPlay send path.
 */
extern "C" int __fastcall zNetwork_SendPacketUnreliable(
    zNetworkPacketHeader *packet
) {
    const unsigned int packetSizeBytes = (unsigned short)(packet->packetSizeBytes);
    if (g_zNetwork_TcpIpAsyncSendEnabled != 0) {
        return zNetwork_DPlay_SendExUnreliableTracked(
            packet,
            packetSizeBytes
        );
    }

    return zNetwork_DPlay_SendUnreliable(
        packet,
        packetSizeBytes
    );
}

/**
 * Reimplements 0x48c080: zNetwork_SendPacketReliable.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp.
 * Purpose: route a reliable packet to the sync or async DirectPlay send path.
 */
extern "C" int __fastcall zNetwork_SendPacketReliable(
    zNetworkPacketHeader *packet
) {
    const unsigned int packetSizeBytes = (unsigned short)(packet->packetSizeBytes);
    if (g_zNetwork_TcpIpAsyncSendEnabled != 0) {
        return zNetwork_DPlay_SendExReliable(
            packet,
            packetSizeBytes
        );
    }

    return zNetwork_DPlay_SendReliable(
        packet,
        packetSizeBytes
    );
}

/**
 * Reimplements 0x48c250: zNetwork_DPlay_ReportError.
 * Purpose: report a DirectPlay HRESULT with the original inline error-name
 * comparisons and message format.
 */
extern "C" RECOIL_NO_GS int __fastcall zNetwork_DPlay_ReportError(
    int hresult,
    const char *sourceFile,
    int sourceLine
) {
    if (hresult == 0) {
        return 1;
    }

    const char *errorName = g_Player_MasterTypeName_Unknown;
    switch (hresult) {
    case (int)(0x8000000a) :
        errorName = g_zNetwork_DpErrorName_Pending;
        break;
    case (int)(0x80004001) :
        errorName = g_zNetwork_DpErrorName_Unsupported;
        break;
    case (int)(0x80004005) :
        errorName = g_zNetwork_DpErrorName_Generic;
        break;
    case (int)(0x8007000e) :
        errorName = g_zNetwork_DpErrorName_OutOfMemory;
        break;
    case (int)(0x80070057) :
        errorName = g_zNetwork_DpErrorName_InvalidParams;
        break;
    case (int)(0x88770005) :
        errorName = g_zNetwork_DpErrorName_AlreadyInitialized;
        break;
    case (int)(0x8877000a) :
        errorName = g_zNetwork_DpErrorName_AccessDenied;
        break;
    case (int)(0x88770014) :
        errorName = g_zNetwork_DpErrorName_ActivePlayers;
        break;
    case (int)(0x8877001e) :
        errorName = g_zNetwork_DpErrorName_BufferTooSmall;
        break;
    case (int)(0x88770028) :
        errorName = g_zNetwork_DpErrorName_CantAddPlayer;
        break;
    case (int)(0x88770032) :
        errorName = g_zNetwork_DpErrorName_CantCreateGroup;
        break;
    case (int)(0x8877003c) :
        errorName = g_zNetwork_DpErrorName_CantCreatePlayer;
        break;
    case (int)(0x88770046) :
        errorName = g_zNetwork_DpErrorName_CantCreateSession;
        break;
    case (int)(0x88770050) :
        errorName = g_zNetwork_DpErrorName_CapsNotAvailableYet;
        break;
    case (int)(0x8877005a) :
        errorName = g_zNetwork_DpErrorName_Exception;
        break;
    case (int)(0x88770078) :
        errorName = g_zNetwork_DpErrorName_InvalidFlags;
        break;
    case (int)(0x88770082) :
        errorName = g_zNetwork_DpErrorName_InvalidObject;
        break;
    case (int)(0x88770096) :
        errorName = g_zNetwork_DpErrorName_InvalidPlayer;
        break;
    case (int)(0x8877009b) :
        errorName = g_zNetwork_DpErrorName_InvalidGroup;
        break;
    case (int)(0x887700a0) :
        errorName = g_zNetwork_DpErrorName_NoCaps;
        break;
    case (int)(0x887700aa) :
        errorName = g_zNetwork_DpErrorName_NoConnection;
        break;
    case (int)(0x887700be) :
        errorName = g_zNetwork_DpErrorName_NoMessages;
        break;
    case (int)(0x887700c8) :
        errorName = g_zNetwork_DpErrorName_NoNameServerFound;
        break;
    case (int)(0x887700d2) :
        errorName = g_zNetwork_DpErrorName_NoPlayers;
        break;
    case (int)(0x887700dc) :
        errorName = g_zNetwork_DpErrorName_NoSessions;
        break;
    case (int)(0x887700e6) :
        errorName = g_zNetwork_DpErrorName_SendTooBig;
        break;
    case (int)(0x887700f0) :
        errorName = g_zNetwork_DpErrorName_Timeout;
        break;
    case (int)(0x887700fa) :
        errorName = g_zNetwork_DpErrorName_Unavailable;
        break;
    case (int)(0x8877010e) :
        errorName = g_zNetwork_DpErrorName_Busy;
        break;
    case (int)(0x88770118) :
        errorName = g_zNetwork_DpErrorName_UserCancel;
        break;
    case (int)(0x88770122) :
        errorName = g_zNetwork_DpErrorName_CannotCreateServer;
        break;
    case (int)(0x8877012c) :
        errorName = g_zNetwork_DpErrorName_PlayerLost;
        break;
    case (int)(0x88770136) :
        errorName = g_zNetwork_DpErrorName_SessionLost;
        break;
    case (int)(0x88770140) :
        errorName = g_zNetwork_DpErrorName_Uninitialized;
        break;
    case (int)(0x8877014a) :
        errorName = g_zNetwork_DpErrorName_NoNewPlayers;
        break;
    case (int)(0x8877015e) :
        errorName = g_zNetwork_DpErrorName_Connecting;
        break;
    case (int)(0x88770168) :
        errorName = g_zNetwork_DpErrorName_ConnectionLost;
        break;
    case (int)(0x88770172) :
        errorName = g_zNetwork_DpErrorName_UnknownMessage;
        break;
    case (int)(0x8877017c) :
        errorName = g_zNetwork_DpErrorName_CancelFailed;
        break;
    case (int)(0x88770186) :
        errorName = g_zNetwork_DpErrorName_InvalidPriority;
        break;
    case (int)(0x887703e8) :
        errorName = g_zNetwork_DpErrorName_BufferTooLarge;
        break;
    case (int)(0x887703f2) :
        errorName = g_zNetwork_DpErrorName_CantCreateProcess;
        break;
    case (int)(0x887703fc) :
        errorName = g_zNetwork_DpErrorName_AppNotStarted;
        break;
    case (int)(0x88770406) :
        errorName = g_zNetwork_DpErrorName_InvalidInterface;
        break;
    case (int)(0x8877041a) :
        errorName = g_zNetwork_DpErrorName_UnknownApplication;
        break;
    case (int)(0x8877042e) :
        errorName = g_zNetwork_DpErrorName_NotLobbied;
        break;
    case (int)(0x887707d0) :
        errorName = g_zNetwork_DpErrorName_AuthenticationFailed;
        break;
    case (int)(0x887707da) :
        errorName = g_zNetwork_DpErrorName_CantLoadSspi;
        break;
    case (int)(0x887707e4) :
        errorName = g_zNetwork_DpErrorName_EncryptionFailed;
        break;
    case (int)(0x887707ee) :
        errorName = g_zNetwork_DpErrorName_SignFailed;
        break;
    case (int)(0x887707f8) :
        errorName = g_zNetwork_DpErrorName_CantLoadSecurityPackage;
        break;
    case (int)(0x88770802) :
        errorName = g_zNetwork_DpErrorName_EncryptionNotSupported;
        break;
    case (int)(0x8877080c) :
        errorName = g_zNetwork_DpErrorName_CantLoadCapi;
        break;
    case (int)(0x88770816) :
        errorName = g_zNetwork_DpErrorName_NotLoggedIn;
        break;
    case (int)(0x88770820) :
        errorName = g_zNetwork_DpErrorName_LogonDenied;
        break;
    }

    char errorNameBuffer[0x100];
    sprintf(
        errorNameBuffer,
        errorName
    );
    zError::ReportOld(
        0x400,
        sourceFile,
        sourceLine,
        g_zNetwork_DirectPlayErrorFmt,
        hresult,
        errorNameBuffer
    );
    return 0;
}

/**
 * Reimplements 0x48a980: zNetwork_DPlay_DestroyCachedLocalPlayer.
 * Purpose: destroy the cached local DirectPlay player if one is registered.
 */
extern "C" int zNetwork_DPlay_DestroyCachedLocalPlayer() {
    zNetwork_PlayerRecord *localPlayer = g_zNetwork_LocalPlayerRecord;
    if (localPlayer == 0) {
        return 0;
    }

    zNetwork_DPlay4 *directPlay = g_zNetwork_pDirectPlay4;
    const int hresult = directPlay->DestroyPlayer(localPlayer->playerKey);
    if (hresult < 0) {
        return zNetwork_DPlay_ReportError(
            hresult,
            g_zNetwork_SourceFile_ZnetDplayCpp,
            0x1ba
        );
    }

    return 1;
}

/**
 * Reimplements 0x48ba60: zNetwork_FindPlayerRecordByKey.
 * Purpose: find a player record in the runtime player list by DirectPlay
 * player key.
 */
extern "C" zNetwork_PlayerRecord *__fastcall zNetwork_FindPlayerRecordByKey(
    int playerKey
) {
    zNetworkPlayerRecordList *const list = g_zNetwork_PlayerRecordList;
    zNetworkPlayerRecordListNode *const sentinel = list->sentinelNode;
    for (zNetworkPlayerRecordListNode *node = sentinel->next; node != sentinel; node = node->next) {
        zNetwork_PlayerRecord *const playerRecord = node->playerRecord;
        if (playerRecord->playerKey == (unsigned int)(playerKey)) {
            return playerRecord;
        }
    }

    return 0;
}

namespace zNetwork {
/**
 * Reimplements 0x48b940: zNetwork::AllocFreePlayerColorIndex.
 * Purpose: reserve and return the first unused player color index.
 */
int AllocFreePlayerColorIndex() {
    const int maxPlayers = g_zNetwork_CurrentSessionDescCache->desc.dwMaxPlayers;
    for (int colorIndex = 1; (unsigned int)(colorIndex) <= (unsigned int)(maxPlayers);
        ++colorIndex) {
        if (g_zNetwork_PlayerColorInUseFlags[colorIndex] == 0) {
            g_zNetwork_PlayerColorInUseFlags[colorIndex] = 1;
            return colorIndex;
        }
    }

    return 0;
}

/**
 * Reimplements 0x48b860: zNetwork::HostSendPlayerColorAssignmentsPacket.
 * Purpose: host-build and send the player color-assignment packet.
 */
void __fastcall HostSendPlayerColorAssignmentsPacket(
    int joiningPlayerKey
) {
    if (IsHost() == 0) {
        return;
    }

    zNetwork_PlayerRecord *const joiningPlayer = zNetwork_FindPlayerRecordByKey(joiningPlayerKey);
    if (joiningPlayer == 0) {
        return;
    }

    if (joiningPlayer->colorIndex <= 0) {
        joiningPlayer->colorIndex = AllocFreePlayerColorIndex();
    }

    const int playerCount = zNetwork_GetPlayerRecordCount();
    const int packetSizeBytes = (int)(sizeof(zNetworkPacketHeader) + sizeof(int) +
                                      (sizeof(zNetworkPlayerColorPair) * playerCount));
    NetPkt01_PlayerColorAssignments *const packet =
        (NetPkt01_PlayerColorAssignments *)(malloc(packetSizeBytes));
    memset(
        packet,
        0,
        packetSizeBytes
    );
    packet->header.packetType = 1;
    packet->header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    packet->header.packetSizeBytes = (short)(packetSizeBytes);
    packet->pairCount = playerCount;

    zNetworkPlayerRecordList *const list = g_zNetwork_PlayerRecordList;
    zNetworkPlayerRecordListNode *const sentinel = list->sentinelNode;
    int pairIndex = 0;
    for (zNetworkPlayerRecordListNode *node = sentinel->next; node != sentinel; node = node->next) {
        zNetwork_PlayerRecord *const playerRecord = node->playerRecord;
        packet->pairs[pairIndex].playerKey = (int)(playerRecord->playerKey);
        packet->pairs[pairIndex].colorIndex = playerRecord->colorIndex;
        ++pairIndex;
    }

    zNetwork_SendPacketReliable(&packet->header);
    free(packet);
}
} // namespace zNetwork

namespace zNetworkDPlay {
/**
 * Reimplements 0x48b3a0: zNetworkDPlay::EnumConnectionsCallback_AddServiceProviderInfo.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp.
 * Purpose: copy an enumerated DirectPlay provider record into the provider list.
 */
int __stdcall EnumConnectionsCallback_AddServiceProviderInfo(
    const GUID *serviceProviderGuid,
    void *connectionData,
    DWORD connectionDataSize,
    const zNetworkDPlayName *providerName,
    DWORD providerFlags,
    void *
) {
    zNetworkDPlayServiceProviderInfo *providerInfo =
        (zNetworkDPlayServiceProviderInfo *)(::operator new(
            sizeof(zNetworkDPlayServiceProviderInfo)
        ));
    if (providerInfo != 0) {
        memcpy(
            &providerInfo->serviceProviderGuid,
            serviceProviderGuid,
            sizeof(providerInfo->serviceProviderGuid)
        );
        providerInfo->displayName = _strdup(providerName->lpszShortNameA);
        providerInfo->connectionData = calloc(
            connectionDataSize,
            1
        );
        memcpy(
            providerInfo->connectionData,
            connectionData,
            connectionDataSize
        );
        providerInfo->providerFlags = (int)(providerFlags);
    }

    AppendServiceProviderInfo(providerInfo);
    return TRUE;
}

} // namespace zNetworkDPlay

namespace zNetwork_DPlay {
/**
 * Reimplements 0x48a0d0: zNetwork_DPlay::RefreshServiceProviderList.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp.
 * Purpose: clear and rebuild the DirectPlay service-provider list.
 */
int RefreshServiceProviderList() {
    zNetwork::ClearServiceProviderList();

    zNetwork_DPlay4 *const directPlay = g_zNetwork_pDirectPlay4;
    const int hresult = directPlay->EnumConnections(
        g_zNetwork_AppGuid,
        (LPDPENUMCONNECTIONSCALLBACK)
            zNetworkDPlay::EnumConnectionsCallback_AddServiceProviderInfo,
        g_RecoilApp_hWndMain,
        0
    );
    if (hresult < 0) {
        return zNetwork_DPlay_ReportError(
            hresult,
            g_zNetwork_SourceFile_ZnetDplayCpp,
            0x6b
        );
    }

    zNetworkServiceProviderListVec *const list = g_zNetwork_ServiceProviderList;
    if (list->begin == 0) {
        return 0;
    }

    return (int)(list->end - list->begin);
}
} // namespace zNetwork_DPlay

namespace zNetworkDPlay {
/**
 * Reimplements 0x48a130: zNetworkDPlay::RefreshAndGetServiceProviderList.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp.
 * Purpose: refresh DirectPlay service providers and return the provider vector.
 */
zNetworkServiceProviderListVec *RefreshAndGetServiceProviderList() {
    zNetwork_DPlay::RefreshServiceProviderList();
    return g_zNetwork_ServiceProviderList;
}

/**
 * Reimplements 0x48a180: zNetworkDPlay::SelectServiceProviderAndInitConnection.
 * Purpose: switch to an enumerated DirectPlay provider and initialize it.
 */
int __fastcall SelectServiceProviderAndInitConnection(
    zNetworkDPlayServiceProviderInfo *providerInfo
) {
    zNetwork_DPlay::CloseReleaseAndCoUninitialize(g_zNetwork_pDirectPlay4);
    g_zNetwork_pDirectPlay4 = 0;

    if (providerInfo->connectionData == 0) {
        return 0;
    }

    const int hresult = zNetwork_DPlay::CreateInterfaceAndCoInitialize(&g_zNetwork_pDirectPlay4);
    if (hresult < 0) {
        return zNetwork_DPlay_ReportError(
            hresult,
            g_zNetwork_SourceFile_ZnetDplayCpp,
            0x8f
        );
    }

    if (g_zNetwork_pDirectPlay4 == 0) {
        return 0;
    }

    g_zNetwork_ActiveProviderIsModem = strstr(
        providerInfo->displayName,
        g_zNetwork_ProviderName_Modem
    ) != 0;
    g_zNetwork_ActiveProviderIsTcpIp = strstr(
        providerInfo->displayName,
        g_zNetwork_ProviderName_TcpIp
    ) != 0;
    g_zNetwork_TcpIpAsyncSendEnabled = g_zNetwork_ActiveProviderIsTcpIp;

    return InitializeConnectionFromProviderInfo(providerInfo);
}

/**
 * Reimplements 0x48a140: zNetworkDPlay::InitializeConnectionFromProviderInfo.
 * Purpose: pass provider connection data to DirectPlay and report failures.
 */
int __fastcall InitializeConnectionFromProviderInfo(
    zNetworkDPlayServiceProviderInfo *providerInfo
) {
    const int kDPlayUserCancel = (int)(0x88770118);
    zNetwork_DPlay4 *const directPlay = g_zNetwork_pDirectPlay4;
    const int hresult = directPlay->InitializeConnection(
        providerInfo->connectionData,
        0
    );

    if (hresult >= 0) {
        return 1;
    }

    if (hresult == kDPlayUserCancel) {
        return 0;
    }

    return zNetwork_DPlay_ReportError(
        hresult,
        g_zNetwork_SourceFile_ZnetDplayCpp,
        0x7d
    );
}

/**
 * Reimplements 0x48a2c0: zNetworkDPlay::GetEnumeratedSessionNameByIndex.
 * Purpose: return the cached session name for an enumerated session index.
 */
char *__fastcall GetEnumeratedSessionNameByIndex(
    int entryIndex
) {
    zNetworkDPlaySessionDescCache *const entry = (zNetworkDPlaySessionDescCache
            *)(zArchiveList_GetAt(
                g_zNetwork_EnumeratedSessionList,
                entryIndex
            ));
    if (entry == 0) {
        return 0;
    }

    return entry->desc.lpszSessionNameA;
}

/**
 * Reimplements 0x48a2e0: zNetworkDPlay::GetEnumeratedSessionPlayerCountsByIndex.
 * Purpose: return current and maximum player counts for an enumerated session.
 */
void __fastcall GetEnumeratedSessionPlayerCountsByIndex(
    int entryIndex,
    int *currentPlayersOut,
    int *maxPlayersOut
) {
    zNetworkDPlaySessionDescCache *const entry = (zNetworkDPlaySessionDescCache
            *)(zArchiveList_GetAt(
                g_zNetwork_EnumeratedSessionList,
                entryIndex
            ));
    if (entry != 0) {
        *maxPlayersOut = entry->desc.dwMaxPlayers;
        *currentPlayersOut = entry->desc.dwCurrentPlayers;
    }
}

/**
 * Reimplements 0x48b5e0: zNetworkDPlay::EnumSessionCallback_AddSessionDescCache.
 * Purpose: cache a DirectPlay session descriptor during session enumeration.
 */
int __stdcall EnumSessionCallback_AddSessionDescCache(
    const zNetworkDPlaySessionDesc *sessionDesc,
    DWORD *,
    DWORD,
    void *
) {
    if (sessionDesc == 0) {
        return 0;
    }

    zNetworkDPlaySessionDescCache *const cache =
        (zNetworkDPlaySessionDescCache *)(malloc(sizeof(zNetworkDPlaySessionDescCache)));
    memcpy(
        &cache->desc,
        sessionDesc,
        sizeof(zNetworkDPlaySessionDesc)
    );
    cache->desc.lpszSessionNameA = _strdup(sessionDesc->lpszSessionNameA);
    zArchiveList_PushBackPayload(
        g_zNetwork_EnumeratedSessionList,
        cache
    );
    return 1;
}

/**
 * Reimplements 0x48a350: zNetworkDPlay::QueryCapsAndConfigureSendMode.
 * Purpose: query DirectPlay capabilities and select the TCP/IP synchronous or
 * asynchronous send path based on provider flags.
 */
int QueryCapsAndConfigureSendMode() {
    zNetwork_DPlay4 *const directPlay = g_zNetwork_pDirectPlay4;
    g_zNetwork_DPlayCaps.dwSize = sizeof(zNetworkDPlayCaps);
    const int hresult = directPlay->GetCaps((LPDPCAPS)&g_zNetwork_DPlayCaps, 1);
    if (hresult < 0) {
        fprintf(
            stderr,
            g_zNetwork_GetCapabilitiesFailedMsg
        );
        return zNetwork_DPlay_ReportError(
            hresult,
            g_zNetwork_SourceFile_ZnetDplayCpp,
            0xea
        );
    }

    if (g_zNetwork_ActiveProviderIsTcpIp != 0) {
        const int flags = g_zNetwork_DPlayCaps.dwFlags;
        if ((flags & 0x40) == 0) {
            fprintf(
                stderr,
                g_zNetwork_GuaranteedTcpIpNotSupportedMsg
            );
            return 0;
        }

        if ((flags & 0x10000) == 0) {
            g_zNetwork_TcpIpAsyncSendEnabled = 0;
        }

        printf(
            g_zNetwork_UsingTcpIpFmt,
            g_zNetwork_TcpIpAsyncSendEnabled != 0 ?
                g_zNetwork_AsyncModeName :
                g_zNetwork_SyncModeName
        );
    }

    return 1;
}

/**
 * Reimplements 0x48afe0: zNetworkDPlay::PumpIncomingMessages.
 * Purpose: handle DirectPlay system messages and dispatch synthesized packets.
 */
int __fastcall PumpIncomingMessages(
    zNetworkDPlaySystemMessage *systemMessage
) {
    zNetworkPacketHeader packet;
    const int msgType = systemMessage->msgType;

    if (msgType == 0x21 || msgType == 7) {
        return 0;
    }

    if (msgType == 3) {
        zNetwork_PlayerRecord *playerRecord =
            (zNetwork_PlayerRecord *)(::operator new(sizeof(zNetwork_PlayerRecord)));
        if (playerRecord != 0) {
            strncpy(
                playerRecord->playerName,
                g_zNetwork_DefaultPlayerName,
                0x50
            );
            playerRecord->playerName[0x4f] = 0;
        }

        playerRecord->playerKey = systemMessage->fields.playerId;
        playerRecord->playerNameInfo.dwSize = systemMessage->fields.createFlagsOrPlayerType;
        playerRecord->playerNameInfo.dwFlags = systemMessage->fields.nameShortOrAsyncHandle;
        playerRecord->playerNameInfo.lpszShortNameA = systemMessage->fields.nameLong;
        playerRecord->playerNameInfo.lpszLongNameA = systemMessage->fields.nameDisplay;
        strcpy(
            playerRecord->playerName,
            playerRecord->playerNameInfo.lpszLongNameA
        );
        strcpy(
            playerRecord->altName,
            playerRecord->playerNameInfo.lpszShortNameA
        );
        playerRecord->colorIndex = 0;

        zNetworkPlayerRecordList *const list = g_zNetwork_PlayerRecordList;
        zNetworkPlayerRecordListNode *const sentinel = list->sentinelNode;
        zNetworkPlayerRecordListNode *prev = sentinel->prev;
        zNetworkPlayerRecordListNode *const node =
            (zNetworkPlayerRecordListNode *)(::operator new(sizeof(zNetworkPlayerRecordListNode)));
        node->next = sentinel != 0 ? sentinel : node;
        node->prev = prev != 0 ? prev : node;
        sentinel->prev = node;
        node->prev->next = node;
        node->playerRecord = playerRecord;
        ++list->count;

        ++g_zNetworkCurrentPlayerCountCached;
        packet.packetType = 2;
        packet.packetSizeBytes = 8;
        packet.payloadDword0 = 0;
        zNetwork_DPlay::DispatchPacketToHandlers(
            systemMessage->fields.playerId,
            &packet
        );
        if (zNetwork::IsHost() != 0) {
            zNetwork::HostSendPlayerColorAssignmentsPacket(systemMessage->fields.playerId);
        }

        return 0;
    }

    if (msgType == 5) {
        packet.packetType = 3;
        packet.packetSizeBytes = 8;
        packet.payloadDword0 = 0;
        zNetwork_DPlay::DispatchPacketToHandlers(
            systemMessage->fields.playerId,
            &packet
        );
        zNetwork::RemovePlayerRecordByKey(systemMessage->fields.playerId);
        --g_zNetworkCurrentPlayerCountCached;
        return 0;
    }

    if (msgType == 0x31) {
        if (g_zNetwork_FatalDisconnectCallback != 0) {
            g_zNetwork_FatalDisconnectCallback(-1);
        }
        g_zNetwork_FatalDisconnectTriggered = 1;
        return -1;
    }

    if (msgType == 0x101) {
        g_zNetwork_IsHostFlag = 1;
        return 0;
    }

    if (msgType == 0x102) {
        packet.packetType = 4;
        packet.packetSizeBytes = 8;
        packet.payloadDword0 = 0;
        zNetwork_DPlay::DispatchPacketToHandlers(
            systemMessage->fields.playerId,
            &packet
        );
        return 0;
    }

    if (msgType == 0x103) {
        packet.packetType = 5;
        packet.packetSizeBytes = 8;
        packet.payloadDword0 = 0;
        zNetwork_DPlay::DispatchPacketToHandlers(
            systemMessage->fields.playerId,
            &packet
        );
        return 0;
    }

    if (msgType == 0x104) {
        memcpy(
            &g_zNetwork_CurrentSessionDescCache->desc,
            systemMessage->payload_004,
            sizeof(zNetworkDPlaySessionDesc)
        );
        return 0;
    }

    if (msgType == 0x10d) {
        if (systemMessage->fields.nameShortOrAsyncHandle == g_zNetwork_LastSendExHandle) {
            g_zNetwork_LastSendExCompleted = 1;
        }
        return 0;
    }

    zError::ReportOld(
        0x200,
        g_zNetwork_SourceFile_ZnetDplayCpp,
        0x346,
        g_zNetwork_UnhandledDirectPlaySystemMessageMsg
    );
    return 0;
}
} // namespace zNetworkDPlay

namespace zNetwork {
/**
 * Reimplements 0x48b9e0: zNetwork::RemovePlayerRecordByKey.
 * Purpose: remove a player record by DirectPlay key and release its color slot.
 */
void __fastcall RemovePlayerRecordByKey(
    int playerKey
) {
    zNetwork_PlayerRecord *const playerRecord = zNetwork_FindPlayerRecordByKey(playerKey);
    if (playerRecord == 0) {
        return;
    }

    const int colorIndex = playerRecord->colorIndex;
    if (colorIndex > 0) {
        g_zNetwork_PlayerColorInUseFlags[colorIndex] = 0;
    }

    zNetworkPlayerRecordList *const list = g_zNetwork_PlayerRecordList;
    zNetworkPlayerRecordListNode *const sentinel = list->sentinelNode;
    zNetworkPlayerRecordListNode *node = sentinel->next;
    while (node != sentinel) {
        if (node->playerRecord == playerRecord) {
            zNetworkPlayerRecordListNode *const deleteNode = node;
            node = node->next;
            deleteNode->prev->next = deleteNode->next;
            deleteNode->next->prev = deleteNode->prev;
            ::operator delete(deleteNode);
            --list->count;
        } else {
            node = node->next;
        }
    }
}
} // namespace zNetwork

namespace zNetworkDPlay {
/**
 * Reimplements 0x48ae70: zNetworkDPlay::ReceivePendingMessages.
 * Purpose: receive pending DirectPlay messages, grow the receive buffer, and
 * dispatch player or system packets.
 */
int __fastcall ReceivePendingMessages(
    int messageBudget
) {
    if (g_zNetwork_SessionRuntimeInitialized == 0) {
        return 0;
    }

    if (g_zNetwork_FatalDisconnectTriggered != 0) {
        return -1;
    }

    int processedCount = 0;
    int pumpResult = 0;
    while (true) {
        unsigned int receiveBufferCapacity = g_zNetwork_ReceiveBufferCapacity;
        unsigned int fromPlayer = 0;
        unsigned int toPlayer = 0;
        const int hresult = g_zNetwork_pDirectPlay4->Receive(
            (LPDPID)&fromPlayer,
            (LPDPID)&toPlayer,
            1,
            g_zNetwork_ReceiveBuffer,
            (LPDWORD)&receiveBufferCapacity
        );

        if (hresult == kDPlayBufferTooSmall) {
            const unsigned int oldCapacity = g_zNetwork_ReceiveBufferCapacity;
            g_zNetwork_ReceiveBuffer = realloc(
                g_zNetwork_ReceiveBuffer,
                receiveBufferCapacity
            );
            zError::ReportOld(
                0x100,
                g_zNetwork_SourceFile_ZnetDplayCpp,
                0x299,
                g_zNetwork_ReceiveBufferIncreasedFmt,
                oldCapacity,
                receiveBufferCapacity
            );
            g_zNetwork_ReceiveBufferCapacity = receiveBufferCapacity;
            continue;
        }

        if (hresult < 0) {
            break;
        }

        if (receiveBufferCapacity >= 4) {
            --messageBudget;
            ++processedCount;
            if (fromPlayer != 0) {
                zNetwork_DPlay::DispatchPacketToHandlers(
                    (int)(fromPlayer),
                    (zNetworkPacketHeader *)(g_zNetwork_ReceiveBuffer)
                );
            } else {
                pumpResult =
                    PumpIncomingMessages((zNetworkDPlaySystemMessage *)(g_zNetwork_ReceiveBuffer));
            }
        }

        if (messageBudget == 0 || pumpResult != 0) {
            break;
        }
    }

    return processedCount;
}

/**
 * Reimplements 0x48b660: zNetworkDPlay::EnumPlayerCallback_AddPlayerRecord.
 * Purpose: append an enumerated DirectPlay player record if it is not cached.
 */
int __stdcall EnumPlayerCallback_AddPlayerRecord(
    DPID playerId,
    DWORD,
    const zNetworkDPlayName *playerNameInfo,
    DWORD,
    void *
) {
    if (zNetwork_FindPlayerRecordByKey((int)(playerId)) != 0) {
        return 1;
    }

    zNetwork_PlayerRecord *const playerRecord =
        (zNetwork_PlayerRecord *)(::operator new(sizeof(zNetwork_PlayerRecord)));
    strncpy(
        playerRecord->playerName,
        playerNameInfo->lpszShortNameA,
        0x50
    );
    playerRecord->playerName[0x4f] = 0;
    playerRecord->playerKey = playerId;

    zNetworkPlayerRecordList *const list = g_zNetwork_PlayerRecordList;
    zNetworkPlayerRecordListNode *const sentinel = list->sentinelNode;
    zNetworkPlayerRecordListNode *prev = sentinel->prev;
    zNetworkPlayerRecordListNode *const node =
        (zNetworkPlayerRecordListNode *)(::operator new(sizeof(zNetworkPlayerRecordListNode)));

    node->next = sentinel != 0 ? sentinel : node;
    if (prev == 0) {
        prev = node;
    }
    node->prev = prev;
    sentinel->prev = node;
    node->prev->next = node;
    node->playerRecord = playerRecord;
    ++list->count;
    return 1;
}
} // namespace zNetworkDPlay

namespace zNetwork_DPlay {
/**
 * Reimplements 0x48a220: zNetwork_DPlay::EnumSessions.
 * Purpose: enumerate current-app DirectPlay sessions into the session cache.
 */
int EnumSessions() {
    zNetwork::ClearEnumeratedSessionList();

    zNetwork_DPlay4 *const directPlay = g_zNetwork_pDirectPlay4;
    if (directPlay == 0) {
        return 0;
    }

    zNetworkDPlaySessionDesc desc;
    memset(
        &desc,
        0,
        sizeof(desc)
    );
    desc.dwSize = sizeof(zNetworkDPlaySessionDesc);
    desc.guidApplication = *g_zNetwork_AppGuid;

    const int hresult = directPlay->EnumSessions(
        (LPDPSESSIONDESC2)&desc,
        0,
        (LPDPENUMSESSIONSCALLBACK2)
            zNetworkDPlay::EnumSessionCallback_AddSessionDescCache,
        0,
        2
    );
    if (hresult == (int)(0x88770118)) {
        return -1;
    }

    if (hresult < 0) {
        return zNetwork_DPlay_ReportError(
            hresult,
            g_zNetwork_SourceFile_ZnetDplayCpp,
            0xb3
        );
    }

    return zArchiveList_GetCount(g_zNetwork_EnumeratedSessionList);
}

/**
 * Reimplements 0x48a310: zNetwork_DPlay::EnumPlayers.
 * Purpose: enumerate DirectPlay players into the recovered player-record list.
 */
int EnumPlayers() {
    zNetwork_DPlay4 *const directPlay = g_zNetwork_pDirectPlay4;
    const int hresult = directPlay->EnumPlayers(
        0,
        (LPDPENUMPLAYERSCALLBACK2)
            zNetworkDPlay::EnumPlayerCallback_AddPlayerRecord,
        0,
        0
    );
    if (hresult < 0) {
        return zNetwork_DPlay_ReportError(
            hresult,
            g_zNetwork_SourceFile_ZnetDplayCpp,
            0xd9
        );
    }

    return g_zNetwork_PlayerRecordList->count;
}

/**
 * Reimplements 0x48a9c0: zNetwork_DPlay::CreateLocalPlayerRecordAndRegister.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp.
 * Purpose: create the local player record, register it with DirectPlay, and
 * insert it into the player list.
 */
int __fastcall CreateLocalPlayerRecordAndRegister(
    char *playerName
) {
    zNetwork_PlayerRecord *const localPlayerRecord =
        (zNetwork_PlayerRecord *)(::operator new(sizeof(zNetwork_PlayerRecord)));
    if (localPlayerRecord != 0) {
        strncpy(
            localPlayerRecord->playerName,
            g_zNetwork_DefaultPlayerName,
            0x50
        );
        localPlayerRecord->playerName[0x4f] = 0;
    }

    EnumPlayers();
    g_zNetwork_LocalPlayerRecord = localPlayerRecord;

    memcpy(
        g_zNetwork_LocalPlayerNameScratch,
        playerName,
        strlen(playerName) + 1
    );
    localPlayerRecord->playerNameInfo.lpszShortNameA = g_zNetwork_LocalPlayerNameScratch;
    localPlayerRecord->playerNameInfo.lpszLongNameA = g_zNetwork_LocalPlayerNameScratch;
    localPlayerRecord->createPlayerEventHandle = 0;
    localPlayerRecord->playerNameInfo.dwSize = sizeof(zNetworkDPlayName);
    localPlayerRecord->playerNameInfo.dwFlags = 0;
    memcpy(
        localPlayerRecord->playerName,
        g_zNetwork_LocalPlayerNameScratch,
        strlen(g_zNetwork_LocalPlayerNameScratch) + 1
    );
    memcpy(
        localPlayerRecord->altName,
        localPlayerRecord->playerNameInfo.lpszShortNameA,
        strlen(localPlayerRecord->playerNameInfo.lpszShortNameA) + 1
    );

    zNetwork_DPlay4 *const directPlay = g_zNetwork_pDirectPlay4;
    const int createResult = directPlay->CreatePlayer(
        (LPDPID)&localPlayerRecord->playerKey,
        (LPDPNAME)&localPlayerRecord->playerNameInfo,
        (HANDLE)localPlayerRecord->createPlayerEventHandle,
        0,
        0,
        0
    );
    if (createResult < 0) {
        if (createResult == (int)(0x88770028)) {
            MessageBoxA(
                g_RecoilApp_hWndMain,
                g_zNetwork_CannotAddAnotherPlayerMsg,
                g_zNetwork_RecoilNetworkErrorMsg,
                MB_OK
            );
        } else if (createResult == (int)(0x80070057) || createResult == (int)(0x88770078)) {
            MessageBoxA(
                g_RecoilApp_hWndMain,
                g_zNetwork_InvalidPlayerParametersMsg,
                g_zNetwork_RecoilNetworkErrorMsg,
                MB_OK
            );
        } else if (createResult == (int)(0x8877003c)) {
            MessageBoxA(
                g_RecoilApp_hWndMain,
                g_zNetwork_CannotCreateAnotherPlayerMsg,
                g_zNetwork_RecoilNetworkErrorMsg,
                MB_OK
            );
        } else if (createResult == (int)(0x887700aa)) {
            MessageBoxA(
                g_RecoilApp_hWndMain,
                g_zNetwork_NoNetworkConnectionMsg,
                g_zNetwork_RecoilNetworkErrorMsg,
                MB_OK
            );
        } else if (createResult == (int)(0x88770168)) {
            MessageBoxA(
                g_RecoilApp_hWndMain,
                g_zNetwork_NetworkConnectionLostMsg,
                g_zNetwork_RecoilNetworkErrorMsg,
                MB_OK
            );
        }
        return 0;
    }

    memset(
        &localPlayerRecord->playerCaps,
        0,
        sizeof(zNetworkDPlayCaps)
    );
    localPlayerRecord->playerCaps.dwSize = sizeof(zNetworkDPlayCaps);
    const int capsResult = directPlay->GetPlayerCaps(
        localPlayerRecord->playerKey,
        (LPDPCAPS)&localPlayerRecord->playerCaps,
        0
    );
    g_zNetwork_IsHostFlag = localPlayerRecord->playerCaps.dwFlags & 2;
    zNetworkDPlay::ReceivePendingMessages(-1);
    if (capsResult < 0) {
        return zNetwork_DPlay_ReportError(
            capsResult,
            g_zNetwork_SourceFile_ZnetDplayCpp,
            0x20e
        );
    }

    zNetworkPlayerRecordList *const list = g_zNetwork_PlayerRecordList;
    g_zNetworkCurrentPlayerCountCached =
        g_zNetwork_CurrentSessionDescCache->desc.dwCurrentPlayers + 1;
    g_zNetwork_LocalPlayerKey = localPlayerRecord->playerKey;

    zNetworkPlayerRecordListNode *const sentinel = list->sentinelNode;
    zNetworkPlayerRecordListNode *prev = sentinel->prev;
    zNetworkPlayerRecordListNode *const node =
        (zNetworkPlayerRecordListNode *)(::operator new(sizeof(zNetworkPlayerRecordListNode)));
    node->next = sentinel != 0 ? sentinel : node;
    if (prev == 0) {
        prev = node;
    }
    node->prev = prev;
    sentinel->prev = node;
    node->prev->next = node;
    node->playerRecord = localPlayerRecord;
    ++list->count;

    if (zNetwork::IsHost() != 0) {
        localPlayerRecord->colorIndex = zNetwork::AllocFreePlayerColorIndex();
    } else {
        localPlayerRecord->colorIndex = 0;
    }

    return localPlayerRecord->playerKey;
}

/**
 * Reimplements 0x48a410: zNetwork_DPlay::CreateSessionFromStatusFields.
 * Purpose: create a DirectPlay host session from the recovered status-field
 * record and cache the opened session descriptor.
 */
int __fastcall CreateSessionFromStatusFields(
    zNetworkSessionDescStatusFields *statusFields
) {
    memcpy(
        g_zNetwork_SessionNameCache,
        statusFields->sessionNameBuf,
        strlen(statusFields->sessionNameBuf) + 1
    );

    zNetworkDPlaySessionDescCache *const cache =
        (zNetworkDPlaySessionDescCache *)(malloc(sizeof(zNetworkDPlaySessionDescCache)));
    memset(
        cache,
        0,
        sizeof(zNetworkDPlaySessionDescCache)
    );
    cache->desc.dwFlags = 0x44;
    cache->desc.dwSize = sizeof(zNetworkDPlaySessionDesc);
    cache->desc.guidApplication = *g_zNetwork_AppGuid;
    cache->desc.dwMaxPlayers = statusFields->maxPlayers;
    cache->desc.dwUser1 = statusFields->eventCode;
    cache->desc.dwUser2 = statusFields->statusFlags;
    cache->desc.dwUser3 = statusFields->valueOrTime;
    cache->desc.dwUser4 = statusFields->auxParam;
    cache->desc.lpszSessionNameA = _strdup(g_zNetwork_SessionNameCache);

    zNetwork_DPlay4 *const directPlay = g_zNetwork_pDirectPlay4;
    const int hresult = directPlay->Open(
        (LPDPSESSIONDESC2)&cache->desc,
        2
    );
    if (hresult == (int)(0x88770118)) {
        return 0;
    }

    if (hresult < 0) {
        return zNetwork_DPlay_ReportError(
            hresult,
            g_zNetwork_SourceFile_ZnetDplayCpp,
            0x11e
        );
    }

    if (zNetworkDPlay::QueryCapsAndConfigureSendMode() == 0) {
        directPlay->Close();
        return 0;
    }

    zNetworkDPlaySessionDescCache *const oldCache = g_zNetwork_CurrentSessionDescCache;
    if (oldCache != 0) {
        free(oldCache);
    }
    g_zNetwork_CurrentSessionDescCache = cache;
    return 1;
}
} // namespace zNetwork_DPlay

namespace zNetworkDPlay {
/**
 * Reimplements 0x48a520: zNetworkDPlay::OpenSelectedSessionAndReadStatusFields.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp.
 * Purpose: open an enumerated DirectPlay session and copy its status fields.
 */
int __fastcall OpenSelectedSessionAndReadStatusFields(
    zNetworkSessionDescStatusFields *statusFields
) {
    zNetworkDPlaySessionDescCache *const sessionCache = (zNetworkDPlaySessionDescCache *)
        zArchiveList_GetAt(
            g_zNetwork_EnumeratedSessionList,
            statusFields->selectedSessionIndex
        );
    g_zNetwork_CurrentSessionDescCache = sessionCache;
    if (sessionCache == 0) {
        return 0;
    }

    sessionCache->openMode = 1;
    sessionCache->desc.dwSize = sizeof(zNetworkDPlaySessionDesc);

    zNetwork_DPlay4 *const directPlay = g_zNetwork_pDirectPlay4;
    const int openResult = directPlay->Open(
        (LPDPSESSIONDESC2)&sessionCache->desc,
        1
    );
    if (openResult < 0) {
        return ReportDPlayOpenFailure(openResult);
    }

    if (QueryCapsAndConfigureSendMode() == 0) {
        directPlay->Close();
        return 0;
    }

    statusFields->eventCode = sessionCache->desc.dwUser1;
    statusFields->statusFlags = sessionCache->desc.dwUser2;
    statusFields->valueOrTime = sessionCache->desc.dwUser3;
    statusFields->auxParam = sessionCache->desc.dwUser4;
    statusFields->maxPlayers = sessionCache->desc.dwMaxPlayers;
    memcpy(
        statusFields->sessionNameBuf,
        sessionCache->desc.lpszSessionNameA,
        strlen(sessionCache->desc.lpszSessionNameA) + 1
    );
    return 1;
}
} // namespace zNetworkDPlay

namespace zNetwork {
/**
 * Reimplements 0x48bf40: zNetwork::DeleteAllDispatchHandlers.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\zNetwork.cpp.
 * Purpose: remove all packet-dispatch handler list nodes.
 */
void DeleteAllDispatchHandlers() {
    zNetworkDispatchHandlerListNode *const sentinel = g_zNetwork_DispatchHandlerListSentinel;
    zNetworkDispatchHandlerListNode *node = sentinel->next;
    while (node != sentinel) {
        zNetworkDispatchHandlerListNode *const next = node->next;
        node->prev->next = node->next;
        node->next->prev = node->prev;
        ::operator delete(node);
        --g_zNetwork_DispatchHandlerListCount;
        node = next;
    }
}
} // namespace zNetwork

namespace zNetworkDPlay {
/**
 * Reimplements 0x48bbe0: zNetworkDPlay::SelectTcpIpProviderAndEnumSessions.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp.
 * Purpose: build a forced TCP/IP DirectPlay address and optionally enumerate
 * sessions through it.
 */
int __fastcall SelectTcpIpProviderAndEnumSessions(
    char *addressString,
    int skipSessionEnumeration
) {
    DPCOMPOUNDADDRESSELEMENT elements[2];
    elements[0].guidDataType = DPAID_ServiceProvider;
    elements[0].dwDataSize = sizeof(DPSPGUID_TCPIP);
    elements[0].lpData = (void *)&DPSPGUID_TCPIP;
    elements[1].guidDataType = DPAID_INet;
    elements[1].dwDataSize = lstrlenA(addressString) + 1;
    elements[1].lpData = addressString;

    IDirectPlayLobby3A *lobby3A;
    CreateLobby3AInterface(&lobby3A);

    DWORD compoundAddressSize;
    lobby3A->CreateCompoundAddress(
        elements,
        2,
        0,
        &compoundAddressSize
    );
    void *const compoundAddress = malloc(compoundAddressSize);
    lobby3A->CreateCompoundAddress(
        elements,
        2,
        compoundAddress,
        &compoundAddressSize
    );

    zNetworkDPlayServiceProviderInfo providerInfo;
    providerInfo.serviceProviderGuid = DPSPGUID_TCPIP;
    providerInfo.displayName = _strdup(g_zNetwork_ForcedTcpIpModeName);
    providerInfo.connectionData = calloc(
        compoundAddressSize,
        1
    );
    memcpy(
        providerInfo.connectionData,
        compoundAddress,
        compoundAddressSize
    );
    providerInfo.providerFlags = 0;

    SelectServiceProviderAndInitConnection(&providerInfo);
    if (skipSessionEnumeration != 0) {
        free(providerInfo.displayName);
        providerInfo.displayName = 0;
        free(providerInfo.connectionData);
        return 1;
    }

    int enumResult;
    do {
        enumResult = EnumSessionsForCurrentApp();
    } while (enumResult == kDPlayConnecting);

    free(providerInfo.displayName);
    providerInfo.displayName = 0;
    free(providerInfo.connectionData);
    return enumResult == 0;
}

/**
 * Reimplements 0x48be10: zNetworkDPlay::CreateLobby3AInterface.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp.
 * Purpose: create a DirectPlayLobby interface and query IDirectPlayLobby3A.
 */
int __fastcall CreateLobby3AInterface(
    IDirectPlayLobby3A **outLobby3A
) {
    IDirectPlayLobby *lobby = 0;
    IDirectPlayLobby3A *lobby3A = 0;
    int result = DirectPlayLobbyCreateA(
        0,
        &lobby,
        0,
        0,
        0
    );
    if (result >= 0) {
        result = lobby->QueryInterface(
            IID_IDirectPlayLobby3A,
            (void **)&lobby3A
        );
        if (result >= 0) {
            lobby->Release();
            result = 0;
            *outLobby3A = lobby3A;
        }
    }

    return result;
}

/**
 * Reimplements 0x48be70: zNetworkDPlay::EnumSessionsForCurrentApp.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp.
 * Purpose: enumerate sessions for the configured application GUID.
 */
int EnumSessionsForCurrentApp() {
    zNetwork::ClearEnumeratedSessionList();

    zNetwork_DPlay4 *const directPlay = g_zNetwork_pDirectPlay4;
    if (directPlay == 0) {
        return 0;
    }

    zNetworkDPlaySessionDesc desc;
    memset(
        &desc,
        0,
        sizeof(desc)
    );
    desc.dwSize = sizeof(desc);
    desc.guidApplication = *g_zNetwork_AppGuid;
    return directPlay->EnumSessions(
        (LPDPSESSIONDESC2)&desc,
        0,
        (LPDPENUMSESSIONSCALLBACK2)
            zNetworkDPlay::EnumSessionCallback_AddSessionDescCache,
        0,
        0x82
    );
}
} // namespace zNetworkDPlay

/**
 * Reimplements 0x48bff0: zNetwork_DestroyDispatchHandlerList.
 * Purpose: delete the packet-dispatch handler sentinel and all list nodes.
 */
extern "C" void zNetwork_DestroyDispatchHandlerList() {
    zNetworkDispatchHandlerListNode *const sentinel = g_zNetwork_DispatchHandlerListSentinel;
    if (sentinel == 0) {
        g_zNetwork_DispatchHandlerListCount = 0;
        return;
    }

    zNetworkDispatchHandlerListNode *node = sentinel->next;
    while (node != sentinel) {
        zNetworkDispatchHandlerListNode *const next = node->next;
        node->prev->next = node->next;
        node->next->prev = node->prev;
        ::operator delete(node);
        --g_zNetwork_DispatchHandlerListCount;
        node = next;
    }

    ::operator delete(sentinel);
    g_zNetwork_DispatchHandlerListSentinel = 0;
    g_zNetwork_DispatchHandlerListCount = 0;
}

/**
 * Reimplements 0x48bfe0: zNetwork_RegisterDispatchHandlerListShutdown.
 * Purpose: register packet-dispatch handler list destruction with atexit.
 */
extern "C" void zNetwork_RegisterDispatchHandlerListShutdown() {
    atexit(zNetwork_DestroyDispatchHandlerList);
}

/**
 * Reimplements 0x48bfb0: zNetwork_CreateEmptyDispatchHandlerList.
 * Purpose: allocate and initialize an empty packet-dispatch handler list.
 */
extern "C" void zNetwork_CreateEmptyDispatchHandlerList() {
    g_zNetwork_DispatchHandlerListFlags = 0;
    zNetworkDispatchHandlerListNode *const sentinel =
        (zNetworkDispatchHandlerListNode *)(::operator new(
            sizeof(zNetworkDispatchHandlerListNode)
        ));
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
    g_zNetwork_DispatchHandlerListSentinel = sentinel;
    g_zNetwork_DispatchHandlerListCount = 0;
}

/**
 * Reimplements 0x48bfa0: zNetwork_InitMessageHandlers.
 * Purpose: initialize packet-dispatch handlers and register shutdown cleanup.
 */
extern "C" void zNetwork_InitMessageHandlers() {
    zNetwork_CreateEmptyDispatchHandlerList();
    zNetwork_RegisterDispatchHandlerListShutdown();
}

/**
 * Reimplements 0x48b820: zNetwork_ApplyPkt01_PlayerColorAssignments.
 * Purpose: apply host-provided player color assignments to matching player
 * records.
 */
extern "C" int __fastcall zNetwork_ApplyPkt01_PlayerColorAssignments(
    int,
    zNetworkPacketHeader *packet
) {
    NetPkt01_PlayerColorAssignments *assignments = (NetPkt01_PlayerColorAssignments *)(packet);
    const int assignmentCount = assignments->pairCount;
    for (int i = 0; i < assignmentCount; ++i) {
        const zNetworkPlayerColorPair &pair = assignments->pairs[i];
        zNetwork_PlayerRecord *const playerRecord = zNetwork_FindPlayerRecordByKey(pair.playerKey);
        if (playerRecord != 0) {
            playerRecord->colorIndex = pair.colorIndex;
            g_zNetwork_PlayerColorInUseFlags[pair.colorIndex] = 1;
        }
    }

    return assignmentCount;
}

namespace zNetwork_DPlay {
/**
 * Reimplements 0x48b730: zNetwork_DPlay::CreateInterfaceAndCoInitialize.
 * Purpose: initialize COM and create the DirectPlay4A interface.
 */
int __fastcall CreateInterfaceAndCoInitialize(
    zNetwork_DPlay4 **outDirectPlay4
) {
    const int kClassNotRegistered = (int)(0x80040154);
    const int kClassCannotBeCreated = (int)(0x80040110);
    const int kCoCreateNotInitialized = (int)(0x800401f0);

    zNetwork_DPlay4 *directPlay4 = 0;
    CoInitialize(0);
    const int hresult = CoCreateInstance(
        CLSID_DirectPlay,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IDirectPlay4A,
        (void **)&directPlay4
    );
    *outDirectPlay4 = directPlay4;

    if (hresult == kClassNotRegistered) {
        zError::ReportOld(
            0x400,
            g_zNetwork_SourceFile_ZnetDplayCpp,
            0x394,
            g_zNetwork_ClassNotRegisteredMsg
        );
    }

    if (hresult == kClassCannotBeCreated) {
        zError::ReportOld(
            0x400,
            g_zNetwork_SourceFile_ZnetDplayCpp,
            0x396,
            g_zNetwork_ClassCannotBeCreatedMsg
        );
    }

    if (hresult == kCoCreateNotInitialized) {
        zError::ReportOld(
            0x400,
            g_zNetwork_SourceFile_ZnetDplayCpp,
            0x398,
            g_zNetwork_CoCreateNotInitializedMsg
        );
        return hresult;
    }

    zNetwork_DPlay_ReportError(
        hresult,
        g_zNetwork_SourceFile_ZnetDplayCpp,
        0x39a
    );
    return hresult;
}

/**
 * Reimplements 0x48b7f0: zNetwork_DPlay::CloseReleaseAndCoUninitialize.
 * Purpose: close and release an optional DirectPlay interface before
 * uninitializing COM.
 */
int __fastcall CloseReleaseAndCoUninitialize(
    zNetwork_DPlay4 *directPlay4
) {
    int releaseRefCount = 0;
    if (directPlay4 != 0) {
        directPlay4->Close();
        releaseRefCount = directPlay4->Release();
    }

    CoUninitialize();
    return releaseRefCount;
}
} // namespace zNetwork_DPlay

namespace zNetworkDPlay {
/**
 * Reimplements 0x48bee0: zNetworkDPlay::FreeServiceProviderInfoBuffers.
 * Purpose: release duplicated provider display-name and connection buffers.
 */
void __fastcall FreeServiceProviderInfoBuffers(
    zNetworkDPlayServiceProviderInfo *providerInfo
) {
    free(providerInfo->displayName);
    providerInfo->displayName = 0;
    free(providerInfo->connectionData);
    providerInfo->connectionData = 0;
}
} // namespace zNetworkDPlay

namespace {
/**
 * Recovered local helper: DeletePlayerRecordNode.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp.
 * Original helper evidence: no standalone retail address; recovered with the surrounding
 * player-list cleanup functions in this source file.
 * Purpose: release a player-record list node and its optional player payload.
 */
void DeletePlayerRecordNode(
    zNetworkPlayerRecordListNode *node
) {
    if (node->playerRecord != 0) {
        ::operator delete(node->playerRecord);
        node->playerRecord = 0;
    }

    ::operator delete(node);
}
} // namespace

namespace zNetwork {
/**
 * Reimplements 0x48c0a0: zNetwork::RegisterPacketHandler.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\zNetwork.cpp.
 * Purpose: allocate a packet-handler record and append it to the dispatch list.
 */
zNetworkDispatchHandlerRecord *__fastcall RegisterPacketHandler(
    int packetType,
    zNetworkPacketHandler handlerProc,
    int mode
) {
    zNetworkDispatchHandlerRecord *const record =
        (zNetworkDispatchHandlerRecord *)(::operator new(sizeof(zNetworkDispatchHandlerRecord)));
    if (record != 0) {
        record->packetType = (short)(packetType);
        record->handler = handlerProc;
        record->mode = mode;
    }

    zNetworkDispatchHandlerListNode *const sentinel = g_zNetwork_DispatchHandlerListSentinel;
    zNetworkDispatchHandlerListNode *const node =
        (zNetworkDispatchHandlerListNode *)(::operator new(
            sizeof(zNetworkDispatchHandlerListNode)
        ));
    zNetworkDispatchHandlerListNode *const prev = sentinel->prev;
    node->next = sentinel != 0 ? sentinel : node;
    node->prev = prev != 0 ? prev : node;
    sentinel->prev = node;
    node->prev->next = node;
    node->record = record;
    ++g_zNetwork_DispatchHandlerListCount;

    return record;
}

/**
 * Reimplements 0x48c120: zNetwork::UnregisterPacketHandler.
 * Purpose: remove packet-handler registrations matching a packet type and
 * handler procedure from the dispatch list.
 */
int __fastcall UnregisterPacketHandler(
    int packetType,
    zNetworkPacketHandler handlerProc
) {
    zNetworkDispatchHandlerListNode *const sentinel = g_zNetwork_DispatchHandlerListSentinel;
    zNetworkDispatchHandlerListNode *node = sentinel->next;
    while (node != sentinel) {
        zNetworkDispatchHandlerRecord *const record = node->record;
        if (record->packetType == packetType && record->handler == handlerProc) {
            break;
        }

        node = node->next;
    }

    if (node != sentinel) {
        zNetworkDispatchHandlerListNode *write = node;
        node = node->next;
        while (node != sentinel) {
            zNetworkDispatchHandlerRecord *const record = node->record;
            if (record->packetType != packetType || record->handler != handlerProc) {
                write->record = record;
                write = write->next;
            }

            node = node->next;
        }

        node = write;
        while (node != sentinel) {
            zNetworkDispatchHandlerListNode *const next = node->next;
            node->prev->next = next;
            next->prev = node->prev;
            ::operator delete(node);
            --g_zNetwork_DispatchHandlerListCount;
            node = next;
        }
    }

    return 1;
}
} // namespace zNetwork

namespace zNetwork_DPlay {
/**
 * Reimplements 0x48c200: zNetwork_DPlay::DispatchPacketToHandlers.
 * Purpose: call every registered handler matching the incoming packet type.
 */
void __fastcall DispatchPacketToHandlers(
    int senderPlayerId,
    zNetworkPacketHeader *packet
) {
    zNetworkDispatchHandlerListNode *const sentinel = g_zNetwork_DispatchHandlerListSentinel;
    zNetworkDispatchHandlerListNode *node = sentinel->next;
    while (node != sentinel) {
        zNetworkDispatchHandlerRecord *const record = node->record;
        if (record->packetType == packet->packetType) {
            record->handler(
                senderPlayerId,
                packet
            );
        }

        node = node->next;
    }
}
} // namespace zNetwork_DPlay

namespace zNetwork {
/**
 * Reimplements 0x489f30: zNetwork::ClearEnumeratedSessionList.
 * Purpose: free cached enumerated DirectPlay session descriptors and their
 * reserved-data buffers.
 */
void ClearEnumeratedSessionList() {
    zNetworkDPlaySessionDesc *desc = (zNetworkDPlaySessionDesc *)(zArchiveList_PopFrontPayload(
        g_zNetwork_EnumeratedSessionList
    ));
    while (desc != 0) {
        if (desc->dwReserved1 != 0) {
            free((void *)(desc->dwReserved1));
        }
        free(desc);
        desc = (zNetworkDPlaySessionDesc *)(zArchiveList_PopFrontPayload(
            g_zNetwork_EnumeratedSessionList
        ));
    }
}

/**
 * Reimplements 0x489fa0: zNetwork::ClearServiceProviderList.
 * Purpose: release DirectPlay service-provider entries and clear the provider
 * vector range.
 */
void ClearServiceProviderList() {
    zNetworkServiceProviderListVec *const list = g_zNetwork_ServiceProviderList;
    for (zNetworkDPlayServiceProviderInfo **it = list->begin; it != list->end; ++it) {
        zNetworkDPlayServiceProviderInfo *const info = *it;
        if (info != 0) {
            free(info->displayName);
            info->displayName = 0;
            free(info->connectionData);
            info->connectionData = 0;
            ::operator delete(info);
        }

        *it = 0;
    }

    zNetworkDPlayServiceProviderInfo **first = list->begin;
    zNetworkDPlayServiceProviderInfo **last = list->end;
    zNetworkDPlayServiceProviderInfo **finish = list->end;
    while (last != finish) {
        *first = *last;
        ++first;
        ++last;
    }

    list->end = first;
}

/**
 * Reimplements 0x48a030: zNetwork::ClearPlayerRecordList.
 * Purpose: release player-record payloads and delete all player-record list
 * nodes while preserving the sentinel.
 */
void ClearPlayerRecordList() {
    zNetworkPlayerRecordList *list = g_zNetwork_PlayerRecordList;
    zNetworkPlayerRecordListNode *const sentinel = list->sentinelNode;
    zNetworkPlayerRecordListNode *node = sentinel->next;
    zNetworkPlayerRecordListNode *clearNode = node;
    int hasNode = node != sentinel;
    while (hasNode != 0) {
        if (node->playerRecord != 0) {
            ::operator delete(node->playerRecord);
        }

        clearNode->playerRecord = 0;
        node = node->next;
        clearNode = clearNode->next;
        hasNode = node != sentinel;
    }

    list = g_zNetwork_PlayerRecordList;
    zNetworkPlayerRecordListNode *const deleteSentinel = list->sentinelNode;
    node = deleteSentinel->next;
    hasNode = node != deleteSentinel;
    if (hasNode != 0) {
        int *const count = &list->count;
        do {
            zNetworkPlayerRecordListNode *const deleteNode = node;
            node = node->next;
            deleteNode->prev->next = deleteNode->next;
            deleteNode->next->prev = deleteNode->prev;
            ::operator delete(deleteNode);
            --*count;
            hasNode = node != deleteSentinel;
        } while (hasNode != 0);
    }
}

/**
 * Reimplements 0x489f90: zNetwork::SetFatalDisconnectCallback.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp.
 * Purpose: set the callback invoked on fatal DirectPlay disconnect.
 */
void __fastcall SetFatalDisconnectCallback(
    zNetworkFatalDisconnectCallback callback
) {
    g_zNetwork_FatalDisconnectCallback = callback;
}

/**
 * Reimplements 0x489d00: zNetwork::InitSessionRuntime.
 * Original source path: D:\Proj\GameZRecoil\zNetwork\zNetwork.cpp.
 * Purpose: initialize DirectPlay session globals, lists, and default handlers.
 */
int __fastcall InitSessionRuntime(
    GUID *appGuid
) {
    zNetwork_DPlay4 *directPlay4 = 0;
    g_zNetwork_FatalDisconnectCallback = 0;
    g_zNetwork_ReceiveBuffer = 0;

    if (zNetwork_DPlay::CreateInterfaceAndCoInitialize(&directPlay4) >= 0) {
        g_zNetwork_SessionRuntimeInitialized = 1;
        g_zNetwork_FatalDisconnectTriggered = 0;
        g_zNetwork_AppGuid = appGuid;
        g_zNetwork_pDirectPlay4 = directPlay4;
        g_zNetwork_CurrentSessionDescCache = 0;
        g_zNetwork_LocalPlayerRecord = 0;
        DeleteAllDispatchHandlers();
    }

    if (g_zNetwork_EnumeratedSessionList == 0) {
        g_zNetwork_EnumeratedSessionList = zArchiveList_CreateEmpty();
    }

    zNetworkPlayerRecordList *playerRecordList =
        (zNetworkPlayerRecordList *)(::operator new(sizeof(zNetworkPlayerRecordList)));
    if (playerRecordList != 0) {
        zNetworkPlayerRecordListNode *const sentinel =
            (zNetworkPlayerRecordListNode *)(::operator new(sizeof(zNetworkPlayerRecordListNode)));
        sentinel->next = sentinel;
        sentinel->prev = sentinel;
        playerRecordList->sentinelNode = sentinel;
        playerRecordList->count = 0;
    } else {
        playerRecordList = 0;
    }
    g_zNetwork_PlayerRecordList = playerRecordList;

    zNetworkServiceProviderListVec *serviceProviderList =
        (zNetworkServiceProviderListVec *)(::operator new(sizeof(zNetworkServiceProviderListVec)));
    if (serviceProviderList != 0) {
        serviceProviderList->begin = 0;
        serviceProviderList->end = 0;
        serviceProviderList->cap = 0;
    } else {
        serviceProviderList = 0;
    }
    g_zNetwork_ServiceProviderList = serviceProviderList;

    RegisterPacketHandler(
        1,
        zNetwork_ApplyPkt01_PlayerColorAssignments,
        2
    );
    return 0;
}

/**
 * Reimplements 0x489e10: zNetwork::ShutdownSessionRuntime.
 * Purpose: close DirectPlay and release all session-runtime network lists and
 * buffers.
 */
int ShutdownSessionRuntime() {
    zNetwork_DPlay::CloseReleaseAndCoUninitialize(g_zNetwork_pDirectPlay4);
    g_zNetwork_SessionRuntimeInitialized = 0;
    UnregisterPacketHandler(
        1,
        zNetwork_ApplyPkt01_PlayerColorAssignments
    );

    ClearEnumeratedSessionList();
    zArchiveList_Destroy(g_zNetwork_EnumeratedSessionList);
    g_zNetwork_EnumeratedSessionList = 0;
    g_zNetwork_CurrentSessionDescCache = 0;

    ClearServiceProviderList();
    if (g_zNetwork_ServiceProviderList != 0) {
        ::operator delete(g_zNetwork_ServiceProviderList->begin);
        g_zNetwork_ServiceProviderList->begin = 0;
        g_zNetwork_ServiceProviderList->end = 0;
        g_zNetwork_ServiceProviderList->cap = 0;
        ::operator delete(g_zNetwork_ServiceProviderList);
        g_zNetwork_ServiceProviderList = 0;
    }

    ClearPlayerRecordList();
    zNetworkPlayerRecordList *const playerRecordList = g_zNetwork_PlayerRecordList;
    if (playerRecordList != 0) {
        zNetworkPlayerRecordListNode *const sentinel = playerRecordList->sentinelNode;
        zNetworkPlayerRecordListNode *node = sentinel->next;
        while (node != sentinel) {
            zNetworkPlayerRecordListNode *const next = node->next;
            node->prev->next = node->next;
            node->next->prev = node->prev;
            ::operator delete(node);
            --playerRecordList->count;
            node = next;
        }

        ::operator delete(playerRecordList->sentinelNode);
        playerRecordList->sentinelNode = 0;
        playerRecordList->count = 0;
        ::operator delete(playerRecordList);
        g_zNetwork_PlayerRecordList = 0;
    }

    if (g_zNetwork_ReceiveBuffer != 0) {
        free(g_zNetwork_ReceiveBuffer);
        g_zNetwork_ReceiveBuffer = 0;
    }

    return 0;
}
} // namespace zNetwork
