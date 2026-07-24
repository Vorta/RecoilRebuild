#include "GameZRecoil/zHud/zhud_ui.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include <string.h>

extern "C" unsigned int g_HudUi_InvalidateMask;

static int g_musicVolumeGetVolumeCount;
static unsigned short g_musicVolumePrimary;
static unsigned short g_musicVolumeSecondary;
static int g_musicVolumeSetVolumeCount;
static unsigned short g_musicVolumeSetPrimary;
static unsigned short g_musicVolumeSetSecondary;
static int g_musicEnablePlayTrackCount;
static int g_musicEnablePlayTrack;
static int g_musicEnablePlayMode;
static int g_musicEnableStopCount;
static int g_perspectiveSelectSpanCount;

struct OptionsPanelFunctionPatch {
    void *target;
    unsigned char original[5];
    int active;
};

static bool OptionsPanelFloatNear(
    float actual,
    float expected
) {
    const float diff = actual - expected;
    return diff > -0.0001f && diff < 0.0001f;
}

static bool PatchOptionsPanelFunctionJump(
    void *target,
    void *replacement,
    OptionsPanelFunctionPatch &patch
) {
    DWORD oldProtect = 0;
    if (!VirtualProtect(target, sizeof(patch.original), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    patch.target = target;
    memcpy(patch.original, target, sizeof(patch.original));

    unsigned char *const bytes = (unsigned char *)(target);
    bytes[0] = 0xe9;
    *(int *)(bytes + 1) =
        (int)((unsigned char *)(replacement) - ((unsigned char *)(target) + 5));

    DWORD ignored = 0;
    VirtualProtect(target, sizeof(patch.original), oldProtect, &ignored);
    patch.active = 1;
    return true;
}

static void RestoreOptionsPanelFunctionPatch(
    OptionsPanelFunctionPatch &patch
) {
    if (patch.active == 0) {
        return;
    }

    DWORD oldProtect = 0;
    DWORD ignored = 0;
    if (VirtualProtect(patch.target, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect)) {
        memcpy(patch.target, patch.original, sizeof(patch.original));
        VirtualProtect(patch.target, sizeof(patch.original), oldProtect, &ignored);
    }

    patch.active = 0;
}

static int __fastcall FakeMusicVolumeGetVolume(
    unsigned short *primaryVolumeOut,
    unsigned short *secondaryVolumeOut
) {
    ++g_musicVolumeGetVolumeCount;
    *primaryVolumeOut = g_musicVolumePrimary;
    *secondaryVolumeOut = g_musicVolumeSecondary;
    return 1;
}

static int __fastcall FakeMusicVolumeSetVolume(
    unsigned short primaryVolume,
    unsigned short secondaryVolume
) {
    ++g_musicVolumeSetVolumeCount;
    g_musicVolumeSetPrimary = primaryVolume;
    g_musicVolumeSetSecondary = secondaryVolume;
    return 1;
}

static int __fastcall FakeMusicEnablePlayTrackWithMode(
    int trackIndex,
    int playbackMode
) {
    ++g_musicEnablePlayTrackCount;
    g_musicEnablePlayTrack = trackIndex;
    g_musicEnablePlayMode = playbackMode;
    return 1;
}

static int FakeMusicEnableStop(void) {
    ++g_musicEnableStopCount;
    return 1;
}

static void FakePerspectiveSelectSpanRoutines(void) {
    ++g_perspectiveSelectSpanCount;
}

extern "C" int zhud_options_panel_lighting_init_from_options_smoke(void) {
    int swFlags = 0x10;
    int hwFlags = 0;
    int *const oldSwFlags = g_zGame_Options_PointerCache.gfxFlagsSw;
    int *const oldHwFlags = g_zGame_Options_PointerCache.gfxFlagsHw;
    const int oldHwMode = g_zOpt_HwMode;

    g_zGame_Options_PointerCache.gfxFlagsSw = &swFlags;
    g_zGame_Options_PointerCache.gfxFlagsHw = &hwFlags;

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
    g_zGame_Options_PointerCache.gfxFlagsSw = oldSwFlags;
    g_zGame_Options_PointerCache.gfxFlagsHw = oldHwFlags;
    g_zOpt_HwMode = oldHwMode;

    return swOk && hwClearOk && hwSetOk ? 0 : 1;
}

extern "C" int zhud_options_panel_lighting_sync_from_options_smoke(void) {
    int swFlags = 0;
    int hwFlags = 0x20;
    int *const oldSwFlags = g_zGame_Options_PointerCache.gfxFlagsSw;
    int *const oldHwFlags = g_zGame_Options_PointerCache.gfxFlagsHw;
    const int oldHwMode = g_zOpt_HwMode;

    g_zGame_Options_PointerCache.gfxFlagsSw = &swFlags;
    g_zGame_Options_PointerCache.gfxFlagsHw = &hwFlags;

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
    g_zGame_Options_PointerCache.gfxFlagsSw = oldSwFlags;
    g_zGame_Options_PointerCache.gfxFlagsHw = oldHwFlags;
    g_zOpt_HwMode = oldHwMode;

    return setOk && clearOk && swOk ? 0 : 1;
}

extern "C" int zhud_options_panel_perspective_init_from_options_smoke(void) {
    int swFlags = 8;
    int hwFlags = 0;
    int *const oldSwFlags = g_zGame_Options_PointerCache.gfxFlagsSw;
    int *const oldHwFlags = g_zGame_Options_PointerCache.gfxFlagsHw;
    const int oldHwMode = g_zOpt_HwMode;

    g_zGame_Options_PointerCache.gfxFlagsSw = &swFlags;
    g_zGame_Options_PointerCache.gfxFlagsHw = &hwFlags;

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
    g_zGame_Options_PointerCache.gfxFlagsSw = oldSwFlags;
    g_zGame_Options_PointerCache.gfxFlagsHw = oldHwFlags;
    g_zOpt_HwMode = oldHwMode;

    return swOk && hwClearOk && hwSetOk ? 0 : 1;
}

extern "C" int zhud_options_panel_perspective_sync_from_options_smoke(void) {
    int swFlags = 0;
    int hwFlags = 0x20;
    int *const oldSwFlags = g_zGame_Options_PointerCache.gfxFlagsSw;
    int *const oldHwFlags = g_zGame_Options_PointerCache.gfxFlagsHw;
    const int oldHwMode = g_zOpt_HwMode;
    OptionsPanelFunctionPatch selectSpanPatch = {0};

    if (!PatchOptionsPanelFunctionJump(
            (void *)(&zRndr::SelectSpanRoutines),
            (void *)(&FakePerspectiveSelectSpanRoutines),
            selectSpanPatch
        )) {
        return 1;
    }

    g_zGame_Options_PointerCache.gfxFlagsSw = &swFlags;
    g_zGame_Options_PointerCache.gfxFlagsHw = &hwFlags;

    HudUiOptionsPanel_Perspective perspective;
    perspective.Constructor();
    perspective.modeOrEnabled = 1;
    g_perspectiveSelectSpanCount = 0;

    g_zOpt_HwMode = 1;
    perspective.checked = 0;
    perspective.SyncFromOptions();
    const bool setOk =
        perspective.checked == 1 &&
        hwFlags == 0x28 &&
        swFlags == 0 &&
        g_perspectiveSelectSpanCount == 1;

    perspective.SyncFromOptions();
    const bool clearOk =
        perspective.checked == 0 &&
        hwFlags == 0x20 &&
        swFlags == 0 &&
        g_perspectiveSelectSpanCount == 2;

    g_zOpt_HwMode = 0;
    perspective.checked = 0;
    swFlags = 0x10;
    perspective.SyncFromOptions();
    const bool swOk =
        perspective.checked == 1 &&
        swFlags == 0x18 &&
        hwFlags == 0x20 &&
        g_perspectiveSelectSpanCount == 3;

    perspective.DestructorCore();
    g_zGame_Options_PointerCache.gfxFlagsSw = oldSwFlags;
    g_zGame_Options_PointerCache.gfxFlagsHw = oldHwFlags;
    g_zOpt_HwMode = oldHwMode;
    RestoreOptionsPanelFunctionPatch(selectSpanPatch);

    return setOk && clearOk && swOk ? 0 : 1;
}

extern "C" int zhud_options_panel_full_hud_init_from_options_smoke(void) {
    int swHudType = ZOPT_HUD_TYPE_PERSPECTIVE;
    int hwHudType = ZOPT_HUD_TYPE_STANDARD;
    int *const oldSwHudType = g_zGame_Options_PointerCache.hudTypeSw;
    int *const oldHwHudType = g_zGame_Options_PointerCache.hudTypeHw;
    const int oldHwMode = g_zOpt_HwMode;

    g_zGame_Options_PointerCache.hudTypeSw = &swHudType;
    g_zGame_Options_PointerCache.hudTypeHw = &hwHudType;

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
    g_zGame_Options_PointerCache.hudTypeSw = oldSwHudType;
    g_zGame_Options_PointerCache.hudTypeHw = oldHwHudType;
    g_zOpt_HwMode = oldHwMode;

    return swPerspectiveOk && hwStandardOk && hwPerspectiveOk ? 0 : 1;
}

extern "C" int zhud_options_panel_object_detail_init_from_options_smoke(void) {
    int swObjectLod = 0;
    int hwObjectLod = 2;
    int *const oldSwObjectLod = g_zGame_Options_PointerCache.objectLodSw;
    int *const oldHwObjectLod = g_zGame_Options_PointerCache.objectLodHw;
    const int oldHwMode = g_zOpt_HwMode;

    g_zGame_Options_PointerCache.objectLodSw = &swObjectLod;
    g_zGame_Options_PointerCache.objectLodHw = &hwObjectLod;

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
    g_zGame_Options_PointerCache.objectLodSw = oldSwObjectLod;
    g_zGame_Options_PointerCache.objectLodHw = oldHwObjectLod;
    g_zOpt_HwMode = oldHwMode;

    return swClampLowOk && hwSelectionOk && hwVisibleClampOk ? 0 : 1;
}

extern "C" int zhud_options_panel_object_detail_sync_from_options_smoke(void) {
    int swObjectLod = 0;
    int hwObjectLod = 0;
    int *const oldSwObjectLod = g_zGame_Options_PointerCache.objectLodSw;
    int *const oldHwObjectLod = g_zGame_Options_PointerCache.objectLodHw;
    zOpt_CameraSection **const oldCameraSection = g_zGame_Options_PointerCache.cameraSection;
    const int oldHwMode = g_zOpt_HwMode;

    g_zGame_Options_PointerCache.objectLodSw = &swObjectLod;
    g_zGame_Options_PointerCache.objectLodHw = &hwObjectLod;
    g_zGame_Options_PointerCache.cameraSection = 0;

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
    g_zGame_Options_PointerCache.objectLodSw = oldSwObjectLod;
    g_zGame_Options_PointerCache.objectLodHw = oldHwObjectLod;
    g_zGame_Options_PointerCache.cameraSection = oldCameraSection;
    g_zOpt_HwMode = oldHwMode;

    return swAdvanceOk && swWrapOk && hwAdvanceOk ? 0 : 1;
}

extern "C" int zopt_toggle_hud_type_for_current_hw_mode_smoke(void) {
    int *const oldHudTypeSw = g_zGame_Options_PointerCache.hudTypeSw;
    int *const oldHudTypeHw = g_zGame_Options_PointerCache.hudTypeHw;
    const int oldHwMode = g_zOpt_HwMode;
    const int oldLayoutsInitialized = g_HudUiMgrHudLayoutsInitialized;

    int hudTypeSw = ZOPT_HUD_TYPE_STANDARD;
    int hudTypeHw = 7;
    g_zGame_Options_PointerCache.hudTypeSw = &hudTypeSw;
    g_zGame_Options_PointerCache.hudTypeHw = &hudTypeHw;
    g_HudUiMgrHudLayoutsInitialized = 0;

    g_zOpt_HwMode = 0;
    int returned = zOpt::ToggleHudTypeForCurrentHwMode();
    bool ok = returned == ZOPT_HUD_TYPE_STANDARD &&
              hudTypeSw == ZOPT_HUD_TYPE_PERSPECTIVE && hudTypeHw == 7;

    returned = zOpt::ToggleHudTypeForCurrentHwMode();
    ok = ok && returned == ZOPT_HUD_TYPE_PERSPECTIVE &&
         hudTypeSw == ZOPT_HUD_TYPE_STANDARD && hudTypeHw == 7;

    hudTypeSw = 9;
    returned = zOpt::ToggleHudTypeForCurrentHwMode();
    ok = ok && returned == 9 && hudTypeSw == 9 && hudTypeHw == 7;

    g_zOpt_HwMode = 1;
    hudTypeHw = ZOPT_HUD_TYPE_PERSPECTIVE;
    returned = zOpt::ToggleHudTypeForCurrentHwMode();
    ok = ok && returned == ZOPT_HUD_TYPE_PERSPECTIVE &&
         hudTypeHw == ZOPT_HUD_TYPE_STANDARD && hudTypeSw == 9;

    g_zGame_Options_PointerCache.hudTypeSw = oldHudTypeSw;
    g_zGame_Options_PointerCache.hudTypeHw = oldHudTypeHw;
    g_zOpt_HwMode = oldHwMode;
    g_HudUiMgrHudLayoutsInitialized = oldLayoutsInitialized;
    return ok ? 0 : 1;
}

extern "C" int zhud_options_panel_texture_memory_init_from_options_smoke(void) {
    int swTextureMemory = 0;
    int hwTextureMemory = 2;
    int *const oldSwTextureMemory = g_zGame_Options_PointerCache.textureMemorySw;
    int *const oldHwTextureMemory = g_zGame_Options_PointerCache.textureMemoryHw;
    const int oldHwMode = g_zOpt_HwMode;

    g_zGame_Options_PointerCache.textureMemorySw = &swTextureMemory;
    g_zGame_Options_PointerCache.textureMemoryHw = &hwTextureMemory;

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
    g_zGame_Options_PointerCache.textureMemorySw = oldSwTextureMemory;
    g_zGame_Options_PointerCache.textureMemoryHw = oldHwTextureMemory;
    g_zOpt_HwMode = oldHwMode;

    return swClampLowOk && hwSelectionOk && hwVisibleClampOk ? 0 : 1;
}

extern "C" int zhud_options_panel_texture_memory_sync_from_options_smoke(void) {
    int swTextureMemory = 0;
    int hwTextureMemory = 0;
    int *const oldSwTextureMemory = g_zGame_Options_PointerCache.textureMemorySw;
    int *const oldHwTextureMemory = g_zGame_Options_PointerCache.textureMemoryHw;
    const int oldHwMode = g_zOpt_HwMode;

    g_zGame_Options_PointerCache.textureMemorySw = &swTextureMemory;
    g_zGame_Options_PointerCache.textureMemoryHw = &hwTextureMemory;

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
    g_zGame_Options_PointerCache.textureMemorySw = oldSwTextureMemory;
    g_zGame_Options_PointerCache.textureMemoryHw = oldHwTextureMemory;
    g_zOpt_HwMode = oldHwMode;

    return swAdvanceOk && swWrapOk && hwAdvanceOk ? 0 : 1;
}

extern "C" int zhud_options_panel_effects_init_from_options_smoke(void) {
    int swEffectsLevel = 0;
    int hwEffectsLevel = 0;
    int videoAcceleration = 0;
    int *const oldSwEffectsLevel = g_zGame_Options_PointerCache.effectsLevelSw;
    int *const oldHwEffectsLevel = g_zGame_Options_PointerCache.effectsLevelHw;
    int *const oldVideoAcceleration = g_zGame_Options_PointerCache.videoAcceleration;
    const int oldHwMode = g_zOpt_HwMode;

    g_zGame_Options_PointerCache.effectsLevelSw = &swEffectsLevel;
    g_zGame_Options_PointerCache.effectsLevelHw = &hwEffectsLevel;
    g_zGame_Options_PointerCache.videoAcceleration = &videoAcceleration;

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
    g_zGame_Options_PointerCache.effectsLevelSw = oldSwEffectsLevel;
    g_zGame_Options_PointerCache.effectsLevelHw = oldHwEffectsLevel;
    g_zGame_Options_PointerCache.videoAcceleration = oldVideoAcceleration;
    g_zOpt_HwMode = oldHwMode;

    return swZeroForcedOk && swRangeOk && hwDirectOk ? 0 : 1;
}

extern "C" int zhud_options_panel_effects_sync_from_options_smoke(void) {
    int swEffectsLevel = 0;
    int hwEffectsLevel = 0;
    int *const oldSwEffectsLevel = g_zGame_Options_PointerCache.effectsLevelSw;
    int *const oldHwEffectsLevel = g_zGame_Options_PointerCache.effectsLevelHw;
    const int oldHwMode = g_zOpt_HwMode;
    const int oldConditionalEffectLevel = g_zEffect_ConditionalEffectLevel;

    g_zGame_Options_PointerCache.effectsLevelSw = &swEffectsLevel;
    g_zGame_Options_PointerCache.effectsLevelHw = &hwEffectsLevel;

    HudUiOptionsPanel_Effects effects;
    effects.Constructor();
    effects.itemCount = 3;
    effects.firstIndex = 0;
    effects.visibleCount = 3;

    g_zOpt_HwMode = 0;
    effects.selectedIndex = 0;
    effects.SyncFromOptions();
    const bool swAdvanceOk =
        effects.selectedIndex == 1 &&
        swEffectsLevel == 1 &&
        g_zEffect_ConditionalEffectLevel == 1;

    effects.selectedIndex = 2;
    effects.SyncFromOptions();
    const bool swWrapOk =
        effects.selectedIndex == 0 &&
        swEffectsLevel == 0 &&
        g_zEffect_ConditionalEffectLevel == 2;

    g_zOpt_HwMode = 1;
    effects.selectedIndex = 1;
    effects.SyncFromOptions();
    const bool hwAdvanceOk =
        effects.selectedIndex == 2 &&
        hwEffectsLevel == 2 &&
        g_zEffect_ConditionalEffectLevel == 0;

    effects.DestructorCore();
    g_zGame_Options_PointerCache.effectsLevelSw = oldSwEffectsLevel;
    g_zGame_Options_PointerCache.effectsLevelHw = oldHwEffectsLevel;
    g_zOpt_HwMode = oldHwMode;
    g_zEffect_ConditionalEffectLevel = oldConditionalEffectLevel;

    return swAdvanceOk && swWrapOk && hwAdvanceOk ? 0 : 1;
}

extern "C" int zhud_options_panel_sound_active_init_from_options_smoke(void) {
    int muteSound = 0;
    int *const oldMuteSound = g_zGame_Options_PointerCache.muteSound;

    g_zGame_Options_PointerCache.muteSound = &muteSound;

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
    g_zGame_Options_PointerCache.muteSound = oldMuteSound;

    return unmutedOk && mutedOk ? 0 : 1;
}

extern "C" int zhud_options_panel_sound_active_sync_from_options_smoke(void) {
    int muteSound = 0;
    int *const oldMuteSound = g_zGame_Options_PointerCache.muteSound;

    g_zGame_Options_PointerCache.muteSound = &muteSound;

    HudUiOptionsPanel_SoundActive soundActive;
    soundActive.Constructor();
    soundActive.modeOrEnabled = 1;

    soundActive.checked = 0;
    soundActive.SyncFromOptions();
    const bool unmutedOk = soundActive.checked == 1 && muteSound == 0;

    soundActive.SyncFromOptions();
    const bool mutedOk = soundActive.checked == 0 && muteSound == 1;

    soundActive.DestructorCore();
    g_zGame_Options_PointerCache.muteSound = oldMuteSound;

    return unmutedOk && mutedOk ? 0 : 1;
}

extern "C" int zhud_options_panel_sound_quality_init_from_options_smoke(void) {
    int soundLod = 2;
    int *const oldSoundLod = g_zGame_Options_PointerCache.soundLod;

    g_zGame_Options_PointerCache.soundLod = &soundLod;

    HudUiOptionsPanel_SoundQuality soundQuality;
    soundQuality.Constructor();
    soundQuality.itemCount = 4;
    soundQuality.firstIndex = 1;
    soundQuality.visibleCount = 3;

    soundLod = 0;
    soundQuality.selectedIndex = 9;
    soundQuality.InitFromOptions();
    const bool lowClampOk = soundQuality.selectedIndex == 1;

    soundLod = 2;
    soundQuality.selectedIndex = 9;
    soundQuality.InitFromOptions();
    const bool selectionOk = soundQuality.selectedIndex == 2;

    soundLod = 3;
    soundQuality.InitFromOptions();
    const bool visibleClampOk = soundQuality.selectedIndex == 2;

    soundQuality.DestructorCore();
    g_zGame_Options_PointerCache.soundLod = oldSoundLod;

    return lowClampOk && selectionOk && visibleClampOk ? 0 : 1;
}

extern "C" int zhud_options_panel_sound_quality_sync_from_options_smoke(void) {
    int soundLod = 0;
    int *const oldSoundLod = g_zGame_Options_PointerCache.soundLod;

    g_zGame_Options_PointerCache.soundLod = &soundLod;

    HudUiOptionsPanel_SoundQuality soundQuality;
    soundQuality.Constructor();
    soundQuality.itemCount = 3;
    soundQuality.firstIndex = 0;
    soundQuality.visibleCount = 3;

    soundQuality.selectedIndex = 0;
    soundQuality.SyncFromOptions();
    const bool advanceOk =
        soundQuality.selectedIndex == 1 && soundLod == 1;

    soundQuality.selectedIndex = 2;
    soundQuality.SyncFromOptions();
    const bool wrapOk =
        soundQuality.selectedIndex == 0 && soundLod == 0;

    soundQuality.DestructorCore();
    g_zGame_Options_PointerCache.soundLod = oldSoundLod;

    return advanceOk && wrapOk ? 0 : 1;
}

extern "C" int zhud_options_panel_sound_volume_sync_from_options_smoke(void) {
    float soundVolume = 0.625f;
    float *const oldSoundVolume = g_zGame_Options_PointerCache.soundVolume;
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;

    g_zGame_Options_PointerCache.soundVolume = &soundVolume;
    g_HudUi_InvalidateMask = 0x80;

    HudUiOptionsPanel_SoundVolume soundVolumeWidget;
    soundVolumeWidget.flags = 0;
    soundVolumeWidget.SyncFromOptions();

    const bool synced =
        soundVolumeWidget.normalizedValue == 0.625f &&
        (soundVolumeWidget.flags & 0x80u) != 0;

    g_zGame_Options_PointerCache.soundVolume = oldSoundVolume;
    g_HudUi_InvalidateMask = oldInvalidateMask;

    return synced ? 0 : 1;
}

extern "C" int zhud_options_panel_sound_volume_on_activate_smoke(void) {
    float soundVolume = 0.0f;
    float globalVolume = 1.0f;
    float *const oldSoundVolume = g_zGame_Options_PointerCache.soundVolume;
    void *const oldGlobalVolumeScalePtr = g_zSnd_GlobalVolumeScalePtr;
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;
    __declspec(align(4)) unsigned char ownerStorage[sizeof(HudUiBackground)] = {0};

    g_zGame_Options_PointerCache.soundVolume = &soundVolume;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_HudUi_InvalidateMask = 0x80;

    zInput::MouseStateSnapshot *const mouseState =
        (zInput::MouseStateSnapshot *)(ownerStorage + 0x14);
    mouseState->cursorClientX = 35;

    zVidImagePartial baseImage = {0};
    baseImage.width = 100;
    zVidImagePartial fillImage = {0};
    fillImage.width = 100;
    fillImage.height = 8;

    HudUiOptionsPanel_SoundVolume soundVolumeWidget;
    soundVolumeWidget.owner = (HudUiBackground *)ownerStorage;
    soundVolumeWidget.x = 10;
    soundVolumeWidget.image = &baseImage;
    soundVolumeWidget.fillImage = &fillImage;
    soundVolumeWidget.flags = 0;

    soundVolumeWidget.OnActivate();

    const bool activated =
        soundVolumeWidget.normalizedValue == 0.25f &&
        soundVolume == 0.25f &&
        globalVolume == 0.25f &&
        soundVolumeWidget.fillRect.right == 25 &&
        soundVolumeWidget.fillRect.bottom == 8 &&
        (soundVolumeWidget.flags & 0x80u) != 0;

    soundVolumeWidget.fillImage = 0;
    soundVolumeWidget.previewImage = 0;
    soundVolumeWidget.image = 0;
    g_zGame_Options_PointerCache.soundVolume = oldSoundVolume;
    g_zSnd_GlobalVolumeScalePtr = oldGlobalVolumeScalePtr;
    g_HudUi_InvalidateMask = oldInvalidateMask;

    return activated ? 0 : 1;
}

extern "C" int zhud_options_panel_music_enable_sync_from_options_smoke(void) {
    int cdAudio = 1;
    int *const oldCdAudio = g_zGame_Options_PointerCache.cdAudio;

    g_zGame_Options_PointerCache.cdAudio = &cdAudio;

    HudUiOptionsPanel_MusicEnable musicEnable;
    musicEnable.Constructor();

    musicEnable.checked = 0;
    musicEnable.SyncFromOptions();
    const bool enabledOk = musicEnable.checked == 1;

    cdAudio = 0;
    musicEnable.checked = 9;
    musicEnable.SyncFromOptions();
    const bool disabledOk = musicEnable.checked == 0;

    musicEnable.DestructorCore();
    g_zGame_Options_PointerCache.cdAudio = oldCdAudio;

    return enabledOk && disabledOk ? 0 : 1;
}

extern "C" int zhud_options_panel_music_enable_on_activate_smoke(void) {
    int cdAudio = -1;
    int *const oldCdAudio = g_zGame_Options_PointerCache.cdAudio;
    g_zGame_Options_PointerCache.cdAudio = &cdAudio;

    OptionsPanelFunctionPatch playPatch = {0};
    OptionsPanelFunctionPatch stopPatch = {0};
    if (!PatchOptionsPanelFunctionJump(
            (void *)(&zSndCd::PlayTrackWithMode),
            (void *)(&FakeMusicEnablePlayTrackWithMode),
            playPatch
        )) {
        g_zGame_Options_PointerCache.cdAudio = oldCdAudio;
        return 1;
    }

    if (!PatchOptionsPanelFunctionJump(
            (void *)(&zSndCd::Stop),
            (void *)(&FakeMusicEnableStop),
            stopPatch
        )) {
        RestoreOptionsPanelFunctionPatch(playPatch);
        g_zGame_Options_PointerCache.cdAudio = oldCdAudio;
        return 2;
    }

    HudUiOptionsPanel_MusicEnable musicEnable;
    musicEnable.Constructor();
    musicEnable.modeOrEnabled = 1;

    g_musicEnablePlayTrackCount = 0;
    g_musicEnablePlayTrack = 0;
    g_musicEnablePlayMode = 0;
    g_musicEnableStopCount = 0;

    musicEnable.checked = 0;
    musicEnable.OnActivate();
    const bool enabledOk =
        musicEnable.checked == 1 &&
        cdAudio == 1 &&
        g_musicEnablePlayTrackCount == 1 &&
        g_musicEnablePlayTrack == 2 &&
        g_musicEnablePlayMode == 5 &&
        g_musicEnableStopCount == 0;

    musicEnable.OnActivate();
    const bool disabledOk =
        musicEnable.checked == 0 &&
        cdAudio == 0 &&
        g_musicEnablePlayTrackCount == 1 &&
        g_musicEnableStopCount == 1;

    musicEnable.DestructorCore();
    RestoreOptionsPanelFunctionPatch(stopPatch);
    RestoreOptionsPanelFunctionPatch(playPatch);
    g_zGame_Options_PointerCache.cdAudio = oldCdAudio;

    return enabledOk && disabledOk ? 0 : 1;
}

extern "C" int zhud_options_panel_music_volume_sync_from_options_smoke(void) {
    OptionsPanelFunctionPatch getVolumePatch = {0};
    if (!PatchOptionsPanelFunctionJump(
            (void *)(&zSndCd::GetVolume),
            (void *)(&FakeMusicVolumeGetVolume),
            getVolumePatch
        )) {
        return 1;
    }

    g_musicVolumeGetVolumeCount = 0;
    g_musicVolumePrimary = 32768;
    g_musicVolumeSecondary = 1234;
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0x80;

    HudUiOptionsPanel_MusicVolume musicVolume;
    musicVolume.flags = 0;
    musicVolume.SyncFromOptions();

    const float expected = (float)(g_musicVolumePrimary) * 1.52590219e-05f;
    const bool synced =
        g_musicVolumeGetVolumeCount == 1 &&
        OptionsPanelFloatNear(musicVolume.normalizedValue, expected) &&
        (musicVolume.flags & 0x80u) != 0;

    RestoreOptionsPanelFunctionPatch(getVolumePatch);
    g_HudUi_InvalidateMask = oldInvalidateMask;

    return synced ? 0 : 1;
}

extern "C" int zhud_options_panel_music_volume_on_activate_smoke(void) {
    OptionsPanelFunctionPatch setVolumePatch = {0};
    if (!PatchOptionsPanelFunctionJump(
            (void *)(&zSndCd::SetVolume),
            (void *)(&FakeMusicVolumeSetVolume),
            setVolumePatch
        )) {
        return 1;
    }

    __declspec(align(4)) unsigned char ownerStorage[sizeof(HudUiBackground)] = {0};
    HudUiBackground *const owner = (HudUiBackground *)(ownerStorage);
    owner->mouseState.cursorClientX = 35;

    zVidImagePartial baseImage = {0};
    baseImage.width = 100;
    zVidImagePartial fillImage = {0};
    fillImage.width = 100;
    fillImage.height = 8;

    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0x80;
    g_musicVolumeSetVolumeCount = 0;
    g_musicVolumeSetPrimary = 0;
    g_musicVolumeSetSecondary = 0;

    HudUiOptionsPanel_MusicVolume musicVolume;
    musicVolume.owner = owner;
    musicVolume.x = 10;
    musicVolume.image = &baseImage;
    musicVolume.fillImage = &fillImage;
    musicVolume.flags = 0;
    musicVolume.OnActivate();

    const unsigned short expectedVolume = (unsigned short)(0.25f * 65535.0f);
    const bool activated =
        OptionsPanelFloatNear(musicVolume.normalizedValue, 0.25f) &&
        g_musicVolumeSetVolumeCount == 1 &&
        g_musicVolumeSetPrimary == expectedVolume &&
        g_musicVolumeSetSecondary == expectedVolume &&
        musicVolume.fillRect.right == 25 &&
        musicVolume.fillRect.bottom == 8 &&
        (musicVolume.flags & 0x80u) != 0;

    musicVolume.fillImage = 0;
    musicVolume.previewImage = 0;
    musicVolume.image = 0;
    RestoreOptionsPanelFunctionPatch(setVolumePatch);
    g_HudUi_InvalidateMask = oldInvalidateMask;

    return activated ? 0 : 1;
}

extern "C" int zhud_options_panel_resolution_sync_from_options_smoke(void) {
    static const int hardwareCases[6][4] = {
        {2, 3, 3, 4},
        {3, 1, 1, 2},
        {4, 2, 2, 3},
        {5, 0, 0, 1},
        {6, 4, 4, 5},
        {7, 5, 5, 6}
    };
    static const int softwareCases[6][4] = {
        {2, 3, 2, 4},
        {3, 1, 0, 2},
        {4, 2, 2, 4},
        {5, 0, 0, 2},
        {6, 4, 4, 5},
        {7, 5, 5, 6}
    };
    int videoMode = 2;
    int videoAcceleration = 1;
    int *const oldVideoMode = g_zGame_Options_PointerCache.videoMode;
    int *const oldVideoAcceleration = g_zGame_Options_PointerCache.videoAcceleration;

    g_zGame_Options_PointerCache.videoMode = &videoMode;
    g_zGame_Options_PointerCache.videoAcceleration = &videoAcceleration;

    HudUiOptionsPanel_Resolution resolution;
    resolution.Constructor();
    resolution.itemCount = 20;

    int index;
    bool hardwareOk = true;
    videoAcceleration = 1;
    for (index = 0; index < 6; ++index) {
        videoMode = hardwareCases[index][0];
        resolution.selectedIndex = 19;
        resolution.firstIndex = 0;
        resolution.visibleCount = 20;
        resolution.SyncFromOptions();
        if (resolution.selectedIndex != hardwareCases[index][1] ||
            resolution.firstIndex != hardwareCases[index][2] ||
            resolution.visibleCount != hardwareCases[index][3]) {
            hardwareOk = false;
        }
    }

    bool softwareOk = true;
    videoAcceleration = 0;
    for (index = 0; index < 6; ++index) {
        videoMode = softwareCases[index][0];
        resolution.selectedIndex = 19;
        resolution.firstIndex = 0;
        resolution.visibleCount = 20;
        resolution.SyncFromOptions();
        if (resolution.selectedIndex != softwareCases[index][1] ||
            resolution.firstIndex != softwareCases[index][2] ||
            resolution.visibleCount != softwareCases[index][3]) {
            softwareOk = false;
        }
    }

    videoMode = 1;
    videoAcceleration = 1;
    resolution.selectedIndex = 2;
    resolution.firstIndex = 1;
    resolution.visibleCount = 3;
    resolution.SyncFromOptions();
    const bool lowOutOfRangeOk =
        resolution.selectedIndex == 2 &&
        resolution.firstIndex == 1 &&
        resolution.visibleCount == 3;

    videoMode = 99;
    resolution.SyncFromOptions();
    const bool highOutOfRangeOk =
        resolution.selectedIndex == 2 &&
        resolution.firstIndex == 1 &&
        resolution.visibleCount == 3;

    resolution.DestructorCore();
    g_zGame_Options_PointerCache.videoMode = oldVideoMode;
    g_zGame_Options_PointerCache.videoAcceleration = oldVideoAcceleration;

    return hardwareOk && softwareOk && lowOutOfRangeOk && highOutOfRangeOk
               ? 0
               : 1;
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
