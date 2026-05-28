#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "recoil/recoil_callconv.h"

struct zArchiveList;
struct zNetwork_DPlay4;
struct zNetworkDPlayName;
struct zNetworkDPlaySessionDesc;
struct zNetworkPacketHeader;

typedef int (RECOIL_FASTCALL *zNetworkPacketHandler)(int senderPlayerId,
                                                              zNetworkPacketHeader *packet);
typedef void (RECOIL_FASTCALL *zNetworkFatalDisconnectCallback)(int reason);
typedef int (RECOIL_STDCALL *zNetworkDPlayEnumPlayersCallback)(
    unsigned int playerId, unsigned int playerType, zNetworkDPlayName *playerNameInfo,
    unsigned int flags, void *context);

struct zNetworkDPlayName {
    unsigned int size;
    unsigned int flags;
    char *shortName;
    char *longName;
};

struct zNetwork_DPlay4Vtable {
    void *reserved_00[9];
    int(RECOIL_STDCALL *DestroyPlayer_24)(zNetwork_DPlay4 *self, unsigned int playerKey);
    void *reserved_28[2];
    int(RECOIL_STDCALL *EnumPlayers_30)(zNetwork_DPlay4 *self, void *sessionGuid,
                                        zNetworkDPlayEnumPlayersCallback callback, void *context,
                                        unsigned int flags);
    void *reserved_34[12];
    int(RECOIL_STDCALL *Receive_64)(zNetwork_DPlay4 *self, unsigned int *fromPlayer,
                                    unsigned int *toPlayer, unsigned int flags, void *packet,
                                    unsigned int *packetSizeBytes);
    int(RECOIL_STDCALL *Send_68)(zNetwork_DPlay4 *self, unsigned int fromPlayer,
                                          unsigned int toPlayer, unsigned int flags, void *packet,
                                          unsigned int packetSizeBytes);
    void *reserved_6c[4];
    int(RECOIL_STDCALL *SetSessionDesc_7c)(zNetwork_DPlay4 *self,
                                                    zNetworkDPlaySessionDesc *sessionDesc,
                                                    unsigned int flags);
    void *reserved_80[17];
    int(RECOIL_STDCALL *SendEx_c4)(zNetwork_DPlay4 *self, unsigned int fromPlayer,
                                            unsigned int toPlayer, unsigned int flags,
                                            void *packet, unsigned int packetSizeBytes,
                                            unsigned int priority, unsigned int timeout,
                                            void *context, unsigned int *asyncHandle);
    void *reserved_c8[1];
    int(RECOIL_STDCALL *CancelMessage_cc)(zNetwork_DPlay4 *self, unsigned int asyncHandle,
                                                   unsigned int flags);
};

struct zNetwork_DPlay4 {
    const zNetwork_DPlay4Vtable *vtbl_00;
};

struct zNetwork_PlayerRecord {
    unsigned int playerKey;
    unsigned char reserved_004[0x28];
    zNetworkDPlayName playerNameInfo;
    unsigned char reserved_03c[0x04];
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

RECOIL_STATIC_ASSERT(offsetof(zNetwork_PlayerRecord, playerNameInfo) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zNetwork_PlayerRecord, playerName) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zNetwork_PlayerRecord, altName) == 0x90);
RECOIL_STATIC_ASSERT(offsetof(zNetwork_PlayerRecord, colorIndex) == 0xe0);
RECOIL_STATIC_ASSERT(sizeof(zNetwork_PlayerRecord) == 0xe4);
RECOIL_STATIC_ASSERT(offsetof(zNetworkDPlaySystemMessageFields, playerId) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zNetworkDPlaySystemMessageFields, createFlagsOrPlayerType) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zNetworkDPlaySystemMessageFields, nameShortOrAsyncHandle) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zNetworkDPlaySystemMessageFields, nameLong) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zNetworkDPlaySystemMessageFields, nameDisplay) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zNetworkDPlaySystemMessage, payload_004) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zNetworkDPlaySystemMessage, fields) == 0x04);
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
    unsigned char reserved_00[0x08];
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
extern int g_zNetwork_TcpIpAsyncSendEnabled;
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

int RECOIL_CDECL zNetwork_DPlay_DestroyCachedLocalPlayer();
int RECOIL_CDECL zNetwork_GetLocalPlayerKey();
int RECOIL_CDECL zNetwork_GetLocalPlayerColorIndex();
void RECOIL_CDECL zNetwork_InitMessageHandlers();
void RECOIL_CDECL zNetwork_CreateEmptyDispatchHandlerList();
void RECOIL_CDECL zNetwork_RegisterDispatchHandlerListShutdown();
void RECOIL_CDECL zNetwork_DestroyDispatchHandlerList();
int RECOIL_FASTCALL zNetwork_DPlay_SendUnreliable(zNetworkPacketHeader *packet,
                                                           unsigned int packetSizeBytes);
int RECOIL_FASTCALL zNetwork_DPlay_SendReliable(zNetworkPacketHeader *packet,
                                                         unsigned int packetSizeBytes);
int RECOIL_FASTCALL zNetwork_DPlay_SendExUnreliableTracked(zNetworkPacketHeader *packet,
                                                                    unsigned int packetSizeBytes);
int RECOIL_FASTCALL zNetwork_DPlay_SendExReliable(zNetworkPacketHeader *packet,
                                                           unsigned int packetSizeBytes);
int RECOIL_FASTCALL zNetwork_SendPacketUnreliable(zNetworkPacketHeader *packet);
int RECOIL_FASTCALL zNetwork_SendPacketReliable(zNetworkPacketHeader *packet);
int RECOIL_FASTCALL zNetwork_DPlay_ReportError(int hresult,
                                                        const char *sourceFile,
                                                        int sourceLine);
int RECOIL_FASTCALL zNetwork_ApplyPkt01_PlayerColorAssignments(
    int senderPlayerId, zNetworkPacketHeader *packet);
zNetwork_PlayerRecord *RECOIL_FASTCALL zNetwork_FindPlayerRecordByKey(int playerKey);
int RECOIL_FASTCALL zNetwork_GetPlayerColorIndexByKey(int playerKey);
int RECOIL_CDECL zNetwork_GetPlayerRecordCount();
int RECOIL_FASTCALL
zNetwork_ExtractStatusFieldsFromSessionDesc(zNetworkSessionDescStatusFields *outFields);
int RECOIL_FASTCALL
zNetwork_ApplyStatusFieldsToSessionDesc(zNetworkSessionDescStatusFields *statusFields);
}

namespace zNetwork_DPlay {
RECOIL_NOINLINE int RECOIL_CDECL EnumPlayers();
RECOIL_NOINLINE int RECOIL_FASTCALL
CloseReleaseAndCoUninitialize(zNetwork_DPlay4 *directPlay4);
RECOIL_NOINLINE void RECOIL_FASTCALL
DispatchPacketToHandlers(int senderPlayerId, zNetworkPacketHeader *packet);
}

namespace zNetworkDPlay {
RECOIL_NOINLINE int RECOIL_STDCALL EnumPlayerCallback_AddPlayerRecord(
    unsigned int playerId, unsigned int playerType, zNetworkDPlayName *playerNameInfo,
    unsigned int flags, void *context);
RECOIL_NOINLINE int RECOIL_FASTCALL PumpIncomingMessages(zNetworkDPlaySystemMessage *systemMessage);
RECOIL_NOINLINE int RECOIL_FASTCALL ReceivePendingMessages(int messageBudget);
}

namespace zNetwork {
RECOIL_NOINLINE int RECOIL_CDECL IsHost();
RECOIL_NOINLINE int RECOIL_CDECL AllocFreePlayerColorIndex();
RECOIL_NOINLINE void RECOIL_FASTCALL HostSendPlayerColorAssignmentsPacket(int joiningPlayerKey);
RECOIL_NOINLINE int RECOIL_FASTCALL
GetPlayerNameByKey(int playerKey, char *destination, unsigned int maxCount);
RECOIL_NOINLINE void RECOIL_CDECL DeleteAllDispatchHandlers();
RECOIL_NOINLINE zNetworkDispatchHandlerRecord *RECOIL_FASTCALL
RegisterPacketHandler(int packetType, zNetworkPacketHandler handlerProc, int mode);
RECOIL_NOINLINE int RECOIL_FASTCALL
UnregisterPacketHandler(int packetType, zNetworkPacketHandler handlerProc);
RECOIL_NOINLINE void RECOIL_FASTCALL RemovePlayerRecordByKey(int playerKey);
RECOIL_NOINLINE void RECOIL_CDECL ClearEnumeratedSessionList();
RECOIL_NOINLINE void RECOIL_CDECL ClearServiceProviderList();
RECOIL_NOINLINE void RECOIL_CDECL ClearPlayerRecordList();
RECOIL_NOINLINE int RECOIL_CDECL ShutdownSessionRuntime();
} // namespace zNetwork

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(sizeof(zNetworkDPlaySessionDesc) == 0x50);
RECOIL_STATIC_ASSERT(offsetof(zNetworkDPlaySessionDesc, maxPlayers) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zNetworkDPlaySessionDesc, sessionName) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zNetworkDPlaySessionDesc, customEventCode) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zNetworkDPlaySessionDescCache, desc) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zNetworkSessionDescStatusFields, selectedSessionIndex) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zNetworkSessionDescStatusFields, sessionNameBuf) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zNetworkSessionDescStatusFields) == 0x74);
#endif
