#include "Battlesport/game_net.h"
#include "Battlesport/cz_recoil_frame.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/recoil_app.h"
#include "Battlesport/player.h"
#include "Battlesport/pickup.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "GameZRecoil/include/zclip_rect.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <type_traits>

extern "C" NetPkt10_QSandEvent g_NetPkt10_QSandEventRelayBuf;

namespace {
int g_setSessionDescCalls;
HRESULT g_setSessionDescResult;
int g_sendCalls;
DWORD g_sendFlags;
void *g_sendPacket;
DWORD g_sendPacketSize;
DWORD g_sendPacketBytesSize;
unsigned char g_sendPacketBytes[0x200];
int g_qsandRelayCallbackCount;
int g_qsandRelayCallbackResult;
int g_remoteHudSetVisibleCount;
int g_remoteHudLastVisible;
int g_remoteHudSetPosCount;
HudUiPanel *g_remoteHudSetPosThis;
int g_remoteHudLastX;
int g_remoteHudLastY;
int g_chatComposeSetTextFmtCalls;
HudUiPanel *g_chatComposeSetTextFmtThis;
char g_chatComposeSetTextFmtText[32];

HRESULT __stdcall SetSessionDescFake(
    zNetwork_DPlay4 *,
    zNetworkDPlaySessionDesc *,
    DWORD
) {
    ++g_setSessionDescCalls;
    return g_setSessionDescResult;
}

HRESULT __stdcall SendFake(
    zNetwork_DPlay4 *,
    DWORD,
    DWORD,
    DWORD flags,
    void *packet,
    DWORD packetSizeBytes
) {
    ++g_sendCalls;
    g_sendFlags = flags;
    g_sendPacket = packet;
    g_sendPacketSize = packetSizeBytes;
    g_sendPacketBytesSize = packetSizeBytes;
    if (packetSizeBytes <= sizeof(g_sendPacketBytes)) {
        std::memcpy(
            g_sendPacketBytes,
            packet,
            packetSizeBytes
        );
    }
    return 0;
}

HRESULT __stdcall SendExFake(
    zNetwork_DPlay4 *,
    DWORD,
    DWORD,
    DWORD flags,
    void *packet,
    DWORD packetSizeBytes,
    DWORD,
    DWORD,
    void *,
    DWORD *asyncHandle
) {
    ++g_sendCalls;
    g_sendFlags = flags;
    g_sendPacket = packet;
    g_sendPacketSize = packetSizeBytes;
    g_sendPacketBytesSize = packetSizeBytes;
    if (packetSizeBytes <= sizeof(g_sendPacketBytes)) {
        std::memcpy(
            g_sendPacketBytes,
            packet,
            packetSizeBytes
        );
    }
    if (asyncHandle != 0) {
        *asyncHandle = 0x2468;
    }
    return 0;
}

struct FakeDirectPlay4 {
    void **vtable;
};

struct ChatComposePanelFake : HudUiPanel {
    virtual void SetTextFmt(
        const char *format,
        ...
    );
};

void ChatComposePanelFake::SetTextFmt(
    const char *format,
    ...
) {
    ++g_chatComposeSetTextFmtCalls;
    g_chatComposeSetTextFmtThis = this;
    std::strncpy(
        g_chatComposeSetTextFmtText,
        format != 0 ? format : "",
        sizeof(g_chatComposeSetTextFmtText)
    );
    g_chatComposeSetTextFmtText[sizeof(g_chatComposeSetTextFmtText) - 1] = '\0';
}

struct ScoreboardPacket2 {
    zNetworkPacketHeader header;
    std::int32_t entryCount;
    NetPkt09_PlayerScoreboardEntry entries[2];
};

void InitDirectPlayVtable(
    void **vtable
) {
    std::memset(
        vtable,
        0,
        sizeof(void *) * 52
    );
    vtable[26] = (void *)(&SendFake);
    vtable[31] = (void *)(&SetSessionDescFake);
    vtable[49] = (void *)(&SendExFake);
}

void ClearDispatchHandlerListForTest(
    zNetworkDispatchHandlerListNode &sentinel
) {
    zNetworkDispatchHandlerListNode *node = sentinel.next;
    while (node != &sentinel) {
        zNetworkDispatchHandlerListNode *const next = node->next;
        ::operator delete(node);
        node = next;
    }
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
}

bool ContainerHasChild(
    const HudUiContainer &container,
    const HudUiElement *child
) {
    const HudUiElement *node = container.childHead;
    while (node != 0) {
        if (node == child) {
            return true;
        }
        node = node->next;
    }
    return false;
}

template <typename Method> void *MethodAddress(
    Method method
) {
    union {
        Method method;
        void *address;
    } value = {method};
    return value.address;
}

template <typename T> T &FieldAt(
    void *object,
    std::size_t offset
) {
    return *(T *)((unsigned char *)object + offset);
}

bool FloatNear(
    float actual,
    float expected
) {
    return actual >= expected - 0.0001f && actual <= expected + 0.0001f;
}

bool Vec3Equals(
    const zVec3 &value,
    const zVec3 &expected
) {
    return FloatNear(
               value.x,
               expected.x
           ) &&
           FloatNear(
               value.y,
               expected.y
           ) &&
           FloatNear(
               value.z,
               expected.z
           );
}

void MakeGameNetReaderFloatNode(
    zReader::Node &node,
    float value
) {
    node.type = zReader::ZRDR_NODE_FLOAT;
    node.value.f32 = value;
}

void MakeGameNetReaderArrayNode(
    zReader::Node &node,
    zReader::Node *payload,
    int count
) {
    payload[0].type = zReader::ZRDR_NODE_INT;
    payload[0].value.i32 = count;
    node.type = zReader::ZRDR_NODE_ARRAY;
    node.value.nodes = payload;
}

bool WriteGameNetZrdU32(
    std::FILE *file,
    unsigned int value
) {
    return std::fwrite(
               &value,
               sizeof(value),
               1,
               file
           ) == 1;
}

bool WriteGameNetZrdNode(
    std::FILE *file,
    const zReader::Node &node
) {
    if (!WriteGameNetZrdU32(
            file,
            (unsigned int)(node.type)
        )) {
        return false;
    }

    switch (node.type) {
    case zReader::ZRDR_NODE_INT:
    case zReader::ZRDR_NODE_FLOAT:
        return WriteGameNetZrdU32(
            file,
            node.value.u32
        );
    case zReader::ZRDR_NODE_STRING: {
        const unsigned int length = (unsigned int)(std::strlen(node.value.str));
        return WriteGameNetZrdU32(
                   file,
                   length
               ) &&
               std::fwrite(
                   node.value.str,
                   1,
                   length,
                   file
               ) == length;
    }
    case zReader::ZRDR_NODE_ARRAY: {
        const int count = node.value.nodes[0].value.i32;
        if (!WriteGameNetZrdU32(
                file,
                (unsigned int)(count)
            )) {
            return false;
        }
        for (int index = 1; index < count; ++index) {
            if (!WriteGameNetZrdNode(
                    file,
                    node.value.nodes[index]
                )) {
                return false;
            }
        }
        return true;
    }
    }

    return false;
}

struct GameNetZrdArchiveEntry {
    const char *name;
    const zReader::Node *root;
};

bool MountGameNetZrdArchive(
    const char *path,
    const GameNetZrdArchiveEntry *entries,
    int entryCount,
    zIndexArchive &archive,
    zZarFileRecord *records,
    zArchiveListNode &archiveNode,
    zArchiveList &archiveList
) {
    std::FILE *const file = std::fopen(
        path,
        "wb"
    );
    if (file == 0) {
        return false;
    }

    bool ok = true;
    for (int index = 0; index < entryCount; ++index) {
        const long offset = std::ftell(file);
        if (offset < 0 ||
            !WriteGameNetZrdNode(
                file,
                *entries[index].root
            )) {
            ok = false;
            break;
        }
        const long endOffset = std::ftell(file);
        if (endOffset < offset) {
            ok = false;
            break;
        }

        records[index] = zZarFileRecord();
        records[index].fileOffset = (unsigned int)(offset);
        records[index].fileSize = (unsigned int)(endOffset - offset);
        std::strcpy(
            records[index].name,
            entries[index].name
        );
    }

    if (std::fclose(file) != 0 || !ok) {
        std::remove(path);
        return false;
    }

    archive = zIndexArchive();
    archive.hFile = CreateFileA(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        0
    );
    if (archive.hFile == INVALID_HANDLE_VALUE) {
        std::remove(path);
        return false;
    }

    archive.recordCount = (unsigned int)(entryCount);
    archive.records = records;

    archiveNode = zArchiveListNode();
    archiveNode.payload = &archive;
    archiveNode.next = &archiveNode;
    archiveNode.prev = &archiveNode;

    archiveList = zArchiveList();
    archiveList.count = 1;
    archiveList.head = &archiveNode;
    g_zArchive_MountedList = &archiveList;
    return true;
}

int __fastcall QSandRelayCallbackFake(
    void *
) {
    ++g_qsandRelayCallbackCount;
    return g_qsandRelayCallbackResult;
}

struct TestRemoteHudPanelOps {
    void SetPos(
        int x,
        int y
    ) {
        ++g_remoteHudSetPosCount;
        g_remoteHudSetPosThis = (HudUiPanel *)this;
        g_remoteHudLastX = x;
        g_remoteHudLastY = y;
    }

    void SetVisible(int visible) {
        ++g_remoteHudSetVisibleCount;
        g_remoteHudLastVisible = visible;
    }
};
} // namespace

extern "C" int gamenet_respawn_player_color_indexed_spawn_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    GameNetSpawnPoint *const oldSpawnHead = g_GameNetSpawnPointHead;
    GameNetSpawnPoint *const oldSpawnTail = g_GameNetSpawnPointTail;
    const unsigned int oldSpawnCount = g_GameNetSpawnPointCount;
    const int oldAllowMaps = g_GameNetStatus_AllowMaps;
    const int oldRaceCheckpointMode = g_HudSensorTracker.raceCheckpointMode;
    const int oldMissionId = g_HudSensorTracker.missionId;
    HWND const oldWindow = g_zInput_hWnd;
    const int oldCenterX = g_zInput_MouseClientCenterX;
    const int oldCenterY = g_zInput_MouseClientCenterY;
    const zInput::MouseStateSnapshot oldMouseState = g_zInput_MouseStateSnapshot;
    POINT originalCursor = {0};
    GetCursorPos(&originalCursor);

    HWND const hwnd = CreateWindowExA(
        0,
        "STATIC",
        "recoil",
        WS_POPUP,
        20,
        30,
        160,
        120,
        0,
        0,
        GetModuleHandleA(0),
        0
    );
    if (hwnd == 0) {
        return 1;
    }

    zUtil_SaveGameState saveState = zUtil_SaveGameState();
    zUtil_PlayerStateStorage playerState = zUtil_PlayerStateStorage();
    PlayerModalState modalState = PlayerModalState();
    PlayerMasterModalData modalData = PlayerMasterModalData();
    GameNetPlayerRow localRow = GameNetPlayerRow();
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    saveState.netPlayerRow = &localRow;
    modalState.masterModalData = &modalData;
    modalData.masterType = 3;
    localRow.playerColorIndex = 2;

    GameNetSpawnPoint firstSpawn = GameNetSpawnPoint();
    GameNetSpawnPoint secondSpawn = GameNetSpawnPoint();
    firstSpawn.position.x = 1.0f;
    firstSpawn.position.y = 2.0f;
    firstSpawn.position.z = 3.0f;
    firstSpawn.yawDegrees = 10.0f;
    firstSpawn.next = &secondSpawn;
    secondSpawn.position.x = 4.0f;
    secondSpawn.position.y = 5.0f;
    secondSpawn.position.z = 6.0f;
    secondSpawn.yawDegrees = 90.0f;
    g_GameNetSpawnPointHead = &firstSpawn;
    g_GameNetSpawnPointTail = &secondSpawn;
    g_GameNetSpawnPointCount = 2;
    g_GameNetStatus_AllowMaps = 0;
    g_HudSensorTracker.raceCheckpointMode = 0;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)(&saveState);
    g_HudSensorTracker.missionId = 11;

    playerState.throttleInput = 1.0f;
    playerState.steeringInput = 2.0f;
    playerState.subVerticalInput = 3.0f;
    playerState.throttleInputCopy = 4.0f;
    playerState.steeringInputCopy = 5.0f;
    playerState.subVerticalInputCopy = 6.0f;
    playerState.localVel.x = 7.0f;
    playerState.localVel.y = 8.0f;
    playerState.localVel.z = 9.0f;
    playerState.projectileSpawnVel.x = 10.0f;
    playerState.projectileSpawnVel.y = 11.0f;
    playerState.projectileSpawnVel.z = 12.0f;
    playerState.yawRotatedLocalVel.x = 13.0f;
    playerState.yawRotatedLocalVel.y = 14.0f;
    playerState.yawRotatedLocalVel.z = 15.0f;
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

    GameNet::RespawnPlayerAndDropWeaponPickupIfAllowed(
        &saveState,
        1
    );

    const zVec3 zero = {0.0f, 0.0f, 0.0f};
    const bool spawnOk = Vec3Equals(playerState.worldPos, secondSpawn.position) &&
                         FloatNear(playerState.restartYawRad, 1.5707964f) &&
                         FloatNear(playerState.previousTransform.posX, 4.0f) &&
                         FloatNear(playerState.previousTransform.posY, 5.0f) &&
                         FloatNear(playerState.previousTransform.posZ, 6.0f);
    const bool resetOk =
        playerState.thirdPersonYawOffset == 0.0f && playerState.cameraElevationOffset == 0.0f &&
        Vec3Equals(playerState.localVel, zero) &&
        Vec3Equals(playerState.projectileSpawnVel, zero) &&
        Vec3Equals(playerState.yawRotatedLocalVel, zero) &&
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
    g_HudSensorTracker.raceCheckpointMode = oldRaceCheckpointMode;
    g_HudSensorTracker.missionId = oldMissionId;
    g_zInput_hWnd = oldWindow;
    g_zInput_MouseClientCenterX = oldCenterX;
    g_zInput_MouseClientCenterY = oldCenterY;
    g_zInput_MouseStateSnapshot = oldMouseState;

    return spawnOk && resetOk && unlockOk ? 0 : 2;
}

extern "C" int gamenet_respawn_player_race_mode_no_spawn_smoke(void) {
    GameNetSpawnPoint *const oldSpawnHead = g_GameNetSpawnPointHead;
    GameNetSpawnPoint *const oldSpawnTail = g_GameNetSpawnPointTail;
    const unsigned int oldSpawnCount = g_GameNetSpawnPointCount;
    const int oldAllowMaps = g_GameNetStatus_AllowMaps;
    const int oldRaceCheckpointMode = g_HudSensorTracker.raceCheckpointMode;
    const int oldMissionId = g_HudSensorTracker.missionId;
    HWND const oldWindow = g_zInput_hWnd;
    const int oldCenterX = g_zInput_MouseClientCenterX;
    const int oldCenterY = g_zInput_MouseClientCenterY;
    const zInput::MouseStateSnapshot oldMouseState = g_zInput_MouseStateSnapshot;
    POINT originalCursor = {0};
    GetCursorPos(&originalCursor);

    HWND const hwnd = CreateWindowExA(
        0,
        "STATIC",
        "recoil",
        WS_POPUP,
        20,
        30,
        160,
        120,
        0,
        0,
        GetModuleHandleA(0),
        0
    );
    if (hwnd == 0) {
        return 1;
    }

    zUtil_SaveGameState saveState = zUtil_SaveGameState();
    zUtil_PlayerStateStorage playerState = zUtil_PlayerStateStorage();
    PlayerModalState modalState = PlayerModalState();
    PlayerMasterModalData modalData = PlayerMasterModalData();
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalData.masterType = 3;

    GameNetSpawnPoint spawn = GameNetSpawnPoint();
    spawn.position.x = 12.0f;
    spawn.position.y = 13.0f;
    spawn.position.z = 14.0f;
    spawn.yawDegrees = 45.0f;
    g_GameNetSpawnPointHead = &spawn;
    g_GameNetSpawnPointTail = &spawn;
    g_GameNetSpawnPointCount = 1;
    g_GameNetStatus_AllowMaps = 0;
    g_HudSensorTracker.raceCheckpointMode = 1;
    g_HudSensorTracker.missionId = 11;

    playerState.worldPos.x = 1.0f;
    playerState.worldPos.y = 2.0f;
    playerState.worldPos.z = 3.0f;
    playerState.restartYawRad = 0.25f;
    playerState.localVel.x = 7.0f;
    playerState.localVel.y = 8.0f;
    playerState.localVel.z = 9.0f;
    playerState.amphibUnlocked = 0;
    playerState.hoverUnlocked = 1;
    playerState.subUnlocked = 1;

    g_zInput_hWnd = hwnd;
    g_zInput_MouseClientCenterX = 32;
    g_zInput_MouseClientCenterY = 24;
    g_zInput_MouseStateSnapshot.cursorClientX = 4;
    g_zInput_MouseStateSnapshot.cursorClientY = 5;

    GameNet::RespawnPlayerAndDropWeaponPickupIfAllowed(
        &saveState,
        0
    );

    const zVec3 original = {1.0f, 2.0f, 3.0f};
    const zVec3 zero = {0.0f, 0.0f, 0.0f};
    const bool spawnSkippedOk =
        Vec3Equals(playerState.worldPos, original) &&
        FloatNear(playerState.restartYawRad, 0.25f);
    const bool resetOk =
        Vec3Equals(playerState.localVel, zero) &&
        playerState.amphibUnlocked == 1 && playerState.hoverUnlocked == 0 &&
        playerState.subUnlocked == 0;

    DestroyWindow(hwnd);
    SetCursorPos(originalCursor.x, originalCursor.y);
    g_GameNetSpawnPointHead = oldSpawnHead;
    g_GameNetSpawnPointTail = oldSpawnTail;
    g_GameNetSpawnPointCount = oldSpawnCount;
    g_GameNetStatus_AllowMaps = oldAllowMaps;
    g_HudSensorTracker.raceCheckpointMode = oldRaceCheckpointMode;
    g_HudSensorTracker.missionId = oldMissionId;
    g_zInput_hWnd = oldWindow;
    g_zInput_MouseClientCenterX = oldCenterX;
    g_zInput_MouseClientCenterY = oldCenterY;
    g_zInput_MouseStateSnapshot = oldMouseState;

    return spawnSkippedOk && resetOk ? 0 : 2;
}

extern "C" int gamenet_reset_hud_timer_panel_net_state_smoke(void) {
    HudUiTimerPanel timer = HudUiTimerPanel();
    HudUiPanel *const panel = (HudUiPanel *)(&timer);
    panel->ConstructorDefault(
        "",
        0,
        0
    );
    g_HudUiMgrTimerPanel = &timer;

    g_HudTimerPanelNetState.timerSeconds = 1.0f;
    g_HudTimerPanelNetState.timeWarningThresholdSec = 2.0f;
    g_HudTimerPanelNetState.timerDirectionNeg = 0;
    g_HudTimerPanelNetState.startGateTriggered = 1;
    g_HudTimerPanelNetState.raceFinishCountdownTriggered = 1;
    g_HudTimerPanelNetState.startCountdownTriggered = 1;
    g_HudTimerPanelNetState.tenSecondWarningsEnabled = 1;
    for (int index = 0; index < 8; ++index) {
        g_HudTimerPanelNetState.tailFlags[index] = 0xffffffffu;
    }
    g_GameNetOneLapLeftMessageShown = 1;
    g_GameNetAllPlayersLapTargetCheckStarted = 1;

    GameNet::ResetHudTimerPanelNetStateLongCountdown();

    bool tailsCleared = true;
    for (int index = 0; index < 8; ++index) {
        tailsCleared = tailsCleared && g_HudTimerPanelNetState.tailFlags[index] == 0;
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
                    FieldAt<int>(&timer, 0x2ac) == -1 &&
                    std::strcmp(&FieldAt<char>(&timer, 0x34), "10:00:00") == 0;

    DeleteObject(panel->hFont);
    panel->hFont = 0;
    g_HudUiMgrTimerPanel = 0;
    return ok ? 0 : 1;
}

extern "C" int gamenet_get_local_player_color_index_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;

    g_GameStateOrMapTable = 0;
    const bool nullStateOk = GameNet::GetLocalPlayerColorIndexOrZero() == 0;

    zUtil_SaveGameState saveState = zUtil_SaveGameState();
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)(&saveState);
    const bool nullRowOk = GameNet::GetLocalPlayerColorIndexOrZero() == 0;

    GameNetPlayerRow row = GameNetPlayerRow();
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
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    GameNetSpawnPoint spawnPoint = GameNetSpawnPoint();
    spawnPoint.position.x = 0.0f;
    spawnPoint.position.y = 0.0f;
    spawnPoint.position.z = 0.0f;

    zUtil_PlayerStateStorage localPlayerState = zUtil_PlayerStateStorage();
    zUtil_PlayerStateStorage farPlayerState = zUtil_PlayerStateStorage();
    zUtil_PlayerStateStorage nearPlayerState = zUtil_PlayerStateStorage();
    localPlayerState.worldPos.x = 1.0f;
    farPlayerState.worldPos.x = 5.0f;
    nearPlayerState.worldPos.x = 2.0f;

    GameNetPlayerSaveState localSave = GameNetPlayerSaveState();
    GameNetPlayerSaveState farSave = GameNetPlayerSaveState();
    GameNetPlayerSaveState nearSave = GameNetPlayerSaveState();
    localSave.playerState = &localPlayerState;
    farSave.playerState = &farPlayerState;
    nearSave.playerState = &nearPlayerState;

    GameNetPlayerRow localRow = GameNetPlayerRow();
    GameNetPlayerRow farRow = GameNetPlayerRow();
    GameNetPlayerRow nearRow = GameNetPlayerRow();
    localRow.saveState = &localSave;
    localRow.next = &farRow;
    farRow.saveState = &farSave;
    farRow.next = &nearRow;
    nearRow.saveState = &nearSave;

    g_GameNetPlayerRowHead = &localRow;
    g_GameNetPlayerRowTail = &nearRow;
    g_GameNetPlayerRowCount = 3;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)(&localSave);

    GameNetPlayerSaveState *nearest = &localSave;
    const float nearestDistance =
        GameNet::GetNearestOtherPlayerDistanceToSpawnPoint(&spawnPoint, &nearest);
    const bool nearestOk = nearestDistance == 4.0f && nearest == &nearSave;

    g_GameNetPlayerRowHead = 0;
    nearest = &localSave;
    const float emptyDistance =
        GameNet::GetNearestOtherPlayerDistanceToSpawnPoint(&spawnPoint, &nearest);
    const bool emptyOk = emptyDistance > 9.0e22f && nearest == &localSave;

    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    return nearestOk && emptyOk ? 0 : 1;
}

extern "C" int gamenet_wait_for_local_player_color_index_smoke(void) {
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldSessionRuntime = g_zNetwork_SessionRuntimeInitialized;

    g_zNetwork_LocalPlayerRecord = 0;
    const bool noWaitOk = GameNet::WaitForLocalPlayerColorIndex(0) == 0;

    zNetwork_PlayerRecord localPlayer = zNetwork_PlayerRecord();
    localPlayer.colorIndex = 6;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_SessionRuntimeInitialized = 0;
    const bool colorOk = GameNet::WaitForLocalPlayerColorIndex(1) == 6;

    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_SessionRuntimeInitialized = oldSessionRuntime;
    return noWaitOk && colorOk ? 0 : 1;
}

extern "C" int net_init_from_zrd_smoke(void) {
    char oldDir[MAX_PATH] = {0};
    if (GetCurrentDirectoryA(sizeof(oldDir), oldDir) == 0) {
        return 1;
    }

    char tempRoot[MAX_PATH] = {0};
    if (GetTempPathA(sizeof(tempRoot), tempRoot) == 0) {
        return 2;
    }

    char tempDir[MAX_PATH] = {0};
    std::sprintf(
        tempDir,
        "%srecoil_gamenet_init",
        tempRoot
    );
    CreateDirectoryA(
        tempDir,
        0
    );
    if (SetCurrentDirectoryA(tempDir) == 0) {
        return 3;
    }

    zReader::Node spawn0Values[5] = {0};
    MakeGameNetReaderFloatNode(spawn0Values[1], 1.0f);
    MakeGameNetReaderFloatNode(spawn0Values[2], 2.0f);
    MakeGameNetReaderFloatNode(spawn0Values[3], 3.0f);
    MakeGameNetReaderFloatNode(spawn0Values[4], 10.0f);
    zReader::Node spawn0 = zReader::Node();
    MakeGameNetReaderArrayNode(spawn0, spawn0Values, 5);

    zReader::Node spawn1Values[5] = {0};
    MakeGameNetReaderFloatNode(spawn1Values[1], 4.0f);
    MakeGameNetReaderFloatNode(spawn1Values[2], 5.0f);
    MakeGameNetReaderFloatNode(spawn1Values[3], 6.0f);
    MakeGameNetReaderFloatNode(spawn1Values[4], 90.0f);
    zReader::Node spawn1 = zReader::Node();
    MakeGameNetReaderArrayNode(spawn1, spawn1Values, 5);

    zReader::Node spawnListValues[3] = {0};
    spawnListValues[1] = spawn0;
    spawnListValues[2] = spawn1;
    zReader::Node spawnList = zReader::Node();
    MakeGameNetReaderArrayNode(spawnList, spawnListValues, 3);

    zReader::Node rootValues[2] = {0};
    rootValues[1] = spawnList;
    zReader::Node root = zReader::Node();
    MakeGameNetReaderArrayNode(root, rootValues, 2);

    zArchiveList *const oldMountedList = g_zArchive_MountedList;
    zIndexArchive archive = zIndexArchive();
    zZarFileRecord records[1] = {0};
    zArchiveListNode archiveNode = zArchiveListNode();
    zArchiveList archiveList = zArchiveList();
    const GameNetZrdArchiveEntry entries[] = {{"net.zrd", &root}};
    if (!MountGameNetZrdArchive(
            "gamenet_init.zar",
            entries,
            1,
            archive,
            records,
            archiveNode,
            archiveList
        )) {
        SetCurrentDirectoryA(oldDir);
        return 4;
    }

    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    zUtil_SaveGameState *const oldSaveHead = g_PlayerSaveStateListHead;
    GameNetPlayerRow *const oldRowHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldRowTail = g_GameNetPlayerRowTail;
    const GameNetPlayerRowListState oldRowList = g_GameNetPlayerRowList;
    const unsigned int oldRowCount = g_GameNetPlayerRowCount;
    GameNetSpawnPoint *const oldSpawnHead = g_GameNetSpawnPointHead;
    GameNetSpawnPoint *const oldSpawnTail = g_GameNetSpawnPointTail;
    const GameNetSpawnPointListState oldSpawnList = g_GameNetSpawnPointList;
    const unsigned int oldSpawnCount = g_GameNetSpawnPointCount;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHost = g_zNetwork_IsHostFlag;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
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
    POINT originalCursor = {0};
    GetCursorPos(&originalCursor);

    HWND const hwnd = CreateWindowExA(
        0,
        "STATIC",
        "recoil",
        WS_POPUP,
        20,
        30,
        160,
        120,
        0,
        0,
        GetModuleHandleA(0),
        0
    );
    if (hwnd == 0) {
        g_zArchive_MountedList = oldMountedList;
        CloseHandle((HANDLE)(archive.hFile));
        std::remove("gamenet_init.zar");
        SetCurrentDirectoryA(oldDir);
        return 5;
    }

    zUtil_SaveGameState saveState = zUtil_SaveGameState();
    zUtil_PlayerStateStorage playerState = zUtil_PlayerStateStorage();
    PlayerModalState modalState = PlayerModalState();
    PlayerMasterModalData modalData = PlayerMasterModalData();
    zClass_Object3DDataPartial objectData = zClass_Object3DDataPartial();
    zClass_NodePartial modalNode = zClass_NodePartial();
    modalNode.classId = 5;
    modalNode.classData = &objectData;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalState.modalNode = &modalNode;
    modalData.masterType = 3;

    zNetwork_PlayerRecord localRecord = zNetwork_PlayerRecord();
    localRecord.playerKey = 0x1234;
    localRecord.colorIndex = 2;
    std::strcpy(localRecord.playerName, "Local");
    zNetworkPlayerRecordListNode sentinel = zNetworkPlayerRecordListNode();
    zNetworkPlayerRecordListNode localNode = zNetworkPlayerRecordListNode();
    sentinel.next = &localNode;
    sentinel.prev = &localNode;
    localNode.next = &sentinel;
    localNode.prev = &sentinel;
    localNode.playerRecord = &localRecord;
    zNetworkPlayerRecordList playerList = zNetworkPlayerRecordList();
    playerList.sentinelNode = &sentinel;
    playerList.count = 1;
    zNetworkDPlaySessionDescCache session = zNetworkDPlaySessionDescCache();
    session.desc.dwMaxPlayers = 8;

    HudUiTriplet triplet = HudUiTriplet();
    triplet.Constructor();
    HudUiStatsListElement statsList = HudUiStatsListElement();
    statsList.triplet = &triplet;
    HudUiTimerPanel timer = HudUiTimerPanel();
    HudUiPanel *const timerPanel = (HudUiPanel *)(&timer);
    timerPanel->ConstructorDefault("", 0, 0);

    g_PlayerSaveStateListHead = 0;
    g_GameNetPlayerRowList = GameNetPlayerRowListState();
    g_GameNetPlayerRowHead = 0;
    g_GameNetPlayerRowTail = 0;
    g_GameNetPlayerRowCount = 0;
    g_GameNetSpawnPointList = GameNetSpawnPointListState();
    g_GameNetSpawnPointHead = 0;
    g_GameNetSpawnPointTail = 0;
    g_GameNetSpawnPointCount = 0;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)(&saveState);
    g_zNetwork_LocalPlayerKey = localRecord.playerKey;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_LocalPlayerRecord = &localRecord;
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
    GameNetSpawnPoint *const secondSpawn = firstSpawn != 0 ? firstSpawn->next : 0;
    int spawnFailure = 0;
    if (firstSpawn == 0) {
        spawnFailure = 20;
    } else if (secondSpawn == 0) {
        spawnFailure = 21;
    } else if (secondSpawn->next != 0) {
        spawnFailure = 22;
    } else if (!Vec3Equals(firstSpawn->position, zVec3{1.0f, 2.0f, 3.0f})) {
        spawnFailure = 23;
    } else if (!FloatNear(firstSpawn->yawDegrees, 10.0f)) {
        spawnFailure = 24;
    } else if (!Vec3Equals(secondSpawn->position, zVec3{4.0f, 5.0f, 6.0f})) {
        spawnFailure = 25;
    } else if (!FloatNear(secondSpawn->yawDegrees, 90.0f)) {
        spawnFailure = 26;
    }
    const bool spawnOk = spawnFailure == 0;
    const bool rowOk = row != 0 && row->playerKey == 0x1234 &&
                       row->playerColorIndex == 2 && std::strcmp(row->displayName, "Local") == 0 &&
                       row->playerColorPackedRgb == g_GameNetPlayerRowStyleColors_00RRGGBB[2];
    const bool hostTimerOk =
        g_GameNetHostHudTimerInitFlag == 0 && g_HudTimerPanelNetState.timerDirectionNeg == 1 &&
        FloatNear(g_HudTimerPanelNetState.statusBitsResendDeadline, 30.0f) &&
        g_HudTimerPanelNetState.timeWarningShown == 0 &&
        g_HudTimerPanelNetState.oneMinuteWarningShown == 0 &&
        FloatNear(FieldAt<float>(&timer, 0x2a4), 30.0f) &&
        FieldAt<int>(&timer, 0x2ac) == -1;
    const bool respawnOk =
        secondSpawn != 0 && Vec3Equals(playerState.worldPos, secondSpawn->position) &&
        FloatNear(playerState.restartYawRad, 1.5707964f) &&
        saveState.netPlayerRow == row && g_GameNetPkt06InitialSyncGate == 1 &&
        g_GameNetPkt06NextSendTimeSec == 0.0f;

    GameNetSpawnPoint *spawn = g_GameNetSpawnPointHead;
    while (spawn != 0) {
        GameNetSpawnPoint *const next = spawn->next;
        ::operator delete(spawn);
        spawn = next;
    }
    ::operator delete(row);

    DeleteObject(timerPanel->hFont);
    timerPanel->hFont = 0;
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
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
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
    g_zArchive_MountedList = oldMountedList;
    CloseHandle((HANDLE)(archive.hFile));
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

extern "C" int gamenet_find_player_row_and_status_bits_smoke(void) {
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    const int oldGoalValue = g_HudSensorTracker.runtimeGoalValue;
    const int oldLapTargetStarted = g_GameNetAllPlayersLapTargetCheckStarted;
    const int oldAllowMaps = g_GameNetStatus_AllowMaps;
    const int oldNameTags = g_GameNetStatus_NameTags;

    GameNetPlayerRow *const first = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    GameNetPlayerRow *const second = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(
        first,
        0,
        sizeof(*first)
    );
    std::memset(
        second,
        0,
        sizeof(*second)
    );
    first->playerKey = 10;
    first->lapCount = 3;
    first->next = second;
    second->playerKey = 20;
    second->lapCount = 4;
    g_GameNetPlayerRowHead = first;
    g_HudSensorTracker.runtimeGoalValue = 3;

    const bool rowLookup =
        GameNet::FindPlayerRowByKey(20) == second && GameNet::FindPlayerRowByKey(30) == 0;
    const bool lapsReached =
        GameNet::AreAllPlayersAtLapTarget() == 1 && g_GameNetAllPlayersLapTargetCheckStarted == 1;
    g_GameNetAllPlayersLapTargetCheckStarted = 0;
    second->lapCount = 2;
    const bool lapsBlocked =
        GameNet::AreAllPlayersAtLapTarget() == 0 && g_GameNetAllPlayersLapTargetCheckStarted == 1;
    g_GameNetAllPlayersLapTargetCheckStarted = 0;
    g_GameNetPlayerRowHead = 0;
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

    g_GameNetPlayerRowHead = oldHead;
    g_HudSensorTracker.runtimeGoalValue = oldGoalValue;
    g_GameNetAllPlayersLapTargetCheckStarted = oldLapTargetStarted;
    g_GameNetStatus_AllowMaps = oldAllowMaps;
    g_GameNetStatus_NameTags = oldNameTags;
    ::operator delete(second);
    ::operator delete(first);

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

    void *panelTable[32] = {};
    panelTable[0x0c / 4] = MethodAddress(&TestRemoteHudPanelOps::SetPos);
    panelTable[0x60 / 4] = MethodAddress(&TestRemoteHudPanelOps::SetVisible);

    zClass_NodePartial localRoot = {};
    zClass_NodePartial remoteRoot = {};
    zUtil_PlayerStateStorage localPlayer = {};
    localPlayer.rootNode = &localRoot;
    zUtil_SaveGameState localSave = {};
    localSave.playerState = &localPlayer;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&localSave);
    g_Player_RuntimeDiScene = 0;

    zUtil_PlayerStateStorage remotePlayer = {};
    remotePlayer.rootNode = &remoteRoot;
    remotePlayer.worldPos = {1.0f, 2.0f, 10.0f};
    zUtil_SaveGameState remoteSave = {};
    remoteSave.playerState = &remotePlayer;
    std::aligned_storage<sizeof(GameNetPlayerRow), alignof(GameNetPlayerRow)>::type rowStorage;
    std::memset(&rowStorage, 0, sizeof(rowStorage));
    GameNetPlayerRow *const row = reinterpret_cast<GameNetPlayerRow *>(&rowStorage);
    *reinterpret_cast<void **>(&row->hudWidget) = panelTable;
    FieldAt<int>(&row->hudWidget, 0x260) = 14;
    FieldAt<int>(&row->hudWidget, 0x270) = 0;
    FieldAt<int>(&row->hudWidget, 0x274) = 0;
    remoteSave.netPlayerRow = row;

    g_remoteHudSetVisibleCount = 0;
    g_remoteHudSetPosCount = 0;
    g_GameNetStatus_NameTags = 0;
    const bool disabledOk = GameNet::UpdateRemotePlayerHudWidgetScreenPos(&remoteSave) == 0 &&
                            g_remoteHudSetVisibleCount == 0 && g_remoteHudSetPosCount == 0;

    g_GameNetStatus_NameTags = 1;
    const int visibleResult = GameNet::UpdateRemotePlayerHudWidgetScreenPos(&remoteSave);
    const bool visibleOk =
        visibleResult == 1 && g_remoteHudSetPosCount == 1 &&
        g_remoteHudSetPosThis == &row->hudWidget && g_remoteHudLastX == 330 &&
        g_remoteHudLastY == 205 && g_remoteHudSetVisibleCount == 1 &&
        g_remoteHudLastVisible == 1;

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

    zNetwork::RegisterPacketHandler(
        6,
        (zNetworkPacketHandler)&GameNet::HandlePkt06_PlayerStateSnapshot,
        0
    );
    zNetwork::RegisterPacketHandler(
        7,
        (zNetworkPacketHandler)&GameNet::HandlePkt07_AltGunDispatch,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x0a,
        (zNetworkPacketHandler)&OptCatalog::HandlePkt0A_RemoveRuntimeRelay,
        0
    );
    zNetwork::RegisterPacketHandler(
        1,
        (zNetworkPacketHandler)&GameNet::ReassignPlayerColorsAndRefreshRows,
        0
    );
    zNetwork::RegisterPacketHandler(
        8,
        (zNetworkPacketHandler)&GameNet::HandlePkt08_PlayerKillEvent,
        0
    );
    zNetwork::RegisterPacketHandler(
        9,
        (zNetworkPacketHandler)&GameNet::HandlePkt09_PlayerScoreboardSnapshot,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x0b,
        (zNetworkPacketHandler)&GameNet::HandlePkt0B_ChatMessage,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x0e,
        (zNetworkPacketHandler)&GameNet::HandlePkt0E_PlayerLapProgress,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x0c,
        (zNetworkPacketHandler)&GameNet::HandlePkt0C_HudTimerStatusBits,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x0d,
        (zNetworkPacketHandler)&GameNet::HandlePkt0D_HudTimerPanelState,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x0f,
        (zNetworkPacketHandler)&zDEClient_Crater::NetRelayCallback,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x10,
        (zNetworkPacketHandler)&zDEClient_QSand::NetRelayCallback,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x11,
        (zNetworkPacketHandler)&Pickup::HandlePkt11_SpawnDelta,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x12,
        (zNetworkPacketHandler)&Pickup::HandlePkt12_AirdropSpawnChuteRelay,
        0
    );
    zNetwork::RegisterPacketHandler(
        0x13,
        (zNetworkPacketHandler)&GameNet::HandlePkt13_EffectAnimActivationRecord,
        0
    );

    GameNet::UnregisterGameplayPacketHandlers();
    const bool ok = g_zNetwork_DispatchHandlerListCount == 0 && sentinel.next == &sentinel &&
                    sentinel.prev == &sentinel && g_GameNet_HandlersRegistered == 0;

    ClearDispatchHandlerListForTest(sentinel);
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
    OptCatalogAllocRuntimeGateCallback const oldAllocGate =
        g_OptCatalog_AllocRuntimeGateCallback;
    OptCatalogAllocRuntimeGateCallback const oldNoOpGate =
        g_OptCatalog_AltGunDispatchNoOpCallback;
    OptCatalogRemoveRuntimeRelayCallback const oldRemoveRelay =
        g_OptCatalog_RemoveRuntimeRelayCallback;
    void(__fastcall *oldEffectDispatch)(
        zEffectAnimActivationRecord *record
    ) = g_zEffectAnim_ActivationDispatchCallback;
    const unsigned int oldEffectDispatchTag = g_zEffectAnim_ActivationDispatchTagHigh;

    zNetworkDispatchHandlerListNode sentinel = {};
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    g_zNetwork_DispatchHandlerListSentinel = &sentinel;
    g_zNetwork_DispatchHandlerListCount = 0;
    g_GameNet_HandlersRegistered = 0;
    g_zDEClientCraterNetRelayCallback = 0;
    g_zDEClientQSandNetRelayCallback = 0;
    g_OptCatalog_AllocRuntimeGateCallback = 0;
    g_OptCatalog_AltGunDispatchNoOpCallback = 0;
    g_OptCatalog_RemoveRuntimeRelayCallback = 0;
    g_zEffectAnim_ActivationDispatchCallback = 0;
    g_zEffectAnim_ActivationDispatchTagHigh = 0;

    GameNet::RegisterGameplayHandlersAndOptCatalogCallbacks();

    unsigned int packetMask = 0;
    bool modesOk = true;
    for (zNetworkDispatchHandlerListNode *node = sentinel.next; node != &sentinel;
         node = node->next) {
        if (node->record == 0 || node->record->mode != 2) {
            modesOk = false;
            break;
        }
        if (node->record->packetType >= 0 && node->record->packetType < 32) {
            packetMask |= 1u << (unsigned int)(node->record->packetType);
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

extern "C" int gamenet_host_update_session_status_fields_smoke(void) {
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetworkDPlaySessionDescCache *const oldSession = g_zNetwork_CurrentSessionDescCache;
    const int oldIsHost = g_zNetwork_IsHostFlag;

    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetworkDPlaySessionDescCache session = {};
    char sessionName[0x5c] = "mission";
    session.desc.lpszSessionNameA = sessionName;
    session.desc.dwMaxPlayers = 8;
    session.desc.dwUser1 = 1;
    session.desc.dwUser2 = 2;
    session.desc.dwUser3 = 3;
    session.desc.dwUser4 = 4;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_CurrentSessionDescCache = &session;
    g_zNetwork_IsHostFlag = 0;
    g_setSessionDescCalls = 0;
    g_setSessionDescResult = 0;

    if (GameNet::HostUpdateSessionDescStatusFields(10, 13, 12, 11) != 0 ||
        g_setSessionDescCalls != 0 || session.desc.dwUser1 != 1) {
        g_zNetwork_IsHostFlag = oldIsHost;
        g_zNetwork_CurrentSessionDescCache = oldSession;
        g_zNetwork_pDirectPlay4 = oldDPlay;
        return 1;
    }

    g_zNetwork_IsHostFlag = 1;
    if (GameNet::HostUpdateSessionDescStatusFields(10, 13, 12, 11) != 1 ||
        g_setSessionDescCalls != 1 || session.desc.dwUser1 != 10 ||
        session.desc.dwUser2 != 11 || session.desc.dwUser3 != 12 ||
        session.desc.dwUser4 != 13 || session.desc.dwMaxPlayers != 8 ||
        std::strcmp(sessionName, "mission") != 0) {
        g_zNetwork_IsHostFlag = oldIsHost;
        g_zNetwork_CurrentSessionDescCache = oldSession;
        g_zNetwork_pDirectPlay4 = oldDPlay;
        return 2;
    }

    g_setSessionDescResult = (HRESULT)(0x88770014);
    if (GameNet::HostUpdateSessionDescStatusFields(20, 23, 22, 21) != 0 ||
        session.desc.dwUser1 != 20) {
        g_zNetwork_IsHostFlag = oldIsHost;
        g_zNetwork_CurrentSessionDescCache = oldSession;
        g_zNetwork_pDirectPlay4 = oldDPlay;
        return 3;
    }

    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_CurrentSessionDescCache = oldSession;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    return 0;
}

extern "C" int gamenet_send_pkt14_hud_timer_and_flags_sync_smoke(void) {
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    const NetPkt14_HudTimerAndFlagsSync oldPacket = g_NetPkt14_HudTimerAndFlagsSyncBuf;

    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x11223344;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x55667788;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(
        g_sendPacketBytes,
        0,
        sizeof(g_sendPacketBytes)
    );

    const int result = GameNet::SendPkt14_HudTimerAndFlagsSync(
        0x12345,
        0xaabbccdd,
        77,
        0x23456
    );
    const NetPkt14_HudTimerAndFlagsSync *const sentPacket =
        (const NetPkt14_HudTimerAndFlagsSync *)(g_sendPacketBytes);
    const bool ok =
        result == 0 && g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt14_HudTimerAndFlagsSyncBuf &&
        g_sendPacketSize == sizeof(NetPkt14_HudTimerAndFlagsSync) &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.header.packetType == 0x14 &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.header.packetSizeBytes ==
            sizeof(NetPkt14_HudTimerAndFlagsSync) &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.header.payloadDword0 == 0x55667788 &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.eventCode == 0x2345 &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.auxParam == 0x3456 &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.valueOrTime == 77 &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.statusFlags == 0xaabbccdd &&
        sentPacket->header.payloadDword0 == 0x55667788 &&
        sentPacket->eventCode == 0x2345 && sentPacket->auxParam == 0x3456 &&
        sentPacket->valueOrTime == 77 && sentPacket->statusFlags == 0xaabbccdd;

    g_NetPkt14_HudTimerAndFlagsSyncBuf = oldPacket;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    return ok ? 0 : 1;
}

extern "C" int gamenet_tick_local_player_pkt06_and_timer_smoke(void) {
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldIsHost = g_zNetwork_IsHostFlag;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
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
    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x11223344;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x55667788;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;

    HudUiTimerPanel timer = HudUiTimerPanel();
    HudUiPanel *const panel = (HudUiPanel *)(&timer);
    panel->ConstructorDefault("", 0, 0);
    g_HudUiMgrTimerPanel = &timer;
    HudUiTimerPanel::SetSeconds(120.0f, -1.0f);

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    static GameNetPlayerRow row;
    std::memset(&row, 0, sizeof(row));
    zVec3 targetA = {1.0f, 2.0f, 3.0f};
    zVec3 targetB = {4.0f, 5.0f, 6.0f};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    saveState.netPlayerRow = &row;
    modalState.masterModalData = &modalData;
    modalData.masterType = 5;
    row.playerColorIndex = 7;

    g_GameStateOrMapTable =
        (zInput_GameStateOrMapTablePartial *)((void *)(&saveState));
    g_Time_AccumulatedTimeSec = 20.0f;
    g_FrameDeltaTimeSec = 0.25f;
    g_GameNetPkt06NextSendTimeSec = 19.0f;
    g_GameNetPkt06InputBit16Latch = 0;
    g_GameNetPkt06InputBit17Latch = 0;
    g_GameNetPkt06InitialSyncGate = 0;
    g_GameNetHudTimerTenSecondWarningArmed = 0;
    g_HudSensorTracker.raceCheckpointMode = 1;
    g_HudTimerPanelNetState = HudTimerPanelNetState();
    g_HudTimerPanelNetState.timerDirectionNeg = 1;
    g_HudTimerPanelNetState.tenSecondWarningsEnabled = 1;
    g_NetPkt06_PlayerStateSnapshotBuf = NetPkt06_PlayerStateSnapshot();

    playerState.netInputBit16Latch = 1;
    playerState.netInputBit17Latch = 0;
    playerState.cachedAltSelectionCode = 301;
    playerState.cachedPrimarySelectionCode = 400;
    playerState.altGunAimOrigin = zVec3{10.0f, 11.0f, 12.0f};
    playerState.storedTargetPos = zVec3{20.0f, 21.0f, 22.0f};
    playerState.worldPos = zVec3{30.0f, 31.0f, 32.0f};
    playerState.vehicleRotationAngles = zVec3{0.1f, 0.2f, 0.3f};
    playerState.statusMeterValue = 88.0f;
    playerState.progressTargetCount = 2;
    playerState.progressTargetSlots[0].targetPos = &targetA;
    playerState.progressTargetSlots[1].targetPos = &targetB;

    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    const int result = GameNet::TickLocalPlayerPkt06ReplicationAndHudTimer(&saveState);
    const NetPkt06_PlayerStateSnapshot *const sentPacket =
        (const NetPkt06_PlayerStateSnapshot *)(g_sendPacketBytes);
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
        Vec3Equals(sentPacket->altGunAimOrigin, zVec3{10.0f, 11.0f, 12.0f}) &&
        Vec3Equals(sentPacket->storedTargetPos, zVec3{20.0f, 21.0f, 22.0f}) &&
        Vec3Equals(sentPacket->worldPos, zVec3{30.0f, 31.0f, 32.0f}) &&
        Vec3Equals(sentPacket->vehicleRotationAngles, zVec3{0.1f, 0.2f, 0.3f}) &&
        sentPacket->statusMeterValue == 88.0f && sentPacket->progressTargetCount == 2 &&
        Vec3Equals(sentPacket->progressTargetPoints[0], targetA) &&
        Vec3Equals(sentPacket->progressTargetPoints[1], targetB) &&
        g_GameNetPkt06InputBit16Latch == 0 && g_GameNetPkt06InputBit17Latch == 0 &&
        FloatNear(g_GameNetPkt06NextSendTimeSec, 20.1f);

    HudUiTimerPanel::SetSeconds(9.0f, -1.0f);
    g_Time_AccumulatedTimeSec = 21.0f;
    g_GameNetPkt06NextSendTimeSec = 20.0f;
    g_HudTimerPanelNetState = HudTimerPanelNetState();
    g_HudTimerPanelNetState.timerDirectionNeg = 1;
    g_HudTimerPanelNetState.tenSecondWarningsEnabled = 1;
    playerState.progressTargetCount = 0;
    g_sendCalls = 0;
    GameNet::TickLocalPlayerPkt06ReplicationAndHudTimer(&saveState);
    const bool countdownTriggeredOk = g_HudTimerPanelNetState.startCountdownTriggered == 1;
    const bool countdownTimerOk = FieldAt<float>(&timer, 0x2a4) == 10.25f &&
                                  FieldAt<int>(&timer, 0x2ac) == -1;
    const bool countdownSendOk = g_sendCalls >= 2;

    DeleteObject(panel->hFont);
    panel->hFont = 0;
    g_HudUiMgrTimerPanel = 0;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
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

extern "C" int gamenet_handle_pkt14_hud_timer_and_flags_sync_smoke(void) {
    CWnd *const oldMainWnd = g_RecoilApp.m_pMainWnd;
    const int oldCurrentStateIndex = g_RecoilApp.m_currentStateIndex;
    RecoilApp_StateQueue oldQueue = g_RecoilApp.m_stateQueue;
    RecoilApp_IState *oldStateStack[16];
    for (int index = 0; index < 16; ++index) {
        oldStateStack[index] = g_RecoilApp.m_stateStack[index];
    }

    const int oldMissionFmvMissionId = g_RecoilApp.m_missionFmvState.m_missionId;
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

    alignas(CZRecoilFrame) unsigned char mainWndStorage[sizeof(CZRecoilFrame)] = {};
    CZRecoilFrame *const mainWnd = reinterpret_cast<CZRecoilFrame *>(mainWndStorage);
    mainWnd->m_useArchiveBanks = 77;
    g_RecoilApp.m_pMainWnd = (CWnd *)(mainWnd);
    g_RecoilApp.m_currentStateIndex = -1;
    std::memset(
        g_RecoilApp.m_stateStack,
        0,
        sizeof(g_RecoilApp.m_stateStack)
    );
    std::memset(
        &g_RecoilApp.m_stateQueue,
        0,
        sizeof(g_RecoilApp.m_stateQueue)
    );
    g_RecoilApp.m_missionFmvState.m_missionId = 99;

    g_HudSensorTracker.runtimeGoalValue = 0;
    g_HudSensorTracker.runtimeTimerSecRaw = 0;
    g_HudSensorTracker.missionId = 0;
    g_HudSensorTracker.missionFlags = 0;
    g_GameNetStatus_AllowMaps = 0;
    g_GameNetStatus_NameTags = 0;
    g_GameNet_HandlersRegistered = 1;

    zNetworkDispatchHandlerListNode sentinel = {};
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    g_zNetwork_DispatchHandlerListSentinel = &sentinel;
    g_zNetwork_DispatchHandlerListCount = 0;

    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetworkDPlaySessionDescCache session = {};
    char sessionName[0x5c] = "pkt14";
    session.desc.lpszSessionNameA = sessionName;
    session.desc.dwMaxPlayers = 8;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_CurrentSessionDescCache = &session;
    g_zNetwork_IsHostFlag = 1;
    g_setSessionDescCalls = 0;
    g_setSessionDescResult = 0;

    NetPkt14_HudTimerAndFlagsSync packet = {};
    packet.header.packetType = 0x14;
    packet.header.packetSizeBytes = sizeof(packet);
    packet.eventCode = 4;
    packet.auxParam = 12;
    packet.valueOrTime = 3;
    packet.statusFlags = 3;

    const int result = GameNet::HandlePkt14_HudTimerAndFlagsSync(
        0x2222,
        &packet
    );

    union TimerSecondsBits {
        float seconds;
        int raw;
    } expectedTimer = {180.0f};

    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue;
    bool queuedIntro = false;
    RecoilApp_StateQueueItem *queuedItem = 0;
    if (queue.m_itemCount == 1 && queue.m_writeBlock.m_cursor != 0) {
        RecoilApp_StateQueueItem **const slot = queue.m_writeBlock.m_cursor - 1;
        queuedItem = *slot;
        queuedIntro =
            queuedItem != 0 &&
            queuedItem->m_kind == RecoilApp_StateQueueKind_SwitchCurrent &&
            queuedItem->m_stateObj == &g_RecoilApp.m_introFmvState &&
            queuedItem->m_param == 0;
    }

    int failure = 0;
    if (result != 1) {
        failure = 1;
    } else if (g_GameNet_HandlersRegistered != 0) {
        failure = 2;
    } else if (g_HudSensorTracker.runtimeTimerSecRaw != expectedTimer.raw ||
               g_HudSensorTracker.runtimeGoalValue != 12) {
        failure = 3;
    } else if (g_HudSensorTracker.missionId != 10 ||
               g_HudSensorTracker.missionFlags != 77) {
        failure = 4;
    } else if (g_GameNetStatus_AllowMaps != 1 ||
               g_GameNetStatus_NameTags != 1) {
        failure = 5;
    } else if (g_RecoilApp.m_missionFmvState.m_missionId != 0 ||
               !queuedIntro) {
        failure = 6;
    } else if (g_setSessionDescCalls != 1 ||
               session.desc.dwUser1 != 4 ||
               session.desc.dwUser4 != 12 ||
               session.desc.dwUser3 != 3 ||
               session.desc.dwUser2 != 3) {
        failure = 7;
    } else if (session.desc.dwMaxPlayers != 8 ||
               std::strcmp(sessionName, "pkt14") != 0) {
        failure = 8;
    }

    if (queuedItem != 0) {
        ::operator delete(queuedItem);
    }
    if (queue.m_readBlock.m_chunkBegin != 0) {
        ::operator delete(queue.m_readBlock.m_chunkBegin);
    }
    if (queue.m_chunkBaseList != 0) {
        ::operator delete(queue.m_chunkBaseList);
    }

    g_RecoilApp.m_pMainWnd = oldMainWnd;
    g_RecoilApp.m_currentStateIndex = oldCurrentStateIndex;
    for (int index = 0; index < 16; ++index) {
        g_RecoilApp.m_stateStack[index] = oldStateStack[index];
    }
    g_RecoilApp.m_stateQueue = oldQueue;
    g_RecoilApp.m_missionFmvState.m_missionId = oldMissionFmvMissionId;
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
    const GameNetPlayerRowListState oldRowList = g_GameNetPlayerRowList;
    GameNetPlayerRow *const oldRowHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldRowTail = g_GameNetPlayerRowTail;
    const unsigned int oldRowCount = g_GameNetPlayerRowCount;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    OptCatalogRuntimeInstanceStorage *const oldFreeRuntimeList =
        g_OptCatalogFreeRuntimeInstanceList;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    HudUiTopMessageStack topStack{};
    topStack.enabled = 0;
    g_HudUiTopMessageStack = &topStack;

    void *panelVtable[32] = {};
    panelVtable[24] = MethodAddress(&TestRemoteHudPanelOps::SetVisible);
    g_remoteHudSetVisibleCount = 0;
    g_remoteHudLastVisible = 7;

    GameNetPlayerRow first{};
    first.playerKey = 0x3101;
    std::strcpy(
        first.displayName,
        "First"
    );
    first.hudWidget.ConstructorDefault(
        "",
        0,
        0
    );
    *reinterpret_cast<void ***>(&first.hudWidget) = panelVtable;

    GameNetPlayerRow *const removed = new GameNetPlayerRow{};
    removed->playerKey = 0x3102;
    removed->playerColorPackedRgb = 0x00123456;
    std::strcpy(
        removed->displayName,
        "Removed"
    );
    removed->hudWidget.ConstructorDefault(
        "",
        0,
        0
    );
    *reinterpret_cast<void ***>(&removed->hudWidget) = panelVtable;
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

    g_GameNetPlayerRowList.flags = 1;
    g_GameNetPlayerRowHead = &first;
    g_GameNetPlayerRowTail = removed;
    g_GameNetPlayerRowCount = 2;

    triplet.AddEntry(&first);
    triplet.AddEntry(removed);
    topStack.AddChild(reinterpret_cast<HudUiElement *>(&first.hudWidget));
    topStack.AddChild(reinterpret_cast<HudUiElement *>(&removed->hudWidget));

    const int result = GameNet::HandlePkt03_RemoveRemotePlayer(removed->playerKey, nullptr);

    const bool playerStateOk =
        result == 0 && playerState.cameraTransitionTimer == 1 && playerState.lifecycleState == 4;
    const bool mineOk = mineEntry.activeRuntimeListHead == nullptr &&
                        g_OptCatalogFreeRuntimeInstanceList == &mineRuntime &&
                       mineRuntime.next == &freeSentinel;
    const bool rowListOk = g_GameNetPlayerRowHead == &first && g_GameNetPlayerRowTail == &first &&
                           g_GameNetPlayerRowCount == 1 && first.next == nullptr;
    const bool hudOk = g_remoteHudSetVisibleCount == 1 && g_remoteHudLastVisible == 0 &&
                       topStack.childHead ==
                           reinterpret_cast<HudUiElement *>(&first.hudWidget) &&
                       topStack.childTail ==
                           reinterpret_cast<HudUiElement *>(&first.hudWidget) &&
                       triplet.entries.begin != nullptr &&
                       triplet.entries.end == triplet.entries.begin + 1 &&
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

extern "C" int gamenet_reset_remote_players_and_spawn_lists_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    const unsigned int oldSpawnListFlags = g_GameNetSpawnPointList.flags;
    GameNetSpawnPoint *const oldSpawnHead = g_GameNetSpawnPointHead;
    GameNetSpawnPoint *const oldSpawnTail = g_GameNetSpawnPointTail;
    const unsigned int oldSpawnCount = g_GameNetSpawnPointCount;
    const unsigned int oldRowListFlags = g_GameNetPlayerRowList.flags;
    GameNetPlayerRow *const oldRowHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldRowTail = g_GameNetPlayerRowTail;
    const unsigned int oldRowCount = g_GameNetPlayerRowCount;

    g_GameNetSpawnPointList.flags = 1;
    g_GameNetSpawnPointHead = (GameNetSpawnPoint *)2;
    g_GameNetSpawnPointTail = (GameNetSpawnPoint *)3;
    g_GameNetSpawnPointCount = 4;
    GameNetSpawnPointList::InitGlobals();
    const bool spawnPointInitGlobalsOk =
        g_GameNetSpawnPointList.flags == 0 && g_GameNetSpawnPointHead == 0 &&
        g_GameNetSpawnPointTail == 0 && g_GameNetSpawnPointCount == 0;

    HudUiTriplet triplet;
    triplet.Constructor();
    HudUiStatsListElement statsList = {};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    HudUiTopMessageStack topStack;
    topStack.Constructor();
    g_HudUiTopMessageStack = &topStack;

    GameNetPlayerRow *const firstRow = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    GameNetPlayerRow *const secondRow = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    GameNetSpawnPoint *const firstSpawn = (GameNetSpawnPoint *)(::operator new(sizeof(GameNetSpawnPoint)));
    GameNetSpawnPoint *const secondSpawn = (GameNetSpawnPoint *)(::operator new(sizeof(GameNetSpawnPoint)));
    std::memset(
        firstRow,
        0,
        sizeof(*firstRow)
    );
    std::memset(
        secondRow,
        0,
        sizeof(*secondRow)
    );
    std::memset(
        firstSpawn,
        0,
        sizeof(*firstSpawn)
    );
    std::memset(
        secondSpawn,
        0,
        sizeof(*secondSpawn)
    );
    firstRow->hudWidget.ConstructorDefault(
        "",
        0,
        0
    );
    secondRow->hudWidget.ConstructorDefault(
        "",
        0,
        0
    );

    firstRow->playerKey = 0x1201;
    firstRow->playerColorPackedRgb = 0x00112233;
    std::strcpy(
        firstRow->displayName,
        "First"
    );
    firstRow->next = secondRow;

    secondRow->playerKey = 0x1202;
    secondRow->playerColorPackedRgb = 0x00445566;
    std::strcpy(
        secondRow->displayName,
        "Second"
    );

    firstSpawn->next = secondSpawn;

    g_GameNetPlayerRowList.flags = 1;
    g_GameNetPlayerRowHead = firstRow;
    g_GameNetPlayerRowTail = secondRow;
    g_GameNetPlayerRowCount = 2;
    g_GameNetSpawnPointList.flags = 1;
    g_GameNetSpawnPointHead = firstSpawn;
    g_GameNetSpawnPointTail = secondSpawn;
    g_GameNetSpawnPointCount = 2;

    triplet.AddEntry(firstRow);
    triplet.AddEntry(secondRow);
    topStack.AddChild((HudUiElement *)(&firstRow->hudWidget));
    topStack.AddChild((HudUiElement *)(&secondRow->hudWidget));

    GameNet::ResetRemotePlayersAndSpawnLists();

    const bool listsCleared =
        g_GameNetPlayerRowList.flags == 0 && g_GameNetPlayerRowHead == 0 &&
        g_GameNetPlayerRowTail == 0 && g_GameNetPlayerRowCount == 0 &&
        g_GameNetSpawnPointList.flags == 0 && g_GameNetSpawnPointHead == 0 &&
        g_GameNetSpawnPointTail == 0 && g_GameNetSpawnPointCount == 0;
    const bool hudCleared =
        !ContainerHasChild(topStack, (const HudUiElement *)(&firstRow->hudWidget)) &&
        !ContainerHasChild(topStack, (const HudUiElement *)(&secondRow->hudWidget)) &&
        triplet.entries.begin != 0 && triplet.entries.end == triplet.entries.begin;

    g_HudUiMgrStatsList = oldStatsList;
    g_HudUiTopMessageStack = oldTopStack;
    g_GameNetSpawnPointList.flags = oldSpawnListFlags;
    g_GameNetSpawnPointHead = oldSpawnHead;
    g_GameNetSpawnPointTail = oldSpawnTail;
    g_GameNetSpawnPointCount = oldSpawnCount;
    g_GameNetPlayerRowList.flags = oldRowListFlags;
    g_GameNetPlayerRowHead = oldRowHead;
    g_GameNetPlayerRowTail = oldRowTail;
    g_GameNetPlayerRowCount = oldRowCount;
    topStack.DestructorCore();
    triplet.DestructorCore();

    if (!spawnPointInitGlobalsOk) {
        return 1;
    }
    return listsCleared && hudCleared ? 0 : 2;
}

extern "C" int gamenet_player_row_list_reset_smoke(void) {
    const unsigned int oldFlags = g_GameNetPlayerRowList.flags;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const unsigned int oldCount = g_GameNetPlayerRowCount;

    g_GameNetPlayerRowList.flags = 5;
    g_GameNetPlayerRowHead = (GameNetPlayerRow *)6;
    g_GameNetPlayerRowTail = (GameNetPlayerRow *)7;
    g_GameNetPlayerRowCount = 8;

    GameNetPlayerRowList::Reset();
    const bool ok =
        g_GameNetPlayerRowList.flags == 0 && g_GameNetPlayerRowHead == 0 &&
        g_GameNetPlayerRowTail == 0 && g_GameNetPlayerRowCount == 0;

    g_GameNetPlayerRowList.flags = oldFlags;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    return ok ? 0 : 1;
}

extern "C" int gamenet_player_row_append_smoke(void) {
    const unsigned int oldFlags = g_GameNetPlayerRowList.flags;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const unsigned int oldCount = g_GameNetPlayerRowCount;

    g_GameNetPlayerRowList.flags = 0;
    g_GameNetPlayerRowHead = 0;
    g_GameNetPlayerRowTail = 0;
    g_GameNetPlayerRowCount = 0;

    GameNetPlayerRow *const first =
        GameNetPlayerRowList::AppendNewRow(
            &g_GameNetPlayerRowList,
            0
        );
    GameNetPlayerRow *const second =
        GameNetPlayerRowList::AppendNewRow(
            &g_GameNetPlayerRowList,
            1
        );
    const bool ok =
        first != 0 && second != 0 && first != second &&
        g_GameNetPlayerRowHead == first &&
        g_GameNetPlayerRowTail == second &&
        g_GameNetPlayerRowCount == 2 &&
        first->next == second &&
        second->next == 0 &&
        second->playerKey == 0 &&
        second->displayName[0] == 0;

    ::operator delete(second);
    ::operator delete(first);
    g_GameNetPlayerRowList.flags = oldFlags;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    return ok ? 0 : 1;
}

extern "C" int gamenet_reassign_player_colors_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    zNetworkPlayerRecordList *const oldPlayerRecordList = g_zNetwork_PlayerRecordList;
    zNetworkDPlaySessionDescCache *const oldSession = g_zNetwork_CurrentSessionDescCache;

    zNetwork_PlayerRecord firstRecord = {};
    firstRecord.playerKey = 0x1111;
    firstRecord.colorIndex = 8;
    zNetwork_PlayerRecord secondRecord = {};
    secondRecord.playerKey = 0x2222;
    secondRecord.colorIndex = 2;

    zNetworkPlayerRecordListNode sentinel = {};
    zNetworkPlayerRecordListNode firstNode = {};
    zNetworkPlayerRecordListNode secondNode = {};
    sentinel.next = &firstNode;
    sentinel.prev = &secondNode;
    firstNode.next = &secondNode;
    firstNode.prev = &sentinel;
    firstNode.playerRecord = &firstRecord;
    secondNode.next = &sentinel;
    secondNode.prev = &firstNode;
    secondNode.playerRecord = &secondRecord;
    zNetworkPlayerRecordList playerList = {};
    playerList.sentinelNode = &sentinel;
    playerList.count = 2;
    g_zNetwork_PlayerRecordList = &playerList;

    zNetworkDPlaySessionDescCache session = {};
    session.desc.dwMaxPlayers = 8;
    g_zNetwork_CurrentSessionDescCache = &session;

    HudUiTriplet triplet = {};
    triplet.Constructor();
    HudUiStatsListElement statsList = {};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    zClass_Object3DDataPartial firstObject = {};
    zClass_NodePartial firstObjectNode = {};
    firstObjectNode.classId = 5;
    firstObjectNode.classData = &firstObject;
    PlayerModalState firstModal = {};
    firstModal.modalNode = &firstObjectNode;
    GameNetPlayerSaveState firstSave = {};
    firstSave.primaryModalState = &firstModal;

    zClass_Object3DDataPartial secondObject = {};
    zClass_NodePartial secondObjectNode = {};
    secondObjectNode.classId = 5;
    secondObjectNode.classData = &secondObject;
    PlayerModalState secondModal = {};
    secondModal.modalNode = &secondObjectNode;
    GameNetPlayerSaveState secondSave = {};
    secondSave.primaryModalState = &secondModal;

    GameNetPlayerRow *const firstRow = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    GameNetPlayerRow *const secondRow = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(
        firstRow,
        0,
        sizeof(*firstRow)
    );
    std::memset(
        secondRow,
        0,
        sizeof(*secondRow)
    );
    firstRow->playerKey = firstRecord.playerKey;
    firstRow->saveState = &firstSave;
    std::strcpy(firstRow->displayName, "First");
    secondRow->playerKey = secondRecord.playerKey;
    secondRow->saveState = &secondSave;
    std::strcpy(secondRow->displayName, "Second");
    firstRow->next = secondRow;
    g_GameNetPlayerRowHead = firstRow;
    g_GameNetPlayerRowTail = secondRow;
    g_GameNetPlayerRowCount = 2;

    GameNet::RefreshPlayerListMenu(firstRow);
    GameNet::RefreshPlayerListMenu(secondRow);
    const std::int32_t result = GameNet::ReassignPlayerColorsAndRefreshRows(0, 0);

    const HudUiScoreboardEntry *firstEntry = 0;
    const HudUiScoreboardEntry *secondEntry = 0;
    for (HudUiScoreboardEntry *entry = triplet.entries.begin; entry != triplet.entries.end;
         ++entry) {
        if (entry->playerKey == firstRow->playerKey) {
            firstEntry = entry;
        }
        if (entry->playerKey == secondRow->playerKey) {
            secondEntry = entry;
        }
    }

    const bool firstOk =
        firstRow->playerColorIndex == 8 && firstRow->playerColorPackedRgb == 0x000040ff &&
        FieldAt<std::uint32_t>(&firstRow->hudWidget, 0x14c) == 0x000040ff &&
        FieldAt<std::uint32_t>(&firstRow->hudWidget, 0x150) == 0x000040ff &&
        FieldAt<std::int32_t>(&firstRow->hudWidget, 0x270) == 1 && firstEntry != 0 &&
        firstEntry->playerColorPackedRgb == 0x000040ff && firstObject.color.red == 1.0f &&
        firstObject.color.green == 1.0f && firstObject.color.blue == 0.0f &&
        firstObject.colorAlpha == 0.2f;

    const bool secondOk =
        secondRow->playerColorIndex == 2 && secondRow->playerColorPackedRgb == 0x0000ff00 &&
        FieldAt<std::uint32_t>(&secondRow->hudWidget, 0x14c) == 0x0000ff00 &&
        FieldAt<std::uint32_t>(&secondRow->hudWidget, 0x150) == 0x0000ff00 &&
        FieldAt<std::int32_t>(&secondRow->hudWidget, 0x270) == 1 && secondEntry != 0 &&
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
    ::operator delete(firstRow);
    ::operator delete(secondRow);

    return result == 1 && firstOk && secondOk ? 0 : 1;
}

extern "C" int gamenet_player_row_apply_color_tint_smoke(void) {
    zClass_Object3DDataPartial objectData = {};
    zClass_NodePartial objectNode = {};
    objectNode.classId = 5;
    objectNode.classData = &objectData;

    PlayerMasterModalData modalData = {};
    modalData.masterType = 5;
    PlayerModalState modalState = {};
    modalState.masterModalData = &modalData;
    modalState.modalNode = &objectNode;
    GameNetPlayerSaveState saveState = {};
    saveState.primaryModalState = &modalState;
    GameNetPlayerRow *const row =
        (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(
        row,
        0,
        sizeof(*row)
    );
    row->playerColorIndex = 8;
    row->saveState = &saveState;

    row->ApplyPlayerColorTint();
    const bool ok = objectData.color.red == 1.0f &&
                    objectData.color.green == 1.0f &&
                    objectData.color.blue == 0.0f &&
                    objectData.colorAlpha == 0.2f &&
                    (objectData.flags & 4) != 0;
    ::operator delete(row);
    return ok ? 0 : 1;
}

extern "C" int gamenet_player_row_destroy_embedded_panel_smoke(void) {
    GameNetPlayerRow row = {};
    row.hudWidget.textPick = 0;
    row.hudWidget.textDirty = 123;

    row.DestroyEmbeddedPanel();
    return row.hudWidget.textPick == 0 &&
                   row.hudWidget.textDirty == 123
               ? 0
               : 1;
}

extern "C" int gamenet_apply_pkt06_player_state_snapshot_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const unsigned int oldCount = g_GameNetPlayerRowCount;
    const int oldFrameTick = g_zVideo_FrameTick;

    HudUiTriplet triplet;
    triplet.Constructor();
    HudUiStatsListElement statsList = {};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    zClass_Object3DDataPartial objectData = {};
    zClass_NodePartial objectNode = {};
    objectNode.classId = 5;
    objectNode.classData = &objectData;

    PlayerMasterModalData modalData = {};
    modalData.masterType = 5;
    PlayerModalState modalState = {};
    modalState.masterModalData = &modalData;
    modalState.modalNode = &objectNode;

    zUtil_PlayerStateStorage playerState = {};
    GameNetPlayerSaveState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    GameNetPlayerRow *const row = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(
        row,
        0,
        sizeof(*row)
    );
    row->playerKey = 0x2468;
    row->playerColorIndex = 1;
    row->saveState = &saveState;
    std::strcpy(
        row->displayName,
        "Pkt06"
    );
    g_GameNetPlayerRowHead = row;
    g_GameNetPlayerRowTail = row;
    g_GameNetPlayerRowCount = 1;
    GameNet::RefreshPlayerListMenu(row);

    zVec3 staleTargets[10] = {};
    for (int index = 0; index < 10; ++index) {
        playerState.progressTargetRuntimeSlots[index].targetPos = &staleTargets[index];
    }
    playerState.cachedAltSelectionCode = 301;
    playerState.cachedPrimarySelectionCode = 201;
    playerState.netLastUpdateFrameTick = oldFrameTick - 1;
    g_zVideo_FrameTick = oldFrameTick + 17;

    NetPkt06_PlayerStateSnapshot packet = {};
    packet.cachedAltSelectionCode = 301;
    packet.cachedPrimarySelectionCode = 201;
    packet.packedMasterTypeColorFlags = 5u | (8u << 8) | 0x10000u;
    packet.storedTargetPos = zVec3_Make(
        1.0f,
        2.0f,
        3.0f
    );
    packet.worldPos = zVec3_Make(
        4.0f,
        5.0f,
        6.0f
    );
    packet.vehicleRotationAngles = zVec3_Make(
        0.25f,
        0.5f,
        0.75f
    );
    packet.statusMeterValue = 77.0f;

    const int clearResult = GameNet::ApplyPkt06_PlayerStateSnapshotToRow(
        row,
        &packet
    );
    bool clearedSlots = true;
    for (int index = 0; index < 10; ++index) {
        clearedSlots =
            clearedSlots && playerState.progressTargetRuntimeSlots[index].targetPos == 0;
    }
    const bool firstOk =
        clearResult == 1 && playerState.netUpdateReceived == 1 &&
        row->playerColorIndex == 8 && row->playerColorPackedRgb == 0x000040ff &&
        FieldAt<unsigned int>(
            &row->hudWidget,
            0x14c
        ) == 0x000040ff &&
        FieldAt<unsigned int>(
            &row->hudWidget,
            0x150
        ) == 0x000040ff &&
        FieldAt<int>(
            &row->hudWidget,
            0x270
        ) == 1 &&
        objectData.color.red == 1.0f && objectData.color.green == 1.0f &&
        objectData.color.blue == 0.0f && objectData.colorAlpha == 0.2f &&
        Vec3Equals(
            playerState.netReceivedPos,
            packet.worldPos
        ) &&
        Vec3Equals(
            playerState.netReceivedAngles,
            packet.vehicleRotationAngles
        ) &&
        Vec3Equals(
            playerState.storedTargetPos,
            packet.storedTargetPos
        ) &&
        playerState.netInputBit16Latch == 1 && playerState.netInputBit17Latch == 0 &&
        playerState.netLastUpdateFrameTick == g_zVideo_FrameTick &&
        FloatNear(
            playerState.statusMeterValue,
            77.0f
        ) &&
        playerState.progressTargetCount == 0 && clearedSlots;

    packet.packedMasterTypeColorFlags = 5u | (8u << 8) | 0x20000u | 0x40000u;
    packet.statusMeterValue = 88.0f;
    packet.progressTargetCount = 2;
    packet.progressTargetPoints[0] = zVec3_Make(
        10.0f,
        11.0f,
        12.0f
    );
    packet.progressTargetPoints[1] = zVec3_Make(
        20.0f,
        21.0f,
        22.0f
    );

    const int targetResult = GameNet::ApplyPkt06_PlayerStateSnapshotToRow(
        row,
        &packet
    );
    const bool secondOk =
        targetResult == 1 && playerState.netInputBit16Latch == 1 &&
        playerState.netInputBit17Latch == 1 &&
        FloatNear(
            playerState.statusMeterValue,
            88.0f
        ) &&
        playerState.progressTargetCount == 2 &&
        playerState.progressTargetRuntimeSlots[0].targetPos ==
            &playerState.progressTargetPointStorage[0] &&
        playerState.progressTargetRuntimeSlots[1].targetPos ==
            &playerState.progressTargetPointStorage[1] &&
        Vec3Equals(
            playerState.progressTargetPointStorage[0],
            packet.progressTargetPoints[0]
        ) &&
        Vec3Equals(
            playerState.progressTargetPointStorage[1],
            packet.progressTargetPoints[1]
        );

    g_HudUiMgrStatsList = oldStatsList;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_zVideo_FrameTick = oldFrameTick;
    ::operator delete(row);
    triplet.DestructorCore();

    return firstOk && secondOk ? 0 : 1;
}

extern "C" int gamenet_handle_pkt06_player_state_snapshot_smoke(void) {
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const unsigned int oldCount = g_GameNetPlayerRowCount;
    const int oldInitialSyncGate = g_GameNetPkt06InitialSyncGate;
    const int oldFrameTick = g_zVideo_FrameTick;

    const int nullResult = GameNet::HandlePkt06_PlayerStateSnapshot(
        0x1111,
        0
    );

    zClass_Object3DDataPartial objectData = {};
    zClass_NodePartial objectNode = {};
    objectNode.classId = 5;
    objectNode.classData = &objectData;

    PlayerMasterModalData modalData = {};
    modalData.masterType = 5;
    PlayerModalState modalState = {};
    modalState.masterModalData = &modalData;
    modalState.modalNode = &objectNode;

    zUtil_PlayerStateStorage playerState = {};
    GameNetPlayerSaveState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    GameNetPlayerRow *const row = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(
        row,
        0,
        sizeof(*row)
    );
    row->playerKey = 0x2468;
    row->playerColorIndex = 1;
    row->saveState = &saveState;
    g_GameNetPlayerRowHead = row;
    g_GameNetPlayerRowTail = row;
    g_GameNetPlayerRowCount = 1;

    NetPkt06_PlayerStateSnapshot ignoredPacket = {};
    ignoredPacket.header.packetType = 5;
    ignoredPacket.header.payloadDword0 = row->playerKey;
    g_GameNetPkt06InitialSyncGate = 1;
    const int ignoredResult = GameNet::HandlePkt06_PlayerStateSnapshot(
        0x1111,
        &ignoredPacket
    );
    const bool ignoredOk =
        ignoredResult == 0 && g_GameNetPkt06InitialSyncGate == 0 &&
        playerState.netUpdateReceived == 0;

    NetPkt06_PlayerStateSnapshot packet = {};
    packet.header.packetType = 6;
    packet.header.payloadDword0 = row->playerKey;
    packet.packedMasterTypeColorFlags = 5u | (1u << 8);
    packet.worldPos = zVec3_Make(
        8.0f,
        9.0f,
        10.0f
    );
    packet.vehicleRotationAngles = zVec3_Make(
        0.125f,
        0.25f,
        0.5f
    );
    g_zVideo_FrameTick = oldFrameTick + 3;

    const int handledResult = GameNet::HandlePkt06_PlayerStateSnapshot(
        0x1111,
        &packet
    );
    const bool handledOk =
        handledResult == 0 && playerState.netUpdateReceived == 1 &&
        row->playerColorIndex == 1 &&
        Vec3Equals(
            playerState.netReceivedPos,
            packet.worldPos
        ) &&
        Vec3Equals(
            playerState.netReceivedAngles,
            packet.vehicleRotationAngles
        );

    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_GameNetPkt06InitialSyncGate = oldInitialSyncGate;
    g_zVideo_FrameTick = oldFrameTick;
    ::operator delete(row);

    return nullResult == -1 && ignoredOk && handledOk ? 0 : 1;
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

    HudUiTriplet triplet;
    triplet.Constructor();
    HudUiStatsListElement statsList = {};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    GameNetPlayerRow *const alpha = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    GameNetPlayerRow *const bravo = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(
        alpha,
        0,
        sizeof(*alpha)
    );
    std::memset(
        bravo,
        0,
        sizeof(*bravo)
    );

    alpha->playerKey = 0x101;
    alpha->playerColorPackedRgb = 0x00112233;
    std::strcpy(
        alpha->displayName,
        "Alpha"
    );

    bravo->playerKey = 0x202;
    bravo->playerColorPackedRgb = 0x00445566;
    std::strcpy(
        bravo->displayName,
        "Bravo"
    );
    alpha->next = bravo;

    g_GameNetPlayerRowHead = alpha;
    g_GameNetPlayerRowTail = bravo;
    g_GameNetPlayerRowCount = 2;
    g_HudSensorTracker.raceCheckpointMode = 0;
    g_HudSensorTracker.runtimeGoalValue = 999;
    g_GameNetOneLapLeftMessageShown = 0;
    g_zNetwork_IsHostFlag = 0;

    GameNet::RefreshPlayerListMenu(alpha);
    GameNet::RefreshPlayerListMenu(bravo);

    ScoreboardPacket2 packet = {};
    packet.header.packetType = 0x09;
    packet.header.packetSizeBytes = sizeof(packet);
    packet.entryCount = 2;
    packet.entries[0].playerKey = alpha->playerKey;
    packet.entries[0].packedScoreAndLapCount = (std::uint16_t)((3 << 9) | 17);
    packet.entries[1].playerKey = bravo->playerKey;
    packet.entries[1].packedScoreAndLapCount = (std::uint16_t)((4 << 9) | 22);

    const std::int32_t handleResult = GameNet::HandlePkt09_PlayerScoreboardSnapshot(
        0,
        (NetPkt09_PlayerScoreboardSnapshot *)(&packet)
    );
    const bool applied =
        handleResult == 1 && alpha->score == 17 && alpha->lapCount == 3 &&
        bravo->score == 22 && bravo->lapCount == 4 &&
        triplet.entries.begin[0].playerKey == bravo->playerKey &&
        triplet.entries.begin[1].playerKey == alpha->playerKey;

    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x4444;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x5678;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(
        g_sendPacketBytes,
        0,
        sizeof(g_sendPacketBytes)
    );

    alpha->score = 33;
    alpha->lapCount = 7;
    bravo->score = 44;
    bravo->lapCount = 5;
    GameNet::SendPkt09_PlayerScoreboardSnapshot();

    const ScoreboardPacket2 *const sentPacket =
        (const ScoreboardPacket2 *)(g_sendPacketBytes);
    const bool sent =
        g_sendCalls == 1 && g_sendFlags == 1 && g_sendPacketSize == sizeof(ScoreboardPacket2) &&
        g_sendPacketBytesSize == sizeof(ScoreboardPacket2) &&
        sentPacket->header.packetType == 0x09 &&
        sentPacket->header.packetSizeBytes == sizeof(ScoreboardPacket2) &&
        sentPacket->header.payloadDword0 == 0x5678 && sentPacket->entryCount == 2 &&
        sentPacket->entries[0].playerKey == alpha->playerKey &&
        sentPacket->entries[0].packedScoreAndLapCount == (std::uint16_t)((7 << 9) | 33) &&
        sentPacket->entries[1].playerKey == bravo->playerKey &&
        sentPacket->entries[1].packedScoreAndLapCount == (std::uint16_t)((5 << 9) | 44);

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
    ::operator delete(alpha);
    ::operator delete(bravo);

    return applied && sent ? 0 : 1;
}

extern "C" int gamenet_lap_progress_packet_smoke(void) {
    struct ScoreboardPacket1 {
        zNetworkPacketHeader header;
        std::int32_t entryCount;
        NetPkt09_PlayerScoreboardEntry entries[1];
    };

    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    const std::int32_t oldRaceMode = g_HudSensorTracker.raceCheckpointMode;
    const std::int32_t oldGoalValue = g_HudSensorTracker.runtimeGoalValue;
    const std::int32_t oldOneLapShown = g_GameNetOneLapLeftMessageShown;
    const std::int32_t oldLapTargetStarted = g_GameNetAllPlayersLapTargetCheckStarted;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldIsHost = g_zNetwork_IsHostFlag;
    const std::int32_t oldTcpIpAsync = g_zNetwork_TcpIpAsyncSendEnabled;

    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x1111;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x2222;
    g_zNetwork_IsHostFlag = 0;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(
        g_sendPacketBytes,
        0,
        sizeof(g_sendPacketBytes)
    );

    zUtil_PlayerStateStorage playerState = {};
    playerState.lapCount = 4;
    playerState.lapTimeSec = 65.0f;
    zUtil_SaveGameState saveState = {};
    GameNetPlayerRow *const localRow =
        (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(
        localRow,
        0,
        sizeof(*localRow)
    );
    saveState.playerState = &playerState;
    saveState.netPlayerRow = localRow;

    GameNet::SendPkt0E_PlayerLapProgress(&saveState);
    const NetPkt0E_PlayerLapProgress *const sentPacket =
        (const NetPkt0E_PlayerLapProgress *)(g_sendPacketBytes);
    const bool clientSend =
        g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacketSize == sizeof(NetPkt0E_PlayerLapProgress) &&
        g_sendPacketBytesSize == sizeof(NetPkt0E_PlayerLapProgress) &&
        sentPacket->header.packetType == 0x0e &&
        sentPacket->header.packetSizeBytes == sizeof(NetPkt0E_PlayerLapProgress) &&
        sentPacket->header.payloadDword0 == 0x2222 && sentPacket->lapCountPacked == 4 &&
        sentPacket->reserved_0a == 0 && sentPacket->lapTimeSec == 65.0f &&
        localRow->lapCount == 0;

    HudUiTriplet triplet;
    triplet.Constructor();
    HudUiStatsListElement statsList = {};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    GameNetPlayerRow *const remoteRow =
        (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    GameNetPlayerRow *const targetRow =
        (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(
        remoteRow,
        0,
        sizeof(*remoteRow)
    );
    std::memset(
        targetRow,
        0,
        sizeof(*targetRow)
    );
    remoteRow->playerKey = 0x3333;
    remoteRow->playerColorPackedRgb = 0x00123456;
    std::strcpy(
        remoteRow->displayName,
        "Remote"
    );
    g_GameNetPlayerRowHead = remoteRow;
    g_GameNetPlayerRowTail = remoteRow;
    g_GameNetPlayerRowCount = 1;
    g_HudSensorTracker.raceCheckpointMode = 0;
    g_HudSensorTracker.runtimeGoalValue = 3;
    g_GameNetOneLapLeftMessageShown = 0;
    GameNet::RefreshPlayerListMenu(remoteRow);

    g_zNetwork_IsHostFlag = 1;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(
        g_sendPacketBytes,
        0,
        sizeof(g_sendPacketBytes)
    );

    NetPkt0E_PlayerLapProgress packet = {};
    packet.lapCountPacked = 2;
    packet.lapTimeSec = 44.0f;
    const std::int32_t handleResult = GameNet::HandlePkt0E_PlayerLapProgress(
        0x3333,
        &packet
    );
    const ScoreboardPacket1 *const scoreboardPacket =
        (const ScoreboardPacket1 *)(g_sendPacketBytes);
    const bool hostHandle =
        handleResult == 1 && remoteRow->lapCount == 2 && remoteRow->lapTimeSec == 44.0f &&
        g_sendCalls == 1 && g_sendFlags == 1 && g_sendPacketSize == sizeof(ScoreboardPacket1) &&
        g_sendPacketBytesSize == sizeof(ScoreboardPacket1) &&
        scoreboardPacket->header.packetType == 0x09 &&
        scoreboardPacket->header.packetSizeBytes == sizeof(ScoreboardPacket1) &&
        scoreboardPacket->entryCount == 1 &&
        scoreboardPacket->entries[0].playerKey == remoteRow->playerKey &&
        scoreboardPacket->entries[0].packedScoreAndLapCount == (std::uint16_t)(2 << 9);

    targetRow->lapCount = 3;
    remoteRow->next = targetRow;
    remoteRow->lapCount = 2;
    g_GameNetPlayerRowTail = targetRow;
    g_GameNetPlayerRowCount = 2;
    g_GameNetAllPlayersLapTargetCheckStarted = 0;
    const bool lapsBlocked =
        GameNet::AreAllPlayersAtLapTarget() == 0 &&
        g_GameNetAllPlayersLapTargetCheckStarted == 1;

    remoteRow->lapCount = 3;
    g_GameNetAllPlayersLapTargetCheckStarted = 0;
    const bool lapsReached =
        GameNet::AreAllPlayersAtLapTarget() == 1 &&
        g_GameNetAllPlayersLapTargetCheckStarted == 1;

    g_HudUiMgrStatsList = oldStatsList;
    g_HudSensorTracker.raceCheckpointMode = oldRaceMode;
    g_HudSensorTracker.runtimeGoalValue = oldGoalValue;
    g_GameNetOneLapLeftMessageShown = oldOneLapShown;
    g_GameNetAllPlayersLapTargetCheckStarted = oldLapTargetStarted;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldTcpIpAsync;
    triplet.DestructorCore();
    ::operator delete(targetRow);
    ::operator delete(remoteRow);
    ::operator delete(localRow);

    return clientSend && hostHandle && lapsBlocked && lapsReached ? 0 : 1;
}

extern "C" int gamenet_chat_compose_key_callback_smoke(void) {
    HudUiChatComposeTextInput oldInput = g_HudUiMgrObjectiveChatComposeTextInput;
    HudUiPanel *const oldDescPanel = g_HudUiMgrObjectiveDescTextPanel;
    const int oldTableReady = g_zInput_KbdDikToAsciiTableReady;

    ChatComposePanelFake descPanel = {};
    g_HudUiMgrObjectiveDescTextPanel = &descPanel;

    g_HudUiMgrObjectiveChatComposeTextInput = HudUiChatComposeTextInput();
    g_HudUiMgrObjectiveChatComposeTextInput.Constructor(8);
    g_HudUiMgrObjectiveChatComposeTextInput.buffer[0] = '\0';
    g_chatComposeSetTextFmtCalls = 0;
    g_chatComposeSetTextFmtThis = 0;
    g_chatComposeSetTextFmtText[0] = '\0';
    g_zInput_KbdDikToAsciiTableReady = 0;
    std::memset(
        g_zInput_KbdDikToAsciiTable,
        0,
        sizeof(g_zInput_KbdDikToAsciiTable)
    );

    GameNet::ChatComposeKeyCallback(0x41e);
    const bool inserted =
        std::strcmp(g_HudUiMgrObjectiveChatComposeTextInput.GetBuffer(), "A") == 0 &&
        g_HudUiMgrObjectiveChatComposeTextInput.cursor == 1 &&
        g_chatComposeSetTextFmtCalls == 1 &&
        g_chatComposeSetTextFmtThis == &descPanel &&
        std::strcmp(g_chatComposeSetTextFmtText, "A") == 0;

    GameNet::ChatComposeKeyCallback(0);
    const bool zeroIgnored =
        g_chatComposeSetTextFmtCalls == 1 &&
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

    HudUiChatComposeTextInput oldInput = g_HudUiMgrObjectiveChatComposeTextInput;
    HudUiPanel *const oldSummaryPanel = g_HudUiMgrObjectiveSummaryTextPanel;
    HudUiPanel *const oldDescPanel = g_HudUiMgrObjectiveDescTextPanel;
    const int oldPhase = g_HudUiMgrObjectivePhase;
    const int oldState = g_HudUiMgrObjectiveState;
    const int oldShowReset = g_HudUiMgrObjectiveShowResetUnused;
    const float oldAutoHide = g_HudUiMgrObjectiveAutoHideDelaySec;
    zInput::KbdKeyDispatchEntry oldDispatch[0x7de];
    std::memcpy(
        oldDispatch,
        g_zInputKbdKeyDispatchTable,
        sizeof(oldDispatch)
    );

    ChatComposePanelFake summaryPanel = {};
    ChatComposePanelFake descPanel = {};
    g_HudUiMgrObjectiveSummaryTextPanel = &summaryPanel;
    g_HudUiMgrObjectiveDescTextPanel = &descPanel;
    g_HudUiMgrObjectivePhase = 0;
    g_HudUiMgrObjectiveState = 0;
    g_HudUiMgrObjectiveChatComposeActive = 0;
    g_chatComposeSetTextFmtCalls = 0;

    g_HudUiMgrObjectiveChatComposeTextInput = HudUiChatComposeTextInput();
    g_HudUiMgrObjectiveChatComposeTextInput.Constructor(8);
    char *const initialBuffer = g_HudUiMgrObjectiveChatComposeTextInput.buffer;
    zInput::BindMapSystem_Init(1);

    networkEnabled = 1;
    GameNet::BeginChatCompose();
    const bool stateOk =
        g_HudUiMgrObjectiveChatComposeActive == 1 &&
        g_HudUiMgrObjectiveState == 1 &&
        g_HudUiMgrObjectivePhase == 1 &&
        g_HudUiMgrObjectiveChatComposeTextInput.capacity == 32 &&
        std::strcmp(g_HudUiMgrObjectiveChatComposeTextInput.GetBuffer(), "") == 0 &&
        g_zInput_BindMapOverlayDepth == 1;
    const void *const callback = (const void *)(&GameNet::ChatComposeKeyCallback);
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

    g_HudUiMgrObjectiveChatComposeTextInput = oldInput;
    g_HudUiMgrObjectiveSummaryTextPanel = oldSummaryPanel;
    g_HudUiMgrObjectiveDescTextPanel = oldDescPanel;
    g_HudUiMgrObjectivePhase = oldPhase;
    g_HudUiMgrObjectiveState = oldState;
    g_HudUiMgrObjectiveChatComposeActive = oldChatComposeActive;
    g_HudUiMgrObjectiveShowResetUnused = oldShowReset;
    g_HudUiMgrObjectiveAutoHideDelaySec = oldAutoHide;
    std::memcpy(
        g_zInputKbdKeyDispatchTable,
        oldDispatch,
        sizeof(oldDispatch)
    );
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;

    return disabledOk && stateOk && keyOk ? 0 : 1;
}

extern "C" int hud_ui_handle_hotkey_command_begin_chat_smoke(void) {
    int networkEnabled = 1;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    HudUiChatComposeTextInput oldInput = g_HudUiMgrObjectiveChatComposeTextInput;
    HudUiPanel *const oldSummaryPanel = g_HudUiMgrObjectiveSummaryTextPanel;
    HudUiPanel *const oldDescPanel = g_HudUiMgrObjectiveDescTextPanel;
    const int oldChatComposeActive = g_HudUiMgrObjectiveChatComposeActive;
    const int oldPhase = g_HudUiMgrObjectivePhase;
    const int oldState = g_HudUiMgrObjectiveState;
    zInput::KbdKeyDispatchEntry oldDispatch[0x7de];
    std::memcpy(
        oldDispatch,
        g_zInputKbdKeyDispatchTable,
        sizeof(oldDispatch)
    );

    ChatComposePanelFake summaryPanel = {};
    ChatComposePanelFake descPanel = {};
    g_HudUiMgrObjectiveSummaryTextPanel = &summaryPanel;
    g_HudUiMgrObjectiveDescTextPanel = &descPanel;
    g_HudUiMgrObjectivePhase = 0;
    g_HudUiMgrObjectiveState = 0;
    g_HudUiMgrObjectiveChatComposeActive = 0;

    g_HudUiMgrObjectiveChatComposeTextInput = HudUiChatComposeTextInput();
    g_HudUiMgrObjectiveChatComposeTextInput.Constructor(8);
    char *const initialBuffer = g_HudUiMgrObjectiveChatComposeTextInput.buffer;
    zInput::BindMapSystem_Init(1);

    HudUi::HandleHotkeyCommand(42);
    const void *const callback = (const void *)(&GameNet::ChatComposeKeyCallback);
    const bool hotkeyOk =
        g_HudUiMgrObjectiveChatComposeActive == 1 &&
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
    std::memcpy(
        g_zInputKbdKeyDispatchTable,
        oldDispatch,
        sizeof(oldDispatch)
    );
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;

    return hotkeyOk ? 0 : 1;
}

extern "C" int gamenet_end_chat_compose_and_send_smoke(void) {
    int networkEnabled = 1;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldTcpIpAsync = g_zNetwork_TcpIpAsyncSendEnabled;
    HudUiTextStack4 *const oldChatStack = g_HudUiChatMessageStack;
    HudUiChatComposeTextInput oldInput = g_HudUiMgrObjectiveChatComposeTextInput;
    HudUiPanel *const oldSummaryPanel = g_HudUiMgrObjectiveSummaryTextPanel;
    HudUiPanel *const oldDescPanel = g_HudUiMgrObjectiveDescTextPanel;
    const int oldPhase = g_HudUiMgrObjectivePhase;
    const int oldState = g_HudUiMgrObjectiveState;
    const int oldChatComposeActive = g_HudUiMgrObjectiveChatComposeActive;
    const int oldOverlayDepth = g_zInput_BindMapOverlayDepth;

    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x10203040;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x10203040;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;

    GameNetPlayerRow row = GameNetPlayerRow();
    std::strcpy(row.displayName, "Pilot");
    zUtil_SaveGameState saveState = {};
    saveState.netPlayerRow = &row;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)(&saveState);

    HudUiChatMessageStack chatStack = HudUiChatMessageStack();
    chatStack.Constructor();
    chatStack.enabled = 1;
    g_HudUiChatMessageStack = &chatStack;

    ChatComposePanelFake summaryPanel = {};
    ChatComposePanelFake descPanel = {};
    g_HudUiMgrObjectiveSummaryTextPanel = &summaryPanel;
    g_HudUiMgrObjectiveDescTextPanel = &descPanel;
    g_HudUiMgrObjectivePhase = 0;
    g_HudUiMgrObjectiveState = 0;
    g_HudUiMgrObjectiveChatComposeActive = 0;

    g_HudUiMgrObjectiveChatComposeTextInput = HudUiChatComposeTextInput();
    g_HudUiMgrObjectiveChatComposeTextInput.Constructor(8);
    char *const initialBuffer = g_HudUiMgrObjectiveChatComposeTextInput.buffer;
    zInput::BindMapSystem_Init(1);
    GameNet::BeginChatCompose();
    g_HudUiMgrObjectiveChatComposeTextInput.SetContents("go");

    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(
        g_sendPacketBytes,
        0,
        sizeof(g_sendPacketBytes)
    );

    GameNet::EndChatComposeAndSendThunk();

    HudUiPanel *const firstLine = &chatStack.lines[0];
    const NetPkt0B_ChatMessage *const sentPacket =
        (const NetPkt0B_ChatMessage *)(g_sendPacketBytes);
    const bool sent =
        g_HudUiMgrObjectiveChatComposeActive == 0 &&
        g_zInput_BindMapOverlayDepth == 0 && g_sendCalls == 1 &&
        g_sendFlags == 1 && g_sendPacketSize == 20 &&
        sentPacket->header.packetType == 0x0b &&
        sentPacket->header.packetSizeBytes == 20 &&
        sentPacket->header.payloadDword0 == 0x10203040 &&
        sentPacket->messageLength == 8 &&
        std::memcmp(
            sentPacket->message,
            "Pilot:go",
            8
        ) == 0 &&
        std::strcmp(firstLine->GetLastTextPtr(), "Pilot:go") == 0 &&
        FieldAt<float>(
            firstLine,
            0x10
        ) == 5.0f;

    g_HudUiMgrObjectiveChatComposeTextInput.SetContents("");
    g_HudUiMgrObjectiveChatComposeActive = 1;
    zInput::BindMapContext_Push(0);
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(
        g_sendPacketBytes,
        0,
        sizeof(g_sendPacketBytes)
    );

    GameNet::EndChatComposeAndSend();

    const bool emptySkipped =
        g_HudUiMgrObjectiveChatComposeActive == 0 &&
        g_zInput_BindMapOverlayDepth == 0 && g_sendCalls == 0 &&
        std::strcmp(firstLine->GetLastTextPtr(), "Pilot:go") == 0;

    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = &chatStack.lines[index];
        DeleteObject(panel->hFont);
        panel->hFont = 0;
    }

    ::operator delete(initialBuffer);
    g_HudUiMgrObjectiveChatComposeTextInput.DestructorCore();
    zInput::BindMapSystem_Shutdown();
    chatStack.DestructorCore();

    g_HudUiMgrObjectiveChatComposeTextInput = oldInput;
    g_HudUiMgrObjectiveSummaryTextPanel = oldSummaryPanel;
    g_HudUiMgrObjectiveDescTextPanel = oldDescPanel;
    g_HudUiMgrObjectivePhase = oldPhase;
    g_HudUiMgrObjectiveState = oldState;
    g_HudUiMgrObjectiveChatComposeActive = oldChatComposeActive;
    g_zInput_BindMapOverlayDepth = oldOverlayDepth;
    g_HudUiChatMessageStack = oldChatStack;
    g_GameStateOrMapTable = oldGameState;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldTcpIpAsync;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;

    return sent && emptySkipped ? 0 : 1;
}

extern "C" int gamenet_chat_message_packet_smoke(void) {
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldTcpIpAsync = g_zNetwork_TcpIpAsyncSendEnabled;
    HudUiTextStack4 *const oldChatStack = g_HudUiChatMessageStack;

    HudUiChatMessageStack *const chatStack =
        (HudUiChatMessageStack *)(::operator new(sizeof(HudUiChatMessageStack)));
    chatStack->Constructor();
    chatStack->enabled = 1;
    g_HudUiChatMessageStack = chatStack;

    NetPkt0B_ChatMessage packet = {};
    packet.messageLength = 5;
    std::memcpy(
        packet.message,
        "hello",
        5
    );
    GameNet::HandlePkt0B_ChatMessage(
        0,
        &packet
    );

    HudUiPanel *const firstLine = &chatStack->lines[0];
    const bool shortMessage =
        std::strcmp(
            firstLine->cachedText,
            "hello"
        ) == 0 &&
        FieldAt<float>(
            firstLine,
            0x10
        ) == 5.0f;

    NetPkt0B_ChatMessage longPacket = {};
    longPacket.messageLength = 0x55;
    for (std::size_t index = 0; index < sizeof(longPacket.message); ++index) {
        longPacket.message[index] = (char)('A' + (index % 26));
    }

    GameNet::HandlePkt0B_ChatMessage(
        0,
        &longPacket
    );
    const bool clamped =
        std::strlen(firstLine->cachedText) == sizeof(longPacket.message) &&
        std::memcmp(
            firstLine->cachedText,
            longPacket.message,
            sizeof(longPacket.message)
        ) == 0 &&
        FieldAt<float>(
            firstLine,
            0x10
        ) == 5.0f;

    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x10203040;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x10203040;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(
        g_sendPacketBytes,
        0,
        sizeof(g_sendPacketBytes)
    );

    GameNet::SendPkt0B_ChatMessage("hello");

    const NetPkt0B_ChatMessage *const sentPacket =
        (const NetPkt0B_ChatMessage *)(g_sendPacketBytes);
    const bool sentShort =
        g_sendCalls == 1 && g_sendFlags == 1 && g_sendPacketSize == 17 &&
        g_sendPacketBytesSize == 17 && sentPacket->header.packetType == 0x0b &&
        sentPacket->header.packetSizeBytes == 17 &&
        sentPacket->header.payloadDword0 == 0x10203040 &&
        sentPacket->messageLength == 5 &&
        std::memcmp(
            sentPacket->message,
            "hello",
            5
        ) == 0 &&
        g_sendPacketBytes[15] == 0 && g_sendPacketBytes[16] == 0;

    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(
        g_sendPacketBytes,
        0xff,
        sizeof(g_sendPacketBytes)
    );

    GameNet::SendPkt0B_ChatMessage("");

    const NetPkt0B_ChatMessage *const emptyPacket =
        (const NetPkt0B_ChatMessage *)(g_sendPacketBytes);
    const bool sentEmpty =
        g_sendCalls == 1 && g_sendFlags == 1 && g_sendPacketSize == 12 &&
        g_sendPacketBytesSize == 12 && emptyPacket->header.packetType == 0x0b &&
        emptyPacket->header.packetSizeBytes == 12 &&
        emptyPacket->header.payloadDword0 == 0x10203040 &&
        emptyPacket->messageLength == 0 && g_sendPacketBytes[10] == 0 &&
        g_sendPacketBytes[11] == 0;

    g_HudUiChatMessageStack = oldChatStack;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldTcpIpAsync;
    chatStack->DestructorCore();
    ::operator delete(chatStack);

    return shortMessage && clamped && sentShort && sentEmpty ? 0 : 1;
}

extern "C" int gamenet_show_player_kill_message_smoke(void) {
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;

    HudUiTopMessageStack *const topStack =
        (HudUiTopMessageStack *)(::operator new(sizeof(HudUiTopMessageStack)));
    topStack->Constructor();
    topStack->enabled = 1;
    g_HudUiTopMessageStack = topStack;

    GameNetPlayerRow *const victim =
        (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    GameNetPlayerRow *const killer =
        (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(
        victim,
        0,
        sizeof(*victim)
    );
    std::memset(
        killer,
        0,
        sizeof(*killer)
    );
    std::strcpy(
        victim->displayName,
        "Victim"
    );
    std::strcpy(
        killer->displayName,
        "Killer"
    );
    OptCatalogEntryDef killEntry = {};
    killEntry.killVerbString = (char *)("tagged");

    GameNet::ShowPlayerKillMessage(
        victim,
        &killEntry,
        killer
    );

    HudUiPanel *const firstLine = &topStack->lines[0];
    const bool ok =
        std::strcmp(
            firstLine->cachedText,
            "Victim tagged Killer"
        ) == 0 &&
        FieldAt<float>(
            firstLine,
            0x10
        ) == 2.0f;

    g_HudUiTopMessageStack = oldTopStack;
    topStack->DestructorCore();
    ::operator delete(topStack);
    ::operator delete(killer);
    ::operator delete(victim);
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

    HudUiTopMessageStack *const topStack =
        (HudUiTopMessageStack *)(::operator new(sizeof(HudUiTopMessageStack)));
    topStack->Constructor();
    topStack->enabled = 1;
    g_HudUiTopMessageStack = topStack;

    HudUiTriplet triplet;
    triplet.Constructor();
    HudUiStatsListElement statsList = {};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    OptCatalogEntryDef killEntry = {};
    killEntry.keyName = (char *)("test_weapon");
    killEntry.ordinalIndex = 3;
    killEntry.killVerbString = (char *)("tagged");
    g_OptCatalog_EntryCount = 1;
    g_OptCatalog_EntryTable = &killEntry;

    GameNetPlayerRow *const killer =
        (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    GameNetPlayerRow *const victim =
        (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(
        killer,
        0,
        sizeof(*killer)
    );
    std::memset(
        victim,
        0,
        sizeof(*victim)
    );

    killer->playerKey = 0x11;
    killer->lapCount = 1;
    killer->playerColorPackedRgb = 0x00112233;
    std::strcpy(
        killer->displayName,
        "Killer"
    );

    victim->playerKey = 0x22;
    victim->score = 4;
    victim->lapCount = 2;
    victim->playerColorPackedRgb = 0x00445566;
    std::strcpy(
        victim->displayName,
        "Victim"
    );
    killer->next = victim;

    g_GameNetPlayerRowHead = killer;
    g_GameNetPlayerRowTail = victim;
    g_GameNetPlayerRowCount = 2;
    g_HudSensorTracker.raceCheckpointMode = 0;
    g_HudSensorTracker.runtimeGoalValue = 999;
    g_GameNetOneLapLeftMessageShown = 0;
    GameNet::RefreshPlayerListMenu(killer);
    GameNet::RefreshPlayerListMenu(victim);

    NetPkt08_PlayerKillEvent packet = {};
    packet.killMethodOrOptCatalogEntryId = 3;
    packet.targetPlayerKey = victim->playerKey;
    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;

    const std::int32_t nonHostResult = GameNet::HandlePkt08_PlayerKillEvent(
        killer->playerKey,
        &packet
    );
    HudUiPanel *const firstLine = &topStack->lines[0];
    const bool nonHost =
        nonHostResult == 1 && victim->score == 4 && g_sendCalls == 0 &&
        std::strcmp(
            firstLine->cachedText,
            "Victim tagged Killer"
        ) == 0;

    packet.targetPlayerKey = 0x7777;
    const bool missingRow = GameNet::HandlePkt08_PlayerKillEvent(
        killer->playerKey,
        &packet
    ) == 0;

    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x4444;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x5678;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;

    packet.targetPlayerKey = victim->playerKey;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(
        g_sendPacketBytes,
        0,
        sizeof(g_sendPacketBytes)
    );
    const std::int32_t hostResult = GameNet::HandlePkt08_PlayerKillEvent(
        killer->playerKey,
        &packet
    );
    const ScoreboardPacket2 *const hostSentPacket =
        (const ScoreboardPacket2 *)(g_sendPacketBytes);
    const bool host =
        hostResult == 1 && victim->score == 5 && g_sendCalls == 1 &&
        g_sendPacketSize == sizeof(ScoreboardPacket2) &&
        hostSentPacket->entries[1].playerKey == victim->playerKey &&
        hostSentPacket->entries[1].packedScoreAndLapCount ==
            (std::uint16_t)((victim->lapCount << 9) | 5);

    packet.targetPlayerKey = killer->playerKey;
    killer->score = 0;
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(
        g_sendPacketBytes,
        0,
        sizeof(g_sendPacketBytes)
    );
    const std::int32_t suicideResult = GameNet::HandlePkt08_PlayerKillEvent(
        killer->playerKey,
        &packet
    );
    const ScoreboardPacket2 *const suicideSentPacket =
        (const ScoreboardPacket2 *)(g_sendPacketBytes);
    const bool suicide =
        suicideResult == 1 && killer->score == 0 && g_sendCalls == 1 &&
        suicideSentPacket->entries[0].playerKey == killer->playerKey &&
        suicideSentPacket->entries[0].packedScoreAndLapCount ==
            (std::uint16_t)(killer->lapCount << 9);

    zUtil_SaveGameState saveState = {};
    saveState.netPlayerRow = victim;
    g_zNetwork_LocalPlayerKey = killer->playerKey;
    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(
        g_sendPacketBytes,
        0,
        sizeof(g_sendPacketBytes)
    );
    GameNet::SendPkt08_PlayerKillEvent(
        &saveState,
        3
    );
    const NetPkt08_PlayerKillEvent *const sentKillPacket =
        (const NetPkt08_PlayerKillEvent *)(g_sendPacketBytes);
    const bool explicitSaveStateSend =
        g_sendCalls == 1 && g_sendPacketSize == sizeof(NetPkt08_PlayerKillEvent) &&
        sentKillPacket->header.packetType == 0x08 &&
        sentKillPacket->header.packetSizeBytes == sizeof(NetPkt08_PlayerKillEvent) &&
        sentKillPacket->header.payloadDword0 == killer->playerKey &&
        sentKillPacket->killMethodOrOptCatalogEntryId == 3 &&
        sentKillPacket->targetPlayerKey == victim->playerKey;

    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)(&saveState);
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(
        g_sendPacketBytes,
        0,
        sizeof(g_sendPacketBytes)
    );
    GameNet::SendPkt08_PlayerKillEvent(
        0,
        3
    );
    const NetPkt08_PlayerKillEvent *const fallbackKillPacket =
        (const NetPkt08_PlayerKillEvent *)(g_sendPacketBytes);
    const bool fallbackSaveStateSend =
        g_sendCalls == 1 && fallbackKillPacket->header.payloadDword0 == killer->playerKey &&
        fallbackKillPacket->targetPlayerKey == victim->playerKey;

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
    topStack->DestructorCore();
    ::operator delete(topStack);
    triplet.DestructorCore();
    ::operator delete(victim);
    ::operator delete(killer);

    return nonHost && missingRow && host && suicide && explicitSaveStateSend &&
                   fallbackSaveStateSend
               ? 0
               : 1;
}

extern "C" int gamenet_send_pkt13_effect_anim_activation_record_smoke(void) {
    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    const int oldSuppressEcho = g_GameNetSuppressPkt13ActivationEcho;

    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x12345678;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x12345678;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_GameNetSuppressPkt13ActivationEcho = 0;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(
        g_sendPacketBytes,
        0,
        sizeof(g_sendPacketBytes)
    );

    zEffectAnimActivationRecord record = {};
    std::memset(
        &record,
        0xab,
        sizeof(record)
    );
    record.commandType = 2;
    GameNet::SendPkt13_EffectAnimActivationRecord(&record);

    const zNetworkPacketHeader *const header =
        (const zNetworkPacketHeader *)(g_sendPacketBytes);
    const bool sentOk =
        g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacketSize == sizeof(zNetworkPacketHeader) + 0x48 &&
        header->packetType == 0x13 &&
        header->packetSizeBytes == sizeof(zNetworkPacketHeader) + 0x48 &&
        header->payloadDword0 == 0x12345678 &&
        std::memcmp(
            g_sendPacketBytes + sizeof(zNetworkPacketHeader),
            &record,
            0x48
        ) == 0;

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
    std::strcpy(
        packet.record.animName,
        "missing_pkt13_activation"
    );
    packet.record.nodeToken = 77;

    g_zEffectAnim_ActivationRecordTable = 0;
    g_zEffectAnim_ActivationRecordCount = 0;
    g_GameNetSuppressPkt13ActivationEcho = 0;
    const int missingResult = GameNet::HandlePkt13_EffectAnimActivationRecord(
        0,
        &packet.header
    );
    const bool missingOk = missingResult == 1 && g_GameNetSuppressPkt13ActivationEcho == 0;

    g_zEffectAnim_ActivationRecordTable = &packet.record;
    g_zEffectAnim_ActivationRecordCount = 1;
    const int duplicateResult = GameNet::HandlePkt13_EffectAnimActivationRecord(
        0,
        &packet.header
    );
    const bool duplicateOk = duplicateResult == 1 && g_GameNetSuppressPkt13ActivationEcho == 0;

    g_zEffectAnim_ActivationRecordTable = oldRecordTable;
    g_zEffectAnim_ActivationRecordCount = oldRecordCount;
    g_GameNetSuppressPkt13ActivationEcho = oldSuppressEcho;

    return missingOk && duplicateOk ? 0 : 1;
}

extern "C" int gamenet_send_all_pkt13_effect_anim_activation_records_smoke(void) {
    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHost = g_zNetwork_IsHostFlag;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    const int oldSuppressEcho = g_GameNetSuppressPkt13ActivationEcho;
    zEffectAnimActivationRecord *const oldRecordTable = g_zEffectAnim_ActivationRecordTable;
    const int oldRecordCount = g_zEffectAnim_ActivationRecordCount;

    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x13572468;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = localPlayer.playerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_GameNetSuppressPkt13ActivationEcho = 0;

    zEffectAnimActivationRecord records[2] = {};
    std::memset(
        &records[0],
        0x11,
        sizeof(records[0])
    );
    std::memset(
        &records[1],
        0x22,
        sizeof(records[1])
    );
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
    std::memset(
        g_sendPacketBytes,
        0,
        sizeof(g_sendPacketBytes)
    );
    GameNet::SendAllPkt13_EffectAnimActivationRecords();
    const zNetworkPacketHeader *const header =
        (const zNetworkPacketHeader *)(g_sendPacketBytes);
    const bool hostOk =
        g_sendCalls == 2 &&
        g_sendPacketSize == sizeof(zNetworkPacketHeader) + 0x4c &&
        header->packetType == 0x13 &&
        header->payloadDword0 == localPlayer.playerKey &&
        std::memcmp(
            g_sendPacketBytes + sizeof(zNetworkPacketHeader),
            &records[1],
            0x4c
        ) == 0;

    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_GameNetSuppressPkt13ActivationEcho = oldSuppressEcho;
    g_zEffectAnim_ActivationRecordTable = oldRecordTable;
    g_zEffectAnim_ActivationRecordCount = oldRecordCount;

    return nonHostOk && hostOk ? 0 : 1;
}

extern "C" int gamenet_spawn_remote_player_missing_template_smoke(void) {
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    zNetworkPlayerRecordList *const oldPlayerRecordList = g_zNetwork_PlayerRecordList;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const unsigned int oldCount = g_GameNetPlayerRowCount;

    zNetwork_PlayerRecord playerRecord = {};
    playerRecord.playerKey = 0x1111;
    std::strcpy(
        playerRecord.playerName,
        "Remote"
    );
    zNetworkPlayerRecordListNode sentinel = {};
    zNetworkPlayerRecordListNode playerNode = {};
    sentinel.next = &playerNode;
    sentinel.prev = &playerNode;
    playerNode.next = &sentinel;
    playerNode.prev = &sentinel;
    playerNode.playerRecord = &playerRecord;
    zNetworkPlayerRecordList playerList = {};
    playerList.sentinelNode = &sentinel;
    playerList.count = 1;
    g_zNetwork_PlayerRecordList = &playerList;

    HudUiTopMessageStack topStack = {};
    topStack.enabled = 0;
    g_HudUiTopMessageStack = &topStack;

    g_GameNetPlayerRowHead = 0;
    g_GameNetPlayerRowTail = 0;
    g_GameNetPlayerRowCount = 0;

    NetPkt06_PlayerStateSnapshot packet = {};
    packet.header.packetType = 6;
    packet.header.payloadDword0 = 0x2222;

    const int result = GameNet::SpawnRemotePlayerFromPkt06_PlayerStateSnapshot(
        (int)playerRecord.playerKey,
        &packet
    );
    const bool ok = result == 0 && g_GameNetPlayerRowHead == 0 &&
                    g_GameNetPlayerRowTail == 0 && g_GameNetPlayerRowCount == 0;

    g_HudUiTopMessageStack = oldTopStack;
    g_zNetwork_PlayerRecordList = oldPlayerRecordList;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;

    return ok ? 0 : 1;
}

extern "C" int hud_sensor_tracker_set_runtime_timer_sec_and_goal_value_smoke(void) {
    HudSensorTracker tracker = {};
    tracker.runtimeGoalValue = 1;
    tracker.runtimeTimerSecRaw = 2;

    tracker.SetRuntimeTimerSecAndGoalValue(
        3600,
        7
    );

    return tracker.runtimeTimerSecRaw == 3600 && tracker.runtimeGoalValue == 7 ? 0 : 1;
}

extern "C" int gamenet_handle_pkt07_alt_gun_dispatch_smoke(void) {
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    int *const oldPendingSpawnTargetCountPtr = g_OptCatalogPendingSpawnTargetCountPtr;
    PlayerProgressTargetSlotRuntime *const oldPendingSpawnTargetListPtr =
        g_OptCatalogPendingSpawnTargetListPtr;
    const std::int32_t oldEntryCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;

    g_GameNetPlayerRowHead = nullptr;
    g_GameNetPlayerRowTail = nullptr;
    g_GameNetPlayerRowCount = 0;

    NetPkt07_AltGunDispatch missingPacket = {};
    missingPacket.header.payloadDword0 = 0x4040;
    const int missingResult = GameNet::HandlePkt07_AltGunDispatch(0x1111, &missingPacket);

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    PlayerGunFireController originalController = {};
    PlayerGunFireController targetController = {};
    playerState.activeAltGunController = &originalController;
    playerState.altGunFireHeldFlag = 1;
    playerState.worldPos = {2.0f, 3.0f, 4.0f};
    playerState.steerBasisRaw = {0.0f, 0.0f, 1.0f};

    OptCatalogEntryDef entry = {};
    entry.keyName = const_cast<char *>("pkt07-alt-gun");
    entry.ordinalIndex = 707;
    g_OptCatalog_EntryCount = 1;
    g_OptCatalog_EntryTable = &entry;
    targetController.optCatalogEntry = &entry;
    playerState.altWeaponBanks[4].controllerA = targetController;

    GameNetPlayerRow *const row = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    std::memset(row, 0, sizeof(*row));
    row->playerKey = 0x3030;
    row->saveState = (GameNetPlayerSaveState *)(&saveState);
    g_GameNetPlayerRowHead = row;
    g_GameNetPlayerRowTail = row;
    g_GameNetPlayerRowCount = 1;

    NetPkt07_AltGunDispatch packet = {};
    packet.header.payloadDword0 = row->playerKey;
    packet.weaponId = 707;
    packet.dispatchFlags = 0x1234;
    packet.targetPos = {10.0f, 11.0f, 12.0f};
    g_OptCatalogPendingSpawnTargetCountPtr = (int *)(0x11112222);
    g_OptCatalogPendingSpawnTargetListPtr =
        (PlayerProgressTargetSlotRuntime *)(0x33334444);

    const int handledResult = GameNet::HandlePkt07_AltGunDispatch(0x1111, &packet);
    int failure = 0;
    if (missingResult != 0) {
        failure = 1;
    } else if (handledResult != 1) {
        failure = 2;
    } else if (playerState.altGunDispatchFlags != 0) {
        failure = 3;
    } else if (playerState.activeAltGunController != &originalController) {
        failure = 4;
    } else if (!Vec3Equals(playerState.storedTargetPos, packet.targetPos)) {
        failure = 5;
    } else if (!Vec3Equals(playerState.altFireOrigin, {2.0f, 4.0f, 4.0f})) {
        failure = 6;
    } else if (g_OptCatalogPendingSpawnTargetCountPtr != nullptr) {
        failure = 7;
    } else if (g_OptCatalogPendingSpawnTargetListPtr != nullptr) {
        failure = 8;
    }

    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_OptCatalogPendingSpawnTargetCountPtr = oldPendingSpawnTargetCountPtr;
    g_OptCatalogPendingSpawnTargetListPtr = oldPendingSpawnTargetListPtr;
    g_OptCatalog_EntryCount = oldEntryCount;
    g_OptCatalog_EntryTable = oldEntryTable;
    ::operator delete(row);

    return failure;
}

extern "C" int gamenet_send_pkt07_alt_gun_dispatch_smoke(void) {
    const NetPkt07_AltGunDispatch oldPacket = g_NetPkt07_AltGunDispatchBuf;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    zUtil_PlayerStateStorage playerState = {};
    playerState.storedTargetPos = {9.0f, 8.0f, 7.0f};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x1234;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x55667788;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_NetPkt07_AltGunDispatchBuf = {{0x07, sizeof(NetPkt07_AltGunDispatch), 0},
                                    0,
                                    0,
                                    0,
                                    {0.0f, 0.0f, 0.0f}};
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

extern "C" int gamenet_alt_gun_dispatch_no_op_callback_smoke(void) {
    OptCatalogEntryDef entry = {};
    void *saveStateSlot = nullptr;
    return GameNet::AltGunDispatchNoOpCallback(&entry, &saveStateSlot) == 1 ? 0 : 1;
}

extern "C" int optcatalog_alt_gun_dispatch_alloc_runtime_gate_smoke(void) {
    const NetPkt07_AltGunDispatch oldPacket = g_NetPkt07_AltGunDispatchBuf;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    OptCatalogEntryDef passEntry = {};
    passEntry.ordinalIndex = 0;
    void *passSlot = reinterpret_cast<void *>(0x11223344u);
    const int passZeroResult =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&passEntry, &passSlot);
    passEntry.ordinalIndex = 1;
    const int passOneResult =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&passEntry, &passSlot);
    const bool passOk = passZeroResult == 1 && passOneResult == 1 &&
                        passSlot == reinterpret_cast<void *>(0x11223344u);

    OptCatalogEntryDef entry = {};
    entry.ordinalIndex = 0x8123;
    void *nullSlot = nullptr;
    const bool nullOk =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&entry, &nullSlot) == 0 &&
        nullSlot == nullptr;

    zUtil_PlayerStateStorage localPlayerState = {};
    localPlayerState.storedTargetPos = {1.0f, 2.0f, 3.0f};
    zUtil_SaveGameState localSaveState = {};
    localSaveState.playerState = &localPlayerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&localSaveState;

    void *vtable[52];
    InitDirectPlayVtable(vtable);
    FakeDirectPlay4 dplay = {vtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x2468;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
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

    zUtil_PlayerStateStorage remotePlayerState = {};
    remotePlayerState.altGunDispatchFlags = 0x01000001;
    zUtil_SaveGameState remoteSaveState = {};
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

extern "C" int optcatalog_handle_pkt0a_remove_runtime_relay_smoke(void) {
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    const std::int32_t oldEntryCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;
    const int oldRelayEnabled = g_OptCatalogProcessRuntimeRelayEnabled;

    static NetPkt0A_RemoveRuntimeRelay packet;
    std::memset(&packet, 0, sizeof(packet));
    packet.ownerPlayerKey = 0x9090;
    const int missingResult = OptCatalog::HandlePkt0A_RemoveRuntimeRelay(0x1111, &packet);

    static zClass_NodePartial ownerRoot;
    std::memset(&ownerRoot, 0, sizeof(ownerRoot));
    static zUtil_PlayerStateStorage playerState;
    std::memset(&playerState, 0, sizeof(playerState));
    playerState.rootNode = &ownerRoot;
    static zUtil_SaveGameState saveState;
    std::memset(&saveState, 0, sizeof(saveState));
    saveState.playerState = &playerState;

    static GameNetPlayerRow row;
    std::memset(&row, 0, sizeof(row));
    row.playerKey = packet.ownerPlayerKey;
    row.saveState = (GameNetPlayerSaveState *)(&saveState);
    g_GameNetPlayerRowHead = &row;
    g_GameNetPlayerRowTail = &row;
    g_GameNetPlayerRowCount = 1;

    static OptCatalogEntryDef entry;
    std::memset(&entry, 0, sizeof(entry));
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
    const NetPkt0A_RemoveRuntimeRelay oldPacket = g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf;
    const int oldRelayEnabled = g_OptCatalogProcessRuntimeRelayEnabled;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;

    OptCatalogEntryDef entry = {};
    entry.ordinalIndex = 0x4567;

    g_OptCatalogProcessRuntimeRelayEnabled = 0;
    g_sendCalls = 0;
    OptCatalog::SendPkt0A_RemoveRuntimeRelay(&entry, nullptr, nullptr);
    const bool disabledOk = g_sendCalls == 0;

    static void *vtable[52];
    InitDirectPlayVtable(vtable);
    static FakeDirectPlay4 dplay;
    dplay.vtable = vtable;
    static zNetwork_PlayerRecord localPlayer;
    std::memset(&localPlayer, 0, sizeof(localPlayer));
    localPlayer.playerKey = 0x1111;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x12345678;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_OptCatalogProcessRuntimeRelayEnabled = 1;

    static zClass_NodePartial ownerNode;
    std::memset(&ownerNode, 0, sizeof(ownerNode));
    g_sendCalls = 0;
    OptCatalog::SendPkt0A_RemoveRuntimeRelay(&entry, nullptr, &ownerNode);
    const bool missingContextOk = g_sendCalls == 0;

    static GameNetPlayerRow ownerRow;
    std::memset(&ownerRow, 0, sizeof(ownerRow));
    ownerRow.playerKey = 0x2468;
    static zUtil_SaveGameState ownerSaveState;
    std::memset(&ownerSaveState, 0, sizeof(ownerSaveState));
    ownerSaveState.netPlayerRow = &ownerRow;
    static HudUiMgrSensorTrackNode trackNode;
    std::memset(&trackNode, 0, sizeof(trackNode));
    trackNode.payload = &ownerSaveState;
    ownerNode.callbackContext = (zClass_NodePartial *)&trackNode;

    g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf =
        {{0x0a, sizeof(NetPkt0A_RemoveRuntimeRelay), 0},
         0,
         0,
         {9.0f, 9.0f, 9.0f},
         0};
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    static zVec3 point;
    point = {4.0f, 5.0f, 6.0f};
    OptCatalog::SendPkt0A_RemoveRuntimeRelay(&entry, &point, &ownerNode);
    const NetPkt0A_RemoveRuntimeRelay *const pointPacket =
        reinterpret_cast<const NetPkt0A_RemoveRuntimeRelay *>(g_sendPacketBytes);
    const bool pointOk =
        g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.header &&
        g_sendPacketSize == sizeof(NetPkt0A_RemoveRuntimeRelay) &&
        pointPacket->header.payloadDword0 == 0x12345678 &&
        pointPacket->optCatalogEntryId == static_cast<short>(0x4567) &&
        Vec3Equals(pointPacket->pointOrVec3, point) &&
        pointPacket->ownerPlayerKey == ownerRow.playerKey;

    g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.pointOrVec3 = {9.0f, 9.0f, 9.0f};
    g_sendCalls = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    OptCatalog::SendPkt0A_RemoveRuntimeRelay(&entry, nullptr, &ownerNode);
    const NetPkt0A_RemoveRuntimeRelay *const zeroPacket =
        reinterpret_cast<const NetPkt0A_RemoveRuntimeRelay *>(g_sendPacketBytes);
    const bool zeroOk =
        g_sendCalls == 1 && zeroPacket->pointOrVec3.x == 0.0f &&
        zeroPacket->pointOrVec3.y == 0.0f && zeroPacket->pointOrVec3.z == 0.0f &&
        zeroPacket->ownerPlayerKey == ownerRow.playerKey;

    g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf = oldPacket;
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

    zDEClient_QSandEventTemplate eventTemplate = {};
    eventTemplate.radius = 12.5f;
    eventTemplate.center = {7.0f, 8.0f, 9.0f};

    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;
    const int nonHostResult = GameNet::HostSendPkt10_QSandFeature(&eventTemplate);
    const bool nonHostOk = nonHostResult == 0 && g_sendCalls == 0;

    static void *vtable[52];
    InitDirectPlayVtable(vtable);
    static FakeDirectPlay4 dplay;
    dplay.vtable = vtable;
    static zNetwork_PlayerRecord localPlayer;
    std::memset(&localPlayer, 0, sizeof(localPlayer));
    localPlayer.playerKey = 0x5555;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x12345678;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_NetPkt10_QSandEventSendBuf =
        {{0x10, sizeof(NetPkt10_QSandEvent), 0}, 0x12u, 0, {0.0f, 0.0f, 0.0f}, 0.0f};
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
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;

    return nonHostOk && hostOk ? 0 : 1;
}

extern "C" int gamenet_send_pkt10_qsand_event_smoke(void) {
    const NetPkt10_QSandEvent oldPacket = g_NetPkt10_QSandEventRelayBuf;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHost = g_zNetwork_IsHostFlag;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zDEClient_NetRelayCallback const oldRelayCallback = g_zDEClientQSandNetRelayCallback;
    const zDEClient_QSandEventTemplate oldDefaults = g_zDEClient_QuickSandEventTemplateDefaults;

    zClass_NodePartial ownerRoot = {};
    zClass_NodePartial otherRoot = {};
    zUtil_PlayerStateStorage playerState = {};
    playerState.rootNode = &ownerRoot;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)(&saveState);

    static void *vtable[52];
    InitDirectPlayVtable(vtable);
    static FakeDirectPlay4 dplay;
    dplay.vtable = vtable;
    static zNetwork_PlayerRecord localPlayer;
    std::memset(&localPlayer, 0, sizeof(localPlayer));
    localPlayer.playerKey = 0x13572468;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = localPlayer.playerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_zDEClientQSandNetRelayCallback = QSandRelayCallbackFake;
    g_zDEClient_QuickSandEventTemplateDefaults = {};
    g_zDEClient_QuickSandEventTemplateDefaults.pointCount = 4;

    zDEClient_QSandEventTemplate negativeEvent = {};
    negativeEvent.radius = -2.25f;
    const int negativeResult = GameNet::SendPkt10_QSandEvent(&negativeEvent);
    const bool negativeOk =
        negativeResult == 1 && FloatNear(negativeEvent.radius, 2.25f);

    g_NetPkt10_QSandEventRelayBuf =
        {{0x10, sizeof(NetPkt10_QSandEvent), 0}, 0x12345678u, 0,
         {0.0f, 0.0f, 0.0f}, 0.0f};
    g_sendCalls = 0;
    zDEClient_QSandEventTemplate otherOwnerEvent = {};
    otherOwnerEvent.radius = 5.0f;
    otherOwnerEvent.center = {1.0f, 2.0f, 3.0f};
    otherOwnerEvent.damageOwnerNode = &otherRoot;
    const int otherOwnerResult = GameNet::SendPkt10_QSandEvent(&otherOwnerEvent);
    const bool otherOwnerOk =
        otherOwnerResult == 0 && g_sendCalls == 0 &&
        g_NetPkt10_QSandEventRelayBuf.header.payloadDword0 == 0 &&
        g_NetPkt10_QSandEventRelayBuf.eventFlags == 0x12345678u;

    zDEClient_QSandEventTemplate nonHostEvent = {};
    nonHostEvent.radius = 6.5f;
    nonHostEvent.center = {4.0f, 5.0f, 6.0f};
    nonHostEvent.damageOwnerNode = &ownerRoot;
    g_zNetwork_IsHostFlag = 0;
    g_NetPkt10_QSandEventRelayBuf =
        {{0x10, sizeof(NetPkt10_QSandEvent), 0}, 0x12345678u, 0,
         {0.0f, 0.0f, 0.0f}, 0.0f};
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    const int nonHostResult = GameNet::SendPkt10_QSandEvent(&nonHostEvent);
    const NetPkt10_QSandEvent *const sentPacket =
        (const NetPkt10_QSandEvent *)(g_sendPacketBytes);
    const bool nonHostOk =
        nonHostResult == 0 && g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt10_QSandEventRelayBuf.header &&
        g_sendPacketSize == sizeof(NetPkt10_QSandEvent) &&
        sentPacket->header.payloadDword0 == localPlayer.playerKey &&
        sentPacket->eventFlags == 0x12340000u &&
        Vec3Equals(sentPacket->center, nonHostEvent.center) &&
        FloatNear(sentPacket->radius, 6.5f);

    g_zNetwork_IsHostFlag = 1;
    g_qsandRelayCallbackCount = 0;
    g_qsandRelayCallbackResult = 0;
    g_sendCalls = 0;
    zDEClient_QSandEventTemplate hostEvent = nonHostEvent;
    hostEvent.radius = 7.75f;
    g_NetPkt10_QSandEventRelayBuf =
        {{0x10, sizeof(NetPkt10_QSandEvent), 0}, 0x87654321u, 0,
         {0.0f, 0.0f, 0.0f}, 0.0f};
    const int hostResult = GameNet::SendPkt10_QSandEvent(&hostEvent);
    const bool hostOk =
        hostResult == 0 && g_qsandRelayCallbackCount == 1 && g_sendCalls == 0 &&
        g_NetPkt10_QSandEventRelayBuf.header.payloadDword0 == localPlayer.playerKey &&
        g_NetPkt10_QSandEventRelayBuf.eventFlags == 0x87650000u &&
        Vec3Equals(g_NetPkt10_QSandEventRelayBuf.center, hostEvent.center) &&
        FloatNear(g_NetPkt10_QSandEventRelayBuf.radius, 7.75f);

    g_NetPkt10_QSandEventRelayBuf = oldPacket;
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
    g_sendPacket = 0;
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

    static zModel_MaterialSlot materialSlots[4];
    std::memset(materialSlots, 0, sizeof(materialSlots));
    g_zModel_MatlPool = materialSlots;
    g_zModel_MatlPoolCapacity = 4;
    g_zModel_MatlPoolInUseCount = 4;

    zDEClient_CraterEventTemplate eventTemplate = {};
    eventTemplate.craterMaterialSlot = &materialSlots[2];
    eventTemplate.radius = 6.25f;
    eventTemplate.center = {3.0f, 4.0f, 5.0f};

    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;
    const int nonHostResult = GameNet::HostSendPkt0F_CraterFeature(&eventTemplate);
    const bool nonHostOk = nonHostResult == 0 && g_sendCalls == 0;

    static void *vtable[52];
    InitDirectPlayVtable(vtable);
    static FakeDirectPlay4 dplay;
    dplay.vtable = vtable;
    static zNetwork_PlayerRecord localPlayer;
    std::memset(&localPlayer, 0, sizeof(localPlayer));
    localPlayer.playerKey = 0x7777;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x23456789;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_NetPkt0F_CraterEventSendBuf =
        {{0x0f, sizeof(NetPkt0F_CraterEvent), 0}, 0x21u, -1, {0.0f, 0.0f, 0.0f}, 0.0f};
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
        sentPacket->eventFlags == (0x21u | 0x80u) &&
        sentPacket->craterTypeId == 2 && Vec3Equals(sentPacket->center, eventTemplate.center) &&
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
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;

    return nonHostOk && hostOk ? 0 : 1;
}
