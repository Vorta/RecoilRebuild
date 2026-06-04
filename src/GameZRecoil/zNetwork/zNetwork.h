#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "recoil/recoil_callconv.h"

struct zArchiveList;
struct zNetwork_DPlay4;
struct IDirectPlayLobby3;
typedef IDirectPlayLobby3 IDirectPlayLobby3A;
struct zNetworkDPlayName;
struct zNetworkDPlayCaps;
struct zNetworkDPlaySessionDesc;
struct zNetworkPacketHeader;

typedef int(__fastcall *zNetworkPacketHandler)(
    int senderPlayerId,
    zNetworkPacketHeader *packet
);
typedef void(__fastcall *zNetworkFatalDisconnectCallback)(int reason);
typedef int(__stdcall *zNetworkDPlayEnumPlayersCallback)(
    unsigned int playerId,
    unsigned int playerType,
    zNetworkDPlayName *playerNameInfo,
    unsigned int flags,
    void *context
);
typedef int(__stdcall *zNetworkDPlayEnumSessionsCallback)(
    zNetworkDPlaySessionDesc *sessionDesc,
    unsigned int *timeoutMs,
    unsigned int flags,
    void *context
);
typedef int(__stdcall *zNetworkDPlayEnumConnectionsCallback)(
    unsigned char *serviceProviderGuid,
    void *connectionData,
    unsigned int connectionDataSize,
    zNetworkDPlayName *providerName,
    unsigned int providerFlags,
    void *context
);

struct zNetworkDPlayName {
    unsigned int size;
    unsigned int flags;
    char *shortName;
    char *longName;
};

struct zNetwork_DPlay4Vtable {
    void *reserved_00[2];
    int(__stdcall *Release_08)(zNetwork_DPlay4 *self);
    void *reserved_0c;
    int(__stdcall *Close_10)(zNetwork_DPlay4 *self);
    void *reserved_14;
    int(__stdcall *CreatePlayer_18)(
        zNetwork_DPlay4 *self,
        unsigned int *playerId,
        zNetworkDPlayName *playerNameInfo,
        void *eventHandle,
        void *data,
        unsigned int dataSize,
        unsigned int flags
    );
    void *reserved_1c[2];
    int(__stdcall *DestroyPlayer_24)(
        zNetwork_DPlay4 *self,
        unsigned int playerKey
    );
    void *reserved_28[2];
    int(__stdcall *EnumPlayers_30)(
        zNetwork_DPlay4 *self,
        void *sessionGuid,
        zNetworkDPlayEnumPlayersCallback callback,
        void *context,
        unsigned int flags
    );
    int(__stdcall *EnumSessions_34)(
        zNetwork_DPlay4 *self,
        zNetworkDPlaySessionDesc *sessionDesc,
        unsigned int timeoutMs,
        zNetworkDPlayEnumSessionsCallback callback,
        void *context,
        unsigned int flags
    );
    int(__stdcall *GetCaps_38)(
        zNetwork_DPlay4 *self,
        zNetworkDPlayCaps *caps,
        unsigned int flags
    );
    void *reserved_3c[4];
    int(__stdcall *GetPlayerCaps_4c)(
        zNetwork_DPlay4 *self,
        unsigned int playerId,
        zNetworkDPlayCaps *caps,
        unsigned int flags
    );
    void *reserved_50[3];
    void *reserved_5c;
    int(__stdcall *Open_60)(
        zNetwork_DPlay4 *self,
        zNetworkDPlaySessionDesc *sessionDesc,
        unsigned int flags
    );
    int(__stdcall *Receive_64)(
        zNetwork_DPlay4 *self,
        unsigned int *fromPlayer,
        unsigned int *toPlayer,
        unsigned int flags,
        void *packet,
        unsigned int *packetSizeBytes
    );
    int(__stdcall *Send_68)(
        zNetwork_DPlay4 *self,
        unsigned int fromPlayer,
        unsigned int toPlayer,
        unsigned int flags,
        void *packet,
        unsigned int packetSizeBytes
    );
    void *reserved_6c[4];
    int(__stdcall *SetSessionDesc_7c)(
        zNetwork_DPlay4 *self,
        zNetworkDPlaySessionDesc *sessionDesc,
        unsigned int flags
    );
    void *reserved_80[3];
    int(__stdcall *EnumConnections_8c)(
        zNetwork_DPlay4 *self,
        unsigned char *applicationGuid,
        zNetworkDPlayEnumConnectionsCallback callback,
        void *context,
        unsigned int flags
    );
    void *reserved_90[2];
    int(__stdcall *InitializeConnection_98)(
        zNetwork_DPlay4 *self,
        void *connectionData,
        unsigned int flags
    );
    void *reserved_9c[10];
    int(__stdcall *SendEx_c4)(
        zNetwork_DPlay4 *self,
        unsigned int fromPlayer,
        unsigned int toPlayer,
        unsigned int flags,
        void *packet,
        unsigned int packetSizeBytes,
        unsigned int priority,
        unsigned int timeout,
        void *context,
        unsigned int *asyncHandle
    );
    void *reserved_c8[1];
    int(__stdcall *CancelMessage_cc)(
        zNetwork_DPlay4 *self,
        unsigned int asyncHandle,
        unsigned int flags
    );
};

struct zNetwork_DPlay4 {
    const zNetwork_DPlay4Vtable *vtbl_00;
};

RECOIL_STATIC_ASSERT(
    offsetof(
        zNetwork_DPlay4Vtable,
        Release_08
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetwork_DPlay4Vtable,
        Close_10
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetwork_DPlay4Vtable,
        CreatePlayer_18
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetwork_DPlay4Vtable,
        DestroyPlayer_24
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetwork_DPlay4Vtable,
        GetCaps_38
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetwork_DPlay4Vtable,
        GetPlayerCaps_4c
    ) == 0x4c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetwork_DPlay4Vtable,
        Open_60
    ) == 0x60
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetwork_DPlay4Vtable,
        Send_68
    ) == 0x68
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetwork_DPlay4Vtable,
        SetSessionDesc_7c
    ) == 0x7c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetwork_DPlay4Vtable,
        EnumConnections_8c
    ) == 0x8c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetwork_DPlay4Vtable,
        InitializeConnection_98
    ) == 0x98
);

struct zNetworkDPlayCaps {
    int size;
    int flags;
    int maxBufferSize;
    int maxQueueSize;
    int maxPlayers;
    int hundredBaud;
    int latency;
    int maxLocalPlayers;
    int headerLength;
    int timeout;
};

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

struct zNetworkDPlaySessionDesc {
    int size;
    int flags;
    unsigned char instanceGuid[0x10];
    unsigned char appGuid[0x10];
    int maxPlayers;
    int currentPlayers;
    char *sessionName;
    char *sessionPassword;
    void *reservedData;
    int reservedDataSize;
    int customEventCode;
    int customStatusFlags;
    int customValueOrTime;
    int customAuxParam;
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
    unsigned char serviceProviderGuid[0x10];
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
extern unsigned char *g_zNetwork_AppGuid;
extern unsigned char g_zNetwork_RecoilAppGuid[16];
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
    unsigned char *serviceProviderGuid,
    void *connectionData,
    unsigned int connectionDataSize,
    zNetworkDPlayName *providerName,
    unsigned int providerFlags,
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
    zNetworkDPlaySessionDesc *sessionDesc,
    unsigned int *timeoutMs,
    unsigned int flags,
    void *context
);
int __stdcall EnumPlayerCallback_AddPlayerRecord(
    unsigned int playerId,
    unsigned int playerType,
    zNetworkDPlayName *playerNameInfo,
    unsigned int flags,
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
int __fastcall InitSessionRuntime(unsigned char *appGuid);
int ShutdownSessionRuntime();
} // namespace zNetwork

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(sizeof(zNetworkDPlayCaps) == 0x28);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetworkDPlayCaps,
        flags
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetwork_DPlay4Vtable,
        EnumSessions_34
    ) == 0x34
);
RECOIL_STATIC_ASSERT(sizeof(zNetworkDPlaySessionDesc) == 0x50);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetworkDPlaySessionDesc,
        maxPlayers
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetworkDPlaySessionDesc,
        sessionName
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zNetworkDPlaySessionDesc,
        customEventCode
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
