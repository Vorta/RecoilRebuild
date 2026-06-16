#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zReader/zReader.h"

#include <cstdlib>
#include <cstring>
#include <new>

namespace {
int g_closeCalls;
int g_releaseCalls;
int g_getCapsCalls;
DWORD g_getCapsFlags;
HRESULT g_getCapsResult;
DWORD g_getCapsReturnedFlags;
int g_openCalls;
zNetworkDPlaySessionDesc *g_openDescPtr;
DWORD g_openFlags;
HRESULT g_openResult;
int g_setSessionDescCalls;
zNetworkDPlaySessionDesc *g_setSessionDescPtr;
DWORD g_setSessionDescFlags;
HRESULT g_setSessionDescResult;
int g_enumPlayersCalls;
HRESULT g_enumPlayersResult;
int g_createPlayerCalls;
DWORD *g_createPlayerIdPtr;
zNetworkDPlayName *g_createPlayerNameInfo;
void *g_createPlayerEventHandle;
void *g_createPlayerData;
DWORD g_createPlayerDataSize;
DWORD g_createPlayerFlags;
DWORD g_createPlayerAssignedId;
HRESULT g_createPlayerResult;
int g_getPlayerCapsCalls;
DWORD g_getPlayerCapsPlayerId;
DWORD g_getPlayerCapsFlags;
HRESULT g_getPlayerCapsResult;
DWORD g_getPlayerCapsReturnedFlags;
int g_receiveCalls;
HRESULT g_receiveResults[4];
DWORD g_receiveFrom[4];
DWORD g_receiveTo[4];
DWORD g_receiveSizes[4];
unsigned char g_receivePayloads[4][128];
int g_sendCalls;
DWORD g_sendFromPlayer;
DWORD g_sendFlags;
void *g_sendPacket;
DWORD g_sendPacketSize;
HRESULT g_sendResult;
int g_sendExCalls;
DWORD g_sendExFlags;
DWORD g_sendExAsyncValue;
int g_cancelCalls;
DWORD g_cancelHandle;
DWORD g_cancelFlags;
int g_dispatchCallsA;
int g_dispatchCallsB;
int g_dispatchSenderA;
int g_dispatchSenderB;
zNetworkPacketHeader *g_dispatchPacketA;
zNetworkPacketHeader *g_dispatchPacketB;
int g_dispatchPacketTypeA;
int g_dispatchPacketTypeB;
int g_dispatchPacketPayloadA;
int g_dispatchPacketPayloadB;
int g_fatalDisconnectCalls;
int g_fatalDisconnectReason;

HRESULT __stdcall FakeDirectPlayClose(
    zNetwork_DPlay4 *
) {
    ++g_closeCalls;
    return 0;
}

ULONG __stdcall FakeDirectPlayRelease(
    zNetwork_DPlay4 *
) {
    ++g_releaseCalls;
    return 17;
}

HRESULT __stdcall FakeDirectPlayGetCaps(
    zNetwork_DPlay4 *,
    zNetworkDPlayCaps *caps,
    DWORD flags
) {
    ++g_getCapsCalls;
    g_getCapsFlags = flags;
    if (caps != 0) {
        caps->dwFlags = g_getCapsReturnedFlags;
    }
    return g_getCapsResult;
}

HRESULT __stdcall FakeDirectPlayOpen(
    zNetwork_DPlay4 *,
    zNetworkDPlaySessionDesc *sessionDesc,
    DWORD flags
) {
    ++g_openCalls;
    g_openDescPtr = sessionDesc;
    g_openFlags = flags;
    return g_openResult;
}

HRESULT __stdcall FakeDirectPlaySetSessionDesc(
    zNetwork_DPlay4 *,
    zNetworkDPlaySessionDesc *sessionDesc,
    DWORD flags
) {
    ++g_setSessionDescCalls;
    g_setSessionDescPtr = sessionDesc;
    g_setSessionDescFlags = flags;
    return g_setSessionDescResult;
}

HRESULT __stdcall FakeDirectPlayEnumPlayers(
    zNetwork_DPlay4 *,
    void *,
    zNetworkDPlayEnumPlayersCallback callback,
    void *,
    DWORD
) {
    ++g_enumPlayersCalls;
    if (g_enumPlayersResult < 0) {
        return g_enumPlayersResult;
    }

    char aceName[] = "Ace";
    char duplicateName[] = "Duplicate";
    char longName[] = "Long";
    zNetworkDPlayName ace = {};
    ace.dwSize = sizeof(zNetworkDPlayName);
    ace.lpszShortNameA = aceName;
    ace.lpszLongNameA = longName;
    zNetworkDPlayName duplicate = {};
    duplicate.dwSize = sizeof(zNetworkDPlayName);
    duplicate.lpszShortNameA = duplicateName;
    duplicate.lpszLongNameA = longName;
    callback(
        0x101,
        0,
        &ace,
        0,
        0
    );
    callback(
        0x101,
        0,
        &duplicate,
        0,
        0
    );
    return g_enumPlayersResult;
}

HRESULT __stdcall FakeDirectPlayCreatePlayer(
    zNetwork_DPlay4 *,
    DWORD *playerId,
    zNetworkDPlayName *playerNameInfo,
    void *eventHandle,
    void *data,
    DWORD dataSize,
    DWORD flags
) {
    ++g_createPlayerCalls;
    g_createPlayerIdPtr = playerId;
    g_createPlayerNameInfo = playerNameInfo;
    g_createPlayerEventHandle = eventHandle;
    g_createPlayerData = data;
    g_createPlayerDataSize = dataSize;
    g_createPlayerFlags = flags;
    if (playerId != 0 && g_createPlayerResult >= 0) {
        *playerId = g_createPlayerAssignedId;
    }
    return g_createPlayerResult;
}

HRESULT __stdcall FakeDirectPlayGetPlayerCaps(
    zNetwork_DPlay4 *,
    DWORD playerId,
    zNetworkDPlayCaps *caps,
    DWORD flags
) {
    ++g_getPlayerCapsCalls;
    g_getPlayerCapsPlayerId = playerId;
    g_getPlayerCapsFlags = flags;
    if (caps != 0) {
        caps->dwFlags = g_getPlayerCapsReturnedFlags;
    }
    return g_getPlayerCapsResult;
}

HRESULT __stdcall FakeDirectPlayReceive(
    zNetwork_DPlay4 *,
    DWORD *fromPlayer,
    DWORD *toPlayer,
    DWORD,
    void *packet,
    DWORD *packetSizeBytes
) {
    const int index = g_receiveCalls++;
    if (fromPlayer != 0) {
        *fromPlayer = g_receiveFrom[index];
    }
    if (toPlayer != 0) {
        *toPlayer = g_receiveTo[index];
    }
    if (packetSizeBytes != 0) {
        *packetSizeBytes = g_receiveSizes[index];
    }
    if (packet != 0 && g_receiveResults[index] >= 0) {
        memcpy(
            packet,
            g_receivePayloads[index],
            g_receiveSizes[index]
        );
    }
    return g_receiveResults[index];
}

HRESULT __stdcall FakeDirectPlaySend(
    zNetwork_DPlay4 *,
    DWORD fromPlayer,
    DWORD,
    DWORD flags,
    void *packet,
    DWORD packetSizeBytes
) {
    ++g_sendCalls;
    g_sendFromPlayer = fromPlayer;
    g_sendFlags = flags;
    g_sendPacket = packet;
    g_sendPacketSize = packetSizeBytes;
    return g_sendResult;
}

HRESULT __stdcall FakeDirectPlaySendEx(
    zNetwork_DPlay4 *,
    DWORD fromPlayer,
    DWORD,
    DWORD flags,
    void *packet,
    DWORD packetSizeBytes,
    DWORD,
    DWORD,
    void *,
    DWORD *asyncHandle
) {
    ++g_sendExCalls;
    g_sendFromPlayer = fromPlayer;
    g_sendExFlags = flags;
    g_sendPacket = packet;
    g_sendPacketSize = packetSizeBytes;
    if (asyncHandle != 0) {
        *asyncHandle = g_sendExAsyncValue;
    }
    return g_sendResult;
}

HRESULT __stdcall FakeDirectPlayCancelMessage(
    zNetwork_DPlay4 *,
    DWORD asyncHandle,
    DWORD flags
) {
    ++g_cancelCalls;
    g_cancelHandle = asyncHandle;
    g_cancelFlags = flags;
    return 0;
}

int __fastcall TestPacketHandlerA(
    int,
    zNetworkPacketHeader *
) {
    return 0;
}

int __fastcall TestPacketHandlerB(
    int,
    zNetworkPacketHeader *
) {
    return 0;
}

int __fastcall TestDispatchHandlerA(
    int senderPlayerId,
    zNetworkPacketHeader *packet
) {
    ++g_dispatchCallsA;
    g_dispatchSenderA = senderPlayerId;
    g_dispatchPacketA = packet;
    g_dispatchPacketTypeA = packet != 0 ? packet->packetType : 0;
    g_dispatchPacketPayloadA = packet != 0 ? packet->payloadDword0 : 0;
    return 0;
}

int __fastcall TestDispatchHandlerB(
    int senderPlayerId,
    zNetworkPacketHeader *packet
) {
    ++g_dispatchCallsB;
    g_dispatchSenderB = senderPlayerId;
    g_dispatchPacketB = packet;
    g_dispatchPacketTypeB = packet != 0 ? packet->packetType : 0;
    g_dispatchPacketPayloadB = packet != 0 ? packet->payloadDword0 : 0;
    return 0;
}

void __fastcall TestFatalDisconnect(
    int reason
) {
    ++g_fatalDisconnectCalls;
    g_fatalDisconnectReason = reason;
}

template <typename T>
T *AllocZeroedObject() {
    T *const object = (T *)(::operator new(sizeof(T)));
    memset(
        object,
        0,
        sizeof(T)
    );
    return object;
}

void ResetDirectPlayScenarioState() {
    g_closeCalls = 0;
    g_releaseCalls = 0;
    g_getCapsCalls = 0;
    g_getCapsFlags = 0;
    g_getCapsResult = 0;
    g_getCapsReturnedFlags = 0;
    g_openCalls = 0;
    g_openDescPtr = 0;
    g_openFlags = 0;
    g_openResult = 0;
    g_setSessionDescCalls = 0;
    g_setSessionDescPtr = 0;
    g_setSessionDescFlags = 0;
    g_setSessionDescResult = 0;
    g_enumPlayersCalls = 0;
    g_enumPlayersResult = 0;
    g_createPlayerCalls = 0;
    g_createPlayerIdPtr = 0;
    g_createPlayerNameInfo = 0;
    g_createPlayerEventHandle = 0;
    g_createPlayerData = 0;
    g_createPlayerDataSize = 0;
    g_createPlayerFlags = 0;
    g_createPlayerAssignedId = 0x33334444;
    g_createPlayerResult = 0;
    g_getPlayerCapsCalls = 0;
    g_getPlayerCapsPlayerId = 0;
    g_getPlayerCapsFlags = 0;
    g_getPlayerCapsResult = 0;
    g_getPlayerCapsReturnedFlags = 0;
    g_receiveCalls = 0;
    memset(
        g_receiveResults,
        0,
        sizeof(g_receiveResults)
    );
    memset(
        g_receiveFrom,
        0,
        sizeof(g_receiveFrom)
    );
    memset(
        g_receiveTo,
        0,
        sizeof(g_receiveTo)
    );
    memset(
        g_receiveSizes,
        0,
        sizeof(g_receiveSizes)
    );
    memset(
        g_receivePayloads,
        0,
        sizeof(g_receivePayloads)
    );
    g_sendCalls = 0;
    g_sendFromPlayer = 0;
    g_sendFlags = 0;
    g_sendPacket = 0;
    g_sendPacketSize = 0;
    g_sendResult = 0;
    g_sendExCalls = 0;
    g_sendExFlags = 0;
    g_sendExAsyncValue = 0xabcdef01;
    g_cancelCalls = 0;
    g_cancelHandle = 0;
    g_cancelFlags = 0;
    g_dispatchCallsA = 0;
    g_dispatchCallsB = 0;
    g_dispatchSenderA = 0;
    g_dispatchSenderB = 0;
    g_dispatchPacketA = 0;
    g_dispatchPacketB = 0;
    g_dispatchPacketTypeA = 0;
    g_dispatchPacketTypeB = 0;
    g_dispatchPacketPayloadA = 0;
    g_dispatchPacketPayloadB = 0;
    g_fatalDisconnectCalls = 0;
    g_fatalDisconnectReason = 0;
    g_zNetwork_pDirectPlay4 = 0;
    g_zNetwork_LocalPlayerRecord = 0;
    g_zNetwork_IsHostFlag = 0;
    g_zNetwork_LocalPlayerKey = 0;
    g_zNetwork_ActiveProviderIsTcpIp = 0;
    g_zNetwork_ActiveProviderIsModem = 0;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_zNetwork_DPlayCaps = zNetworkDPlayCaps();
    g_zNetwork_AppGuid = 0;
    g_zNetwork_LastSendExHandle = 0;
    g_zNetwork_LastSendExCompleted = 0;
    g_zNetwork_SessionRuntimeInitialized = 0;
    g_zNetwork_CurrentSessionDescCache = 0;
    g_zNetwork_FatalDisconnectCallback = 0;
    g_zNetwork_FatalDisconnectTriggered = 0;
    g_zNetworkCurrentPlayerCountCached = 0;
    g_zNetwork_EnumeratedSessionList = 0;
    g_zNetwork_ServiceProviderList = 0;
    g_zNetwork_PlayerRecordList = 0;
    g_zNetwork_ReceiveBuffer = 0;
    g_zNetwork_ReceiveBufferCapacity = 0;
    g_zNetwork_DispatchHandlerListSentinel = 0;
    g_zNetwork_DispatchHandlerListCount = 0;
    memset(
        g_zNetwork_PlayerColorInUseFlags,
        0,
        sizeof(g_zNetwork_PlayerColorInUseFlags)
    );
    memset(
        g_zNetwork_SessionNameCache,
        0,
        sizeof(g_zNetwork_SessionNameCache)
    );
}

void BuildMinimalDirectPlayVtable(
    void **vtable
) {
    memset(
        vtable,
        0,
        sizeof(void *) * 26
    );
    vtable[2] = (void *)(&FakeDirectPlayRelease);
    vtable[4] = (void *)(&FakeDirectPlayClose);
    vtable[6] = (void *)(&FakeDirectPlayCreatePlayer);
    vtable[12] = (void *)(&FakeDirectPlayEnumPlayers);
    vtable[14] = (void *)(&FakeDirectPlayGetCaps);
    vtable[19] = (void *)(&FakeDirectPlayGetPlayerCaps);
    vtable[24] = (void *)(&FakeDirectPlayOpen);
    vtable[25] = (void *)(&FakeDirectPlayReceive);
}

void BuildPacketSendDirectPlayVtable(
    void **vtable
) {
    memset(
        vtable,
        0,
        sizeof(void *) * 52
    );
    vtable[26] = (void *)(&FakeDirectPlaySend);
    vtable[49] = (void *)(&FakeDirectPlaySendEx);
    vtable[51] = (void *)(&FakeDirectPlayCancelMessage);
}

void BuildSessionStatusDirectPlayVtable(
    void **vtable
) {
    memset(
        vtable,
        0,
        sizeof(void *) * 52
    );
    vtable[31] = (void *)(&FakeDirectPlaySetSessionDesc);
}
} // namespace

extern "C" int znetwork_local_identity_smoke(void) {
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHostFlag = g_zNetwork_IsHostFlag;
    zNetwork_PlayerRecord *const oldLocalPlayerRecord = g_zNetwork_LocalPlayerRecord;

    g_zNetwork_LocalPlayerKey = 0x12345678;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_LocalPlayerRecord = 0;

    int result = 0;
    if (zNetwork_GetLocalPlayerKey() != 0x12345678 ||
        zNetwork::IsHost() != 1 ||
        zNetwork_GetLocalPlayerColorIndex() != 0) {
        result = 1;
    }

    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.colorIndex = 7;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    if (result == 0 && zNetwork_GetLocalPlayerColorIndex() != 7) {
        result = 2;
    }

    g_zNetwork_IsHostFlag = 0;
    g_zNetwork_LocalPlayerRecord = 0;
    if (result == 0 && zNetwork::IsHost() != 0) {
        result = 3;
    }

    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHostFlag;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayerRecord;
    return result;
}

extern "C" int znetwork_alloc_free_player_color_index_smoke(void) {
    zNetworkDPlaySessionDescCache *const oldCurrentSessionDescCache =
        g_zNetwork_CurrentSessionDescCache;
    int oldColorFlags[16];
    memcpy(
        oldColorFlags,
        g_zNetwork_PlayerColorInUseFlags,
        sizeof(oldColorFlags)
    );

    memset(
        g_zNetwork_PlayerColorInUseFlags,
        0,
        sizeof(g_zNetwork_PlayerColorInUseFlags)
    );

    zNetworkDPlaySessionDescCache session = {};
    session.desc.dwMaxPlayers = 4;
    g_zNetwork_CurrentSessionDescCache = &session;
    g_zNetwork_PlayerColorInUseFlags[1] = 1;
    g_zNetwork_PlayerColorInUseFlags[2] = 1;
    g_zNetwork_PlayerColorInUseFlags[4] = 1;

    int result = 0;
    const int firstFree = zNetwork::AllocFreePlayerColorIndex();
    if (firstFree != 3 || g_zNetwork_PlayerColorInUseFlags[3] != 1) {
        result = 1;
    }

    const int noneFree = zNetwork::AllocFreePlayerColorIndex();
    if (result == 0 && noneFree != 0) {
        result = 2;
    }

    session.desc.dwMaxPlayers = 0;
    g_zNetwork_PlayerColorInUseFlags[1] = 0;
    if (result == 0 &&
        (zNetwork::AllocFreePlayerColorIndex() != 0 ||
         g_zNetwork_PlayerColorInUseFlags[1] != 0)) {
        result = 3;
    }

    g_zNetwork_CurrentSessionDescCache = oldCurrentSessionDescCache;
    memcpy(
        g_zNetwork_PlayerColorInUseFlags,
        oldColorFlags,
        sizeof(g_zNetwork_PlayerColorInUseFlags)
    );
    return result;
}

extern "C" int znetwork_packet_send_wrappers_smoke(void) {
    ResetDirectPlayScenarioState();

    void *vtable[52];
    BuildPacketSendDirectPlayVtable(vtable);

    void **fakeDirectPlay = vtable;
    zNetwork_DPlay4 *const dplay = (zNetwork_DPlay4 *)(&fakeDirectPlay);
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x10203040;
    g_zNetwork_pDirectPlay4 = dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x10203040;

    zNetworkPacketHeader packet = {};
    packet.packetType = 6;
    packet.packetSizeBytes = 12;

    if (zNetwork_SendPacketReliable(&packet) != 0 ||
        g_sendCalls != 1 ||
        g_sendFromPlayer != 0x10203040 ||
        g_sendFlags != 1 ||
        g_sendPacket != &packet ||
        g_sendPacketSize != 12) {
        return 1;
    }

    g_sendCalls = 0;
    if (zNetwork_SendPacketUnreliable(&packet) != 0 ||
        g_sendCalls != 1 ||
        g_sendFlags != 0) {
        return 2;
    }

    g_zNetwork_TcpIpAsyncSendEnabled = 1;
    g_zNetwork_LastSendExHandle = 0x55aa;
    g_zNetwork_LastSendExCompleted = 0;
    if (zNetwork_SendPacketUnreliable(&packet) != 0 ||
        g_cancelCalls != 1 ||
        g_cancelHandle != 0x55aa ||
        g_cancelFlags != 0 ||
        g_sendExCalls != 1 ||
        g_sendExFlags != 0x200 ||
        g_zNetwork_LastSendExHandle != g_sendExAsyncValue) {
        return 3;
    }

    packet.packetType = 7;
    g_sendExCalls = 0;
    g_cancelCalls = 0;
    g_zNetwork_LastSendExCompleted = 1;
    if (zNetwork_SendPacketUnreliable(&packet) != 0 ||
        g_cancelCalls != 0 ||
        g_zNetwork_LastSendExCompleted != 1 ||
        g_sendExCalls != 1 ||
        g_sendExFlags != 0x600) {
        return 4;
    }

    g_sendExCalls = 0;
    if (zNetwork_SendPacketReliable(&packet) != 0 ||
        g_sendExCalls != 1 ||
        g_sendExFlags != 0x601) {
        return 5;
    }

    g_sendResult = (HRESULT)(0x8000000a);
    return zNetwork_SendPacketReliable(&packet) == 0 ? 0 : 6;
}

extern "C" int znetwork_session_status_fields_smoke(void) {
    ResetDirectPlayScenarioState();

    void *vtable[52];
    BuildSessionStatusDirectPlayVtable(vtable);

    void **fakeDirectPlay = vtable;
    zNetworkDPlaySessionDescCache session = {};
    char sessionName[0x5c] = "original";
    session.desc.dwMaxPlayers = 8;
    session.desc.lpszSessionNameA = sessionName;
    session.desc.dwUser1 = 1;
    session.desc.dwUser2 = 2;
    session.desc.dwUser3 = 3;
    session.desc.dwUser4 = 4;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fakeDirectPlay);
    g_zNetwork_CurrentSessionDescCache = &session;

    zNetworkSessionDescStatusFields fields = {};
    if (zNetwork_ExtractStatusFieldsFromSessionDesc(&fields) != 1 ||
        fields.eventCode != 1 ||
        fields.statusFlags != 2 ||
        fields.valueOrTime != 3 ||
        fields.auxParam != 4 ||
        fields.maxPlayers != 8 ||
        fields.selectedSessionIndex != -1 ||
        strcmp(fields.sessionNameBuf, "original") != 0) {
        return 1;
    }

    fields.eventCode = 10;
    fields.statusFlags = 11;
    fields.valueOrTime = 12;
    fields.auxParam = 13;
    fields.maxPlayers = 6;
    strcpy(
        fields.sessionNameBuf,
        "updated"
    );
    if (zNetwork_ApplyStatusFieldsToSessionDesc(&fields) != 1 ||
        g_setSessionDescCalls != 1 ||
        g_setSessionDescPtr != &session.desc ||
        g_setSessionDescFlags != 0 ||
        session.desc.dwUser1 != 10 ||
        session.desc.dwUser2 != 11 ||
        session.desc.dwUser3 != 12 ||
        session.desc.dwUser4 != 13 ||
        session.desc.dwMaxPlayers != 6 ||
        strcmp(sessionName, "updated") != 0 ||
        strcmp(g_zNetwork_SessionNameCache, "updated") != 0) {
        return 2;
    }

    g_setSessionDescResult = (HRESULT)(0x88770014);
    strcpy(
        fields.sessionNameBuf,
        "failed"
    );
    strcpy(
        g_zNetwork_SessionNameCache,
        "updated"
    );
    if (zNetwork_ApplyStatusFieldsToSessionDesc(&fields) != 0 ||
        strcmp(g_zNetwork_SessionNameCache, "updated") != 0) {
        return 3;
    }

    return 0;
}

extern "C" int znetwork_dplay_close_release_smoke(void) {
    g_closeCalls = 0;
    g_releaseCalls = 0;

    if (zNetwork_DPlay::CloseReleaseAndCoUninitialize(0) != 0) {
        return 1;
    }

    void *vtable[5] = {};
    vtable[2] = (void *)(&FakeDirectPlayRelease);
    vtable[4] = (void *)(&FakeDirectPlayClose);

    struct FakeDirectPlay {
        void **vtable;
    } fake = {vtable};

    const int result = zNetwork_DPlay::CloseReleaseAndCoUninitialize(
        (zNetwork_DPlay4 *)(&fake)
    );

    if (result != 17) {
        return 2;
    }

    if (g_closeCalls != 1 || g_releaseCalls != 1) {
        return 3;
    }

    return 0;
}

extern "C" int znetwork_dplay_report_error_smoke(void) {
    if (zNetwork_DPlay_ReportError(0, __FILE__, __LINE__) != 1) {
        return 1;
    }

    return zNetwork_DPlay_ReportError(
               (int)(0x88770014),
               __FILE__,
               __LINE__
           ) == 0
               ? 0
               : 2;
}

extern "C" int znetwork_dplay_query_caps_configure_send_mode_smoke(void) {
    void *vtable[26];
    BuildMinimalDirectPlayVtable(vtable);
    struct FakeDirectPlay {
        void **vtable;
    } fake = {vtable};

    ResetDirectPlayScenarioState();
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);
    if (zNetworkDPlay::QueryCapsAndConfigureSendMode() != 1 ||
        g_getCapsCalls != 1 ||
        g_getCapsFlags != 1 ||
        g_zNetwork_DPlayCaps.dwSize != sizeof(zNetworkDPlayCaps)) {
        return 1;
    }

    ResetDirectPlayScenarioState();
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);
    g_zNetwork_ActiveProviderIsTcpIp = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 1;
    g_getCapsReturnedFlags = 0x40;
    if (zNetworkDPlay::QueryCapsAndConfigureSendMode() != 1 ||
        g_zNetwork_TcpIpAsyncSendEnabled != 0) {
        return 2;
    }

    ResetDirectPlayScenarioState();
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);
    g_zNetwork_ActiveProviderIsTcpIp = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 1;
    g_getCapsReturnedFlags = 0x10040;
    if (zNetworkDPlay::QueryCapsAndConfigureSendMode() != 1 ||
        g_zNetwork_TcpIpAsyncSendEnabled != 1) {
        return 3;
    }

    ResetDirectPlayScenarioState();
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);
    g_zNetwork_ActiveProviderIsTcpIp = 1;
    g_getCapsReturnedFlags = 0;
    if (zNetworkDPlay::QueryCapsAndConfigureSendMode() != 0) {
        return 4;
    }

    ResetDirectPlayScenarioState();
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);
    g_getCapsResult = -1;
    return zNetworkDPlay::QueryCapsAndConfigureSendMode() == 0 ? 0 : 5;
}

extern "C" int znetwork_dplay_create_session_from_status_fields_smoke(void) {
    void *vtable[26];
    BuildMinimalDirectPlayVtable(vtable);
    struct FakeDirectPlay {
        void **vtable;
    } fake = {vtable};

    GUID appGuid = {
        0x76543210,
        0xba98,
        0xfedc,
        {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef}
    };

    ResetDirectPlayScenarioState();
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);
    g_zNetwork_AppGuid = &appGuid;

    zNetworkDPlaySessionDescCache *const oldCache =
        (zNetworkDPlaySessionDescCache *)(malloc(sizeof(zNetworkDPlaySessionDescCache)));
    memset(
        oldCache,
        0,
        sizeof(zNetworkDPlaySessionDescCache)
    );
    g_zNetwork_CurrentSessionDescCache = oldCache;

    zNetworkSessionDescStatusFields fields = {};
    fields.eventCode = 3;
    fields.statusFlags = 5;
    fields.valueOrTime = 7;
    fields.auxParam = 11;
    fields.maxPlayers = 8;
    strcpy(
        fields.sessionNameBuf,
        "Arena"
    );

    g_getCapsReturnedFlags = 0x10040;
    const int success = zNetwork_DPlay::CreateSessionFromStatusFields(&fields);
    zNetworkDPlaySessionDescCache *const cache = g_zNetwork_CurrentSessionDescCache;
    const int successOk =
        success == 1 &&
        cache != 0 &&
        cache != oldCache &&
        g_openCalls == 1 &&
        g_openDescPtr == &cache->desc &&
        g_openFlags == 2 &&
        strcmp(g_zNetwork_SessionNameCache, "Arena") == 0 &&
        cache->desc.dwSize == sizeof(zNetworkDPlaySessionDesc) &&
        cache->desc.dwFlags == 0x44 &&
        memcmp(
            &cache->desc.guidApplication,
            &appGuid,
            sizeof(cache->desc.guidApplication)
        ) == 0 &&
        cache->desc.dwMaxPlayers == 8 &&
        cache->desc.dwUser1 == 3 &&
        cache->desc.dwUser2 == 5 &&
        cache->desc.dwUser3 == 7 &&
        cache->desc.dwUser4 == 11 &&
        strcmp(cache->desc.lpszSessionNameA, "Arena") == 0;

    free(cache->desc.lpszSessionNameA);
    free(cache);
    g_zNetwork_CurrentSessionDescCache = 0;

    ResetDirectPlayScenarioState();
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);
    g_zNetwork_AppGuid = &appGuid;
    g_openResult = (HRESULT)(0x88770118);
    strcpy(
        fields.sessionNameBuf,
        "Cancel"
    );
    const int cancelResult = zNetwork_DPlay::CreateSessionFromStatusFields(&fields);
    zNetworkDPlaySessionDescCache *leakedCache =
        (zNetworkDPlaySessionDescCache *)(
            (unsigned char *)(g_openDescPtr) -
            offsetof(zNetworkDPlaySessionDescCache, desc)
        );
    free(g_openDescPtr->lpszSessionNameA);
    free(leakedCache);

    ResetDirectPlayScenarioState();
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);
    g_zNetwork_AppGuid = &appGuid;
    g_zNetwork_ActiveProviderIsTcpIp = 1;
    g_getCapsReturnedFlags = 0;
    g_openResult = 0;
    const int capsRejected = zNetwork_DPlay::CreateSessionFromStatusFields(&fields);
    leakedCache =
        (zNetworkDPlaySessionDescCache *)(
            (unsigned char *)(g_openDescPtr) -
            offsetof(zNetworkDPlaySessionDescCache, desc)
        );
    free(g_openDescPtr->lpszSessionNameA);
    free(leakedCache);

    if (!successOk) {
        return 2;
    }
    if (cancelResult != 0) {
        return 3;
    }
    if (capsRejected != 0) {
        return 4;
    }
    if (g_closeCalls != 1) {
        return 5;
    }
    return 0;
}

extern "C" int znetwork_unregister_packet_handler_smoke(void) {
    zNetworkDispatchHandlerListNode sentinel = {};
    zNetworkDispatchHandlerListNode first = {};
    zNetworkDispatchHandlerListNode *second =
        (zNetworkDispatchHandlerListNode *)(::operator new(sizeof(zNetworkDispatchHandlerListNode)));
    zNetworkDispatchHandlerListNode *third =
        (zNetworkDispatchHandlerListNode *)(::operator new(sizeof(zNetworkDispatchHandlerListNode)));

    zNetworkDispatchHandlerRecord firstRecord = {};
    zNetworkDispatchHandlerRecord secondRecord = {};
    zNetworkDispatchHandlerRecord thirdRecord = {};
    firstRecord.packetType = 7;
    firstRecord.handler = TestPacketHandlerA;
    secondRecord.packetType = 8;
    secondRecord.handler = TestPacketHandlerB;
    thirdRecord.packetType = 7;
    thirdRecord.handler = TestPacketHandlerA;

    sentinel.next = &first;
    sentinel.prev = third;
    first.next = second;
    first.prev = &sentinel;
    first.record = &firstRecord;
    second->next = third;
    second->prev = &first;
    second->record = &secondRecord;
    third->next = &sentinel;
    third->prev = second;
    third->record = &thirdRecord;

    zNetworkDispatchHandlerListNode *const oldSentinel =
        g_zNetwork_DispatchHandlerListSentinel;
    const int oldCount = g_zNetwork_DispatchHandlerListCount;
    g_zNetwork_DispatchHandlerListSentinel = &sentinel;
    g_zNetwork_DispatchHandlerListCount = 3;

    const int result = zNetwork::UnregisterPacketHandler(
        7,
        TestPacketHandlerA
    );

    const int ok = result == 1 &&
                   g_zNetwork_DispatchHandlerListCount == 1 &&
                   sentinel.next == &first &&
                   sentinel.prev == &first &&
                   first.next == &sentinel &&
                   first.prev == &sentinel &&
                   first.record == &secondRecord;

    g_zNetwork_DispatchHandlerListSentinel = oldSentinel;
    g_zNetwork_DispatchHandlerListCount = oldCount;
    return ok ? 0 : 1;
}

extern "C" int znetwork_clear_enumerated_session_list_smoke(void) {
    zArchiveList list = {};
    zArchiveListNode *const first =
        (zArchiveListNode *)(std::malloc(sizeof(zArchiveListNode)));
    zArchiveListNode *const second =
        (zArchiveListNode *)(std::malloc(sizeof(zArchiveListNode)));
    zNetworkDPlaySessionDesc *const firstDesc =
        (zNetworkDPlaySessionDesc *)(std::malloc(sizeof(zNetworkDPlaySessionDesc)));
    zNetworkDPlaySessionDesc *const secondDesc =
        (zNetworkDPlaySessionDesc *)(std::malloc(sizeof(zNetworkDPlaySessionDesc)));
    std::memset(first, 0, sizeof(zArchiveListNode));
    std::memset(second, 0, sizeof(zArchiveListNode));
    std::memset(firstDesc, 0, sizeof(zNetworkDPlaySessionDesc));
    std::memset(secondDesc, 0, sizeof(zNetworkDPlaySessionDesc));
    firstDesc->dwReserved1 = (DWORD)(std::malloc(4));

    first->payload = firstDesc;
    first->next = second;
    first->prev = second;
    second->payload = secondDesc;
    second->next = first;
    second->prev = first;
    list.count = 2;
    list.head = first;

    zArchiveList *const oldEnumeratedList = g_zNetwork_EnumeratedSessionList;
    zArchiveList *const oldFreePool = g_zUtil_ZRDR_FreePool;
    const int oldFreeCount = g_zUtil_ZRDR_FreeCount;
    const int oldGrowCount = g_zUtil_ZRDR_GrowCount;
    const int oldTotalAllocated = g_zUtil_ZRDR_TotalAllocated;
    g_zNetwork_EnumeratedSessionList = &list;
    g_zUtil_ZRDR_FreePool = 0;
    g_zUtil_ZRDR_FreeCount = 0;
    g_zUtil_ZRDR_GrowCount = 0;
    g_zUtil_ZRDR_TotalAllocated = 0;

    zNetwork::ClearEnumeratedSessionList();
    const int ok = list.count == 0 && list.head == 0;

    zUtil_ZRDR_FreeNodePool();
    g_zNetwork_EnumeratedSessionList = oldEnumeratedList;
    g_zUtil_ZRDR_FreePool = oldFreePool;
    g_zUtil_ZRDR_FreeCount = oldFreeCount;
    g_zUtil_ZRDR_GrowCount = oldGrowCount;
    g_zUtil_ZRDR_TotalAllocated = oldTotalAllocated;
    return ok ? 0 : 1;
}

extern "C" int znetwork_clear_service_provider_list_smoke(void) {
    zNetworkDPlayServiceProviderInfo **const slots =
        (zNetworkDPlayServiceProviderInfo **)(::operator new(
            sizeof(zNetworkDPlayServiceProviderInfo *) * 2
        ));
    zNetworkDPlayServiceProviderInfo *const info =
        (zNetworkDPlayServiceProviderInfo *)(::operator new(
            sizeof(zNetworkDPlayServiceProviderInfo)
        ));
    std::memset(info, 0, sizeof(zNetworkDPlayServiceProviderInfo));
    info->displayName = (char *)(std::malloc(8));
    info->connectionData = std::malloc(8);
    slots[0] = info;
    slots[1] = 0;

    zNetworkServiceProviderListVec list = {};
    list.begin = slots;
    list.end = slots + 2;
    list.cap = slots + 2;

    zNetworkServiceProviderListVec *const oldList = g_zNetwork_ServiceProviderList;
    g_zNetwork_ServiceProviderList = &list;

    zNetwork::ClearServiceProviderList();
    const int ok = list.end == list.begin && slots[0] == 0 && slots[1] == 0;

    ::operator delete(slots);
    g_zNetwork_ServiceProviderList = oldList;
    return ok ? 0 : 1;
}

extern "C" int znetwork_clear_player_record_list_smoke(void) {
    zNetworkPlayerRecordListNode sentinel = {};
    zNetworkPlayerRecordListNode *const first =
        (zNetworkPlayerRecordListNode *)(::operator new(sizeof(zNetworkPlayerRecordListNode)));
    zNetworkPlayerRecordListNode *const second =
        (zNetworkPlayerRecordListNode *)(::operator new(sizeof(zNetworkPlayerRecordListNode)));
    zNetwork_PlayerRecord *const firstRecord =
        (zNetwork_PlayerRecord *)(::operator new(sizeof(zNetwork_PlayerRecord)));
    zNetwork_PlayerRecord *const secondRecord =
        (zNetwork_PlayerRecord *)(::operator new(sizeof(zNetwork_PlayerRecord)));
    std::memset(first, 0, sizeof(zNetworkPlayerRecordListNode));
    std::memset(second, 0, sizeof(zNetworkPlayerRecordListNode));
    std::memset(firstRecord, 0, sizeof(zNetwork_PlayerRecord));
    std::memset(secondRecord, 0, sizeof(zNetwork_PlayerRecord));

    sentinel.next = first;
    sentinel.prev = second;
    first->next = second;
    first->prev = &sentinel;
    first->playerRecord = firstRecord;
    second->next = &sentinel;
    second->prev = first;
    second->playerRecord = secondRecord;

    zNetworkPlayerRecordList list = {};
    list.sentinelNode = &sentinel;
    list.count = 2;

    zNetworkPlayerRecordList *const oldList = g_zNetwork_PlayerRecordList;
    g_zNetwork_PlayerRecordList = &list;

    zNetwork::ClearPlayerRecordList();
    const int ok = list.count == 0 &&
                   sentinel.next == &sentinel &&
                   sentinel.prev == &sentinel;

    g_zNetwork_PlayerRecordList = oldList;
    return ok ? 0 : 1;
}

extern "C" int znetwork_player_record_accessors_smoke(void) {
    zNetworkPlayerRecordListNode sentinel = {};
    zNetworkPlayerRecordListNode node = {};
    zNetwork_PlayerRecord record = {};
    record.playerKey = 0x12345678;
    record.colorIndex = 5;

    sentinel.next = &node;
    sentinel.prev = &node;
    node.next = &sentinel;
    node.prev = &sentinel;
    node.playerRecord = &record;

    zNetworkPlayerRecordList list = {};
    list.sentinelNode = &sentinel;
    list.count = 1;

    zNetworkPlayerRecordList *const oldList = g_zNetwork_PlayerRecordList;
    g_zNetwork_PlayerRecordList = &list;

    const int ok = zNetwork_FindPlayerRecordByKey(0x12345678) == &record &&
                   zNetwork_FindPlayerRecordByKey(0x87654321) == 0;

    g_zNetwork_PlayerRecordList = oldList;
    return ok ? 0 : 1;
}

extern "C" int znetwork_apply_pkt01_player_color_assignments_smoke(void) {
    zNetworkPlayerRecordListNode sentinel = {};
    zNetworkPlayerRecordListNode node = {};
    zNetwork_PlayerRecord record = {};
    record.playerKey = 0x12345678;
    record.colorIndex = 2;

    sentinel.next = &node;
    sentinel.prev = &node;
    node.next = &sentinel;
    node.prev = &sentinel;
    node.playerRecord = &record;

    zNetworkPlayerRecordList list = {};
    list.sentinelNode = &sentinel;
    list.count = 1;

    NetPkt01_PlayerColorAssignments packet = {};
    packet.header.packetType = 1;
    packet.header.packetSizeBytes = sizeof(packet);
    packet.pairCount = 1;
    packet.pairs[0].playerKey = 0x12345678;
    packet.pairs[0].colorIndex = 6;

    zNetworkPlayerRecordList *const oldList = g_zNetwork_PlayerRecordList;
    const int oldColorFlag = g_zNetwork_PlayerColorInUseFlags[6];
    g_zNetwork_PlayerRecordList = &list;
    g_zNetwork_PlayerColorInUseFlags[6] = 0;

    const int result = zNetwork_ApplyPkt01_PlayerColorAssignments(
        0,
        &packet.header
    );
    const int ok = result == 1 &&
                   record.colorIndex == 6 &&
                   g_zNetwork_PlayerColorInUseFlags[6] == 1;

    g_zNetwork_PlayerRecordList = oldList;
    g_zNetwork_PlayerColorInUseFlags[6] = oldColorFlag;
    return ok ? 0 : 1;
}

extern "C" int znetwork_dispatch_packet_to_handlers_smoke(void) {
    ResetDirectPlayScenarioState();
    zNetwork_InitMessageHandlers();

    zNetworkDispatchHandlerRecord *const recordA =
        zNetwork::RegisterPacketHandler(
            0x77,
            TestDispatchHandlerA,
            3
        );
    zNetworkDispatchHandlerRecord *const recordSkip =
        zNetwork::RegisterPacketHandler(
            0x78,
            TestPacketHandlerA,
            4
        );
    zNetworkDispatchHandlerRecord *const recordB =
        zNetwork::RegisterPacketHandler(
            0x77,
            TestDispatchHandlerB,
            5
        );

    zNetworkPacketHeader packet = {};
    packet.packetType = 0x77;
    packet.packetSizeBytes = sizeof(packet);
    packet.payloadDword0 = 0x12345678;
    zNetwork_DPlay::DispatchPacketToHandlers(
        0x1234,
        &packet
    );

    const int dispatchOk =
        g_dispatchCallsA == 1 &&
        g_dispatchCallsB == 1 &&
        g_dispatchSenderA == 0x1234 &&
        g_dispatchSenderB == 0x1234 &&
        g_dispatchPacketA == &packet &&
        g_dispatchPacketB == &packet &&
        g_dispatchPacketPayloadA == 0x12345678 &&
        g_dispatchPacketPayloadB == 0x12345678;

    packet.packetType = 0x79;
    zNetwork_DPlay::DispatchPacketToHandlers(
        0x5678,
        &packet
    );

    const int skipOk = g_dispatchCallsA == 1 && g_dispatchCallsB == 1;

    ::operator delete(recordA);
    ::operator delete(recordSkip);
    ::operator delete(recordB);
    zNetwork_DestroyDispatchHandlerList();
    return dispatchOk != 0 && skipOk != 0 ? 0 : 1;
}

extern "C" int znetwork_dplay_pump_incoming_messages_smoke(void) {
    ResetDirectPlayScenarioState();
    zNetwork_InitMessageHandlers();

    zNetworkDispatchHandlerRecord *const joinRecord =
        zNetwork::RegisterPacketHandler(
            2,
            TestDispatchHandlerA,
            0
        );
    zNetworkDispatchHandlerRecord *const leaveRecord =
        zNetwork::RegisterPacketHandler(
            3,
            TestDispatchHandlerB,
            0
        );
    zNetworkDispatchHandlerRecord *const sessionJoinRecord =
        zNetwork::RegisterPacketHandler(
            4,
            TestDispatchHandlerA,
            0
        );
    zNetworkDispatchHandlerRecord *const sessionLeaveRecord =
        zNetwork::RegisterPacketHandler(
            5,
            TestDispatchHandlerB,
            0
        );

    zNetworkPlayerRecordList *playerList = AllocZeroedObject<zNetworkPlayerRecordList>();
    zNetworkPlayerRecordListNode *sentinel =
        AllocZeroedObject<zNetworkPlayerRecordListNode>();
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
    playerList->sentinelNode = sentinel;
    playerList->count = 0;
    g_zNetwork_PlayerRecordList = playerList;

    zNetworkDPlaySessionDescCache session = {};
    g_zNetwork_CurrentSessionDescCache = &session;

    char shortName[] = "Ace";
    char longName[] = "Ace Pilot";
    zNetworkDPlaySystemMessage message = {};
    message.msgType = 3;
    message.fields.playerId = 0x11112222;
    message.fields.createFlagsOrPlayerType = sizeof(zNetworkDPlayName);
    message.fields.nameShortOrAsyncHandle = 7;
    message.fields.nameLong = shortName;
    message.fields.nameDisplay = longName;

    const int createResult = zNetworkDPlay::PumpIncomingMessages(&message);
    zNetwork_PlayerRecord *const createdRecord =
        zNetwork_FindPlayerRecordByKey(0x11112222);
    const int createOk =
        createResult == 0 &&
        createdRecord != 0 &&
        playerList->count == 1 &&
        g_zNetworkCurrentPlayerCountCached == 1 &&
        createdRecord->playerKey == 0x11112222 &&
        createdRecord->playerNameInfo.dwSize == sizeof(zNetworkDPlayName) &&
        createdRecord->playerNameInfo.dwFlags == 7 &&
        createdRecord->playerNameInfo.lpszShortNameA == shortName &&
        createdRecord->playerNameInfo.lpszLongNameA == longName &&
        strcmp(createdRecord->playerName, "Ace Pilot") == 0 &&
        strcmp(createdRecord->altName, "Ace") == 0 &&
        createdRecord->colorIndex == 0 &&
        g_dispatchCallsA == 1 &&
        g_dispatchSenderA == 0x11112222 &&
        g_dispatchPacketTypeA == 2 &&
        g_dispatchPacketPayloadA == 0;

    message.msgType = 5;
    g_dispatchCallsB = 0;
    zNetworkDPlay::PumpIncomingMessages(&message);
    const int leaveOk =
        playerList->count == 0 &&
        g_zNetworkCurrentPlayerCountCached == 0 &&
        g_dispatchCallsB == 1 &&
        g_dispatchSenderB == 0x11112222 &&
        g_dispatchPacketTypeB == 3 &&
        zNetwork_FindPlayerRecordByKey(0x11112222) == 0;

    message.msgType = 0x101;
    g_zNetwork_IsHostFlag = 0;
    zNetworkDPlay::PumpIncomingMessages(&message);
    const int hostOk = g_zNetwork_IsHostFlag == 1;

    message.msgType = 0x102;
    g_dispatchCallsA = 0;
    zNetworkDPlay::PumpIncomingMessages(&message);
    const int sessionJoinOk = g_dispatchCallsA == 1 && g_dispatchPacketTypeA == 4;

    message.msgType = 0x103;
    g_dispatchCallsB = 0;
    zNetworkDPlay::PumpIncomingMessages(&message);
    const int sessionLeaveOk = g_dispatchCallsB == 1 && g_dispatchPacketTypeB == 5;

    zNetworkDPlaySessionDesc desc = {};
    desc.dwMaxPlayers = 6;
    desc.dwUser1 = 10;
    desc.dwUser2 = 11;
    desc.dwUser3 = 12;
    desc.dwUser4 = 13;
    memcpy(
        message.payload_004,
        &desc,
        sizeof(desc)
    );
    message.msgType = 0x104;
    zNetworkDPlay::PumpIncomingMessages(&message);
    const int descOk =
        session.desc.dwMaxPlayers == 6 &&
        session.desc.dwUser1 == 10 &&
        session.desc.dwUser2 == 11 &&
        session.desc.dwUser3 == 12 &&
        session.desc.dwUser4 == 13;

    message.msgType = 0x10d;
    message.fields.nameShortOrAsyncHandle = 0x55aa;
    g_zNetwork_LastSendExHandle = 0x55aa;
    g_zNetwork_LastSendExCompleted = 0;
    zNetworkDPlay::PumpIncomingMessages(&message);
    const int sendCompleteOk = g_zNetwork_LastSendExCompleted == 1;

    message.msgType = 0x31;
    g_zNetwork_FatalDisconnectCallback = TestFatalDisconnect;
    const int fatalResult = zNetworkDPlay::PumpIncomingMessages(&message);
    const int fatalOk =
        fatalResult == -1 &&
        g_zNetwork_FatalDisconnectTriggered == 1 &&
        g_fatalDisconnectCalls == 1 &&
        g_fatalDisconnectReason == -1;

    ::operator delete(createdRecord);
    ::operator delete(joinRecord);
    ::operator delete(leaveRecord);
    ::operator delete(sessionJoinRecord);
    ::operator delete(sessionLeaveRecord);
    zNetwork_DestroyDispatchHandlerList();
    ::operator delete(sentinel);
    ::operator delete(playerList);
    g_zNetwork_PlayerRecordList = 0;
    g_zNetwork_CurrentSessionDescCache = 0;
    return createOk != 0 &&
                   leaveOk != 0 &&
                   hostOk != 0 &&
                   sessionJoinOk != 0 &&
                   sessionLeaveOk != 0 &&
                   descOk != 0 &&
                   sendCompleteOk != 0 &&
                   fatalOk != 0
               ? 0
               : 1;
}

extern "C" int znetwork_dplay_receive_pending_messages_smoke(void) {
    ResetDirectPlayScenarioState();
    zNetwork_InitMessageHandlers();

    void *vtable[26];
    BuildMinimalDirectPlayVtable(vtable);
    struct FakeDirectPlay {
        void **vtable;
    } fake = {vtable};
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);

    g_zNetwork_SessionRuntimeInitialized = 0;
    if (zNetworkDPlay::ReceivePendingMessages(1) != 0 || g_receiveCalls != 0) {
        zNetwork_DestroyDispatchHandlerList();
        return 1;
    }

    g_zNetwork_SessionRuntimeInitialized = 1;
    g_zNetwork_FatalDisconnectTriggered = 1;
    if (zNetworkDPlay::ReceivePendingMessages(1) != -1 || g_receiveCalls != 0) {
        zNetwork_DestroyDispatchHandlerList();
        return 2;
    }

    g_zNetwork_FatalDisconnectTriggered = 0;
    zNetworkDispatchHandlerRecord *const sessionRecord =
        zNetwork::RegisterPacketHandler(
            4,
            TestDispatchHandlerA,
            0
        );
    zNetworkDispatchHandlerRecord *const playerRecord =
        zNetwork::RegisterPacketHandler(
            7,
            TestDispatchHandlerB,
            0
        );

    g_zNetwork_ReceiveBufferCapacity = 8;
    g_zNetwork_ReceiveBuffer = malloc(g_zNetwork_ReceiveBufferCapacity);

    g_receiveResults[0] = (HRESULT)(0x8877001e);
    g_receiveSizes[0] = sizeof(zNetworkDPlaySystemMessage);

    zNetworkDPlaySystemMessage systemMessage = {};
    systemMessage.msgType = 0x102;
    systemMessage.fields.playerId = 0x11112222;
    memcpy(
        g_receivePayloads[1],
        &systemMessage,
        sizeof(systemMessage)
    );
    g_receiveResults[1] = 0;
    g_receiveFrom[1] = 0;
    g_receiveSizes[1] = sizeof(systemMessage);

    zNetworkPacketHeader playerPacket = {};
    playerPacket.packetType = 7;
    playerPacket.packetSizeBytes = sizeof(playerPacket);
    playerPacket.payloadDword0 = 0x12345678;
    memcpy(
        g_receivePayloads[2],
        &playerPacket,
        sizeof(playerPacket)
    );
    g_receiveResults[2] = 0;
    g_receiveFrom[2] = 0x33334444;
    g_receiveSizes[2] = sizeof(playerPacket);

    const int processed = zNetworkDPlay::ReceivePendingMessages(2);
    const int receiveOk =
        processed == 2 &&
        g_receiveCalls == 3 &&
        g_zNetwork_ReceiveBufferCapacity == sizeof(zNetworkDPlaySystemMessage) &&
        g_zNetwork_ReceiveBuffer != 0 &&
        g_dispatchCallsA == 1 &&
        g_dispatchPacketTypeA == 4 &&
        g_dispatchSenderA == 0x11112222 &&
        g_dispatchCallsB == 1 &&
        g_dispatchPacketTypeB == 7 &&
        g_dispatchSenderB == 0x33334444 &&
        g_dispatchPacketPayloadB == 0x12345678;

    free(g_zNetwork_ReceiveBuffer);
    g_zNetwork_ReceiveBuffer = 0;
    g_zNetwork_ReceiveBufferCapacity = 0;
    ::operator delete(sessionRecord);
    ::operator delete(playerRecord);
    zNetwork_DestroyDispatchHandlerList();
    g_zNetwork_pDirectPlay4 = 0;
    return receiveOk != 0 ? 0 : 3;
}

extern "C" int znetwork_dplay_enum_players_smoke(void) {
    ResetDirectPlayScenarioState();

    zNetworkPlayerRecordList *playerList = AllocZeroedObject<zNetworkPlayerRecordList>();
    zNetworkPlayerRecordListNode *sentinel =
        AllocZeroedObject<zNetworkPlayerRecordListNode>();
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
    playerList->sentinelNode = sentinel;
    playerList->count = 0;
    g_zNetwork_PlayerRecordList = playerList;

    void *vtable[26];
    BuildMinimalDirectPlayVtable(vtable);
    struct FakeDirectPlay {
        void **vtable;
    } fake = {vtable};
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);

    const int count = zNetwork_DPlay::EnumPlayers();
    zNetworkPlayerRecordListNode *const node = sentinel->next;
    const int ok =
        count == 1 &&
        g_enumPlayersCalls == 1 &&
        playerList->count == 1 &&
        node != sentinel &&
        node->next == sentinel &&
        node->prev == sentinel &&
        sentinel->prev == node &&
        node->playerRecord != 0 &&
        node->playerRecord->playerKey == 0x101 &&
        strcmp(node->playerRecord->playerName, "Ace") == 0;

    zNetwork::ClearPlayerRecordList();
    ::operator delete(sentinel);
    ::operator delete(playerList);
    g_zNetwork_PlayerRecordList = 0;
    g_zNetwork_pDirectPlay4 = 0;
    return ok != 0 ? 0 : 1;
}

extern "C" int znetwork_dplay_create_local_player_record_smoke(void) {
    ResetDirectPlayScenarioState();
    void *vtable[26];
    BuildMinimalDirectPlayVtable(vtable);
    struct FakeDirectPlay {
        void **vtable;
    } fake = {vtable};
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);

    zNetworkPlayerRecordList *playerList = AllocZeroedObject<zNetworkPlayerRecordList>();
    zNetworkPlayerRecordListNode *sentinel =
        AllocZeroedObject<zNetworkPlayerRecordListNode>();
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
    playerList->sentinelNode = sentinel;
    playerList->count = 0;
    g_zNetwork_PlayerRecordList = playerList;

    zNetworkDPlaySessionDescCache session = {};
    session.desc.dwCurrentPlayers = 2;
    session.desc.dwMaxPlayers = 8;
    g_zNetwork_CurrentSessionDescCache = &session;
    g_getPlayerCapsReturnedFlags = 2;

    char playerName[] = "Pilot";
    const int playerKey =
        zNetwork_DPlay::CreateLocalPlayerRecordAndRegister(playerName);
    zNetwork_PlayerRecord *const localPlayer = g_zNetwork_LocalPlayerRecord;

    const int localOk =
        playerKey == (int)(g_createPlayerAssignedId) &&
        localPlayer != 0 &&
        localPlayer->playerKey == g_createPlayerAssignedId &&
        g_createPlayerCalls == 1 &&
        g_createPlayerIdPtr == (DWORD *)(&localPlayer->playerKey) &&
        g_createPlayerNameInfo == &localPlayer->playerNameInfo &&
        g_createPlayerEventHandle == 0 &&
        g_createPlayerData == 0 &&
        g_createPlayerDataSize == 0 &&
        g_createPlayerFlags == 0 &&
        localPlayer->playerNameInfo.dwSize == sizeof(zNetworkDPlayName) &&
        localPlayer->playerNameInfo.dwFlags == 0 &&
        localPlayer->playerNameInfo.lpszShortNameA == g_zNetwork_LocalPlayerNameScratch &&
        localPlayer->playerNameInfo.lpszLongNameA == g_zNetwork_LocalPlayerNameScratch &&
        strcmp(g_zNetwork_LocalPlayerNameScratch, "Pilot") == 0 &&
        strcmp(localPlayer->playerName, "Pilot") == 0 &&
        strcmp(localPlayer->altName, "Pilot") == 0;

    const int capsOk =
        g_getPlayerCapsCalls == 1 &&
        g_getPlayerCapsPlayerId == g_createPlayerAssignedId &&
        g_getPlayerCapsFlags == 0 &&
        localPlayer->playerCaps.dwSize == sizeof(zNetworkDPlayCaps) &&
        localPlayer->playerCaps.dwFlags == 2 &&
        g_zNetwork_IsHostFlag == 2;

    const int listOk =
        playerList->count == 2 &&
        sentinel->prev->playerRecord == localPlayer &&
        g_zNetwork_LocalPlayerKey == (int)(g_createPlayerAssignedId) &&
        g_zNetworkCurrentPlayerCountCached == 3 &&
        localPlayer->colorIndex == 1 &&
        g_zNetwork_PlayerColorInUseFlags[1] == 1;

    zNetwork::ClearPlayerRecordList();
    ::operator delete(sentinel);
    ::operator delete(playerList);
    g_zNetwork_PlayerRecordList = 0;
    g_zNetwork_LocalPlayerRecord = 0;

    if (localOk == 0) {
        return 1;
    }
    if (capsOk == 0) {
        return 2;
    }
    if (listOk == 0) {
        return 3;
    }

    ResetDirectPlayScenarioState();
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fake);
    playerList = AllocZeroedObject<zNetworkPlayerRecordList>();
    sentinel = AllocZeroedObject<zNetworkPlayerRecordListNode>();
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
    playerList->sentinelNode = sentinel;
    playerList->count = 0;
    g_zNetwork_PlayerRecordList = playerList;
    g_zNetwork_CurrentSessionDescCache = &session;
    g_getPlayerCapsResult = -1;
    g_getPlayerCapsReturnedFlags = 0;
    strcpy(
        playerName,
        "Fail"
    );
    const int failResult =
        zNetwork_DPlay::CreateLocalPlayerRecordAndRegister(playerName);
    ::operator delete(g_zNetwork_LocalPlayerRecord);
    g_zNetwork_LocalPlayerRecord = 0;
    zNetwork::ClearPlayerRecordList();
    ::operator delete(sentinel);
    ::operator delete(playerList);
    g_zNetwork_PlayerRecordList = 0;
    return failResult == 0 ? 0 : 4;
}

extern "C" int znetwork_shutdown_session_runtime_smoke(void) {
    void *vtable[5] = {};
    vtable[2] = (void *)(&FakeDirectPlayRelease);
    vtable[4] = (void *)(&FakeDirectPlayClose);
    struct FakeDirectPlay {
        void **vtable;
    } fakeDirectPlay = {vtable};

    zNetworkDispatchHandlerRecord handlerRecord = {};
    handlerRecord.packetType = 1;
    handlerRecord.handler = zNetwork_ApplyPkt01_PlayerColorAssignments;
    zNetworkDispatchHandlerListNode dispatchSentinel = {};
    zNetworkDispatchHandlerListNode *const dispatchNode =
        (zNetworkDispatchHandlerListNode *)(::operator new(
            sizeof(zNetworkDispatchHandlerListNode)
        ));
    dispatchSentinel.next = dispatchNode;
    dispatchSentinel.prev = dispatchNode;
    dispatchNode->next = &dispatchSentinel;
    dispatchNode->prev = &dispatchSentinel;
    dispatchNode->record = &handlerRecord;

    zArchiveList *const enumeratedList =
        (zArchiveList *)(std::malloc(sizeof(zArchiveList)));
    zArchiveListNode *const sessionNode =
        (zArchiveListNode *)(std::malloc(sizeof(zArchiveListNode)));
    zNetworkDPlaySessionDesc *const sessionDesc =
        (zNetworkDPlaySessionDesc *)(std::malloc(sizeof(zNetworkDPlaySessionDesc)));
    std::memset(enumeratedList, 0, sizeof(zArchiveList));
    std::memset(sessionNode, 0, sizeof(zArchiveListNode));
    std::memset(sessionDesc, 0, sizeof(zNetworkDPlaySessionDesc));
    sessionDesc->dwReserved1 = (DWORD)(std::malloc(4));
    sessionNode->payload = sessionDesc;
    sessionNode->next = sessionNode;
    sessionNode->prev = sessionNode;
    enumeratedList->count = 1;
    enumeratedList->head = sessionNode;

    zNetworkServiceProviderListVec *const providerList =
        (zNetworkServiceProviderListVec *)(::operator new(
            sizeof(zNetworkServiceProviderListVec)
        ));
    zNetworkDPlayServiceProviderInfo **const providerSlots =
        (zNetworkDPlayServiceProviderInfo **)(::operator new(
            sizeof(zNetworkDPlayServiceProviderInfo *)
        ));
    zNetworkDPlayServiceProviderInfo *const provider =
        (zNetworkDPlayServiceProviderInfo *)(::operator new(
            sizeof(zNetworkDPlayServiceProviderInfo)
        ));
    std::memset(provider, 0, sizeof(zNetworkDPlayServiceProviderInfo));
    provider->displayName = (char *)(std::malloc(8));
    provider->connectionData = std::malloc(8);
    providerSlots[0] = provider;
    providerList->begin = providerSlots;
    providerList->end = providerSlots + 1;
    providerList->cap = providerSlots + 1;

    zNetworkPlayerRecordList *const playerList =
        (zNetworkPlayerRecordList *)(::operator new(sizeof(zNetworkPlayerRecordList)));
    zNetworkPlayerRecordListNode *const playerSentinel =
        (zNetworkPlayerRecordListNode *)(::operator new(
            sizeof(zNetworkPlayerRecordListNode)
        ));
    zNetworkPlayerRecordListNode *const playerNode =
        (zNetworkPlayerRecordListNode *)(::operator new(
            sizeof(zNetworkPlayerRecordListNode)
        ));
    zNetwork_PlayerRecord *const playerRecord =
        (zNetwork_PlayerRecord *)(::operator new(sizeof(zNetwork_PlayerRecord)));
    playerSentinel->next = playerNode;
    playerSentinel->prev = playerNode;
    playerNode->next = playerSentinel;
    playerNode->prev = playerSentinel;
    playerNode->playerRecord = playerRecord;
    playerList->sentinelNode = playerSentinel;
    playerList->count = 1;

    zNetwork_DPlay4 *const oldDirectPlay = g_zNetwork_pDirectPlay4;
    const int oldSessionRuntime = g_zNetwork_SessionRuntimeInitialized;
    zNetworkDispatchHandlerListNode *const oldDispatchSentinel =
        g_zNetwork_DispatchHandlerListSentinel;
    const int oldDispatchCount = g_zNetwork_DispatchHandlerListCount;
    zArchiveList *const oldEnumeratedList = g_zNetwork_EnumeratedSessionList;
    zNetworkDPlaySessionDescCache *const oldCurrentCache =
        g_zNetwork_CurrentSessionDescCache;
    zNetworkServiceProviderListVec *const oldProviderList =
        g_zNetwork_ServiceProviderList;
    zNetworkPlayerRecordList *const oldPlayerList = g_zNetwork_PlayerRecordList;
    void *const oldReceiveBuffer = g_zNetwork_ReceiveBuffer;
    zArchiveList *const oldFreePool = g_zUtil_ZRDR_FreePool;
    const int oldFreeCount = g_zUtil_ZRDR_FreeCount;
    const int oldGrowCount = g_zUtil_ZRDR_GrowCount;
    const int oldTotalAllocated = g_zUtil_ZRDR_TotalAllocated;

    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&fakeDirectPlay);
    g_zNetwork_SessionRuntimeInitialized = 1;
    g_zNetwork_DispatchHandlerListSentinel = &dispatchSentinel;
    g_zNetwork_DispatchHandlerListCount = 1;
    g_zNetwork_EnumeratedSessionList = enumeratedList;
    g_zNetwork_CurrentSessionDescCache = (zNetworkDPlaySessionDescCache *)(&fakeDirectPlay);
    g_zNetwork_ServiceProviderList = providerList;
    g_zNetwork_PlayerRecordList = playerList;
    g_zNetwork_ReceiveBuffer = std::malloc(8);
    g_zUtil_ZRDR_FreePool = 0;
    g_zUtil_ZRDR_FreeCount = 0;
    g_zUtil_ZRDR_GrowCount = 0;
    g_zUtil_ZRDR_TotalAllocated = 0;
    g_closeCalls = 0;
    g_releaseCalls = 0;

    const int result = zNetwork::ShutdownSessionRuntime();
    const int ok = result == 0 &&
                   g_closeCalls == 1 &&
                   g_releaseCalls == 1 &&
                   g_zNetwork_SessionRuntimeInitialized == 0 &&
                   g_zNetwork_DispatchHandlerListCount == 0 &&
                   g_zNetwork_EnumeratedSessionList == 0 &&
                   g_zNetwork_CurrentSessionDescCache == 0 &&
                   g_zNetwork_ServiceProviderList == 0 &&
                   g_zNetwork_PlayerRecordList == 0 &&
                   g_zNetwork_ReceiveBuffer == 0;

    zUtil_ZRDR_FreeNodePool();
    g_zNetwork_pDirectPlay4 = oldDirectPlay;
    g_zNetwork_SessionRuntimeInitialized = oldSessionRuntime;
    g_zNetwork_DispatchHandlerListSentinel = oldDispatchSentinel;
    g_zNetwork_DispatchHandlerListCount = oldDispatchCount;
    g_zNetwork_EnumeratedSessionList = oldEnumeratedList;
    g_zNetwork_CurrentSessionDescCache = oldCurrentCache;
    g_zNetwork_ServiceProviderList = oldProviderList;
    g_zNetwork_PlayerRecordList = oldPlayerList;
    g_zNetwork_ReceiveBuffer = oldReceiveBuffer;
    g_zUtil_ZRDR_FreePool = oldFreePool;
    g_zUtil_ZRDR_FreeCount = oldFreeCount;
    g_zUtil_ZRDR_GrowCount = oldGrowCount;
    g_zUtil_ZRDR_TotalAllocated = oldTotalAllocated;
    return ok ? 0 : 1;
}
