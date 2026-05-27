#include "Battlesport/GameNet.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/RecoilApp.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/include/zClipRect.h"

#include <cstdio>
#include <cstring>

namespace {
int g_setSessionDescCalls;
std::int32_t g_setSessionDescResult;
int g_sendCalls;
std::uint32_t g_sendFlags;
void *g_sendPacket;
std::uint32_t g_sendPacketSize;
std::uint32_t g_sendPacketBytesSize;
unsigned char g_sendPacketBytes[0x200];
int g_chatComposeSetTextFmtCalls;
HudUiPanel *g_chatComposeSetTextFmtThis;
char g_chatComposeSetTextFmtText[32];
int g_remoteHudSetVisibleCount;
int g_remoteHudLastVisible;
int g_remoteHudSetPosCount;
HudUiPanel *g_remoteHudSetPosThis;
int g_remoteHudLastX;
int g_remoteHudLastY;
int g_spawnRemoteSetVisibleCount;
int g_spawnRemoteLastVisible;
int g_pkt14StateEnterCount;
int g_qsandRelayCallbackCount;
int g_qsandRelayCallbackResult;

struct ScoreboardPacket2 {
    zNetworkPacketHeader header;
    std::int32_t entryCount;
    NetPkt09_PlayerScoreboardEntry entries[2];
};

template <typename T> T &FieldAt(void *object, std::size_t offset) {
    return *reinterpret_cast<T *>(static_cast<unsigned char *>(object) + offset);
}

void MakeGameNetReaderFloatNode(zReader::Node &node, float value) {
    node.type = zReader::ZRDR_NODE_FLOAT;
    node.value.f32 = value;
}

void MakeGameNetReaderArrayNode(zReader::Node &node, zReader::Node *payload, int count) {
    payload[0].type = zReader::ZRDR_NODE_INT;
    payload[0].value.i32 = count;
    node.type = zReader::ZRDR_NODE_ARRAY;
    node.value.nodes = payload;
}

bool WriteGameNetZrdU32(std::FILE *file, unsigned int value) {
    return std::fwrite(&value, sizeof(value), 1, file) == 1;
}

bool WriteGameNetZrdNode(std::FILE *file, const zReader::Node &node) {
    if (!WriteGameNetZrdU32(file, static_cast<unsigned int>(node.type))) {
        return false;
    }

    switch (node.type) {
    case zReader::ZRDR_NODE_INT:
    case zReader::ZRDR_NODE_FLOAT:
        return WriteGameNetZrdU32(file, node.value.u32);
    case zReader::ZRDR_NODE_STRING: {
        const unsigned int length = static_cast<unsigned int>(std::strlen(node.value.str));
        return WriteGameNetZrdU32(file, length) &&
               std::fwrite(node.value.str, 1, length, file) == length;
    }
    case zReader::ZRDR_NODE_ARRAY: {
        const int count = node.value.nodes[0].value.i32;
        if (!WriteGameNetZrdU32(file, static_cast<unsigned int>(count))) {
            return false;
        }
        for (int index = 1; index < count; ++index) {
            if (!WriteGameNetZrdNode(file, node.value.nodes[index])) {
                return false;
            }
        }
        return true;
    }
    }

    return false;
}

bool WriteGameNetZrdFile(const char *path, const zReader::Node &root) {
    std::FILE *const file = std::fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }

    const bool ok = WriteGameNetZrdNode(file, root);
    return std::fclose(file) == 0 && ok;
}

struct GameNetZrdArchiveEntry {
    const char *name;
    const zReader::Node *root;
};

bool MountGameNetZrdArchive(const char *path, const GameNetZrdArchiveEntry *entries,
                            int entryCount, zIndexArchive &archive, zZarFileRecord *records,
                            zArchiveListNode &archiveNode, zArchiveList &archiveList) {
    std::FILE *const file = std::fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }

    bool ok = true;
    for (int index = 0; index < entryCount; ++index) {
        const long offset = std::ftell(file);
        if (offset < 0 || !WriteGameNetZrdNode(file, *entries[index].root)) {
            ok = false;
            break;
        }
        const long endOffset = std::ftell(file);
        if (endOffset < offset) {
            ok = false;
            break;
        }

        records[index] = {};
        records[index].fileOffset = static_cast<unsigned int>(offset);
        records[index].fileSize = static_cast<unsigned int>(endOffset - offset);
        std::strcpy(records[index].name, entries[index].name);
    }

    if (std::fclose(file) != 0 || !ok) {
        std::remove(path);
        return false;
    }

    archive = {};
    archive.hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, nullptr);
    if (archive.hFile == INVALID_HANDLE_VALUE) {
        std::remove(path);
        return false;
    }

    archive.recordCount = static_cast<unsigned int>(entryCount);
    archive.records = records;

    archiveNode = {};
    archiveNode.payload = &archive;
    archiveNode.next = &archiveNode;
    archiveNode.prev = &archiveNode;

    archiveList = {};
    archiveList.count = 1;
    archiveList.head = &archiveNode;
    g_zArchive_MountedList = &archiveList;
    return true;
}

template <typename Method> unsigned int MethodAddress(Method method) {
    union {
        Method method;
        unsigned int address;
    } value = {method};
    return value.address;
}

struct TestPkt14AppState : RecoilApp_IState {
    void RECOIL_THISCALL OnEnter() {
        ++g_pkt14StateEnterCount;
    }
};

RecoilApp_IState_Vtbl MakePkt14StateVtable() {
    RecoilApp_IState_Vtbl vtable{};
    vtable.OnEnter = MethodAddress(&TestPkt14AppState::OnEnter);
    return vtable;
}

RecoilApp_IState_Vtbl g_pkt14StateVtable = MakePkt14StateVtable();

void CleanupSingleQueuedItem(RecoilApp_StateQueue &queue) {
    const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
    RecoilPtr32 *const slot = reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
    RecoilApp_StateQueueItem *const item =
        reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
    RecoilPtr32 *const chunkList =
        reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(queue.m_chunkPtrList));
    void *const chunk = reinterpret_cast<void *>(static_cast<std::uintptr_t>(chunkList[1]));
    ::operator delete(item);
    ::operator delete(chunk);
    ::operator delete(chunkList);
}

bool FloatNear(float actual, float expected) {
    return actual >= expected - 0.0001f && actual <= expected + 0.0001f;
}

bool Vec3Equals(const zVec3 &value, const zVec3 &expected) {
    return FloatNear(value.x, expected.x) && FloatNear(value.y, expected.y) &&
           FloatNear(value.z, expected.z);
}

void DeleteTopMessageStackFonts(HudUiTopMessageStack &stack) {
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&stack.lines[index][0]);
        DeleteObject(panel->hFont);
        panel->hFont = nullptr;
    }
}

std::int32_t RECOIL_STDCALL SetSessionDescFake(zNetwork_DPlay4 *, zNetworkDPlaySessionDesc *,
                                               std::uint32_t) {
    ++g_setSessionDescCalls;
    return g_setSessionDescResult;
}

std::int32_t RECOIL_STDCALL SendFake(zNetwork_DPlay4 *, std::uint32_t, std::uint32_t,
                                     std::uint32_t flags, void *packet,
                                     std::uint32_t packetSizeBytes) {
    ++g_sendCalls;
    g_sendFlags = flags;
    g_sendPacket = packet;
    g_sendPacketSize = packetSizeBytes;
    g_sendPacketBytesSize = packetSizeBytes;
    if (packetSizeBytes <= sizeof(g_sendPacketBytes)) {
        std::memcpy(g_sendPacketBytes, packet, packetSizeBytes);
    }
    return 0;
}

void RECOIL_CDECL ChatComposeSetTextFmtFake(HudUiPanel *self, const char *format, ...) {
    ++g_chatComposeSetTextFmtCalls;
    g_chatComposeSetTextFmtThis = self;
    std::strncpy(g_chatComposeSetTextFmtText, format != nullptr ? format : "", sizeof(g_chatComposeSetTextFmtText));
    g_chatComposeSetTextFmtText[sizeof(g_chatComposeSetTextFmtText) - 1] = '\0';
}

int RECOIL_FASTCALL QSandRelayCallbackFake(void *) {
    ++g_qsandRelayCallbackCount;
    return g_qsandRelayCallbackResult;
}

struct TestRemoteHudPanelOps {
    void RECOIL_THISCALL SetPos(int x, int y) {
        ++g_remoteHudSetPosCount;
        g_remoteHudSetPosThis = reinterpret_cast<HudUiPanel *>(this);
        g_remoteHudLastX = x;
        g_remoteHudLastY = y;
    }

    void RECOIL_THISCALL SetVisible(int visible) {
        ++g_remoteHudSetVisibleCount;
        g_remoteHudLastVisible = visible;
    }
};

struct TestSpawnRemoteHudPanelOps {
    void RECOIL_THISCALL SetVisible(int visible) {
        ++g_spawnRemoteSetVisibleCount;
        g_spawnRemoteLastVisible = visible;
    }
};

const zNetwork_DPlay4Vtable kDPlayVtable = {
    {}, nullptr, {}, nullptr, {}, nullptr, SendFake, {}, SetSessionDescFake, {}, nullptr, {}, nullptr,
};

void ClearDispatchHandlerListForTest(zNetworkDispatchHandlerListNode &sentinel) {
    zNetworkDispatchHandlerListNode *node = sentinel.next;
    while (node != &sentinel) {
        zNetworkDispatchHandlerListNode *const next = node->next;
        ::operator delete(node->record);
        ::operator delete(node);
        node = next;
    }
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
}
} // namespace

extern "C" int gamenet_list_reset_smoke(void) {
    g_GameNetSpawnPointList = 1;
    g_GameNetSpawnPointHead = reinterpret_cast<GameNetSpawnPoint *>(2);
    g_GameNetSpawnPointTail = reinterpret_cast<GameNetSpawnPoint *>(3);
    g_GameNetSpawnPointCount = 4;
    GameNetSpawnPointList::InitGlobals();
    if (g_GameNetSpawnPointList != 0 || g_GameNetSpawnPointHead != nullptr ||
        g_GameNetSpawnPointTail != nullptr || g_GameNetSpawnPointCount != 0) {
        return 1;
    }

    g_GameNetPlayerRowList = 5;
    g_GameNetPlayerRowHead = reinterpret_cast<GameNetPlayerRow *>(6);
    g_GameNetPlayerRowTail = reinterpret_cast<GameNetPlayerRow *>(7);
    g_GameNetPlayerRowCount = 8;
    GameNetPlayerRowList::Reset();
    return g_GameNetPlayerRowList == 0 && g_GameNetPlayerRowHead == nullptr &&
                   g_GameNetPlayerRowTail == nullptr && g_GameNetPlayerRowCount == 0
               ? 0
               : 2;
}

extern "C" int gamenet_player_row_append_smoke(void) {
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const unsigned int oldList = g_GameNetPlayerRowList;
    const unsigned int oldCount = g_GameNetPlayerRowCount;

    g_GameNetPlayerRowList = 0;
    g_GameNetPlayerRowHead = nullptr;
    g_GameNetPlayerRowTail = nullptr;
    g_GameNetPlayerRowCount = 0;

    GameNetPlayerRowListState *const list =
        (GameNetPlayerRowListState *)(&g_GameNetPlayerRowList);
    GameNetPlayerRow *const first = GameNetPlayerRowList::AppendNewRow(list, 0);
    GameNetPlayerRow *const second = GameNetPlayerRowList::AppendNewRow(list, 1);
    const bool ok = first != nullptr && second != nullptr && first != second &&
                    g_GameNetPlayerRowHead == first && g_GameNetPlayerRowTail == second &&
                    g_GameNetPlayerRowCount == 2 && first->next == second &&
                    second->next == nullptr && second->playerKey == 0 &&
                    second->displayName[0] == 0;

    ::operator delete(second);
    ::operator delete(first);
    g_GameNetPlayerRowList = oldList;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    return ok ? 0 : 1;
}

extern "C" int gamenet_unregister_gameplay_packet_handlers_smoke(void) {
    zNetworkDispatchHandlerListNode *const oldSentinel = g_zNetwork_DispatchHandlerListSentinel;
    const int oldCount = g_zNetwork_DispatchHandlerListCount;
    const int oldRegistered = g_GameNet_HandlersRegistered;

    zNetworkDispatchHandlerListNode sentinel = {};
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    g_zNetwork_DispatchHandlerListSentinel = &sentinel;
    g_zNetwork_DispatchHandlerListCount = 0;
    g_GameNet_HandlersRegistered = 1;

    zNetwork::RegisterPacketHandler(6, (zNetworkPacketHandler)&GameNet::HandlePkt06_PlayerStateSnapshot, 0);
    zNetwork::RegisterPacketHandler(7, (zNetworkPacketHandler)&GameNet::HandlePkt07_AltGunDispatch, 0);
    zNetwork::RegisterPacketHandler(
        0x0a, (zNetworkPacketHandler)&OptCatalog::HandlePkt0A_RemoveRuntimeRelay, 0);
    zNetwork::RegisterPacketHandler(1, (zNetworkPacketHandler)&GameNet::ReassignPlayerColorsAndRefreshRows, 0);
    zNetwork::RegisterPacketHandler(8, (zNetworkPacketHandler)&GameNet::HandlePkt08_PlayerKillEvent, 0);
    zNetwork::RegisterPacketHandler(9, (zNetworkPacketHandler)&GameNet::HandlePkt09_PlayerScoreboardSnapshot, 0);
    zNetwork::RegisterPacketHandler(0x0b, (zNetworkPacketHandler)&GameNet::HandlePkt0B_ChatMessage, 0);
    zNetwork::RegisterPacketHandler(0x0e, (zNetworkPacketHandler)&GameNet::HandlePkt0E_PlayerLapProgress, 0);
    zNetwork::RegisterPacketHandler(0x0c, (zNetworkPacketHandler)&GameNet::HandlePkt0C_HudTimerStatusBits, 0);
    zNetwork::RegisterPacketHandler(0x0d, (zNetworkPacketHandler)&GameNet::HandlePkt0D_HudTimerPanelState, 0);
    zNetwork::RegisterPacketHandler(0x0f, (zNetworkPacketHandler)&zDEClient_Crater::NetRelayCallback, 0);
    zNetwork::RegisterPacketHandler(0x10, (zNetworkPacketHandler)&zDEClient_QSand::NetRelayCallback, 0);
    zNetwork::RegisterPacketHandler(0x11, (zNetworkPacketHandler)&Pickup::HandlePkt11_SpawnDelta, 0);
    zNetwork::RegisterPacketHandler(
        0x12, (zNetworkPacketHandler)&Pickup::HandlePkt12_AirdropSpawnChuteRelay, 0);
    zNetwork::RegisterPacketHandler(
        0x13, (zNetworkPacketHandler)&GameNet::HandlePkt13_EffectAnimActivationRecord, 0);

    GameNet::UnregisterGameplayPacketHandlers();
    const bool ok = g_zNetwork_DispatchHandlerListCount == 0 && sentinel.next == &sentinel &&
                    sentinel.prev == &sentinel && g_GameNet_HandlersRegistered == 0;

    g_zNetwork_DispatchHandlerListSentinel = oldSentinel;
    g_zNetwork_DispatchHandlerListCount = oldCount;
    g_GameNet_HandlersRegistered = oldRegistered;

    return ok ? 0 : 1;
}

extern "C" int gamenet_register_gameplay_handlers_and_callbacks_smoke(void) {
    zNetworkDispatchHandlerListNode *const oldSentinel = g_zNetwork_DispatchHandlerListSentinel;
    const int oldCount = g_zNetwork_DispatchHandlerListCount;
    const int oldRegistered = g_GameNet_HandlersRegistered;
    zDEClient_NetRelayCallback const oldCraterRelay = g_zDEClientCraterNetRelayCallback;
    zDEClient_NetRelayCallback const oldQSandRelay = g_zDEClientQSandNetRelayCallback;
    OptCatalogAllocRuntimeGateCallback const oldAllocGate = g_OptCatalog_AllocRuntimeGateCallback;
    OptCatalogAllocRuntimeGateCallback const oldNoOpGate =
        g_OptCatalog_AltGunDispatchNoOpCallback;
    OptCatalogRemoveRuntimeRelayCallback const oldRemoveRelay =
        g_OptCatalog_RemoveRuntimeRelayCallback;
    void(RECOIL_FASTCALL *oldEffectDispatch)(zEffectAnimActivationRecord *) =
        g_zEffectAnim_ActivationDispatchCallback;
    const unsigned int oldEffectDispatchTag = g_zEffectAnim_ActivationDispatchTagHigh;

    zNetworkDispatchHandlerListNode sentinel{};
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    g_zNetwork_DispatchHandlerListSentinel = &sentinel;
    g_zNetwork_DispatchHandlerListCount = 0;
    g_GameNet_HandlersRegistered = 0;
    g_zDEClientCraterNetRelayCallback = nullptr;
    g_zDEClientQSandNetRelayCallback = nullptr;
    g_OptCatalog_AllocRuntimeGateCallback = nullptr;
    g_OptCatalog_AltGunDispatchNoOpCallback = nullptr;
    g_OptCatalog_RemoveRuntimeRelayCallback = nullptr;
    g_zEffectAnim_ActivationDispatchCallback = nullptr;
    g_zEffectAnim_ActivationDispatchTagHigh = 0;

    GameNet::RegisterGameplayHandlersAndOptCatalogCallbacks();

    unsigned int packetMask = 0;
    bool modesOk = true;
    for (zNetworkDispatchHandlerListNode *node = sentinel.next; node != &sentinel;
         node = node->next) {
        if (node->record == nullptr || node->record->mode != 2) {
            modesOk = false;
            break;
        }
        if (node->record->packetType >= 0 && node->record->packetType < 32) {
            packetMask |= 1u << static_cast<unsigned int>(node->record->packetType);
        }
    }

    const unsigned int expectedMask =
        (1u << 1) | (1u << 3) | (1u << 6) | (1u << 7) | (1u << 8) |
        (1u << 9) | (1u << 0x0a) | (1u << 0x0b) | (1u << 0x0c) |
        (1u << 0x0d) | (1u << 0x0e) | (1u << 0x0f) | (1u << 0x10) |
        (1u << 0x11) | (1u << 0x12) | (1u << 0x13) | (1u << 0x14);
    const bool registeredOk = g_GameNet_HandlersRegistered == 1 &&
                              g_zNetwork_DispatchHandlerListCount == 17 && modesOk &&
                              packetMask == expectedMask;
    const bool callbacksOk =
        g_zDEClientCraterNetRelayCallback ==
            (zDEClient_NetRelayCallback)&zDEClient_Crater::Execute &&
        g_zDEClientQSandNetRelayCallback ==
            (zDEClient_NetRelayCallback)&GameNet::SendPkt10_QSandEvent &&
        g_OptCatalog_AllocRuntimeGateCallback ==
            &OptCatalog::AltGunDispatchAllocRuntimeGateCallback &&
        g_OptCatalog_AltGunDispatchNoOpCallback == &GameNet::AltGunDispatchNoOpCallback &&
        g_OptCatalog_RemoveRuntimeRelayCallback == &OptCatalog::SendPkt0A_RemoveRuntimeRelay &&
        g_zEffectAnim_ActivationDispatchCallback ==
            &GameNet::SendPkt13_EffectAnimActivationRecord &&
        g_zEffectAnim_ActivationDispatchTagHigh == 0x0c000000u;

    GameNet::RegisterGameplayHandlersAndOptCatalogCallbacks();
    const bool noDuplicateOk = g_zNetwork_DispatchHandlerListCount == 17;

    ClearDispatchHandlerListForTest(sentinel);
    g_zNetwork_DispatchHandlerListSentinel = oldSentinel;
    g_zNetwork_DispatchHandlerListCount = oldCount;
    g_GameNet_HandlersRegistered = oldRegistered;
    g_zDEClientCraterNetRelayCallback = oldCraterRelay;
    g_zDEClientQSandNetRelayCallback = oldQSandRelay;
    g_OptCatalog_AllocRuntimeGateCallback = oldAllocGate;
    g_OptCatalog_AltGunDispatchNoOpCallback = oldNoOpGate;
    g_OptCatalog_RemoveRuntimeRelayCallback = oldRemoveRelay;
    g_zEffectAnim_ActivationDispatchCallback = oldEffectDispatch;
    g_zEffectAnim_ActivationDispatchTagHigh = oldEffectDispatchTag;

    return registeredOk && callbacksOk && noDuplicateOk ? 0 : 1;
}

extern "C" int gamenet_chat_compose_key_callback_smoke(void) {
    HudUiTextInput oldInput = g_HudUiMgrObjectiveChatComposeTextInput;
    HudUiPanel *const oldDescPanel = g_HudUiMgrObjectiveDescTextPanel;
    const int oldTableReady = g_zInput_KbdDikToAsciiTableReady;

    HudUiPanel_FTable panelTable{};
    panelTable.slots[0x74 / 4] = reinterpret_cast<unsigned int>(&ChatComposeSetTextFmtFake);
    HudUiPanel descPanel{};
    descPanel.vtbl = &panelTable;
    g_HudUiMgrObjectiveDescTextPanel = &descPanel;

    g_HudUiMgrObjectiveChatComposeTextInput = {};
    g_HudUiMgrObjectiveChatComposeTextInput.Constructor(8);
    g_HudUiMgrObjectiveChatComposeTextInput.buffer[0] = '\0';
    g_chatComposeSetTextFmtCalls = 0;
    g_chatComposeSetTextFmtThis = nullptr;
    g_chatComposeSetTextFmtText[0] = '\0';
    g_zInput_KbdDikToAsciiTableReady = 0;
    std::memset(g_zInput_KbdDikToAsciiTable, 0, sizeof(g_zInput_KbdDikToAsciiTable));

    GameNet::ChatComposeKeyCallback(0x41e);
    const bool inserted =
        std::strcmp(g_HudUiMgrObjectiveChatComposeTextInput.GetBuffer(), "A") == 0 &&
        g_HudUiMgrObjectiveChatComposeTextInput.cursor == 1 &&
        g_chatComposeSetTextFmtCalls == 1 &&
        g_chatComposeSetTextFmtThis == &descPanel &&
        std::strcmp(g_chatComposeSetTextFmtText, "A") == 0;

    GameNet::ChatComposeKeyCallback(0);
    const bool zeroIgnored = g_chatComposeSetTextFmtCalls == 1 &&
                             std::strcmp(g_HudUiMgrObjectiveChatComposeTextInput.GetBuffer(), "A") == 0;

    g_HudUiMgrObjectiveChatComposeTextInput.DestructorCore();
    g_HudUiMgrObjectiveChatComposeTextInput = oldInput;
    g_HudUiMgrObjectiveDescTextPanel = oldDescPanel;
    g_zInput_KbdDikToAsciiTableReady = oldTableReady;

    return inserted && zeroIgnored ? 0 : 1;
}

extern "C" int gamenet_begin_chat_compose_smoke(void) {
    int networkEnabled = 0;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    ZOPT_NETWORK_ENABLED = &networkEnabled;

    const int oldChatComposeActive = g_HudUiMgrObjectiveChatComposeActive;
    g_HudUiMgrObjectiveChatComposeActive = 77;
    GameNet::BeginChatCompose();
    const bool disabledOk = g_HudUiMgrObjectiveChatComposeActive == 77;

    HudUiTextInput oldInput = g_HudUiMgrObjectiveChatComposeTextInput;
    HudUiPanel *const oldSummaryPanel = g_HudUiMgrObjectiveSummaryTextPanel;
    HudUiPanel *const oldDescPanel = g_HudUiMgrObjectiveDescTextPanel;
    const int oldPhase = g_HudUiMgrObjectivePhase;
    const int oldState = g_HudUiMgrObjectiveState;
    const int oldShowReset = g_HudUiMgrObjectiveShowResetUnused;
    const float oldAutoHide = g_HudUiMgrObjectiveAutoHideDelaySec;
    zInput::KbdKeyDispatchEntry oldDispatch[0x7de];
    std::memcpy(oldDispatch, g_zInputKbdKeyDispatchTable, sizeof(oldDispatch));

    HudUiPanel_FTable panelTable{};
    panelTable.slots[0x74 / 4] = reinterpret_cast<unsigned int>(&ChatComposeSetTextFmtFake);
    HudUiPanel summaryPanel{};
    HudUiPanel descPanel{};
    summaryPanel.vtbl = &panelTable;
    descPanel.vtbl = &panelTable;
    g_HudUiMgrObjectiveSummaryTextPanel = &summaryPanel;
    g_HudUiMgrObjectiveDescTextPanel = &descPanel;
    g_HudUiMgrObjectiveWidget.ftable = &g_HudUiWidget_FTable;
    g_HudUiMgrObjectiveSensorRect.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiWidget_FTable);
    g_HudUiMgrSensorOverlay.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiWidget_FTable);
    g_HudUiMgrObjectiveBar.ftable = &g_HudUiBar_FTable;
    g_HudUiMgrObjectivePhase = 0;
    g_HudUiMgrObjectiveState = 0;
    g_HudUiMgrObjectiveChatComposeActive = 0;
    g_chatComposeSetTextFmtCalls = 0;

    g_HudUiMgrObjectiveChatComposeTextInput = {};
    g_HudUiMgrObjectiveChatComposeTextInput.Constructor(8);
    char *const initialBuffer = g_HudUiMgrObjectiveChatComposeTextInput.buffer;
    zInput::BindMapSystem_Init(1);

    networkEnabled = 1;
    GameNet::BeginChatCompose();
    const bool stateOk = g_HudUiMgrObjectiveChatComposeActive == 1 &&
                         g_HudUiMgrObjectiveState == 1 &&
                         g_HudUiMgrObjectivePhase == 1 &&
                         g_HudUiMgrObjectiveChatComposeTextInput.capacity == 32 &&
                         std::strcmp(g_HudUiMgrObjectiveChatComposeTextInput.GetBuffer(), "") == 0 &&
                         g_zInput_BindMapOverlayDepth == 1;
    const void *const callback = reinterpret_cast<void *>(&GameNet::ChatComposeKeyCallback);
    const bool keyOk =
        g_zInputKbdKeyDispatchTable[0x02].callback == callback &&
        g_zInputKbdKeyDispatchTable[0x402].callback == callback &&
        g_zInputKbdKeyDispatchTable[0x0e].callback == callback &&
        g_zInputKbdKeyDispatchTable[0x10].callback == callback &&
        g_zInputKbdKeyDispatchTable[0x42b].callback == callback &&
        g_zInputKbdKeyDispatchTable[0x1e].callback == callback &&
        g_zInputKbdKeyDispatchTable[0x428].callback == callback &&
        g_zInputKbdKeyDispatchTable[0x2c].callback == callback &&
        g_zInputKbdKeyDispatchTable[0x435].callback == callback &&
        g_zInputKbdKeyDispatchTable[0x39].callback == callback;

    ::operator delete(initialBuffer);
    g_HudUiMgrObjectiveChatComposeTextInput.DestructorCore();
    zInput::BindMapContext_Pop();
    zInput::BindMapSystem_Shutdown();

    g_HudUiMgrObjectiveChatComposeTextInput = oldInput;
    g_HudUiMgrObjectiveSummaryTextPanel = oldSummaryPanel;
    g_HudUiMgrObjectiveDescTextPanel = oldDescPanel;
    g_HudUiMgrObjectivePhase = oldPhase;
    g_HudUiMgrObjectiveState = oldState;
    g_HudUiMgrObjectiveChatComposeActive = oldChatComposeActive;
    g_HudUiMgrObjectiveShowResetUnused = oldShowReset;
    g_HudUiMgrObjectiveAutoHideDelaySec = oldAutoHide;
    std::memcpy(g_zInputKbdKeyDispatchTable, oldDispatch, sizeof(oldDispatch));
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;

    return disabledOk && stateOk && keyOk ? 0 : 1;
}

extern "C" int hud_ui_handle_hotkey_command_begin_chat_smoke(void) {
    int networkEnabled = 1;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    HudUiTextInput oldInput = g_HudUiMgrObjectiveChatComposeTextInput;
    HudUiPanel *const oldSummaryPanel = g_HudUiMgrObjectiveSummaryTextPanel;
    HudUiPanel *const oldDescPanel = g_HudUiMgrObjectiveDescTextPanel;
    const int oldChatComposeActive = g_HudUiMgrObjectiveChatComposeActive;
    const int oldPhase = g_HudUiMgrObjectivePhase;
    const int oldState = g_HudUiMgrObjectiveState;
    zInput::KbdKeyDispatchEntry oldDispatch[0x7de];
    std::memcpy(oldDispatch, g_zInputKbdKeyDispatchTable, sizeof(oldDispatch));

    HudUiPanel_FTable panelTable{};
    panelTable.slots[0x74 / 4] = reinterpret_cast<unsigned int>(&ChatComposeSetTextFmtFake);
    HudUiPanel summaryPanel{};
    HudUiPanel descPanel{};
    summaryPanel.vtbl = &panelTable;
    descPanel.vtbl = &panelTable;
    g_HudUiMgrObjectiveSummaryTextPanel = &summaryPanel;
    g_HudUiMgrObjectiveDescTextPanel = &descPanel;
    g_HudUiMgrObjectiveWidget.ftable = &g_HudUiWidget_FTable;
    g_HudUiMgrObjectiveSensorRect.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiWidget_FTable);
    g_HudUiMgrSensorOverlay.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiWidget_FTable);
    g_HudUiMgrObjectiveBar.ftable = &g_HudUiBar_FTable;
    g_HudUiMgrObjectivePhase = 0;
    g_HudUiMgrObjectiveState = 0;
    g_HudUiMgrObjectiveChatComposeActive = 0;

    g_HudUiMgrObjectiveChatComposeTextInput = {};
    g_HudUiMgrObjectiveChatComposeTextInput.Constructor(8);
    char *const initialBuffer = g_HudUiMgrObjectiveChatComposeTextInput.buffer;
    zInput::BindMapSystem_Init(1);

    HudUi::HandleHotkeyCommand(42);
    const void *const callback = reinterpret_cast<void *>(&GameNet::ChatComposeKeyCallback);
    const bool hotkeyOk = g_HudUiMgrObjectiveChatComposeActive == 1 &&
                          g_HudUiMgrObjectiveChatComposeTextInput.capacity == 32 &&
                          g_zInput_BindMapOverlayDepth == 1 &&
                          g_zInputKbdKeyDispatchTable[0x39].callback == callback &&
                          g_zInputKbdKeyDispatchTable[0x42b].callback == callback;

    ::operator delete(initialBuffer);
    g_HudUiMgrObjectiveChatComposeTextInput.DestructorCore();
    zInput::BindMapContext_Pop();
    zInput::BindMapSystem_Shutdown();

    g_HudUiMgrObjectiveChatComposeTextInput = oldInput;
    g_HudUiMgrObjectiveSummaryTextPanel = oldSummaryPanel;
    g_HudUiMgrObjectiveDescTextPanel = oldDescPanel;
    g_HudUiMgrObjectiveChatComposeActive = oldChatComposeActive;
    g_HudUiMgrObjectivePhase = oldPhase;
    g_HudUiMgrObjectiveState = oldState;
    std::memcpy(g_zInputKbdKeyDispatchTable, oldDispatch, sizeof(oldDispatch));
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;

    return hotkeyOk ? 0 : 1;
}

extern "C" int hud_timer_panel_net_state_clear_tail_flags_smoke(void) {
    HudTimerPanelNetState state{};
    for (std::uint32_t &flag : state.tailFlags) {
        flag = 0xffffffff;
    }

    state.ClearTailFlagsLocal();
    for (std::uint32_t flag : state.tailFlags) {
        if (flag != 0) {
            return 1;
        }
    }

    return 0;
}

extern "C" int gamenet_find_player_row_and_status_bits_smoke(void) {
    GameNetPlayerRow first{};
    GameNetPlayerRow second{};
    first.playerKey = 10;
    first.lapCount = 3;
    first.next = &second;
    second.playerKey = 20;
    second.lapCount = 4;
    g_GameNetPlayerRowHead = &first;
    g_HudSensorTracker.runtimeGoalValue = 3;

    const bool rowLookup =
        GameNet::FindPlayerRowByKey(20) == &second && GameNet::FindPlayerRowByKey(30) == nullptr;
    const bool lapsReached =
        GameNet::AreAllPlayersAtLapTarget() == 1 && g_GameNetAllPlayersLapTargetCheckStarted == 1;
    g_GameNetAllPlayersLapTargetCheckStarted = 0;
    second.lapCount = 2;
    const bool lapsBlocked =
        GameNet::AreAllPlayersAtLapTarget() == 0 && g_GameNetAllPlayersLapTargetCheckStarted == 1;
    g_GameNetAllPlayersLapTargetCheckStarted = 0;
    g_GameNetPlayerRowHead = nullptr;
    const bool emptyListReached =
        GameNet::AreAllPlayersAtLapTarget() == 1 && g_GameNetAllPlayersLapTargetCheckStarted == 1;

    GameNet::SetStatusBitsFromFlags(3);
    const bool bothSet = g_GameNetStatus_AllowMaps == 1 && g_GameNetStatus_NameTags == 1 &&
                         GameNet::GetStatusBitAllowMaps() == 1 &&
                         GameNet::GetStatusBitNameTags() == 1;

    GameNet::SetStatusBitsFromFlags(0);
    const bool bothClear = g_GameNetStatus_AllowMaps == 0 && g_GameNetStatus_NameTags == 0 &&
                           GameNet::GetStatusBitAllowMaps() == 0 &&
                           GameNet::GetStatusBitNameTags() == 0;

    g_GameNetPlayerRowHead = nullptr;
    return rowLookup && lapsReached && lapsBlocked && emptyListReached && bothSet && bothClear ? 0
                                                                                               : 1;
}

extern "C" int gamenet_update_remote_player_hud_widget_screen_pos_smoke(void) {
    const int oldNameTags = g_GameNetStatus_NameTags;
    int *const oldReplicateOption = ZOPT_REPLICATE;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeDiScene = g_Player_RuntimeDiScene;
    int *const oldMatrixIdentitySlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const oldMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;

    int replicateMode = 0;
    ZOPT_REPLICATE = &replicateMode;

    int matrixIdentityFlags[2] = {};
    float *matrixSlots[2] = {};
    zMat4x3 baseMatrix = {};
    zMath::g_currentMatrixIdentityFlagSlot = &matrixIdentityFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    matrixSlots[0] = reinterpret_cast<float *>(&baseMatrix);
    zMath::g_zMath_CameraScratchB = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                     0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    g_zMath_ProjScaleX = 100.0f;
    g_zMath_ProjScaleY = -50.0f;
    g_zMath_ProjOffsetX = 320.0f;
    g_zMath_ProjOffsetY = 240.0f;
    gClipRect_Primary.zMin = 1.0f;
    gClipRect_Primary.xMaxAlt = 640.0f;
    g_zVideo_ProjectClipLeft = 0.0f;
    g_zVideo_ProjectClipTop = 0.0f;
    g_zVideo_ProjectClipRight = 640.0f;
    g_zVideo_ProjectClipBottom = 480.0f;

    HudUiPanel_FTable panelTable = {};
    panelTable.slots[0x0c / 4] = MethodAddress(&TestRemoteHudPanelOps::SetPos);
    panelTable.slots[0x60 / 4] = MethodAddress(&TestRemoteHudPanelOps::SetVisible);

    zClass_NodePartial localRoot = {};
    zClass_NodePartial remoteRoot = {};
    zUtil_PlayerStateStorage localPlayer = {};
    localPlayer.rootNode = &localRoot;
    zUtil_SaveGameState localSave = {};
    localSave.playerState = &localPlayer;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&localSave);
    g_Player_RuntimeDiScene = nullptr;

    zUtil_PlayerStateStorage remotePlayer = {};
    remotePlayer.rootNode = &remoteRoot;
    remotePlayer.worldPos = {1.0f, 2.0f, 10.0f};
    zUtil_SaveGameState remoteSave = {};
    remoteSave.playerState = &remotePlayer;
    GameNetPlayerRow row = {};
    row.hudWidget.vtbl = &panelTable;
    FieldAt<int>(&row.hudWidget, 0x260) = 14;
    FieldAt<int>(&row.hudWidget, 0x270) = 0;
    FieldAt<int>(&row.hudWidget, 0x274) = 0;
    remoteSave.netPlayerRow = &row;

    g_remoteHudSetVisibleCount = 0;
    g_remoteHudSetPosCount = 0;
    g_GameNetStatus_NameTags = 0;
    const bool disabledOk = GameNet::UpdateRemotePlayerHudWidgetScreenPos(&remoteSave) == 0 &&
                            g_remoteHudSetVisibleCount == 0 && g_remoteHudSetPosCount == 0;

    g_GameNetStatus_NameTags = 1;
    const int visibleResult = GameNet::UpdateRemotePlayerHudWidgetScreenPos(&remoteSave);
    const bool visibleOk =
        visibleResult == 1 && g_remoteHudSetPosCount == 1 && g_remoteHudSetPosThis == &row.hudWidget &&
        g_remoteHudLastX == 330 && g_remoteHudLastY == 205 &&
        g_remoteHudSetVisibleCount == 1 && g_remoteHudLastVisible == 1;

    replicateMode = 1;
    g_remoteHudSetVisibleCount = 0;
    g_remoteHudSetPosCount = 0;
    const int replicateResult = GameNet::UpdateRemotePlayerHudWidgetScreenPos(&remoteSave);
    const bool replicateOk = replicateResult == 1 && g_remoteHudLastX == 660 &&
                             g_remoteHudLastY == 420 && g_remoteHudLastVisible == 1;

    replicateMode = 0;
    remotePlayer.worldPos = {1.0f, 38.0f, 10.0f};
    g_remoteHudSetVisibleCount = 0;
    g_remoteHudSetPosCount = 0;
    const bool marginHideOk = GameNet::UpdateRemotePlayerHudWidgetScreenPos(&remoteSave) == 0 &&
                              g_remoteHudSetVisibleCount == 1 &&
                              g_remoteHudLastVisible == 0 && g_remoteHudSetPosCount == 0;

    remotePlayer.worldPos = {-100.0f, 2.0f, 10.0f};
    g_remoteHudSetVisibleCount = 0;
    g_remoteHudSetPosCount = 0;
    const bool clippedHideOk = GameNet::UpdateRemotePlayerHudWidgetScreenPos(&remoteSave) == 0 &&
                               g_remoteHudSetVisibleCount == 1 &&
                               g_remoteHudLastVisible == 0 && g_remoteHudSetPosCount == 0;

    g_GameNetStatus_NameTags = oldNameTags;
    ZOPT_REPLICATE = oldReplicateOption;
    g_GameStateOrMapTable = oldGameState;
    g_Player_RuntimeDiScene = oldRuntimeDiScene;
    zMath::g_currentMatrixIdentityFlagSlot = oldMatrixIdentitySlot;
    zMath::g_currentMatrixPtrSlot = oldMatrixPtrSlot;

    return disabledOk && visibleOk && replicateOk && marginHideOk && clippedHideOk ? 0 : 1;
}

extern "C" int gamenet_get_local_player_color_index_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;

    g_GameStateOrMapTable = nullptr;
    const bool nullStateOk = GameNet::GetLocalPlayerColorIndexOrZero() == 0;

    zUtil_SaveGameState saveState{};
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)(&saveState);
    const bool nullRowOk = GameNet::GetLocalPlayerColorIndexOrZero() == 0;

    GameNetPlayerRow row{};
    row.playerColorIndex = 6;
    saveState.netPlayerRow = &row;
    const bool colorOk = GameNet::GetLocalPlayerColorIndexOrZero() == 6;

    g_GameStateOrMapTable = oldGameState;
    return nullStateOk && nullRowOk && colorOk ? 0 : 1;
}

extern "C" int gamenet_get_nearest_other_player_distance_to_spawn_point_smoke(void) {
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const unsigned int oldCount = g_GameNetPlayerRowCount;
    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;

    GameNetSpawnPoint spawnPoint = {};
    spawnPoint.position.x = 0.0f;
    spawnPoint.position.y = 0.0f;
    spawnPoint.position.z = 0.0f;

    zUtil_PlayerStateStorage localPlayerState = {};
    zUtil_PlayerStateStorage farPlayerState = {};
    zUtil_PlayerStateStorage nearPlayerState = {};
    localPlayerState.worldPos.x = 1.0f;
    farPlayerState.worldPos.x = 5.0f;
    nearPlayerState.worldPos.x = 2.0f;

    GameNetPlayerSaveState localSave = {};
    GameNetPlayerSaveState farSave = {};
    GameNetPlayerSaveState nearSave = {};
    localSave.playerState = &localPlayerState;
    farSave.playerState = &farPlayerState;
    nearSave.playerState = &nearPlayerState;

    GameNetPlayerRow localRow = {};
    GameNetPlayerRow farRow = {};
    GameNetPlayerRow nearRow = {};
    localRow.saveState = &localSave;
    localRow.next = &farRow;
    farRow.saveState = &farSave;
    farRow.next = &nearRow;
    nearRow.saveState = &nearSave;

    g_GameNetPlayerRowHead = &localRow;
    g_GameNetPlayerRowTail = &nearRow;
    g_GameNetPlayerRowCount = 3;
    g_LocalPlayerSaveState = reinterpret_cast<zUtil_SaveGameState *>(&localSave);

    GameNetPlayerSaveState *nearest = &localSave;
    const float nearestDistance =
        GameNet::GetNearestOtherPlayerDistanceToSpawnPoint(&spawnPoint, &nearest);
    const bool nearestOk = nearestDistance == 4.0f && nearest == &nearSave;

    g_GameNetPlayerRowHead = nullptr;
    nearest = &localSave;
    const float emptyDistance =
        GameNet::GetNearestOtherPlayerDistanceToSpawnPoint(&spawnPoint, &nearest);
    const bool emptyOk = emptyDistance > 9.0e22f && nearest == &localSave;

    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_LocalPlayerSaveState = oldLocalSaveState;

    return nearestOk && emptyOk ? 0 : 1;
}

extern "C" int gamenet_respawn_player_color_indexed_spawn_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    GameNetSpawnPoint *const oldSpawnHead = g_GameNetSpawnPointHead;
    GameNetSpawnPoint *const oldSpawnTail = g_GameNetSpawnPointTail;
    const std::uint32_t oldSpawnCount = g_GameNetSpawnPointCount;
    const std::int32_t oldAllowMaps = g_GameNetStatus_AllowMaps;
    const std::int32_t oldMissionId = g_HudSensorTracker.missionId;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    HWND const oldWindow = g_zInput_hWnd;
    const int oldCenterX = g_zInput_MouseClientCenterX;
    const int oldCenterY = g_zInput_MouseClientCenterY;
    const zInput::MouseStateSnapshot oldMouseState = g_zInput_MouseStateSnapshot;
    POINT originalCursor = {};
    GetCursorPos(&originalCursor);

    HWND const hwnd = CreateWindowExA(0, "STATIC", "recoil", WS_POPUP, 20, 30, 160, 120,
                                      nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    if (hwnd == nullptr) {
        return 1;
    }

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    GameNetPlayerRow localRow = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    saveState.netPlayerRow = &localRow;
    modalState.masterModalData = &modalData;
    modalData.masterType = 3;
    localRow.playerColorIndex = 2;

    GameNetSpawnPoint firstSpawn = {};
    GameNetSpawnPoint secondSpawn = {};
    firstSpawn.position = {1.0f, 2.0f, 3.0f};
    firstSpawn.yawDegrees = 10.0f;
    firstSpawn.next = &secondSpawn;
    secondSpawn.position = {4.0f, 5.0f, 6.0f};
    secondSpawn.yawDegrees = 90.0f;
    g_GameNetSpawnPointHead = &firstSpawn;
    g_GameNetSpawnPointTail = &secondSpawn;
    g_GameNetSpawnPointCount = 2;
    g_GameNetStatus_AllowMaps = 0;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    g_HudSensorTracker.missionId = 11;

    playerState.throttleInput = 1.0f;
    playerState.steeringInput = 2.0f;
    playerState.subVerticalInput = 3.0f;
    playerState.throttleInputCopy = 4.0f;
    playerState.steeringInputCopy = 5.0f;
    playerState.subVerticalInputCopy = 6.0f;
    playerState.localVel = {7.0f, 8.0f, 9.0f};
    playerState.projectileSpawnVel = {10.0f, 11.0f, 12.0f};
    playerState.yawRotatedLocalVel = {13.0f, 14.0f, 15.0f};
    playerState.angVelPitch = 16.0f;
    playerState.angVelYaw = 17.0f;
    playerState.angVelRoll = 18.0f;
    playerState.thirdPersonYawOffset = 19.0f;
    playerState.cameraElevationOffset = 20.0f;
    playerState.amphibUnlocked = 0;
    playerState.hoverUnlocked = 1;
    playerState.subUnlocked = 1;

    g_zInput_hWnd = hwnd;
    g_zInput_MouseClientCenterX = 32;
    g_zInput_MouseClientCenterY = 24;
    g_zInput_MouseStateSnapshot.cursorClientX = 4;
    g_zInput_MouseStateSnapshot.cursorClientY = 5;
    g_zInput_MouseStateSnapshot.cursorNormX = 0.25f;
    g_zInput_MouseStateSnapshot.cursorNormY = -0.5f;

    GameNet::RespawnPlayerAndDropWeaponPickupIfAllowed(&saveState, 1);

    const bool spawnOk = Vec3Equals(playerState.worldPos, secondSpawn.position) &&
                         FloatNear(playerState.restartYawRad, 1.5707964f) &&
                         FloatNear(playerState.previousTransform.posX, 4.0f) &&
                         FloatNear(playerState.previousTransform.posY, 5.0f) &&
                         FloatNear(playerState.previousTransform.posZ, 6.0f);
    const bool resetOk =
        playerState.thirdPersonYawOffset == 0.0f && playerState.cameraElevationOffset == 0.0f &&
        Vec3Equals(playerState.localVel, {0.0f, 0.0f, 0.0f}) &&
        Vec3Equals(playerState.projectileSpawnVel, {0.0f, 0.0f, 0.0f}) &&
        Vec3Equals(playerState.yawRotatedLocalVel, {0.0f, 0.0f, 0.0f}) &&
        playerState.angVelPitch == 0.0f && playerState.angVelYaw == 0.0f &&
        playerState.angVelRoll == 0.0f && playerState.throttleInput == 0.0f &&
        playerState.steeringInput == 0.0f && playerState.subVerticalInput == 0.0f &&
        playerState.throttleInputCopy == 0.0f && playerState.steeringInputCopy == 0.0f &&
        playerState.subVerticalInputCopy == 0.0f &&
        g_zInput_MouseStateSnapshot.cursorClientX == 32 &&
        g_zInput_MouseStateSnapshot.cursorClientY == 24 &&
        g_zInput_MouseStateSnapshot.cursorNormX == 0.0f &&
        g_zInput_MouseStateSnapshot.cursorNormY == 0.0f;
    const bool unlockOk =
        playerState.amphibUnlocked == 1 && playerState.hoverUnlocked == 0 &&
        playerState.subUnlocked == 0;

    DestroyWindow(hwnd);
    SetCursorPos(originalCursor.x, originalCursor.y);
    g_GameStateOrMapTable = oldGameState;
    g_GameNetSpawnPointHead = oldSpawnHead;
    g_GameNetSpawnPointTail = oldSpawnTail;
    g_GameNetSpawnPointCount = oldSpawnCount;
    g_GameNetStatus_AllowMaps = oldAllowMaps;
    g_HudSensorTracker.missionId = oldMissionId;
    g_VariantTag_Current = oldVariantTagCurrent;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_zInput_hWnd = oldWindow;
    g_zInput_MouseClientCenterX = oldCenterX;
    g_zInput_MouseClientCenterY = oldCenterY;
    g_zInput_MouseStateSnapshot = oldMouseState;

    return spawnOk && resetOk && unlockOk ? 0 : 2;
}

extern "C" int gamenet_reset_hud_timer_panel_net_state_smoke(void) {
    HudUiTimerPanel timer{};
    HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&timer);
    panel->ConstructorDefault("", 0, 0);
    g_HudUiMgrTimerPanel = &timer;

    g_HudTimerPanelNetState.timerSeconds = 1.0f;
    g_HudTimerPanelNetState.timeWarningThresholdSec = 2.0f;
    g_HudTimerPanelNetState.timerDirectionNeg = 0;
    g_HudTimerPanelNetState.startGateTriggered = 1;
    g_HudTimerPanelNetState.raceFinishCountdownTriggered = 1;
    g_HudTimerPanelNetState.startCountdownTriggered = 1;
    g_HudTimerPanelNetState.tenSecondWarningsEnabled = 1;
    for (std::uint32_t &flag : g_HudTimerPanelNetState.tailFlags) {
        flag = 0xffffffff;
    }
    g_GameNetOneLapLeftMessageShown = 1;
    g_GameNetAllPlayersLapTargetCheckStarted = 1;

    GameNet::ResetHudTimerPanelNetStateLongCountdown();

    bool tailsCleared = true;
    for (std::uint32_t flag : g_HudTimerPanelNetState.tailFlags) {
        tailsCleared = tailsCleared && flag == 0;
    }

    const bool ok = g_HudTimerPanelNetState.timerSeconds == 36000.0f &&
                    g_HudTimerPanelNetState.timeWarningThresholdSec == 120.0f &&
                    g_HudTimerPanelNetState.timerDirectionNeg == 1 &&
                    g_HudTimerPanelNetState.startGateTriggered == 0 &&
                    g_HudTimerPanelNetState.raceFinishCountdownTriggered == 0 &&
                    g_HudTimerPanelNetState.startCountdownTriggered == 0 &&
                    g_HudTimerPanelNetState.tenSecondWarningsEnabled == 0 &&
                    g_GameNetOneLapLeftMessageShown == 0 &&
                    g_GameNetAllPlayersLapTargetCheckStarted == 0 && tailsCleared &&
                    FieldAt<float>(&timer, 0x2a4) == 36000.0f &&
                    FieldAt<std::int32_t>(&timer, 0x2ac) == -1 &&
                    std::strcmp(&FieldAt<char>(&timer, 0x34), "10:00:00") == 0;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    g_HudUiMgrTimerPanel = nullptr;
    return ok ? 0 : 1;
}

extern "C" int gamenet_wait_for_local_player_color_index_smoke(void) {
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldSessionRuntime = g_zNetwork_SessionRuntimeInitialized;

    g_zNetwork_LocalPlayerRecord = nullptr;
    const bool noWaitOk = GameNet::WaitForLocalPlayerColorIndex(0) == 0;

    zNetwork_PlayerRecord localPlayer{};
    localPlayer.colorIndex = 6;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_SessionRuntimeInitialized = 0;
    const bool colorOk = GameNet::WaitForLocalPlayerColorIndex(1) == 6;

    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_SessionRuntimeInitialized = oldSessionRuntime;
    return noWaitOk && colorOk ? 0 : 1;
}

extern "C" int net_init_from_zrd_smoke(void) {
    char oldDir[MAX_PATH] = {};
    if (GetCurrentDirectoryA(sizeof(oldDir), oldDir) == 0) {
        return 1;
    }

    char tempRoot[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempRoot), tempRoot) == 0) {
        return 2;
    }

    char tempDir[MAX_PATH] = {};
    std::sprintf(tempDir, "%srecoil_gamenet_init", tempRoot);
    CreateDirectoryA(tempDir, nullptr);
    if (SetCurrentDirectoryA(tempDir) == 0) {
        return 3;
    }

    zReader::Node spawn0Values[5] = {};
    MakeGameNetReaderFloatNode(spawn0Values[1], 1.0f);
    MakeGameNetReaderFloatNode(spawn0Values[2], 2.0f);
    MakeGameNetReaderFloatNode(spawn0Values[3], 3.0f);
    MakeGameNetReaderFloatNode(spawn0Values[4], 10.0f);
    zReader::Node spawn0{};
    MakeGameNetReaderArrayNode(spawn0, spawn0Values, 5);

    zReader::Node spawn1Values[5] = {};
    MakeGameNetReaderFloatNode(spawn1Values[1], 4.0f);
    MakeGameNetReaderFloatNode(spawn1Values[2], 5.0f);
    MakeGameNetReaderFloatNode(spawn1Values[3], 6.0f);
    MakeGameNetReaderFloatNode(spawn1Values[4], 90.0f);
    zReader::Node spawn1{};
    MakeGameNetReaderArrayNode(spawn1, spawn1Values, 5);

    zReader::Node spawnListValues[3] = {};
    spawnListValues[1] = spawn0;
    spawnListValues[2] = spawn1;
    zReader::Node spawnList{};
    MakeGameNetReaderArrayNode(spawnList, spawnListValues, 3);

    zReader::Node rootValues[2] = {};
    rootValues[1] = spawnList;
    zReader::Node root{};
    MakeGameNetReaderArrayNode(root, rootValues, 2);

    zArchiveList *const oldMountedList = g_zArchive_MountedList;
    zIndexArchive archive{};
    zZarFileRecord records[1] = {};
    zArchiveListNode archiveNode{};
    zArchiveList archiveList{};
    const GameNetZrdArchiveEntry entries[] = {{"net.zrd", &root}};
    if (!MountGameNetZrdArchive("gamenet_init.zar", entries, 1, archive, records, archiveNode,
                                archiveList)) {
        SetCurrentDirectoryA(oldDir);
        return 4;
    }

    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    zUtil_SaveGameState *const oldSaveHead = g_PlayerSaveStateListHead;
    GameNetPlayerRow *const oldRowHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldRowTail = g_GameNetPlayerRowTail;
    const unsigned int oldRowList = g_GameNetPlayerRowList;
    const unsigned int oldRowCount = g_GameNetPlayerRowCount;
    GameNetSpawnPoint *const oldSpawnHead = g_GameNetSpawnPointHead;
    GameNetSpawnPoint *const oldSpawnTail = g_GameNetSpawnPointTail;
    const unsigned int oldSpawnList = g_GameNetSpawnPointList;
    const unsigned int oldSpawnCount = g_GameNetSpawnPointCount;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHost = g_zNetwork_IsHostFlag;
    zNetworkPlayerRecordList *const oldPlayerRecordList = g_zNetwork_PlayerRecordList;
    zNetworkDPlaySessionDescCache *const oldSession = g_zNetwork_CurrentSessionDescCache;
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    HudUiTimerPanel *const oldTimerPanel = g_HudUiMgrTimerPanel;
    const HudTimerPanelNetState oldTimerState = g_HudTimerPanelNetState;
    const int oldHostTimerInitFlag = g_GameNetHostHudTimerInitFlag;
    const int oldInitialSyncGate = g_GameNetPkt06InitialSyncGate;
    const float oldNextSendTime = g_GameNetPkt06NextSendTimeSec;
    const int oldRaceMode = g_HudSensorTracker.raceCheckpointMode;
    const int oldRuntimeTimerRaw = g_HudSensorTracker.runtimeTimerSecRaw;
    const int oldMissionId = g_HudSensorTracker.missionId;
    HWND const oldWindow = g_zInput_hWnd;
    const int oldCenterX = g_zInput_MouseClientCenterX;
    const int oldCenterY = g_zInput_MouseClientCenterY;
    const zInput::MouseStateSnapshot oldMouseState = g_zInput_MouseStateSnapshot;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    POINT originalCursor = {};
    GetCursorPos(&originalCursor);

    HWND const hwnd = CreateWindowExA(0, "STATIC", "recoil", WS_POPUP, 20, 30, 160, 120,
                                      nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    if (hwnd == nullptr) {
        g_zArchive_MountedList = oldMountedList;
        CloseHandle(static_cast<HANDLE>(archive.hFile));
        std::remove("gamenet_init.zar");
        SetCurrentDirectoryA(oldDir);
        return 5;
    }

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    zClass_Object3DDataPartial objectData = {};
    zClass_NodePartial modalNode = {};
    modalNode.classId = 5;
    modalNode.classData = &objectData;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalState.modalNode = &modalNode;
    modalData.masterType = 3;

    zNetwork_PlayerRecord localRecord{};
    localRecord.playerKey = 0x1234;
    localRecord.colorIndex = 2;
    std::strcpy(localRecord.playerName, "Local");
    zNetworkPlayerRecordListNode sentinel{};
    zNetworkPlayerRecordListNode localNode{};
    sentinel.next = &localNode;
    sentinel.prev = &localNode;
    localNode.next = &sentinel;
    localNode.prev = &sentinel;
    localNode.playerRecord = &localRecord;
    zNetworkPlayerRecordList playerList{};
    playerList.sentinelNode = &sentinel;
    playerList.count = 1;
    zNetworkDPlaySessionDescCache session{};
    session.desc.maxPlayers = 8;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    HudUiTimerPanel timer{};
    HudUiPanel *const timerPanel = reinterpret_cast<HudUiPanel *>(&timer);
    timerPanel->ConstructorDefault("", 0, 0);

    g_PlayerSaveStateListHead = nullptr;
    g_GameNetPlayerRowList = 0;
    g_GameNetPlayerRowHead = nullptr;
    g_GameNetPlayerRowTail = nullptr;
    g_GameNetPlayerRowCount = 0;
    g_GameNetSpawnPointList = 0;
    g_GameNetSpawnPointHead = nullptr;
    g_GameNetSpawnPointTail = nullptr;
    g_GameNetSpawnPointCount = 0;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    g_zNetwork_LocalPlayerKey = localRecord.playerKey;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_PlayerRecordList = &playerList;
    g_zNetwork_CurrentSessionDescCache = &session;
    g_HudUiMgrStatsList = &statsList;
    g_HudUiMgrTimerPanel = &timer;
    g_HudSensorTracker.raceCheckpointMode = 0;
    g_HudSensorTracker.runtimeTimerSecRaw = 0x41f00000;
    g_HudSensorTracker.missionId = 11;
    g_GameNetHostHudTimerInitFlag = 99;
    g_GameNetPkt06InitialSyncGate = 0;
    g_GameNetPkt06NextSendTimeSec = 7.0f;
    g_HudTimerPanelNetState.timeWarningShown = 1;
    g_HudTimerPanelNetState.oneMinuteWarningShown = 1;
    g_zInput_hWnd = hwnd;
    g_zInput_MouseClientCenterX = 32;
    g_zInput_MouseClientCenterY = 24;

    Net::InitFromZrd();

    GameNetPlayerRow *const row = saveState.netPlayerRow;
    GameNetSpawnPoint *const firstSpawn = g_GameNetSpawnPointHead;
    GameNetSpawnPoint *const secondSpawn = firstSpawn != nullptr ? firstSpawn->next : nullptr;
    int spawnFailure = 0;
    if (firstSpawn == nullptr) {
        spawnFailure = 20;
    } else if (secondSpawn == nullptr) {
        spawnFailure = 21;
    } else if (secondSpawn->next != nullptr) {
        spawnFailure = 22;
    } else if (!Vec3Equals(firstSpawn->position, {1.0f, 2.0f, 3.0f})) {
        spawnFailure = 23;
    } else if (!FloatNear(firstSpawn->yawDegrees, 10.0f)) {
        spawnFailure = 24;
    } else if (!Vec3Equals(secondSpawn->position, {4.0f, 5.0f, 6.0f})) {
        spawnFailure = 25;
    } else if (!FloatNear(secondSpawn->yawDegrees, 90.0f)) {
        spawnFailure = 26;
    }
    const bool spawnOk = spawnFailure == 0;
    const bool rowOk = row != nullptr && row->playerKey == 0x1234 &&
                       row->playerColorIndex == 2 && std::strcmp(row->displayName, "Local") == 0 &&
                       row->playerColorPackedRgb == g_GameNetPlayerRowStyleColors_00RRGGBB[2];
    const bool hostTimerOk =
        g_GameNetHostHudTimerInitFlag == 0 && g_HudTimerPanelNetState.timerDirectionNeg == 1 &&
        FloatNear(g_HudTimerPanelNetState.statusBitsResendDeadline, 30.0f) &&
        g_HudTimerPanelNetState.timeWarningShown == 0 &&
        g_HudTimerPanelNetState.oneMinuteWarningShown == 0 &&
        FloatNear(FieldAt<float>(&timer, 0x2a4), 30.0f) &&
        FieldAt<std::int32_t>(&timer, 0x2ac) == -1;
    const bool respawnOk =
        secondSpawn != nullptr && Vec3Equals(playerState.worldPos, secondSpawn->position) &&
        FloatNear(playerState.restartYawRad, 1.5707964f) &&
        saveState.netPlayerRow == row && g_GameNetPkt06InitialSyncGate == 1 &&
        g_GameNetPkt06NextSendTimeSec == 0.0f;

    GameNetSpawnPoint *spawn = g_GameNetSpawnPointHead;
    while (spawn != nullptr) {
        GameNetSpawnPoint *const next = spawn->next;
        ::operator delete(spawn);
        spawn = next;
    }
    ::operator delete(row);

    DeleteObject(timerPanel->hFont);
    timerPanel->hFont = nullptr;
    DestroyWindow(hwnd);
    SetCursorPos(originalCursor.x, originalCursor.y);
    g_GameStateOrMapTable = oldGameState;
    g_PlayerSaveStateListHead = oldSaveHead;
    g_GameNetPlayerRowList = oldRowList;
    g_GameNetPlayerRowHead = oldRowHead;
    g_GameNetPlayerRowTail = oldRowTail;
    g_GameNetPlayerRowCount = oldRowCount;
    g_GameNetSpawnPointList = oldSpawnList;
    g_GameNetSpawnPointHead = oldSpawnHead;
    g_GameNetSpawnPointTail = oldSpawnTail;
    g_GameNetSpawnPointCount = oldSpawnCount;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_PlayerRecordList = oldPlayerRecordList;
    g_zNetwork_CurrentSessionDescCache = oldSession;
    g_HudUiMgrStatsList = oldStatsList;
    g_HudUiMgrTimerPanel = oldTimerPanel;
    g_HudTimerPanelNetState = oldTimerState;
    g_GameNetHostHudTimerInitFlag = oldHostTimerInitFlag;
    g_GameNetPkt06InitialSyncGate = oldInitialSyncGate;
    g_GameNetPkt06NextSendTimeSec = oldNextSendTime;
    g_HudSensorTracker.raceCheckpointMode = oldRaceMode;
    g_HudSensorTracker.runtimeTimerSecRaw = oldRuntimeTimerRaw;
    g_HudSensorTracker.missionId = oldMissionId;
    g_zInput_hWnd = oldWindow;
    g_zInput_MouseClientCenterX = oldCenterX;
    g_zInput_MouseClientCenterY = oldCenterY;
    g_zInput_MouseStateSnapshot = oldMouseState;
    g_VariantTag_Current = oldVariantTagCurrent;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_zArchive_MountedList = oldMountedList;
    CloseHandle(static_cast<HANDLE>(archive.hFile));
    std::remove("gamenet_init.zar");
    SetCurrentDirectoryA(oldDir);
    RemoveDirectoryA(tempDir);

    if (!spawnOk) {
        return spawnFailure;
    }
    if (!rowOk) {
        return 11;
    }
    if (!hostTimerOk) {
        return 12;
    }
    if (!respawnOk) {
        return 13;
    }
    return 0;
}

extern "C" int gamenet_host_update_session_status_fields_smoke(void) {
    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetworkDPlaySessionDescCache session{};
    char sessionName[0x5c] = "mission";
    session.desc.sessionName = sessionName;
    session.desc.maxPlayers = 8;
    session.desc.customEventCode = 1;
    session.desc.customStatusFlags = 2;
    session.desc.customValueOrTime = 3;
    session.desc.customAuxParam = 4;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_CurrentSessionDescCache = &session;
    g_zNetwork_IsHostFlag = 0;
    g_setSessionDescCalls = 0;
    g_setSessionDescResult = 0;

    if (GameNet::HostUpdateSessionDescStatusFields(10, 13, 12, 11) != 0 ||
        g_setSessionDescCalls != 0 || session.desc.customEventCode != 1) {
        return 1;
    }

    g_zNetwork_IsHostFlag = 1;
    if (GameNet::HostUpdateSessionDescStatusFields(10, 13, 12, 11) != 1 ||
        g_setSessionDescCalls != 1 || session.desc.customEventCode != 10 ||
        session.desc.customStatusFlags != 11 || session.desc.customValueOrTime != 12 ||
        session.desc.customAuxParam != 13 || session.desc.maxPlayers != 8 ||
        std::strcmp(sessionName, "mission") != 0) {
        return 2;
    }

    g_setSessionDescResult = static_cast<std::int32_t>(0x88770014);
    if (GameNet::HostUpdateSessionDescStatusFields(20, 23, 22, 21) != 0 ||
        session.desc.customEventCode != 20) {
        return 3;
    }

    g_zNetwork_IsHostFlag = 0;
    g_zNetwork_CurrentSessionDescCache = nullptr;
    g_zNetwork_pDirectPlay4 = nullptr;
    return 0;
}

extern "C" int gamenet_timer_status_packet_smoke(void) {
    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x1234;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x5678;
    g_zNetwork_IsHostFlag = 1;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;

    HudUiTimerPanel timer{};
    HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&timer);
    panel->ConstructorDefault("", 0, 0);
    g_HudUiMgrTimerPanel = &timer;
    HudUiTimerPanel::SetSeconds(42.0f, -1.0f);

    g_Time_AccumulatedTimeSec = 12.0f;
    g_HudTimerPanelNetState = {};
    g_HudTimerPanelNetState.timerDirectionNeg = 1;
    GameNet::SendPkt0C_HudTimerStatusBits(&g_HudTimerPanelNetState);

    const bool sent = g_sendCalls == 1 && g_sendFlags == 1 &&
                      g_sendPacket == &g_NetPkt0C_HudTimerStatusBitsBuf &&
                      g_sendPacketSize == sizeof(NetPkt0C_HudTimerStatusBits) &&
                      g_NetPkt0C_HudTimerStatusBitsBuf.header.packetType == 0x0c &&
                      g_NetPkt0C_HudTimerStatusBitsBuf.header.packetSizeBytes ==
                          sizeof(NetPkt0C_HudTimerStatusBits) &&
                      g_NetPkt0C_HudTimerStatusBitsBuf.header.payloadDword0 == 0x5678 &&
                      g_NetPkt0C_HudTimerStatusBitsBuf.timerSeconds == 42.0f &&
                      g_NetPkt0C_HudTimerStatusBitsBuf.statusBitsPackedHiWord == 1;

    const bool applied = g_HudTimerPanelNetState.timerSeconds == 42.0f &&
                         g_HudTimerPanelNetState.timerDirectionNeg == 1 &&
                         g_HudTimerPanelNetState.statusBitsResendDeadline == 42.0f &&
                         FieldAt<float>(&timer, 0x2a4) == 42.0f &&
                         FieldAt<std::int32_t>(&timer, 0x2ac) == -1;

    NetPkt0C_HudTimerStatusBits packet = {};
    packet.timerSeconds = 7.0f;
    packet.statusBitsPackedHiWord = 0;
    GameNet::HandlePkt0C_HudTimerStatusBits(0, &packet);
    const bool directApply = g_HudTimerPanelNetState.timerSeconds == 7.0f &&
                             g_HudTimerPanelNetState.timerDirectionNeg == 0 &&
                             FieldAt<float>(&timer, 0x2a4) == 7.0f &&
                             FieldAt<std::int32_t>(&timer, 0x2ac) == 1;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    g_HudUiMgrTimerPanel = nullptr;
    g_zNetwork_pDirectPlay4 = nullptr;
    g_zNetwork_LocalPlayerRecord = nullptr;
    g_zNetwork_LocalPlayerKey = 0;
    g_zNetwork_IsHostFlag = 0;
    g_Time_AccumulatedTimeSec = 0.0f;
    return sent && applied && directApply ? 0 : 1;
}

extern "C" int gamenet_timer_panel_state_packet_smoke(void) {
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldIsHost = g_zNetwork_IsHostFlag;
    const HudTimerPanelNetState oldTimerState = g_HudTimerPanelNetState;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x4321;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x8765;
    g_zNetwork_IsHostFlag = 1;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    HudUiTimerPanel timer{};
    HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&timer);
    panel->ConstructorDefault("", 0, 0);
    g_HudUiMgrTimerPanel = &timer;
    HudUiTimerPanel::SetSeconds(88.0f, -1.0f);

    HudTimerPanelNetState state{};
    state.timerDirectionNeg = 1;
    state.startGateTriggered = 1;
    state.raceFinishCountdownTriggered = 1;
    GameNet::SendPkt0D_HudTimerPanelState(&state);

    const NetPkt0D_HudTimerPanelState *const sentPacket =
        reinterpret_cast<const NetPkt0D_HudTimerPanelState *>(g_sendPacketBytes);
    const bool sent = g_sendCalls == 1 && g_sendFlags == 1 &&
                      g_sendPacketSize == sizeof(NetPkt0D_HudTimerPanelState) &&
                      sentPacket->header.packetType == 0x0d &&
                      sentPacket->header.packetSizeBytes ==
                          sizeof(NetPkt0D_HudTimerPanelState) &&
                      sentPacket->header.payloadDword0 == 0x8765 &&
                      sentPacket->seconds == 88.0f &&
                      sentPacket->hudTimerFlagsPacked == 0x19;

    const bool localApply = g_HudTimerPanelNetState.timerSeconds == 88.0f &&
                            g_HudTimerPanelNetState.timerDirectionNeg == 1 &&
                            g_HudTimerPanelNetState.startGateTriggered == 1 &&
                            g_HudTimerPanelNetState.raceFinishCountdownTriggered == 1 &&
                            FieldAt<float>(&timer, 0x2a4) == 88.0f &&
                            FieldAt<std::int32_t>(&timer, 0x2ac) == -1;

    NetPkt0D_HudTimerPanelState packet{};
    packet.seconds = 12.0f;
    packet.hudTimerFlagsPacked = 0;
    const bool handled = GameNet::HandlePkt0D_HudTimerPanelState(0, &packet) == 1 &&
                         g_HudTimerPanelNetState.timerSeconds == 12.0f &&
                         g_HudTimerPanelNetState.timerDirectionNeg == 0 &&
                         FieldAt<float>(&timer, 0x2a4) == 12.0f &&
                         FieldAt<std::int32_t>(&timer, 0x2ac) == 1;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    g_HudUiMgrTimerPanel = nullptr;
    g_HudTimerPanelNetState = oldTimerState;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    return sent && localApply && handled ? 0 : 1;
}

extern "C" int gamenet_tick_local_player_pkt06_and_timer_smoke(void) {
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldIsHost = g_zNetwork_IsHostFlag;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    const std::int32_t oldRaceMode = g_HudSensorTracker.raceCheckpointMode;
    const HudTimerPanelNetState oldTimerState = g_HudTimerPanelNetState;
    const int oldInitialGate = g_GameNetPkt06InitialSyncGate;
    const int oldLatch16 = g_GameNetPkt06InputBit16Latch;
    const int oldLatch17 = g_GameNetPkt06InputBit17Latch;
    const float oldNextSend = g_GameNetPkt06NextSendTimeSec;
    const int oldTenSecondArmed = g_GameNetHudTimerTenSecondWarningArmed;
    const int oldPendingReminderArmed = g_GameNetHudTimerPendingSaveReminderArmed;
    const NetPkt06_PlayerStateSnapshot oldPkt06 = g_NetPkt06_PlayerStateSnapshotBuf;

    int networkEnabled = 1;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x11223344;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x55667788;
    g_zNetwork_IsHostFlag = 1;

    HudUiTimerPanel timer{};
    HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&timer);
    panel->ConstructorDefault("", 0, 0);
    g_HudUiMgrTimerPanel = &timer;
    HudUiTimerPanel::SetSeconds(120.0f, -1.0f);

    zUtil_SaveGameState saveState{};
    zUtil_PlayerStateStorage playerState{};
    PlayerModalState modalState{};
    PlayerMasterModalData modalData{};
    GameNetPlayerRow row{};
    zVec3 targetA{1.0f, 2.0f, 3.0f};
    zVec3 targetB{4.0f, 5.0f, 6.0f};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    saveState.netPlayerRow = &row;
    modalState.masterModalData = &modalData;
    modalData.masterType = 5;
    row.playerColorIndex = 7;

    g_GameStateOrMapTable =
        static_cast<zInput_GameStateOrMapTablePartial *>(static_cast<void *>(&saveState));
    g_Time_AccumulatedTimeSec = 20.0f;
    g_FrameDeltaTimeSec = 0.25f;
    g_GameNetPkt06NextSendTimeSec = 19.0f;
    g_GameNetPkt06InputBit16Latch = 0;
    g_GameNetPkt06InputBit17Latch = 0;
    g_GameNetPkt06InitialSyncGate = 0;
    g_HudSensorTracker.raceCheckpointMode = 1;
    g_HudTimerPanelNetState = {};
    g_HudTimerPanelNetState.timerDirectionNeg = 1;
    g_HudTimerPanelNetState.tenSecondWarningsEnabled = 1;
    g_NetPkt06_PlayerStateSnapshotBuf = {};

    playerState.netInputBit16Latch = 1;
    playerState.netInputBit17Latch = 0;
    playerState.cachedAltSelectionCode = 301;
    playerState.cachedPrimarySelectionCode = 400;
    playerState.altGunAimOrigin = {10.0f, 11.0f, 12.0f};
    playerState.storedTargetPos = {20.0f, 21.0f, 22.0f};
    playerState.worldPos = {30.0f, 31.0f, 32.0f};
    playerState.vehicleRotationAngles = {0.1f, 0.2f, 0.3f};
    playerState.statusMeterValue = 88.0f;
    playerState.progressTargetCount = 2;
    playerState.progressTargetSlots[0].targetPos = &targetA;
    playerState.progressTargetSlots[1].targetPos = &targetB;

    g_sendCalls = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    const int result = GameNet::TickLocalPlayerPkt06ReplicationAndHudTimer(&saveState);
    const NetPkt06_PlayerStateSnapshot *const sentPacket =
        reinterpret_cast<const NetPkt06_PlayerStateSnapshot *>(g_sendPacketBytes);
    const bool packetOk =
        result == 0 && g_sendCalls == 1 && g_sendFlags == 0 &&
        g_sendPacket == &g_NetPkt06_PlayerStateSnapshotBuf.header &&
        g_sendPacketSize == 0x44 + 4 + 2 * sizeof(zVec3) &&
        sentPacket->header.packetType == 0x06 &&
        sentPacket->header.packetSizeBytes == 0x44 + 4 + 2 * sizeof(zVec3) &&
        sentPacket->header.payloadDword0 == 0x55667788 &&
        sentPacket->cachedAltSelectionCode == 301 &&
        sentPacket->cachedPrimarySelectionCode == 400 &&
        (sentPacket->packedMasterTypeColorFlags & 0x7ffffu) ==
            (5u | (7u << 8) | 0x10000u | 0x40000u) &&
        Vec3Equals(sentPacket->altGunAimOrigin, {10.0f, 11.0f, 12.0f}) &&
        Vec3Equals(sentPacket->storedTargetPos, {20.0f, 21.0f, 22.0f}) &&
        Vec3Equals(sentPacket->worldPos, {30.0f, 31.0f, 32.0f}) &&
        Vec3Equals(sentPacket->vehicleRotationAngles, {0.1f, 0.2f, 0.3f}) &&
        sentPacket->statusMeterValue == 88.0f && sentPacket->progressTargetCount == 2 &&
        Vec3Equals(sentPacket->progressTargetPoints[0], targetA) &&
        Vec3Equals(sentPacket->progressTargetPoints[1], targetB) &&
        g_GameNetPkt06InputBit16Latch == 0 && g_GameNetPkt06InputBit17Latch == 0 &&
        FloatNear(g_GameNetPkt06NextSendTimeSec, 20.1f);

    HudUiTimerPanel::SetSeconds(9.0f, -1.0f);
    g_Time_AccumulatedTimeSec = 21.0f;
    g_GameNetPkt06NextSendTimeSec = 20.0f;
    g_HudTimerPanelNetState = {};
    g_HudTimerPanelNetState.timerDirectionNeg = 1;
    g_HudTimerPanelNetState.tenSecondWarningsEnabled = 1;
    playerState.progressTargetCount = 0;
    g_sendCalls = 0;
    GameNet::TickLocalPlayerPkt06ReplicationAndHudTimer(&saveState);
    const bool countdownTriggeredOk = g_HudTimerPanelNetState.startCountdownTriggered == 1;
    const bool countdownTimerOk = FieldAt<float>(&timer, 0x2a4) == 10.25f &&
                                  FieldAt<std::int32_t>(&timer, 0x2ac) == -1;
    const bool countdownSendOk = g_sendCalls >= 2;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    g_HudUiMgrTimerPanel = nullptr;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_GameStateOrMapTable = oldGameState;
    g_HudSensorTracker.raceCheckpointMode = oldRaceMode;
    g_HudTimerPanelNetState = oldTimerState;
    g_GameNetPkt06InitialSyncGate = oldInitialGate;
    g_GameNetPkt06InputBit16Latch = oldLatch16;
    g_GameNetPkt06InputBit17Latch = oldLatch17;
    g_GameNetPkt06NextSendTimeSec = oldNextSend;
    g_GameNetHudTimerTenSecondWarningArmed = oldTenSecondArmed;
    g_GameNetHudTimerPendingSaveReminderArmed = oldPendingReminderArmed;
    g_NetPkt06_PlayerStateSnapshotBuf = oldPkt06;
    g_Time_AccumulatedTimeSec = 0.0f;
    g_FrameDeltaTimeSec = 0.0f;

    if (!packetOk) {
        return 1;
    }
    if (!countdownTriggeredOk) {
        return 2;
    }
    if (!countdownTimerOk) {
        return 3;
    }
    if (!countdownSendOk) {
        return 4;
    }
    return 0;
}

extern "C" int gamenet_scoreboard_snapshot_packet_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    const std::int32_t oldRaceMode = g_HudSensorTracker.raceCheckpointMode;
    const std::int32_t oldGoalValue = g_HudSensorTracker.runtimeGoalValue;
    const std::int32_t oldOneLapShown = g_GameNetOneLapLeftMessageShown;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldIsHost = g_zNetwork_IsHostFlag;
    const std::int32_t oldTcpIpAsync = g_zNetwork_TcpIpAsyncSendEnabled;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    GameNetPlayerRow alpha{};
    alpha.playerKey = 0x101;
    alpha.playerColorPackedRgb = 0x00112233;
    std::strcpy(alpha.displayName, "Alpha");

    GameNetPlayerRow bravo{};
    bravo.playerKey = 0x202;
    bravo.playerColorPackedRgb = 0x00445566;
    std::strcpy(bravo.displayName, "Bravo");
    alpha.next = &bravo;

    g_GameNetPlayerRowHead = &alpha;
    g_GameNetPlayerRowTail = &bravo;
    g_GameNetPlayerRowCount = 2;
    g_HudSensorTracker.raceCheckpointMode = 0;
    g_HudSensorTracker.runtimeGoalValue = 999;
    g_GameNetOneLapLeftMessageShown = 0;
    g_zNetwork_IsHostFlag = 0;

    GameNet::RefreshPlayerListMenu(&alpha);
    GameNet::RefreshPlayerListMenu(&bravo);

    ScoreboardPacket2 packet{};
    packet.header.packetType = 0x09;
    packet.header.packetSizeBytes = sizeof(packet);
    packet.entryCount = 2;
    packet.entries[0].playerKey = alpha.playerKey;
    packet.entries[0].packedScoreAndLapCount = static_cast<std::uint16_t>((3 << 9) | 17);
    packet.entries[1].playerKey = bravo.playerKey;
    packet.entries[1].packedScoreAndLapCount = static_cast<std::uint16_t>((4 << 9) | 22);

    const std::int32_t handleResult = GameNet::HandlePkt09_PlayerScoreboardSnapshot(
        0, reinterpret_cast<NetPkt09_PlayerScoreboardSnapshot *>(&packet));
    const bool applied = handleResult == 1 && alpha.score == 17 && alpha.lapCount == 3 &&
                         bravo.score == 22 && bravo.lapCount == 4 &&
                         triplet.entries.begin[0].playerKey == bravo.playerKey &&
                         triplet.entries.begin[1].playerKey == alpha.playerKey;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x4444;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x5678;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    alpha.score = 33;
    alpha.lapCount = 7;
    bravo.score = 44;
    bravo.lapCount = 5;
    GameNet::SendPkt09_PlayerScoreboardSnapshot();

    const ScoreboardPacket2 *const sentPacket =
        reinterpret_cast<const ScoreboardPacket2 *>(g_sendPacketBytes);
    const bool sent =
        g_sendCalls == 1 && g_sendFlags == 1 && g_sendPacketSize == sizeof(ScoreboardPacket2) &&
        g_sendPacketBytesSize == sizeof(ScoreboardPacket2) &&
        sentPacket->header.packetType == 0x09 &&
        sentPacket->header.packetSizeBytes == sizeof(ScoreboardPacket2) &&
        sentPacket->header.payloadDword0 == 0x5678 && sentPacket->entryCount == 2 &&
        sentPacket->entries[0].playerKey == alpha.playerKey &&
        sentPacket->entries[0].packedScoreAndLapCount ==
            static_cast<std::uint16_t>((7 << 9) | 33) &&
        sentPacket->entries[1].playerKey == bravo.playerKey &&
        sentPacket->entries[1].packedScoreAndLapCount == static_cast<std::uint16_t>((5 << 9) | 44);

    g_HudUiMgrStatsList = oldStatsList;
    g_HudSensorTracker.raceCheckpointMode = oldRaceMode;
    g_HudSensorTracker.runtimeGoalValue = oldGoalValue;
    g_GameNetOneLapLeftMessageShown = oldOneLapShown;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldTcpIpAsync;
    triplet.DestructorCore();

    return applied && sent ? 0 : 1;
}

extern "C" int gamenet_lap_progress_packet_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    const std::int32_t oldGoalValue = g_HudSensorTracker.runtimeGoalValue;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldIsHost = g_zNetwork_IsHostFlag;
    const std::int32_t oldTcpIpAsync = g_zNetwork_TcpIpAsyncSendEnabled;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x1111;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x2222;
    g_zNetwork_IsHostFlag = 0;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    zUtil_PlayerStateStorage playerState{};
    playerState.lapCount = 4;
    playerState.lapTimeSec = 65.0f;
    zUtil_SaveGameState saveState{};
    GameNetPlayerRow localRow{};
    saveState.playerState = &playerState;
    saveState.netPlayerRow = &localRow;

    GameNet::SendPkt0E_PlayerLapProgress(&saveState);
    const NetPkt0E_PlayerLapProgress *const sentPacket =
        reinterpret_cast<const NetPkt0E_PlayerLapProgress *>(g_sendPacketBytes);
    const bool clientSend =
        g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacketSize == sizeof(NetPkt0E_PlayerLapProgress) &&
        sentPacket->header.packetType == 0x0e &&
        sentPacket->header.packetSizeBytes == sizeof(NetPkt0E_PlayerLapProgress) &&
        sentPacket->header.payloadDword0 == 0x2222 && sentPacket->lapCountPacked == 4 &&
        sentPacket->lapTimeSec == 65.0f && localRow.lapCount == 0;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    GameNetPlayerRow remoteRow{};
    remoteRow.playerKey = 0x3333;
    remoteRow.playerColorPackedRgb = 0x00123456;
    std::strcpy(remoteRow.displayName, "Remote");
    g_GameNetPlayerRowHead = &remoteRow;
    g_GameNetPlayerRowTail = &remoteRow;
    g_GameNetPlayerRowCount = 1;
    g_HudSensorTracker.runtimeGoalValue = 3;
    GameNet::RefreshPlayerListMenu(&remoteRow);

    g_zNetwork_IsHostFlag = 1;
    g_sendCalls = 0;
    NetPkt0E_PlayerLapProgress packet{};
    packet.lapCountPacked = 2;
    packet.lapTimeSec = 44.0f;
    const bool hostHandle = GameNet::HandlePkt0E_PlayerLapProgress(0x3333, &packet) == 1 &&
                            remoteRow.lapCount == 2 && remoteRow.lapTimeSec == 44.0f &&
                            g_sendCalls == 1;

    g_HudUiMgrStatsList = oldStatsList;
    g_HudSensorTracker.runtimeGoalValue = oldGoalValue;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldTcpIpAsync;
    triplet.DestructorCore();
    return clientSend && hostHandle ? 0 : 1;
}

extern "C" int gamenet_chat_message_packet_smoke(void) {
    HudUiChatMessageStack chat{};
    chat.Constructor();
    chat.base.enabled = 1;
    g_HudUiChatMessageStack = &chat;

    NetPkt0B_ChatMessage packet = {};
    packet.messageLength = 5;
    std::memcpy(packet.message, "hello", 5);
    GameNet::HandlePkt0B_ChatMessage(0, &packet);

    HudUiPanel *const firstLine = reinterpret_cast<HudUiPanel *>(&chat.lines[0][0]);
    const bool shortMessage = std::strcmp(firstLine->GetLastTextPtr(), "hello") == 0 &&
                              FieldAt<float>(firstLine, 0x10) == 5.0f;

    NetPkt0B_ChatMessage longPacket = {};
    longPacket.messageLength = 0x55;
    for (std::size_t index = 0; index < sizeof(longPacket.message); ++index) {
        longPacket.message[index] = static_cast<char>('A' + (index % 26));
    }

    GameNet::HandlePkt0B_ChatMessage(0, &longPacket);
    const char *const text = firstLine->GetLastTextPtr();
    const bool clamped = std::strlen(text) == sizeof(longPacket.message) &&
                         std::memcmp(text, longPacket.message, sizeof(longPacket.message)) == 0;

    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&chat.lines[index][0]);
        DeleteObject(panel->hFont);
        panel->hFont = nullptr;
    }

    g_HudUiChatMessageStack = nullptr;
    return shortMessage && clamped ? 0 : 1;
}

extern "C" int gamenet_show_player_kill_message_smoke(void) {
    HudUiTopMessageStack top{};
    top.Constructor();
    top.base.enabled = 1;
    g_HudUiTopMessageStack = &top;

    GameNetPlayerRow victim{};
    std::strcpy(victim.displayName, "Victim");
    GameNetPlayerRow killer{};
    std::strcpy(killer.displayName, "Killer");
    OptCatalogEntryDef killEntry{};
    killEntry.killVerbString = const_cast<char *>("tagged");

    GameNet::ShowPlayerKillMessage(&victim, &killEntry, &killer);

    HudUiPanel *const firstLine = reinterpret_cast<HudUiPanel *>(&top.lines[0][0]);
    const bool ok = std::strcmp(firstLine->GetLastTextPtr(), "Victim tagged Killer") == 0 &&
                    FieldAt<float>(firstLine, 0x10) == 2.0f;

    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&top.lines[index][0]);
        DeleteObject(panel->hFont);
        panel->hFont = nullptr;
    }

    g_HudUiTopMessageStack = nullptr;
    return ok ? 0 : 1;
}

extern "C" int gamenet_player_kill_event_packet_smoke(void) {
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    const std::int32_t oldRaceMode = g_HudSensorTracker.raceCheckpointMode;
    const std::int32_t oldGoalValue = g_HudSensorTracker.runtimeGoalValue;
    const std::int32_t oldOneLapShown = g_GameNetOneLapLeftMessageShown;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    const std::int32_t oldOptCatalogEntryCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldOptCatalogEntryTable = g_OptCatalog_EntryTable;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldIsHost = g_zNetwork_IsHostFlag;
    const std::int32_t oldTcpIpAsync = g_zNetwork_TcpIpAsyncSendEnabled;

    HudUiTopMessageStack top{};
    top.Constructor();
    top.base.enabled = 1;
    g_HudUiTopMessageStack = &top;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    OptCatalogEntryDef killEntry{};
    killEntry.keyName = const_cast<char *>("test_weapon");
    killEntry.ordinalIndex = 3;
    killEntry.killVerbString = const_cast<char *>("tagged");
    g_OptCatalog_EntryCount = 1;
    g_OptCatalog_EntryTable = &killEntry;

    GameNetPlayerRow killer{};
    killer.playerKey = 0x11;
    killer.lapCount = 1;
    killer.playerColorPackedRgb = 0x00112233;
    std::strcpy(killer.displayName, "Killer");

    GameNetPlayerRow victim{};
    victim.playerKey = 0x22;
    victim.score = 4;
    victim.lapCount = 2;
    victim.playerColorPackedRgb = 0x00445566;
    std::strcpy(victim.displayName, "Victim");
    killer.next = &victim;

    g_GameNetPlayerRowHead = &killer;
    g_GameNetPlayerRowTail = &victim;
    g_GameNetPlayerRowCount = 2;
    g_HudSensorTracker.raceCheckpointMode = 0;
    g_HudSensorTracker.runtimeGoalValue = 999;
    g_GameNetOneLapLeftMessageShown = 0;
    GameNet::RefreshPlayerListMenu(&killer);
    GameNet::RefreshPlayerListMenu(&victim);

    NetPkt08_PlayerKillEvent packet{};
    packet.killMethodOrOptCatalogEntryId = 3;
    packet.targetPlayerKey = victim.playerKey;
    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;

    const std::int32_t nonHostResult =
        GameNet::HandlePkt08_PlayerKillEvent(killer.playerKey, &packet);
    HudUiPanel *const firstLine = reinterpret_cast<HudUiPanel *>(&top.lines[0][0]);
    const bool nonHost = nonHostResult == 1 && victim.score == 4 && g_sendCalls == 0 &&
                         std::strcmp(firstLine->GetLastTextPtr(), "Victim tagged Killer") == 0;

    packet.targetPlayerKey = 0x7777;
    const bool missingRow = GameNet::HandlePkt08_PlayerKillEvent(killer.playerKey, &packet) == 0;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x4444;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x5678;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;

    packet.targetPlayerKey = victim.playerKey;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    const std::int32_t hostResult = GameNet::HandlePkt08_PlayerKillEvent(killer.playerKey, &packet);
    const ScoreboardPacket2 *const hostSentPacket =
        reinterpret_cast<const ScoreboardPacket2 *>(g_sendPacketBytes);
    const bool host = hostResult == 1 && victim.score == 5 && g_sendCalls == 1 &&
                      g_sendPacketSize == sizeof(ScoreboardPacket2) &&
                      hostSentPacket->entries[1].playerKey == victim.playerKey &&
                      hostSentPacket->entries[1].packedScoreAndLapCount ==
                          static_cast<std::uint16_t>((victim.lapCount << 9) | 5);

    packet.targetPlayerKey = killer.playerKey;
    killer.score = 0;
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    const std::int32_t suicideResult =
        GameNet::HandlePkt08_PlayerKillEvent(killer.playerKey, &packet);
    const ScoreboardPacket2 *const suicideSentPacket =
        reinterpret_cast<const ScoreboardPacket2 *>(g_sendPacketBytes);
    const bool suicide = suicideResult == 1 && killer.score == 0 && g_sendCalls == 1 &&
                         suicideSentPacket->entries[0].playerKey == killer.playerKey &&
                         suicideSentPacket->entries[0].packedScoreAndLapCount ==
                             static_cast<std::uint16_t>(killer.lapCount << 9);

    zUtil_SaveGameState saveState{};
    saveState.netPlayerRow = &victim;
    g_zNetwork_LocalPlayerKey = killer.playerKey;
    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    GameNet::SendPkt08_PlayerKillEvent(&saveState, 3);
    const NetPkt08_PlayerKillEvent *const sentKillPacket =
        reinterpret_cast<const NetPkt08_PlayerKillEvent *>(g_sendPacketBytes);
    const bool explicitSaveStateSend =
        g_sendCalls == 1 && g_sendPacketSize == sizeof(NetPkt08_PlayerKillEvent) &&
        sentKillPacket->header.packetType == 0x08 &&
        sentKillPacket->header.packetSizeBytes == sizeof(NetPkt08_PlayerKillEvent) &&
        sentKillPacket->header.payloadDword0 == killer.playerKey &&
        sentKillPacket->killMethodOrOptCatalogEntryId == 3 &&
        sentKillPacket->targetPlayerKey == victim.playerKey;

    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    GameNet::SendPkt08_PlayerKillEvent(nullptr, 3);
    const NetPkt08_PlayerKillEvent *const fallbackKillPacket =
        reinterpret_cast<const NetPkt08_PlayerKillEvent *>(g_sendPacketBytes);
    const bool fallbackSaveStateSend =
        g_sendCalls == 1 && fallbackKillPacket->header.payloadDword0 == killer.playerKey &&
        fallbackKillPacket->targetPlayerKey == victim.playerKey;

    g_HudUiTopMessageStack = oldTopStack;
    g_HudUiMgrStatsList = oldStatsList;
    g_HudSensorTracker.raceCheckpointMode = oldRaceMode;
    g_HudSensorTracker.runtimeGoalValue = oldGoalValue;
    g_GameNetOneLapLeftMessageShown = oldOneLapShown;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_OptCatalog_EntryCount = oldOptCatalogEntryCount;
    g_OptCatalog_EntryTable = oldOptCatalogEntryTable;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldTcpIpAsync;
    DeleteTopMessageStackFonts(top);
    triplet.DestructorCore();

    return nonHost && missingRow && host && suicide && explicitSaveStateSend &&
                   fallbackSaveStateSend
               ? 0
               : 1;
}

extern "C" int gamenet_reassign_player_colors_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    zNetworkPlayerRecordList *const oldPlayerRecordList = g_zNetwork_PlayerRecordList;
    zNetworkDPlaySessionDescCache *const oldSession = g_zNetwork_CurrentSessionDescCache;

    zNetwork_PlayerRecord firstRecord{};
    firstRecord.playerKey = 0x1111;
    firstRecord.colorIndex = 8;
    zNetwork_PlayerRecord secondRecord{};
    secondRecord.playerKey = 0x2222;
    secondRecord.colorIndex = 2;

    zNetworkPlayerRecordListNode sentinel{};
    zNetworkPlayerRecordListNode firstNode{};
    zNetworkPlayerRecordListNode secondNode{};
    sentinel.next = &firstNode;
    sentinel.prev = &secondNode;
    firstNode.next = &secondNode;
    firstNode.prev = &sentinel;
    firstNode.playerRecord = &firstRecord;
    secondNode.next = &sentinel;
    secondNode.prev = &firstNode;
    secondNode.playerRecord = &secondRecord;
    zNetworkPlayerRecordList playerList{};
    playerList.sentinelNode = &sentinel;
    playerList.count = 2;
    g_zNetwork_PlayerRecordList = &playerList;

    zNetworkDPlaySessionDescCache session{};
    session.desc.maxPlayers = 8;
    g_zNetwork_CurrentSessionDescCache = &session;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    zClass_Object3DDataPartial firstObject{};
    zClass_NodePartial firstObjectNode{};
    firstObjectNode.classId = 5;
    firstObjectNode.classData = &firstObject;
    PlayerModalState firstModal{};
    firstModal.modalNode = &firstObjectNode;
    GameNetPlayerSaveState firstSave{};
    firstSave.primaryModalState = &firstModal;

    zClass_Object3DDataPartial secondObject{};
    zClass_NodePartial secondObjectNode{};
    secondObjectNode.classId = 5;
    secondObjectNode.classData = &secondObject;
    PlayerModalState secondModal{};
    secondModal.modalNode = &secondObjectNode;
    GameNetPlayerSaveState secondSave{};
    secondSave.primaryModalState = &secondModal;

    GameNetPlayerRow firstRow{};
    firstRow.playerKey = firstRecord.playerKey;
    firstRow.saveState = &firstSave;
    std::strcpy(firstRow.displayName, "First");
    GameNetPlayerRow secondRow{};
    secondRow.playerKey = secondRecord.playerKey;
    secondRow.saveState = &secondSave;
    std::strcpy(secondRow.displayName, "Second");
    firstRow.next = &secondRow;
    g_GameNetPlayerRowHead = &firstRow;
    g_GameNetPlayerRowTail = &secondRow;
    g_GameNetPlayerRowCount = 2;

    GameNet::RefreshPlayerListMenu(&firstRow);
    GameNet::RefreshPlayerListMenu(&secondRow);
    const std::int32_t result = GameNet::ReassignPlayerColorsAndRefreshRows(0, nullptr);

    const HudUiScoreboardEntry *firstEntry = nullptr;
    const HudUiScoreboardEntry *secondEntry = nullptr;
    for (HudUiScoreboardEntry *entry = triplet.entries.begin; entry != triplet.entries.end;
         ++entry) {
        if (entry->playerKey == firstRow.playerKey) {
            firstEntry = entry;
        }
        if (entry->playerKey == secondRow.playerKey) {
            secondEntry = entry;
        }
    }

    const bool firstOk = firstRow.playerColorIndex == 8 &&
                         firstRow.playerColorPackedRgb == 0x000040ff &&
                         FieldAt<std::uint32_t>(&firstRow.hudWidget, 0x14c) == 0x000040ff &&
                         FieldAt<std::uint32_t>(&firstRow.hudWidget, 0x150) == 0x000040ff &&
                         FieldAt<std::int32_t>(&firstRow.hudWidget, 0x270) == 1 &&
                         firstEntry != nullptr && firstEntry->playerColorPackedRgb == 0x000040ff &&
                         firstObject.color.red == 1.0f && firstObject.color.green == 1.0f &&
                         firstObject.color.blue == 0.0f && firstObject.colorAlpha == 0.2f;

    const bool secondOk =
        secondRow.playerColorIndex == 2 && secondRow.playerColorPackedRgb == 0x0000ff00 &&
        FieldAt<std::uint32_t>(&secondRow.hudWidget, 0x14c) == 0x0000ff00 &&
        FieldAt<std::uint32_t>(&secondRow.hudWidget, 0x150) == 0x0000ff00 &&
        FieldAt<std::int32_t>(&secondRow.hudWidget, 0x270) == 1 && secondEntry != nullptr &&
        secondEntry->playerColorPackedRgb == 0x0000ff00 && secondObject.color.red == 0.0f &&
        secondObject.color.green == 1.0f && secondObject.color.blue == 0.0f &&
        secondObject.colorAlpha == 0.2f;

    g_HudUiMgrStatsList = oldStatsList;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_zNetwork_PlayerRecordList = oldPlayerRecordList;
    g_zNetwork_CurrentSessionDescCache = oldSession;
    triplet.DestructorCore();

    return result == 1 && firstOk && secondOk ? 0 : 1;
}

extern "C" int gamenet_player_row_apply_color_tint_smoke(void) {
    zClass_Object3DDataPartial objectData{};
    zClass_NodePartial objectNode{};
    objectNode.classId = 5;
    objectNode.classData = &objectData;

    PlayerModalState modalState{};
    modalState.modalNode = &objectNode;
    GameNetPlayerSaveState saveState{};
    saveState.primaryModalState = &modalState;
    GameNetPlayerRow row{};
    row.playerColorIndex = 8;
    row.saveState = &saveState;

    row.ApplyPlayerColorTint();
    return objectData.color.red == 1.0f && objectData.color.green == 1.0f &&
                   objectData.color.blue == 0.0f && objectData.colorAlpha == 0.2f &&
                   (objectData.flags & 4) != 0
               ? 0
               : 1;
}

extern "C" int gamenet_apply_pkt06_player_state_snapshot_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    const int oldFrameTick = g_zVideo_FrameTick;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    zClass_Object3DDataPartial objectData{};
    zClass_NodePartial objectNode{};
    objectNode.classId = 5;
    objectNode.classData = &objectData;

    PlayerMasterModalData modalData{};
    modalData.masterType = 5;
    PlayerModalState modalState{};
    modalState.masterModalData = &modalData;
    modalState.modalNode = &objectNode;

    zUtil_PlayerStateStorage playerState{};
    GameNetPlayerSaveState saveState{};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    GameNetPlayerRow row{};
    row.playerKey = 0x2468;
    row.playerColorIndex = 1;
    row.saveState = &saveState;
    std::strcpy(row.displayName, "Pkt06");
    g_GameNetPlayerRowHead = &row;
    g_GameNetPlayerRowTail = &row;
    g_GameNetPlayerRowCount = 1;
    GameNet::RefreshPlayerListMenu(&row);

    zVec3 staleTargets[10]{};
    for (int index = 0; index < 10; ++index) {
        playerState.progressTargetRuntimeSlots[index].targetPos = &staleTargets[index];
    }
    playerState.cachedAltSelectionCode = 301;
    playerState.cachedPrimarySelectionCode = 201;
    playerState.netLastUpdateFrameTick = oldFrameTick - 1;
    g_zVideo_FrameTick = oldFrameTick + 17;

    NetPkt06_PlayerStateSnapshot packet{};
    packet.cachedAltSelectionCode = 301;
    packet.cachedPrimarySelectionCode = 201;
    packet.packedMasterTypeColorFlags = 5u | (8u << 8) | 0x10000u;
    packet.storedTargetPos = {1.0f, 2.0f, 3.0f};
    packet.worldPos = {4.0f, 5.0f, 6.0f};
    packet.vehicleRotationAngles = {0.25f, 0.5f, 0.75f};
    packet.statusMeterValue = 77.0f;

    const int clearResult = GameNet::ApplyPkt06_PlayerStateSnapshotToRow(&row, &packet);
    bool clearedSlots = true;
    for (int index = 0; index < 10; ++index) {
        clearedSlots = clearedSlots && playerState.progressTargetRuntimeSlots[index].targetPos == nullptr;
    }
    const bool firstOk =
        clearResult == 1 && playerState.netUpdateReceived == 1 &&
        row.playerColorIndex == 8 && row.playerColorPackedRgb == 0x000040ff &&
        FieldAt<std::uint32_t>(&row.hudWidget, 0x14c) == 0x000040ff &&
        FieldAt<std::uint32_t>(&row.hudWidget, 0x150) == 0x000040ff &&
        FieldAt<std::int32_t>(&row.hudWidget, 0x270) == 1 &&
        objectData.color.red == 1.0f && objectData.color.green == 1.0f &&
        objectData.color.blue == 0.0f && objectData.colorAlpha == 0.2f &&
        Vec3Equals(playerState.netReceivedPos, packet.worldPos) &&
        Vec3Equals(playerState.netReceivedAngles, packet.vehicleRotationAngles) &&
        Vec3Equals(playerState.storedTargetPos, packet.storedTargetPos) &&
        playerState.netInputBit16Latch == 1 && playerState.netInputBit17Latch == 0 &&
        playerState.netLastUpdateFrameTick == g_zVideo_FrameTick &&
        FloatNear(playerState.statusMeterValue, 77.0f) &&
        playerState.progressTargetCount == 0 && clearedSlots;

    packet.packedMasterTypeColorFlags = 5u | (8u << 8) | 0x20000u | 0x40000u;
    packet.statusMeterValue = 88.0f;
    packet.progressTargetCount = 2;
    packet.progressTargetPoints[0] = {10.0f, 11.0f, 12.0f};
    packet.progressTargetPoints[1] = {20.0f, 21.0f, 22.0f};

    const int targetResult = GameNet::ApplyPkt06_PlayerStateSnapshotToRow(&row, &packet);
    const bool secondOk =
        targetResult == 1 && playerState.netInputBit16Latch == 1 &&
        playerState.netInputBit17Latch == 1 && FloatNear(playerState.statusMeterValue, 88.0f) &&
        playerState.progressTargetCount == 2 &&
        playerState.progressTargetRuntimeSlots[0].targetPos ==
            &playerState.progressTargetPointStorage[0] &&
        playerState.progressTargetRuntimeSlots[1].targetPos ==
            &playerState.progressTargetPointStorage[1] &&
        Vec3Equals(playerState.progressTargetPointStorage[0], packet.progressTargetPoints[0]) &&
        Vec3Equals(playerState.progressTargetPointStorage[1], packet.progressTargetPoints[1]);

    g_HudUiMgrStatsList = oldStatsList;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_zVideo_FrameTick = oldFrameTick;
    triplet.DestructorCore();

    return firstOk && secondOk ? 0 : 1;
}

extern "C" int gamenet_handle_pkt06_player_state_snapshot_smoke(void) {
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    const int oldInitialSyncGate = g_GameNetPkt06InitialSyncGate;
    const int oldFrameTick = g_zVideo_FrameTick;

    const int nullResult = GameNet::HandlePkt06_PlayerStateSnapshot(0x1111, nullptr);

    zClass_Object3DDataPartial objectData{};
    zClass_NodePartial objectNode{};
    objectNode.classId = 5;
    objectNode.classData = &objectData;

    PlayerMasterModalData modalData{};
    modalData.masterType = 5;
    PlayerModalState modalState{};
    modalState.masterModalData = &modalData;
    modalState.modalNode = &objectNode;

    zUtil_PlayerStateStorage playerState{};
    GameNetPlayerSaveState saveState{};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    GameNetPlayerRow row{};
    row.playerKey = 0x2468;
    row.playerColorIndex = 1;
    row.saveState = &saveState;
    g_GameNetPlayerRowHead = &row;
    g_GameNetPlayerRowTail = &row;
    g_GameNetPlayerRowCount = 1;

    NetPkt06_PlayerStateSnapshot ignoredPacket{};
    ignoredPacket.header.packetType = 5;
    ignoredPacket.header.payloadDword0 = row.playerKey;
    g_GameNetPkt06InitialSyncGate = 1;
    const int ignoredResult = GameNet::HandlePkt06_PlayerStateSnapshot(0x1111, &ignoredPacket);
    const bool ignoredOk =
        ignoredResult == 0 && g_GameNetPkt06InitialSyncGate == 0 && playerState.netUpdateReceived == 0;

    NetPkt06_PlayerStateSnapshot packet{};
    packet.header.packetType = 6;
    packet.header.payloadDword0 = row.playerKey;
    packet.packedMasterTypeColorFlags = 5u | (1u << 8);
    packet.worldPos = {8.0f, 9.0f, 10.0f};
    packet.vehicleRotationAngles = {0.125f, 0.25f, 0.5f};
    g_zVideo_FrameTick = oldFrameTick + 3;

    const int handledResult = GameNet::HandlePkt06_PlayerStateSnapshot(0x1111, &packet);
    const bool handledOk = handledResult == 0 && playerState.netUpdateReceived == 1 &&
                           row.playerColorIndex == 1 &&
                           Vec3Equals(playerState.netReceivedPos, packet.worldPos) &&
                           Vec3Equals(playerState.netReceivedAngles, packet.vehicleRotationAngles);

    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_GameNetPkt06InitialSyncGate = oldInitialSyncGate;
    g_zVideo_FrameTick = oldFrameTick;

    return nullResult == -1 && ignoredOk && handledOk ? 0 : 1;
}

extern "C" int gamenet_spawn_remote_player_missing_template_smoke(void) {
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    zNetworkPlayerRecordList *const oldPlayerRecordList = g_zNetwork_PlayerRecordList;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;

    zNetwork_PlayerRecord playerRecord{};
    playerRecord.playerKey = 0x1111;
    std::strcpy(playerRecord.playerName, "Remote");
    zNetworkPlayerRecordListNode sentinel{};
    zNetworkPlayerRecordListNode playerNode{};
    sentinel.next = &playerNode;
    sentinel.prev = &playerNode;
    playerNode.next = &sentinel;
    playerNode.prev = &sentinel;
    playerNode.playerRecord = &playerRecord;
    zNetworkPlayerRecordList playerList{};
    playerList.sentinelNode = &sentinel;
    playerList.count = 1;
    g_zNetwork_PlayerRecordList = &playerList;

    HudUiTopMessageStack topStack{};
    topStack.base.enabled = 0;
    g_HudUiTopMessageStack = &topStack;

    g_GameNetPlayerRowHead = nullptr;
    g_GameNetPlayerRowTail = nullptr;
    g_GameNetPlayerRowCount = 0;

    NetPkt06_PlayerStateSnapshot packet{};
    packet.header.packetType = 6;
    packet.header.payloadDword0 = 0x2222;

    const int result = GameNet::SpawnRemotePlayerFromPkt06_PlayerStateSnapshot(
        static_cast<int>(playerRecord.playerKey), &packet);
    const bool ok = result == 0 && g_GameNetPlayerRowHead == nullptr &&
                    g_GameNetPlayerRowTail == nullptr && g_GameNetPlayerRowCount == 0;

    g_HudUiTopMessageStack = oldTopStack;
    g_zNetwork_PlayerRecordList = oldPlayerRecordList;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;

    return ok ? 0 : 1;
}

extern "C" int gamenet_handle_pkt07_alt_gun_dispatch_smoke(void) {
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    void *const oldPendingSpawnTargetCountPtr = g_OptCatalogPendingSpawnTargetCountPtr;
    void *const oldPendingSpawnTargetListPtr = g_OptCatalogPendingSpawnTargetListPtr;
    const std::int32_t oldEntryCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;

    NetPkt07_AltGunDispatch missingPacket{};
    missingPacket.header.payloadDword0 = 0x4040;
    const int missingResult = GameNet::HandlePkt07_AltGunDispatch(0x1111, &missingPacket);

    zUtil_SaveGameState saveState{};
    zUtil_PlayerStateStorage playerState{};
    saveState.playerState = &playerState;

    PlayerGunFireController originalController{};
    PlayerGunFireController targetController{};
    playerState.activeAltGunController = &originalController;
    playerState.altGunFireHeldFlag = 1;
    playerState.worldPos = {2.0f, 3.0f, 4.0f};
    playerState.steerBasisRaw = {0.0f, 0.0f, 1.0f};

    OptCatalogEntryDef entry{};
    entry.ordinalIndex = 707;
    g_OptCatalog_EntryCount = 1;
    g_OptCatalog_EntryTable = &entry;
    targetController.optCatalogEntry = &entry;
    playerState.altWeaponBanks[4].controllerA = targetController;

    GameNetPlayerRow row{};
    row.playerKey = 0x3030;
    row.saveState = (GameNetPlayerSaveState *)(&saveState);
    g_GameNetPlayerRowHead = &row;
    g_GameNetPlayerRowTail = &row;
    g_GameNetPlayerRowCount = 1;

    NetPkt07_AltGunDispatch packet{};
    packet.header.payloadDword0 = row.playerKey;
    packet.weaponId = 707;
    packet.dispatchFlags = 0x1234;
    packet.targetPos = {10.0f, 11.0f, 12.0f};
    g_OptCatalogPendingSpawnTargetCountPtr = (void *)(0x11112222);
    g_OptCatalogPendingSpawnTargetListPtr = (void *)(0x33334444);

    const int handledResult = GameNet::HandlePkt07_AltGunDispatch(0x1111, &packet);
    const bool handledOk =
        handledResult == 1 && playerState.altGunDispatchFlags == 0 &&
        playerState.activeAltGunController == &originalController &&
        Vec3Equals(playerState.storedTargetPos, packet.targetPos) &&
        Vec3Equals(playerState.altFireOrigin, {2.0f, 4.0f, 4.0f}) &&
        g_OptCatalogPendingSpawnTargetCountPtr == nullptr &&
        g_OptCatalogPendingSpawnTargetListPtr == nullptr;

    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_OptCatalogPendingSpawnTargetCountPtr = oldPendingSpawnTargetCountPtr;
    g_OptCatalogPendingSpawnTargetListPtr = oldPendingSpawnTargetListPtr;
    g_OptCatalog_EntryCount = oldEntryCount;
    g_OptCatalog_EntryTable = oldEntryTable;

    return missingResult == 0 && handledOk ? 0 : 1;
}

extern "C" int gamenet_send_pkt07_alt_gun_dispatch_smoke(void) {
    const NetPkt07_AltGunDispatch oldPacket = g_NetPkt07_AltGunDispatchBuf;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    zUtil_PlayerStateStorage playerState{};
    playerState.storedTargetPos = {9.0f, 8.0f, 7.0f};
    zUtil_SaveGameState saveState{};
    saveState.playerState = &playerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x1234;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x55667788;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_NetPkt07_AltGunDispatchBuf = {{0x07, sizeof(NetPkt07_AltGunDispatch), 0}, 0, 0,
                                    0, {0.0f, 0.0f, 0.0f}};
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    GameNet::SendPkt07_AltGunDispatch(static_cast<short>(0x8123), 0x01000001u);

    const NetPkt07_AltGunDispatch *const sentPacket =
        reinterpret_cast<const NetPkt07_AltGunDispatch *>(g_sendPacketBytes);
    const bool ok =
        g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt07_AltGunDispatchBuf.header &&
        g_sendPacketSize == sizeof(NetPkt07_AltGunDispatch) &&
        sentPacket->header.packetType == 7 &&
        sentPacket->header.packetSizeBytes == sizeof(NetPkt07_AltGunDispatch) &&
        sentPacket->header.payloadDword0 == 0x55667788 &&
        sentPacket->weaponId == static_cast<short>(0x8123) &&
        sentPacket->dispatchFlags == 0x01000001u &&
        Vec3Equals(sentPacket->targetPos, playerState.storedTargetPos);

    g_NetPkt07_AltGunDispatchBuf = oldPacket;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;

    return ok ? 0 : 1;
}

extern "C" int optcatalog_alt_gun_dispatch_alloc_runtime_gate_smoke(void) {
    const NetPkt07_AltGunDispatch oldPacket = g_NetPkt07_AltGunDispatchBuf;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    OptCatalogEntryDef passEntry{};
    passEntry.ordinalIndex = 0;
    void *passSlot = reinterpret_cast<void *>(0x11223344u);
    const int passZeroResult =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&passEntry, &passSlot);
    passEntry.ordinalIndex = 1;
    const int passOneResult =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&passEntry, &passSlot);
    const bool passOk = passZeroResult == 1 && passOneResult == 1 &&
                        passSlot == reinterpret_cast<void *>(0x11223344u);

    OptCatalogEntryDef entry{};
    entry.ordinalIndex = 0x8123;
    void *nullSlot = nullptr;
    const bool nullOk =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&entry, &nullSlot) == 0 &&
        nullSlot == nullptr;

    zUtil_PlayerStateStorage localPlayerState{};
    localPlayerState.storedTargetPos = {1.0f, 2.0f, 3.0f};
    zUtil_SaveGameState localSaveState{};
    localSaveState.playerState = &localPlayerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&localSaveState;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x2468;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x13572468;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_NetPkt07_AltGunDispatchBuf = {{0x07, sizeof(NetPkt07_AltGunDispatch), 0}, 0, 0,
                                    0, {0.0f, 0.0f, 0.0f}};
    g_sendCalls = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    void *localSlot = &localSaveState;
    const int localResult =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&entry, &localSlot);
    const NetPkt07_AltGunDispatch *const sentPacket =
        reinterpret_cast<const NetPkt07_AltGunDispatch *>(g_sendPacketBytes);
    const bool localOk =
        localResult == 1 && localSlot == reinterpret_cast<void *>(0x01000000u) &&
        g_sendCalls == 1 && sentPacket->header.payloadDword0 == 0x13572468 &&
        sentPacket->weaponId == static_cast<short>(0x8123) &&
        sentPacket->dispatchFlags == 0 &&
        Vec3Equals(sentPacket->targetPos, localPlayerState.storedTargetPos);

    zUtil_PlayerStateStorage remotePlayerState{};
    remotePlayerState.altGunDispatchFlags = 0x01000001;
    zUtil_SaveGameState remoteSaveState{};
    remoteSaveState.playerState = &remotePlayerState;
    void *remoteSlot = &remoteSaveState;
    const bool remoteRejectedOk =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&entry, &remoteSlot) == 0 &&
        remoteSlot == &remoteSaveState;

    remotePlayerState.altGunDispatchFlags = 0x02000012;
    remoteSlot = &remoteSaveState;
    const bool remoteAcceptedOk =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&entry, &remoteSlot) == 1 &&
        remoteSlot == reinterpret_cast<void *>(0x02000012u);

    g_NetPkt07_AltGunDispatchBuf = oldPacket;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;

    if (!passOk) {
        return 1;
    }
    if (!nullOk) {
        return 2;
    }
    if (!localOk) {
        return 3;
    }
    if (!remoteRejectedOk) {
        return 4;
    }
    return remoteAcceptedOk ? 0 : 5;
}

extern "C" int gamenet_alt_gun_dispatch_no_op_callback_smoke(void) {
    OptCatalogEntryDef entry{};
    void *saveStateSlot = nullptr;
    return GameNet::AltGunDispatchNoOpCallback(&entry, &saveStateSlot) == 1 ? 0 : 1;
}

extern "C" int optcatalog_handle_pkt0a_remove_runtime_relay_smoke(void) {
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    const std::int32_t oldEntryCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;
    const int oldRelayEnabled = g_OptCatalogProcessRuntimeRelayEnabled;

    NetPkt0A_RemoveRuntimeRelay packet{};
    packet.ownerPlayerKey = 0x9090;
    const int missingResult = OptCatalog::HandlePkt0A_RemoveRuntimeRelay(0x1111, &packet);

    zClass_NodePartial ownerRoot{};
    zUtil_PlayerStateStorage playerState{};
    playerState.rootNode = &ownerRoot;
    zUtil_SaveGameState saveState{};
    saveState.playerState = &playerState;

    GameNetPlayerRow row{};
    row.playerKey = packet.ownerPlayerKey;
    row.saveState = (GameNetPlayerSaveState *)(&saveState);
    g_GameNetPlayerRowHead = &row;
    g_GameNetPlayerRowTail = &row;
    g_GameNetPlayerRowCount = 1;

    OptCatalogEntryDef entry{};
    entry.ordinalIndex = 303;
    g_OptCatalog_EntryCount = 1;
    g_OptCatalog_EntryTable = &entry;
    g_OptCatalogProcessRuntimeRelayEnabled = 1;

    packet.optCatalogEntryId = 303;
    packet.pointOrVec3 = {0.0f, 0.0f, 0.0f};
    const int handledZeroResult = OptCatalog::HandlePkt0A_RemoveRuntimeRelay(0x1111, &packet);
    const bool zeroOk = handledZeroResult == 1 && g_OptCatalogProcessRuntimeRelayEnabled == 1;

    packet.pointOrVec3 = {1.0f, 0.0f, 0.0f};
    const int handledPointResult = OptCatalog::HandlePkt0A_RemoveRuntimeRelay(0x1111, &packet);
    const bool pointOk = handledPointResult == 1 && g_OptCatalogProcessRuntimeRelayEnabled == 1;

    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_OptCatalog_EntryCount = oldEntryCount;
    g_OptCatalog_EntryTable = oldEntryTable;
    g_OptCatalogProcessRuntimeRelayEnabled = oldRelayEnabled;

    return missingResult == 0 && zeroOk && pointOk ? 0 : 1;
}

extern "C" int optcatalog_send_pkt0a_remove_runtime_relay_smoke(void) {
    const NetPkt0A_RemoveRuntimeRelay oldPacket = g_NetPkt0A_RemoveRuntimeRelayBuf;
    const int oldRelayEnabled = g_OptCatalogProcessRuntimeRelayEnabled;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;

    OptCatalogEntryDef entry{};
    entry.ordinalIndex = 0x4567;

    g_OptCatalogProcessRuntimeRelayEnabled = 0;
    g_sendCalls = 0;
    OptCatalog::SendPkt0A_RemoveRuntimeRelay(&entry, nullptr, nullptr);
    const bool disabledOk = g_sendCalls == 0;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x1111;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x12345678;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_OptCatalogProcessRuntimeRelayEnabled = 1;

    zClass_NodePartial ownerNode{};
    g_sendCalls = 0;
    OptCatalog::SendPkt0A_RemoveRuntimeRelay(&entry, nullptr, &ownerNode);
    const bool missingContextOk = g_sendCalls == 0;

    GameNetPlayerRow ownerRow{};
    ownerRow.playerKey = 0x2468;
    zUtil_SaveGameState ownerSaveState{};
    ownerSaveState.netPlayerRow = &ownerRow;
    HudUiMgrSensorTrackNode trackNode{};
    trackNode.payload = &ownerSaveState;
    ownerNode.callbackContext = (zClass_NodePartial *)&trackNode;

    g_NetPkt0A_RemoveRuntimeRelayBuf = {{0x0a, sizeof(NetPkt0A_RemoveRuntimeRelay), 0},
                                        0, 0, {9.0f, 9.0f, 9.0f}, 0};
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    zVec3 point{4.0f, 5.0f, 6.0f};
    OptCatalog::SendPkt0A_RemoveRuntimeRelay(&entry, &point, &ownerNode);
    const NetPkt0A_RemoveRuntimeRelay *const pointPacket =
        reinterpret_cast<const NetPkt0A_RemoveRuntimeRelay *>(g_sendPacketBytes);
    const bool pointOk =
        g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt0A_RemoveRuntimeRelayBuf.header &&
        g_sendPacketSize == sizeof(NetPkt0A_RemoveRuntimeRelay) &&
        pointPacket->header.payloadDword0 == 0x12345678 &&
        pointPacket->optCatalogEntryId == static_cast<short>(0x4567) &&
        Vec3Equals(pointPacket->pointOrVec3, point) &&
        pointPacket->ownerPlayerKey == ownerRow.playerKey;

    g_NetPkt0A_RemoveRuntimeRelayBuf.pointOrVec3 = {9.0f, 9.0f, 9.0f};
    g_sendCalls = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    OptCatalog::SendPkt0A_RemoveRuntimeRelay(&entry, nullptr, &ownerNode);
    const NetPkt0A_RemoveRuntimeRelay *const zeroPacket =
        reinterpret_cast<const NetPkt0A_RemoveRuntimeRelay *>(g_sendPacketBytes);
    const bool zeroOk =
        g_sendCalls == 1 && zeroPacket->pointOrVec3.x == 0.0f &&
        zeroPacket->pointOrVec3.y == 0.0f && zeroPacket->pointOrVec3.z == 0.0f &&
        zeroPacket->ownerPlayerKey == ownerRow.playerKey;

    g_NetPkt0A_RemoveRuntimeRelayBuf = oldPacket;
    g_OptCatalogProcessRuntimeRelayEnabled = oldRelayEnabled;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;

    if (!disabledOk) {
        return 1;
    }
    if (!missingContextOk) {
        return 2;
    }
    if (!pointOk) {
        return 3;
    }
    return zeroOk ? 0 : 4;
}

extern "C" int gamenet_host_send_pkt10_qsand_feature_smoke(void) {
    const NetPkt10_QSandEvent oldPacket = g_NetPkt10_QSandEventSendBuf;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHost = g_zNetwork_IsHostFlag;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;

    zDEClient_QSandEventTemplate eventTemplate{};
    eventTemplate.radius = 12.5f;
    eventTemplate.center = {7.0f, 8.0f, 9.0f};

    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;
    const int nonHostResult = GameNet::HostSendPkt10_QSandFeature(&eventTemplate);
    const bool nonHostOk = nonHostResult == 0 && g_sendCalls == 0;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x5555;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x12345678;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_NetPkt10_QSandEventSendBuf = {{0x10, sizeof(NetPkt10_QSandEvent), 0}, 0x12u, 0,
                                    {0.0f, 0.0f, 0.0f}, 0.0f};
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    const int hostResult = GameNet::HostSendPkt10_QSandFeature(&eventTemplate);
    const NetPkt10_QSandEvent *const sentPacket =
        reinterpret_cast<const NetPkt10_QSandEvent *>(g_sendPacketBytes);
    const bool hostOk =
        hostResult == 1 && g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt10_QSandEventSendBuf.header &&
        g_sendPacketSize == sizeof(NetPkt10_QSandEvent) &&
        g_sendPacketBytesSize == sizeof(NetPkt10_QSandEvent) &&
        sentPacket->header.packetType == 0x10 &&
        sentPacket->header.packetSizeBytes == sizeof(NetPkt10_QSandEvent) &&
        sentPacket->header.payloadDword0 == 0x12345678 &&
        sentPacket->eventFlags == (0x12u | 0x80u) &&
        Vec3Equals(sentPacket->center, eventTemplate.center) &&
        FloatNear(sentPacket->radius, eventTemplate.radius);

    g_NetPkt10_QSandEventSendBuf = oldPacket;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;

    return nonHostOk && hostOk ? 0 : 1;
}

extern "C" int gamenet_send_pkt10_qsand_event_smoke(void) {
    const NetPkt10_QSandEvent oldPacket = g_NetPkt10_QSandEventSendBuf;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHost = g_zNetwork_IsHostFlag;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zDEClient_NetRelayCallback const oldRelayCallback = g_zDEClientQSandNetRelayCallback;
    const zDEClient_QSandEventTemplate oldDefaults = g_zDEClient_QuickSandEventTemplateDefaults;

    zClass_NodePartial ownerRoot{};
    zClass_NodePartial otherRoot{};
    zUtil_PlayerStateStorage playerState{};
    playerState.rootNode = &ownerRoot;
    zUtil_SaveGameState saveState{};
    saveState.playerState = &playerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x13572468;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = localPlayer.playerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_zDEClientQSandNetRelayCallback = QSandRelayCallbackFake;
    g_zDEClient_QuickSandEventTemplateDefaults = {};
    g_zDEClient_QuickSandEventTemplateDefaults.pointCount = 4;

    zDEClient_QSandEventTemplate negativeEvent{};
    negativeEvent.radius = -2.25f;
    const int negativeResult = GameNet::SendPkt10_QSandEvent(&negativeEvent);
    const bool negativeOk = negativeResult == 1 && FloatNear(negativeEvent.radius, 2.25f);

    g_NetPkt10_QSandEventSendBuf = {{0x10, sizeof(NetPkt10_QSandEvent), 0}, 0x12345678u, 0,
                                    {0.0f, 0.0f, 0.0f}, 0.0f};
    g_sendCalls = 0;
    zDEClient_QSandEventTemplate otherOwnerEvent{};
    otherOwnerEvent.radius = 5.0f;
    otherOwnerEvent.center = {1.0f, 2.0f, 3.0f};
    otherOwnerEvent.damageOwnerNode = &otherRoot;
    const int otherOwnerResult = GameNet::SendPkt10_QSandEvent(&otherOwnerEvent);
    const bool otherOwnerOk = otherOwnerResult == 0 && g_sendCalls == 0 &&
                              g_NetPkt10_QSandEventSendBuf.header.payloadDword0 == 0 &&
                              g_NetPkt10_QSandEventSendBuf.eventFlags == 0x12345678u;

    zDEClient_QSandEventTemplate nonHostEvent{};
    nonHostEvent.radius = 6.5f;
    nonHostEvent.center = {4.0f, 5.0f, 6.0f};
    nonHostEvent.damageOwnerNode = &ownerRoot;
    g_zNetwork_IsHostFlag = 0;
    g_NetPkt10_QSandEventSendBuf = {{0x10, sizeof(NetPkt10_QSandEvent), 0}, 0x12345678u, 0,
                                    {0.0f, 0.0f, 0.0f}, 0.0f};
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    const int nonHostResult = GameNet::SendPkt10_QSandEvent(&nonHostEvent);
    const NetPkt10_QSandEvent *const sentPacket =
        reinterpret_cast<const NetPkt10_QSandEvent *>(g_sendPacketBytes);
    const bool nonHostOk =
        nonHostResult == 0 && g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt10_QSandEventSendBuf.header &&
        g_sendPacketSize == sizeof(NetPkt10_QSandEvent) &&
        sentPacket->header.payloadDword0 == localPlayer.playerKey &&
        sentPacket->eventFlags == 0x12340000u && Vec3Equals(sentPacket->center, nonHostEvent.center) &&
        FloatNear(sentPacket->radius, 6.5f);

    g_zNetwork_IsHostFlag = 1;
    g_qsandRelayCallbackCount = 0;
    g_qsandRelayCallbackResult = 0;
    g_sendCalls = 0;
    zDEClient_QSandEventTemplate hostEvent = nonHostEvent;
    hostEvent.radius = 7.75f;
    g_NetPkt10_QSandEventSendBuf = {{0x10, sizeof(NetPkt10_QSandEvent), 0}, 0x87654321u, 0,
                                    {0.0f, 0.0f, 0.0f}, 0.0f};
    const int hostResult = GameNet::SendPkt10_QSandEvent(&hostEvent);
    const bool hostOk =
        hostResult == 0 && g_qsandRelayCallbackCount == 1 && g_sendCalls == 0 &&
        g_NetPkt10_QSandEventSendBuf.header.payloadDword0 == localPlayer.playerKey &&
        g_NetPkt10_QSandEventSendBuf.eventFlags == 0x87650000u &&
        Vec3Equals(g_NetPkt10_QSandEventSendBuf.center, hostEvent.center) &&
        FloatNear(g_NetPkt10_QSandEventSendBuf.radius, 7.75f);

    g_NetPkt10_QSandEventSendBuf = oldPacket;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_zDEClientQSandNetRelayCallback = oldRelayCallback;
    g_zDEClient_QuickSandEventTemplateDefaults = oldDefaults;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;

    if (!negativeOk) {
        return 1;
    }
    if (!otherOwnerOk) {
        return 2;
    }
    if (!nonHostOk) {
        return 3;
    }
    return hostOk ? 0 : 4;
}

extern "C" int gamenet_host_send_pkt0f_crater_feature_smoke(void) {
    const NetPkt0F_CraterEvent oldPacket = g_NetPkt0F_CraterEventSendBuf;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHost = g_zNetwork_IsHostFlag;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    zModel_MaterialSlot *const oldMatlPool = g_zModel_MatlPool;
    const int oldMatlCapacity = g_zModel_MatlPoolCapacity;
    const int oldMatlInUse = g_zModel_MatlPoolInUseCount;

    zModel_MaterialSlot materialSlots[4]{};
    g_zModel_MatlPool = materialSlots;
    g_zModel_MatlPoolCapacity = 4;
    g_zModel_MatlPoolInUseCount = 4;

    zDEClient_CraterEventTemplate eventTemplate{};
    eventTemplate.craterMaterialSlot = &materialSlots[2];
    eventTemplate.radius = 6.25f;
    eventTemplate.center = {3.0f, 4.0f, 5.0f};

    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;
    const int nonHostResult = GameNet::HostSendPkt0F_CraterFeature(&eventTemplate);
    const bool nonHostOk = nonHostResult == 0 && g_sendCalls == 0;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x7777;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x23456789;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_NetPkt0F_CraterEventSendBuf = {{0x0f, sizeof(NetPkt0F_CraterEvent), 0}, 0x21u, -1,
                                     {0.0f, 0.0f, 0.0f}, 0.0f};
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    const int hostResult = GameNet::HostSendPkt0F_CraterFeature(&eventTemplate);
    const NetPkt0F_CraterEvent *const sentPacket =
        reinterpret_cast<const NetPkt0F_CraterEvent *>(g_sendPacketBytes);
    const bool hostOk =
        hostResult == 1 && g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt0F_CraterEventSendBuf.header &&
        g_sendPacketSize == sizeof(NetPkt0F_CraterEvent) &&
        g_sendPacketBytesSize == sizeof(NetPkt0F_CraterEvent) &&
        sentPacket->header.packetType == 0x0f &&
        sentPacket->header.packetSizeBytes == sizeof(NetPkt0F_CraterEvent) &&
        sentPacket->header.payloadDword0 == 0x23456789 &&
        sentPacket->eventFlags == (0x21u | 0x80u) && sentPacket->craterTypeId == 2 &&
        Vec3Equals(sentPacket->center, eventTemplate.center) &&
        FloatNear(sentPacket->radius, eventTemplate.radius);

    g_NetPkt0F_CraterEventSendBuf = oldPacket;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_zModel_MatlPool = oldMatlPool;
    g_zModel_MatlPoolCapacity = oldMatlCapacity;
    g_zModel_MatlPoolInUseCount = oldMatlInUse;

    return nonHostOk && hostOk ? 0 : 1;
}

extern "C" int gamenet_send_pkt13_effect_anim_activation_record_smoke(void) {
    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    const int oldSuppressEcho = g_GameNetSuppressPkt13ActivationEcho;

    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x12345678;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x12345678;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_GameNetSuppressPkt13ActivationEcho = 0;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    zEffectAnimActivationRecord record{};
    std::memset(&record, 0xab, sizeof(record));
    record.commandType = 2;
    GameNet::SendPkt13_EffectAnimActivationRecord(&record);

    const zNetworkPacketHeader *const header =
        reinterpret_cast<const zNetworkPacketHeader *>(g_sendPacketBytes);
    const bool sentOk = g_sendCalls == 1 && g_sendFlags == 1 &&
                        g_sendPacketSize == sizeof(zNetworkPacketHeader) + 0x48 &&
                        header->packetType == 0x13 &&
                        header->packetSizeBytes == sizeof(zNetworkPacketHeader) + 0x48 &&
                        header->payloadDword0 == 0x12345678 &&
                        std::memcmp(g_sendPacketBytes + sizeof(zNetworkPacketHeader), &record,
                                    0x48) == 0;

    g_GameNetSuppressPkt13ActivationEcho = 1;
    g_sendCalls = 0;
    GameNet::SendPkt13_EffectAnimActivationRecord(&record);
    const bool suppressOk = g_sendCalls == 0;

    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_GameNetSuppressPkt13ActivationEcho = oldSuppressEcho;

    return sentOk && suppressOk ? 0 : 1;
}

extern "C" int gamenet_handle_pkt13_effect_anim_activation_record_smoke(void) {
    struct Packet13 {
        zNetworkPacketHeader header;
        zEffectAnimActivationRecord record;
    };

    zEffectAnimActivationRecord *const oldRecordTable = g_zEffectAnim_ActivationRecordTable;
    const int oldRecordCount = g_zEffectAnim_ActivationRecordCount;
    const int oldSuppressEcho = g_GameNetSuppressPkt13ActivationEcho;

    Packet13 packet = {};
    packet.header.packetType = 0x13;
    packet.header.packetSizeBytes = sizeof(Packet13);
    packet.record.commandType = 2;
    std::strcpy(packet.record.animName, "missing_pkt13_activation");
    packet.record.nodeToken = 77;

    g_zEffectAnim_ActivationRecordTable = nullptr;
    g_zEffectAnim_ActivationRecordCount = 0;
    g_GameNetSuppressPkt13ActivationEcho = 0;
    const int missingResult = GameNet::HandlePkt13_EffectAnimActivationRecord(0, &packet.header);
    const bool missingOk = missingResult == 1 && g_GameNetSuppressPkt13ActivationEcho == 0;

    g_zEffectAnim_ActivationRecordTable = &packet.record;
    g_zEffectAnim_ActivationRecordCount = 1;
    const int duplicateResult = GameNet::HandlePkt13_EffectAnimActivationRecord(0, &packet.header);
    const bool duplicateOk = duplicateResult == 1 && g_GameNetSuppressPkt13ActivationEcho == 0;

    g_zEffectAnim_ActivationRecordTable = oldRecordTable;
    g_zEffectAnim_ActivationRecordCount = oldRecordCount;
    g_GameNetSuppressPkt13ActivationEcho = oldSuppressEcho;

    return missingOk && duplicateOk ? 0 : 1;
}

extern "C" int gamenet_send_all_pkt13_effect_anim_activation_records_smoke(void) {
    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHost = g_zNetwork_IsHostFlag;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    zEffectAnimActivationRecord *const oldRecordTable = g_zEffectAnim_ActivationRecordTable;
    const int oldRecordCount = g_zEffectAnim_ActivationRecordCount;

    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x13572468;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = localPlayer.playerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;

    zEffectAnimActivationRecord records[2] = {};
    std::memset(&records[0], 0x11, sizeof(records[0]));
    std::memset(&records[1], 0x22, sizeof(records[1]));
    records[0].commandType = 1;
    records[1].commandType = 3;
    g_zEffectAnim_ActivationRecordTable = records;
    g_zEffectAnim_ActivationRecordCount = 2;

    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;
    GameNet::SendAllPkt13_EffectAnimActivationRecords();
    const bool nonHostOk = g_sendCalls == 0;

    g_zNetwork_IsHostFlag = 1;
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    GameNet::SendAllPkt13_EffectAnimActivationRecords();
    const zNetworkPacketHeader *const header =
        reinterpret_cast<const zNetworkPacketHeader *>(g_sendPacketBytes);
    const bool hostOk = g_sendCalls == 2 &&
                        g_sendPacketSize == sizeof(zNetworkPacketHeader) + 0x4c &&
                        header->packetType == 0x13 &&
                        header->payloadDword0 == localPlayer.playerKey &&
                        std::memcmp(g_sendPacketBytes + sizeof(zNetworkPacketHeader), &records[1],
                                    0x4c) == 0;

    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_zEffectAnim_ActivationRecordTable = oldRecordTable;
    g_zEffectAnim_ActivationRecordCount = oldRecordCount;

    return nonHostOk && hostOk ? 0 : 1;
}

extern "C" int gamenet_player_row_destroy_embedded_panel_smoke(void) {
    GameNetPlayerRow row{};
    row.hudWidget.vtbl = &g_HudUiPanel_FTable;

    row.DestroyEmbeddedPanel();
    return row.hudWidget.vtbl == &g_HudUiCommon_FTable && row.hudWidget.textPick == nullptr ? 0 : 1;
}

extern "C" int gamenet_reset_remote_players_and_spawn_lists_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    const unsigned int oldSpawnList = g_GameNetSpawnPointList;
    GameNetSpawnPoint *const oldSpawnHead = g_GameNetSpawnPointHead;
    GameNetSpawnPoint *const oldSpawnTail = g_GameNetSpawnPointTail;
    const unsigned int oldSpawnCount = g_GameNetSpawnPointCount;
    const unsigned int oldRowList = g_GameNetPlayerRowList;
    GameNetPlayerRow *const oldRowHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldRowTail = g_GameNetPlayerRowTail;
    const unsigned int oldRowCount = g_GameNetPlayerRowCount;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    HudUiTopMessageStack topStack{};
    topStack.base.ConstructorDefault();
    g_HudUiTopMessageStack = &topStack;

    GameNetPlayerRow *const firstRow = new GameNetPlayerRow{};
    GameNetPlayerRow *const secondRow = new GameNetPlayerRow{};
    GameNetSpawnPoint *const firstSpawn = new GameNetSpawnPoint{};
    GameNetSpawnPoint *const secondSpawn = new GameNetSpawnPoint{};

    firstRow->playerKey = 0x1201;
    firstRow->playerColorPackedRgb = 0x00112233;
    std::strcpy(firstRow->displayName, "First");
    firstRow->hudWidget.vtbl = &g_HudUiPanel_FTable;
    firstRow->next = secondRow;

    secondRow->playerKey = 0x1202;
    secondRow->playerColorPackedRgb = 0x00445566;
    std::strcpy(secondRow->displayName, "Second");
    secondRow->hudWidget.vtbl = &g_HudUiPanel_FTable;

    firstSpawn->next = secondSpawn;

    g_GameNetPlayerRowList = 1;
    g_GameNetPlayerRowHead = firstRow;
    g_GameNetPlayerRowTail = secondRow;
    g_GameNetPlayerRowCount = 2;
    g_GameNetSpawnPointList = 1;
    g_GameNetSpawnPointHead = firstSpawn;
    g_GameNetSpawnPointTail = secondSpawn;
    g_GameNetSpawnPointCount = 2;

    triplet.AddEntry(firstRow);
    triplet.AddEntry(secondRow);
    topStack.base.AddChild(reinterpret_cast<HudUiElement *>(&firstRow->hudWidget));
    topStack.base.AddChild(reinterpret_cast<HudUiElement *>(&secondRow->hudWidget));

    GameNet::ResetRemotePlayersAndSpawnLists();

    const bool listsCleared =
        g_GameNetPlayerRowList == 0 && g_GameNetPlayerRowHead == nullptr &&
        g_GameNetPlayerRowTail == nullptr && g_GameNetPlayerRowCount == 0 &&
        g_GameNetSpawnPointList == 0 && g_GameNetSpawnPointHead == nullptr &&
        g_GameNetSpawnPointTail == nullptr && g_GameNetSpawnPointCount == 0;
    const bool hudCleared =
        topStack.base.childHead == nullptr && topStack.base.childTail == nullptr &&
        triplet.entries.begin != nullptr && triplet.entries.end == triplet.entries.begin;

    g_HudUiMgrStatsList = oldStatsList;
    g_HudUiTopMessageStack = oldTopStack;
    g_GameNetSpawnPointList = oldSpawnList;
    g_GameNetSpawnPointHead = oldSpawnHead;
    g_GameNetSpawnPointTail = oldSpawnTail;
    g_GameNetSpawnPointCount = oldSpawnCount;
    g_GameNetPlayerRowList = oldRowList;
    g_GameNetPlayerRowHead = oldRowHead;
    g_GameNetPlayerRowTail = oldRowTail;
    g_GameNetPlayerRowCount = oldRowCount;
    triplet.DestructorCore();

    return listsCleared && hudCleared ? 0 : 1;
}

extern "C" int gamenet_handle_pkt14_hud_timer_and_flags_sync_smoke(void) {
    const RecoilApp oldApp = g_RecoilApp;
    const int oldRuntimeGoalValue = g_HudSensorTracker.runtimeGoalValue;
    const int oldRuntimeTimerSecRaw = g_HudSensorTracker.runtimeTimerSecRaw;
    const int oldMissionId = g_HudSensorTracker.missionId;
    const int oldMissionFlags = g_HudSensorTracker.missionFlags;
    const int oldAllowMaps = g_GameNetStatus_AllowMaps;
    const int oldNameTags = g_GameNetStatus_NameTags;
    const int oldHandlersRegistered = g_GameNet_HandlersRegistered;
    zNetworkDispatchHandlerListNode *const oldSentinel = g_zNetwork_DispatchHandlerListSentinel;
    const int oldHandlerCount = g_zNetwork_DispatchHandlerListCount;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetworkDPlaySessionDescCache *const oldSession = g_zNetwork_CurrentSessionDescCache;
    const int oldIsHost = g_zNetwork_IsHostFlag;

    std::memset(&g_RecoilApp, 0, sizeof(g_RecoilApp));
    CZRecoilFrame mainWnd{};
    mainWnd.m_useArchiveBanks = 77;
    g_RecoilApp.m_pMainWnd = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&mainWnd));
    g_RecoilApp.m_currentStateIndex_0c8 = -1;
    g_RecoilApp.m_introFmvState_1a0.base.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_pkt14StateVtable));
    g_RecoilApp.m_missionFmvState_1d8.m_missionId = 99;

    g_HudSensorTracker.runtimeGoalValue = 0;
    g_HudSensorTracker.runtimeTimerSecRaw = 0;
    g_HudSensorTracker.missionId = 0;
    g_HudSensorTracker.missionFlags = 0;
    g_GameNetStatus_AllowMaps = 0;
    g_GameNetStatus_NameTags = 0;
    g_GameNet_HandlersRegistered = 1;
    g_pkt14StateEnterCount = 0;

    zNetworkDispatchHandlerListNode sentinel{};
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    g_zNetwork_DispatchHandlerListSentinel = &sentinel;
    g_zNetwork_DispatchHandlerListCount = 0;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetworkDPlaySessionDescCache session{};
    char sessionName[0x5c] = "pkt14";
    session.desc.sessionName = sessionName;
    session.desc.maxPlayers = 8;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_CurrentSessionDescCache = &session;
    g_zNetwork_IsHostFlag = 1;
    g_setSessionDescCalls = 0;
    g_setSessionDescResult = 0;

    NetPkt14_HudTimerAndFlagsSync packet{};
    packet.header.packetType = 0x14;
    packet.header.packetSizeBytes = sizeof(packet);
    packet.eventCode = 4;
    packet.auxParam = 12;
    packet.valueOrTime = 3;
    packet.statusFlags = 3;

    const int result = GameNet::HandlePkt14_HudTimerAndFlagsSync(0x2222, &packet);

    union TimerSecondsBits {
        float seconds;
        int raw;
    } expectedTimer = {180.0f};
    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue_118;
    bool queuedIntro = false;
    if (queue.m_itemCount == 1) {
        const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
        RecoilPtr32 *const slot =
            reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
        RecoilApp_StateQueueItem *const item =
            reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
        queuedIntro = item->m_kind == RecoilApp_StateQueueKind_SwitchCurrent &&
                      item->m_stateObj == static_cast<RecoilPtr32>(
                                             reinterpret_cast<std::uintptr_t>(
                                                 &g_RecoilApp.m_introFmvState_1a0.base)) &&
                      item->m_param == 0;
        CleanupSingleQueuedItem(queue);
    }

    int failure = 0;
    if (result != 1) {
        failure = 1;
    } else if (g_GameNet_HandlersRegistered != 0) {
        failure = 2;
    } else if (g_HudSensorTracker.runtimeTimerSecRaw != expectedTimer.raw ||
               g_HudSensorTracker.runtimeGoalValue != 12) {
        failure = 3;
    } else if (g_HudSensorTracker.missionId != 10 || g_HudSensorTracker.missionFlags != 77) {
        failure = 4;
    } else if (g_GameNetStatus_AllowMaps != 1 || g_GameNetStatus_NameTags != 1) {
        failure = 5;
    } else if (g_RecoilApp.m_missionFmvState_1d8.m_missionId != 0 ||
               g_pkt14StateEnterCount != 1 || !queuedIntro) {
        failure = 6;
    } else if (g_setSessionDescCalls != 1 || session.desc.customEventCode != 4 ||
               session.desc.customAuxParam != 12 || session.desc.customValueOrTime != 3 ||
               session.desc.customStatusFlags != 3) {
        failure = 7;
    } else if (session.desc.maxPlayers != 8 || std::strcmp(sessionName, "pkt14") != 0) {
        failure = 8;
    }

    g_RecoilApp = oldApp;
    g_HudSensorTracker.runtimeGoalValue = oldRuntimeGoalValue;
    g_HudSensorTracker.runtimeTimerSecRaw = oldRuntimeTimerSecRaw;
    g_HudSensorTracker.missionId = oldMissionId;
    g_HudSensorTracker.missionFlags = oldMissionFlags;
    g_GameNetStatus_AllowMaps = oldAllowMaps;
    g_GameNetStatus_NameTags = oldNameTags;
    g_GameNet_HandlersRegistered = oldHandlersRegistered;
    g_zNetwork_DispatchHandlerListSentinel = oldSentinel;
    g_zNetwork_DispatchHandlerListCount = oldHandlerCount;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_CurrentSessionDescCache = oldSession;
    g_zNetwork_IsHostFlag = oldIsHost;

    return failure;
}

extern "C" int gamenet_handle_pkt03_remove_remote_player_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    const unsigned int oldRowList = g_GameNetPlayerRowList;
    GameNetPlayerRow *const oldRowHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldRowTail = g_GameNetPlayerRowTail;
    const unsigned int oldRowCount = g_GameNetPlayerRowCount;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    void *const oldFreeRuntimeList = g_OptCatalogFreeRuntimeInstanceList;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    HudUiTopMessageStack topStack{};
    topStack.base.ConstructorDefault();
    topStack.base.enabled = 0;
    g_HudUiTopMessageStack = &topStack;

    HudUiPanel_FTable panelTable = {};
    panelTable.slots[0x60 / 4] = MethodAddress(&TestRemoteHudPanelOps::SetVisible);
    g_remoteHudSetVisibleCount = 0;
    g_remoteHudLastVisible = 7;

    GameNetPlayerRow first{};
    first.playerKey = 0x3101;
    std::strcpy(first.displayName, "First");
    first.hudWidget.vtbl = &panelTable;

    GameNetPlayerRow *const removed = new GameNetPlayerRow{};
    removed->playerKey = 0x3102;
    removed->playerColorPackedRgb = 0x00123456;
    std::strcpy(removed->displayName, "Removed");
    removed->hudWidget.vtbl = &panelTable;
    first.next = removed;

    zUtil_SaveGameState saveState{};
    zUtil_PlayerStateStorage playerState{};
    saveState.playerState = &playerState;
    removed->saveState = (GameNetPlayerSaveState *)&saveState;
    playerState.lifecycleState = 3;
    playerState.cameraTransitionTimer = 0;
    playerState.activeAltGunController = &playerState.altWeaponBanks[2].controllerA;

    zClass_NodePartial rootNode{};
    playerState.rootNode = &rootNode;
    zClass_NodePartial runtimeWorld{};
    runtimeWorld.classId = 3;
    zClass_NodeFreeListSlot projectile{};
    zClass_Object3DDataPartial projectileData{};
    projectile.node.classId = 5;
    projectile.node.classData = &projectileData;
    zClass_NodePartial *worldChildren[1] = {&projectile.node};
    runtimeWorld.listB = worldChildren;
    runtimeWorld.listCountB = 1;

    OptCatalogEntryDef mineEntry{};
    OptCatalogRuntimeInstanceStorage mineRuntime{};
    mineRuntime.ownerNode = &rootNode;
    mineRuntime.projectileNode = &projectile.node;
    mineRuntime.lifetime = 0.0f;
    mineEntry.activeRuntimeListHead = &mineRuntime;
    playerState.altWeaponBanks[4].controllerA.optCatalogEntry = &mineEntry;

    OptCatalogRuntimeInstanceStorage freeSentinel{};
    g_OptCatalogRuntimeWorld = &runtimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &freeSentinel;

    g_GameNetPlayerRowList = 1;
    g_GameNetPlayerRowHead = &first;
    g_GameNetPlayerRowTail = removed;
    g_GameNetPlayerRowCount = 2;

    triplet.AddEntry(&first);
    triplet.AddEntry(removed);
    topStack.base.AddChild(reinterpret_cast<HudUiElement *>(&first.hudWidget));
    topStack.base.AddChild(reinterpret_cast<HudUiElement *>(&removed->hudWidget));

    const int result = GameNet::HandlePkt03_RemoveRemotePlayer(removed->playerKey, nullptr);

    const bool playerStateOk =
        result == 0 && playerState.cameraTransitionTimer == 1 && playerState.lifecycleState == 4;
    const bool mineOk = mineEntry.activeRuntimeListHead == nullptr &&
                        g_OptCatalogFreeRuntimeInstanceList == &mineRuntime &&
                        mineRuntime.next == &freeSentinel;
    const bool rowListOk = g_GameNetPlayerRowHead == &first && g_GameNetPlayerRowTail == &first &&
                           g_GameNetPlayerRowCount == 1 && first.next == nullptr;
    const bool hudOk = g_remoteHudSetVisibleCount == 1 && g_remoteHudLastVisible == 0 &&
                       topStack.base.childHead == reinterpret_cast<HudUiElement *>(&first.hudWidget) &&
                       topStack.base.childTail == reinterpret_cast<HudUiElement *>(&first.hudWidget) &&
                       triplet.entries.begin != nullptr && triplet.entries.end == triplet.entries.begin + 1 &&
                       triplet.entries.begin[0].playerKey == first.playerKey;

    g_HudUiMgrStatsList = oldStatsList;
    g_HudUiTopMessageStack = oldTopStack;
    g_GameNetPlayerRowList = oldRowList;
    g_GameNetPlayerRowHead = oldRowHead;
    g_GameNetPlayerRowTail = oldRowTail;
    g_GameNetPlayerRowCount = oldRowCount;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    triplet.DestructorCore();

    if (!playerStateOk) {
        return 1;
    }
    if (!mineOk) {
        return 2;
    }
    if (!rowListOk) {
        return 3;
    }
    return hudOk ? 0 : 4;
}
