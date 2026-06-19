#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include <string.h>

extern "C" unsigned int g_HudUi_InvalidateMask;

static int g_musicVolumeGetVolumeCount;
static unsigned short g_musicVolumePrimary;
static unsigned short g_musicVolumeSecondary;
static int g_musicVolumeSetVolumeCount;
static unsigned short g_musicVolumeSetPrimary;
static unsigned short g_musicVolumeSetSecondary;
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

static void FakePerspectiveSelectSpanRoutines(void) {
    ++g_perspectiveSelectSpanCount;
}

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

extern "C" int zhud_options_panel_perspective_sync_from_options_smoke(void) {
    int swFlags = 0;
    int hwFlags = 0x20;
    int *const oldSwFlags = ZOPT_GFX_FLAGS_SW;
    int *const oldHwFlags = ZOPT_GFX_FLAGS_HW;
    const int oldHwMode = g_zOpt_HwMode;
    OptionsPanelFunctionPatch selectSpanPatch = {0};

    if (!PatchOptionsPanelFunctionJump(
            (void *)(&zRndr::SelectSpanRoutines),
            (void *)(&FakePerspectiveSelectSpanRoutines),
            selectSpanPatch
        )) {
        return 1;
    }

    ZOPT_GFX_FLAGS_SW = &swFlags;
    ZOPT_GFX_FLAGS_HW = &hwFlags;

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
    ZOPT_GFX_FLAGS_SW = oldSwFlags;
    ZOPT_GFX_FLAGS_HW = oldHwFlags;
    g_zOpt_HwMode = oldHwMode;
    RestoreOptionsPanelFunctionPatch(selectSpanPatch);

    return setOk && clearOk && swOk ? 0 : 1;
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

extern "C" int zhud_options_panel_sound_quality_init_from_options_smoke(void) {
    int soundLod = 2;
    int *const oldSoundLod = ZOPT_SOUND_LOD;

    ZOPT_SOUND_LOD = &soundLod;

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
    ZOPT_SOUND_LOD = oldSoundLod;

    return lowClampOk && selectionOk && visibleClampOk ? 0 : 1;
}

extern "C" int zhud_options_panel_sound_quality_sync_from_options_smoke(void) {
    int soundLod = 0;
    int *const oldSoundLod = ZOPT_SOUND_LOD;

    ZOPT_SOUND_LOD = &soundLod;

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
    ZOPT_SOUND_LOD = oldSoundLod;

    return advanceOk && wrapOk ? 0 : 1;
}

extern "C" int zhud_options_panel_sound_volume_sync_from_options_smoke(void) {
    float soundVolume = 0.625f;
    float *const oldSoundVolume = ZOPT_SOUND_VOLUME;
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;

    ZOPT_SOUND_VOLUME = &soundVolume;
    g_HudUi_InvalidateMask = 0x80;

    HudUiOptionsPanel_SoundVolume soundVolumeWidget;
    soundVolumeWidget.Constructor();
    soundVolumeWidget.flags = 0;
    soundVolumeWidget.SyncFromOptions();

    const bool synced =
        soundVolumeWidget.normalizedValue == 0.625f &&
        (soundVolumeWidget.flags & 0x80u) != 0;

    soundVolumeWidget.DestructorCore();
    ZOPT_SOUND_VOLUME = oldSoundVolume;
    g_HudUi_InvalidateMask = oldInvalidateMask;

    return synced ? 0 : 1;
}

extern "C" int zhud_options_panel_sound_volume_on_activate_smoke(void) {
    float soundVolume = 0.0f;
    float globalVolume = 1.0f;
    float *const oldSoundVolume = ZOPT_SOUND_VOLUME;
    void *const oldGlobalVolumeScalePtr = g_zSnd_GlobalVolumeScalePtr;
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;
    __declspec(align(4)) unsigned char ownerStorage[sizeof(HudUiBackground)] = {0};

    ZOPT_SOUND_VOLUME = &soundVolume;
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
    soundVolumeWidget.Constructor();
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
    soundVolumeWidget.DestructorCore();
    ZOPT_SOUND_VOLUME = oldSoundVolume;
    g_zSnd_GlobalVolumeScalePtr = oldGlobalVolumeScalePtr;
    g_HudUi_InvalidateMask = oldInvalidateMask;

    return activated ? 0 : 1;
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
    musicVolume.Constructor();
    musicVolume.flags = 0;
    musicVolume.SyncFromOptions();

    const float expected = (float)(g_musicVolumePrimary) * 1.52590219e-05f;
    const bool synced =
        g_musicVolumeGetVolumeCount == 1 &&
        OptionsPanelFloatNear(musicVolume.normalizedValue, expected) &&
        (musicVolume.flags & 0x80u) != 0;

    musicVolume.DestructorCore();
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
    musicVolume.Constructor();
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
    musicVolume.DestructorCore();
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
    int *const oldVideoMode = ZOPT_VIDEO_MODE;
    int *const oldVideoAcceleration = ZOPT_VIDEO_ACCELERATION;

    ZOPT_VIDEO_MODE = &videoMode;
    ZOPT_VIDEO_ACCELERATION = &videoAcceleration;

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
    ZOPT_VIDEO_MODE = oldVideoMode;
    ZOPT_VIDEO_ACCELERATION = oldVideoAcceleration;

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
