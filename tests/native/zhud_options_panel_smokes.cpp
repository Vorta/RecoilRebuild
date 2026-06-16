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

extern "C" int zhud_options_panel_object_detail_init_from_options_smoke(void) {
    int swObjectLod = 0;
    int hwObjectLod = 2;
    int *const oldSwObjectLod = ZOPT_OBJECT_LOD_SW;
    int *const oldHwObjectLod = ZOPT_OBJECT_LOD_HW;
    const int oldHwMode = g_zOpt_HwMode;

    ZOPT_OBJECT_LOD_SW = &swObjectLod;
    ZOPT_OBJECT_LOD_HW = &hwObjectLod;

    HudUiOptionsPanel_ObjectDetail objectDetail;
    objectDetail.Constructor();
    objectDetail.itemCount = 4;
    objectDetail.firstIndex = 1;
    objectDetail.visibleCount = 3;

    g_zOpt_HwMode = 0;
    objectDetail.selectedIndex = 9;
    objectDetail.InitFromOptions();
    const bool swClampLowOk = objectDetail.selectedIndex == 1;

    g_zOpt_HwMode = 1;
    objectDetail.selectedIndex = 9;
    objectDetail.InitFromOptions();
    const bool hwSelectionOk = objectDetail.selectedIndex == 2;

    hwObjectLod = 3;
    objectDetail.InitFromOptions();
    const bool hwVisibleClampOk = objectDetail.selectedIndex == 2;

    objectDetail.DestructorCore();
    ZOPT_OBJECT_LOD_SW = oldSwObjectLod;
    ZOPT_OBJECT_LOD_HW = oldHwObjectLod;
    g_zOpt_HwMode = oldHwMode;

    return swClampLowOk && hwSelectionOk && hwVisibleClampOk ? 0 : 1;
}

extern "C" int zhud_options_panel_object_detail_sync_from_options_smoke(void) {
    int swObjectLod = 0;
    int hwObjectLod = 0;
    int *const oldSwObjectLod = ZOPT_OBJECT_LOD_SW;
    int *const oldHwObjectLod = ZOPT_OBJECT_LOD_HW;
    zOpt_CameraSection **const oldCameraSection = g_zOpt_CameraSectionOption;
    const int oldHwMode = g_zOpt_HwMode;

    ZOPT_OBJECT_LOD_SW = &swObjectLod;
    ZOPT_OBJECT_LOD_HW = &hwObjectLod;
    g_zOpt_CameraSectionOption = 0;

    HudUiOptionsPanel_ObjectDetail objectDetail;
    objectDetail.Constructor();
    objectDetail.itemCount = 3;
    objectDetail.firstIndex = 0;
    objectDetail.visibleCount = 3;

    g_zOpt_HwMode = 0;
    objectDetail.selectedIndex = 0;
    objectDetail.SyncFromOptions();
    const bool swAdvanceOk = objectDetail.selectedIndex == 1 && swObjectLod == 1;

    objectDetail.selectedIndex = 2;
    objectDetail.SyncFromOptions();
    const bool swWrapOk = objectDetail.selectedIndex == 0 && swObjectLod == 0;

    g_zOpt_HwMode = 1;
    objectDetail.selectedIndex = 1;
    objectDetail.SyncFromOptions();
    const bool hwAdvanceOk = objectDetail.selectedIndex == 2 && hwObjectLod == 2;

    objectDetail.DestructorCore();
    ZOPT_OBJECT_LOD_SW = oldSwObjectLod;
    ZOPT_OBJECT_LOD_HW = oldHwObjectLod;
    g_zOpt_CameraSectionOption = oldCameraSection;
    g_zOpt_HwMode = oldHwMode;

    return swAdvanceOk && swWrapOk && hwAdvanceOk ? 0 : 1;
}

extern "C" int zhud_options_panel_texture_memory_init_from_options_smoke(void) {
    int swTextureMemory = 0;
    int hwTextureMemory = 2;
    int *const oldSwTextureMemory = ZOPT_TEXTURE_MEMORY_SW;
    int *const oldHwTextureMemory = ZOPT_TEXTURE_MEMORY_HW;
    const int oldHwMode = g_zOpt_HwMode;

    ZOPT_TEXTURE_MEMORY_SW = &swTextureMemory;
    ZOPT_TEXTURE_MEMORY_HW = &hwTextureMemory;

    HudUiOptionsPanel_TextureMemory textureMemory;
    textureMemory.Constructor();
    textureMemory.itemCount = 4;
    textureMemory.firstIndex = 1;
    textureMemory.visibleCount = 3;

    g_zOpt_HwMode = 0;
    textureMemory.selectedIndex = 9;
    textureMemory.InitFromOptions();
    const bool swClampLowOk = textureMemory.selectedIndex == 1;

    g_zOpt_HwMode = 1;
    textureMemory.selectedIndex = 9;
    textureMemory.InitFromOptions();
    const bool hwSelectionOk = textureMemory.selectedIndex == 2;

    hwTextureMemory = 3;
    textureMemory.InitFromOptions();
    const bool hwVisibleClampOk = textureMemory.selectedIndex == 2;

    textureMemory.DestructorCore();
    ZOPT_TEXTURE_MEMORY_SW = oldSwTextureMemory;
    ZOPT_TEXTURE_MEMORY_HW = oldHwTextureMemory;
    g_zOpt_HwMode = oldHwMode;

    return swClampLowOk && hwSelectionOk && hwVisibleClampOk ? 0 : 1;
}

extern "C" int zhud_options_panel_texture_memory_sync_from_options_smoke(void) {
    int swTextureMemory = 0;
    int hwTextureMemory = 0;
    int *const oldSwTextureMemory = ZOPT_TEXTURE_MEMORY_SW;
    int *const oldHwTextureMemory = ZOPT_TEXTURE_MEMORY_HW;
    const int oldHwMode = g_zOpt_HwMode;

    ZOPT_TEXTURE_MEMORY_SW = &swTextureMemory;
    ZOPT_TEXTURE_MEMORY_HW = &hwTextureMemory;

    HudUiOptionsPanel_TextureMemory textureMemory;
    textureMemory.Constructor();
    textureMemory.itemCount = 3;
    textureMemory.firstIndex = 0;
    textureMemory.visibleCount = 3;

    g_zOpt_HwMode = 0;
    textureMemory.selectedIndex = 0;
    textureMemory.SyncFromOptions();
    const bool swAdvanceOk =
        textureMemory.selectedIndex == 1 && swTextureMemory == 1;

    textureMemory.selectedIndex = 2;
    textureMemory.SyncFromOptions();
    const bool swWrapOk =
        textureMemory.selectedIndex == 0 && swTextureMemory == 0;

    g_zOpt_HwMode = 1;
    textureMemory.selectedIndex = 1;
    textureMemory.SyncFromOptions();
    const bool hwAdvanceOk =
        textureMemory.selectedIndex == 2 && hwTextureMemory == 2;

    textureMemory.DestructorCore();
    ZOPT_TEXTURE_MEMORY_SW = oldSwTextureMemory;
    ZOPT_TEXTURE_MEMORY_HW = oldHwTextureMemory;
    g_zOpt_HwMode = oldHwMode;

    return swAdvanceOk && swWrapOk && hwAdvanceOk ? 0 : 1;
}

extern "C" int zhud_options_panel_effects_init_from_options_smoke(void) {
    int swEffectsLevel = 0;
    int hwEffectsLevel = 0;
    int videoAcceleration = 0;
    int *const oldSwEffectsLevel = ZOPT_EFFECTS_LEVEL_SW;
    int *const oldHwEffectsLevel = ZOPT_EFFECTS_LEVEL_HW;
    int *const oldVideoAcceleration = ZOPT_VIDEO_ACCELERATION;
    const int oldHwMode = g_zOpt_HwMode;

    ZOPT_EFFECTS_LEVEL_SW = &swEffectsLevel;
    ZOPT_EFFECTS_LEVEL_HW = &hwEffectsLevel;
    ZOPT_VIDEO_ACCELERATION = &videoAcceleration;

    HudUiOptionsPanel_Effects effects;
    effects.Constructor();
    effects.itemCount = 4;
    effects.firstIndex = 0;
    effects.visibleCount = 4;

    g_zOpt_HwMode = 0;
    videoAcceleration = 0;
    swEffectsLevel = 0;
    effects.selectedIndex = 0;
    effects.InitFromOptions();
    const bool swZeroForcedOk =
        effects.firstIndex == 1 &&
        effects.visibleCount == 3 &&
        effects.selectedIndex == 1;

    swEffectsLevel = 2;
    effects.firstIndex = 0;
    effects.visibleCount = 4;
    effects.selectedIndex = 0;
    effects.InitFromOptions();
    const bool swRangeOk =
        effects.firstIndex == 1 &&
        effects.visibleCount == 3 &&
        effects.selectedIndex == 2;

    g_zOpt_HwMode = 1;
    videoAcceleration = 1;
    hwEffectsLevel = 0;
    effects.firstIndex = 0;
    effects.visibleCount = 4;
    effects.selectedIndex = 9;
    effects.InitFromOptions();
    const bool hwDirectOk =
        effects.firstIndex == 0 &&
        effects.visibleCount == 4 &&
        effects.selectedIndex == 0;

    effects.DestructorCore();
    ZOPT_EFFECTS_LEVEL_SW = oldSwEffectsLevel;
    ZOPT_EFFECTS_LEVEL_HW = oldHwEffectsLevel;
    ZOPT_VIDEO_ACCELERATION = oldVideoAcceleration;
    g_zOpt_HwMode = oldHwMode;

    return swZeroForcedOk && swRangeOk && hwDirectOk ? 0 : 1;
}

extern "C" int zhud_options_panel_sound_active_init_from_options_smoke(void) {
    int muteSound = 0;
    int *const oldMuteSound = ZOPT_MUTE_SOUND;

    ZOPT_MUTE_SOUND = &muteSound;

    HudUiOptionsPanel_SoundActive soundActive;
    soundActive.Constructor();

    muteSound = 0;
    soundActive.checked = 0;
    soundActive.InitFromOptions();
    const bool unmutedOk = soundActive.checked == 1;

    muteSound = 1;
    soundActive.checked = 9;
    soundActive.InitFromOptions();
    const bool mutedOk = soundActive.checked == 0;

    soundActive.DestructorCore();
    ZOPT_MUTE_SOUND = oldMuteSound;

    return unmutedOk && mutedOk ? 0 : 1;
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
