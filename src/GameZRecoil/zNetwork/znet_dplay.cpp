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

extern "C" {
zNetwork_DPlay4 *g_zNetwork_pDirectPlay4 = 0;
zNetwork_PlayerRecord *g_zNetwork_LocalPlayerRecord = 0;
int g_zNetwork_IsHostFlag = 0;
int g_zNetwork_LocalPlayerKey = 0;
char g_zNetwork_LocalPlayerNameScratch[0x50] = {0};
int g_zNetwork_TcpIpAsyncSendEnabled = 0;
int g_zNetwork_ActiveProviderIsModem = 0;
int g_zNetwork_ActiveProviderIsTcpIp = 0;
zNetworkDPlayCaps g_zNetwork_DPlayCaps = {0};
unsigned char *g_zNetwork_AppGuid = 0;
unsigned char g_zNetwork_RecoilAppGuid[16] = {0xa2,
    0xbc,
    0x4e,
    0xc9,
    0xb7,
    0x95,
    0xd2,
    0x11,
    0xa7,
    0x7c,
    0x00,
    0x60,
    0x08,
    0x98,
    0x77,
    0x43};
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
const int kDPlayPending = (int)(0x8000000a);
const int kDPlayBufferTooSmall = (int)(0x8877001e);
const int kDPlayConnecting = (int)(0x8877015e);

struct DPlayErrorName {
    int hresult;
    const char *name;
};

const DPlayErrorName kDPlayErrorNames[] = {
    {(int)(0x8000000a), "DPERR_PENDING"},
    {(int)(0x80004001), "DPERR_UNSUPPORTED"},
    {(int)(0x80004005), "DPERR_GENERIC"},
    {(int)(0x8007000e), "DPERR_OUTOFMEMORY"},
    {(int)(0x80070057), "DPERR_INVALIDPARAMS"},
    {(int)(0x88770005), "DPERR_ALREADYINITIALIZED"},
    {(int)(0x8877000a), "DPERR_ACCESSDENIED"},
    {(int)(0x88770014), "DPERR_ACTIVEPLAYERS"},
    {(int)(0x8877001e), "DPERR_BUFFERTOOSMALL"},
    {(int)(0x88770028), "DPERR_CANTADDPLAYER"},
    {(int)(0x88770032), "DPERR_CANTCREATEGROUP:"},
    {(int)(0x8877003c), "DPERR_CANTCREATEPLAYER"},
    {(int)(0x88770046), "DPERR_CANTCREATESESSION"},
    {(int)(0x88770050), "DPERR_CAPSNOTAVAILABLEYET"},
    {(int)(0x8877005a), "DPERR_EXCEPTION"},
    {(int)(0x88770078), "DPERR_INVALIDFLAGS"},
    {(int)(0x88770082), "DPERR_INVALIDOBJECT"},
    {(int)(0x88770096), "DPERR_INVALIDPLAYER"},
    {(int)(0x8877009b), "DPERR_INVALIDGROUP"},
    {(int)(0x887700a0), "DPERR_NOCAPS"},
    {(int)(0x887700aa), "DPERR_NOCONNECTION"},
    {(int)(0x887700be), "DPERR_NOMESSAGES "},
    {(int)(0x887700c8), "DPERR_NONAMESERVERFOUND"},
    {(int)(0x887700d2), "DPERR_NOPLAYERS"},
    {(int)(0x887700dc), "DPERR_NOSESSIONS"},
    {(int)(0x887700e6), "DPERR_SENDTOOBIG"},
    {(int)(0x887700f0), "DPERR_TIMEOUT"},
    {(int)(0x887700fa), "DPERR_UNAVAILABLE"},
    {(int)(0x8877010e), "DPERR_BUSY"},
    {(int)(0x88770118), "DPERR_USERCANCEL"},
    {(int)(0x88770122), "DPERR_CANNOTCREATESERVER"},
    {(int)(0x8877012c), "DPERR_PLAYERLOST "},
    {(int)(0x88770136), "DPERR_SESSIONLOST"},
    {(int)(0x88770140), "DPERR_UNINITIALIZED "},
    {(int)(0x8877014a), "DPERR_NONEWPLAYERS"},
    {(int)(0x8877015e), "DPERR_CONNECTING"},
    {(int)(0x88770168), "DPERR_CONNECTIONLOST"},
    {(int)(0x88770172), "DPERR_UNKNOWNMESSAGE"},
    {(int)(0x8877017c), "DPERR_CANCELFAILED"},
    {(int)(0x88770186), "DPERR_INVALIDPRIORITY"},
    {(int)(0x887703e8), "DPERR_BUFFERTOOLARGE"},
    {(int)(0x887703f2), "DPERR_CANTCREATEPROCESS"},
    {(int)(0x887703fc), "DPERR_APPNOTSTARTED"},
    {(int)(0x88770406), "DPERR_INVALIDINTERFACE "},
    {(int)(0x8877041a), "DPERR_UNKNOWNAPPLICATION"},
    {(int)(0x8877042e), "DPERR_NOTLOBBIED"},
    {(int)(0x887707d0), "DPERR_AUTHENTICATIONFAILED"},
    {(int)(0x887707da), "DPERR_CANTLOADSSPI"},
    {(int)(0x887707e4), "DPERR_ENCRYPTIONFAILED"},
    {(int)(0x887707ee), "DPERR_SIGNFAILED"},
    {(int)(0x887707f8), "DPERR_CANTLOADSECURITYPACKAGE"},
    {(int)(0x88770802), "DPERR_ENCRYPTIONNOTSUPPORTED"},
    {(int)(0x8877080c), "DPERR_CANTLOADCAPI"},
    {(int)(0x88770816), "DPERR_NOTLOGGEDIN"},
    {(int)(0x88770820), "DPERR_LOGONDENIED "},
};

const char *GetDPlayErrorName(
    int hresult
) {
    {
        int entryIndex1;
        for (entryIndex1 = 0;
            entryIndex1 < (int)(sizeof(kDPlayErrorNames) / sizeof((kDPlayErrorNames)[0]));
            ++entryIndex1) {
            const DPlayErrorName &entry = (kDPlayErrorNames)[entryIndex1];
            if (entry.hresult == hresult) {
                return entry.name;
            }
        }
    }

    return "UNKNOWN";
}

int ReportDPlayOpenFailure(
    int hresult
) {
    const char *message = 0;
    switch (hresult) {
    case (int)(0x80070057) :
        message = "Sorry, Invalid Parameters";
        break;
    case (int)(0x88770005) :
        message = "Already Initialized";
        break;
    case (int)(0x8877000a) :
        message = "Access Denied";
        break;
    case (int)(0x88770078) :
        message = "Sorry, Invalid Flags";
        break;
    case (int)(0x887700aa) :
        message = "No Connection";
        break;
    case (int)(0x887700dc) :
        message = "No Existing Sessions";
        break;
    case (int)(0x887700f0) :
        message = "Timeout Error";
        break;
    case (int)(0x88770122) :
        message = "Cannot Create Server";
        break;
    case (int)(0x88770140) :
        message = "Initialization Error";
        break;
    case (int)(0x8877014a) :
        message = "No New Players Allowed";
        break;
    case (int)(0x88770154) :
        message = "Invalid Password";
        break;
    case (int)(0x8877015e) :
        message = "Error Connecting";
        break;
    case (int)(0x88770168) :
        message = "Connection Lost";
        break;
    case (int)(0x887707d0) :
        message = "Authentication Failed";
        break;
    case (int)(0x887707da) :
        message = "Security Support Provider Error";
        break;
    case (int)(0x887707e4) :
        message = "Encryption Failed";
        break;
    case (int)(0x887707ee) :
        message = "Signature Failure";
        break;
    case (int)(0x887707f8) :
        message = "Cannot Load Security Package";
        break;
    case (int)(0x88770802) :
        message = "Encryption Not Supported";
        break;
    case (int)(0x8877080c) :
        message = "Cryptography Services Error";
        break;
    case (int)(0x88770820) :
        message = "Logon Denied";
        break;
    }

    if (message != 0) {
        MessageBoxA(
            g_RecoilApp_hWndMain,
            message,
            "Recoil Network Error",
            MB_OK
        );
    }

    return 0;
}

void AppendServiceProviderInfo(
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
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL zNetwork_GetPlayerColorIndexByKey(
    int playerKey
) {
    zNetwork_PlayerRecord *const playerRecord = zNetwork_FindPlayerRecordByKey(playerKey);
    if (playerRecord == 0 || g_zNetwork_CurrentSessionDescCache == 0) {
        return 0;
    }

    const int colorIndex = playerRecord->colorIndex;
    if (colorIndex < 1 || (unsigned int)(colorIndex) >
                              (unsigned int)(g_zNetwork_CurrentSessionDescCache->desc.maxPlayers)) {
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
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL zNetwork_ExtractStatusFieldsFromSessionDesc(
    zNetworkSessionDescStatusFields *outFields
) {
    const zNetworkDPlaySessionDesc sessionDesc = g_zNetwork_CurrentSessionDescCache->desc;

    outFields->eventCode = sessionDesc.customEventCode;
    outFields->statusFlags = sessionDesc.customStatusFlags;
    outFields->valueOrTime = sessionDesc.customValueOrTime;
    outFields->auxParam = sessionDesc.customAuxParam;
    outFields->maxPlayers = sessionDesc.maxPlayers;
    outFields->selectedSessionIndex = -1;

    const size_t sessionNameBytes = strlen(sessionDesc.sessionName) + 1;
    memcpy(
        outFields->sessionNameBuf,
        sessionDesc.sessionName,
        sessionNameBytes
    );
    return 1;
}

// Reimplements 0x48bb20: zNetwork_ApplyStatusFieldsToSessionDesc
// (D:\Proj\GameZRecoil\zNetwork\zNetwork.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL zNetwork_ApplyStatusFieldsToSessionDesc(
    zNetworkSessionDescStatusFields *statusFields
) {
    zNetworkDPlaySessionDesc *const sessionDesc = &g_zNetwork_CurrentSessionDescCache->desc;

    sessionDesc->customEventCode = statusFields->eventCode;
    sessionDesc->customStatusFlags = statusFields->statusFlags;
    sessionDesc->customValueOrTime = statusFields->valueOrTime;
    sessionDesc->customAuxParam = statusFields->auxParam;
    sessionDesc->maxPlayers = statusFields->maxPlayers;

    memcpy(
        sessionDesc->sessionName,
        statusFields->sessionNameBuf,
        strlen(statusFields->sessionNameBuf) + 1
    );

    const int hresult = g_zNetwork_pDirectPlay4->vtbl_00
                            ->SetSessionDesc_7c(
                                g_zNetwork_pDirectPlay4,
                                sessionDesc,
                                0
                            );
    if (hresult < 0) {
        return 0;
    }

    memcpy(
        g_zNetwork_SessionNameCache,
        sessionDesc->sessionName,
        strlen(sessionDesc->sessionName) + 1
    );
    return 1;
}

namespace zNetwork {
// Reimplements 0x489f80: zNetwork::IsHost (D:\Proj\Battlesport\zNetwork\zNetwork.cpp)
RECOIL_NOINLINE int RECOIL_CDECL IsHost() {
    return g_zNetwork_IsHostFlag;
}

// Reimplements 0x48afa0: zNetwork::GetPlayerNameByKey
RECOIL_NOINLINE int RECOIL_FASTCALL GetPlayerNameByKey(
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

// Reimplements 0x48acf0: zNetwork_DPlay_SendUnreliable (GameZRecoil/zNetwork/znet_dplay.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL zNetwork_DPlay_SendUnreliable(
    zNetworkPacketHeader *packet,
    unsigned int packetSizeBytes
) {
    const int hresult = g_zNetwork_pDirectPlay4->vtbl_00->Send_68(
        g_zNetwork_pDirectPlay4,
        g_zNetwork_LocalPlayerRecord->playerKey,
        0,
        0,
        packet,
        packetSizeBytes
    );
    if (hresult != kDPlayPending && hresult < 0) {
        return zNetwork_DPlay_ReportError(
            hresult,
            kZNetworkDPlaySourceFile,
            0x226
        );
    }

    return 0;
}

// Reimplements 0x48ad30: zNetwork_DPlay_SendReliable (D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL zNetwork_DPlay_SendReliable(
    zNetworkPacketHeader *packet,
    unsigned int packetSizeBytes
) {
    const int hresult = g_zNetwork_pDirectPlay4->vtbl_00->Send_68(
        g_zNetwork_pDirectPlay4,
        g_zNetwork_LocalPlayerRecord->playerKey,
        0,
        1,
        packet,
        packetSizeBytes
    );
    if (hresult != 0 && hresult < 0) {
        return zNetwork_DPlay_ReportError(
            hresult,
            kZNetworkDPlaySourceFile,
            0x234
        );
    }

    return 0;
}

// Reimplements 0x48ad70: zNetwork_DPlay_SendExUnreliableTracked
// (GameZRecoil/zNetwork/znet_dplay.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL zNetwork_DPlay_SendExUnreliableTracked(
    zNetworkPacketHeader *packet,
    unsigned int packetSizeBytes
) {
    unsigned int flags = 0x600;
    if (packet->packetType == 6) {
        flags = 0x200;
        if (g_zNetwork_LastSendExCompleted == 0) {
            g_zNetwork_pDirectPlay4->vtbl_00
                ->CancelMessage_cc(
                    g_zNetwork_pDirectPlay4,
                    g_zNetwork_LastSendExHandle,
                    0
                );
        } else {
            g_zNetwork_LastSendExCompleted = 0;
        }
    }

    const int hresult = g_zNetwork_pDirectPlay4->vtbl_00->SendEx_c4(
        g_zNetwork_pDirectPlay4,
        g_zNetwork_LocalPlayerRecord->playerKey,
        0,
        flags,
        packet,
        packetSizeBytes,
        0,
        0,
        0,
        &g_zNetwork_LastSendExHandle
    );
    if (hresult != kDPlayPending && hresult < 0) {
        return zNetwork_DPlay_ReportError(
            hresult,
            kZNetworkDPlaySourceFile,
            0x25a
        );
    }

    return 0;
}

// Reimplements 0x48ae10: zNetwork_DPlay_SendExReliable
// (D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL zNetwork_DPlay_SendExReliable(
    zNetworkPacketHeader *packet,
    unsigned int packetSizeBytes
) {
    unsigned int asyncHandle = 0;
    const int hresult = g_zNetwork_pDirectPlay4->vtbl_00->SendEx_c4(
        g_zNetwork_pDirectPlay4,
        g_zNetwork_LocalPlayerRecord->playerKey,
        0,
        0x601,
        packet,
        packetSizeBytes,
        0,
        0,
        0,
        &asyncHandle
    );
    if (hresult != kDPlayPending && hresult < 0) {
        return zNetwork_DPlay_ReportError(
            hresult,
            kZNetworkDPlaySourceFile,
            0x26f
        );
    }

    return 0;
}

// Reimplements 0x48c060: zNetwork_SendPacketUnreliable (GameZRecoil/zNetwork/znet_dplay.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL zNetwork_SendPacketUnreliable(
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

// Reimplements 0x48c080: zNetwork_SendPacketReliable (D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL zNetwork_SendPacketReliable(
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

// Reimplements 0x48c250: zNetwork_DPlay_ReportError
extern "C" RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL zNetwork_DPlay_ReportError(
    int hresult,
    const char *sourceFile,
    int sourceLine
) {
    if (hresult == 0) {
        return 1;
    }

    char errorNameBuffer[0x100];
    sprintf(
        errorNameBuffer,
        GetDPlayErrorName(hresult)
    );
    zError::ReportOld(
        0x400,
        sourceFile,
        sourceLine,
        "DirectPlay Error (0x%08x)[%s]",
        hresult,
        errorNameBuffer
    );
    return 0;
}

// Reimplements 0x48a980: zNetwork_DPlay_DestroyCachedLocalPlayer
extern "C" RECOIL_NOINLINE int RECOIL_CDECL zNetwork_DPlay_DestroyCachedLocalPlayer() {
    zNetwork_PlayerRecord *localPlayer = g_zNetwork_LocalPlayerRecord;
    if (localPlayer == 0) {
        return 0;
    }

    zNetwork_DPlay4 *directPlay = g_zNetwork_pDirectPlay4;
    const int hresult = directPlay->vtbl_00->DestroyPlayer_24(
        directPlay,
        localPlayer->playerKey
    );
    if (hresult < 0) {
        return zNetwork_DPlay_ReportError(
            hresult,
            kZNetworkDPlaySourceFile,
            0x1ba
        );
    }

    return 1;
}

// Reimplements 0x48ba60: zNetwork_FindPlayerRecordByKey
extern "C" RECOIL_NOINLINE zNetwork_PlayerRecord *RECOIL_FASTCALL zNetwork_FindPlayerRecordByKey(
    int playerKey
) {
    zNetworkPlayerRecordList *const list = g_zNetwork_PlayerRecordList;
    if (list == 0 || list->sentinelNode == 0) {
        return 0;
    }

    zNetworkPlayerRecordListNode *const sentinel = list->sentinelNode;
    for (zNetworkPlayerRecordListNode *node = sentinel->next; node != sentinel; node = node->next) {
        zNetwork_PlayerRecord *const playerRecord = node->playerRecord;
        if (playerRecord != 0 && playerRecord->playerKey == (unsigned int)(playerKey)) {
            return playerRecord;
        }
    }

    return 0;
}

namespace zNetwork {
// Reimplements 0x48b940: zNetwork::AllocFreePlayerColorIndex
RECOIL_NOINLINE int RECOIL_CDECL AllocFreePlayerColorIndex() {
    const int maxPlayers = g_zNetwork_CurrentSessionDescCache->desc.maxPlayers;
    for (int colorIndex = 1; (unsigned int)(colorIndex) <= (unsigned int)(maxPlayers);
        ++colorIndex) {
        if (g_zNetwork_PlayerColorInUseFlags[colorIndex] == 0) {
            g_zNetwork_PlayerColorInUseFlags[colorIndex] = 1;
            return colorIndex;
        }
    }

    return 0;
}

// Reimplements 0x48b860: zNetwork::HostSendPlayerColorAssignmentsPacket
RECOIL_NOINLINE void RECOIL_FASTCALL HostSendPlayerColorAssignmentsPacket(
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
// Reimplements 0x48b3a0: zNetworkDPlay::EnumConnectionsCallback_AddServiceProviderInfo
// (D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp)
RECOIL_NOINLINE int RECOIL_STDCALL EnumConnectionsCallback_AddServiceProviderInfo(
    unsigned char *serviceProviderGuid,
    void *connectionData,
    unsigned int connectionDataSize,
    zNetworkDPlayName *providerName,
    unsigned int providerFlags,
    void *
) {
    zNetworkDPlayServiceProviderInfo *providerInfo =
        (zNetworkDPlayServiceProviderInfo *)(::operator new(
            sizeof(zNetworkDPlayServiceProviderInfo)
        ));
    if (providerInfo != 0) {
        memcpy(
            providerInfo->serviceProviderGuid,
            serviceProviderGuid,
            sizeof(providerInfo->serviceProviderGuid)
        );
        providerInfo->displayName = _strdup(providerName->shortName);
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
// Reimplements 0x48a0d0: zNetwork_DPlay::RefreshServiceProviderList
// (D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp)
RECOIL_NOINLINE int RECOIL_CDECL RefreshServiceProviderList() {
    zNetwork::ClearServiceProviderList();

    zNetwork_DPlay4 *const directPlay = g_zNetwork_pDirectPlay4;
    const int hresult = directPlay->vtbl_00->EnumConnections_8c(
        directPlay,
        g_zNetwork_AppGuid,
        zNetworkDPlay::EnumConnectionsCallback_AddServiceProviderInfo,
        g_RecoilApp_hWndMain,
        0
    );
    if (hresult < 0) {
        return zNetwork_DPlay_ReportError(
            hresult,
            kZNetworkDPlaySourceFile,
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
// Reimplements 0x48a130: zNetworkDPlay::RefreshAndGetServiceProviderList
// (D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp)
RECOIL_NOINLINE zNetworkServiceProviderListVec *RECOIL_CDECL RefreshAndGetServiceProviderList() {
    zNetwork_DPlay::RefreshServiceProviderList();
    return g_zNetwork_ServiceProviderList;
}

// Reimplements 0x48a180: zNetworkDPlay::SelectServiceProviderAndInitConnection
RECOIL_NOINLINE int RECOIL_FASTCALL SelectServiceProviderAndInitConnection(
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
            kZNetworkDPlaySourceFile,
            0x8f
        );
    }

    if (g_zNetwork_pDirectPlay4 == 0) {
        return 0;
    }

    g_zNetwork_ActiveProviderIsModem = strstr(
        providerInfo->displayName,
        "Modem"
    ) != 0;
    g_zNetwork_ActiveProviderIsTcpIp = strstr(
        providerInfo->displayName,
        "TCP/IP"
    ) != 0;
    g_zNetwork_TcpIpAsyncSendEnabled = g_zNetwork_ActiveProviderIsTcpIp;

    return InitializeConnectionFromProviderInfo(providerInfo);
}

// Reimplements 0x48a140: zNetworkDPlay::InitializeConnectionFromProviderInfo
RECOIL_NOINLINE int RECOIL_FASTCALL InitializeConnectionFromProviderInfo(
    zNetworkDPlayServiceProviderInfo *providerInfo
) {
    const int kDPlayUserCancel = (int)(0x88770118);
    zNetwork_DPlay4 *const directPlay = g_zNetwork_pDirectPlay4;
    const int hresult =
        directPlay->vtbl_00->InitializeConnection_98(
            directPlay,
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
        kZNetworkDPlaySourceFile,
        0x7d
    );
}

// Reimplements 0x48a2c0: zNetworkDPlay::GetEnumeratedSessionNameByIndex
RECOIL_NOINLINE char *RECOIL_FASTCALL GetEnumeratedSessionNameByIndex(
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

    return entry->desc.sessionName;
}

// Reimplements 0x48a2e0: zNetworkDPlay::GetEnumeratedSessionPlayerCountsByIndex
RECOIL_NOINLINE void RECOIL_FASTCALL GetEnumeratedSessionPlayerCountsByIndex(
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
        *maxPlayersOut = entry->desc.maxPlayers;
        *currentPlayersOut = entry->desc.currentPlayers;
    }
}

// Reimplements 0x48b5e0: zNetworkDPlay::EnumSessionCallback_AddSessionDescCache
RECOIL_NOINLINE int RECOIL_STDCALL EnumSessionCallback_AddSessionDescCache(
    zNetworkDPlaySessionDesc *sessionDesc,
    unsigned int *,
    unsigned int,
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
    memcpy(
        cache->desc.instanceGuid,
        sessionDesc->instanceGuid,
        sizeof(cache->desc.instanceGuid)
    );
    cache->desc.sessionName = _strdup(sessionDesc->sessionName);
    zArchiveList_PushBackPayload(
        g_zNetwork_EnumeratedSessionList,
        cache
    );
    return 1;
}

// Reimplements 0x48a350: zNetworkDPlay::QueryCapsAndConfigureSendMode
// (D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp)
RECOIL_NOINLINE int RECOIL_CDECL QueryCapsAndConfigureSendMode() {
    zNetwork_DPlay4 *const directPlay = g_zNetwork_pDirectPlay4;
    g_zNetwork_DPlayCaps.size = sizeof(zNetworkDPlayCaps);
    const int hresult = directPlay->vtbl_00->GetCaps_38(
        directPlay,
        &g_zNetwork_DPlayCaps,
        1
    );
    if (hresult < 0) {
        fprintf(
            stderr,
            "Failed to get network capabilities\n"
        );
        return zNetwork_DPlay_ReportError(
            hresult,
            kZNetworkDPlaySourceFile,
            0xea
        );
    }

    if (g_zNetwork_ActiveProviderIsTcpIp != 0) {
        const int flags = g_zNetwork_DPlayCaps.flags;
        if ((flags & 0x40) == 0) {
            fprintf(
                stderr,
                "Guaranteed TCP/IP not supported n"
            );
            return 0;
        }

        if ((flags & 0x10000) == 0) {
            g_zNetwork_TcpIpAsyncSendEnabled = 0;
        }

        printf(
            "Network using TCP/IP %s\n",
            g_zNetwork_TcpIpAsyncSendEnabled != 0 ? "ASYNCH" : "SYNCH"
        );
    }

    return 1;
}

// Reimplements 0x48afe0: zNetworkDPlay::PumpIncomingMessages
RECOIL_NOINLINE int RECOIL_FASTCALL PumpIncomingMessages(
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
                "noname",
                0x50
            );
            playerRecord->playerName[0x4f] = 0;
        }

        playerRecord->playerKey = systemMessage->fields.playerId;
        playerRecord->playerNameInfo.size = systemMessage->fields.createFlagsOrPlayerType;
        playerRecord->playerNameInfo.flags = systemMessage->fields.nameShortOrAsyncHandle;
        playerRecord->playerNameInfo.shortName = systemMessage->fields.nameLong;
        playerRecord->playerNameInfo.longName = systemMessage->fields.nameDisplay;
        strcpy(
            playerRecord->playerName,
            playerRecord->playerNameInfo.longName
        );
        strcpy(
            playerRecord->altName,
            playerRecord->playerNameInfo.shortName
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
        kZNetworkDPlaySourceFile,
        0x346,
        "Unhandled DirectPlay system message"
    );
    return 0;
}
} // namespace zNetworkDPlay

namespace zNetwork {
// Reimplements 0x48b9e0: zNetwork::RemovePlayerRecordByKey
RECOIL_NOINLINE void RECOIL_FASTCALL RemovePlayerRecordByKey(
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
// Reimplements 0x48ae70: zNetworkDPlay::ReceivePendingMessages
RECOIL_NOINLINE int RECOIL_FASTCALL ReceivePendingMessages(
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
        const int hresult = g_zNetwork_pDirectPlay4->vtbl_00->Receive_64(
            g_zNetwork_pDirectPlay4,
            &fromPlayer,
            &toPlayer,
            1,
            g_zNetwork_ReceiveBuffer,
            &receiveBufferCapacity
        );

        if (hresult == kDPlayBufferTooSmall) {
            const unsigned int oldCapacity = g_zNetwork_ReceiveBufferCapacity;
            g_zNetwork_ReceiveBuffer = realloc(
                g_zNetwork_ReceiveBuffer,
                receiveBufferCapacity
            );
            zError::ReportOld(
                0x100,
                kZNetworkDPlaySourceFile,
                0x299,
                "Receiving buffer size increased from %d to %d",
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

// Reimplements 0x48b660: zNetworkDPlay::EnumPlayerCallback_AddPlayerRecord
RECOIL_NOINLINE int RECOIL_STDCALL EnumPlayerCallback_AddPlayerRecord(
    unsigned int playerId,
    unsigned int,
    zNetworkDPlayName *playerNameInfo,
    unsigned int,
    void *
) {
    if (zNetwork_FindPlayerRecordByKey((int)(playerId)) != 0) {
        return 1;
    }

    zNetwork_PlayerRecord *const playerRecord =
        (zNetwork_PlayerRecord *)(::operator new(sizeof(zNetwork_PlayerRecord)));
    strncpy(
        playerRecord->playerName,
        playerNameInfo->shortName,
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
// Reimplements 0x48a220: zNetwork_DPlay::EnumSessions
RECOIL_NOINLINE int RECOIL_CDECL EnumSessions() {
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
    desc.size = sizeof(zNetworkDPlaySessionDesc);
    memcpy(
        desc.appGuid,
        g_zNetwork_AppGuid,
        sizeof(desc.appGuid)
    );

    const int hresult = directPlay->vtbl_00->EnumSessions_34(
        directPlay,
        &desc,
        0,
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
            kZNetworkDPlaySourceFile,
            0xb3
        );
    }

    return zArchiveList_GetCount(g_zNetwork_EnumeratedSessionList);
}

// Reimplements 0x48a310: zNetwork_DPlay::EnumPlayers
RECOIL_NOINLINE int RECOIL_CDECL EnumPlayers() {
    zNetwork_DPlay4 *const directPlay = g_zNetwork_pDirectPlay4;
    const int hresult = directPlay->vtbl_00->EnumPlayers_30(
        directPlay,
        0,
        zNetworkDPlay::EnumPlayerCallback_AddPlayerRecord,
        0,
        0
    );
    if (hresult < 0) {
        return zNetwork_DPlay_ReportError(
            hresult,
            kZNetworkDPlaySourceFile,
            0xd9
        );
    }

    return g_zNetwork_PlayerRecordList->count;
}

// Reimplements 0x48a9c0: zNetwork_DPlay::CreateLocalPlayerRecordAndRegister
// (D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL CreateLocalPlayerRecordAndRegister(
    char *playerName
) {
    zNetwork_PlayerRecord *const localPlayerRecord =
        (zNetwork_PlayerRecord *)(::operator new(sizeof(zNetwork_PlayerRecord)));
    if (localPlayerRecord != 0) {
        strncpy(
            localPlayerRecord->playerName,
            "noname",
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
    localPlayerRecord->playerNameInfo.shortName = g_zNetwork_LocalPlayerNameScratch;
    localPlayerRecord->playerNameInfo.longName = g_zNetwork_LocalPlayerNameScratch;
    localPlayerRecord->createPlayerEventHandle = 0;
    localPlayerRecord->playerNameInfo.size = sizeof(zNetworkDPlayName);
    localPlayerRecord->playerNameInfo.flags = 0;
    memcpy(
        localPlayerRecord->playerName,
        g_zNetwork_LocalPlayerNameScratch,
        strlen(g_zNetwork_LocalPlayerNameScratch) + 1
    );
    memcpy(
        localPlayerRecord->altName,
        localPlayerRecord->playerNameInfo.shortName,
        strlen(localPlayerRecord->playerNameInfo.shortName) + 1
    );

    zNetwork_DPlay4 *const directPlay = g_zNetwork_pDirectPlay4;
    const int createResult = directPlay->vtbl_00->CreatePlayer_18(
        directPlay,
        &localPlayerRecord->playerKey,
        &localPlayerRecord->playerNameInfo,
        localPlayerRecord->createPlayerEventHandle,
        0,
        0,
        0
    );
    if (createResult < 0) {
        if (createResult == (int)(0x88770028)) {
            MessageBoxA(
                g_RecoilApp_hWndMain,
                "Cannot Add Another Player",
                "Recoil Network Error",
                MB_OK
            );
        } else if (createResult == (int)(0x80070057) || createResult == (int)(0x88770078)) {
            MessageBoxA(
                g_RecoilApp_hWndMain,
                "Sorry, Invalid Player Parameters",
                "Recoil Network Error",
                MB_OK
            );
        } else if (createResult == (int)(0x8877003c)) {
            MessageBoxA(
                g_RecoilApp_hWndMain,
                "Cannot Create Another Player",
                "Recoil Network Error",
                MB_OK
            );
        } else if (createResult == (int)(0x887700aa)) {
            MessageBoxA(
                g_RecoilApp_hWndMain,
                "No Network Connection",
                "Recoil Network Error",
                MB_OK
            );
        } else if (createResult == (int)(0x88770168)) {
            MessageBoxA(
                g_RecoilApp_hWndMain,
                "Your Network Connection Has Been Lost",
                "Recoil Network Error",
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
    localPlayerRecord->playerCaps.size = sizeof(zNetworkDPlayCaps);
    const int capsResult = directPlay->vtbl_00->GetPlayerCaps_4c(
        directPlay,
        localPlayerRecord->playerKey,
        &localPlayerRecord->playerCaps,
        0
    );
    g_zNetwork_IsHostFlag = localPlayerRecord->playerCaps.flags & 2;
    zNetworkDPlay::ReceivePendingMessages(-1);
    if (capsResult < 0) {
        return zNetwork_DPlay_ReportError(
            capsResult,
            kZNetworkDPlaySourceFile,
            0x20e
        );
    }

    zNetworkPlayerRecordList *const list = g_zNetwork_PlayerRecordList;
    g_zNetworkCurrentPlayerCountCached =
        g_zNetwork_CurrentSessionDescCache->desc.currentPlayers + 1;
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

// Reimplements 0x48a410: zNetwork_DPlay::CreateSessionFromStatusFields
// (D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL CreateSessionFromStatusFields(
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
    cache->desc.flags = 0x44;
    cache->desc.size = sizeof(zNetworkDPlaySessionDesc);
    memcpy(
        cache->desc.appGuid,
        g_zNetwork_AppGuid,
        sizeof(cache->desc.appGuid)
    );
    cache->desc.maxPlayers = statusFields->maxPlayers;
    cache->desc.customEventCode = statusFields->eventCode;
    cache->desc.customStatusFlags = statusFields->statusFlags;
    cache->desc.customValueOrTime = statusFields->valueOrTime;
    cache->desc.customAuxParam = statusFields->auxParam;
    cache->desc.sessionName = _strdup(g_zNetwork_SessionNameCache);

    zNetwork_DPlay4 *const directPlay = g_zNetwork_pDirectPlay4;
    const int hresult = directPlay->vtbl_00->Open_60(
        directPlay,
        &cache->desc,
        2
    );
    if (hresult == (int)(0x88770118)) {
        return 0;
    }

    if (hresult < 0) {
        return zNetwork_DPlay_ReportError(
            hresult,
            kZNetworkDPlaySourceFile,
            0x11e
        );
    }

    if (zNetworkDPlay::QueryCapsAndConfigureSendMode() == 0) {
        directPlay->vtbl_00->Close_10(directPlay);
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
// Reimplements 0x48a520: zNetworkDPlay::OpenSelectedSessionAndReadStatusFields
// (D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL OpenSelectedSessionAndReadStatusFields(
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
    sessionCache->desc.size = sizeof(zNetworkDPlaySessionDesc);

    zNetwork_DPlay4 *const directPlay = g_zNetwork_pDirectPlay4;
    const int openResult = directPlay->vtbl_00->Open_60(
        directPlay,
        &sessionCache->desc,
        1
    );
    if (openResult < 0) {
        return ReportDPlayOpenFailure(openResult);
    }

    if (QueryCapsAndConfigureSendMode() == 0) {
        directPlay->vtbl_00->Close_10(directPlay);
        return 0;
    }

    statusFields->eventCode = sessionCache->desc.customEventCode;
    statusFields->statusFlags = sessionCache->desc.customStatusFlags;
    statusFields->valueOrTime = sessionCache->desc.customValueOrTime;
    statusFields->auxParam = sessionCache->desc.customAuxParam;
    statusFields->maxPlayers = sessionCache->desc.maxPlayers;
    memcpy(
        statusFields->sessionNameBuf,
        sessionCache->desc.sessionName,
        strlen(sessionCache->desc.sessionName) + 1
    );
    return 1;
}
} // namespace zNetworkDPlay

namespace zNetwork {
// Reimplements 0x48bf40: zNetwork::DeleteAllDispatchHandlers
// (D:\Proj\GameZRecoil\zNetwork\zNetwork.cpp)
RECOIL_NOINLINE void RECOIL_CDECL DeleteAllDispatchHandlers() {
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
// Reimplements 0x48bbe0: zNetworkDPlay::SelectTcpIpProviderAndEnumSessions
// (D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL SelectTcpIpProviderAndEnumSessions(
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
    memcpy(
        providerInfo.serviceProviderGuid,
        &DPSPGUID_TCPIP,
        sizeof(providerInfo.serviceProviderGuid)
    );
    providerInfo.displayName = _strdup("forced TCP/IP");
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

// Reimplements 0x48be10: zNetworkDPlay::CreateLobby3AInterface
// (D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL CreateLobby3AInterface(
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

// Reimplements 0x48be70: zNetworkDPlay::EnumSessionsForCurrentApp
// (D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp)
RECOIL_NOINLINE int RECOIL_CDECL EnumSessionsForCurrentApp() {
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
    desc.size = sizeof(desc);
    memcpy(
        desc.appGuid,
        g_zNetwork_AppGuid,
        sizeof(desc.appGuid)
    );
    return directPlay->vtbl_00->EnumSessions_34(
        directPlay,
        &desc,
        0,
        zNetworkDPlay::EnumSessionCallback_AddSessionDescCache,
        0,
        0x82
    );
}
} // namespace zNetworkDPlay

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
    zNetworkDispatchHandlerListNode *const sentinel =
        (zNetworkDispatchHandlerListNode *)(::operator new(
            sizeof(zNetworkDispatchHandlerListNode)
        ));
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
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL zNetwork_ApplyPkt01_PlayerColorAssignments(
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
            if (pair.colorIndex >= 0 && pair.colorIndex < 16) {
                g_zNetwork_PlayerColorInUseFlags[pair.colorIndex] = 1;
            }
        }
    }

    return assignmentCount;
}

namespace zNetwork_DPlay {
// Reimplements 0x48b730: zNetwork_DPlay::CreateInterfaceAndCoInitialize
RECOIL_NOINLINE int RECOIL_FASTCALL CreateInterfaceAndCoInitialize(
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
            kZNetworkDPlaySourceFile,
            0x394,
            "Class not registered"
        );
    }

    if (hresult == kClassCannotBeCreated) {
        zError::ReportOld(
            0x400,
            kZNetworkDPlaySourceFile,
            0x396,
            "Class cannot be created"
        );
    }

    if (hresult == kCoCreateNotInitialized) {
        zError::ReportOld(
            0x400,
            kZNetworkDPlaySourceFile,
            0x398,
            "CoCreate not initialized"
        );
        return hresult;
    }

    zNetwork_DPlay_ReportError(
        hresult,
        kZNetworkDPlaySourceFile,
        0x39a
    );
    return hresult;
}

// Reimplements 0x48b7f0: zNetwork_DPlay::CloseReleaseAndCoUninitialize
RECOIL_NOINLINE int RECOIL_FASTCALL CloseReleaseAndCoUninitialize(
    zNetwork_DPlay4 *directPlay4
) {
    int releaseRefCount = 0;
    if (directPlay4 != 0) {
        directPlay4->vtbl_00->Close_10(directPlay4);
        releaseRefCount = directPlay4->vtbl_00->Release_08(directPlay4);
    }

    CoUninitialize();
    return releaseRefCount;
}
} // namespace zNetwork_DPlay

namespace {
void FreeServiceProviderInfo(
    zNetworkDPlayServiceProviderInfo *info
) {
    if (info == 0) {
        return;
    }

    free(info->displayName);
    info->displayName = 0;
    free(info->connectionData);
    info->connectionData = 0;
    ::operator delete(info);
}

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
// Reimplements 0x48c0a0: zNetwork::RegisterPacketHandler
// (D:\Proj\GameZRecoil\zNetwork\zNetwork.cpp)
RECOIL_NOINLINE zNetworkDispatchHandlerRecord *RECOIL_FASTCALL RegisterPacketHandler(
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

// Reimplements 0x48c120: zNetwork::UnregisterPacketHandler
RECOIL_NOINLINE int RECOIL_FASTCALL UnregisterPacketHandler(
    int packetType,
    zNetworkPacketHandler handlerProc
) {
    zNetworkDispatchHandlerListNode *const sentinel = g_zNetwork_DispatchHandlerListSentinel;
    if (sentinel == 0) {
        return 1;
    }

    zNetworkDispatchHandlerListNode *node = sentinel->next;
    while (node != sentinel) {
        zNetworkDispatchHandlerListNode *const next = node->next;
        zNetworkDispatchHandlerRecord *const record = node->record;
        if (record != 0 && record->packetType == packetType && record->handler == handlerProc) {
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
RECOIL_NOINLINE void RECOIL_FASTCALL DispatchPacketToHandlers(
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
// Reimplements 0x489f30: zNetwork::ClearEnumeratedSessionList
RECOIL_NOINLINE void RECOIL_CDECL ClearEnumeratedSessionList() {
    zNetworkDPlaySessionDesc *desc = (zNetworkDPlaySessionDesc *)(zArchiveList_PopFrontPayload(
        g_zNetwork_EnumeratedSessionList
    ));
    while (desc != 0) {
        free(desc->reservedData);
        free(desc);
        desc = (zNetworkDPlaySessionDesc *)(zArchiveList_PopFrontPayload(
            g_zNetwork_EnumeratedSessionList
        ));
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

// Reimplements 0x489f90: zNetwork::SetFatalDisconnectCallback
// (D:\Proj\GameZRecoil\zNetwork\znet_dplay.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SetFatalDisconnectCallback(
    zNetworkFatalDisconnectCallback callback
) {
    g_zNetwork_FatalDisconnectCallback = callback;
}

// Reimplements 0x489d00: zNetwork::InitSessionRuntime
// (D:\Proj\GameZRecoil\zNetwork\zNetwork.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL InitSessionRuntime(
    unsigned char *appGuid
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

// Reimplements 0x489e10: zNetwork::ShutdownSessionRuntime
RECOIL_NOINLINE int RECOIL_CDECL ShutdownSessionRuntime() {
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
