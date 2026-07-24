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

typedef int zOptGameControlFlags;

struct zGame_OptionsPointerCache {
    int *effectsLevelSw;
    int *effectsLevelHw;
    int *gfxFlagsSw;
    int *gfxFlagsHw;
    int *objectLodSw;
    int *objectLodHw;
    int *textureMemorySw;
    int *textureMemoryHw;
    int *hudVisibilitySw;
    int *hudVisibilityHw;
    int *hudTypeSw;
    int *hudTypeHw;
    int *videoMode;
    int *audioApi;
    int *soundLod;
    zOptGameControlFlags *gameControlOptions;
    int *muteSound;
    float *soundVolume;
    int *gameDifficulty;
    zOptionEntryPartial *playerName;
    int *cdAudio;
    int *videoFullscreen;
    int *videoAcceleration;
    int *hardwareApi;
    int *inputJoystick;
    int *joystickNumAxes;
    int *joystickNumButtons;
    int *replicate;
    int *videoStride;
    int *networkEnabled;
    int *networkListen;
    zOpt_CameraSection **cameraSection;
    zOpt_ViewRectSection **renderSection;
    zOpt_ViewRectSection **displaySection;
    zOpt_ViewRectSection **windowSection;
    void *unusedOption;
    int *networkModem;
    int *wolPasswordFlag;
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

    zGame_OptionsRuntimeConfig * CopyDefault();
    RECOIL_NO_GS int InitFromSystem();
    RECOIL_NO_GS void LoadCpuVendorString();
};

extern zOptionEntryPartial *g_zGame_Options_OptionListHead;
extern char *g_zGame_Options_RegKeyRoot;
extern char *g_zGame_Options_RegKeyCurrentUser;
extern char *g_zGame_Options_RegKeyGame;
extern int g_zGame_Options_RegContextInitialized;
extern zGame_OptionsRuntimeConfig g_zGame_Options_RuntimeConfigDefaults;
extern zGame_OptionsRuntimeConfig g_zGame_Options_RuntimeConfig;
extern zGame_OptionsPointerCache g_zGame_Options_PointerCache;
extern int g_zOpt_HwMode;
}

enum zOptHudTypeOption {
    ZOPT_HUD_TYPE_STANDARD = 1,
    ZOPT_HUD_TYPE_PERSPECTIVE = 2,
};

namespace zGame {
void ReturnOnlyStub();
zOptionEntryPartial *__fastcall Options_FindOption(const char *name) throw();
zOptionEntryPartial *__fastcall Options_GetOrCreateOption(
    const char *name,
    int storageType,
    int dataSize,
    int registryScope
);
void __fastcall Options_InitRegistryContext(
    const char *regKeyRoot,
    const char *regKeyCurrentUser,
    const char *regKeyGame
);
RECOIL_NO_GS int Options_LoadGameOptions();
RECOIL_NO_GS int Options_LoadFromRegistry();
RECOIL_NO_GS int Options_SaveToRegistry();
int Options_SaveGameOptions();
void Options_ShutdownRegistryContext();
} // namespace zGame

namespace zOpt {
int __fastcall LookupNamedValueAsInt(const char *key);
int __fastcall ReadScalarValueAsInt(zReader::Node *scalarValueNode);
int __fastcall EvaluateProfileMetricCondition(
    zReader::Node *metricConditionNode
);
int __fastcall SelectProfileValueForSystem(
    zReader::Node *parentNode,
    const char *profileName,
    int defaultValue
);
int __fastcall EvalIntCompareOp(
    const char *opString,
    int lhs,
    int rhs
);
zOpt_ViewRectSection *GetDisplaySection();
int GetDisplaySectionBitsPerPixel();
int GetVideoStrideValue();
zOpt_ViewRectSection *GetWindowSection();
int GetWindowSectionHeight();
void __fastcall SetFullscreenOption(int fullscreenOption);
int GetFullscreenOption();
void __fastcall SetHudVisibilityOption(int hudVisibility);
int GetHudVisibilityOption();
int GetHudTypeForCurrentHwMode();
int ToggleHudTypeForCurrentHwMode();
void __fastcall SetReplicateMode(int replicateMode);
int GetReplicateMode();
int GetNetworkEnabled();
void __fastcall ViewRectSection_SetPosition(
    zOpt_ViewRectSection *section,
    int x,
    int y
);
void __fastcall ViewRectSection_SetSize(
    zOpt_ViewRectSection *section,
    int width,
    int height
);
void __fastcall ViewRectSection_ClampPointToInclusiveBounds(
    zOpt_ViewRectSection *section,
    float *pointXY
);
void __fastcall RenderSection_SetPosition(
    int x,
    int y
);
void __fastcall RenderSection_SetSize(
    int width,
    int height
);
void __fastcall RenderSection_SetTargetWindow(zClass_NodePartial *windowNode);
zOpt_ViewRectSection *GetRenderSection();
void __fastcall DisplaySection_SetPosition(
    int x,
    int y
);
void __fastcall DisplaySection_SetSize(
    int width,
    int height
);
void __fastcall DisplaySection_SetTargetDisplay(
    zClass_NodePartial *displayNode
);
void __fastcall DisplaySection_SetBitsPerPixel(int bitsPerPixel);
void __fastcall WindowSection_SetPosition(
    int x,
    int y
);
void __fastcall WindowSection_SetSize(
    int width,
    int height
);
void __fastcall CameraSection_SetActiveCamera(zClass_NodePartial *camera);
void __fastcall SetGameControlOptions(zOptGameControlFlags value);
void __fastcall SetThrottleMode(int enable);
int GetThrottleMode();
void __fastcall SetSteeringMode(int enable);
int GetSteeringMode();
void __fastcall SetCursorMode(int enable);
int GetCursorMode();
void __fastcall SetCameraMode(int enableThirdPerson);
int GetCameraModePlayerState();
void __fastcall SetGameDifficultyMode(int value);
int GetGameDifficultyMode();
void __fastcall SetEffectsLevelForCurrentHwMode(int level);
int GetEffectsLevelForCurrentHwMode();
void __fastcall SetObjectLODForCurrentHwMode(int level);
int GetObjectLODForCurrentHwMode();
int GetMuteSoundOption();
void __fastcall SetMuteSoundOption(int value);
void __fastcall SetSoundVolumeOption(float volume);
float GetSoundVolumeOption();
void __fastcall SetSoundLODOption(int value);
int GetSoundLODOption();
void __fastcall SetTextureMemoryForCurrentHwMode(int value);
int GetTextureMemoryForCurrentHwMode();
void __fastcall SetPlayerName(const char *name);
int GetGraphicsFlagsForCurrentHwMode();
void __fastcall SetGraphicsFlagsForCurrentHwMode(int flags);
int __fastcall SetHudTypeForCurrentHwMode(int hudType);
void __fastcall SetNetworkEnabled(int value);
void __fastcall SetNetworkModemEnabled(int value);
void __fastcall SetNetworkListenEnabled(int value);
int GetNetworkModemEnabled();
void __fastcall SetWolPasswordFlag(int value);
} // namespace zOpt

zClass_NodePartial *zOpt_CameraSection_GetActiveCamera();
char *zOpt_GetPlayerName();
int zOpt_GetWolPasswordFlagValue();
int zOpt_DisplaySection_GetWidth();
int zOpt_DisplaySection_GetHeight();
