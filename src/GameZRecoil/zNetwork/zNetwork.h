#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include <dplay.h>

#include "recoil/recoil_callconv.h"

struct zArchiveList;
struct IDirectPlayLobby3;
typedef IDirectPlayLobby3 IDirectPlayLobby3A;
typedef IDirectPlay4A zNetwork_DPlay4;
typedef DPNAME zNetworkDPlayName;
typedef DPCAPS zNetworkDPlayCaps;
typedef DPSESSIONDESC2 zNetworkDPlaySessionDesc;
struct zNetworkPacketHeader;

typedef int(__fastcall *zNetworkPacketHandler)(
    int senderPlayerId,
    zNetworkPacketHeader *packet
);
typedef void(__fastcall *zNetworkFatalDisconnectCallback)(int reason);
typedef LPDPENUMPLAYERSCALLBACK2 zNetworkDPlayEnumPlayersCallback;
typedef LPDPENUMSESSIONSCALLBACK2 zNetworkDPlayEnumSessionsCallback;
typedef LPDPENUMCONNECTIONSCALLBACK zNetworkDPlayEnumConnectionsCallback;

struct zNetwork_PlayerRecord {
    unsigned int playerKey;
    zNetworkDPlayCaps playerCaps;
    zNetworkDPlayName playerNameInfo;
    void *createPlayerEventHandle;
    char playerName[0x50];
    char altName[0x50];
    int colorIndex;
};

struct zNetworkDPlaySystemMessageFields {
    unsigned char reserved_004[0x04];
    unsigned int playerId;
    unsigned char reserved_00c[0x0c];
    unsigned int createFlagsOrPlayerType;
    unsigned int nameShortOrAsyncHandle;
    char *nameLong;
    char *nameDisplay;
};

struct zNetworkDPlaySystemMessage {
    int msgType;
    union {
        unsigned char payload_004[0x50];
        zNetworkDPlaySystemMessageFields fields;
    };
};

RECOIL_STATIC_ASSERT(
    offsetof(
        zNetwork_PlayerRecord,
        playerNameInfo
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetwork_PlayerRecord,
        createPlayerEventHandle
    ) == 0x3c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetwork_PlayerRecord,
        playerName
    ) == 0x40
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetwork_PlayerRecord,
        altName
    ) == 0x90
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetwork_PlayerRecord,
        colorIndex
    ) == 0xe0
);
RECOIL_STATIC_ASSERT(sizeof(zNetwork_PlayerRecord) == 0xe4);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetworkDPlaySystemMessageFields,
        playerId
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetworkDPlaySystemMessageFields,
        createFlagsOrPlayerType
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetworkDPlaySystemMessageFields,
        nameShortOrAsyncHandle
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetworkDPlaySystemMessageFields,
        nameLong
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetworkDPlaySystemMessageFields,
        nameDisplay
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetworkDPlaySystemMessage,
        payload_004
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetworkDPlaySystemMessage,
        fields
    ) == 0x04
);
RECOIL_STATIC_ASSERT(sizeof(zNetworkDPlaySystemMessage) == 0x54);

struct zNetworkPacketHeader {
    short packetType;
    short packetSizeBytes;
    int payloadDword0;
};

struct zNetworkPlayerColorPair {
    int playerKey;
    int colorIndex;
};

struct NetPkt01_PlayerColorAssignments {
    zNetworkPacketHeader header;
    int pairCount;
    zNetworkPlayerColorPair pairs[1];
};

struct zNetworkDPlaySessionDescCache {
    int openMode;
    int reserved_04;
    zNetworkDPlaySessionDesc desc;
};

struct zNetworkSessionDescStatusFields {
    int eventCode;
    int statusFlags;
    int valueOrTime;
    int auxParam;
    int selectedSessionIndex;
    int maxPlayers;
    char sessionNameBuf[0x5c];
};

struct zNetworkDPlayServiceProviderInfo {
    GUID serviceProviderGuid;
    char *displayName;
    void *connectionData;
    int providerFlags;
};

struct zNetworkServiceProviderListVec {
    int flags;
    zNetworkDPlayServiceProviderInfo **begin;
    zNetworkDPlayServiceProviderInfo **end;
    zNetworkDPlayServiceProviderInfo **cap;
};

struct zNetworkPlayerRecordListNode {
    zNetworkPlayerRecordListNode *next;
    zNetworkPlayerRecordListNode *prev;
    zNetwork_PlayerRecord *playerRecord;
};

struct zNetworkPlayerRecordList {
    int flags;
    zNetworkPlayerRecordListNode *sentinelNode;
    int count;
};

struct zNetworkDispatchHandlerRecord {
    short packetType;
    short unknown_02;
    zNetworkPacketHandler handler;
    int mode;
};

struct zNetworkDispatchHandlerListNode {
    zNetworkDispatchHandlerListNode *next;
    zNetworkDispatchHandlerListNode *prev;
    zNetworkDispatchHandlerRecord *record;
};

extern "C" {
extern zNetwork_DPlay4 *g_zNetwork_pDirectPlay4;
extern zNetwork_PlayerRecord *g_zNetwork_LocalPlayerRecord;
extern int g_zNetwork_IsHostFlag;
extern int g_zNetwork_LocalPlayerKey;
extern char g_zNetwork_LocalPlayerNameScratch[0x50];
extern int g_zNetwork_TcpIpAsyncSendEnabled;
extern int g_zNetwork_ActiveProviderIsModem;
extern int g_zNetwork_ActiveProviderIsTcpIp;
extern zNetworkDPlayCaps g_zNetwork_DPlayCaps;
extern GUID *g_zNetwork_AppGuid;
extern GUID g_zNetwork_RecoilAppGuid;
extern unsigned int g_zNetwork_LastSendExHandle;
extern int g_zNetwork_LastSendExCompleted;
extern int g_zNetwork_SessionRuntimeInitialized;
extern zNetworkDPlaySessionDescCache *g_zNetwork_CurrentSessionDescCache;
extern zNetworkFatalDisconnectCallback g_zNetwork_FatalDisconnectCallback;
extern int g_zNetwork_FatalDisconnectTriggered;
extern int g_zNetworkCurrentPlayerCountCached;
extern char g_zNetwork_SessionNameCache[0x5c];
extern zArchiveList *g_zNetwork_EnumeratedSessionList;
extern zNetworkServiceProviderListVec *g_zNetwork_ServiceProviderList;
extern zNetworkPlayerRecordList *g_zNetwork_PlayerRecordList;
extern void *g_zNetwork_ReceiveBuffer;
extern unsigned int g_zNetwork_ReceiveBufferCapacity;
extern int g_zNetwork_PlayerColorInUseFlags[16];
extern zNetworkDispatchHandlerListNode *g_zNetwork_DispatchHandlerListSentinel;
extern int g_zNetwork_DispatchHandlerListCount;
extern unsigned char g_zNetwork_DispatchHandlerListFlags;

int zNetwork_DPlay_DestroyCachedLocalPlayer();
int zNetwork_GetLocalPlayerKey();
int zNetwork_GetLocalPlayerColorIndex();
void zNetwork_InitMessageHandlers();
void zNetwork_CreateEmptyDispatchHandlerList();
void zNetwork_RegisterDispatchHandlerListShutdown();
void zNetwork_DestroyDispatchHandlerList();
int __fastcall zNetwork_DPlay_SendUnreliable(
    zNetworkPacketHeader *packet,
    unsigned int packetSizeBytes
);
int __fastcall zNetwork_DPlay_SendReliable(
    zNetworkPacketHeader *packet,
    unsigned int packetSizeBytes
);
int __fastcall zNetwork_DPlay_SendExUnreliableTracked(
    zNetworkPacketHeader *packet,
    unsigned int packetSizeBytes
);
int __fastcall zNetwork_DPlay_SendExReliable(
    zNetworkPacketHeader *packet,
    unsigned int packetSizeBytes
);
int __fastcall zNetwork_SendPacketUnreliable(zNetworkPacketHeader *packet);
int __fastcall zNetwork_SendPacketReliable(zNetworkPacketHeader *packet);
int __fastcall zNetwork_DPlay_ReportError(
    int hresult,
    const char *sourceFile,
    int sourceLine
);
int __fastcall zNetwork_ApplyPkt01_PlayerColorAssignments(
    int senderPlayerId,
    zNetworkPacketHeader *packet
);
zNetwork_PlayerRecord *__fastcall zNetwork_FindPlayerRecordByKey(int playerKey);
int __fastcall zNetwork_GetPlayerColorIndexByKey(int playerKey);
int zNetwork_GetPlayerRecordCount();
int __fastcall zNetwork_ExtractStatusFieldsFromSessionDesc(
    zNetworkSessionDescStatusFields *outFields
);
int __fastcall zNetwork_ApplyStatusFieldsToSessionDesc(
    zNetworkSessionDescStatusFields *statusFields
);
}

namespace zNetwork_DPlay {
int RefreshServiceProviderList();
int EnumSessions();
int EnumPlayers();
int __fastcall CreateLocalPlayerRecordAndRegister(char *playerName);
int __fastcall CreateSessionFromStatusFields(
    zNetworkSessionDescStatusFields *statusFields
);
int __fastcall CreateInterfaceAndCoInitialize(
    zNetwork_DPlay4 **outDirectPlay4
);
int __fastcall CloseReleaseAndCoUninitialize(zNetwork_DPlay4 *directPlay4);
void __fastcall DispatchPacketToHandlers(
    int senderPlayerId,
    zNetworkPacketHeader *packet
);
} // namespace zNetwork_DPlay

namespace zNetworkDPlay {
int __fastcall CreateLobby3AInterface(IDirectPlayLobby3A **outLobby3A);
zNetworkServiceProviderListVec *RefreshAndGetServiceProviderList();
int __stdcall EnumConnectionsCallback_AddServiceProviderInfo(
    const GUID *serviceProviderGuid,
    void *connectionData,
    DWORD connectionDataSize,
    const zNetworkDPlayName *providerName,
    DWORD providerFlags,
    void *context
);
char *__fastcall GetEnumeratedSessionNameByIndex(int entryIndex);
void __fastcall GetEnumeratedSessionPlayerCountsByIndex(
    int entryIndex,
    int *currentPlayersOut,
    int *maxPlayersOut
);
int QueryCapsAndConfigureSendMode();
int __stdcall EnumSessionCallback_AddSessionDescCache(
    const zNetworkDPlaySessionDesc *sessionDesc,
    DWORD *timeoutMs,
    DWORD flags,
    void *context
);
int __stdcall EnumPlayerCallback_AddPlayerRecord(
    DPID playerId,
    DWORD playerType,
    const zNetworkDPlayName *playerNameInfo,
    DWORD flags,
    void *context
);
void __fastcall FreeServiceProviderInfoBuffers(
    zNetworkDPlayServiceProviderInfo *providerInfo
);
int __fastcall InitializeConnectionFromProviderInfo(
    zNetworkDPlayServiceProviderInfo *providerInfo
);
int __fastcall SelectServiceProviderAndInitConnection(
    zNetworkDPlayServiceProviderInfo *providerInfo
);
int __fastcall SelectTcpIpProviderAndEnumSessions(
    char *addressString,
    int skipSessionEnumeration
);
int __fastcall OpenSelectedSessionAndReadStatusFields(
    zNetworkSessionDescStatusFields *statusFields
);
int EnumSessionsForCurrentApp();
int __fastcall PumpIncomingMessages(zNetworkDPlaySystemMessage *systemMessage);
int __fastcall ReceivePendingMessages(int messageBudget);
} // namespace zNetworkDPlay

namespace zNetwork {
int IsHost();
int AllocFreePlayerColorIndex();
void __fastcall HostSendPlayerColorAssignmentsPacket(int joiningPlayerKey);
int __fastcall GetPlayerNameByKey(
    int playerKey,
    char *destination,
    unsigned int maxCount
);
void DeleteAllDispatchHandlers();
zNetworkDispatchHandlerRecord *__fastcall RegisterPacketHandler(
    int packetType,
    zNetworkPacketHandler handlerProc,
    int mode
);
int __fastcall UnregisterPacketHandler(
    int packetType,
    zNetworkPacketHandler handlerProc
);
void __fastcall RemovePlayerRecordByKey(int playerKey);
void ClearEnumeratedSessionList();
void ClearServiceProviderList();
void ClearPlayerRecordList();
void __fastcall SetFatalDisconnectCallback(
    zNetworkFatalDisconnectCallback callback
);
int __fastcall InitSessionRuntime(GUID *appGuid);
int ShutdownSessionRuntime();
} // namespace zNetwork

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(sizeof(zNetworkDPlayCaps) == 0x28);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetworkDPlayCaps,
        dwFlags
    ) == 0x04
);
RECOIL_STATIC_ASSERT(sizeof(zNetworkDPlaySessionDesc) == 0x50);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetworkDPlaySessionDesc,
        dwMaxPlayers
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetworkDPlaySessionDesc,
        lpszSessionNameA
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetworkDPlaySessionDesc,
        dwUser1
    ) == 0x40
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetworkDPlaySessionDescCache,
        desc
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetworkSessionDescStatusFields,
        selectedSessionIndex
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetworkSessionDescStatusFields,
        sessionNameBuf
    ) == 0x18
);
RECOIL_STATIC_ASSERT(sizeof(zNetworkSessionDescStatusFields) == 0x74);
#endif
