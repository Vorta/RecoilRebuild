#include "Battlesport/Mfc42Abi.h"

#include "zGame.h"

#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "zClass.h"

#include <windows.h>

#include <intrin.h>
#include <malloc.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

namespace Player {
void __fastcall ApplyCameraState(int newState);
}

extern "C" {
extern char g_zGame_Options_RegRootPrefix[];
/**
 * Reimplements data 0x4e4668: g_zGame_Options_RegKeyVersionSegment.
 * Purpose: points at the writable registry-root prefix segment used for
 * options registry key construction.
 */
char *g_zGame_Options_RegKeyVersionSegment = g_zGame_Options_RegRootPrefix;
/**
 * Reimplements data 0x4e466c: g_zGame_Options_RegRootPrefix.
 * Purpose: stores the writable SOFTWARE\ registry-root prefix.
 */
char g_zGame_Options_RegRootPrefix[] = "SOFTWARE\\";
/**
 * Reimplements data 0x4e4678: g_zGame_Options_RegPathSeparator.
 * Purpose: stores the writable registry path separator.
 */
char g_zGame_Options_RegPathSeparator[] = "\\";
/**
 * Reimplements data 0x56bcd0: g_zGame_Options_OptionListHead.
 * Reimplements data 0x56bcd8: g_zGame_Options_RegKeyRoot.
 * Reimplements data 0x56bcdc: g_zGame_Options_RegKeyCurrentUser.
 * Reimplements data 0x56bce0: g_zGame_Options_RegKeyGame.
 * Reimplements data 0x56bcd4: g_zGame_Options_RegContextInitialized.
 * Purpose: stores the runtime registry option-list head and allocated registry
 * key context pointers for zGame option load/save.
 */
zOptionEntryPartial *g_zGame_Options_OptionListHead = 0;
char *g_zGame_Options_RegKeyRoot = 0;
char *g_zGame_Options_RegKeyCurrentUser = 0;
char *g_zGame_Options_RegKeyGame = 0;
int g_zGame_Options_RegContextInitialized = 0;
zGame_OptionsRuntimeConfig g_zGame_Options_RuntimeConfigDefaults = {0};
/**
 * Reimplements data 0x4e5d54: ZOPT_VIDEO_FULLSCREEN.
 * Purpose: Stores ZOPT VIDEO FULLSCREEN data used by engine.zgame.zopt_fullscreen_option_global.
 */
int *ZOPT_VIDEO_FULLSCREEN = 0;
/**
 * Reimplements data 0x4e5d70: ZOPT_VIDEO_STRIDE.
 * Purpose: stores the option-value pointer populated by game option loading
 * for video stride.
 */
int *ZOPT_VIDEO_STRIDE = 0;
int *ZOPT_HUD_SW = 0;
int *ZOPT_HUD_HW = 0;
/**
 * Reimplements data 0x4e5d28: ZOPT_HUD_TYPE_SW.
 * Reimplements data 0x4e5d2c: ZOPT_HUD_TYPE_HW.
 * Reimplements data 0x4e5d6c: ZOPT_REPLICATE.
 * Purpose: stores option-value pointers for HUD type in software/hardware
 * modes and video replicate mode.
 */
int *ZOPT_HUD_TYPE_SW = 0;
int *ZOPT_HUD_TYPE_HW = 0;
int *ZOPT_REPLICATE = 0;
int *ZOPT_NETWORK_ENABLED = 0;
/**
 * Reimplements data 0x4e5d90: g_zOpt_NetworkModemOption.
 * Reimplements data 0x4e5d78: g_zOpt_NetworkListenOption.
 * Purpose: stores option-value pointers for network modem and network listen
 * configuration.
 */
int *g_zOpt_NetworkModemOption = 0;
int *g_zOpt_NetworkListenOption = 0;
/**
 * Reimplements data 0x4e5d48: g_zOpt_GameDifficultyOption.
 * Purpose: Stores g zOpt GameDifficultyOption data used by engine.zgame.zopt_game_difficulty_option_global.
 */
int *g_zOpt_GameDifficultyOption = 0;
/**
 * Reimplements data 0x4e5d94: g_zOpt_WolPasswordFlagOption.
 * Reimplements data 0x4e5d00: ZOPT_EFFECTS_LEVEL_SW.
 * Reimplements data 0x4e5d04: ZOPT_EFFECTS_LEVEL_HW.
 * Reimplements data 0x4e5d10: ZOPT_OBJECT_LOD_SW.
 * Reimplements data 0x4e5d14: ZOPT_OBJECT_LOD_HW.
 * Purpose: stores option-value pointers populated by game option loading for
 * the WOL password flag, effects level, and object LOD settings.
 */
int *g_zOpt_WolPasswordFlagOption = 0;
int *ZOPT_EFFECTS_LEVEL_SW = 0;
int *ZOPT_EFFECTS_LEVEL_HW = 0;
int *ZOPT_OBJECT_LOD_SW = 0;
int *ZOPT_OBJECT_LOD_HW = 0;
/**
 * Reimplements data 0x4e5d40: ZOPT_MUTE_SOUND.
 * Purpose: Stores ZOPT MUTE SOUND data used by engine.zgame.zopt_sound_option_globals.
 */
int *ZOPT_MUTE_SOUND = 0;
/**
 * Reimplements data 0x4e5d44: ZOPT_SOUND_VOLUME.
 * Purpose: Stores ZOPT SOUND VOLUME data used by engine.zgame.zopt_sound_option_globals.
 */
float *ZOPT_SOUND_VOLUME = 0;
/**
 * Reimplements data 0x4e5d38: ZOPT_SOUND_LOD.
 * Purpose: Stores ZOPT SOUND LOD data used by engine.zgame.zopt_sound_option_globals.
 */
int *ZOPT_SOUND_LOD = 0;
/**
 * Reimplements data 0x4e5d18: ZOPT_TEXTURE_MEMORY_SW.
 * Reimplements data 0x4e5d1c: ZOPT_TEXTURE_MEMORY_HW.
 * Purpose: stores option-value pointers populated by game option loading for
 * software and hardware texture-memory limits.
 */
int *ZOPT_TEXTURE_MEMORY_SW = 0;
int *ZOPT_TEXTURE_MEMORY_HW = 0;
/**
 * Reimplements data 0x4e5d4c: ZOPT_PLAYER_NAME.
 * Purpose: Stores ZOPT PLAYER NAME data used by engine.zgame.zopt_player_name_option_global.
 */
zOptionEntryPartial *ZOPT_PLAYER_NAME = 0;
/**
 * Reimplements data 0x4e5d08: ZOPT_GFX_FLAGS_SW.
 * Reimplements data 0x4e5d0c: ZOPT_GFX_FLAGS_HW.
 * Reimplements data 0x4e5d80: g_zOpt_RenderSectionOption.
 * Reimplements data 0x4e5d84: g_zOpt_DisplaySectionOption.
 * Reimplements data 0x4e5d88: g_zOpt_WindowSectionOption.
 * Reimplements data 0x4e5d7c: g_zOpt_CameraSectionOption.
 * Reimplements data 0x4e5dcc: g_zOpt_HwMode.
 * Reimplements data 0x4e5d3c: ZOPT_GAME_CONTROL_OPTIONS.
 * Purpose: stores graphics, view-section, camera-section, current hardware
 * mode, and game-control option globals used by zOpt accessors.
 */
int *ZOPT_GFX_FLAGS_SW = 0;
int *ZOPT_GFX_FLAGS_HW = 0;
zOpt_ViewRectSection **g_zOpt_RenderSectionOption = 0;
zOpt_ViewRectSection **g_zOpt_DisplaySectionOption = 0;
zOpt_ViewRectSection **g_zOpt_WindowSectionOption = 0;
zOpt_CameraSection **g_zOpt_CameraSectionOption = 0;
int g_zOpt_HwMode = 0;
zOptGameControlFlags *ZOPT_GAME_CONTROL_OPTIONS = 0;
zGame_OptionsRuntimeConfig g_zGame_Options_RuntimeConfig = {0};

/**
 * Reimplements data 0x4da63c..0x4da8b4: zOpt profile and option literal pool.
 * Purpose: preserve the writable VC5-era char globals used by profile selection
 * and option registration.
 */
/**
 * Reimplements data 0x4da63c: g_zOpt_OpStr_TolEq.
 * Purpose: Stores the writable profile comparison token for approximate equality.
 */
char g_zOpt_OpStr_TolEq[] = "~=";
/**
 * Reimplements data 0x4da640: g_zOpt_OpStr_Ne.
 * Purpose: Stores the writable profile comparison token for inequality.
 */
char g_zOpt_OpStr_Ne[] = "!=";
/**
 * Reimplements data 0x4da644: g_zOpt_OpStr_Ge.
 * Purpose: Stores the writable profile comparison token for greater-or-equal tests.
 */
char g_zOpt_OpStr_Ge[] = ">=";
/**
 * Reimplements data 0x4da648: g_zOpt_OpStr_Le.
 * Purpose: Stores the writable profile comparison token for less-or-equal tests.
 */
char g_zOpt_OpStr_Le[] = "<=";
/**
 * Reimplements data 0x4da64c: g_zOpt_OpStr_Gt.
 * Purpose: Stores the writable profile comparison token for greater-than tests.
 */
char g_zOpt_OpStr_Gt[] = ">";
/**
 * Reimplements data 0x4da650: g_zOpt_OpStr_Lt.
 * Purpose: Stores the writable profile comparison token for less-than tests.
 */
char g_zOpt_OpStr_Lt[] = "<";
/**
 * Reimplements data 0x4da654: g_zOpt_OpStr_Eq.
 * Purpose: Stores the writable profile comparison token for equality tests.
 */
char g_zOpt_OpStr_Eq[] = "==";
/**
 * Reimplements data 0x4da658: k_zOpt_ProfileMetricDefault.
 * Purpose: Stores the writable DEFAULT profile metric key.
 */
char k_zOpt_ProfileMetricDefault[] = "DEFAULT";
/**
 * Reimplements data 0x4da660: k_zOpt_ProfileMetricHwAccel.
 * Purpose: Stores the writable HW_ACCEL profile metric key.
 */
char k_zOpt_ProfileMetricHwAccel[] = "HW_ACCEL";
/**
 * Reimplements data 0x4da66c: k_zOpt_ProfileMetricRamKb.
 * Purpose: Stores the writable RAM_KB profile metric key.
 */
char k_zOpt_ProfileMetricRamKb[] = "RAM_KB";
/**
 * Reimplements data 0x4da674: k_zOpt_ProfileMetricVideoKb.
 * Purpose: Stores the writable VIDEO_KB profile metric key.
 */
char k_zOpt_ProfileMetricVideoKb[] = "VIDEO_KB";
/**
 * Reimplements data 0x4da680: k_zOpt_ProfileMetricCpuMhz.
 * Purpose: Stores the writable CPU_MHZ profile metric key.
 */
char k_zOpt_ProfileMetricCpuMhz[] = "CPU_MHZ";
/**
 * Reimplements data 0x4da688: k_zOpt_ProfileMetricCpuClass.
 * Purpose: Stores the writable CPU_CLASS profile metric key.
 */
char k_zOpt_ProfileMetricCpuClass[] = "CPU_CLASS";
/**
 * Reimplements data 0x4da694: g_zOpt_OptionName_VStride.
 * Purpose: Stores the writable option name used to register VStride.
 */
char g_zOpt_OptionName_VStride[] = "VStride";
/**
 * Reimplements data 0x4da69c: g_zOpt_OptionName_VMode.
 * Purpose: Stores the writable option name used to register VMode.
 */
char g_zOpt_OptionName_VMode[] = "VMode";
/**
 * Reimplements data 0x4da6a4: g_zOpt_OptionName_Replicate.
 * Purpose: Stores the writable option name used to register Replicate.
 */
char g_zOpt_OptionName_Replicate[] = "Replicate";
/**
 * Reimplements data 0x4da6b0: g_zOpt_OptionName_Window.
 * Purpose: Stores the writable option name used to register Window.
 */
char g_zOpt_OptionName_Window[] = "Window";
/**
 * Reimplements data 0x4da6b8: g_zOpt_OptionName_Display.
 * Purpose: Stores the writable option name used to register Display.
 */
char g_zOpt_OptionName_Display[] = "Display";
/**
 * Reimplements data 0x4da6c0: g_zOpt_OptionName_Render.
 * Purpose: Stores the writable option name used to register Render.
 */
char g_zOpt_OptionName_Render[] = "Render";
/**
 * Reimplements data 0x4da6c8: g_zOpt_OptionName_Camera.
 * Purpose: Stores the writable option name used to register Camera.
 */
char g_zOpt_OptionName_Camera[] = "Camera";
/**
 * Reimplements data 0x4da6d0: g_zOpt_OptionName_NetListen.
 * Purpose: Stores the writable option name used to register NetListen.
 */
char g_zOpt_OptionName_NetListen[] = "NetListen";
/**
 * Reimplements data 0x4da6dc: g_zOpt_OptionName_NetworkModem.
 * Purpose: Stores the writable option name used to register NetworkModem.
 */
char g_zOpt_OptionName_NetworkModem[] = "NetworkModem";
/**
 * Reimplements data 0x4da6ec: g_zOpt_OptionName_Network.
 * Purpose: Stores the writable option name used to register Network.
 */
char g_zOpt_OptionName_Network[] = "Network";
/**
 * Reimplements data 0x4da6f4: g_zOpt_OptionName_JoystickNumButtons.
 * Purpose: Stores the writable option name used to register JoystickNumButtons.
 */
char g_zOpt_OptionName_JoystickNumButtons[] = "JoystickNumButtons";
/**
 * Reimplements data 0x4da708: g_zOpt_OptionName_JoystickNumAxes.
 * Purpose: Stores the writable option name used to register JoystickNumAxes.
 */
char g_zOpt_OptionName_JoystickNumAxes[] = "JoystickNumAxes";
/**
 * Reimplements data 0x4da718: g_zOpt_OptionName_WOLPasswordFlag.
 * Purpose: Stores the writable option name used to register WOLPasswordFlag.
 */
char g_zOpt_OptionName_WOLPasswordFlag[] = "WOLPasswordFlag";
/**
 * Reimplements data 0x4da728: g_zOpt_OptionName_Joystick.
 * Purpose: Stores the writable option name used to register Joystick.
 */
char g_zOpt_OptionName_Joystick[] = "Joystick";
/**
 * Reimplements data 0x4da734: g_zOpt_OptionName_HwApi.
 * Purpose: Stores the writable option name used to register HWAPI.
 */
char g_zOpt_OptionName_HwApi[] = "HWAPI";
/**
 * Reimplements data 0x4da73c: g_zOpt_OptionName_HudTypeHw.
 * Purpose: Stores the writable option name used to register HUDType_HW.
 */
char g_zOpt_OptionName_HudTypeHw[] = "HUDType_HW";
/**
 * Reimplements data 0x4da748: g_zOpt_OptionName_HudTypeSw.
 * Purpose: Stores the writable option name used to register HUDType_SW.
 */
char g_zOpt_OptionName_HudTypeSw[] = "HUDType_SW";
/**
 * Reimplements data 0x4da754: g_zOpt_OptionName_HudFlagHw.
 * Purpose: Stores the writable option name used to register HUDFlag_HW.
 */
char g_zOpt_OptionName_HudFlagHw[] = "HUDFlag_HW";
/**
 * Reimplements data 0x4da760: g_zOpt_OptionName_HudFlagSw.
 * Purpose: Stores the writable option name used to register HUDFlag_SW.
 */
char g_zOpt_OptionName_HudFlagSw[] = "HUDFlag_SW";
/**
 * Reimplements data 0x4da76c: g_zOpt_OptionName_FullScreen.
 * Purpose: Stores the writable option name used to register FullScreen.
 */
char g_zOpt_OptionName_FullScreen[] = "FullScreen";
/**
 * Reimplements data 0x4da778: g_zOpt_OptionName_CDAudio.
 * Purpose: Stores the writable option name used to register CDAudio.
 */
char g_zOpt_OptionName_CDAudio[] = "CDAudio";
/**
 * Reimplements data 0x4da780: g_zOpt_OptionName_PlayerName.
 * Purpose: Stores the writable option name used to register PlayerName.
 */
char g_zOpt_OptionName_PlayerName[] = "PlayerName";
/**
 * Reimplements data 0x4da78c: g_zOpt_OptionName_SoundApi.
 * Purpose: Stores the writable option name used to register SoundAPI.
 */
char g_zOpt_OptionName_SoundApi[] = "SoundAPI";
/**
 * Reimplements data 0x4da798: g_zOpt_OptionName_SoundLOD.
 * Purpose: Stores the writable option name used to register SoundLOD.
 */
char g_zOpt_OptionName_SoundLOD[] = "SoundLOD";
/**
 * Reimplements data 0x4da7a4: g_zOpt_OptionName_SoundVolume.
 * Purpose: Stores the writable option name used to register SoundVolume.
 */
char g_zOpt_OptionName_SoundVolume[] = "SoundVolume";
/**
 * Reimplements data 0x4da7b0: g_zOpt_OptionName_MuteSound.
 * Purpose: Stores the writable option name used to register MuteSound.
 */
char g_zOpt_OptionName_MuteSound[] = "MuteSound";
/**
 * Reimplements data 0x4da7bc: g_zOpt_OptionName_GameIntensity.
 * Purpose: Stores the writable option name used to register GameIntensity.
 */
char g_zOpt_OptionName_GameIntensity[] = "GameIntensity";
/**
 * Reimplements data 0x4da7cc: g_zOpt_OptionName_GameCtlOptions.
 * Purpose: Stores the writable option name used to register GameCtlOptions.
 */
char g_zOpt_OptionName_GameCtlOptions[] = "GameCtlOptions";
/**
 * Reimplements data 0x4da7dc: g_zOpt_OptionName_TextureMemoryHw.
 * Purpose: Stores the writable option name used to register TextureMemory_HW.
 */
char g_zOpt_OptionName_TextureMemoryHw[] = "TextureMemory_HW";
/**
 * Reimplements data 0x4da7f0: g_zOpt_OptionName_TextureMemorySw.
 * Purpose: Stores the writable option name used to register TextureMemory_SW.
 */
char g_zOpt_OptionName_TextureMemorySw[] = "TextureMemory_SW";
/**
 * Reimplements data 0x4da804: g_zOpt_OptionName_ObjectLODHw.
 * Purpose: Stores the writable option name used to register ObjectLOD_HW.
 */
char g_zOpt_OptionName_ObjectLODHw[] = "ObjectLOD_HW";
/**
 * Reimplements data 0x4da814: g_zOpt_OptionName_ObjectLODSw.
 * Purpose: Stores the writable option name used to register ObjectLOD_SW.
 */
char g_zOpt_OptionName_ObjectLODSw[] = "ObjectLOD_SW";
/**
 * Reimplements data 0x4da824: g_zOpt_OptionName_GlobalLightHw.
 * Purpose: Stores the writable option name used to register GlobalLight_HW.
 */
char g_zOpt_OptionName_GlobalLightHw[] = "GlobalLight_HW";
/**
 * Reimplements data 0x4da834: g_zOpt_OptionName_GfxFlagsHw.
 * Purpose: Stores the writable option name used to register GfxFlags_HW.
 */
char g_zOpt_OptionName_GfxFlagsHw[] = "GfxFlags_HW";
/**
 * Reimplements data 0x4da840: g_zOpt_OptionName_AllVideoBuffer.
 * Purpose: Stores the writable option name used to register AllVideoBuffer.
 */
char g_zOpt_OptionName_AllVideoBuffer[] = "AllVideoBuffer";
/**
 * Reimplements data 0x4da850: g_zOpt_OptionName_GlobalLightSw.
 * Purpose: Stores the writable option name used to register GlobalLight_SW.
 */
char g_zOpt_OptionName_GlobalLightSw[] = "GlobalLight_SW";
/**
 * Reimplements data 0x4da860: g_zOpt_OptionName_Perspective.
 * Purpose: Stores the writable option name used to register Perspective.
 */
char g_zOpt_OptionName_Perspective[] = "Perspective";
/**
 * Reimplements data 0x4da86c: g_zOpt_OptionName_Lighting.
 * Purpose: Stores the writable option name used to register Lighting.
 */
char g_zOpt_OptionName_Lighting[] = "Lighting";
/**
 * Reimplements data 0x4da878: g_zOpt_OptionName_Transparency.
 * Purpose: Stores the writable option name used to register Transparency.
 */
char g_zOpt_OptionName_Transparency[] = "Transparency";
/**
 * Reimplements data 0x4da888: g_zOpt_OptionName_GfxFlagsSw.
 * Purpose: Stores the writable option name used to register GfxFlags_SW.
 */
char g_zOpt_OptionName_GfxFlagsSw[] = "GfxFlags_SW";
/**
 * Reimplements data 0x4da894: g_zOpt_OptionName_EffectsLevelHw.
 * Purpose: Stores the writable option name used to register EffectsLevel_HW.
 */
char g_zOpt_OptionName_EffectsLevelHw[] = "EffectsLevel_HW";
/**
 * Reimplements data 0x4da8a4: g_zOpt_OptionName_EffectsLevelSw.
 * Purpose: Stores the writable option name used to register EffectsLevel_SW.
 */
char g_zOpt_OptionName_EffectsLevelSw[] = "EffectsLevel_SW";
/**
 * Reimplements data 0x4da8b4: g_zOpt_OptionName_HwCardFlag.
 * Purpose: Stores the writable option name used to register HWCardFlag.
 */
char g_zOpt_OptionName_HwCardFlag[] = "HWCardFlag";
/**
 * Reimplements data 0x4da8c0: g_zOpt_DetailArchiveName.
 * Purpose: Stores the writable detail archive name used by game option loading.
 */
char g_zOpt_DetailArchiveName[] = "detail.zrd";
/**
 * Reimplements data 0x4da8cc: g_zOpt_DetailOptionName_Sunlight.
 * Purpose: Stores the writable node name used to apply the sunlight graphics flag.
 */
char g_zOpt_DetailOptionName_Sunlight[] = "sunlight";
}

RECOIL_STATIC_ASSERT(
    offsetof(
        zOptionEntryPartial,
        payloadOrBuffer
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zOptionEntryPartial,
        storageType
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zOptionEntryPartial,
        dataSize
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zOptionEntryPartial,
        name
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zOptionEntryPartial,
        registryScope
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zOptionEntryPartial,
        next
    ) == 0x18
);
RECOIL_STATIC_ASSERT(sizeof(zOptionEntryPartial) == 0x20);
RECOIL_STATIC_ASSERT(
    offsetof(
        zOpt_ViewRectSection,
        x
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zOpt_ViewRectSection,
        y
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zOpt_ViewRectSection,
        width
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zOpt_ViewRectSection,
        height
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zOpt_ViewRectSection,
        maxXInclusive
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zOpt_ViewRectSection,
        bitsPerPixel
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zOpt_ViewRectSection,
        target
    ) == 0x24
);
RECOIL_STATIC_ASSERT(sizeof(zOpt_ViewRectSection) == 0x28);
RECOIL_STATIC_ASSERT(
    offsetof(
        zOpt_CameraSection,
        m_pCamera
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGame_OptionsRuntimeConfig,
        cpuClass
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGame_OptionsRuntimeConfig,
        defaultFlags
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zGame_OptionsRuntimeConfig,
        soundHardwareMemKb
    ) == 0x24
);
RECOIL_STATIC_ASSERT(sizeof(zGame_OptionsRuntimeConfig) == 0x30);

/**
 * Reimplements 0x4b3090: zGame_OptionsRuntimeConfig::CopyDefault.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: copy the probed default runtime configuration into this active config.
 */
zGame_OptionsRuntimeConfig * zGame_OptionsRuntimeConfig::CopyDefault() {
    if (this == 0) {
        return &g_zGame_Options_RuntimeConfigDefaults;
    }

    memcpy(
        this,
        &g_zGame_Options_RuntimeConfigDefaults,
        sizeof(*this)
    );
    return this;
}

/**
 * Reimplements 0x4b3160: zGame_OptionsRuntimeConfig::LoadCpuVendorString.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: load the CPUID vendor string into the runtime configuration when available.
 */
RECOIL_NO_GS void zGame_OptionsRuntimeConfig::LoadCpuVendorString() {
    if (zSys::HasCpuidSupportRuntimeOptions() == 0) {
        return;
    }

    int cpuInfo[4];
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    __asm {
        lea edi, cpuInfo
        xor eax, eax
        _emit 0x0f
        _emit 0xa2
        mov dword ptr [edi], eax
        mov dword ptr [edi + 004h], ebx
        mov dword ptr [edi + 008h], ecx
        mov dword ptr [edi + 00ch], edx
    }
#else
    __cpuid(
        cpuInfo,
        0
    );
#endif
    char vendor[0x0c];
    memcpy(
        &vendor[0],
        &cpuInfo[1],
        4
    );
    memcpy(
        &vendor[4],
        &cpuInfo[3],
        4
    );
    memcpy(
        &vendor[8],
        &cpuInfo[2],
        4
    );
    strncpy(
        cpuVendor,
        vendor,
        0x0c
    );
    cpuVendor[0x0c] = '\0';
}

/**
 * Reimplements 0x4b30b0: zGame_OptionsRuntimeConfig::InitFromSystem.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: populate runtime option defaults from CPU, memory, video, and sound probes.
 */
RECOIL_NO_GS int zGame_OptionsRuntimeConfig::InitFromSystem() {
    LoadCpuVendorString();
    cpuClass = zSys::GetCpuClass();
    cpuMhz = zSys::GetCpuMhz();

    unsigned int probe = (unsigned int)(zSnd::HasMmxMixerSupport()) & 1u;
    unsigned int flags = defaultFlags;
    defaultFlags = ((flags ^ probe) & 1u) ^ flags;

    probe = ((unsigned int)(zSys::ReturnZeroStub()) & 1u) << 1;
    flags = defaultFlags;
    defaultFlags = (flags & ~2u) | probe;

    systemRamKb = zSys::GetTotalPhysKb();

    probe = ((unsigned int)(zSys::ReturnZeroStub()) & 1u) << 2;
    flags = defaultFlags;
    defaultFlags = (flags & ~4u) | probe;

    probe = ((unsigned int)(zVid::HasAcceptedHardwareRenderer()) & 1u) << 6;
    flags = defaultFlags;
    defaultFlags = (flags & ~0x40u) | probe;

    soundHardwareMemKb = 0;
    if (zSnd::AcquireCachedDirectSound(0) != 0) {
        DSCAPS caps;
        zSnd::CachedDirectSound_GetCaps(&caps);
        soundHardwareMemKb = caps.dwTotalHwMemBytes >> 10;
        zSnd::ReleaseCachedDirectSound();
    }

    reservedCapabilityValue = zSys::ReturnZeroStub();
    return 0;
}

namespace zGame {
namespace {
const int ZGAME_OPTION_INLINE_DWORD = 0;
const int ZGAME_OPTION_INLINE_BINARY4 = 1;
const int ZGAME_OPTION_INLINE_BINARY8 = 2;
const int ZGAME_OPTION_STRING_BUFFER = 3;
const int ZGAME_OPTION_HEAP_BUFFER = 5;
const int ZGAME_OPTION_STORAGE_MAX = 7;
const int ZGAME_OPTION_SCOPE_USER = 1;
const int ZGAME_OPTION_SCOPE_TRANSIENT = 2;
const int ZVID_HW_MODE_SOFTWARE = 0;
const int ZVID_HW_MODE_HARDWARE = 1;
const zOptGameControlFlags ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON = 0x08;
const int ZOPT_GRAPHICS_MMX = 1;
const int ZOPT_GRAPHICS_TRANSPARENCY = 2;
const int ZOPT_GRAPHICS_LIGHTING = 4;
const int ZOPT_GRAPHICS_PERSPECTIVE = 8;
const int ZOPT_GRAPHICS_GLOBAL_LIGHT = 0x10;
const int ZOPT_GRAPHICS_ALL_VIDEO_BUFFER = 0x20;

template <typename T>
/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in caller 0x407700 from repeated option-entry pointer casts in option loading.
 * Purpose: return an option entry as the typed option-value pointer stored by zOpt globals.
 */
T *OptionValuePointer(
    zOptionEntryPartial *entry
) {
    return (T *)(entry);
}

/**
 * Restores likely original static helper; no standalone retail function exists.
 * Observed in caller 0x407700 from repeated profile metric selection for graphics flags.
 * Purpose: build the graphics option bitmask selected for the active profile.
 */
int BuildGraphicsFlags(
    zReader::Node *profileRoot,
    const char *globalLightKey,
    int globalLightDefault
) {
    int flags = 0;
    if ((g_zGame_Options_RuntimeConfig.defaultFlags & 1u) != 0) {
        flags |= ZOPT_GRAPHICS_MMX;
    }
    if (zOpt::SelectProfileValueForSystem(
        profileRoot,
        g_zOpt_OptionName_Transparency,
        1
    ) != 0) {
        flags |= ZOPT_GRAPHICS_TRANSPARENCY;
    }
    if (zOpt::SelectProfileValueForSystem(
        profileRoot,
        g_zOpt_OptionName_Lighting,
        1
    ) != 0) {
        flags |= ZOPT_GRAPHICS_LIGHTING;
    }
    if (zOpt::SelectProfileValueForSystem(
        profileRoot,
        g_zOpt_OptionName_Perspective,
        1
    ) != 0) {
        flags |= ZOPT_GRAPHICS_PERSPECTIVE;
    }
    if (zOpt::SelectProfileValueForSystem(
        profileRoot,
        globalLightKey,
        globalLightDefault
    ) != 0) {
        flags |= ZOPT_GRAPHICS_GLOBAL_LIGHT;
    }
    if (zOpt::SelectProfileValueForSystem(
        profileRoot,
        g_zOpt_OptionName_AllVideoBuffer,
        0
    ) != 0) {
        flags |= ZOPT_GRAPHICS_ALL_VIDEO_BUFFER;
    }

    return flags;
}

/**
 * Original inline helper; no standalone retail function exists. Observed in caller 0x407700.
 * Purpose: clear all cached option value pointers before rebuilding the option list.
 */
void ResetOptionPointers() {
    ZOPT_VIDEO_ACCELERATION = 0;
    ZOPT_VIDEO_MODE = 0;
    ZOPT_HW_API = 0;
    ZOPT_VIDEO_FULLSCREEN = 0;
    ZOPT_VIDEO_STRIDE = 0;
    ZOPT_HUD_SW = 0;
    ZOPT_HUD_HW = 0;
    ZOPT_HUD_TYPE_SW = 0;
    ZOPT_HUD_TYPE_HW = 0;
    ZOPT_REPLICATE = 0;
    ZOPT_NETWORK_ENABLED = 0;
    g_zOpt_NetworkModemOption = 0;
    g_zOpt_NetworkListenOption = 0;
    g_zOpt_GameDifficultyOption = 0;
    g_zOpt_WolPasswordFlagOption = 0;
    ZOPT_EFFECTS_LEVEL_SW = 0;
    ZOPT_EFFECTS_LEVEL_HW = 0;
    ZOPT_OBJECT_LOD_SW = 0;
    ZOPT_OBJECT_LOD_HW = 0;
    ZOPT_MUTE_SOUND = 0;
    ZOPT_SOUND_VOLUME = 0;
    ZOPT_SOUND_LOD = 0;
    ZOPT_TEXTURE_MEMORY_SW = 0;
    ZOPT_TEXTURE_MEMORY_HW = 0;
    ZOPT_PLAYER_NAME = 0;
    ZOPT_GFX_FLAGS_SW = 0;
    ZOPT_GFX_FLAGS_HW = 0;
    g_zOpt_RenderSectionOption = 0;
    g_zOpt_DisplaySectionOption = 0;
    g_zOpt_WindowSectionOption = 0;
    g_zOpt_CameraSectionOption = 0;
    ZOPT_GAME_CONTROL_OPTIONS = 0;
    ZOPT_INPUT_JOYSTICK = 0;
    ZOPT_JOYSTICK_NUM_AXES = 0;
    ZOPT_JOYSTICK_NUM_BUTTONS = 0;
    ZOPT_AUDIO_API = 0;
    ZOPT_SOUND_CDAUDIO = 0;
}
} // namespace

/**
 * Reimplements 0x4076f0: zGame::ReturnOnlyStub.
 * Purpose: preserve the empty zGame stub used by the option/load cluster.
 */
void ReturnOnlyStub() {}

/**
 * Reimplements 0x4b3380: zGame::Options_FindOption.
 * Purpose: scan the registered option-entry list for an exact name match.
 */
zOptionEntryPartial *__fastcall Options_FindOption(
    const char *name
) throw() {
    for (zOptionEntryPartial *entry = g_zGame_Options_OptionListHead; entry != 0;
        entry = entry->next) {
        if (strcmp(
            name,
            entry->name
        ) == 0) {
            return entry;
        }
    }

    return 0;
}

/**
 * Reimplements 0x4b2e80: zGame::Options_GetOrCreateOption.
 * Purpose: return an existing option entry or allocate and link a typed option record.
 */
zOptionEntryPartial *__fastcall Options_GetOrCreateOption(
    const char *name,
    int storageType,
    int dataSize,
    int registryScope
) {
    zOptionEntryPartial *result = Options_FindOption(name);
    if (result != 0) {
        return result;
    }

    result = (zOptionEntryPartial *)(calloc(
        1,
        sizeof(zOptionEntryPartial)
    ));
    result->name = _strdup(name);
    result->storageType = storageType;
    result->dataSize = dataSize;
    result->registryScope = registryScope;

    switch (storageType) {
    case 0:
    case 2:
        result->dataSize = 4;
        break;

    case 1:
        result->dataSize = 8;
        break;

    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        if (dataSize == 0) {
            free(result);
            return 0;
        }
        result->payloadOrBuffer = (int)(calloc(
            1,
            dataSize
        ));
        break;

    default:
        break;
    }

    result->next = g_zGame_Options_OptionListHead;
    g_zGame_Options_OptionListHead = result;
    return result;
}

/**
 * Reimplements 0x4b3260: zGame::Options_InitRegistryContext.
 * Purpose: initialize the registry-key context and reset the option-entry list.
 */
void __fastcall Options_InitRegistryContext(
    const char *regKeyRoot,
    const char *regKeyCurrentUser,
    const char *regKeyGame
) {
    g_zGame_Options_RegKeyRoot = _strdup(regKeyRoot);
    g_zGame_Options_RegKeyCurrentUser = _strdup(regKeyCurrentUser);
    g_zGame_Options_RegKeyGame = _strdup(regKeyGame);
    g_zGame_Options_OptionListHead = 0;
    g_zGame_Options_RegContextInitialized = 1;
    g_zGame_Options_RuntimeConfigDefaults.InitFromSystem();
}

/**
 * Reimplements 0x4b2960: zGame::Options_LoadFromRegistry.
 * Purpose: load registered option payloads from the configured registry keys.
 */
RECOIL_NO_GS int Options_LoadFromRegistry() {
    const size_t subKeyLength = strlen(g_zGame_Options_RegKeyVersionSegment) +
                                strlen(g_zGame_Options_RegKeyRoot) + 1 +
                                strlen(g_zGame_Options_RegKeyCurrentUser) + 1 +
                                strlen(g_zGame_Options_RegKeyGame) + 1;
    char *const subKey = (char *)(_alloca((subKeyLength + 3u) & ~(size_t)(3u)));
    strcpy(
        subKey,
        g_zGame_Options_RegRootPrefix
    );
    strcat(
        subKey,
        g_zGame_Options_RegKeyRoot
    );
    strcat(
        subKey,
        g_zGame_Options_RegPathSeparator
    );
    strcat(
        subKey,
        g_zGame_Options_RegKeyCurrentUser
    );
    strcat(
        subKey,
        g_zGame_Options_RegPathSeparator
    );
    strcat(
        subKey,
        g_zGame_Options_RegKeyGame
    );

    HKEY currentUserKey = 0;
    if (RegOpenKeyExA(
        HKEY_CURRENT_USER,
        subKey,
        0,
        KEY_READ,
        &currentUserKey
    ) != ERROR_SUCCESS) {
        return 0;
    }

    HKEY localMachineKey = 0;
    if (RegOpenKeyExA(
        HKEY_LOCAL_MACHINE,
        subKey,
        0,
        KEY_READ,
        &localMachineKey
    ) != ERROR_SUCCESS) {
        RegCloseKey(currentUserKey);
        return 0;
    }

    for (zOptionEntryPartial *entry = g_zGame_Options_OptionListHead; entry != 0;
        entry = entry->next) {
        HKEY *key = 0;
        if (entry->registryScope == ZGAME_OPTION_SCOPE_USER) {
            key = &currentUserKey;
        } else if (entry->registryScope == 0) {
            key = &localMachineKey;
        }

        if (key == 0) {
            continue;
        }

        DWORD expectedSize = 0;
        BYTE *payload = 0;
        switch (entry->storageType) {
        case 0:
        case 1:
            expectedSize = 4;
            payload = (BYTE *)(entry);
            break;

        case 2:
            expectedSize = 8;
            payload = (BYTE *)(entry);
            break;

        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            expectedSize = (DWORD)(entry->dataSize);
            payload = (BYTE *)((unsigned int)(entry->payloadOrBuffer));
            break;

        default:
            continue;
        }

        DWORD valueType = 0;
        DWORD actualSize = 0;
        if (RegQueryValueExA(*key, entry->name, 0, &valueType, 0, &actualSize) == ERROR_SUCCESS &&
            actualSize == expectedSize) {
            RegQueryValueExA(
                *key,
                entry->name,
                0,
                &valueType,
                payload,
                &expectedSize
            );
        }
    }

    RegCloseKey(currentUserKey);
    RegCloseKey(localMachineKey);
    return 1;
}

/**
 * Reimplements 0x4b2bf0: zGame::Options_SaveToRegistry.
 * Purpose: persist registered option payloads to the configured registry keys.
 */
RECOIL_NO_GS int Options_SaveToRegistry() {
    const size_t subKeyLength = strlen(g_zGame_Options_RegKeyVersionSegment) +
                                strlen(g_zGame_Options_RegKeyRoot) + 1 +
                                strlen(g_zGame_Options_RegKeyCurrentUser) + 1 +
                                strlen(g_zGame_Options_RegKeyGame) + 1;
    char *const subKey = (char *)(_alloca((subKeyLength + 3u) & ~(size_t)(3u)));
    strcpy(
        subKey,
        g_zGame_Options_RegRootPrefix
    );
    strcat(
        subKey,
        g_zGame_Options_RegKeyRoot
    );
    strcat(
        subKey,
        g_zGame_Options_RegPathSeparator
    );
    strcat(
        subKey,
        g_zGame_Options_RegKeyCurrentUser
    );
    strcat(
        subKey,
        g_zGame_Options_RegPathSeparator
    );
    strcat(
        subKey,
        g_zGame_Options_RegKeyGame
    );

    DWORD disposition = 0;
    HKEY currentUserKey = 0;
    if (RegCreateKeyExA(
            HKEY_CURRENT_USER,
            subKey,
            0,
            0,
            REG_OPTION_RESERVED,
            KEY_WRITE,
            0,
            &currentUserKey,
            &disposition
        ) != ERROR_SUCCESS) {
        RegCloseKey(currentUserKey);
        return 0;
    }

    HKEY localMachineKey = 0;
    if (RegCreateKeyExA(
            HKEY_LOCAL_MACHINE,
            subKey,
            0,
            0,
            REG_OPTION_RESERVED,
            KEY_WRITE,
            0,
            &localMachineKey,
            &disposition
        ) != ERROR_SUCCESS) {
        RegCloseKey(currentUserKey);
        RegCloseKey(localMachineKey);
        return 0;
    }

    for (zOptionEntryPartial *entry = g_zGame_Options_OptionListHead; entry != 0;
        entry = entry->next) {
        HKEY *key = 0;
        if (entry->registryScope == ZGAME_OPTION_SCOPE_USER) {
            key = &currentUserKey;
        } else if (entry->registryScope == 0) {
            key = &localMachineKey;
        }

        if (key == 0 || (unsigned int)(entry->storageType) > ZGAME_OPTION_STORAGE_MAX) {
            continue;
        }

        DWORD valueType = REG_BINARY;
        const BYTE *payload = (const BYTE *)(entry);
        if (entry->storageType == ZGAME_OPTION_INLINE_DWORD) {
            valueType = REG_DWORD;
        } else if (entry->storageType >= ZGAME_OPTION_STRING_BUFFER) {
            payload = (const BYTE *)((unsigned int)(entry->payloadOrBuffer));
        }

        if (RegSetValueExA(*key, entry->name, 0, valueType, payload, (DWORD)(entry->dataSize)) !=
            ERROR_SUCCESS) {
            return 0;
        }
    }

    RegCloseKey(currentUserKey);
    RegCloseKey(localMachineKey);
    return 1;
}

/**
 * Reimplements 0x407e00: zGame::Options_SaveGameOptions.
 * Purpose: clear transient input/network state before saving the option registry.
 */
int Options_SaveGameOptions() {
    zInput::BindGroupList_Clear();
    zOpt::SetNetworkEnabled(0);
    zOpt::SetNetworkModemEnabled(0);
    return Options_SaveToRegistry();
}

/**
 * Reimplements 0x4b32c0: zGame::Options_ShutdownRegistryContext.
 * Purpose: free the option-entry list and registry-key context globals.
 */
void Options_ShutdownRegistryContext() {
    if (g_zGame_Options_RegContextInitialized == 0) {
        return;
    }

    zOptionEntryPartial *entry = g_zGame_Options_OptionListHead;
    while (entry != 0) {
        zOptionEntryPartial *const next = entry->next;
        if (entry->name != 0) {
            free(entry->name);
            entry->name = 0;
        }

        if (entry->storageType != ZGAME_OPTION_INLINE_DWORD &&
            entry->storageType > ZGAME_OPTION_INLINE_BINARY8 &&
            entry->storageType <= ZGAME_OPTION_STORAGE_MAX) {
            void *const payload = (void *)((unsigned int)(entry->payloadOrBuffer));
            if (payload != 0) {
                free(payload);
                entry->payloadOrBuffer = 0;
            }
        }

        free(entry);
        entry = next;
    }

    g_zGame_Options_OptionListHead = 0;
    if (g_zGame_Options_RegKeyGame != 0) {
        free(g_zGame_Options_RegKeyGame);
        g_zGame_Options_RegKeyGame = 0;
    }
    if (g_zGame_Options_RegKeyCurrentUser != 0) {
        free(g_zGame_Options_RegKeyCurrentUser);
        g_zGame_Options_RegKeyCurrentUser = 0;
    }
    if (g_zGame_Options_RegKeyRoot != 0) {
        free(g_zGame_Options_RegKeyRoot);
        g_zGame_Options_RegKeyRoot = 0;
    }

    g_zGame_Options_RegContextInitialized = 0;
}

/**
 * Reimplements 0x407700: zGame::Options_LoadGameOptions.
 * Purpose: load detail.zrd and register the game option globals.
 */
RECOIL_NO_GS int Options_LoadGameOptions() {
    ResetOptionPointers();

    zReader::Node *const detailRoot = zReader::LoadNodeFromPath(
        g_zOpt_DetailArchiveName,
        0,
        0
    );
    if (detailRoot == 0) {
        return 0;
    }

    g_zGame_Options_RuntimeConfig.CopyDefault();

    ZOPT_VIDEO_ACCELERATION = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_HwCardFlag,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_VIDEO_ACCELERATION != 0) {
        zVid::SetAccelerationOption(ZVID_HW_MODE_HARDWARE);
    }

    ZOPT_EFFECTS_LEVEL_SW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_EffectsLevelSw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_EFFECTS_LEVEL_SW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetEffectsLevelForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_EffectsLevelSw, 1)
        );
    }

    ZOPT_EFFECTS_LEVEL_HW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_EffectsLevelHw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_EFFECTS_LEVEL_HW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetEffectsLevelForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_EffectsLevelHw, 0)
        );
    }

    ZOPT_GFX_FLAGS_SW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_GfxFlagsSw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_GFX_FLAGS_SW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetGraphicsFlagsForCurrentHwMode(BuildGraphicsFlags(
            detailRoot,
            g_zOpt_OptionName_GlobalLightSw,
            0
        ));
    }

    ZOPT_GFX_FLAGS_HW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_GfxFlagsHw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_GFX_FLAGS_HW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetGraphicsFlagsForCurrentHwMode(BuildGraphicsFlags(
            detailRoot,
            g_zOpt_OptionName_GlobalLightHw,
            1
        ));
    }

    ZOPT_OBJECT_LOD_SW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_ObjectLODSw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_OBJECT_LOD_SW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetObjectLODForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_ObjectLODSw, 0)
        );
    }

    ZOPT_OBJECT_LOD_HW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_ObjectLODHw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_OBJECT_LOD_HW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetObjectLODForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_ObjectLODHw, 0)
        );
    }

    ZOPT_TEXTURE_MEMORY_SW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_TextureMemorySw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_TEXTURE_MEMORY_SW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetTextureMemoryForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_TextureMemorySw, 0)
        );
    }

    ZOPT_TEXTURE_MEMORY_HW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_TextureMemoryHw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_TEXTURE_MEMORY_HW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetTextureMemoryForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_TextureMemoryHw, 0)
        );
    }

    ZOPT_GAME_CONTROL_OPTIONS = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_GameCtlOptions,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_GAME_CONTROL_OPTIONS != 0) {
        zOpt::SetGameControlOptions(ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON);
    }

    g_zOpt_GameDifficultyOption = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_GameIntensity,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zOpt_GameDifficultyOption != 0) {
        zOpt::SetGameDifficultyMode(1);
    }

    ZOPT_MUTE_SOUND = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_MuteSound,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_MUTE_SOUND != 0) {
        zOpt::SetMuteSoundOption(0);
    }

    ZOPT_SOUND_VOLUME = OptionValuePointer<float>(Options_GetOrCreateOption(
        g_zOpt_OptionName_SoundVolume,
        ZGAME_OPTION_INLINE_BINARY4,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_SOUND_VOLUME != 0) {
        zOpt::SetSoundVolumeOption(1.0f);
    }

    ZOPT_SOUND_LOD = OptionValuePointer<int>(
        Options_GetOrCreateOption(g_zOpt_OptionName_SoundLOD, ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER)
    );
    if (ZOPT_SOUND_LOD != 0) {
        zOpt::SetSoundLODOption(zOpt::SelectProfileValueForSystem(
            detailRoot,
            g_zOpt_OptionName_SoundLOD,
            0
        ));
    }

    ZOPT_AUDIO_API = OptionValuePointer<int>(
        Options_GetOrCreateOption(g_zOpt_OptionName_SoundApi, ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER)
    );
    if (ZOPT_AUDIO_API != 0) {
        zSnd::SetAudioApiOption(1);
    }

    ZOPT_PLAYER_NAME = Options_GetOrCreateOption(
        g_zOpt_OptionName_PlayerName,
        ZGAME_OPTION_STRING_BUFFER,
        0x16,
        ZGAME_OPTION_SCOPE_USER
    );
    if (ZOPT_PLAYER_NAME != 0) {
        DWORD userNameSize = 0xfe;
        char userName[0x100];
        GetUserNameA(
            userName,
            &userNameSize
        );
        userName[userNameSize] = '\0';
        zOpt::SetPlayerName(userName);
    }

    ZOPT_SOUND_CDAUDIO = OptionValuePointer<int>(
        Options_GetOrCreateOption(g_zOpt_OptionName_CDAudio, ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER)
    );
    if (ZOPT_SOUND_CDAUDIO != 0) {
        zSnd::SetCDAudioOption(1);
    }

    ZOPT_VIDEO_FULLSCREEN = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_FullScreen,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_VIDEO_FULLSCREEN != 0) {
        zOpt::SetFullscreenOption(1);
    }

    ZOPT_HUD_SW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_HudFlagSw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_HUD_SW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetHudVisibilityOption(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_HudFlagSw, 1)
        );
    }

    ZOPT_HUD_HW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_HudFlagHw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_HUD_HW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetHudVisibilityOption(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_HudFlagHw, 1)
        );
    }

    ZOPT_HUD_TYPE_SW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_HudTypeSw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_HUD_TYPE_SW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetHudTypeForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_HudTypeSw, 1)
        );
    }

    ZOPT_HUD_TYPE_HW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_HudTypeHw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_HUD_TYPE_HW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetHudTypeForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_HudTypeHw, 1)
        );
    }

    ZOPT_HW_API = OptionValuePointer<int>(
        Options_GetOrCreateOption(g_zOpt_OptionName_HwApi, ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER)
    );
    if (ZOPT_HW_API != 0) {
        zVid::SetHwApiOption(1);
    }

    ZOPT_INPUT_JOYSTICK = OptionValuePointer<int>(
        Options_GetOrCreateOption(g_zOpt_OptionName_Joystick, ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER)
    );
    if (ZOPT_INPUT_JOYSTICK != 0) {
        zInp::SetJoystickOption(0);
    }

    g_zOpt_WolPasswordFlagOption = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_WOLPasswordFlag,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zOpt_WolPasswordFlagOption != 0) {
        zOpt::SetWolPasswordFlag(1);
    }

    ZOPT_JOYSTICK_NUM_AXES = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_JoystickNumAxes,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    if (ZOPT_JOYSTICK_NUM_AXES != 0) {
        zInp::SetJoystickAxesCountOption(0);
    }

    ZOPT_JOYSTICK_NUM_BUTTONS = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_JoystickNumButtons,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    if (ZOPT_JOYSTICK_NUM_BUTTONS != 0) {
        zInp::SetJoystickButtonCountOption(0);
    }

    ZOPT_NETWORK_ENABLED = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_Network,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    if (ZOPT_NETWORK_ENABLED != 0) {
        zOpt::SetNetworkEnabled(0);
    }

    g_zOpt_NetworkModemOption = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_NetworkModem,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    if (g_zOpt_NetworkModemOption != 0) {
        zOpt::SetNetworkModemEnabled(0);
    }

    g_zOpt_NetworkListenOption = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_NetListen,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    if (g_zOpt_NetworkListenOption != 0) {
        zOpt::SetNetworkListenEnabled(0);
    }

    g_zOpt_CameraSectionOption = OptionValuePointer<zOpt_CameraSection *>(Options_GetOrCreateOption(
        g_zOpt_OptionName_Camera,
        ZGAME_OPTION_HEAP_BUFFER,
        0x0c,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    g_zOpt_RenderSectionOption =
        OptionValuePointer<zOpt_ViewRectSection *>(Options_GetOrCreateOption(
            g_zOpt_OptionName_Render,
            ZGAME_OPTION_HEAP_BUFFER,
            0x28,
            ZGAME_OPTION_SCOPE_TRANSIENT
        ));
    g_zOpt_DisplaySectionOption =
        OptionValuePointer<zOpt_ViewRectSection *>(Options_GetOrCreateOption(
            g_zOpt_OptionName_Display,
            ZGAME_OPTION_HEAP_BUFFER,
            0x28,
            ZGAME_OPTION_SCOPE_TRANSIENT
        ));
    g_zOpt_WindowSectionOption =
        OptionValuePointer<zOpt_ViewRectSection *>(Options_GetOrCreateOption(
            g_zOpt_OptionName_Window,
            ZGAME_OPTION_HEAP_BUFFER,
            0x28,
            ZGAME_OPTION_SCOPE_TRANSIENT
        ));
    ZOPT_REPLICATE = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_Replicate,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));

    ZOPT_VIDEO_MODE = OptionValuePointer<int>(
        Options_GetOrCreateOption(g_zOpt_OptionName_VMode, ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER)
    );
    if (ZOPT_VIDEO_MODE != 0) {
        zVid::SetVideoModeIndex(zOpt::SelectProfileValueForSystem(
            detailRoot,
            g_zOpt_OptionName_VMode,
            5
        ));
    }

    ZOPT_VIDEO_STRIDE = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_VStride,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    if (ZOPT_VIDEO_STRIDE != 0) {
        *ZOPT_VIDEO_STRIDE = 1;
    }

    zInput::BindMap_InitDefaultBindings();
    Options_LoadFromRegistry();
    zInput::BindMap_Current_RebuildLookupIndices();
    zOpt::SetNetworkEnabled(0);
    zOpt::SetNetworkModemEnabled(0);

    if (g_zOpt_CameraSectionOption != 0 && *g_zOpt_CameraSectionOption != 0) {
        (*g_zOpt_CameraSectionOption)->m_pCamera = 0;
    }
    if (g_zOpt_RenderSectionOption != 0 && *g_zOpt_RenderSectionOption != 0) {
        (*g_zOpt_RenderSectionOption)->target = 0;
    }
    if (g_zOpt_DisplaySectionOption != 0 && *g_zOpt_DisplaySectionOption != 0) {
        (*g_zOpt_DisplaySectionOption)->target = 0;
    }
    if (g_zOpt_WindowSectionOption != 0 && *g_zOpt_WindowSectionOption != 0) {
        (*g_zOpt_WindowSectionOption)->target = 0;
    }

    zReader::FreeLoadedTree(detailRoot);
    g_zOpt_HwMode = zVid::GetAccelerationOption();
    zSnd::SetAudioApiOption(zSnd::GetAudioApiOption());
    return 1;
}
} // namespace zGame

namespace zOpt {
namespace {
struct zOpt_NameInt32Pair {
    const char *name;
    int value;
};

const zOpt_NameInt32Pair g_zOpt_NamedScalarValues[] = {
    {"TRUE", 1},
    {"FALSE", 0},
    {"HIGH", 0},
    {"MEDIUM", 1},
    {"LOW", 2},
    {"CPU_CLASS_8086", 0},
    {"CPU_CLASS_80286", 2},
    {"CPU_CLASS_80386", 3},
    {"CPU_CLASS_80486", 4},
    {"CPU_CLASS_PENTIUM", 5},
    {"CPU_CLASS_PENTIUM_PRO", 6},
    {"CPU_CLASS_PENTIUM_NEWER", 7},
    {"TEXMEM_MAX", 0},
    {"TEXMEM_8MB", 1},
    {"TEXMEM_6MB", 2},
    {"TEXMEM_4MB", 3},
    {"TEXMEM_2MB", 4},
    {"ZVID_320x200x16", 2},
    {"ZVID_320x240x16", 3},
    {"ZVID_640x400x16", 4},
    {"ZVID_640x480x16", 5},
    {"ZVID_800x600x16", 6},
    {"ZVID_1024x768x16", 7},
    {"HUD_TYPEI", 1},
    {"HUD_TYPEII", 2},
    {"SOUND_API_DSOUND", 0},
    {"SOUND_API_A3D", 1},
};

const zOptGameControlFlags ZOPT_GAME_CONTROL_THROTTLE = 0x01;
const zOptGameControlFlags ZOPT_GAME_CONTROL_STEERING = 0x02;
const zOptGameControlFlags ZOPT_GAME_CONTROL_CURSOR = 0x04;
const zOptGameControlFlags ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON = 0x08;
const double ZOPT_COMPARE_TOLERANCE_PCT = 0.02;

/**
 * Original inline/static helper; no standalone retail function exists. Observed in caller
 * 0x407220.
 * Evidence basis: the branchless signed absolute-difference idiom is embedded in the
 * "~=" comparison path of zOpt::EvalIntCompareOp, with no assigned address-backed retail
 * helper for this source-file owner.
 * Purpose: compute the absolute difference used by the profile metric tolerance compare.
 */
int WrappedAbsDifference(
    int lhs,
    int rhs
) {
    const unsigned int diff = (unsigned int)(lhs) - (unsigned int)(rhs);
    const unsigned int signMask = 0u - (diff >> 31);
    return (int)((diff ^ signMask) - signMask);
}

} // namespace

/**
 * Reimplements 0x407190: zOpt::LookupNamedValueAsInt.
 * Original source path: D:\Proj\GameZRecoil\zGame\zopt.c.
 * Purpose: map profile scalar names to their integer option values.
 */
int __fastcall LookupNamedValueAsInt(
    const char *key
) {
    unsigned int pairIndex;
    for (pairIndex = 0;
        pairIndex < sizeof(g_zOpt_NamedScalarValues) / sizeof(g_zOpt_NamedScalarValues[0]);
        ++pairIndex) {
        if (strcmp(
            g_zOpt_NamedScalarValues[pairIndex].name,
            key
        ) == 0) {
            return g_zOpt_NamedScalarValues[pairIndex].value;
        }
    }

    return 0;
}

/**
 * Reimplements 0x4071f0: zOpt::ReadScalarValueAsInt.
 * Original source path: D:\Proj\GameZRecoil\zGame\zopt.c.
 * Purpose: coerce an integer, float, or named string scalar node into an integer value.
 */
int __fastcall ReadScalarValueAsInt(
    zReader::Node *scalarValueNode
) {
    if (scalarValueNode->type == zReader::ZRDR_NODE_INT) {
        return scalarValueNode->value.i32;
    }
    if (scalarValueNode->type == zReader::ZRDR_NODE_FLOAT) {
        return (int)(scalarValueNode->value.f32);
    }
    if (scalarValueNode->type == zReader::ZRDR_NODE_STRING) {
        return LookupNamedValueAsInt(scalarValueNode->value.str);
    }

    return 0;
}

/**
 * Reimplements 0x407470: zOpt::EvaluateProfileMetricCondition.
 * Original source path: D:\Proj\GameZRecoil\zGame\zopt.c.
 * Purpose: evaluate one profile-selection condition against the current runtime metrics.
 */
int __fastcall EvaluateProfileMetricCondition(
    zReader::Node *metricConditionNode
) {
    if (metricConditionNode->type == zReader::ZRDR_NODE_STRING) {
        return strcmp(
            metricConditionNode->value.str,
            k_zOpt_ProfileMetricDefault
        ) == 0;
    }

    if (metricConditionNode->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const conditionArray = metricConditionNode->value.nodes;
    if (conditionArray[0].value.i32 != 4) {
        return 0;
    }

    const char *const metricKey = conditionArray[1].value.str;
    const char *const opString = conditionArray[2].value.str;
    const int rhs = ReadScalarValueAsInt(&conditionArray[3]);
    int currentMetricValue = 0;

    if (strcmp(
        metricKey,
        k_zOpt_ProfileMetricCpuClass
    ) == 0) {
        currentMetricValue = g_zGame_Options_RuntimeConfig.cpuClass;
    } else if (strcmp(
        metricKey,
        k_zOpt_ProfileMetricCpuMhz
    ) == 0) {
        currentMetricValue = g_zGame_Options_RuntimeConfig.cpuMhz;
    } else if (strcmp(
        metricKey,
        k_zOpt_ProfileMetricVideoKb
    ) == 0) {
        currentMetricValue = (int)(g_zGame_Options_RuntimeConfig.soundHardwareMemKb);
    } else if (strcmp(
        metricKey,
        k_zOpt_ProfileMetricRamKb
    ) == 0) {
        currentMetricValue = (int)(g_zGame_Options_RuntimeConfig.systemRamKb);
    } else if (strcmp(
        metricKey,
        k_zOpt_ProfileMetricHwAccel
    ) == 0) {
        currentMetricValue = (int)((g_zGame_Options_RuntimeConfig.defaultFlags >> 6) & 1u);
    } else {
        return 0;
    }

    return EvalIntCompareOp(
        opString,
        currentMetricValue,
        rhs
    );
}

/**
 * Reimplements 0x407680: zOpt::SelectProfileValueForSystem.
 * Original source path: D:\Proj\GameZRecoil\zGame\zopt.c.
 * Purpose: choose the first matching profile rule value for the current system metrics.
 */
int __fastcall SelectProfileValueForSystem(
    zReader::Node *parentNode,
    const char *profileName,
    int defaultValue
) {
    if (parentNode == 0) {
        return defaultValue;
    }

    zReader::Node *const profileRuleListNode = zReader_GetNamedNode(
        parentNode,
        profileName
    );
    if (profileRuleListNode == 0) {
        return defaultValue;
    }

    zReader::Node *const ruleList = profileRuleListNode->value.nodes;
    const int count = ruleList[0].value.i32;
    {
        for (int ruleIndex = 1; ruleIndex < count; ++ruleIndex) {
            zReader::Node *const ruleCells = ruleList[ruleIndex].value.nodes;
            if (EvaluateProfileMetricCondition(&ruleCells[1]) != 0) {
                return ReadScalarValueAsInt(&ruleCells[2]);
            }
        }
    }

    return defaultValue;
}

/**
 * Reimplements 0x407220: zOpt::EvalIntCompareOp.
 * Original source path: D:\Proj\GameZRecoil\zGame\zopt.c.
 * Purpose: apply an integer comparison operator used by profile metric rules.
 */
int __fastcall EvalIntCompareOp(
    const char *opString,
    int lhs,
    int rhs
) {
    if (strcmp(
        opString,
        g_zOpt_OpStr_Eq
    ) == 0) {
        return lhs == rhs;
    }
    if (strcmp(
        opString,
        g_zOpt_OpStr_Lt
    ) == 0) {
        return lhs < rhs;
    }
    if (strcmp(
        opString,
        g_zOpt_OpStr_Gt
    ) == 0) {
        return lhs > rhs;
    }
    if (strcmp(
        opString,
        g_zOpt_OpStr_Le
    ) == 0) {
        return lhs <= rhs;
    }
    if (strcmp(
        opString,
        g_zOpt_OpStr_Ge
    ) == 0) {
        return lhs >= rhs;
    }
    if (strcmp(
        opString,
        g_zOpt_OpStr_Ne
    ) == 0) {
        return lhs != rhs;
    }
    if (strcmp(
        opString,
        g_zOpt_OpStr_TolEq
    ) == 0) {
        return (double)(WrappedAbsDifference(
            lhs,
            rhs
        )) < (double)(lhs)*ZOPT_COMPARE_TOLERANCE_PCT;
    }

    return 0;
}

/**
 * Reimplements 0x407e20: zOpt::SetGameControlOptions.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: replace the packed game-control option bitmask.
 */
void __fastcall SetGameControlOptions(
    zOptGameControlFlags value
) {
    *ZOPT_GAME_CONTROL_OPTIONS = value;
}

/**
 * Reimplements 0x407e30: zOpt::SetThrottleMode.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: set or clear the throttle-control bit in the game-control option mask.
 */
void __fastcall SetThrottleMode(
    int enable
) {
    if (enable != 0) {
        *ZOPT_GAME_CONTROL_OPTIONS |= ZOPT_GAME_CONTROL_THROTTLE;
    } else {
        *ZOPT_GAME_CONTROL_OPTIONS &= ~ZOPT_GAME_CONTROL_THROTTLE;
    }
}

/**
 * Reimplements 0x407e50: zOpt::GetThrottleMode.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: return the throttle-control bit from the game-control option mask.
 */
int GetThrottleMode() {
    return *ZOPT_GAME_CONTROL_OPTIONS & ZOPT_GAME_CONTROL_THROTTLE;
}

/**
 * Reimplements 0x407e60: zOpt::SetSteeringMode.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: set or clear the steering-control bit in the game-control option mask.
 */
void __fastcall SetSteeringMode(
    int enable
) {
    if (enable != 0) {
        *ZOPT_GAME_CONTROL_OPTIONS |= ZOPT_GAME_CONTROL_STEERING;
    } else {
        *ZOPT_GAME_CONTROL_OPTIONS &= ~ZOPT_GAME_CONTROL_STEERING;
    }
}

/**
 * Reimplements 0x407e80: zOpt::GetSteeringMode.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: return the steering-control bit from the game-control option mask.
 */
int GetSteeringMode() {
    return (*ZOPT_GAME_CONTROL_OPTIONS >> 1) & 1;
}

/**
 * Reimplements 0x407e90: zOpt::SetCursorMode.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: set or clear the cursor-control bit in the game-control option mask.
 */
void __fastcall SetCursorMode(
    int enable
) {
    if (enable != 0) {
        *ZOPT_GAME_CONTROL_OPTIONS |= ZOPT_GAME_CONTROL_CURSOR;
    } else {
        *ZOPT_GAME_CONTROL_OPTIONS &= ~ZOPT_GAME_CONTROL_CURSOR;
    }
}

/**
 * Reimplements 0x407eb0: zOpt::GetCursorMode.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: return the cursor-control bit from the game-control option mask.
 */
int GetCursorMode() {
    return (*ZOPT_GAME_CONTROL_OPTIONS >> 2) & 1;
}

/**
 * Reimplements 0x407ec0: zOpt::SetCameraMode.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: store first-person or third-person camera mode and apply the player camera state.
 */
void __fastcall SetCameraMode(
    int enableThirdPerson
) {
    if (enableThirdPerson != 0) {
        *ZOPT_GAME_CONTROL_OPTIONS |= ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON;
        Player::ApplyCameraState(1);
    } else {
        *ZOPT_GAME_CONTROL_OPTIONS &= ~ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON;
        Player::ApplyCameraState(3);
    }
}

/**
 * Reimplements 0x407ef0: zOpt::GetCameraModeAsPlayerCameraState.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: map the third-person camera option bit to the player camera state value.
 */
int GetCameraModePlayerState() {
    return ((~*ZOPT_GAME_CONTROL_OPTIONS & ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON) | 4) >> 2;
}

/**
 * Reimplements 0x407f10: zOpt::SetGameDifficultyMode.
 * Original source: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: Store the current game difficulty option value.
 */
void __fastcall SetGameDifficultyMode(
    int value
) {
    *g_zOpt_GameDifficultyOption = value;
}

/**
 * Reimplements 0x407f20: zOpt::GetGameDifficultyMode.
 * Original source: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: Return the current game difficulty option value.
 */
int GetGameDifficultyMode() {
    return *g_zOpt_GameDifficultyOption;
}

/**
 * Reimplements 0x407f30: zOpt::SetEffectsLevelForCurrentHwMode.
 * Purpose: store the active hardware-mode effects level and apply the matching conditional effect level.
 */
void __fastcall SetEffectsLevelForCurrentHwMode(
    int level
) {
    *(g_zOpt_HwMode != 0 ? ZOPT_EFFECTS_LEVEL_HW : ZOPT_EFFECTS_LEVEL_SW) = level;

    if (level == 0) {
        zEffect::SetConditionalEffectLevel(2);
    } else if (level == 1) {
        zEffect::SetConditionalEffectLevel(1);
    } else if (level == 2) {
        zEffect::SetConditionalEffectLevel(0);
    }
}

/**
 * Reimplements 0x407f80: zOpt::GetEffectsLevelForCurrentHwMode.
 * Purpose: return the effects level stored for the active hardware mode.
 */
int GetEffectsLevelForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? ZOPT_EFFECTS_LEVEL_HW : ZOPT_EFFECTS_LEVEL_SW);
}

/**
 * Reimplements 0x407fa0: zOpt::SetObjectLODForCurrentHwMode.
 * Purpose: store the object LOD value for the active hardware mode and apply its camera clip distance.
 */
void __fastcall SetObjectLODForCurrentHwMode(
    int level
) {
    zClass_NodePartial *const camera = zOpt_CameraSection_GetActiveCamera();
    *(g_zOpt_HwMode != 0 ? ZOPT_OBJECT_LOD_HW : ZOPT_OBJECT_LOD_SW) = level;

    if (camera == 0) {
        return;
    }

    float clipDistance = 1.0f;
    if (level == 1) {
        clipDistance = 0.75f;
    } else if (level == 2) {
        clipDistance = 0.5f;
    }

    zClass_Camera::gwCameraSetClipDistance(
        camera,
        clipDistance
    );
}

/**
 * Reimplements 0x408030: zOpt::GetObjectLODForCurrentHwMode.
 * Purpose: return the object LOD value for the active hardware mode.
 */
int GetObjectLODForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? ZOPT_OBJECT_LOD_HW : ZOPT_OBJECT_LOD_SW);
}

/**
 * Reimplements 0x408060: zOpt::GetMuteSoundOption.
 * Purpose: return the current mute-sound option value.
 */
int GetMuteSoundOption() {
    return *ZOPT_MUTE_SOUND;
}

/**
 * Reimplements 0x408050: zOpt::SetMuteSoundOption.
 * Purpose: store the mute-sound option and apply it to active sound voices.
 */
void __fastcall SetMuteSoundOption(
    int value
) {
    *ZOPT_MUTE_SOUND = value;
    zSnd::ApplyMuteStateToActiveVoices(value);
}

/**
 * Reimplements 0x408070: zOpt::SetSoundVolumeOption.
 * Purpose: store the sound-volume option and apply the global sound scale.
 */
void __fastcall SetSoundVolumeOption(
    float volume
) {
    *ZOPT_SOUND_VOLUME = volume;
    zSnd::SetGlobalVolumeScale(volume);
}

/**
 * Reimplements 0x408090: zOpt::GetSoundVolumeOption.
 * Purpose: return the current sound-volume option value.
 */
float GetSoundVolumeOption() {
    return *ZOPT_SOUND_VOLUME;
}

/**
 * Reimplements 0x4080c0: zOpt::SetSoundLODOption.
 * Purpose: store the sound LOD option value.
 */
void __fastcall SetSoundLODOption(
    int value
) {
    *ZOPT_SOUND_LOD = value;
}

/**
 * Reimplements 0x4080d0: zOpt::GetSoundLODOption.
 * Purpose: return the current sound LOD option value.
 */
int GetSoundLODOption() {
    return *ZOPT_SOUND_LOD;
}

/**
 * Reimplements 0x4080e0: zOpt::SetTextureMemoryForCurrentHwMode.
 * Purpose: store the texture memory value for the active hardware mode.
 */
void __fastcall SetTextureMemoryForCurrentHwMode(
    int value
) {
    *(g_zOpt_HwMode != 0 ? ZOPT_TEXTURE_MEMORY_HW : ZOPT_TEXTURE_MEMORY_SW) = value;
}

/**
 * Reimplements 0x408100: zOpt::GetTextureMemoryForCurrentHwMode.
 * Purpose: return the texture memory value for the active hardware mode.
 */
int GetTextureMemoryForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? ZOPT_TEXTURE_MEMORY_HW : ZOPT_TEXTURE_MEMORY_SW);
}

/**
 * Reimplements 0x408120: zOpt::SetPlayerName.
 * Original source path: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: copy the supplied player name into the configured option buffer.
 */
void __fastcall SetPlayerName(
    const char *name
) {
    char *const buffer = (char *)(ZOPT_PLAYER_NAME->payloadOrBuffer);
    const unsigned int dataSize = (unsigned int)(ZOPT_PLAYER_NAME->dataSize);
    const size_t nameLength = strlen(name);

    if (nameLength < dataSize) {
        memcpy(
            buffer,
            name,
            nameLength + 1
        );
    } else {
        strncpy(
            buffer,
            name,
            dataSize - 1
        );
        buffer[dataSize - 1] = '\0';
    }
}

/**
 * Reimplements 0x4081f0: zOpt::GetGraphicsFlagsForCurrentHwMode.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: return the graphics option bitmask for the active hardware mode.
 */
int GetGraphicsFlagsForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? ZOPT_GFX_FLAGS_HW : ZOPT_GFX_FLAGS_SW);
}

/**
 * Reimplements 0x4081a0: zOpt::SetGraphicsFlagsForCurrentHwMode.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: store the graphics option bitmask for the active hardware mode and
 * mirror its lighting bit to the sunlight node.
 */
void __fastcall SetGraphicsFlagsForCurrentHwMode(
    int flags
) {
    *(g_zOpt_HwMode != 0 ? ZOPT_GFX_FLAGS_HW : ZOPT_GFX_FLAGS_SW) = flags;

    zClass_NodePartial *const sunlight = zClass::FindByTypeAndName(
        6,
        g_zOpt_DetailOptionName_Sunlight
    );
    if (sunlight != 0) {
        zClass_Class::gwNodeSetActive(
            sunlight,
            (flags & 0x10) != 0 ? 1 : 0
        );
    }
}

/**
 * Reimplements 0x4082d0: zOpt::SetHudTypeForCurrentHwMode.
 * Purpose: apply the requested HUD layout mode and store it for the active hardware mode.
 */
int __fastcall SetHudTypeForCurrentHwMode(
    int hudType
) {
    const int previous = HudUiMgr::ApplyHudModeSwitch(hudType);

    if (g_zOpt_HwMode != 0) {
        *ZOPT_HUD_TYPE_HW = hudType;
        return previous;
    }

    *ZOPT_HUD_TYPE_SW = hudType;
    return previous;
}

/**
 * Reimplements 0x408230: zOpt::SetNetworkEnabled.
 * Original source path: D:\Proj\Battlesport\zOpt.cpp.
 * Purpose: store the network-enabled option value through its option pointer.
 */
void __fastcall SetNetworkEnabled(
    int value
) {
    *ZOPT_NETWORK_ENABLED = value;
}

/**
 * Reimplements 0x408240: zOpt::SetNetworkModemEnabled.
 * Original source path: D:\Proj\Battlesport\zOpt.cpp.
 * Purpose: store the network-modem option value through its option pointer.
 */
void __fastcall SetNetworkModemEnabled(
    int value
) {
    *g_zOpt_NetworkModemOption = value;
}

/**
 * Reimplements 0x408250: zOpt::SetNetworkListenEnabled.
 * Original source path: D:\Proj\Battlesport\zOpt.cpp.
 * Purpose: store the network-listen option value through its option pointer.
 */
void __fastcall SetNetworkListenEnabled(
    int value
) {
    *g_zOpt_NetworkListenOption = value;
}

/**
 * Reimplements 0x408270: zOpt::GetNetworkModemEnabled.
 * Original source path: D:\Proj\Battlesport\zOpt.cpp.
 * Purpose: return the network-modem option value through its option pointer.
 */
int GetNetworkModemEnabled() {
    return *g_zOpt_NetworkModemOption;
}

/**
 * Reimplements 0x408a10: zOpt::SetWolPasswordFlag.
 * Purpose: store the WOL password flag option value through its option pointer.
 */
void __fastcall SetWolPasswordFlag(
    int value
) {
    *g_zOpt_WolPasswordFlagOption = value;
}

/**
 * Reimplements 0x408650: zOpt::GetDisplaySection.
 * Original source path: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the active display view-rect option record.
 */
zOpt_ViewRectSection *GetDisplaySection() {
    return *g_zOpt_DisplaySectionOption;
}

/**
 * Reimplements 0x408690: zOpt::GetDisplaySectionBitsPerPixel.
 * Original source path: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the active display section bit depth.
 */
int GetDisplaySectionBitsPerPixel() {
    return (*g_zOpt_DisplaySectionOption)->bitsPerPixel;
}

/**
 * Reimplements 0x4086a0: zOpt::GetVideoStrideValue.
 * Original source path: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the configured video stride option value.
 */
int GetVideoStrideValue() {
    return *ZOPT_VIDEO_STRIDE;
}

/**
 * Reimplements 0x4086c0: zOpt::GetWindowSection.
 * Original source path: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the active window view-rect option record.
 */
zOpt_ViewRectSection *GetWindowSection() {
    return *g_zOpt_WindowSectionOption;
}

/**
 * Reimplements 0x4086d0: zOpt::GetWindowSectionHeight.
 * Original source path: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the active window section height.
 */
int GetWindowSectionHeight() {
    return (*g_zOpt_WindowSectionOption)->height;
}

/**
 * Reimplements 0x4082a0: zOpt::SetFullscreenOption.
 * Original source path: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: store the persisted fullscreen/windowed option value.
 */
void __fastcall SetFullscreenOption(
    int fullscreenOption
) {
    *ZOPT_VIDEO_FULLSCREEN = fullscreenOption;
}

/**
 * Reimplements 0x408330: zOpt::GetFullscreenOption.
 * Original source path: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the persisted fullscreen/windowed option value.
 */
int GetFullscreenOption() {
    return *ZOPT_VIDEO_FULLSCREEN;
}

/**
 * Reimplements 0x4082b0: zOpt::SetHudVisibilityOption.
 * Purpose: store the HUD visibility option for the active hardware mode.
 */
void __fastcall SetHudVisibilityOption(
    int hudVisibility
) {
    *(g_zOpt_HwMode != 0 ? ZOPT_HUD_HW : ZOPT_HUD_SW) = hudVisibility;
}

/**
 * Reimplements 0x408340: zOpt::GetHudVisibilityOption.
 * Purpose: return the HUD visibility option for the active hardware mode.
 */
int GetHudVisibilityOption() {
    return *(g_zOpt_HwMode != 0 ? ZOPT_HUD_HW : ZOPT_HUD_SW);
}

/**
 * Reimplements 0x408360: zOpt::GetHudTypeForCurrentHwMode.
 * Original source path: D:\Proj\Battlesport\zopt.cpp.
 * Purpose: return the HUD type option for the active hardware mode.
 */
int GetHudTypeForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? ZOPT_HUD_TYPE_HW : ZOPT_HUD_TYPE_SW);
}

/**
 * Reimplements 0x408300: zOpt::SetReplicateMode.
 * Original file: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: store the active video replicate-mode option.
 *
 * Evidence: BN writes ecx through ZOPT_REPLICATE and returns; the shared
 * zopt_video_section_setters VC5SP3 target byte-matches after relocation
 * masking.
 */
void __fastcall SetReplicateMode(
    int replicateMode
) {
    *ZOPT_REPLICATE = replicateMode;
}

/** Reimplements 0x408380: zOpt::GetReplicateMode
 * Purpose: return the active video replicate-mode option. */
int GetReplicateMode() {
    return *ZOPT_REPLICATE;
}
/**
 * Reimplements 0x408260: zOpt::GetNetworkEnabled.
 * Original source path: D:\Proj\Battlesport\zOpt.cpp.
 * Purpose: return the network-enabled option value through its option pointer.
 */
int GetNetworkEnabled() {
    return *ZOPT_NETWORK_ENABLED;
}

/** Reimplements 0x4083d0: zOpt_ViewRectSection::SetPosition
 * Purpose: store origin and recompute bounds from size. */
void __fastcall ViewRectSection_SetPosition(
    zOpt_ViewRectSection *section,
    int x,
    int y
) {
    section->x = x;
    section->y = y;
    section->rightExclusive = x + section->width;
    section->bottomExclusive = y + section->height;
    section->maxXInclusive = section->rightExclusive - 1;
    section->maxYInclusive = section->bottomExclusive - 1;
}
/** Reimplements 0x408400: zOpt_ViewRectSection::SetSize
 * Purpose: store size and recompute bounds from origin. */
void __fastcall ViewRectSection_SetSize(
    zOpt_ViewRectSection *section,
    int width,
    int height
) {
    section->width = width;
    section->height = height;
    section->rightExclusive = section->x + width;
    section->bottomExclusive = section->y + height;
    section->maxXInclusive = section->rightExclusive - 1;
    section->maxYInclusive = section->bottomExclusive - 1;
}
/** Reimplements 0x408430: zOpt::ViewRectSection_ClampPointToInclusiveBounds
 * Purpose: clamp a point to inclusive bounds. */
void __fastcall ViewRectSection_ClampPointToInclusiveBounds(
    zOpt_ViewRectSection *section,
    float *pointXY
) {
    if (pointXY[0] < (float)(section->x)) {
        pointXY[0] = (float)(section->x);
    } else if (!(pointXY[0] <= (float)(section->maxXInclusive))) {
        pointXY[0] = (float)(section->maxXInclusive);
    }

    if (pointXY[1] < (float)(section->y)) {
        pointXY[1] = (float)(section->y);
    } else if (!(pointXY[1] <= (float)(section->maxYInclusive))) {
        pointXY[1] = (float)(section->maxYInclusive);
    }
}
/**
 * Reimplements 0x408530: zOpt::RenderSection_SetPosition.
 * Original file: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: set the render-section origin and push the new viewport rectangle
 * to the attached window target.
 *
 * Evidence: BN forwards g_zOpt_RenderSectionOption->value to
 * zOpt_ViewRectSection::SetPosition, then calls gwWindowSetResolution and
 * gwWindowSetSize when the section target is non-null; the shared
 * zopt_video_section_setters VC5SP3 target byte-matches after relocation
 * masking.
 */
void __fastcall RenderSection_SetPosition(
    int x,
    int y
) {
    zOpt_ViewRectSection *section = *g_zOpt_RenderSectionOption;
    ViewRectSection_SetPosition(
        section,
        x,
        y
    );
    if (section->target != 0) {
        zClass_Window::gwWindowSetResolution(
            (zClass_NodePartial *)(section->target),
            section->width,
            section->height
        );
        zClass_Window::gwWindowSetSize(
            (zClass_NodePartial *)(section->target),
            section->x,
            section->y
        );
    }
}

/**
 * Reimplements 0x408500: zOpt::RenderSection_SetSize.
 * Original file: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: set the render-section dimensions and push the new resolution to
 * the attached window target.
 *
 * Evidence: BN forwards g_zOpt_RenderSectionOption->value to
 * zOpt_ViewRectSection::SetSize, then calls gwWindowSetResolution when the
 * section target is non-null; the shared zopt_video_section_setters VC5SP3
 * target byte-matches after relocation masking.
 */
void __fastcall RenderSection_SetSize(
    int width,
    int height
) {
    zOpt_ViewRectSection *section = *g_zOpt_RenderSectionOption;
    ViewRectSection_SetSize(
        section,
        width,
        height
    );
    if (section->target != 0) {
        zClass_Window::gwWindowSetResolution(
            (zClass_NodePartial *)(section->target),
            section->width,
            section->height
        );
    }
}

/** Reimplements 0x408570: zOpt::RenderSection_SetTargetWindow
 * Purpose: attach target window and apply render rectangle. */
void __fastcall RenderSection_SetTargetWindow(
    zClass_NodePartial *windowNode
) {
    zOpt_ViewRectSection *section = *g_zOpt_RenderSectionOption;
    section->target = windowNode;
    if (windowNode != 0) {
        zClass_Window::gwWindowSetResolution(
            windowNode,
            section->width,
            section->height
        );
        zClass_Window::gwWindowSetSize(
            (zClass_NodePartial *)(section->target),
            section->x,
            section->y
        );
    }
}
/** Reimplements 0x4085a0: zOpt::GetRenderSection
 * Purpose: return the active render section pointer. */
zOpt_ViewRectSection *GetRenderSection() {
    return *g_zOpt_RenderSectionOption;
}
/**
 * Reimplements 0x4085e0: zOpt::DisplaySection_SetPosition.
 * Original file: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: set the display-section origin and push the new display rectangle
 * to the attached display target.
 *
 * Evidence: BN forwards g_zOpt_DisplaySectionOption->value to
 * zOpt_ViewRectSection::SetPosition, then calls gwDisplaySetSize and
 * gwDisplaySetPosition when the section target is non-null; the shared
 * zopt_video_section_setters VC5SP3 target byte-matches after relocation
 * masking.
 */
void __fastcall DisplaySection_SetPosition(
    int x,
    int y
) {
    zOpt_ViewRectSection *section = *g_zOpt_DisplaySectionOption;
    ViewRectSection_SetPosition(
        section,
        x,
        y
    );
    if (section->target != 0) {
        zClass_Display::gwDisplaySetSize(
            (zClass_NodePartial *)(section->target),
            section->width,
            section->height
        );
        zClass_Display::gwDisplaySetPosition(
            (zClass_NodePartial *)(section->target),
            section->x,
            section->y
        );
    }
}

/**
 * Reimplements 0x408620: zOpt::DisplaySection_SetSize.
 * Original file: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: set the display-section dimensions and push the new size to the
 * attached display target.
 *
 * Evidence: BN forwards g_zOpt_DisplaySectionOption->value to
 * zOpt_ViewRectSection::SetSize, then calls gwDisplaySetSize when the section
 * target is non-null; the shared zopt_video_section_setters VC5SP3 target
 * byte-matches after relocation masking.
 */
void __fastcall DisplaySection_SetSize(
    int width,
    int height
) {
    zOpt_ViewRectSection *section = *g_zOpt_DisplaySectionOption;
    ViewRectSection_SetSize(
        section,
        width,
        height
    );
    if (section->target != 0) {
        zClass_Display::gwDisplaySetSize(
            (zClass_NodePartial *)(section->target),
            section->width,
            section->height
        );
    }
}

/** Reimplements 0x4085b0: zOpt::DisplaySection_SetTargetDisplay
 * Purpose: attach target display and apply display rectangle. */
void __fastcall DisplaySection_SetTargetDisplay(
    zClass_NodePartial *displayNode
) {
    zOpt_ViewRectSection *section = *g_zOpt_DisplaySectionOption;
    section->target = displayNode;
    if (displayNode != 0) {
        zClass_Display::gwDisplaySetSize(
            displayNode,
            section->width,
            section->height
        );
        zClass_Display::gwDisplaySetPosition(
            (zClass_NodePartial *)(section->target),
            section->x,
            section->y
        );
    }
}
/**
 * Reimplements 0x408680: zOpt::DisplaySection_SetBitsPerPixel.
 * Original file: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: store the active display-section bit depth.
 *
 * Evidence: BN writes ecx to g_zOpt_DisplaySectionOption->value->bitsPerPixel;
 * the shared zopt_video_section_setters VC5SP3 target byte-matches after
 * relocation masking.
 */
void __fastcall DisplaySection_SetBitsPerPixel(
    int bitsPerPixel
) {
    (*g_zOpt_DisplaySectionOption)->bitsPerPixel = bitsPerPixel;
}

/**
 * Reimplements 0x408700: zOpt::WindowSection_SetPosition.
 * Original file: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: set the window-section origin.
 *
 * Evidence: BN forwards g_zOpt_WindowSectionOption->value to
 * zOpt_ViewRectSection::SetPosition; the shared zopt_video_section_setters
 * VC5SP3 target byte-matches after relocation masking.
 */
void __fastcall WindowSection_SetPosition(
    int x,
    int y
) {
    ViewRectSection_SetPosition(
        *g_zOpt_WindowSectionOption,
        x,
        y
    );
}

/**
 * Reimplements 0x4086e0: zOpt::WindowSection_SetSize.
 * Original file: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: set the window-section dimensions.
 *
 * Evidence: BN forwards g_zOpt_WindowSectionOption->value to
 * zOpt_ViewRectSection::SetSize; the shared zopt_video_section_setters VC5SP3
 * target byte-matches after relocation masking.
 */
void __fastcall WindowSection_SetSize(
    int width,
    int height
) {
    ViewRectSection_SetSize(
        *g_zOpt_WindowSectionOption,
        width,
        height
    );
}

/** Reimplements 0x408480: zOpt::CameraSection_SetActiveCamera
 * Purpose: store camera, recompute FOV, and reapply LOD. */
void __fastcall CameraSection_SetActiveCamera(
    zClass_NodePartial *camera
) {
    zOpt_CameraSection *const cameraSection = *g_zOpt_CameraSectionOption;
    cameraSection->m_pCamera = camera;
    if (camera == 0) {
        return;
    }

    zOpt_ViewRectSection *const renderSection = *g_zOpt_RenderSectionOption;
    float fovX = 0.0f;
    float fovY = 0.0f;
    zClass_Camera::gwCameraGetFOV(
        camera,
        &fovX,
        &fovY
    );

    fovX = (float)(renderSection->width) * fovY / (float)(renderSection->height);
    zClass_Camera::gwCameraSetFOV(
        cameraSection->m_pCamera,
        fovX,
        fovY
    );
    zOpt::SetObjectLODForCurrentHwMode(zOpt::GetObjectLODForCurrentHwMode());
}
} // namespace zOpt
/** Reimplements 0x4084e0: zOpt_CameraSection_GetActiveCamera
 * Purpose: return active camera or null when unavailable. */
zClass_NodePartial *zOpt_CameraSection_GetActiveCamera() {
    if (g_zOpt_CameraSectionOption == 0 || *g_zOpt_CameraSectionOption == 0) {
        return 0;
    }

    return (*g_zOpt_CameraSectionOption)->m_pCamera;
}
/**
 * Reimplements 0x408190: zOpt::GetPlayerName.
 * Original source path: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the configured player-name option buffer.
 */
char *zOpt_GetPlayerName() {
    return (char *)(ZOPT_PLAYER_NAME->payloadOrBuffer);
}

/**
 * Reimplements 0x408a20: zOpt_GetWolPasswordFlagValue.
 * Purpose: return the WOL password flag option value through its option pointer.
 */
int zOpt_GetWolPasswordFlagValue() {
    return *g_zOpt_WolPasswordFlagOption;
}

/**
 * Reimplements 0x408660: zOpt_DisplaySection_GetWidth.
 * Purpose: return the active display section width.
 */
int zOpt_DisplaySection_GetWidth() {
    return (*g_zOpt_DisplaySectionOption)->width;
}

/**
 * Reimplements 0x408670: zOpt_DisplaySection_GetHeight.
 * Purpose: return the active display section height.
 */
int zOpt_DisplaySection_GetHeight() {
    return (*g_zOpt_DisplaySectionOption)->height;
}
