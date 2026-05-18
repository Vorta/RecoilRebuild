#pragma once

#include "recoil/recoil_types.h"

#include "recoil/recoil_callconv.h"

extern "C" {
struct zOptionEntryPartial {
    int payloadOrBuffer;
    unsigned int flagsOrDefault;
    int storageType;
    int dataSize;
    char *name;
    int registryScope;
    zOptionEntryPartial *next;
    int unknown_1c;
};

struct zOpt_ViewRectSection {
    int x;
    int y;
    int rightExclusive;
    int bottomExclusive;
    int width;
    int height;
    int maxXInclusive;
    int maxYInclusive;
    int bitsPerPixel;
    void *target;
};

struct zClass_NodePartial;
namespace zReader {
struct Node;
}

struct zOpt_CameraSection {
    int unknown_00;
    int unknown_04;
    zClass_NodePartial *m_pCamera;
};

struct zGame_OptionsRuntimeConfig {
    char cpuVendor[0x10];
    int cpuClass;
    int cpuMhz;
    unsigned int defaultFlags;
    unsigned int systemRamKb;
    unsigned int unknown_20;
    unsigned int soundHardwareMemKb;
    unsigned int reservedCapabilityValue;
    unsigned int unknown_2c;

    RECOIL_NOINLINE zGame_OptionsRuntimeConfig *RECOIL_THISCALL CopyDefault();
    RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_THISCALL InitFromSystem();
    RECOIL_NOINLINE RECOIL_NO_GS void RECOIL_THISCALL LoadCpuVendorString();
};

extern zOptionEntryPartial *g_zGame_Options_OptionListHead;
extern char *g_zGame_Options_RegKeyRoot;
extern char *g_zGame_Options_RegKeyCurrentUser;
extern char *g_zGame_Options_RegKeyGame;
extern int g_zGame_Options_RegContextInitialized;
extern zGame_OptionsRuntimeConfig g_zGame_Options_RuntimeConfigDefaults;
extern zGame_OptionsRuntimeConfig g_zGame_Options_RuntimeConfig;
extern int *ZOPT_VIDEO_FULLSCREEN;
extern int *ZOPT_VIDEO_STRIDE;
extern int *ZOPT_HUD_SW;
extern int *ZOPT_HUD_HW;
extern int *ZOPT_HUD_TYPE_SW;
extern int *ZOPT_HUD_TYPE_HW;
extern int *ZOPT_REPLICATE;
extern int *ZOPT_NETWORK_ENABLED;
extern int *g_zOpt_NetworkModemOption;
extern int *g_zOpt_NetworkListenOption;
extern int *g_zOpt_GameDifficultyOption;
extern int *g_zOpt_WolPasswordFlagOption;
extern int *ZOPT_EFFECTS_LEVEL_SW;
extern int *ZOPT_EFFECTS_LEVEL_HW;
extern int *ZOPT_OBJECT_LOD_SW;
extern int *ZOPT_OBJECT_LOD_HW;
extern int *ZOPT_MUTE_SOUND;
extern float *ZOPT_SOUND_VOLUME;
extern int *ZOPT_SOUND_LOD;
extern int *ZOPT_TEXTURE_MEMORY_SW;
extern int *ZOPT_TEXTURE_MEMORY_HW;
extern zOptionEntryPartial *ZOPT_PLAYER_NAME;
extern int *ZOPT_GFX_FLAGS_SW;
extern int *ZOPT_GFX_FLAGS_HW;
extern zOpt_ViewRectSection **g_zOpt_RenderSectionOption;
extern zOpt_ViewRectSection **g_zOpt_DisplaySectionOption;
extern zOpt_ViewRectSection **g_zOpt_WindowSectionOption;
extern zOpt_CameraSection **g_zOpt_CameraSectionOption;
extern int g_zOpt_HwMode;
extern int *ZOPT_GAME_CONTROL_OPTIONS;
}

namespace zGame {
void RECOIL_CDECL ReturnOnlyStub();
zOptionEntryPartial *RECOIL_FASTCALL Options_FindOption(const char *name);
zOptionEntryPartial *RECOIL_FASTCALL Options_GetOrCreateOption(const char *name,
                                                               int storageType,
                                                               int dataSize,
                                                               int registryScope);
RECOIL_NOINLINE void RECOIL_FASTCALL Options_InitRegistryContext(const char *regKeyRoot,
                                                                 const char *regKeyCurrentUser,
                                                                 const char *regKeyGame);
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_CDECL Options_LoadGameOptions();
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_CDECL Options_LoadFromRegistry();
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_CDECL Options_SaveToRegistry();
RECOIL_NOINLINE int RECOIL_CDECL Options_SaveGameOptions();
RECOIL_NOINLINE void RECOIL_CDECL Options_ShutdownRegistryContext();
} // namespace zGame

namespace zOpt {
RECOIL_NOINLINE int RECOIL_FASTCALL LookupNamedValueAsInt(const char *key);
RECOIL_NOINLINE int RECOIL_FASTCALL ReadScalarValueAsInt(zReader::Node *scalarValueNode);
RECOIL_NOINLINE int RECOIL_FASTCALL
EvaluateProfileMetricCondition(zReader::Node *metricConditionNode);
RECOIL_NOINLINE int RECOIL_FASTCALL SelectProfileValueForSystem(zReader::Node *parentNode,
                                                                         const char *profileName,
                                                                         int defaultValue);
RECOIL_NOINLINE int RECOIL_FASTCALL EvalIntCompareOp(const char *opString,
                                                              int lhs, int rhs);
RECOIL_NOINLINE zOpt_ViewRectSection *RECOIL_CDECL GetDisplaySection();
RECOIL_NOINLINE int RECOIL_CDECL GetDisplaySectionBitsPerPixel();
RECOIL_NOINLINE int RECOIL_CDECL GetVideoStrideValue();
RECOIL_NOINLINE zOpt_ViewRectSection *RECOIL_CDECL GetWindowSection();
RECOIL_NOINLINE int RECOIL_CDECL GetWindowSectionHeight();
RECOIL_NOINLINE void RECOIL_FASTCALL SetFullscreenOption(int fullscreenOption);
RECOIL_NOINLINE int RECOIL_CDECL GetFullscreenOption();
RECOIL_NOINLINE void RECOIL_FASTCALL SetHudVisibilityOption(int hudVisibility);
RECOIL_NOINLINE int RECOIL_CDECL GetHudVisibilityOption();
RECOIL_NOINLINE int RECOIL_CDECL GetHudTypeForCurrentHwMode();
RECOIL_NOINLINE void RECOIL_FASTCALL SetReplicateMode(int replicateMode);
RECOIL_NOINLINE int RECOIL_CDECL GetReplicateMode();
RECOIL_NOINLINE int RECOIL_CDECL GetNetworkEnabled();
RECOIL_NOINLINE void RECOIL_FASTCALL ViewRectSection_SetPosition(zOpt_ViewRectSection *section,
                                                                 int x, int y);
RECOIL_NOINLINE void RECOIL_FASTCALL ViewRectSection_SetSize(zOpt_ViewRectSection *section,
                                                             int width,
                                                             int height);
RECOIL_NOINLINE void RECOIL_FASTCALL
ViewRectSection_ClampPointToInclusiveBounds(zOpt_ViewRectSection *section, float *pointXY);
RECOIL_NOINLINE void RECOIL_FASTCALL RenderSection_SetPosition(int x, int y);
RECOIL_NOINLINE void RECOIL_FASTCALL RenderSection_SetSize(int width, int height);
RECOIL_NOINLINE void RECOIL_FASTCALL RenderSection_SetTargetWindow(zClass_NodePartial *windowNode);
RECOIL_NOINLINE zOpt_ViewRectSection *RECOIL_CDECL GetRenderSection();
RECOIL_NOINLINE void RECOIL_FASTCALL DisplaySection_SetPosition(int x, int y);
RECOIL_NOINLINE void RECOIL_FASTCALL DisplaySection_SetSize(int width,
                                                            int height);
RECOIL_NOINLINE void RECOIL_FASTCALL
DisplaySection_SetTargetDisplay(zClass_NodePartial *displayNode);
RECOIL_NOINLINE void RECOIL_FASTCALL DisplaySection_SetBitsPerPixel(int bitsPerPixel);
RECOIL_NOINLINE void RECOIL_FASTCALL WindowSection_SetPosition(int x, int y);
RECOIL_NOINLINE void RECOIL_FASTCALL WindowSection_SetSize(int width, int height);
RECOIL_NOINLINE void RECOIL_FASTCALL CameraSection_SetActiveCamera(zClass_NodePartial *camera);
RECOIL_NOINLINE void RECOIL_FASTCALL SetGameControlOptions(int value);
RECOIL_NOINLINE void RECOIL_FASTCALL SetThrottleMode(int enable);
RECOIL_NOINLINE int RECOIL_CDECL GetThrottleMode();
RECOIL_NOINLINE void RECOIL_FASTCALL SetSteeringMode(int enable);
RECOIL_NOINLINE int RECOIL_CDECL GetSteeringMode();
RECOIL_NOINLINE void RECOIL_FASTCALL SetCursorMode(int enable);
RECOIL_NOINLINE int RECOIL_CDECL GetCursorMode();
RECOIL_NOINLINE int RECOIL_CDECL GetCameraModePlayerState();
RECOIL_NOINLINE void RECOIL_FASTCALL SetGameDifficultyMode(int value);
RECOIL_NOINLINE int RECOIL_CDECL GetGameDifficultyMode();
RECOIL_NOINLINE void RECOIL_FASTCALL SetEffectsLevelForCurrentHwMode(int level);
RECOIL_NOINLINE int RECOIL_CDECL GetEffectsLevelForCurrentHwMode();
RECOIL_NOINLINE void RECOIL_FASTCALL SetObjectLODForCurrentHwMode(int level);
RECOIL_NOINLINE int RECOIL_CDECL GetObjectLODForCurrentHwMode();
RECOIL_NOINLINE int RECOIL_CDECL GetMuteSoundOption();
RECOIL_NOINLINE void RECOIL_FASTCALL SetMuteSoundOption(int value);
RECOIL_NOINLINE void RECOIL_FASTCALL SetSoundVolumeOption(float volume);
RECOIL_NOINLINE float RECOIL_CDECL GetSoundVolumeOption();
RECOIL_NOINLINE void RECOIL_FASTCALL SetSoundLODOption(int value);
RECOIL_NOINLINE int RECOIL_CDECL GetSoundLODOption();
RECOIL_NOINLINE void RECOIL_FASTCALL SetTextureMemoryForCurrentHwMode(int value);
RECOIL_NOINLINE int RECOIL_CDECL GetTextureMemoryForCurrentHwMode();
RECOIL_NOINLINE void RECOIL_FASTCALL SetPlayerName(const char *name);
RECOIL_NOINLINE int RECOIL_CDECL GetGraphicsFlagsForCurrentHwMode();
RECOIL_NOINLINE void RECOIL_FASTCALL SetGraphicsFlagsForCurrentHwMode(int flags);
RECOIL_NOINLINE int RECOIL_FASTCALL SetHudTypeForCurrentHwMode(int hudType);
RECOIL_NOINLINE void RECOIL_FASTCALL SetNetworkEnabled(int value);
RECOIL_NOINLINE void RECOIL_FASTCALL SetNetworkModemEnabled(int value);
RECOIL_NOINLINE void RECOIL_FASTCALL SetNetworkListenEnabled(int value);
RECOIL_NOINLINE int RECOIL_CDECL GetNetworkModemEnabled();
RECOIL_NOINLINE void RECOIL_FASTCALL SetWolPasswordFlag(int value);
} // namespace zOpt

RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL zOpt_CameraSection_GetActiveCamera();
RECOIL_NOINLINE char *RECOIL_CDECL zOpt_GetPlayerName();
RECOIL_NOINLINE int RECOIL_CDECL zOpt_GetWolPasswordFlagValue();
RECOIL_NOINLINE int RECOIL_CDECL zOpt_DisplaySection_GetWidth();
RECOIL_NOINLINE int RECOIL_CDECL zOpt_DisplaySection_GetHeight();
