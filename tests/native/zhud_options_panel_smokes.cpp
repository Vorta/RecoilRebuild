#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"
#include "GameZRecoil/zGame/zGame.h"

extern "C" int zhud_options_panel_lighting_init_from_options_smoke(void) {
    int swFlags = 0x10;
    int hwFlags = 0;
    int *const oldSwFlags = ZOPT_GFX_FLAGS_SW;
    int *const oldHwFlags = ZOPT_GFX_FLAGS_HW;
    const int oldHwMode = g_zOpt_HwMode;

    ZOPT_GFX_FLAGS_SW = &swFlags;
    ZOPT_GFX_FLAGS_HW = &hwFlags;

    HudUiOptionsPanel_Lighting lighting;
    lighting.Constructor();

    g_zOpt_HwMode = 0;
    lighting.checked = 0;
    lighting.InitFromOptions();
    const bool swOk = lighting.checked == 0x10;

    g_zOpt_HwMode = 1;
    lighting.checked = 7;
    lighting.InitFromOptions();
    const bool hwClearOk = lighting.checked == 0;

    hwFlags = 0x31;
    lighting.InitFromOptions();
    const bool hwSetOk = lighting.checked == 0x10;

    lighting.DestructorCore();
    ZOPT_GFX_FLAGS_SW = oldSwFlags;
    ZOPT_GFX_FLAGS_HW = oldHwFlags;
    g_zOpt_HwMode = oldHwMode;

    return swOk && hwClearOk && hwSetOk ? 0 : 1;
}

extern "C" int zhud_options_panel_lighting_sync_from_options_smoke(void) {
    int swFlags = 0;
    int hwFlags = 0x20;
    int *const oldSwFlags = ZOPT_GFX_FLAGS_SW;
    int *const oldHwFlags = ZOPT_GFX_FLAGS_HW;
    const int oldHwMode = g_zOpt_HwMode;

    ZOPT_GFX_FLAGS_SW = &swFlags;
    ZOPT_GFX_FLAGS_HW = &hwFlags;

    HudUiOptionsPanel_Lighting lighting;
    lighting.Constructor();
    lighting.modeOrEnabled = 1;

    g_zOpt_HwMode = 1;
    lighting.checked = 0;
    lighting.SyncFromOptions();
    const bool setOk = lighting.checked == 1 && hwFlags == 0x30 && swFlags == 0;

    lighting.SyncFromOptions();
    const bool clearOk = lighting.checked == 0 && hwFlags == 0x20 && swFlags == 0;

    g_zOpt_HwMode = 0;
    lighting.checked = 0;
    swFlags = 4;
    lighting.SyncFromOptions();
    const bool swOk = lighting.checked == 1 && swFlags == 0x14 && hwFlags == 0x20;

    lighting.DestructorCore();
    ZOPT_GFX_FLAGS_SW = oldSwFlags;
    ZOPT_GFX_FLAGS_HW = oldHwFlags;
    g_zOpt_HwMode = oldHwMode;

    return setOk && clearOk && swOk ? 0 : 1;
}

extern "C" int zhud_options_panel_resolution_on_activate_smoke(void) {
    const zVidModeIndex oldDeferredMode =
        g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex;

    HudUiOptionsPanel_Resolution resolution;
    resolution.Constructor();
    resolution.itemCount = 7;
    resolution.firstIndex = 0;
    resolution.visibleCount = 6;

    resolution.selectedIndex = 0;
    RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(
        ZVID_MODE_INVALID_COMPLEMENT
    );
    resolution.OnActivate();
    const bool case1Ok =
        resolution.selectedIndex == 1 &&
        g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex ==
            ZVID_MODE_320X240_TO_640X480;

    resolution.visibleCount = 1;
    resolution.selectedIndex = 0;
    RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(
        ZVID_MODE_INVALID_COMPLEMENT
    );
    resolution.OnActivate();
    const bool case0WrapOk =
        resolution.selectedIndex == 0 &&
        g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex ==
            ZVID_MODE_640X480;

    resolution.visibleCount = 6;
    resolution.selectedIndex = 1;
    RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(
        ZVID_MODE_INVALID_COMPLEMENT
    );
    resolution.OnActivate();
    const bool case2Ok =
        resolution.selectedIndex == 2 &&
        g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex ==
            ZVID_MODE_640X400;

    resolution.selectedIndex = 2;
    RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(
        ZVID_MODE_INVALID_COMPLEMENT
    );
    resolution.OnActivate();
    const bool case3Ok =
        resolution.selectedIndex == 3 &&
        g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex ==
            ZVID_MODE_320X200_TO_640X400;

    resolution.selectedIndex = 3;
    RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(
        ZVID_MODE_INVALID_COMPLEMENT
    );
    resolution.OnActivate();
    const bool case4Ok =
        resolution.selectedIndex == 4 &&
        g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex ==
            ZVID_MODE_800X600;

    resolution.selectedIndex = 4;
    RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(
        ZVID_MODE_INVALID_COMPLEMENT
    );
    resolution.OnActivate();
    const bool case5Ok =
        resolution.selectedIndex == 5 &&
        g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex ==
            ZVID_MODE_1024X768;

    resolution.visibleCount = 7;
    resolution.selectedIndex = 5;
    RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(ZVID_MODE_640X480);
    resolution.OnActivate();
    const bool defaultOk =
        resolution.selectedIndex == 6 &&
        g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex ==
            ZVID_MODE_640X480;

    resolution.DestructorCore();
    RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(oldDeferredMode);

    return case0WrapOk && case1Ok && case2Ok && case3Ok && case4Ok && case5Ok &&
                   defaultOk
               ? 0
               : 1;
}
