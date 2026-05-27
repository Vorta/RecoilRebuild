#include "GameZRecoil/zNetwork/zNetwork.h"

#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zReader/zReader.h"

#include <objbase.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
zNetwork_DPlay4 *g_zNetwork_pDirectPlay4 = 0;
zNetwork_PlayerRecord *g_zNetwork_LocalPlayerRecord = 0;
int g_zNetwork_IsHostFlag = 0;
int g_zNetwork_LocalPlayerKey = 0;
int g_zNetwork_TcpIpAsyncSendEnabled = 0;
unsigned int g_zNetwork_LastSendExHandle = 0;
int g_zNetwork_LastSendExCompleted = 0;
int g_zNetwork_SessionRuntimeInitialized = 0;
zNetworkDPlaySessionDescCache *g_zNetwork_CurrentSessionDescCache = 0;
zNetworkFatalDisconnectCallback g_zNetwork_FatalDisconnectCallback = 0;
int g_zNetwork_FatalDisconnectTriggered = 0;
int g_zNetworkCurrentPlayerCountCached = 0;
char g_zNetwork_SessionNameCache[0x5c] = {0};
zArchiveList *g_zNetwork_EnumeratedSessionList = 0;
zNetworkServiceProviderListVec *g_zNetwork_ServiceProviderList = 0;
zNetworkPlayerRecordList *g_zNetwork_PlayerRecordList = 0;
void *g_zNetwork_ReceiveBuffer = 0;
unsigned int g_zNetwork_ReceiveBufferCapacity = 0;
int g_zNetwork_PlayerColorInUseFlags[16] = {0};
zNetworkDispatchHandlerListNode *g_zNetwork_DispatchHandlerListSentinel = 0;
int g_zNetwork_DispatchHandlerListCount = 0;
unsigned char g_zNetwork_DispatchHandlerListFlags = 0;
}

namespace {
const char *kZNetworkDPlaySourceFile = "D:\\Proj\\GameZRecoil\\zNetwork\\znet_dplay.cpp";
const int kDPlayPending = static_cast<int>(0x8000000a);
const int kDPlayBufferTooSmall = static_cast<int>(0x8877001e);

struct DPlayErrorName {
    int hresult;
    const char *name;
};

const DPlayErrorName kDPlayErrorNames[] = {
    {static_cast<int>(0x8000000a), "DPERR_PENDING"},
    {static_cast<int>(0x80004001), "DPERR_UNSUPPORTED"},
    {static_cast<int>(0x80004005), "DPERR_GENERIC"},
    {static_cast<int>(0x8007000e), "DPERR_OUTOFMEMORY"},
    {static_cast<int>(0x80070057), "DPERR_INVALIDPARAMS"},
    {static_cast<int>(0x88770005), "DPERR_ALREADYINITIALIZED"},
    {static_cast<int>(0x8877000a), "DPERR_ACCESSDENIED"},
    {static_cast<int>(0x88770014), "DPERR_ACTIVEPLAYERS"},
    {static_cast<int>(0x8877001e), "DPERR_BUFFERTOOSMALL"},
    {static_cast<int>(0x88770028), "DPERR_CANTADDPLAYER"},
    {static_cast<int>(0x88770032), "DPERR_CANTCREATEGROUP:"},
    {static_cast<int>(0x8877003c), "DPERR_CANTCREATEPLAYER"},
    {static_cast<int>(0x88770046), "DPERR_CANTCREATESESSION"},
    {static_cast<int>(0x88770050), "DPERR_CAPSNOTAVAILABLEYET"},
    {static_cast<int>(0x8877005a), "DPERR_EXCEPTION"},
    {static_cast<int>(0x88770078), "DPERR_INVALIDFLAGS"},
    {static_cast<int>(0x88770082), "DPERR_INVALIDOBJECT"},
    {static_cast<int>(0x88770096), "DPERR_INVALIDPLAYER"},
    {static_cast<int>(0x8877009b), "DPERR_INVALIDGROUP"},
    {static_cast<int>(0x887700a0), "DPERR_NOCAPS"},
    {static_cast<int>(0x887700aa), "DPERR_NOCONNECTION"},
    {static_cast<int>(0x887700be), "DPERR_NOMESSAGES "},
    {static_cast<int>(0x887700c8), "DPERR_NONAMESERVERFOUND"},
    {static_cast<int>(0x887700d2), "DPERR_NOPLAYERS"},
    {static_cast<int>(0x887700dc), "DPERR_NOSESSIONS"},
    {static_cast<int>(0x887700e6), "DPERR_SENDTOOBIG"},
    {static_cast<int>(0x887700f0), "DPERR_TIMEOUT"},
    {static_cast<int>(0x887700fa), "DPERR_UNAVAILABLE"},
    {static_cast<int>(0x8877010e), "DPERR_BUSY"},
    {static_cast<int>(0x88770118), "DPERR_USERCANCEL"},
    {static_cast<int>(0x88770122), "DPERR_CANNOTCREATESERVER"},
    {static_cast<int>(0x8877012c), "DPERR_PLAYERLOST "},
    {static_cast<int>(0x88770136), "DPERR_SESSIONLOST"},
    {static_cast<int>(0x88770140), "DPERR_UNINITIALIZED "},
    {static_cast<int>(0x8877014a), "DPERR_NONEWPLAYERS"},
    {static_cast<int>(0x8877015e), "DPERR_CONNECTING"},
    {static_cast<int>(0x88770168), "DPERR_CONNECTIONLOST"},
    {static_cast<int>(0x88770172), "DPERR_UNKNOWNMESSAGE"},
    {static_cast<int>(0x8877017c), "DPERR_CANCELFAILED"},
    {static_cast<int>(0x88770186), "DPERR_INVALIDPRIORITY"},
    {static_cast<int>(0x887703e8), "DPERR_BUFFERTOOLARGE"},
    {static_cast<int>(0x887703f2), "DPERR_CANTCREATEPROCESS"},
    {static_cast<int>(0x887703fc), "DPERR_APPNOTSTARTED"},
    {static_cast<int>(0x88770406), "DPERR_INVALIDINTERFACE "},
    {static_cast<int>(0x8877041a), "DPERR_UNKNOWNAPPLICATION"},
    {static_cast<int>(0x8877042e), "DPERR_NOTLOBBIED"},
    {static_cast<int>(0x887707d0), "DPERR_AUTHENTICATIONFAILED"},
    {static_cast<int>(0x887707da), "DPERR_CANTLOADSSPI"},
    {static_cast<int>(0x887707e4), "DPERR_ENCRYPTIONFAILED"},
    {static_cast<int>(0x887707ee), "DPERR_SIGNFAILED"},
    {static_cast<int>(0x887707f8), "DPERR_CANTLOADSECURITYPACKAGE"},
    {static_cast<int>(0x88770802), "DPERR_ENCRYPTIONNOTSUPPORTED"},
    {static_cast<int>(0x8877080c), "DPERR_CANTLOADCAPI"},
    {static_cast<int>(0x88770816), "DPERR_NOTLOGGEDIN"},
    {static_cast<int>(0x88770820), "DPERR_LOGONDENIED "},
};

const char *GetDPlayErrorName(int hresult) {
    {
        int entryIndex1;
        for (entryIndex1 = 0; entryIndex1 < (int)(sizeof(kDPlayErrorNames) / sizeof((kDPlayErrorNames)[0])); ++entryIndex1) {
            const DPlayErrorName & entry = (kDPlayErrorNames)[entryIndex1];
        if (entry.hresult == hresult) {
            return entry.name;
        }
    }
    }

    return "UNKNOWN";
}
} // namespace

// Reimplements 0x489f70: zNetwork_GetLocalPlayerKey (D:\Proj\Battlesport\zNetwork\zNetwork.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_CDECL zNetwork_GetLocalPlayerKey() {
    return g_zNetwork_LocalPlayerKey;
}

// Reimplements 0x48b980: zNetwork_GetLocalPlayerColorIndex (D:\Proj\GameZRecoil\zNetwork.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_CDECL zNetwork_GetLocalPlayerColorIndex() {
    if (g_zNetwork_LocalPlayerRecord == 0) {
        return 0;
    }

    return g_zNetwork_LocalPlayerRecord->colorIndex;
}

// Reimplements 0x48b9a0: zNetwork_GetPlayerColorIndexByKey (D:\Proj\GameZRecoil\zNetwork.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zNetwork_GetPlayerColorIndexByKey(int playerKey) {
    zNetwork_PlayerRecord *const playerRecord = zNetwork_FindPlayerRecordByKey(playerKey);
    if (playerRecord == 0 || g_zNetwork_CurrentSessionDescCache == 0) {
        return 0;
    }

    const int colorIndex = playerRecord->colorIndex;
    if (colorIndex < 1 ||
        static_cast<unsigned int>(colorIndex) >
            static_cast<unsigned int>(g_zNetwork_CurrentSessionDescCache->desc.maxPlayers)) {
        return 0;
    }

    return colorIndex;
}

// Reimplements 0x48b9d0: zNetwork_GetPlayerRecordCount (D:\Proj\GameZRecoil\zNetwork\zNetwork.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_CDECL zNetwork_GetPlayerRecordCount() {
    return g_zNetwork_PlayerRecordList->count;
}

// Reimplements 0x48bab0: zNetwork_ExtractStatusFieldsFromSessionDesc
// (D:\Proj\GameZRecoil\zNetwork\zNetwork.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zNetwork_ExtractStatusFieldsFromSessionDesc(zNetworkSessionDescStatusFields *outFields) {
    const zNetworkDPlaySessionDesc sessionDesc = g_zNetwork_CurrentSessionDescCache->desc;

    outFields->eventCode = sessionDesc.customEventCode;
    outFields->statusFlags = sessionDesc.customStatusFlags;
    outFields->valueOrTime = sessionDesc.customValueOrTime;
    outFields->auxParam = sessionDesc.customAuxParam;
    outFields->maxPlayers = sessionDesc.maxPlayers;
    outFields->selectedSessionIndex = -1;

    const size_t sessionNameBytes = strlen(sessionDesc.sessionName) + 1;
    memcpy(outFields->sessionNameBuf, sessionDesc.sessionName, sessionNameBytes);
    return 1;
}

// Reimplements 0x48bb20: zNetwork_ApplyStatusFieldsToSessionDesc
// (D:\Proj\GameZRecoil\zNetwork\zNetwork.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zNetwork_ApplyStatusFieldsToSessionDesc(zNetworkSessionDescStatusFields *statusFields) {
    zNetworkDPlaySessionDesc *const sessionDesc = &g_zNetwork_CurrentSessionDescCache->desc;

    sessionDesc->customEventCode = statusFields->eventCode;
    sessionDesc->customStatusFlags = statusFields->statusFlags;
    sessionDesc->customValueOrTime = statusFields->valueOrTime;
    sessionDesc->customAuxParam = statusFields->auxParam;
    sessionDesc->maxPlayers = statusFields->maxPlayers;

    memcpy(sessionDesc->sessionName, statusFields->sessionNameBuf,
                strlen(statusFields->sessionNameBuf) + 1);

    const int hresult = g_zNetwork_pDirectPlay4->vtbl_00->SetSessionDesc_7c(
        g_zNetwork_pDirectPlay4, sessionDesc, 0);
    if (hresult < 0) {
        return 0;
    }

    memcpy(g_zNetwork_SessionNameCache, sessionDesc->sessionName,
                strlen(sessionDesc->sessionName) + 1);
    return 1;
}

namespace zNetwork {
// Reimplements 0x489f80: zNetwork::IsHost (D:\Proj\Battlesport\zNetwork\zNetwork.cpp)
RECOIL_NOINLINE int RECOIL_CDECL IsHost() {
    return g_zNetwork_IsHostFlag;
}

// Reimplements 0x48afa0: zNetwork::GetPlayerNameByKey
RECOIL_NOINLINE int RECOIL_FASTCALL
GetPlayerNameByKey(int playerKey, char *destination, unsigned int maxCount) {
    zNetwork_PlayerRecord *const playerRecord = zNetwork_FindPlayerRecordByKey(playerKey);
    if (playerRecord == 0 || playerRecord->playerName == 0) {
        return 0;
    }

    strncpy(destination, playerRecord->playerName, maxCount);
    return 1;
}
} // namespace zNetwork

// Reimplements 0x48acf0: zNetwork_DPlay_SendUnreliable (GameZRecoil/zNetwork/znet_dplay.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zNetwork_DPlay_SendUnreliable(zNetworkPacketHeader *packet, unsigned int packetSizeBytes) {
    const int hresult = g_zNetwork_pDirectPlay4->vtbl_00->Send_68(
        g_zNetwork_pDirectPlay4, g_zNetwork_LocalPlayerRecord->playerKey, 0, 0, packet,
        packetSizeBytes);
    if (hresult != kDPlayPending && hresult < 0) {
        return zNetwork_DPlay_ReportError(hresult, kZNetworkDPlaySourceFile, 0x226);
    }

    return 0;
}

// Reimplements 0x48ad30: zNetwork_DPlay_SendReliable (D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zNetwork_DPlay_SendReliable(zNetworkPacketHeader *packet, unsigned int packetSizeBytes) {
    const int hresult = g_zNetwork_pDirectPlay4->vtbl_00->Send_68(
        g_zNetwork_pDirectPlay4, g_zNetwork_LocalPlayerRecord->playerKey, 0, 1, packet,
        packetSizeBytes);
    if (hresult != 0 && hresult < 0) {
        return zNetwork_DPlay_ReportError(hresult, kZNetworkDPlaySourceFile, 0x234);
    }

    return 0;
}

// Reimplements 0x48ad70: zNetwork_DPlay_SendExUnreliableTracked
// (GameZRecoil/zNetwork/znet_dplay.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL zNetwork_DPlay_SendExUnreliableTracked(
    zNetworkPacketHeader *packet, unsigned int packetSizeBytes) {
    unsigned int flags = 0x600;
    if (packet->packetType == 6) {
        flags = 0x200;
        if (g_zNetwork_LastSendExCompleted == 0) {
            g_zNetwork_pDirectPlay4->vtbl_00->CancelMessage_cc(g_zNetwork_pDirectPlay4,
                                                               g_zNetwork_LastSendExHandle, 0);
        } else {
            g_zNetwork_LastSendExCompleted = 0;
        }
    }

    const int hresult = g_zNetwork_pDirectPlay4->vtbl_00->SendEx_c4(
        g_zNetwork_pDirectPlay4, g_zNetwork_LocalPlayerRecord->playerKey, 0, flags, packet,
        packetSizeBytes, 0, 0, 0, &g_zNetwork_LastSendExHandle);
    if (hresult != kDPlayPending && hresult < 0) {
        return zNetwork_DPlay_ReportError(hresult, kZNetworkDPlaySourceFile, 0x25a);
    }

    return 0;
}

// Reimplements 0x48ae10: zNetwork_DPlay_SendExReliable
// (D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zNetwork_DPlay_SendExReliable(zNetworkPacketHeader *packet, unsigned int packetSizeBytes) {
    unsigned int asyncHandle = 0;
    const int hresult = g_zNetwork_pDirectPlay4->vtbl_00->SendEx_c4(
        g_zNetwork_pDirectPlay4, g_zNetwork_LocalPlayerRecord->playerKey, 0, 0x601, packet,
        packetSizeBytes, 0, 0, 0, &asyncHandle);
    if (hresult != kDPlayPending && hresult < 0) {
        return zNetwork_DPlay_ReportError(hresult, kZNetworkDPlaySourceFile, 0x26f);
    }

    return 0;
}

// Reimplements 0x48c060: zNetwork_SendPacketUnreliable (GameZRecoil/zNetwork/znet_dplay.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zNetwork_SendPacketUnreliable(zNetworkPacketHeader *packet) {
    const unsigned int packetSizeBytes = static_cast<unsigned short>(packet->packetSizeBytes);
    if (g_zNetwork_TcpIpAsyncSendEnabled != 0) {
        return zNetwork_DPlay_SendExUnreliableTracked(packet, packetSizeBytes);
    }

    return zNetwork_DPlay_SendUnreliable(packet, packetSizeBytes);
}

// Reimplements 0x48c080: zNetwork_SendPacketReliable (D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zNetwork_SendPacketReliable(zNetworkPacketHeader *packet) {
    const unsigned int packetSizeBytes = static_cast<unsigned short>(packet->packetSizeBytes);
    if (g_zNetwork_TcpIpAsyncSendEnabled != 0) {
        return zNetwork_DPlay_SendExReliable(packet, packetSizeBytes);
    }

    return zNetwork_DPlay_SendReliable(packet, packetSizeBytes);
}

// Reimplements 0x48c250: zNetwork_DPlay_ReportError
extern "C" RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL
zNetwork_DPlay_ReportError(int hresult, const char *sourceFile, int sourceLine) {
    if (hresult == 0) {
        return 1;
    }

    char errorNameBuffer[0x100];
    sprintf(errorNameBuffer, GetDPlayErrorName(hresult));
    zError::ReportOld(0x400, sourceFile, sourceLine, "DirectPlay Error (0x%08x)[%s]", hresult,
                      errorNameBuffer);
    return 0;
}

// Reimplements 0x48a980: zNetwork_DPlay_DestroyCachedLocalPlayer
extern "C" RECOIL_NOINLINE int RECOIL_CDECL zNetwork_DPlay_DestroyCachedLocalPlayer() {
    zNetwork_PlayerRecord *localPlayer = g_zNetwork_LocalPlayerRecord;
    if (localPlayer == 0) {
        return 0;
    }

    zNetwork_DPlay4 *directPlay = g_zNetwork_pDirectPlay4;
    const int hresult =
        directPlay->vtbl_00->DestroyPlayer_24(directPlay, localPlayer->playerKey);
    if (hresult < 0) {
        return zNetwork_DPlay_ReportError(hresult, kZNetworkDPlaySourceFile, 0x1ba);
    }

    return 1;
}

// Reimplements 0x48ba60: zNetwork_FindPlayerRecordByKey
extern "C" RECOIL_NOINLINE zNetwork_PlayerRecord *RECOIL_FASTCALL
zNetwork_FindPlayerRecordByKey(int playerKey) {
    zNetworkPlayerRecordList *const list = g_zNetwork_PlayerRecordList;
    if (list == 0 || list->sentinelNode == 0) {
        return 0;
    }

    zNetworkPlayerRecordListNode *const sentinel = list->sentinelNode;
    for (zNetworkPlayerRecordListNode *node = sentinel->next; node != sentinel; node = node->next) {
        zNetwork_PlayerRecord *const playerRecord = node->playerRecord;
        if (playerRecord != 0 &&
            playerRecord->playerKey == static_cast<unsigned int>(playerKey)) {
            return playerRecord;
        }
    }

    return 0;
}

namespace zNetwork {
// Reimplements 0x48b940: zNetwork::AllocFreePlayerColorIndex
RECOIL_NOINLINE int RECOIL_CDECL AllocFreePlayerColorIndex() {
    const int maxPlayers = g_zNetwork_CurrentSessionDescCache->desc.maxPlayers;
    for (int colorIndex = 1;
         static_cast<unsigned int>(colorIndex) <= static_cast<unsigned int>(maxPlayers);
         ++colorIndex) {
        if (g_zNetwork_PlayerColorInUseFlags[colorIndex] == 0) {
            g_zNetwork_PlayerColorInUseFlags[colorIndex] = 1;
            return colorIndex;
        }
    }

    return 0;
}

// Reimplements 0x48b860: zNetwork::HostSendPlayerColorAssignmentsPacket
RECOIL_NOINLINE void RECOIL_FASTCALL HostSendPlayerColorAssignmentsPacket(int joiningPlayerKey) {
    if (IsHost() == 0) {
        return;
    }

    zNetwork_PlayerRecord *const joiningPlayer =
        zNetwork_FindPlayerRecordByKey(joiningPlayerKey);
    if (joiningPlayer == 0) {
        return;
    }

    if (joiningPlayer->colorIndex <= 0) {
        joiningPlayer->colorIndex = AllocFreePlayerColorIndex();
    }

    const int playerCount = zNetwork_GetPlayerRecordCount();
    const int packetSizeBytes =
        static_cast<int>(sizeof(zNetworkPacketHeader) + sizeof(int) +
                         (sizeof(zNetworkPlayerColorPair) * playerCount));
    NetPkt01_PlayerColorAssignments *const packet =
        static_cast<NetPkt01_PlayerColorAssignments *>(malloc(packetSizeBytes));
    memset(packet, 0, packetSizeBytes);
    packet->header.packetType = 1;
    packet->header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    packet->header.packetSizeBytes = static_cast<short>(packetSizeBytes);
    packet->pairCount = playerCount;

    zNetworkPlayerRecordList *const list = g_zNetwork_PlayerRecordList;
    zNetworkPlayerRecordListNode *const sentinel = list->sentinelNode;
    int pairIndex = 0;
    for (zNetworkPlayerRecordListNode *node = sentinel->next; node != sentinel;
         node = node->next) {
        zNetwork_PlayerRecord *const playerRecord = node->playerRecord;
        packet->pairs[pairIndex].playerKey = static_cast<int>(playerRecord->playerKey);
        packet->pairs[pairIndex].colorIndex = playerRecord->colorIndex;
        ++pairIndex;
    }

    zNetwork_SendPacketReliable(&packet->header);
    free(packet);
}
} // namespace zNetwork

namespace zNetworkDPlay {
// Reimplements 0x48afe0: zNetworkDPlay::PumpIncomingMessages
RECOIL_NOINLINE int RECOIL_FASTCALL
PumpIncomingMessages(zNetworkDPlaySystemMessage *systemMessage) {
    zNetworkPacketHeader packet;
    const int msgType = systemMessage->msgType;

    if (msgType == 0x21 || msgType == 7) {
        return 0;
    }

    if (msgType == 3) {
        zNetwork_PlayerRecord *playerRecord =
            static_cast<zNetwork_PlayerRecord *>(::operator new(sizeof(zNetwork_PlayerRecord)));
        if (playerRecord != 0) {
            strncpy(playerRecord->playerName, "noname", 0x50);
            playerRecord->playerName[0x4f] = 0;
        }

        playerRecord->playerKey = systemMessage->fields.playerId;
        playerRecord->playerNameInfo.size = systemMessage->fields.createFlagsOrPlayerType;
        playerRecord->playerNameInfo.flags = systemMessage->fields.nameShortOrAsyncHandle;
        playerRecord->playerNameInfo.shortName = systemMessage->fields.nameLong;
        playerRecord->playerNameInfo.longName = systemMessage->fields.nameDisplay;
        strcpy(playerRecord->playerName, playerRecord->playerNameInfo.longName);
        strcpy(playerRecord->altName, playerRecord->playerNameInfo.shortName);
        playerRecord->colorIndex = 0;

        zNetworkPlayerRecordList *const list = g_zNetwork_PlayerRecordList;
        zNetworkPlayerRecordListNode *const sentinel = list->sentinelNode;
        zNetworkPlayerRecordListNode *prev = sentinel->prev;
        zNetworkPlayerRecordListNode *const node =
            static_cast<zNetworkPlayerRecordListNode *>(
                ::operator new(sizeof(zNetworkPlayerRecordListNode)));
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
        zNetwork_DPlay::DispatchPacketToHandlers(systemMessage->fields.playerId, &packet);
        if (zNetwork::IsHost() != 0) {
            zNetwork::HostSendPlayerColorAssignmentsPacket(systemMessage->fields.playerId);
        }

        return 0;
    }

    if (msgType == 5) {
        packet.packetType = 3;
        packet.packetSizeBytes = 8;
        packet.payloadDword0 = 0;
        zNetwork_DPlay::DispatchPacketToHandlers(systemMessage->fields.playerId, &packet);
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
        zNetwork_DPlay::DispatchPacketToHandlers(systemMessage->fields.playerId, &packet);
        return 0;
    }

    if (msgType == 0x103) {
        packet.packetType = 5;
        packet.packetSizeBytes = 8;
        packet.payloadDword0 = 0;
        zNetwork_DPlay::DispatchPacketToHandlers(systemMessage->fields.playerId, &packet);
        return 0;
    }

    if (msgType == 0x104) {
        memcpy(&g_zNetwork_CurrentSessionDescCache->desc, systemMessage->payload_004,
               sizeof(zNetworkDPlaySessionDesc));
        return 0;
    }

    if (msgType == 0x10d) {
        if (systemMessage->fields.nameShortOrAsyncHandle == g_zNetwork_LastSendExHandle) {
            g_zNetwork_LastSendExCompleted = 1;
        }
        return 0;
    }

    zError::ReportOld(0x200, kZNetworkDPlaySourceFile, 0x346,
                      "Unhandled DirectPlay system message");
    return 0;
}
} // namespace zNetworkDPlay

namespace zNetwork {
// Reimplements 0x48b9e0: zNetwork::RemovePlayerRecordByKey
RECOIL_NOINLINE void RECOIL_FASTCALL RemovePlayerRecordByKey(int playerKey) {
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
// Reimplements 0x48ae70: zNetworkDPlay::ReceivePendingMessages
RECOIL_NOINLINE int RECOIL_FASTCALL ReceivePendingMessages(int messageBudget) {
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
        const int hresult = g_zNetwork_pDirectPlay4->vtbl_00->Receive_64(
            g_zNetwork_pDirectPlay4, &fromPlayer, &toPlayer, 1, g_zNetwork_ReceiveBuffer,
            &receiveBufferCapacity);

        if (hresult == kDPlayBufferTooSmall) {
            const unsigned int oldCapacity = g_zNetwork_ReceiveBufferCapacity;
            g_zNetwork_ReceiveBuffer = realloc(g_zNetwork_ReceiveBuffer, receiveBufferCapacity);
            zError::ReportOld(0x100, kZNetworkDPlaySourceFile, 0x299,
                              "Receiving buffer size increased from %d to %d", oldCapacity,
                              receiveBufferCapacity);
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
                    static_cast<int>(fromPlayer),
                    static_cast<zNetworkPacketHeader *>(g_zNetwork_ReceiveBuffer));
            } else {
                pumpResult = PumpIncomingMessages(
                    static_cast<zNetworkDPlaySystemMessage *>(g_zNetwork_ReceiveBuffer));
            }
        }

        if (messageBudget == 0 || pumpResult != 0) {
            break;
        }
    }

    return processedCount;
}

// Reimplements 0x48b660: zNetworkDPlay::EnumPlayerCallback_AddPlayerRecord
RECOIL_NOINLINE int RECOIL_STDCALL EnumPlayerCallback_AddPlayerRecord(
    unsigned int playerId, unsigned int, zNetworkDPlayName *playerNameInfo, unsigned int,
    void *) {
    if (zNetwork_FindPlayerRecordByKey(static_cast<int>(playerId)) != 0) {
        return 1;
    }

    zNetwork_PlayerRecord *const playerRecord =
        static_cast<zNetwork_PlayerRecord *>(::operator new(sizeof(zNetwork_PlayerRecord)));
    strncpy(playerRecord->playerName, playerNameInfo->shortName, 0x50);
    playerRecord->playerName[0x4f] = 0;
    playerRecord->playerKey = playerId;

    zNetworkPlayerRecordList *const list = g_zNetwork_PlayerRecordList;
    zNetworkPlayerRecordListNode *const sentinel = list->sentinelNode;
    zNetworkPlayerRecordListNode *prev = sentinel->prev;
    zNetworkPlayerRecordListNode *const node =
        static_cast<zNetworkPlayerRecordListNode *>(::operator new(sizeof(zNetworkPlayerRecordListNode)));

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
// Reimplements 0x48a310: zNetwork_DPlay::EnumPlayers
RECOIL_NOINLINE int RECOIL_CDECL EnumPlayers() {
    zNetwork_DPlay4 *const directPlay = g_zNetwork_pDirectPlay4;
    const int hresult = directPlay->vtbl_00->EnumPlayers_30(
        directPlay, 0, zNetworkDPlay::EnumPlayerCallback_AddPlayerRecord, 0, 0);
    if (hresult < 0) {
        return zNetwork_DPlay_ReportError(hresult, kZNetworkDPlaySourceFile, 0xd9);
    }

    return g_zNetwork_PlayerRecordList->count;
}
} // namespace zNetwork_DPlay

// Reimplements 0x48bff0: zNetwork_DestroyDispatchHandlerList
extern "C" RECOIL_NOINLINE void RECOIL_CDECL zNetwork_DestroyDispatchHandlerList() {
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

// Reimplements 0x48bfe0: zNetwork_RegisterDispatchHandlerListShutdown
extern "C" RECOIL_NOINLINE void RECOIL_CDECL zNetwork_RegisterDispatchHandlerListShutdown() {
    atexit(zNetwork_DestroyDispatchHandlerList);
}

// Reimplements 0x48bfb0: zNetwork_CreateEmptyDispatchHandlerList
extern "C" RECOIL_NOINLINE void RECOIL_CDECL zNetwork_CreateEmptyDispatchHandlerList() {
    g_zNetwork_DispatchHandlerListFlags = 0;
    zNetworkDispatchHandlerListNode *const sentinel = static_cast<zNetworkDispatchHandlerListNode *>(
        ::operator new(sizeof(zNetworkDispatchHandlerListNode)));
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
    g_zNetwork_DispatchHandlerListSentinel = sentinel;
    g_zNetwork_DispatchHandlerListCount = 0;
}

// Reimplements 0x48bfa0: zNetwork_InitMessageHandlers
extern "C" RECOIL_NOINLINE void RECOIL_CDECL zNetwork_InitMessageHandlers() {
    zNetwork_CreateEmptyDispatchHandlerList();
    zNetwork_RegisterDispatchHandlerListShutdown();
}

// Reimplements 0x48b820: zNetwork_ApplyPkt01_PlayerColorAssignments
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zNetwork_ApplyPkt01_PlayerColorAssignments(int, zNetworkPacketHeader *packet) {
    NetPkt01_PlayerColorAssignments * assignments = (NetPkt01_PlayerColorAssignments *)(packet);
    const int assignmentCount = assignments->pairCount;
    for (int i = 0; i < assignmentCount; ++i) {
        const zNetworkPlayerColorPair &pair = assignments->pairs[i];
        zNetwork_PlayerRecord *const playerRecord = zNetwork_FindPlayerRecordByKey(pair.playerKey);
        if (playerRecord != 0) {
            playerRecord->colorIndex = pair.colorIndex;
            if (pair.colorIndex >= 0 && pair.colorIndex < 16) {
                g_zNetwork_PlayerColorInUseFlags[pair.colorIndex] = 1;
            }
        }
    }

    return assignmentCount;
}

namespace zNetwork_DPlay {
// Reimplements 0x48b7f0: zNetwork_DPlay::CloseReleaseAndCoUninitialize
RECOIL_NOINLINE int RECOIL_FASTCALL
CloseReleaseAndCoUninitialize(zNetwork_DPlay4 *directPlay4) {
    int releaseRefCount = 0;
    if (directPlay4 != 0) {
        typedef int (RECOIL_STDCALL *DPlayMethod)(zNetwork_DPlay4 * self);
        void *const *const vtbl = (void *const *)(directPlay4->vtbl_00);
        DPlayMethod const close = (DPlayMethod)(vtbl[4]);
        DPlayMethod const release = (DPlayMethod)(vtbl[2]);

        close(directPlay4);
        releaseRefCount = release(directPlay4);
    }

    CoUninitialize();
    return releaseRefCount;
}
} // namespace zNetwork_DPlay

namespace {
void FreeServiceProviderInfo(zNetworkDPlayServiceProviderInfo *info) {
    if (info == 0) {
        return;
    }

    free(info->displayName);
    info->displayName = 0;
    free(info->connectionData);
    info->connectionData = 0;
    ::operator delete(info);
}

void DeletePlayerRecordNode(zNetworkPlayerRecordListNode *node) {
    if (node->playerRecord != 0) {
        ::operator delete(node->playerRecord);
        node->playerRecord = 0;
    }

    ::operator delete(node);
}
} // namespace

namespace zNetwork {
// Reimplements 0x48c0a0: zNetwork::RegisterPacketHandler
// (D:\Proj\GameZRecoil\zNetwork\zNetwork.cpp)
RECOIL_NOINLINE zNetworkDispatchHandlerRecord *RECOIL_FASTCALL
RegisterPacketHandler(int packetType, zNetworkPacketHandler handlerProc, int mode) {
    zNetworkDispatchHandlerRecord *const record =
        static_cast<zNetworkDispatchHandlerRecord *>(
            ::operator new(sizeof(zNetworkDispatchHandlerRecord)));
    if (record != 0) {
        record->packetType = static_cast<short>(packetType);
        record->handler = handlerProc;
        record->mode = mode;
    }

    zNetworkDispatchHandlerListNode *const sentinel = g_zNetwork_DispatchHandlerListSentinel;
    zNetworkDispatchHandlerListNode *const node =
        static_cast<zNetworkDispatchHandlerListNode *>(
            ::operator new(sizeof(zNetworkDispatchHandlerListNode)));
    zNetworkDispatchHandlerListNode *const prev = sentinel->prev;
    node->next = sentinel != 0 ? sentinel : node;
    node->prev = prev != 0 ? prev : node;
    sentinel->prev = node;
    node->prev->next = node;
    node->record = record;
    ++g_zNetwork_DispatchHandlerListCount;

    return record;
}

// Reimplements 0x48c120: zNetwork::UnregisterPacketHandler
RECOIL_NOINLINE int RECOIL_FASTCALL
UnregisterPacketHandler(int packetType, zNetworkPacketHandler handlerProc) {
    zNetworkDispatchHandlerListNode *const sentinel = g_zNetwork_DispatchHandlerListSentinel;
    if (sentinel == 0) {
        return 1;
    }

    zNetworkDispatchHandlerListNode *node = sentinel->next;
    while (node != sentinel) {
        zNetworkDispatchHandlerListNode *const next = node->next;
        zNetworkDispatchHandlerRecord *const record = node->record;
        if (record != 0 && record->packetType == packetType &&
            record->handler == handlerProc) {
            node->prev->next = node->next;
            node->next->prev = node->prev;
            ::operator delete(node);
            --g_zNetwork_DispatchHandlerListCount;
        }

        node = next;
    }

    return 1;
}
} // namespace zNetwork

namespace zNetwork_DPlay {
// Reimplements 0x48c200: zNetwork_DPlay::DispatchPacketToHandlers
RECOIL_NOINLINE void RECOIL_FASTCALL
DispatchPacketToHandlers(int senderPlayerId, zNetworkPacketHeader *packet) {
    zNetworkDispatchHandlerListNode *const sentinel = g_zNetwork_DispatchHandlerListSentinel;
    zNetworkDispatchHandlerListNode *node = sentinel->next;
    while (node != sentinel) {
        zNetworkDispatchHandlerRecord *const record = node->record;
        if (record->packetType == packet->packetType) {
            record->handler(senderPlayerId, packet);
        }

        node = node->next;
    }
}
} // namespace zNetwork_DPlay

namespace zNetwork {
// Reimplements 0x489f30: zNetwork::ClearEnumeratedSessionList
RECOIL_NOINLINE void RECOIL_CDECL ClearEnumeratedSessionList() {
    zNetworkDPlaySessionDesc *desc = static_cast<zNetworkDPlaySessionDesc *>(
        zArchiveList_PopFrontPayload(g_zNetwork_EnumeratedSessionList));
    while (desc != 0) {
        free(desc->reservedData);
        free(desc);
        desc = static_cast<zNetworkDPlaySessionDesc *>(
            zArchiveList_PopFrontPayload(g_zNetwork_EnumeratedSessionList));
    }
}

// Reimplements 0x489fa0: zNetwork::ClearServiceProviderList
RECOIL_NOINLINE void RECOIL_CDECL ClearServiceProviderList() {
    zNetworkServiceProviderListVec *const list = g_zNetwork_ServiceProviderList;
    if (list == 0) {
        return;
    }

    for (zNetworkDPlayServiceProviderInfo **it = list->begin; it != list->end; ++it) {
        FreeServiceProviderInfo(*it);
        *it = 0;
    }

    list->end = list->begin;
}

// Reimplements 0x48a030: zNetwork::ClearPlayerRecordList
RECOIL_NOINLINE void RECOIL_CDECL ClearPlayerRecordList() {
    zNetworkPlayerRecordList *const list = g_zNetwork_PlayerRecordList;
    if (list == 0 || list->sentinelNode == 0) {
        return;
    }

    zNetworkPlayerRecordListNode *const sentinel = list->sentinelNode;
    zNetworkPlayerRecordListNode *node = sentinel->next;
    while (node != sentinel) {
        zNetworkPlayerRecordListNode *const next = node->next;
        DeletePlayerRecordNode(node);
        node = next;
    }

    sentinel->next = sentinel;
    sentinel->prev = sentinel;
    list->count = 0;
}

// Reimplements 0x489e10: zNetwork::ShutdownSessionRuntime
RECOIL_NOINLINE int RECOIL_CDECL ShutdownSessionRuntime() {
    zNetwork_DPlay::CloseReleaseAndCoUninitialize(g_zNetwork_pDirectPlay4);
    g_zNetwork_SessionRuntimeInitialized = 0;
    UnregisterPacketHandler(1, zNetwork_ApplyPkt01_PlayerColorAssignments);

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
    if (g_zNetwork_PlayerRecordList != 0) {
        ::operator delete(g_zNetwork_PlayerRecordList->sentinelNode);
        g_zNetwork_PlayerRecordList->sentinelNode = 0;
        g_zNetwork_PlayerRecordList->count = 0;
        ::operator delete(g_zNetwork_PlayerRecordList);
        g_zNetwork_PlayerRecordList = 0;
    }

    if (g_zNetwork_ReceiveBuffer != 0) {
        free(g_zNetwork_ReceiveBuffer);
        g_zNetwork_ReceiveBuffer = 0;
    }

    return 0;
}
} // namespace zNetwork
