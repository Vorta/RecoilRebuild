#include "Battlesport/GameNet.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "GameZRecoil/include/zClipRect.h"

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

struct ScoreboardPacket2 {
    zNetworkPacketHeader header;
    std::int32_t entryCount;
    NetPkt09_PlayerScoreboardEntry entries[2];
};

template <typename T> T &FieldAt(void *object, std::size_t offset) {
    return *reinterpret_cast<T *>(static_cast<unsigned char *>(object) + offset);
}

template <typename Method> unsigned int MethodAddress(Method method) {
    union {
        Method method;
        unsigned int address;
    } value = {method};
    return value.address;
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

const zNetwork_DPlay4Vtable kDPlayVtable = {
    {}, nullptr, {}, SendFake, {}, SetSessionDescFake, {}, nullptr, {}, nullptr,
};
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

extern "C" int gamenet_player_row_destroy_embedded_panel_smoke(void) {
    GameNetPlayerRow row{};
    row.hudWidget.vtbl = &g_HudUiPanel_FTable;

    row.DestroyEmbeddedPanel();
    return row.hudWidget.vtbl == &g_HudUiCommon_FTable && row.hudWidget.textPick == nullptr ? 0 : 1;
}
