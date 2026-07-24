#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"

#include <dplobby.h>
#include <windows.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace {
template <typename T>
T *AllocZeroed() {
    T *const value = static_cast<T *>(::operator new(sizeof(T)));
    std::memset(value, 0, sizeof(T));
    return value;
}

void FillGuid(GUID &guid, unsigned char firstByte) {
    unsigned char *const bytes = reinterpret_cast<unsigned char *>(&guid);
    for (int index = 0; index < 16; ++index) {
        bytes[index] = static_cast<unsigned char>(firstByte + index);
    }
}

struct CodeFunctionPatch {
    void *target;
    unsigned char original[5];
    bool active;
};

bool PatchFunctionJump(void *target, void *replacement, CodeFunctionPatch &patch) {
    DWORD oldProtect = 0;
    if (!VirtualProtect(target, sizeof(patch.original), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    patch.target = target;
    std::memcpy(patch.original, target, sizeof(patch.original));
    unsigned char jump[5] = {0xe9, 0, 0, 0, 0};
    const std::intptr_t relative =
        reinterpret_cast<unsigned char *>(replacement) -
        (reinterpret_cast<unsigned char *>(target) + sizeof(jump));
    const std::int32_t relative32 = static_cast<std::int32_t>(relative);
    std::memcpy(jump + 1, &relative32, sizeof(relative32));
    std::memcpy(target, jump, sizeof(jump));
    FlushInstructionCache(GetCurrentProcess(), target, sizeof(jump));
    DWORD ignored = 0;
    VirtualProtect(target, sizeof(patch.original), oldProtect, &ignored);
    patch.active = true;
    return true;
}

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
    if (!patch.active) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.target,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        )) {
        std::memcpy(patch.target, patch.original, sizeof(patch.original));
        FlushInstructionCache(
            GetCurrentProcess(),
            patch.target,
            sizeof(patch.original)
        );
        DWORD ignored = 0;
        VirtualProtect(patch.target, sizeof(patch.original), oldProtect, &ignored);
    }
    patch.active = false;
}

int g_sendCalls;
DPID g_sendFrom;
DWORD g_sendPacketSize;
unsigned char g_sendBytes[512];
int g_enumCalls;
zNetworkDPlaySessionDesc g_enumDesc;
DWORD g_enumTimeout;
zNetworkDPlayEnumSessionsCallback g_enumCallback;
void *g_enumContext;
DWORD g_enumFlags;
HRESULT g_enumResult;
int g_enumConnectingRepeats;
bool g_suppressEnumCallback;
int g_openCalls;
zNetworkDPlaySessionDesc *g_openDesc;
DWORD g_openFlags;
HRESULT g_openResult;
int g_getCapsCalls;
HRESULT g_getCapsResult;
DWORD g_getCapsReturnedFlags;
int g_closeCalls;
int g_initializeCalls;
void *g_initializeData;
DWORD g_initializeFlags;
HRESULT g_initializeResult;
unsigned char g_initializeBytes[16];

void *g_directPlayVtable[52];
struct FakeDirectPlayObject {
    void **vtable;
};
FakeDirectPlayObject g_directPlayObject;

HRESULT __stdcall FakeClose(IDirectPlay4A *) {
    ++g_closeCalls;
    return DP_OK;
}

HRESULT __stdcall FakeSend(
    IDirectPlay4A *,
    DPID fromPlayer,
    DPID,
    DWORD,
    void *packet,
    DWORD packetSize
) {
    ++g_sendCalls;
    g_sendFrom = fromPlayer;
    g_sendPacketSize = packetSize;
    if (packet != 0 && packetSize <= sizeof(g_sendBytes)) {
        std::memcpy(g_sendBytes, packet, packetSize);
    }
    return DP_OK;
}

HRESULT __stdcall FakeEnumSessions(
    IDirectPlay4A *,
    LPDPSESSIONDESC2 desc,
    DWORD timeout,
    LPDPENUMSESSIONSCALLBACK2 callback,
    void *context,
    DWORD flags
) {
    ++g_enumCalls;
    g_enumDesc = *desc;
    g_enumTimeout = timeout;
    g_enumCallback = callback;
    g_enumContext = context;
    g_enumFlags = flags;

    if (g_enumConnectingRepeats > 0) {
        --g_enumConnectingRepeats;
        return static_cast<HRESULT>(0x8877015e);
    }

    if (g_enumResult >= 0 && callback != 0 && !g_suppressEnumCallback) {
        char sessionName[] = "listed";
        zNetworkDPlaySessionDesc listed = {};
        listed.dwSize = sizeof(listed);
        listed.dwMaxPlayers = 8;
        listed.dwCurrentPlayers = 2;
        listed.lpszSessionNameA = sessionName;
        callback(&listed, 0, 0, context);
    }
    return g_enumResult;
}

HRESULT __stdcall FakeOpen(
    IDirectPlay4A *,
    LPDPSESSIONDESC2 desc,
    DWORD flags
) {
    ++g_openCalls;
    g_openDesc = desc;
    g_openFlags = flags;
    return g_openResult;
}

HRESULT __stdcall FakeGetCaps(IDirectPlay4A *, LPDPCAPS caps, DWORD) {
    ++g_getCapsCalls;
    if (caps != 0) {
        caps->dwFlags = g_getCapsReturnedFlags;
    }
    return g_getCapsResult;
}

HRESULT __stdcall FakeInitializeConnection(
    IDirectPlay4A *,
    void *connectionData,
    DWORD flags
) {
    ++g_initializeCalls;
    g_initializeData = connectionData;
    g_initializeFlags = flags;
    if (connectionData != 0) {
        std::memcpy(g_initializeBytes, connectionData, sizeof(g_initializeBytes));
    }
    return g_initializeResult;
}

zNetwork_DPlay4 *MakeDirectPlayFake() {
    std::memset(g_directPlayVtable, 0, sizeof(g_directPlayVtable));
    g_directPlayVtable[0x10 / 4] = reinterpret_cast<void *>(&FakeClose);
    g_directPlayVtable[0x34 / 4] = reinterpret_cast<void *>(&FakeEnumSessions);
    g_directPlayVtable[0x38 / 4] = reinterpret_cast<void *>(&FakeGetCaps);
    g_directPlayVtable[0x60 / 4] = reinterpret_cast<void *>(&FakeOpen);
    g_directPlayVtable[0x68 / 4] = reinterpret_cast<void *>(&FakeSend);
    g_directPlayVtable[0x98 / 4] =
        reinterpret_cast<void *>(&FakeInitializeConnection);
    g_directPlayObject.vtable = g_directPlayVtable;
    return reinterpret_cast<zNetwork_DPlay4 *>(&g_directPlayObject);
}

void ResetNetworkFakes() {
    g_sendCalls = 0;
    g_sendFrom = 0;
    g_sendPacketSize = 0;
    std::memset(g_sendBytes, 0, sizeof(g_sendBytes));
    g_enumCalls = 0;
    g_enumDesc = zNetworkDPlaySessionDesc{};
    g_enumTimeout = 0;
    g_enumCallback = 0;
    g_enumContext = 0;
    g_enumFlags = 0;
    g_enumResult = DP_OK;
    g_enumConnectingRepeats = 0;
    g_suppressEnumCallback = false;
    g_openCalls = 0;
    g_openDesc = 0;
    g_openFlags = 0;
    g_openResult = DP_OK;
    g_getCapsCalls = 0;
    g_getCapsResult = DP_OK;
    g_getCapsReturnedFlags = 0;
    g_closeCalls = 0;
    g_initializeCalls = 0;
    g_initializeData = 0;
    g_initializeFlags = 0;
    g_initializeResult = DP_OK;
    std::memset(g_initializeBytes, 0, sizeof(g_initializeBytes));

    g_zNetwork_pDirectPlay4 = 0;
    g_zNetwork_LocalPlayerRecord = 0;
    g_zNetwork_LocalPlayerKey = 0;
    g_zNetwork_IsHostFlag = 0;
    g_zNetwork_CurrentSessionDescCache = 0;
    g_zNetwork_EnumeratedSessionList = 0;
    g_zNetwork_AppGuid = 0;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_zNetwork_ActiveProviderIsTcpIp = 0;
    std::memset(
        g_zNetwork_PlayerColorInUseFlags,
        0,
        sizeof(g_zNetwork_PlayerColorInUseFlags)
    );
}

int g_createInterfaceCalls;
zNetwork_DPlay4 *g_createInterfaceValue;
int g_createInterfaceResult;

int __fastcall FakeCreateInterfaceAndCoInitialize(
    zNetwork_DPlay4 **outDirectPlay
) {
    ++g_createInterfaceCalls;
    if (outDirectPlay != 0) {
        *outDirectPlay = g_createInterfaceValue;
    }
    return g_createInterfaceResult;
}

int g_lobbyCreateCalls;
int g_lobbyQueryCalls;
int g_lobbyReleaseCalls;
int g_compoundAddressCalls;
DPCOMPOUNDADDRESSELEMENT g_compoundElements[2];
DWORD g_compoundElementCount;
DWORD g_compoundRequiredSize;
unsigned char g_compoundBytes[16];
void *g_lobbyVtable[3];
void *g_lobbyObject[1];
void *g_lobby3Vtable[15];
void *g_lobby3Object[1];

HRESULT __stdcall FakeLobbyQueryInterface(
    IDirectPlayLobby *,
    REFIID,
    void **outInterface
) {
    ++g_lobbyQueryCalls;
    if (outInterface != 0) {
        *outInterface = g_lobby3Object;
    }
    return S_OK;
}

ULONG __stdcall FakeLobbyRelease(IDirectPlayLobby *) {
    ++g_lobbyReleaseCalls;
    return 0;
}

HRESULT __stdcall FakeCreateCompoundAddress(
    IDirectPlayLobby3A *,
    LPCDPCOMPOUNDADDRESSELEMENT elements,
    DWORD elementCount,
    void *addressBuffer,
    DWORD *addressSize
) {
    ++g_compoundAddressCalls;
    g_compoundElementCount = elementCount;
    for (DWORD index = 0; index < elementCount && index < 2; ++index) {
        g_compoundElements[index] = elements[index];
    }
    if (addressSize != 0) {
        *addressSize = g_compoundRequiredSize;
    }
    if (addressBuffer != 0) {
        std::memcpy(addressBuffer, g_compoundBytes, g_compoundRequiredSize);
    }
    return S_OK;
}

HRESULT WINAPI FakeDirectPlayLobbyCreateA(
    LPGUID,
    LPDIRECTPLAYLOBBYA *outLobby,
    IUnknown *,
    void *,
    DWORD
) {
    ++g_lobbyCreateCalls;
    if (outLobby != 0) {
        *outLobby = reinterpret_cast<LPDIRECTPLAYLOBBYA>(g_lobbyObject);
    }
    return S_OK;
}

void ResetLobbyFakes() {
    g_createInterfaceCalls = 0;
    g_createInterfaceValue = MakeDirectPlayFake();
    g_createInterfaceResult = 0;
    g_lobbyCreateCalls = 0;
    g_lobbyQueryCalls = 0;
    g_lobbyReleaseCalls = 0;
    g_compoundAddressCalls = 0;
    std::memset(g_compoundElements, 0, sizeof(g_compoundElements));
    g_compoundElementCount = 0;
    g_compoundRequiredSize = sizeof(g_compoundBytes);
    for (int index = 0; index < static_cast<int>(sizeof(g_compoundBytes)); ++index) {
        g_compoundBytes[index] = static_cast<unsigned char>(0x90 + index);
    }
    g_lobbyVtable[0] = reinterpret_cast<void *>(&FakeLobbyQueryInterface);
    g_lobbyVtable[1] = 0;
    g_lobbyVtable[2] = reinterpret_cast<void *>(&FakeLobbyRelease);
    g_lobbyObject[0] = g_lobbyVtable;
    std::memset(g_lobby3Vtable, 0, sizeof(g_lobby3Vtable));
    g_lobby3Vtable[14] = reinterpret_cast<void *>(&FakeCreateCompoundAddress);
    g_lobby3Object[0] = g_lobby3Vtable;
}
} // namespace

extern "C" int znetwork_remove_player_record_by_key_smoke(void) {
    ResetNetworkFakes();

    zNetworkPlayerRecordList *const list = AllocZeroed<zNetworkPlayerRecordList>();
    zNetworkPlayerRecordListNode *const sentinel =
        AllocZeroed<zNetworkPlayerRecordListNode>();
    zNetworkPlayerRecordListNode *const removeNode =
        AllocZeroed<zNetworkPlayerRecordListNode>();
    zNetworkPlayerRecordListNode *const keepNode =
        AllocZeroed<zNetworkPlayerRecordListNode>();
    zNetwork_PlayerRecord *const removeRecord =
        AllocZeroed<zNetwork_PlayerRecord>();
    zNetwork_PlayerRecord *const keepRecord =
        AllocZeroed<zNetwork_PlayerRecord>();

    removeRecord->playerKey = 0x11112222;
    removeRecord->colorIndex = 5;
    keepRecord->playerKey = 0x33334444;
    keepRecord->colorIndex = 6;
    sentinel->next = removeNode;
    sentinel->prev = keepNode;
    removeNode->next = keepNode;
    removeNode->prev = sentinel;
    removeNode->playerRecord = removeRecord;
    keepNode->next = sentinel;
    keepNode->prev = removeNode;
    keepNode->playerRecord = keepRecord;
    list->sentinelNode = sentinel;
    list->count = 2;
    g_zNetwork_PlayerRecordList = list;
    g_zNetwork_PlayerColorInUseFlags[5] = 1;
    g_zNetwork_PlayerColorInUseFlags[6] = 1;

    zNetwork::RemovePlayerRecordByKey(0x11112222);
    const bool removedOk =
        list->count == 1 &&
        sentinel->next == keepNode &&
        sentinel->prev == keepNode &&
        keepNode->next == sentinel &&
        keepNode->prev == sentinel &&
        keepNode->playerRecord == keepRecord &&
        g_zNetwork_PlayerColorInUseFlags[5] == 0 &&
        g_zNetwork_PlayerColorInUseFlags[6] == 1;

    zNetwork::RemovePlayerRecordByKey(0x99999999);
    const bool missingOk =
        list->count == 1 &&
        sentinel->next == keepNode &&
        sentinel->prev == keepNode;

    ::operator delete(removeRecord);
    ::operator delete(keepRecord);
    ::operator delete(keepNode);
    ::operator delete(sentinel);
    ::operator delete(list);
    g_zNetwork_PlayerRecordList = 0;
    return removedOk && missingOk ? 0 : 1;
}

extern "C" int znetwork_host_send_player_color_assignments_packet_smoke(void) {
    ResetNetworkFakes();
    g_zNetwork_pDirectPlay4 = MakeDirectPlayFake();

    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0xaaaabbbb;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = localPlayer.playerKey;
    g_zNetwork_IsHostFlag = 1;

    zNetworkDPlaySessionDescCache session = {};
    session.desc.dwMaxPlayers = 4;
    g_zNetwork_CurrentSessionDescCache = &session;
    g_zNetwork_PlayerColorInUseFlags[1] = 1;

    zNetworkPlayerRecordList *const list = AllocZeroed<zNetworkPlayerRecordList>();
    zNetworkPlayerRecordListNode *const sentinel =
        AllocZeroed<zNetworkPlayerRecordListNode>();
    zNetworkPlayerRecordListNode *const joinNode =
        AllocZeroed<zNetworkPlayerRecordListNode>();
    zNetworkPlayerRecordListNode *const otherNode =
        AllocZeroed<zNetworkPlayerRecordListNode>();
    zNetwork_PlayerRecord *const joinRecord =
        AllocZeroed<zNetwork_PlayerRecord>();
    zNetwork_PlayerRecord *const otherRecord =
        AllocZeroed<zNetwork_PlayerRecord>();
    joinRecord->playerKey = 0x11112222;
    otherRecord->playerKey = 0x33334444;
    otherRecord->colorIndex = 4;
    sentinel->next = joinNode;
    sentinel->prev = otherNode;
    joinNode->next = otherNode;
    joinNode->prev = sentinel;
    joinNode->playerRecord = joinRecord;
    otherNode->next = sentinel;
    otherNode->prev = joinNode;
    otherNode->playerRecord = otherRecord;
    list->sentinelNode = sentinel;
    list->count = 2;
    g_zNetwork_PlayerRecordList = list;

    zNetwork::HostSendPlayerColorAssignmentsPacket(joinRecord->playerKey);
    struct SentPacket {
        zNetworkPacketHeader header;
        int pairCount;
        zNetworkPlayerColorPair pairs[2];
    } packet = {};
    if (g_sendPacketSize == sizeof(packet)) {
        std::memcpy(&packet, g_sendBytes, sizeof(packet));
    }
    const bool packetOk =
        g_sendCalls == 1 &&
        g_sendFrom == static_cast<DPID>(localPlayer.playerKey) &&
        g_sendPacketSize == sizeof(packet) &&
        packet.header.packetType == 1 &&
        packet.header.packetSizeBytes == sizeof(packet) &&
        packet.pairCount == 2 &&
        packet.pairs[0].playerKey == joinRecord->playerKey &&
        packet.pairs[0].colorIndex == 2 &&
        packet.pairs[1].playerKey == otherRecord->playerKey &&
        packet.pairs[1].colorIndex == 4 &&
        joinRecord->colorIndex == 2;

    g_sendCalls = 0;
    zNetwork::HostSendPlayerColorAssignmentsPacket(0x77777777);
    const bool missingOk = g_sendCalls == 0;
    g_zNetwork_IsHostFlag = 0;
    zNetwork::HostSendPlayerColorAssignmentsPacket(joinRecord->playerKey);
    const bool nonHostOk = g_sendCalls == 0;

    ::operator delete(joinRecord);
    ::operator delete(otherRecord);
    ::operator delete(joinNode);
    ::operator delete(otherNode);
    ::operator delete(sentinel);
    ::operator delete(list);
    g_zNetwork_PlayerRecordList = 0;
    g_zNetwork_CurrentSessionDescCache = 0;
    g_zNetwork_LocalPlayerRecord = 0;
    g_zNetwork_pDirectPlay4 = 0;
    return packetOk && missingOk && nonHostOk ? 0 : 1;
}

extern "C" int znetwork_enumerated_session_accessors_smoke(void) {
    ResetNetworkFakes();

    char name0[] = "alpha";
    char name1[] = "bravo";
    zNetworkDPlaySessionDescCache session0 = {};
    zNetworkDPlaySessionDescCache session1 = {};
    session0.desc.dwMaxPlayers = 8;
    session0.desc.dwCurrentPlayers = 3;
    session0.desc.lpszSessionNameA = name0;
    session1.desc.dwMaxPlayers = 12;
    session1.desc.dwCurrentPlayers = 5;
    session1.desc.lpszSessionNameA = name1;

    zArchiveListNode node0 = {};
    zArchiveListNode node1 = {};
    node0.payload = &session0;
    node0.next = &node1;
    node0.prev = &node1;
    node1.payload = &session1;
    node1.next = &node0;
    node1.prev = &node0;
    zArchiveList list = {};
    list.count = 2;
    list.head = &node0;
    g_zNetwork_EnumeratedSessionList = &list;

    const bool namesOk =
        zNetworkDPlay::GetEnumeratedSessionNameByIndex(0) == name0 &&
        zNetworkDPlay::GetEnumeratedSessionNameByIndex(1) == name1 &&
        zNetworkDPlay::GetEnumeratedSessionNameByIndex(2) == 0;

    int currentPlayers = -1;
    int maxPlayers = -1;
    zNetworkDPlay::GetEnumeratedSessionPlayerCountsByIndex(
        1,
        &currentPlayers,
        &maxPlayers
    );
    const bool countsOk = currentPlayers == 5 && maxPlayers == 12;

    currentPlayers = 77;
    maxPlayers = 88;
    zNetworkDPlay::GetEnumeratedSessionPlayerCountsByIndex(
        2,
        &currentPlayers,
        &maxPlayers
    );
    const bool invalidOk = currentPlayers == 77 && maxPlayers == 88;
    g_zNetwork_EnumeratedSessionList = 0;
    return namesOk && countsOk && invalidOk ? 0 : 1;
}

extern "C" int znetwork_dplay_enum_session_callback_smoke(void) {
    ResetNetworkFakes();
    if (g_zUtil_ZRDR_FreePool == 0) {
        zUtil::ZRDR_PreallocNodePool(1);
    }

    zArchiveList list = {};
    g_zNetwork_EnumeratedSessionList = &list;
    char sessionName[] = "hosted";
    char password[] = "secret";
    int reservedData = 0x1357;
    zNetworkDPlaySessionDesc desc = {};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0x44;
    FillGuid(desc.guidInstance, 0xa0);
    FillGuid(desc.guidApplication, 0xc0);
    desc.dwMaxPlayers = 8;
    desc.dwCurrentPlayers = 2;
    desc.lpszSessionNameA = sessionName;
    desc.lpszPasswordA = password;
    desc.dwReserved1 = reinterpret_cast<DWORD>(&reservedData);
    desc.dwReserved2 = sizeof(reservedData);
    desc.dwUser1 = 3;
    desc.dwUser2 = 4;
    desc.dwUser3 = 5;
    desc.dwUser4 = 6;

    const bool nullOk =
        zNetworkDPlay::EnumSessionCallback_AddSessionDescCache(0, 0, 0, 0) == 0 &&
        list.count == 0;
    const bool added =
        zNetworkDPlay::EnumSessionCallback_AddSessionDescCache(&desc, 0, 0, 0) == 1 &&
        list.count == 1 &&
        list.head != 0;
    zNetworkDPlaySessionDescCache *const cache =
        added
            ? static_cast<zNetworkDPlaySessionDescCache *>(list.head->payload)
            : 0;
    const bool copied =
        cache != 0 &&
        cache->desc.dwFlags == desc.dwFlags &&
        std::memcmp(&cache->desc.guidInstance, &desc.guidInstance, sizeof(GUID)) == 0 &&
        cache->desc.dwMaxPlayers == 8 &&
        cache->desc.dwCurrentPlayers == 2 &&
        cache->desc.lpszSessionNameA != sessionName &&
        std::strcmp(cache->desc.lpszSessionNameA, sessionName) == 0 &&
        cache->desc.lpszPasswordA == password &&
        cache->desc.dwUser4 == 6;

    if (cache != 0) {
        std::free(cache->desc.lpszSessionNameA);
        std::free(cache);
    }
    if (list.head != 0) {
        zUtil_ZRDR_PushFreeNode(list.head);
    }
    g_zNetwork_EnumeratedSessionList = 0;
    return nullOk && added && copied ? 0 : 1;
}

extern "C" int znetwork_dplay_enum_sessions_smoke(void) {
    ResetNetworkFakes();
    if (g_zUtil_ZRDR_FreePool == 0) {
        zUtil::ZRDR_PreallocNodePool(2);
    }

    GUID appGuid = {};
    FillGuid(appGuid, 0x40);
    zArchiveList list = {};
    g_zNetwork_AppGuid = &appGuid;
    g_zNetwork_EnumeratedSessionList = &list;

    const bool noInterfaceOk =
        zNetwork_DPlay::EnumSessions() == 0 && g_enumCalls == 0;
    g_zNetwork_pDirectPlay4 = MakeDirectPlayFake();
    const bool successOk =
        zNetwork_DPlay::EnumSessions() == 1 &&
        g_enumCalls == 1 &&
        g_enumDesc.dwSize == sizeof(zNetworkDPlaySessionDesc) &&
        std::memcmp(&g_enumDesc.guidApplication, &appGuid, sizeof(GUID)) == 0 &&
        g_enumTimeout == 0 &&
        g_enumCallback ==
            zNetworkDPlay::EnumSessionCallback_AddSessionDescCache &&
        g_enumContext == 0 &&
        g_enumFlags == 2 &&
        list.count == 1;

    zNetwork::ClearEnumeratedSessionList();
    const bool clearOk = list.count == 0;
    g_enumResult = static_cast<HRESULT>(0x88770118);
    const bool noSessionsOk = zNetwork_DPlay::EnumSessions() == -1;

    g_zNetwork_pDirectPlay4 = 0;
    g_zNetwork_EnumeratedSessionList = 0;
    g_zNetwork_AppGuid = 0;
    return noInterfaceOk && successOk && clearOk && noSessionsOk ? 0 : 1;
}

extern "C" int znetwork_dplay_enum_sessions_for_current_app_smoke(void) {
    ResetNetworkFakes();
    GUID appGuid = {};
    FillGuid(appGuid, 0x70);
    g_zNetwork_AppGuid = &appGuid;
    g_zNetwork_EnumeratedSessionList = zArchiveList_CreateEmpty();

    const bool noInterfaceOk =
        zNetworkDPlay::EnumSessionsForCurrentApp() == 0 && g_enumCalls == 0;
    g_zNetwork_pDirectPlay4 = MakeDirectPlayFake();
    g_enumResult = -33;
    const bool callOk =
        zNetworkDPlay::EnumSessionsForCurrentApp() == -33 &&
        g_enumCalls == 1 &&
        g_enumDesc.dwSize == sizeof(zNetworkDPlaySessionDesc) &&
        std::memcmp(&g_enumDesc.guidApplication, &appGuid, sizeof(GUID)) == 0 &&
        g_enumTimeout == 0 &&
        g_enumCallback ==
            zNetworkDPlay::EnumSessionCallback_AddSessionDescCache &&
        g_enumContext == 0 &&
        g_enumFlags == 0x82;

    zArchiveList_Destroy(g_zNetwork_EnumeratedSessionList);
    g_zNetwork_EnumeratedSessionList = 0;
    g_zNetwork_pDirectPlay4 = 0;
    g_zNetwork_AppGuid = 0;
    return noInterfaceOk && callOk ? 0 : 1;
}

extern "C" int znetwork_dplay_open_selected_session_and_read_status_fields_smoke(void) {
    ResetNetworkFakes();
    g_zNetwork_pDirectPlay4 = MakeDirectPlayFake();

    char sessionName[] = "selected";
    zNetworkDPlaySessionDescCache session = {};
    session.desc.lpszSessionNameA = sessionName;
    session.desc.dwUser1 = 12;
    session.desc.dwUser2 = 34;
    session.desc.dwUser3 = 56;
    session.desc.dwUser4 = 78;
    session.desc.dwMaxPlayers = 10;
    zArchiveListNode node = {};
    node.payload = &session;
    node.next = &node;
    node.prev = &node;
    zArchiveList list = {};
    list.count = 1;
    list.head = &node;
    g_zNetwork_EnumeratedSessionList = &list;

    zNetworkSessionDescStatusFields fields = {};
    fields.selectedSessionIndex = 0;
    const bool opened =
        zNetworkDPlay::OpenSelectedSessionAndReadStatusFields(&fields) == 1 &&
        g_zNetwork_CurrentSessionDescCache == &session &&
        session.openMode == 1 &&
        session.desc.dwSize == sizeof(zNetworkDPlaySessionDesc) &&
        g_openCalls == 1 &&
        g_openDesc == &session.desc &&
        g_openFlags == 1 &&
        g_getCapsCalls == 1 &&
        fields.eventCode == 12 &&
        fields.statusFlags == 34 &&
        fields.valueOrTime == 56 &&
        fields.auxParam == 78 &&
        fields.maxPlayers == 10 &&
        std::strcmp(fields.sessionNameBuf, sessionName) == 0;

    fields.selectedSessionIndex = 3;
    g_zNetwork_CurrentSessionDescCache = &session;
    const bool invalidOk =
        zNetworkDPlay::OpenSelectedSessionAndReadStatusFields(&fields) == 0 &&
        g_zNetwork_CurrentSessionDescCache == 0;

    g_zNetwork_pDirectPlay4 = 0;
    g_zNetwork_EnumeratedSessionList = 0;
    return opened && invalidOk ? 0 : 1;
}

extern "C" int znetwork_dplay_select_tcp_ip_provider_and_enum_sessions_smoke(void) {
    CodeFunctionPatch lobbyPatch = {};
    CodeFunctionPatch createInterfacePatch = {};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&DirectPlayLobbyCreateA),
            reinterpret_cast<void *>(&FakeDirectPlayLobbyCreateA),
            lobbyPatch
        )) {
        return 90;
    }
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zNetwork_DPlay::CreateInterfaceAndCoInitialize),
            reinterpret_cast<void *>(&FakeCreateInterfaceAndCoInitialize),
            createInterfacePatch
        )) {
        RestoreFunctionPatch(lobbyPatch);
        return 91;
    }

    ResetNetworkFakes();
    ResetLobbyFakes();
    GUID appGuid = {};
    char address[] = "127.0.0.1";
    g_zNetwork_AppGuid = &appGuid;

    const int skipResult =
        zNetworkDPlay::SelectTcpIpProviderAndEnumSessions(address, 1);
    const bool providerOk =
        skipResult == 1 &&
        g_lobbyCreateCalls == 1 &&
        g_lobbyQueryCalls == 1 &&
        g_lobbyReleaseCalls == 1 &&
        g_compoundAddressCalls == 2 &&
        g_compoundElementCount == 2 &&
        std::memcmp(
            &g_compoundElements[0].guidDataType,
            &DPAID_ServiceProvider,
            sizeof(GUID)
        ) == 0 &&
        g_compoundElements[0].lpData == &DPSPGUID_TCPIP &&
        std::memcmp(
            &g_compoundElements[1].guidDataType,
            &DPAID_INet,
            sizeof(GUID)
        ) == 0 &&
        g_compoundElements[1].lpData == address &&
        g_initializeCalls == 1 &&
        g_initializeFlags == 0 &&
        std::memcmp(
            g_initializeBytes,
            g_compoundBytes,
            sizeof(g_initializeBytes)
        ) == 0 &&
        g_zNetwork_ActiveProviderIsTcpIp != 0 &&
        g_zNetwork_TcpIpAsyncSendEnabled != 0 &&
        g_enumCalls == 0;

    if (g_zNetwork_EnumeratedSessionList != 0) {
        zArchiveList_Destroy(g_zNetwork_EnumeratedSessionList);
    }
    g_zNetwork_EnumeratedSessionList = zArchiveList_CreateEmpty();
    g_suppressEnumCallback = true;
    g_enumConnectingRepeats = 2;
    g_enumResult = DP_OK;
    const bool enumOk =
        zNetworkDPlay::SelectTcpIpProviderAndEnumSessions(address, 0) == 1 &&
        g_enumCalls == 3 &&
        g_enumFlags == 0x82;

    zArchiveList_Destroy(g_zNetwork_EnumeratedSessionList);
    g_zNetwork_EnumeratedSessionList = 0;
    g_zNetwork_pDirectPlay4 = 0;
    g_zNetwork_AppGuid = 0;
    RestoreFunctionPatch(createInterfacePatch);
    RestoreFunctionPatch(lobbyPatch);
    return providerOk && enumOk ? 0 : 1;
}
