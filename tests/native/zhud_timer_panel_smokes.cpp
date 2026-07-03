#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zUtil/zbd.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

extern "C" std::uint32_t g_HudUi_InvalidateMask;

namespace {
void ReleasePanelFont(HudUiPanel *panel) {
    if (panel->hFont != nullptr) {
        DeleteObject(panel->hFont);
        panel->hFont = nullptr;
    }
}
}

extern "C" int zhud_timer_panel_set_time_smoke(void) {
    HudUiTimerPanel timer{};
    HudUiPanel *const panel = &timer;
    panel->ConstructorDefault("", 0, 0);

    timer.SetTimeSeconds(1, 2, 3);
    const bool positive = std::strcmp(timer.cachedText, "01:02:03") == 0;

    timer.SetTimeSeconds(-1, 2, 3);
    const bool fallback = std::strcmp(timer.cachedText, "00:00:00") == 0;

    ReleasePanelFont(panel);
    return positive && fallback ? 0 : 1;
}

extern "C" int zhud_timer_panel_update_hms_smoke(void) {
    HudUiTimerPanel timer{};
    HudUiPanel *const panel = &timer;
    panel->ConstructorDefault("", 0, 0);

    timer.UpdateHMSFromSeconds(3661.9f);
    const bool positive =
        timer.elapsedSeconds == 3661.9f && std::strcmp(timer.cachedText, "01:01:01") == 0;

    timer.UpdateHMSFromSeconds(-1.0f);
    const bool fallback =
        timer.elapsedSeconds == -1.0f && std::strcmp(timer.cachedText, "00:00:00") == 0;

    ReleasePanelFont(panel);
    return positive && fallback ? 0 : 1;
}

extern "C" int zhud_timer_panel_update_smoke(void) {
    int networkEnabled = 0;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    const float oldFrameDelta = g_FrameDeltaTimeSec;
    const float oldUnscaledDelta = g_Time_UnscaledDeltaTimeSec;
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_FrameDeltaTimeSec = 2.0f;
    g_Time_UnscaledDeltaTimeSec = 4.0f;
    g_HudUi_InvalidateMask = 0;

    HudUiTimerPanel timer{};
    HudUiPanel *const panel = &timer;
    panel->ConstructorDefault("", 0, 0);
    timer.flags = 0x02;

    timer.elapsedSeconds = 10.0f;
    timer.stopped = 0;
    timer.secondsStep = 3;
    timer.Update(0.25f);
    const bool normal =
        timer.elapsedSeconds == 16.0f && std::strcmp(timer.cachedText, "00:00:16") == 0;

    networkEnabled = 1;
    timer.Update(0.25f);
    const bool network =
        timer.elapsedSeconds == 28.0f && std::strcmp(timer.cachedText, "00:00:28") == 0;

    timer.stopped = 1;
    timer.Update(0.25f);
    const bool stopped = timer.elapsedSeconds == 28.0f;

    ReleasePanelFont(panel);
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_FrameDeltaTimeSec = oldFrameDelta;
    g_Time_UnscaledDeltaTimeSec = oldUnscaledDelta;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    return normal && network && stopped ? 0 : 1;
}

extern "C" int zhud_timer_panel_zar_read_smoke(void) {
    HudUiTimerPanel timer{};
    HudUiPanel *const panel = &timer;
    panel->ConstructorDefault("", 0, 0);

    const int oldChatComposeActive = g_HudUiMgrObjectiveChatComposeActive;
    const int oldObjectivePhase = g_HudUiMgrObjectivePhase;
    const float oldPhaseDuration = g_HudUiMgrObjectivePhaseDurationSec;
    const float oldPhaseTimer = g_HudUiMgrObjectivePhaseTimerSec;
    const float oldAutoHideDelay = g_HudUiMgrObjectiveAutoHideDelaySec;

    g_HudUiMgrObjectiveChatComposeActive = 0;
    g_HudUiMgrObjectivePhase = 1;
    g_HudUiMgrObjectivePhaseDurationSec = 10.0f;
    g_HudUiMgrObjectivePhaseTimerSec = 2.0f;
    g_HudUiMgrObjectiveAutoHideDelaySec = 5.0f;

    const float payload = 7322.4f;
    HudUiTimerPanel::ZarReadTimerData(&payload, sizeof(payload), &timer);

    const bool timerUpdated =
        timer.elapsedSeconds == payload && std::strcmp(timer.cachedText, "02:02:02") == 0;
    const bool objectiveBegan = g_HudUiMgrObjectivePhase == 3 &&
                                g_HudUiMgrObjectivePhaseTimerSec == 8.0f &&
                                g_HudUiMgrObjectiveAutoHideDelaySec == 0.0f;

    g_HudUiMgrObjectiveChatComposeActive = oldChatComposeActive;
    g_HudUiMgrObjectivePhase = oldObjectivePhase;
    g_HudUiMgrObjectivePhaseDurationSec = oldPhaseDuration;
    g_HudUiMgrObjectivePhaseTimerSec = oldPhaseTimer;
    g_HudUiMgrObjectiveAutoHideDelaySec = oldAutoHideDelay;
    ReleasePanelFont(panel);
    return timerUpdated && objectiveBegan ? 0 : 1;
}

extern "C" int zhud_timer_panel_zar_write_smoke(void) {
    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    GetTempPathA(sizeof(tempPath), tempPath);
    GetTempFileNameA(tempPath, "hud", 0, tempFile);

    HANDLE const file =
        CreateFileA(tempFile, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 1;
    }

    HudUiTimerPanel timer{};
    timer.elapsedSeconds = 123.5f;

    zZbdManager manager = {};
    manager.indexArchive.hFile = file;
    zZbdSectionHandler handler = {};
    handler.sectionName = "HUD";
    zZbdSectionCallbackCtx sectionCtx = {};
    sectionCtx.manager = &manager;
    sectionCtx.sectionHandler = &handler;

    HudUiTimerPanel::ZarWriteTimerDataCallback(&sectionCtx, &timer);

    SetFilePointer(file, 0, nullptr, FILE_BEGIN);
    float readBack = 0.0f;
    DWORD read = 0;
    ReadFile(file, &readBack, sizeof(readBack), &read, nullptr);

    const bool ok = manager.indexArchive.recordCount == 1 &&
                    manager.indexArchive.records != nullptr &&
                    manager.indexArchive.records[0].fileSize == sizeof(float) &&
                    std::strcmp(manager.indexArchive.records[0].name, "HUD/TimerData") == 0 &&
                    read == sizeof(readBack) && readBack == 123.5f;

    std::free(manager.indexArchive.records);
    manager.indexArchive.records = nullptr;
    CloseHandle(file);
    return ok ? 0 : 1;
}

extern "C" int zhud_timer_and_counter_constructor_smoke(void) {
    HudUiElement *const oldHead = g_HudUiMgr.childHead;
    HudUiElement *const oldTail = g_HudUiMgr.childTail;
    g_HudUiMgr.childHead = nullptr;
    g_HudUiMgr.childTail = nullptr;

    HudUiTimerPanel timer{};
    HudUiCounterTextPanel counter{};
    HudUiPanel *const timerPanel = &timer;
    HudUiPanel *const counterPanel = &counter;

    timer.ConstructorDefault();
    counter.Constructor();

    const bool timerOk = std::strcmp(timer.cachedText, "00:00:00") == 0 &&
                         timer.elapsedSeconds == 0.0f && timer.stopped == 1 &&
                         timer.secondsStep == 1;

    const bool counterOk = std::strcmp(counter.cachedText, "0") == 0;

    const bool linked = g_HudUiMgr.childHead == &timer &&
                        g_HudUiMgr.childTail == &counter &&
                        timer.next == &counter;

    ReleasePanelFont(timerPanel);
    ReleasePanelFont(counterPanel);
    g_HudUiMgr.childHead = oldHead;
    g_HudUiMgr.childTail = oldTail;
    return timerOk && counterOk && linked ? 0 : 1;
}
