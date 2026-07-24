// Checked-in focused native smoke translation unit, formerly extracted from zhud_ui_tests.cpp.
// Emits only the zHud/HudSensorTracker objective smokes needed by functional manifests.

#include "Battlesport/game_net.h"
#include "Battlesport/briefing.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud_ui_mp_exit_dialog.h"
#include "Battlesport/hud_ui_net_game_setup.h"
#include "Battlesport/hud_ui_net_exit_panel.h"
#include "Battlesport/hud.h"
#include "Battlesport/pickup.h"
#include "GameZRecoil/Time/time.h"
#include "Battlesport/mission.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zClass/cls_stubs.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/include/zclip_alt.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "Battlesport/turret.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <cstddef>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <mmsystem.h>
#include <new>

extern "C" std::uint32_t g_HudUi_InvalidateMask;
extern "C" int g_Hud_MapOverlayRefCount;
extern "C" int g_HudSensorTracker_ObjectiveCommandLocked;
extern "C" float g_HudLineClip_CurrentLeft;
extern "C" float g_HudLineClip_CurrentTop;
extern "C" float g_HudLineClip_CurrentRight;
extern "C" float g_HudLineClip_CurrentBottom;
extern "C" zVec3 g_HudSensor_ClipSegmentStart;
extern "C" zVec3 g_HudSensor_ClipSegmentEnd;
extern "C" HWND g_RecoilApp_hWndMain;
extern float g_zMath_ClipZLowerBound;
extern float g_zMath_ClipZUpperBound;
extern zFMV_Playback *g_HudUiSensorWindowPlayback;

namespace {
template <typename T> T &TestFieldAt(void *base, std::size_t offset) {
    return *reinterpret_cast<T *>(static_cast<std::uint8_t *>(base) + offset);
}

static std::uintptr_t *TestVTable(void *object) {
    return *reinterpret_cast<std::uintptr_t **>(object);
}

bool HudFloatNear(float actual, float expected) {
    const float delta = actual - expected;
    return delta > -0.0001f && delta < 0.0001f;
}

struct TestReticleAttachState {
    std::uint8_t unknown_00[0x0c];
    zClass_NodePartial *projectileNode;
};

struct TestReticleAltGunController {
    OptCatalogEntryDef *optCatalogEntry;
    std::uint8_t unknown_04[0x24];
    TestReticleAttachState *attachState;
};

struct TestReticlePlayerState {
    std::uint8_t unknown_000[0x58c];
    std::int32_t cameraState;
    std::uint8_t unknown_590[0x54];
    TestReticleAltGunController *activeAltGunController;
    std::uint8_t unknown_5e8[0x8e8];
    zClass_NodePartial *rootNode;
};

int g_HudTestLine4Count = 0;
void *g_HudTestLine4FrameBuffer = nullptr;
int g_HudTestLine4Args[4][5] = {};
int g_HudTestLine5Count = 0;
void *g_HudTestLineFrameBuffer = nullptr;
const void *g_HudTestLineClipRect = nullptr;

int g_hudSensorMciSendCommandCount = 0;
MCIDEVICEID g_hudSensorMciDevices[4] = {};
UINT g_hudSensorMciMessages[4] = {};
DWORD_PTR g_hudSensorMciFlags[4] = {};
DWORD_PTR g_hudSensorMciParams[4] = {};
zFMV_Playback *g_hudSensorExpectedClosePlayback = nullptr;
int g_hudSensorCloseParamOk = 0;

MCIERROR WINAPI FakeHudSensorMciSendCommandA(MCIDEVICEID deviceId, UINT message,
                                             DWORD_PTR flags, DWORD_PTR params) {
    const int index = g_hudSensorMciSendCommandCount;
    if (index < 4) {
        g_hudSensorMciDevices[index] = deviceId;
        g_hudSensorMciMessages[index] = message;
        g_hudSensorMciFlags[index] = flags;
        g_hudSensorMciParams[index] = params;
        if (message == 0x804 && params != 0) {
            zFMV_Playback *const *const playbackParam =
                reinterpret_cast<zFMV_Playback *const *>(params);
            if (*playbackParam == g_hudSensorExpectedClosePlayback) {
                g_hudSensorCloseParamOk = 1;
            }
        }
    }
    ++g_hudSensorMciSendCommandCount;
    return 0;
}
int g_HudTestLineArgs[4][5] = {};
int g_HudTestPointOpCount = 0;
void *g_HudTestPointOpFrameBuffer = nullptr;
int g_HudTestPointOpArgs[3] = {};
int g_HudCircleDrawBaseCount = 0;

void __fastcall HudTestImmediateRaster4(void *frameBuffer, int x0, int y0, int x1,
                                             int y1, int color16) {
    if (g_HudTestLine4Count < 4) {
        g_HudTestLine4Args[g_HudTestLine4Count][0] = x0;
        g_HudTestLine4Args[g_HudTestLine4Count][1] = y0;
        g_HudTestLine4Args[g_HudTestLine4Count][2] = x1;
        g_HudTestLine4Args[g_HudTestLine4Count][3] = y1;
        g_HudTestLine4Args[g_HudTestLine4Count][4] = color16;
    }

    ++g_HudTestLine4Count;
    g_HudTestLine4FrameBuffer = frameBuffer;
}

void __fastcall HudTestImmediateRaster5(void *frameBuffer, const void *clipRect, int x0,
                                             int y0, int x1, int y1, int color16) {
    if (g_HudTestLine5Count < 4) {
        g_HudTestLineArgs[g_HudTestLine5Count][0] = x0;
        g_HudTestLineArgs[g_HudTestLine5Count][1] = y0;
        g_HudTestLineArgs[g_HudTestLine5Count][2] = x1;
        g_HudTestLineArgs[g_HudTestLine5Count][3] = y1;
        g_HudTestLineArgs[g_HudTestLine5Count][4] = color16;
    }

    ++g_HudTestLine5Count;
    g_HudTestLineFrameBuffer = frameBuffer;
    g_HudTestLineClipRect = clipRect;
}

void __fastcall HudTestPointOp(void *frameBuffer, int y, int x, int color16) {
    ++g_HudTestPointOpCount;
    g_HudTestPointOpFrameBuffer = frameBuffer;
    g_HudTestPointOpArgs[0] = y;
    g_HudTestPointOpArgs[1] = x;
    g_HudTestPointOpArgs[2] = color16;
}
} // namespace

extern "C" int zhud_sensor_viewport_rect_smoke(void) {
    std::int32_t replicate = 0;
    g_zGame_Options_PointerCache.replicate = &replicate;
    g_HudUiMgrSensorBlock = {};
    g_HudUiMgrSensorBlock.sensorParam = 2.0f;

    HudUiMgrSensor::SetViewportRect(10, 20, 100, 80);
    const bool normal = g_HudUiMgrSensorBlock.sensorRectRaw.left == 10 &&
                        g_HudUiMgrSensorBlock.sensorRectRaw.top == 20 &&
                        g_HudUiMgrSensorBlock.sensorRectRaw.right == 110 &&
                        g_HudUiMgrSensorBlock.sensorRectRaw.bottom == 100 &&
                        g_HudUiMgrSensorBlock.sensorRectScaled.left == 10 &&
                        g_HudUiMgrSensorBlock.sensorRectScaled.top == 20 &&
                        g_HudUiMgrSensorBlock.sensorRectScaled.right == 110 &&
                        g_HudUiMgrSensorBlock.sensorRectScaled.bottom == 100 &&
                        g_HudUiMgrSensorBlock.sensorPiVSrcRect.left == 10.0f &&
                        g_HudUiMgrSensorBlock.sensorPiVSrcRect.top == 20.0f &&
                        g_HudUiMgrSensorBlock.sensorPiVSrcRect.right == 110.0f &&
                        g_HudUiMgrSensorBlock.sensorPiVSrcRect.bottom == 100.0f &&
                        g_HudUiMgrSensorBlock.sensorClampHalfW == 50.0f &&
                        g_HudUiMgrSensorBlock.sensorClampHalfH == 40.0f &&
                        g_zClipAlt_SourceWidth == 100.0f && g_zClipAlt_SourceHeight == 80.0f;

    replicate = 1;
    HudUiMgrSensor::SetViewportRect(11, 21, 101, 81);
    const bool replicated = g_HudUiMgrSensorBlock.sensorRectRaw.left == 11 &&
                            g_HudUiMgrSensorBlock.sensorRectRaw.top == 21 &&
                            g_HudUiMgrSensorBlock.sensorRectRaw.right == 112 &&
                            g_HudUiMgrSensorBlock.sensorRectRaw.bottom == 102 &&
                            g_HudUiMgrSensorBlock.sensorRectScaled.left == 5 &&
                            g_HudUiMgrSensorBlock.sensorRectScaled.top == 10 &&
                            g_HudUiMgrSensorBlock.sensorRectScaled.right == 55 &&
                            g_HudUiMgrSensorBlock.sensorRectScaled.bottom == 50 &&
                            g_HudUiMgrSensorBlock.sensorPiVSrcRect.left == 5.0f &&
                            g_HudUiMgrSensorBlock.sensorPiVSrcRect.top == 10.0f &&
                            g_HudUiMgrSensorBlock.sensorPiVSrcRect.right == 55.0f &&
                            g_HudUiMgrSensorBlock.sensorPiVSrcRect.bottom == 50.0f &&
                            g_HudUiMgrSensorBlock.sensorClampHalfW == 25.0f &&
                            g_HudUiMgrSensorBlock.sensorClampHalfH == 20.0f &&
                            g_zClipAlt_SourceLeft == 5.0f && g_zClipAlt_SourceTop == 10.0f;

    return normal && replicated ? 0 : 1;
}

extern "C" int zhud_sensor_get_fx_rect_smoke(void) {
    g_HudUiMgrSensorFxRect = {7, 9, 107, 89};

    HudUiRect outRect{};
    HudUiMgrSensor::GetFxRect(&outRect);

    return outRect.left == 7 && outRect.top == 9 && outRect.right == 107 &&
                   outRect.bottom == 89
               ? 0
               : 1;
}

extern "C" int hud_sensor_tracker_constructor_smoke(void) {
    HudSensorTracker tracker{};
    tracker.hudScale = 0.25f;
    tracker.raceCheckpointMode = 7;
    tracker.fxPass3Obj = reinterpret_cast<HudUiElement *>(0x13572468);
    tracker.hasPendingPlayerSave = 1;
    tracker.pendingPlayerSave.skipTimerResetOnStart = 1;

    HudSensorTracker *const returned = tracker.Constructor();

    const bool constructorOk =
        tracker.fxPass3Obj == nullptr && HudFloatNear(tracker.hudScale, 1.0f) &&
        tracker.raceCheckpointMode == 0 && tracker.hasPendingPlayerSave == 0 &&
        tracker.pendingPlayerSave.skipTimerResetOnStart == 0;
    const bool resetOk =
        tracker.missionLoaded == 0 && tracker.missionId == 0 &&
        tracker.missionFlags == 1 && tracker.objectiveCount == 0;

    tracker.Shutdown();

    if (returned != &tracker) {
        return 1;
    }
    if (tracker.mapFileVersion != 5 || tracker.mapHeaderDword != 0 ||
        tracker.mapNodeListHead != nullptr || tracker.mapLoadedFlag != 0) {
        return 4;
    }
    if (tracker.trackedSaveStateSelection != nullptr ||
        tracker.trackedWorldOriginPtr != nullptr ||
        tracker.trackedForwardVecPtr != nullptr) {
        return 5;
    }
    if (!HudFloatNear(tracker.mapZoom, 1.0f)) {
        return 6;
    }
    if (!HudFloatNear(tracker.saveStateMarkerMaxDistSq, 202500.0f)) {
        return 7;
    }
    if (!constructorOk) {
        return 2;
    }
    return resetOk ? 0 : 3;
}

extern "C" int hud_sensor_tracker_construct_global_smoke(void) {
    g_HudSensorTracker.hudScale = 0.125f;
    g_HudSensorTracker.fxPass3Obj = reinterpret_cast<HudUiElement *>(0x24681357);
    g_HudSensorTracker.hasPendingPlayerSave = 1;
    g_HudSensorTracker.pendingPlayerSave.skipTimerResetOnStart = 1;

    HudSensorTracker *const returned = HudSensorTracker::ConstructGlobal();

    const bool globalOk =
        returned == &g_HudSensorTracker && g_HudSensorTracker.mapFileVersion == 5 &&
        g_HudSensorTracker.fxPass3Obj == nullptr &&
        HudFloatNear(g_HudSensorTracker.hudScale, 1.0f) &&
        g_HudSensorTracker.hasPendingPlayerSave == 0 &&
        g_HudSensorTracker.pendingPlayerSave.skipTimerResetOnStart == 0 &&
        g_HudSensorTracker.missionLoaded == 0 && g_HudSensorTracker.missionFlags == 1;

    g_HudSensorTracker.Shutdown();

    return globalOk ? 0 : 1;
}

extern "C" int hud_sensor_tracker_shutdown_global_smoke(void) {
    HudSensorTracker::ConstructGlobal();
    g_HudSensorTracker.SetZbdPath("global_shutdown.gs");
    g_HudSensorTracker.loadedMapPath = _strdup("global_shutdown.zmap");
    if (g_HudSensorTracker.loadedMapPath == nullptr) {
        g_HudSensorTracker.Shutdown();
        return 1;
    }
    g_HudSensorTracker.mapLoadedFlag = 1;

    HudSensorTracker::ShutdownGlobal();

    const bool globalOk =
        std::strcmp((const char *)g_HudSensorTracker.zbdPath, "") == 0 &&
        g_HudSensorTracker.mapFileVersion == 5 &&
        g_HudSensorTracker.mapLoadedFlag == 0 &&
        g_HudSensorTracker.loadedMapPath == nullptr &&
        g_HudSensorTracker.mapNodeListHead == nullptr;

    return globalOk ? 0 : 2;
}

extern "C" int hud_sensor_tracker_register_global_on_exit_smoke(void) {
    HudSensorTracker::ConstructGlobal();
    HudSensorTracker::RegisterGlobalOnExit();
    return g_HudSensorTracker.mapFileVersion == 5 &&
                   HudFloatNear(g_HudSensorTracker.hudScale, 1.0f)
               ? 0
               : 1;
}

extern "C" int mission_init_objectives_smoke(void) {
    g_HudSensorTracker.hudScale = 0.25f;
    g_HudSensorTracker.fxPass3Obj = reinterpret_cast<HudUiElement *>(0x12345678);
    g_HudSensorTracker.hasPendingPlayerSave = 1;

    Mission::InitObjectives();

    const bool initialized =
        g_HudSensorTracker.mapFileVersion == 5 &&
        g_HudSensorTracker.fxPass3Obj == nullptr &&
        HudFloatNear(g_HudSensorTracker.hudScale, 1.0f) &&
        g_HudSensorTracker.hasPendingPlayerSave == 0 &&
        g_HudSensorTracker.missionLoaded == 0 && g_HudSensorTracker.missionFlags == 1;

    g_HudSensorTracker.Shutdown();

    return initialized ? 0 : 1;
}

extern "C" int hud_sensor_tracker_write_mission_data_section_smoke(void) {
    struct MissionDataForTest {
        std::int32_t missionId;
        std::int32_t missionFlags;
        std::int32_t currentObjectiveIndex;
        std::int32_t firstIncompleteObjectiveIndex;
        std::int32_t completedObjectiveCount;
        std::int32_t objectiveFlowState;
        std::int32_t objectiveFlowDeadlineSecRaw;
        std::int32_t missionStat0;
        std::int32_t missionStat1;
        std::int32_t missionStat2;
        std::int32_t missionStat3;
        std::int32_t weaponsFoundMask;
        std::int32_t objectiveCompletedFlags[10];
        std::int32_t difficultyMode;
    };

    char tempDir[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "hsm", 0, tempFile) == 0) {
        return 1;
    }

    HANDLE file = CreateFileA(tempFile, GENERIC_READ | GENERIC_WRITE, 0, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempFile);
        return 2;
    }

    int difficulty = 3;
    int *const oldDifficultyOption = g_zGame_Options_PointerCache.gameDifficulty;
    g_zGame_Options_PointerCache.gameDifficulty = &difficulty;

    zZbdManager manager = {};
    manager.indexArchive.hFile = file;
    zZbdSectionHandler handler = {};
    handler.sectionName = "Mission";
    zZbdSectionCallbackCtx callbackCtx = {};
    callbackCtx.manager = &manager;
    callbackCtx.sectionHandler = &handler;

    HudSensorTracker tracker{};
    tracker.missionId = 11;
    tracker.missionFlags = 12;
    tracker.currentObjectiveIndex = 13;
    tracker.firstIncompleteObjectiveIndex = 14;
    tracker.completedObjectiveCount = 15;
    tracker.objectiveFlowState = 16;
    tracker.objectiveFlowDeadlineSecRaw = 17;
    tracker.missionStat0 = 18;
    tracker.missionStat1 = 19;
    tracker.primaryGunDispatchCount = 20;
    tracker.missionStat3 = 21;
    tracker.weaponsFoundMask = 22;
    for (int index = 0; index < 10; ++index) {
        tracker.objectiveSlots[index].completedFlag = index + 30;
    }

    const int result = tracker.WriteMissionDataSection(&callbackCtx);
    MissionDataForTest payload = {};
    DWORD read = 0;
    if (manager.indexArchive.recordCount == 1) {
        SetFilePointer(file, manager.indexArchive.records[0].fileOffset, nullptr, FILE_BEGIN);
        ReadFile(file, &payload, sizeof(payload), &read, nullptr);
    }

    bool payloadOk =
        result == 1 && manager.indexArchive.recordCount == 1 &&
        manager.indexArchive.records != nullptr &&
        std::strcmp(manager.indexArchive.records[0].name, "Mission/MissionData") == 0 &&
        manager.indexArchive.records[0].fileSize == sizeof(payload) && read == sizeof(payload) &&
        payload.missionId == 11 && payload.missionFlags == 12 &&
        payload.currentObjectiveIndex == 13 &&
        payload.firstIncompleteObjectiveIndex == 14 &&
        payload.completedObjectiveCount == 15 && payload.objectiveFlowState == 16 &&
        payload.objectiveFlowDeadlineSecRaw == 17 && payload.missionStat0 == 18 &&
        payload.missionStat1 == 19 && payload.missionStat2 == 20 &&
        payload.missionStat3 == 21 && payload.weaponsFoundMask == 22 &&
        payload.difficultyMode == 3;
    for (int index = 0; index < 10 && payloadOk; ++index) {
        payloadOk = payload.objectiveCompletedFlags[index] == index + 30;
    }

    std::free(manager.indexArchive.records);
    g_zGame_Options_PointerCache.gameDifficulty = oldDifficultyOption;
    CloseHandle(file);
    DeleteFileA(tempFile);

    return payloadOk ? 0 : 3;
}

extern "C" int hud_sensor_tracker_zar_mission_save_callback_smoke(void) {
    struct MissionDataForTest {
        std::int32_t missionId;
        std::int32_t missionFlags;
        std::int32_t currentObjectiveIndex;
        std::int32_t firstIncompleteObjectiveIndex;
        std::int32_t completedObjectiveCount;
        std::int32_t objectiveFlowState;
        std::int32_t objectiveFlowDeadlineSecRaw;
        std::int32_t missionStat0;
        std::int32_t missionStat1;
        std::int32_t missionStat2;
        std::int32_t missionStat3;
        std::int32_t weaponsFoundMask;
        std::int32_t objectiveCompletedFlags[10];
        std::int32_t difficultyMode;
    };

    char tempDir[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "hsc", 0, tempFile) == 0) {
        return 1;
    }

    HANDLE file = CreateFileA(
        tempFile,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY,
        nullptr
    );
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempFile);
        return 2;
    }

    int difficulty = 1;
    int *const oldDifficultyOption = g_zGame_Options_PointerCache.gameDifficulty;
    g_zGame_Options_PointerCache.gameDifficulty = &difficulty;

    zZbdManager manager = {};
    manager.indexArchive.hFile = file;
    zZbdSectionHandler handler = {};
    handler.sectionName = "Mission";
    zZbdSectionCallbackCtx callbackCtx = {};
    callbackCtx.manager = &manager;
    callbackCtx.sectionHandler = &handler;

    HudSensorTracker tracker{};
    tracker.missionId = 23;
    tracker.missionFlags = 24;
    tracker.currentObjectiveIndex = 25;
    tracker.firstIncompleteObjectiveIndex = 26;
    tracker.completedObjectiveCount = 27;
    tracker.objectiveSlots[3].completedFlag = 28;

    const int result = HudSensorTracker::ZarMission_SaveCallback(&callbackCtx, &tracker);

    MissionDataForTest payload = {};
    DWORD read = 0;
    if (manager.indexArchive.recordCount == 1) {
        SetFilePointer(file, manager.indexArchive.records[0].fileOffset, nullptr, FILE_BEGIN);
        ReadFile(file, &payload, sizeof(payload), &read, nullptr);
    }

    const bool payloadOk =
        result == 1 && manager.indexArchive.recordCount == 1 &&
        manager.indexArchive.records != nullptr &&
        std::strcmp(manager.indexArchive.records[0].name, "Mission/MissionData") == 0 &&
        manager.indexArchive.records[0].fileSize == sizeof(payload) && read == sizeof(payload) &&
        payload.missionId == 23 && payload.missionFlags == 24 &&
        payload.currentObjectiveIndex == 25 &&
        payload.firstIncompleteObjectiveIndex == 26 &&
        payload.completedObjectiveCount == 27 &&
        payload.objectiveCompletedFlags[3] == 28 && payload.difficultyMode == 1;

    std::free(manager.indexArchive.records);
    g_zGame_Options_PointerCache.gameDifficulty = oldDifficultyOption;
    CloseHandle(file);
    DeleteFileA(tempFile);

    return payloadOk ? 0 : 3;
}

extern "C" int hud_sensor_tracker_zar_mission_late_save_callback_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "hsl", 0, tempFile) == 0) {
        return 1;
    }

    HANDLE file = CreateFileA(
        tempFile,
        GENERIC_READ | GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY,
        nullptr
    );
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempFile);
        return 2;
    }

    zZbdManager manager = {};
    manager.indexArchive.hFile = file;
    zZbdSectionHandler handler = {};
    handler.sectionName = "MissionLate";
    zZbdSectionCallbackCtx callbackCtx = {};
    callbackCtx.manager = &manager;
    callbackCtx.sectionHandler = &handler;

    HudSensorTracker tracker{};
    HudSensorTracker::ZarMissionLate_SaveCallback(&callbackCtx, &tracker);

    std::uint32_t payload = 0;
    DWORD read = 0;
    if (manager.indexArchive.recordCount == 1) {
        SetFilePointer(file, manager.indexArchive.records[0].fileOffset, nullptr, FILE_BEGIN);
        ReadFile(file, &payload, sizeof(payload), &read, nullptr);
    }

    const bool payloadOk =
        manager.indexArchive.recordCount == 1 && manager.indexArchive.records != nullptr &&
        std::strcmp(manager.indexArchive.records[0].name, "MissionLate/LateMissionData") == 0 &&
        manager.indexArchive.records[0].fileSize == sizeof(payload) && read == sizeof(payload) &&
        payload == 1;

    std::free(manager.indexArchive.records);
    CloseHandle(file);
    DeleteFileA(tempFile);

    return payloadOk ? 0 : 3;
}

extern "C" int hud_sensor_tracker_apply_mission_data_smoke(void) {
    struct MissionDataForTest {
        std::int32_t missionId;
        std::int32_t missionFlags;
        std::int32_t currentObjectiveIndex;
        std::int32_t firstIncompleteObjectiveIndex;
        std::int32_t completedObjectiveCount;
        std::int32_t objectiveFlowState;
        std::int32_t objectiveFlowDeadlineSecRaw;
        std::int32_t missionStat0;
        std::int32_t missionStat1;
        std::int32_t missionStat2;
        std::int32_t missionStat3;
        std::int32_t weaponsFoundMask;
        std::int32_t objectiveCompletedFlags[10];
        std::int32_t difficultyMode;
    };

    int difficulty = 0;
    int *const oldDifficultyOption = g_zGame_Options_PointerCache.gameDifficulty;
    zZbdManager *const oldZbdManager = g_zUtil_ZbdManager;
    g_zGame_Options_PointerCache.gameDifficulty = &difficulty;

    zZbdManager manager = {};
    g_zUtil_ZbdManager = &manager;

    MissionDataForTest firstLoad = {};
    firstLoad.missionId = 8;
    firstLoad.missionFlags = 9;
    firstLoad.completedObjectiveCount = 10;
    firstLoad.difficultyMode = 2;

    HudSensorTracker tracker{};
    tracker.missionId = 0;
    const int firstResult = tracker.ApplyMissionDataAndReload(
        nullptr,
        "MissionData",
        &firstLoad,
        sizeof(firstLoad)
    );
    const bool firstOk =
        firstResult == 1 && tracker.pendingPlayerSave.skipTimerResetOnStart == 1 &&
        tracker.completedObjectiveCount == 10 && difficulty == 2 && tracker.missionId == 8 &&
        tracker.missionFlags == 9 && manager.stopRequested == 1;

    MissionDataForTest restore = {};
    restore.missionId = 8;
    restore.missionFlags = 12;
    restore.currentObjectiveIndex = 1;
    restore.firstIncompleteObjectiveIndex = 2;
    restore.completedObjectiveCount = 3;
    restore.objectiveFlowState = 4;
    restore.objectiveFlowDeadlineSecRaw = 5;
    restore.missionStat0 = 6;
    restore.missionStat1 = 7;
    restore.missionStat2 = 8;
    restore.missionStat3 = 9;
    restore.weaponsFoundMask = 10;
    restore.difficultyMode = 1;
    for (int index = 0; index < 10; ++index) {
        restore.objectiveCompletedFlags[index] = index & 1;
    }

    manager.stopRequested = 0;
    const int restoreResult = HudSensorTracker::ZarMission_RestoreCallback(
        nullptr,
        "MissionData",
        &restore,
        sizeof(restore),
        &tracker
    );

    bool restoreOk =
        restoreResult == 1 && manager.stopRequested == 0 && difficulty == 1 &&
        tracker.missionId == 8 && tracker.missionFlags == 9 &&
        tracker.currentObjectiveIndex == 1 && tracker.firstIncompleteObjectiveIndex == 2 &&
        tracker.completedObjectiveCount == 3 && tracker.objectiveFlowState == 4 &&
        tracker.objectiveFlowDeadlineSecRaw == 5 && tracker.missionStat0 == 6 &&
        tracker.missionStat1 == 7 && tracker.primaryGunDispatchCount == 8 &&
        tracker.missionStat3 == 9 && tracker.weaponsFoundMask == 10;
    for (int index = 0; index < 10 && restoreOk; ++index) {
        restoreOk = tracker.objectiveSlots[index].completedFlag == (index & 1);
    }

    g_zGame_Options_PointerCache.gameDifficulty = oldDifficultyOption;
    g_zUtil_ZbdManager = oldZbdManager;
    return firstOk && restoreOk ? 0 : 1;
}

extern "C" int hud_sensor_tracker_show_objective_pickup_info_smoke(void) {
    alignas(HudUiPanel) std::uint8_t summaryStorage[0x2a4]{};
    alignas(HudUiPanel) std::uint8_t descStorage[0x2a4]{};
    auto *const summary = reinterpret_cast<HudUiPanel *>(summaryStorage);
    auto *const desc = reinterpret_cast<HudUiPanel *>(descStorage);
    summary->ConstructorDefault(nullptr, 0, 0);
    desc->ConstructorDefault(nullptr, 0, 0);

    zVidImagePartial objectiveImage = {};
    zVidImagePartial widgetImage = {};
    widgetImage.width = 16;

    g_HudUiMgrObjectiveSummaryTextPanel = summary;
    g_HudUiMgrObjectiveDescTextPanel = desc;
    g_HudUiMgrObjectiveWidget.image = &widgetImage;
    g_HudUiMgrObjectiveBar.flags = 0x10;
    g_HudUiMgrObjectivePhase = 0;
    g_HudUiMgrObjectiveChatComposeActive = 0;

    OptCatalogEntryDef entry = {};
    entry.description = const_cast<char *>("Pickup objective");
    entry.fireRateInterval = 0.5f;
    entry.range = 120.0f;
    entry.impactProximity = 12.0f;
    entry.damage = 4.5f;
    entry.flags = 0x00080000u | 0x00010000u | 0x00004000u | 0x00000002u;

    PickupType oldPickupType = g_PickupTypes[17];
    g_PickupTypes[17] = {};
    g_PickupTypes[17].optEntry = &entry;
    g_PickupTypes[17].optMetaImage = &objectiveImage;

    HudSensorTracker tracker = {};
    float readTime = 4.0f;
    std::memcpy(&tracker.objectiveReadTimeSecRaw, &readTime, sizeof(readTime));
    const float oldUnscaledTime = g_Time_UnscaledAccumulatedTimeSec;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    g_Time_UnscaledAccumulatedTimeSec = 3.0f;

    tracker.ShowObjectivePickupInfo(1, 1, &entry);
    float deadline = 0.0f;
    std::memcpy(&deadline, &tracker.objectiveFlowDeadlineSecRaw, sizeof(deadline));
    const bool showOk =
        tracker.objectiveUiMode == 4 && deadline == 7.0f &&
        g_HudUiMgrObjectiveSensorRect.image == &objectiveImage &&
        std::strcmp(&TestFieldAt<char>(summary, 0x34), "Pickup objective") == 0 &&
        std::strstr(&TestFieldAt<char>(desc, 0x34), "Damage Proximity") != nullptr &&
        std::strstr(&TestFieldAt<char>(desc, 0x34), "Remote") != nullptr &&
        std::strstr(&TestFieldAt<char>(desc, 0x34), "Lock On") != nullptr;

    g_HudUiMgrObjectivePhase = 1;
    tracker.ShowObjectivePickupInfo(0, 0, &entry);
    const bool hideOk = tracker.objectiveUiMode == 0 && g_HudUiMgrObjectivePhase == 3;

    TestReticleAltGunController altGun{};
    altGun.optCatalogEntry = &entry;
    TestReticlePlayerState playerState{};
    playerState.activeAltGunController = &altGun;
    zInput_GameStateOrMapTablePartial gameState{};
    gameState.playerState = reinterpret_cast<zInput_PlayerStatePartial *>(&playerState);
    g_GameStateOrMapTable = &gameState;

    tracker.objectiveUiMode = 0;
    g_HudUiMgrObjectivePhase = 0;
    tracker.Command_ShowObjectivePickupInfo();
    const bool commandShowOk =
        tracker.objectiveUiMode == 3 &&
        std::strcmp(&TestFieldAt<char>(summary, 0x34), "Pickup objective") == 0;

    g_HudUiMgrObjectivePhase = 1;
    tracker.Command_ShowObjectivePickupInfo();
    const bool commandHideOk = tracker.objectiveUiMode == 0 && g_HudUiMgrObjectivePhase == 3;

    DeleteObject(summary->hFont);
    DeleteObject(desc->hFont);
    summary->hFont = nullptr;
    desc->hFont = nullptr;
    g_PickupTypes[17] = oldPickupType;
    g_Time_UnscaledAccumulatedTimeSec = oldUnscaledTime;
    g_GameStateOrMapTable = oldGameState;
    return showOk && hideOk && commandShowOk && commandHideOk ? 0 : 1;
}

extern "C" int hud_sensor_tracker_objective_panel_visible_smoke(void) {
    alignas(HudUiPanel) std::uint8_t summaryStorage[0x2a4]{};
    alignas(HudUiPanel) std::uint8_t descStorage[0x2a4]{};
    auto *const summary = reinterpret_cast<HudUiPanel *>(summaryStorage);
    auto *const desc = reinterpret_cast<HudUiPanel *>(descStorage);
    summary->ConstructorDefault(nullptr, 0, 0);
    desc->ConstructorDefault(nullptr, 0, 0);

    HMODULE const oldMessagesDll = g_zLoc_MessagesDllHandle;
    HudUiPanel *const oldSummaryPanel = g_HudUiMgrObjectiveSummaryTextPanel;
    HudUiPanel *const oldDescPanel = g_HudUiMgrObjectiveDescTextPanel;
    HudUiPanel *const oldLabelPanel = g_HudUiMgrObjectiveLabelTextPanel;
    const int oldHitCount = g_OptCatalog_DamageFeedbackHitCount;
    void *const oldVolumeScalePtr = g_zSnd_GlobalVolumeScalePtr;
    const int oldFlag10 = g_zSnd_Flag10PlaybackEnabled;
    const int oldSndInitialized = g_zSnd_IsInitialized;
    const int oldSndPreInitialized = g_zSnd_PreInitialized;
    const int oldActiveBackend = g_zSnd_ActiveBackend;
    const float oldHudScale = g_HudSensorTracker.hudScale;
    const float oldUnscaledTime = g_Time_UnscaledAccumulatedTimeSec;
    HudSensorTracker const oldGlobalTracker = g_HudSensorTracker;
    const int oldObjectiveCommandLocked = g_HudSensorTracker_ObjectiveCommandLocked;
    const int oldMapOverlayRefCount = g_Hud_MapOverlayRefCount;
    int *const oldNetworkEnabled = g_zGame_Options_PointerCache.networkEnabled;
    HudUiTimerPanel *const oldTimerPanel = g_HudUiMgrTimerPanel;

    HMODULE messagesDll = LoadLibraryA("support\\messages.dll");
    if (messagesDll == nullptr) {
        messagesDll = LoadLibraryA("..\\..\\..\\..\\support\\messages.dll");
    }
    if (messagesDll == nullptr) {
        DeleteObject(summary->hFont);
        DeleteObject(desc->hFont);
        return 1;
    }

    zVidImagePartial firstImage = {};
    zVidImagePartial currentImage = {};
    zVidImagePartial widgetImage = {};
    widgetImage.width = 16;

    g_zLoc_MessagesDllHandle = messagesDll;
    g_HudUiMgrObjectiveSummaryTextPanel = summary;
    g_HudUiMgrObjectiveDescTextPanel = desc;
    g_HudUiMgrObjectiveLabelTextPanel = summary;
    g_HudUiMgrObjectiveWidget.image = &widgetImage;
    g_HudUiMgrObjectiveChatComposeActive = 0;

    HudSensorTracker tracker = {};
    tracker.objectiveCount = 5;
    tracker.completedObjectiveCount = 2;
    tracker.primaryGunDispatchCount = 8;
    tracker.missionStat0 = 9;
    tracker.missionStat1 = 7;
    tracker.missionStat3 = 11;
    tracker.weaponsFoundMask = 0x15;
    tracker.objectiveMeterSeconds = 125.8f;
    tracker.currentObjectiveIndex = -1;
    tracker.objectiveSlots[0].objectiveImage = &firstImage;
    std::strcpy(tracker.objectiveSlots[0].objectiveTitle, "First objective");
    tracker.objectiveSlots[2].objectiveImage = &currentImage;
    std::strcpy(tracker.objectiveSlots[2].objectiveTitle, "Review title");
    std::strcpy(tracker.objectiveSlots[2].objectiveDesc, "Review description");
    std::strcpy(tracker.objectiveSlots[2].objectiveSummary, "Current summary");
    g_OptCatalog_DamageFeedbackHitCount = 3;

    g_HudUiMgrObjectivePhase = 1;
    tracker.objectiveUiMode = 2;
    tracker.SetObjectivePanelVisible(0);
    const bool hideOk = tracker.objectiveUiMode == 0 && g_HudUiMgrObjectivePhase == 3;

    char objectiveLine[0x80] = {};
    char statLine[0x80] = {};
    char timeLine[0x80] = {};
    if (zLoc::FormatMessage(objectiveLine, 0x40, 0x116, 2, 5, 37) == 0 ||
        zLoc::FormatMessage(statLine, 0x40, 0x117, 7, 7, 11, 0x15) == 0 ||
        zLoc::FormatMessage(timeLine, 0x40, 0x118, 2, 5) == 0) {
        g_zLoc_MessagesDllHandle = oldMessagesDll;
        g_HudUiMgrObjectiveSummaryTextPanel = oldSummaryPanel;
        g_HudUiMgrObjectiveDescTextPanel = oldDescPanel;
        g_HudUiMgrObjectiveLabelTextPanel = oldLabelPanel;
        g_OptCatalog_DamageFeedbackHitCount = oldHitCount;
        FreeLibrary(messagesDll);
        DeleteObject(summary->hFont);
        DeleteObject(desc->hFont);
        return 2;
    }

    char expectedSummary[0x400] = {};
    std::sprintf(expectedSummary, "%s\n%s\n%s", objectiveLine, statLine, timeLine);
    for (char *percent = std::strstr(expectedSummary, "%%"); percent != nullptr;
         percent = std::strstr(percent + 1, "%%")) {
        std::memmove(percent, percent + 1, std::strlen(percent));
    }

    g_HudUiMgrObjectivePhase = 0;
    tracker.SetObjectivePanelVisible(1);
    const bool firstSlotOk =
        tracker.objectiveUiMode == 2 && g_HudUiMgrObjectiveSensorRect.image == &firstImage &&
        std::strcmp(&TestFieldAt<char>(summary, 0x34), "First objective") == 0 &&
        std::strcmp(&TestFieldAt<char>(desc, 0x34), expectedSummary) == 0;

    tracker.currentObjectiveIndex = 2;
    g_HudUiMgrObjectivePhase = 0;
    tracker.SetObjectivePanelVisible(1);
    const bool currentSlotOk =
        tracker.objectiveUiMode == 2 && g_HudUiMgrObjectiveSensorRect.image == &currentImage &&
        std::strcmp(&TestFieldAt<char>(summary, 0x34), "Current summary") == 0 &&
        std::strcmp(&TestFieldAt<char>(desc, 0x34), expectedSummary) == 0;

    tracker.objectiveReviewSfx = nullptr;
    tracker.objectiveUiMode = 0;
    g_HudUiMgrObjectivePhase = 0;
    tracker.Command_ToggleObjectivePanel();
    const bool commandShowOk = tracker.objectiveUiMode == 2;

    g_HudUiMgrObjectivePhase = 1;
    tracker.Command_ToggleObjectivePanel();
    const bool commandHideOk = tracker.objectiveUiMode == 0 && g_HudUiMgrObjectivePhase == 3;

    tracker.firstIncompleteObjectiveIndex = 2;
    tracker.objectiveFlowState = 0;
    g_HudUiMgrObjectivePhase = 0;
    tracker.SetObjectiveReviewVisible(1);
    const bool reviewShowOk =
        tracker.objectiveFlowState == 0x65 && tracker.objectiveUiMode == 1 &&
        g_HudUiMgrObjectiveSensorRect.image == &currentImage &&
        std::strcmp(&TestFieldAt<char>(summary, 0x34), "Review title") == 0 &&
        std::strcmp(&TestFieldAt<char>(desc, 0x34), "Review description") == 0;

    char completeTitle[0x100] = {};
    char *const completeTitleRaw = zLoc::GetMessageString(0x0f0f);
    if (completeTitleRaw == nullptr) {
        g_zLoc_MessagesDllHandle = oldMessagesDll;
        g_HudUiMgrObjectiveSummaryTextPanel = oldSummaryPanel;
        g_HudUiMgrObjectiveDescTextPanel = oldDescPanel;
        g_HudUiMgrObjectiveLabelTextPanel = oldLabelPanel;
        g_OptCatalog_DamageFeedbackHitCount = oldHitCount;
        g_zSnd_GlobalVolumeScalePtr = oldVolumeScalePtr;
        g_zSnd_Flag10PlaybackEnabled = oldFlag10;
        g_HudSensorTracker.hudScale = oldHudScale;
        FreeLibrary(messagesDll);
        DeleteObject(summary->hFont);
        DeleteObject(desc->hFont);
        return 8;
    }
    std::strcpy(completeTitle, completeTitleRaw);

    tracker.firstIncompleteObjectiveIndex = tracker.objectiveCount;
    tracker.currentObjectiveIndex = 2;
    g_HudUiMgrObjectivePhase = 0;
    tracker.SetObjectiveReviewVisible(1);
    const bool reviewCompleteOk =
        tracker.objectiveUiMode == 1 && g_HudUiMgrObjectiveSensorRect.image == &currentImage &&
        std::strcmp(&TestFieldAt<char>(summary, 0x34), "Review title") == 0;

    float globalScale = 0.2f;
    g_zSnd_GlobalVolumeScalePtr = &globalScale;
    g_zSnd_Flag10PlaybackEnabled = 0;
    g_HudSensorTracker.hudScale = 0.625f;
    g_HudUiMgrObjectivePhase = 1;
    tracker.SetObjectiveReviewVisible(0);
    const bool reviewHideOk = tracker.objectiveUiMode == 0 &&
                              g_HudUiMgrObjectivePhase == 3 &&
                              globalScale == 0.625f &&
                              g_zSnd_Flag10PlaybackEnabled == 1;

    zSndSample completeSfx = {};
    zSndSample previousReadSfx = {};
    zSndSample firstReadSfx = {};
    zSndSample thirdReadSfx = {};
    tracker.objectiveCompleteSfx = &completeSfx;
    tracker.objectiveSlots[0].readSoundSample = &firstReadSfx;
    tracker.objectiveSlots[2].readSoundSample = &thirdReadSfx;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 2;
    g_HudSensorTracker.hudScale = 0.75f;

    tracker.objectiveUiMode = 0;
    tracker.objectiveFlowState = 0;
    tracker.firstIncompleteObjectiveIndex = 2;
    g_HudUiMgrObjectivePhase = 0;
    tracker.AdvanceObjectiveState();
    const bool advanceShowReviewOk =
        tracker.objectiveFlowState == 0x65 && tracker.objectiveUiMode == 1 &&
        g_HudUiMgrObjectiveSensorRect.image == &currentImage &&
        std::strcmp(&TestFieldAt<char>(summary, 0x34), "Review title") == 0;

    globalScale = 0.4f;
    g_zSnd_GlobalVolumeScalePtr = &globalScale;
    g_zSnd_Flag10PlaybackEnabled = 0;
    tracker.objectiveUiMode = 1;
    tracker.objectiveFlowState = 0;
    tracker.currentObjectiveReadSound = &previousReadSfx;
    g_HudUiMgrObjectivePhase = 1;
    tracker.AdvanceObjectiveState();
    const bool advanceHideReviewOk =
        tracker.objectiveFlowState == 0x65 && tracker.objectiveUiMode == 0 &&
        g_HudUiMgrObjectivePhase == 3 && globalScale == 0.75f &&
        g_zSnd_Flag10PlaybackEnabled == 1;

    globalScale = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalScale;
    g_zSnd_Flag10PlaybackEnabled = 1;
    g_Time_UnscaledAccumulatedTimeSec = 10.0f;
    float readTime = 2.5f;
    std::memcpy(&tracker.objectiveReadTimeSecRaw, &readTime, sizeof(readTime));
    tracker.objectiveUiMode = 0;
    tracker.objectiveFlowState = 0x64;
    tracker.missionId = 2;
    tracker.currentObjectiveIndex = -1;
    tracker.firstIncompleteObjectiveIndex = 0;
    g_HudUiMgrObjectivePhase = 0;
    tracker.AdvanceObjectiveState();
    float deadline = 0.0f;
    std::memcpy(&deadline, &tracker.objectiveFlowDeadlineSecRaw, sizeof(deadline));
    const bool advanceSequentialOk =
        tracker.currentObjectiveReadSound == &firstReadSfx &&
        tracker.objectiveFlowState == 0x68 && deadline == 12.5f &&
        tracker.objectiveUiMode == 2 && tracker.hudScale == 1.0f &&
        globalScale == 0.600000024f && g_zSnd_Flag10PlaybackEnabled == 0;

    globalScale = 0.5f;
    g_zSnd_GlobalVolumeScalePtr = &globalScale;
    g_zSnd_Flag10PlaybackEnabled = 1;
    tracker.objectiveUiMode = 0;
    tracker.objectiveFlowState = 0x67;
    tracker.currentObjectiveIndex = 0;
    tracker.firstIncompleteObjectiveIndex = 2;
    tracker.currentObjectiveReadSound = nullptr;
    g_HudUiMgrObjectivePhase = 0;
    tracker.AdvanceObjectiveState();
    const bool advanceJumpOk =
        tracker.currentObjectiveReadSound == &thirdReadSfx &&
        tracker.objectiveFlowState == 0x69 && tracker.hudScale == 0.5f &&
        globalScale == 0.300000012f && g_zSnd_Flag10PlaybackEnabled == 0;

    int networkEnabled = 0;
    g_zGame_Options_PointerCache.networkEnabled = &networkEnabled;
    g_HudSensorTracker = {};
    g_HudSensorTracker.objectiveCount = 5;
    g_HudSensorTracker.objectiveCompleteSfx = &completeSfx;
    g_HudSensorTracker.objectiveSlots[0].objectiveImage = &firstImage;
    g_HudSensorTracker.objectiveSlots[0].readSoundSample = &firstReadSfx;
    g_HudSensorTracker.objectiveSlots[2].objectiveImage = &currentImage;
    g_HudSensorTracker.objectiveSlots[2].readSoundSample = &thirdReadSfx;
    std::strcpy(g_HudSensorTracker.objectiveSlots[0].objectiveTitle, "First objective");
    std::strcpy(g_HudSensorTracker.objectiveSlots[2].objectiveTitle, "Review title");
    std::strcpy(g_HudSensorTracker.objectiveSlots[2].objectiveDesc, "Review description");
    std::strcpy(g_HudSensorTracker.objectiveSlots[2].objectiveSummary, "Current summary");

    g_HudSensorTracker_ObjectiveCommandLocked = 1;
    g_HudSensorTracker.objectiveUiMode = 0;
    g_HudSensorTracker.objectiveFlowState = 0;
    HudSensorTracker::OnObjectiveCommand(0x18);
    const bool commandLockedOk =
        g_HudSensorTracker.objectiveUiMode == 0 && g_HudSensorTracker.objectiveFlowState == 0;

    g_HudSensorTracker_ObjectiveCommandLocked = 0;
    g_HudSensorTracker.firstIncompleteObjectiveIndex = 2;
    g_HudUiMgrObjectivePhase = 0;
    HudSensorTracker::OnObjectiveCommand(0x18);
    const bool commandAdvanceOk =
        g_HudSensorTracker.objectiveUiMode == 1 &&
        g_HudSensorTracker.objectiveFlowState == 0x65;

    g_HudSensorTracker.objectiveUiMode = 0;
    g_HudSensorTracker.currentObjectiveIndex = -1;
    g_HudUiMgrObjectivePhase = 0;
    HudSensorTracker::OnObjectiveCommand(0x19);
    const bool commandPanelOk = g_HudSensorTracker.objectiveUiMode == 2;

    g_HudSensorTracker.mapScaleLerpActive = 1;
    g_HudSensorTracker.mapScaleCurrent.x = 4.0f;
    g_HudSensorTracker.mapScaleCurrent.y = 5.0f;
    g_HudSensorTracker.mapScaleCurrent.z = 6.0f;
    g_HudSensorTracker.mapLoadedFlag = 0;
    g_Hud_MapOverlayRefCount = 1;
    HudSensorTracker::OnObjectiveCommand(0x1b);
    const bool commandMapToggleOk =
        g_HudSensorTracker.mapScaleLerpActive == 0 && g_Hud_MapOverlayRefCount == 0;

    g_HudSensorTracker.mapScaleLerpActive = 1;
    g_HudSensorTracker.mapZoom = 10.0f;
    g_HudSensorTracker.mapSndClick = &completeSfx;
    HudSensorTracker::OnObjectiveCommand(0x1c);
    HudSensorTracker::OnObjectiveCommand(0x1d);
    const bool commandZoomOk = g_HudSensorTracker.mapZoom > 9.89f &&
                               g_HudSensorTracker.mapZoom < 9.91f;

    g_HudSensorTracker.hudScale = 0.875f;
    g_HudSensorTracker.firstIncompleteObjectiveIndex = 2;
    g_HudSensorTracker.objectiveUiMode = 0;
    g_HudSensorTracker.objectiveFlowState = 0;
    g_HudUiMgrObjectivePhase = 0;
    HudSensorTracker::OnObjectiveReadSoundEvent(0);
    const bool readEventShowOk =
        g_HudSensorTracker.objectiveFlowState == 0x65 &&
        g_HudSensorTracker.objectiveUiMode == 1 &&
        g_HudUiMgrObjectiveSensorRect.image == &currentImage &&
        std::strcmp(&TestFieldAt<char>(summary, 0x34), "Review title") == 0 &&
        std::strcmp(&TestFieldAt<char>(desc, 0x34), "Review description") == 0;

    globalScale = 0.25f;
    g_zSnd_GlobalVolumeScalePtr = &globalScale;
    g_zSnd_Flag10PlaybackEnabled = 0;
    g_HudUiMgrObjectivePhase = 1;
    HudSensorTracker::OnObjectiveReadSoundEvent(1);
    const bool readEventHideOk =
        g_HudSensorTracker.objectiveUiMode == 0 &&
        g_HudUiMgrObjectivePhase == 3 &&
        globalScale == 0.875f &&
        g_zSnd_Flag10PlaybackEnabled == 1;

    globalScale = 0.1f;
    g_zSnd_GlobalVolumeScalePtr = &globalScale;
    g_zSnd_Flag10PlaybackEnabled = 0;
    HudSensorTracker::OnObjectiveReadSoundEvent(2);
    const bool readEventRestoreOk =
        globalScale == 0.875f && g_zSnd_Flag10PlaybackEnabled == 1;

    globalScale = 0.2f;
    g_zSnd_GlobalVolumeScalePtr = &globalScale;
    g_zSnd_Flag10PlaybackEnabled = 0;
    HudSensorTracker::OnObjectiveReadSoundEvent(99);
    const bool readEventIgnoreOk =
        globalScale == 0.2f && g_zSnd_Flag10PlaybackEnabled == 0;

    HudUiTimerPanel timerPanel{};
    g_HudUiMgrTimerPanel = &timerPanel;
    TestFieldAt<float>(&timerPanel, 0x2a4) = 33.25f;

    zClass_NodePartial activationNode{};
    activationNode.flags = 4;
    HudSensorTracker updateTracker{};
    updateTracker.objectiveCount = 3;
    updateTracker.objectiveReviewDelaySecRaw = 0;
    float updateSeconds = 3.0f;
    std::memcpy(&updateTracker.objectiveReviewDelaySecRaw, &updateSeconds,
                sizeof(updateSeconds));
    updateTracker.objectiveSlots[0].completedFlag = 1;
    updateTracker.objectiveSlots[1].activationNode = &activationNode;
    updateTracker.objectiveIncomingSfx = &completeSfx;
    updateTracker.objectiveCompleteSfx = &completeSfx;
    updateTracker.objectiveSlots[2].objectiveImage = &currentImage;
    updateTracker.objectiveSlots[2].readSoundSample = &thirdReadSfx;
    std::strcpy(updateTracker.objectiveSlots[2].objectiveTitle, "Review title");
    std::strcpy(updateTracker.objectiveSlots[2].objectiveDesc, "Review description");
    std::strcpy(updateTracker.objectiveSlots[2].objectiveSummary, "Current summary");

    networkEnabled = 0;
    g_Time_UnscaledAccumulatedTimeSec = 10.0f;
    updateTracker.UpdateObjectiveFlow();
    std::memcpy(&deadline, &updateTracker.objectiveFlowDeadlineSecRaw, sizeof(deadline));
    const bool updateCompletesObjectiveOk =
        updateTracker.objectiveMeterSeconds == 33.25f &&
        updateTracker.firstIncompleteObjectiveIndex == 1 &&
        updateTracker.objectiveSlots[1].completedFlag == 1 &&
        updateTracker.completedObjectiveCount == 1 &&
        updateTracker.objectiveFlowState == 0x67 &&
        updateTracker.currentObjectiveIndex == 1 && deadline == 13.0f;

    g_Time_UnscaledAccumulatedTimeSec = 14.0f;
    updateTracker.UpdateObjectiveFlow();
    const bool updateDeadlineShowsMeterOk =
        updateTracker.objectiveFlowState == 0x6b &&
        updateTracker.firstIncompleteObjectiveIndex == 2;

    updateTracker.objectiveFlowState = 0x68;
    updateSeconds = 12.0f;
    std::memcpy(&updateTracker.objectiveFlowDeadlineSecRaw, &updateSeconds,
                sizeof(updateSeconds));
    g_Time_UnscaledAccumulatedTimeSec = 12.0f;
    g_HudUiMgrObjectivePhase = 1;
    updateTracker.UpdateObjectiveFlow();
    const bool updateDeadlineHidesPanelOk =
        updateTracker.objectiveFlowState == 0x69 && g_HudUiMgrObjectivePhase == 3;

    updateSeconds = 2.0f;
    std::memcpy(&updateTracker.objectiveReadTimeSecRaw, &updateSeconds,
                sizeof(updateSeconds));
    updateTracker.objectiveFlowState = 0x6b;
    updateTracker.firstIncompleteObjectiveIndex = 2;
    updateTracker.currentObjectiveIndex = 1;
    updateTracker.objectiveSlots[2].autoplayFlag = 1;
    g_HudUiMgrObjectivePhase = 0;
    g_Time_UnscaledAccumulatedTimeSec = 20.0f;
    updateTracker.UpdateObjectiveFlow();
    std::memcpy(&deadline, &updateTracker.objectiveFlowDeadlineSecRaw, sizeof(deadline));
    const bool updateAutoplayAdvanceOk =
        updateTracker.currentObjectiveReadSound == &thirdReadSfx &&
        updateTracker.objectiveFlowState == 0x68 && deadline == 22.0f;

    TestReticleAltGunController updateAltGun{};
    TestReticlePlayerState updatePlayerState{};
    updatePlayerState.activeAltGunController = &updateAltGun;
    zInput_GameStateOrMapTablePartial updateGameState{};
    updateGameState.playerState = reinterpret_cast<zInput_PlayerStatePartial *>(
        &updatePlayerState);
    g_GameStateOrMapTable = &updateGameState;
    updateTracker.objectiveUiMode = 4;
    updateSeconds = 19.0f;
    std::memcpy(&updateTracker.objectiveFlowDeadlineSecRaw, &updateSeconds,
                sizeof(updateSeconds));
    g_Time_UnscaledAccumulatedTimeSec = 19.0f;
    g_HudUiMgrObjectivePhase = 1;
    updateTracker.UpdateObjectiveFlow();
    const bool updatePickupAutoHideOk =
        updateTracker.objectiveUiMode == 0 && g_HudUiMgrObjectivePhase == 3;

    g_zLoc_MessagesDllHandle = oldMessagesDll;
    g_HudUiMgrObjectiveSummaryTextPanel = oldSummaryPanel;
    g_HudUiMgrObjectiveDescTextPanel = oldDescPanel;
    g_HudUiMgrObjectiveLabelTextPanel = oldLabelPanel;
    g_OptCatalog_DamageFeedbackHitCount = oldHitCount;
    g_zSnd_GlobalVolumeScalePtr = oldVolumeScalePtr;
    g_zSnd_Flag10PlaybackEnabled = oldFlag10;
    g_zSnd_ActiveBackend = oldActiveBackend;
    g_zSnd_PreInitialized = oldSndPreInitialized;
    g_zSnd_IsInitialized = oldSndInitialized;
    g_HudSensorTracker.hudScale = oldHudScale;
    g_Time_UnscaledAccumulatedTimeSec = oldUnscaledTime;
    g_HudSensorTracker = oldGlobalTracker;
    g_HudSensorTracker_ObjectiveCommandLocked = oldObjectiveCommandLocked;
    g_Hud_MapOverlayRefCount = oldMapOverlayRefCount;
    g_zGame_Options_PointerCache.networkEnabled = oldNetworkEnabled;
    g_HudUiMgrTimerPanel = oldTimerPanel;
    FreeLibrary(messagesDll);
    DeleteObject(summary->hFont);
    DeleteObject(desc->hFont);
    summary->hFont = nullptr;
    desc->hFont = nullptr;

    if (!hideOk) {
        return 3;
    }
    if (!firstSlotOk) {
        return 4;
    }
    if (!currentSlotOk) {
        return 5;
    }
    if (!commandShowOk) {
        return 6;
    }
    if (!commandHideOk) {
        return 7;
    }
    if (!reviewShowOk) {
        return 9;
    }
    if (!reviewCompleteOk) {
        return 10;
    }
    if (!reviewHideOk) {
        return 11;
    }
    if (!advanceShowReviewOk) {
        return 12;
    }
    if (!advanceHideReviewOk) {
        return 13;
    }
    if (!advanceSequentialOk) {
        return 14;
    }
    if (!advanceJumpOk) {
        return 15;
    }
    if (!commandLockedOk) {
        return 16;
    }
    if (!commandAdvanceOk) {
        return 17;
    }
    if (!commandPanelOk) {
        return 18;
    }
    if (!commandMapToggleOk) {
        return 19;
    }
    if (!commandZoomOk) {
        return 20;
    }
    if (!readEventShowOk) {
        return 21;
    }
    if (!readEventHideOk) {
        return 22;
    }
    if (!readEventRestoreOk) {
        return 23;
    }
    if (!readEventIgnoreOk) {
        return 24;
    }
    if (!updateCompletesObjectiveOk) {
        return 25;
    }
    if (!updateDeadlineShowsMeterOk) {
        return 26;
    }
    if (!updateDeadlineHidesPanelOk) {
        return 27;
    }
    if (!updateAutoplayAdvanceOk) {
        return 28;
    }
    if (!updatePickupAutoHideOk) {
        return 29;
    }

    return 0;
}

extern "C" int hud_sensor_tracker_shutdown_smoke(void) {
    HudSensorTracker tracker{};
    tracker.Constructor();
    tracker.SetZbdPath("shutdown_test.gs");
    tracker.missionDataPath = "mission.dat";
    tracker.missionGsPath = "mission.gs";
    tracker.loadedMapPath = _strdup("shutdown_map.zmap");
    if (tracker.loadedMapPath == nullptr) {
        tracker.Shutdown();
        return 1;
    }
    tracker.mapLoadedFlag = 1;

    tracker.Shutdown();

    const bool stringOk = std::strcmp((const char *)tracker.zbdPath, "") == 0 &&
                          std::strcmp((const char *)tracker.missionDataPath, "") == 0 &&
                          std::strcmp((const char *)tracker.missionGsPath, "") == 0;
    const bool resetOk =
        tracker.mapFileVersion == 5 && tracker.mapLoadedFlag == 0 &&
        tracker.loadedMapPath == nullptr && tracker.mapNodeListHead == nullptr &&
        tracker.trackedSaveStateSelection == nullptr &&
        HudFloatNear(tracker.mapZoom, 1.0f);

    return stringOk && resetOk ? 0 : 2;
}

extern "C" int hud_sensor_tracker_save_state_marker_smoke(void) {
    void *const oldFrameBuffer = zRndr::g_frameBuffer;
    zRndr::ImmediateRaster5Proc const oldRaster5 = zRndr::g_pfnImmediateRaster5;
    zRndr::PointOpProc const oldPointOp = zRndr::g_pfnPointOpActive;
    int *const oldNetworkEnabled = g_zGame_Options_PointerCache.networkEnabled;
    const int oldRMaskShifted = g_zVideo_PixelPack.rMaskShifted;
    const int oldGMaskShifted = g_zVideo_PixelPack.gMaskShifted;
    const int oldRShift = g_zVideo_PixelPack.packedBase;
    const int oldGShift = g_zVideo_PixelPack.sumMinus8;
    const int oldBShiftTo8 = g_zVideo_PixelPack.bShiftTo8;

    int networkEnabled = 0;
    g_zGame_Options_PointerCache.networkEnabled = &networkEnabled;
    g_zVideo_PixelPack.rMaskShifted = 0xf8;
    g_zVideo_PixelPack.gMaskShifted = 0xfc;
    g_zVideo_PixelPack.packedBase = 8;
    g_zVideo_PixelPack.sumMinus8 = 3;
    g_zVideo_PixelPack.bShiftTo8 = 3;

    HudSensorTracker tracker{};
    zVec3 trackedOrigin{10.0f, 20.0f, 30.0f};
    zVec3 trackedForward{1.0f, 0.0f, 0.0f};
    tracker.trackedWorldOriginPtr = &trackedOrigin;
    tracker.trackedForwardVecPtr = &trackedForward;
    tracker.mapScaleCurrent.x = 1.0f;
    tracker.mapScaleCurrent.z = 1.0f;
    tracker.mapZoom = 1.0f;
    tracker.mapOverlayCenterX = 100;
    tracker.mapOverlayCenterY = 100;
    tracker.outerRect = {0, 0, 200, 200};
    tracker.saveStateMarkerMaxDistSq = 0.0f;

    zUtil_PlayerStateStorage playerState{};
    playerState.lifecycleState = 0;
    playerState.worldPos = {13.0f, 99.0f, 34.0f};
    zUtil_SaveGameState saveState{};
    saveState.playerState = &playerState;
    tracker.trackedSaveStateSelection = &saveState;

    zRndr::g_frameBuffer = reinterpret_cast<void *>(0x16180339);
    zRndr::g_pfnImmediateRaster5 =
        reinterpret_cast<zRndr::ImmediateRaster5Proc>(HudTestImmediateRaster5);
    g_HudTestLine5Count = 0;
    g_HudTestLineFrameBuffer = nullptr;
    g_HudTestLineClipRect = nullptr;
    std::memset(g_HudTestLineArgs, 0, sizeof(g_HudTestLineArgs));

    const int trackedResult = tracker.DrawTrackedSaveStateMarker();
    const bool trackedOk =
        trackedResult == 1 && g_HudTestLine5Count == 2 &&
        g_HudTestLineFrameBuffer == reinterpret_cast<void *>(0x16180339) &&
        g_HudTestLineClipRect == &tracker &&
        g_HudTestLineArgs[0][0] == 101 && g_HudTestLineArgs[0][1] == 97 &&
        g_HudTestLineArgs[0][2] == 107 && g_HudTestLineArgs[0][3] == 97 &&
        g_HudTestLineArgs[0][4] == 0x07e0 &&
        g_HudTestLineArgs[1][0] == 104 && g_HudTestLineArgs[1][1] == 100 &&
        g_HudTestLineArgs[1][2] == 104 && g_HudTestLineArgs[1][3] == 94 &&
        g_HudTestLineArgs[1][4] == 0x07e0;

    zUtil_SaveGameState otherSaveState{};
    otherSaveState.playerState = &playerState;
    tracker.trackedSaveStateSelection = nullptr;
    zRndr::g_pfnPointOpActive = HudTestPointOp;
    g_HudTestPointOpCount = 0;
    g_HudTestPointOpFrameBuffer = nullptr;
    g_HudTestPointOpArgs[0] = 0;
    g_HudTestPointOpArgs[1] = 0;
    g_HudTestPointOpArgs[2] = 0;

    const int saveResult = tracker.DrawSaveStateMarker(&otherSaveState);
    const bool saveOk = saveResult == 1 && g_HudTestPointOpCount == 1 &&
                        g_HudTestPointOpFrameBuffer ==
                            reinterpret_cast<void *>(0x16180339) &&
                        g_HudTestPointOpArgs[0] == 97 &&
                        g_HudTestPointOpArgs[1] == 104 &&
                        g_HudTestPointOpArgs[2] == 0xf800;

    playerState.lifecycleState = 4;
    g_HudTestPointOpCount = 0;
    const bool inactiveOk = tracker.DrawSaveStateMarker(&otherSaveState) == 0 &&
                            g_HudTestPointOpCount == 0;

    zRndr::g_frameBuffer = oldFrameBuffer;
    zRndr::g_pfnImmediateRaster5 = oldRaster5;
    zRndr::g_pfnPointOpActive = oldPointOp;
    g_zGame_Options_PointerCache.networkEnabled = oldNetworkEnabled;
    g_zVideo_PixelPack.rMaskShifted = oldRMaskShifted;
    g_zVideo_PixelPack.gMaskShifted = oldGMaskShifted;
    g_zVideo_PixelPack.packedBase = oldRShift;
    g_zVideo_PixelPack.sumMinus8 = oldGShift;
    g_zVideo_PixelPack.bShiftTo8 = oldBShiftTo8;

    return trackedOk && saveOk && inactiveOk ? 0 : 1;
}

extern "C" int hud_sensor_tracker_update_smoke(void) {
    void *const oldFrameBuffer = zRndr::g_frameBuffer;
    zRndr::ImmediateRaster4Proc const oldRaster4 = zRndr::g_pfnImmediateRaster4;
    zRndr::ImmediateRaster5Proc const oldRaster5 = zRndr::g_pfnImmediateRaster5;
    int *const oldNetworkEnabled = g_zGame_Options_PointerCache.networkEnabled;
    zUtil_SaveGameState *const oldSaveStateListHead = g_PlayerSaveStateListHead;

    HudSensorTracker earlyTracker{};
    earlyTracker.Update();
    const bool earlyOk = HudFloatNear(earlyTracker.mapScaleLerpStep, 0.150000006f) &&
                         earlyTracker.trackedWorldOriginPtr == nullptr;

    int networkEnabled = 0;
    g_zGame_Options_PointerCache.networkEnabled = &networkEnabled;
    g_PlayerSaveStateListHead = nullptr;

    HudSensorTracker tracker{};
    zVec3 trackedOrigin{0.0f, 0.0f, 0.0f};
    zVec3 trackedForward{1.0f, 0.0f, 0.0f};
    tracker.mapScaleLerpActive = 1;
    tracker.trackedWorldOriginPtr = &trackedOrigin;
    tracker.trackedForwardVecPtr = &trackedForward;
    tracker.mapScaleCurrent.x = 1.0f;
    tracker.mapScaleCurrent.z = 1.0f;
    tracker.mapZoom = 1.0f;
    tracker.mapOverlayCenterX = 100;
    tracker.mapOverlayCenterY = 100;
    tracker.outerRect = {0, 0, 200, 200};
    tracker.innerRectExpanded = {0, 0, 10, 10};
    tracker.mapLoadedFlag = 0;

    HudSensorMapPoint points[2] = {{0.0f, 0.0f, 0.0f}, {10.0f, 0.0f, 0.0f}};
    HudSensorMapNode node{};
    node.pointCount = 2;
    node.points = points;
    node.selectedPointIndex = -1;
    node.packedColor565Pair = 0x12345678;
    tracker.mapNodeListHead = &node;

    zRndr::g_frameBuffer = reinterpret_cast<void *>(0x42424242);
    zRndr::g_pfnImmediateRaster4 =
        reinterpret_cast<zRndr::ImmediateRaster4Proc>(HudTestImmediateRaster4);
    zRndr::g_pfnImmediateRaster5 =
        reinterpret_cast<zRndr::ImmediateRaster5Proc>(HudTestImmediateRaster5);
    g_HudTestLine4Count = 0;
    g_HudTestLine4FrameBuffer = nullptr;
    std::memset(g_HudTestLine4Args, 0, sizeof(g_HudTestLine4Args));
    g_HudTestLine5Count = 0;

    tracker.Update();
    const bool updateOk =
        HudFloatNear(tracker.mapScaleLerpStep, 0.150000006f) &&
        g_HudTestLine4Count == 2 && g_HudTestLine5Count == 0 &&
        g_HudTestLine4FrameBuffer == reinterpret_cast<void *>(0x42424242) &&
        g_HudTestLine4Args[0][0] == 100 && g_HudTestLine4Args[0][1] == 100 &&
        g_HudTestLine4Args[0][2] == 100 && g_HudTestLine4Args[0][3] == 90 &&
        g_HudTestLine4Args[0][4] == 0x5678 &&
        g_HudTestLine4Args[1][0] == 100 && g_HudTestLine4Args[1][1] == 90 &&
        g_HudTestLine4Args[1][2] == 100 && g_HudTestLine4Args[1][3] == 100 &&
        g_HudTestLine4Args[1][4] == 0x5678;

    zRndr::g_frameBuffer = oldFrameBuffer;
    zRndr::g_pfnImmediateRaster4 = oldRaster4;
    zRndr::g_pfnImmediateRaster5 = oldRaster5;
    g_zGame_Options_PointerCache.networkEnabled = oldNetworkEnabled;
    g_PlayerSaveStateListHead = oldSaveStateListHead;

    return earlyOk && updateOk ? 0 : 1;
}

extern "C" int hud_sensor_map_node_draw_projected_path_smoke(void) {
    void *const oldFrameBuffer = zRndr::g_frameBuffer;
    zRndr::ImmediateRaster5Proc const oldRaster5 = zRndr::g_pfnImmediateRaster5;
    const float oldProjScaleX = g_zMath_ProjScaleX;
    const float oldProjScaleY = g_zMath_ProjScaleY;
    const float oldProjOffsetX = g_zMath_ProjOffsetX;
    const float oldProjOffsetY = g_zMath_ProjOffsetY;
    const float oldClipLower = g_zMath_ClipZLowerBound;
    const float oldClipUpper = g_zMath_ClipZUpperBound;
    zMat4x3 const oldCameraScratchB = zMath::g_zMath_CameraScratchB;
    int *const oldMatrixIdentitySlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const oldMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;

    int matrixIdentityFlags[2] = {};
    float *matrixSlots[2] = {};
    zMat4x3 baseMatrix{};
    matrixSlots[0] = reinterpret_cast<float *>(&baseMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixIdentityFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    zMath::g_zMath_CameraScratchB = {};
    zMath::g_zMath_CameraScratchB.xx = 1.0f;
    zMath::g_zMath_CameraScratchB.yy = 1.0f;
    zMath::g_zMath_CameraScratchB.zz = 1.0f;
    g_zMath_ProjScaleX = 8.0f;
    g_zMath_ProjScaleY = 6.0f;
    g_zMath_ProjOffsetX = 10.0f;
    g_zMath_ProjOffsetY = 20.0f;
    g_zMath_ClipZLowerBound = 1.0f;
    g_zMath_ClipZUpperBound = 5.0f;

    zRndr::g_frameBuffer = reinterpret_cast<void *>(0x24681357);
    zRndr::g_pfnImmediateRaster5 =
        reinterpret_cast<zRndr::ImmediateRaster5Proc>(HudTestImmediateRaster5);
    g_HudTestLine5Count = 0;
    g_HudTestLineFrameBuffer = nullptr;
    g_HudTestLineClipRect = nullptr;
    std::memset(g_HudTestLineArgs, 0, sizeof(g_HudTestLineArgs));

    HudSensorTracker tracker{};
    HudSensorMapPoint points[2] = {{2.0f, 4.0f, 2.0f}, {6.0f, 8.0f, 2.0f}};
    HudSensorMapNode node{};
    node.pointCount = 2;
    node.points = points;
    node.packedColor565Pair = 0x12345678;

    const int drawResult = node.DrawProjectedPath(&tracker);
    const bool drawOk = drawResult == 1 && g_HudTestLine5Count == 2 &&
                        g_HudTestLineFrameBuffer == reinterpret_cast<void *>(0x24681357) &&
                        g_HudTestLineClipRect == &tracker;
    const bool colorOk = g_HudTestLineArgs[0][4] == 0x1234 &&
                         g_HudTestLineArgs[1][4] == 0x1234;
    const bool lineOk =
        g_HudTestLineArgs[0][0] == 36 && g_HudTestLineArgs[0][1] == 64 &&
        g_HudTestLineArgs[0][2] == 68 && g_HudTestLineArgs[0][3] == 88 &&
        g_HudTestLineArgs[1][0] == 68 && g_HudTestLineArgs[1][1] == 88 &&
        g_HudTestLineArgs[1][2] == 36 && g_HudTestLineArgs[1][3] == 64;

    node.pointCount = 0;
    g_HudTestLine5Count = 0;
    const bool emptyOk = node.DrawProjectedPath(&tracker) == 1 && g_HudTestLine5Count == 0;

    zRndr::g_frameBuffer = oldFrameBuffer;
    zRndr::g_pfnImmediateRaster5 = oldRaster5;
    g_zMath_ProjScaleX = oldProjScaleX;
    g_zMath_ProjScaleY = oldProjScaleY;
    g_zMath_ProjOffsetX = oldProjOffsetX;
    g_zMath_ProjOffsetY = oldProjOffsetY;
    g_zMath_ClipZLowerBound = oldClipLower;
    g_zMath_ClipZUpperBound = oldClipUpper;
    zMath::g_zMath_CameraScratchB = oldCameraScratchB;
    zMath::g_currentMatrixIdentityFlagSlot = oldMatrixIdentitySlot;
    zMath::g_currentMatrixPtrSlot = oldMatrixPtrSlot;

    return drawOk && colorOk && lineOk && emptyOk ? 0 : 1;
}

extern "C" int hud_sensor_map_node_draw_on_tracker_smoke(void) {
    void *const oldFrameBuffer = zRndr::g_frameBuffer;
    zRndr::ImmediateRaster4Proc const oldRaster4 = zRndr::g_pfnImmediateRaster4;
    zRndr::ImmediateRaster5Proc const oldRaster5 = zRndr::g_pfnImmediateRaster5;

    HudSensorTracker tracker{};
    zVec3 trackedOrigin{0.0f, 0.0f, 0.0f};
    zVec3 trackedForward{1.0f, 0.0f, 0.0f};
    tracker.trackedWorldOriginPtr = &trackedOrigin;
    tracker.trackedForwardVecPtr = &trackedForward;
    tracker.mapScaleCurrent.x = 1.0f;
    tracker.mapScaleCurrent.z = 1.0f;
    tracker.mapZoom = 1.0f;
    tracker.mapOverlayCenterX = 100;
    tracker.mapOverlayCenterY = 100;
    tracker.outerRect = {0, 0, 200, 200};
    tracker.innerRectExpanded = {0, 0, 10, 10};

    HudSensorMapPoint points[2] = {{0.0f, 0.0f, 0.0f}, {10.0f, 0.0f, 0.0f}};
    HudSensorMapNode node{};
    node.pointCount = 2;
    node.points = points;
    node.selectedPointIndex = 0;
    node.isEnabled = 1;
    node.blinkTimerSec = 0.05f;
    node.packedColor565Pair = 0x12345678;

    zRndr::g_frameBuffer = reinterpret_cast<void *>(0x31415926);
    zRndr::g_pfnImmediateRaster4 =
        reinterpret_cast<zRndr::ImmediateRaster4Proc>(HudTestImmediateRaster4);
    zRndr::g_pfnImmediateRaster5 =
        reinterpret_cast<zRndr::ImmediateRaster5Proc>(HudTestImmediateRaster5);
    g_HudTestLine4Count = 0;
    g_HudTestLine4FrameBuffer = nullptr;
    std::memset(g_HudTestLine4Args, 0, sizeof(g_HudTestLine4Args));
    g_HudTestLine5Count = 0;
    g_HudTestLineFrameBuffer = nullptr;
    g_HudTestLineClipRect = nullptr;
    std::memset(g_HudTestLineArgs, 0, sizeof(g_HudTestLineArgs));

    const int result = node.DrawOnTracker(&tracker, nullptr);
    const bool blinkOk = result == 1 && node.packedColor565Pair == 0x56781234 &&
                         HudFloatNear(node.blinkTimerSec, 0.25f);
    const bool pathDrawOk =
        g_HudTestLine4Count == 2 &&
        g_HudTestLine4FrameBuffer == reinterpret_cast<void *>(0x31415926) &&
        g_HudTestLine4Args[0][0] == 100 && g_HudTestLine4Args[0][1] == 100 &&
        g_HudTestLine4Args[0][2] == 100 && g_HudTestLine4Args[0][3] == 90 &&
        g_HudTestLine4Args[0][4] == 0x1234 &&
        g_HudTestLine4Args[1][0] == 100 && g_HudTestLine4Args[1][1] == 90 &&
        g_HudTestLine4Args[1][2] == 100 && g_HudTestLine4Args[1][3] == 100 &&
        g_HudTestLine4Args[1][4] == 0x1234;
    const bool markerOk =
        g_HudTestLine5Count == 4 && g_HudTestLineClipRect == &tracker &&
        g_HudTestLineArgs[0][0] == 96 && g_HudTestLineArgs[0][1] == 100 &&
        g_HudTestLineArgs[0][2] == 100 && g_HudTestLineArgs[0][3] == 104 &&
        g_HudTestLineArgs[0][4] == 0x5678 &&
        g_HudTestLineArgs[3][0] == 100 && g_HudTestLineArgs[3][1] == 96 &&
        g_HudTestLineArgs[3][2] == 96 && g_HudTestLineArgs[3][3] == 100 &&
        g_HudTestLineArgs[3][4] == 0x5678;

    zRndr::g_frameBuffer = oldFrameBuffer;
    zRndr::g_pfnImmediateRaster4 = oldRaster4;
    zRndr::g_pfnImmediateRaster5 = oldRaster5;

    return blinkOk && pathDrawOk && markerOk ? 0 : 1;
}
