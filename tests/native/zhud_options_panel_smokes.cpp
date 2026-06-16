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

extern "C" int zhud_options_panel_perspective_init_from_options_smoke(void) {
    int swFlags = 8;
    int hwFlags = 0;
    int *const oldSwFlags = ZOPT_GFX_FLAGS_SW;
    int *const oldHwFlags = ZOPT_GFX_FLAGS_HW;
    const int oldHwMode = g_zOpt_HwMode;

    ZOPT_GFX_FLAGS_SW = &swFlags;
    ZOPT_GFX_FLAGS_HW = &hwFlags;

    HudUiOptionsPanel_Perspective perspective;
    perspective.Constructor();

    g_zOpt_HwMode = 0;
    perspective.checked = 0;
    perspective.InitFromOptions();
    const bool swOk = perspective.checked == 8;

    g_zOpt_HwMode = 1;
    perspective.checked = 7;
    perspective.InitFromOptions();
    const bool hwClearOk = perspective.checked == 0;

    hwFlags = 0x2a;
    perspective.InitFromOptions();
    const bool hwSetOk = perspective.checked == 8;

    perspective.DestructorCore();
    ZOPT_GFX_FLAGS_SW = oldSwFlags;
    ZOPT_GFX_FLAGS_HW = oldHwFlags;
    g_zOpt_HwMode = oldHwMode;

    return swOk && hwClearOk && hwSetOk ? 0 : 1;
}

extern "C" int zhud_options_panel_full_hud_init_from_options_smoke(void) {
    int swHudType = ZOPT_HUD_TYPE_PERSPECTIVE;
    int hwHudType = ZOPT_HUD_TYPE_STANDARD;
    int *const oldSwHudType = ZOPT_HUD_TYPE_SW;
    int *const oldHwHudType = ZOPT_HUD_TYPE_HW;
    const int oldHwMode = g_zOpt_HwMode;

    ZOPT_HUD_TYPE_SW = &swHudType;
    ZOPT_HUD_TYPE_HW = &hwHudType;

    HudUiOptionsPanel_FullHud fullHud;
    fullHud.Constructor();

    g_zOpt_HwMode = 0;
    fullHud.checked = 0;
    fullHud.InitFromOptions();
    const bool swPerspectiveOk = fullHud.checked == 1;

    g_zOpt_HwMode = 1;
    fullHud.checked = 9;
    fullHud.InitFromOptions();
    const bool hwStandardOk = fullHud.checked == 0;

    hwHudType = ZOPT_HUD_TYPE_PERSPECTIVE;
    fullHud.InitFromOptions();
    const bool hwPerspectiveOk = fullHud.checked == 1;

    fullHud.DestructorCore();
    ZOPT_HUD_TYPE_SW = oldSwHudType;
    ZOPT_HUD_TYPE_HW = oldHwHudType;
    g_zOpt_HwMode = oldHwMode;

    return swPerspectiveOk && hwStandardOk && hwPerspectiveOk ? 0 : 1;
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
