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

struct CZNodePartial;
namespace zReader {
struct Node;
}

struct zOpt_CameraSection {
    int unknown_00;
    int unknown_04;
    CZNodePartial *m_pCamera;
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
    unsigned int videoMemoryKb; // Inferred from the VIDEO_KB profile metric.
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
void __cdecl ReturnOnlyStub();
zOptionEntryPartial *__fastcall OptionsFindOption(const char *name) throw();
zOptionEntryPartial *__fastcall OptionsGetOrCreateOption(
    const char *name,
    int storageType,
    int dataSize,
    int registryScope
);
void __fastcall OptionsInitRegistryContext(
    const char *regKeyRoot,
    const char *regKeyCurrentUser,
    const char *regKeyGame
);
RECOIL_NO_GS int __fastcall OptionsLoadGameOptions();
RECOIL_NO_GS int __cdecl OptionsLoadFromRegistry();
RECOIL_NO_GS int __cdecl OptionsSaveToRegistry();
int OptionsSaveGameOptions();
void __cdecl OptionsShutdownRegistryContext();
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
void __fastcall ViewRectSectionSetPosition(
    zOpt_ViewRectSection *section,
    int x,
    int y
);
void __fastcall ViewRectSectionSetSize(
    zOpt_ViewRectSection *section,
    int width,
    int height
);
void __fastcall ViewRectSectionClampPointToInclusiveBounds(
    zOpt_ViewRectSection *section,
    float *pointXY
);
void __fastcall RenderSectionSetPosition(
    int x,
    int y
);
void __fastcall RenderSectionSetSize(
    int width,
    int height
);
void __fastcall RenderSectionSetTargetWindow(CZNodePartial *windowNode);
zOpt_ViewRectSection *GetRenderSection();
void __fastcall DisplaySectionSetPosition(
    int x,
    int y
);
void __fastcall DisplaySectionSetSize(
    int width,
    int height
);
void __fastcall DisplaySectionSetTargetDisplay(
    CZNodePartial *displayNode
);
void __fastcall DisplaySectionSetBitsPerPixel(int bitsPerPixel);
void __fastcall WindowSectionSetPosition(
    int x,
    int y
);
void __fastcall WindowSectionSetSize(
    int width,
    int height
);
void __fastcall CameraSectionSetActiveCamera(CZNodePartial *camera);
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

CZNodePartial *zOptCameraSectionGetActiveCamera();
char *zOptGetPlayerName();
int zOptGetWolPasswordFlagValue();
int zOptDisplaySectionGetWidth();
int zOptDisplaySectionGetHeight();
