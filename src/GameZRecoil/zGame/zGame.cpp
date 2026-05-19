#include "zGame.h"

#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zSys/zSys.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "zClass.h"

#include <windows.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <intrin.h>
#include <malloc.h>

extern "C" {
zOptionEntryPartial *g_zGame_Options_OptionListHead = 0;
char *g_zGame_Options_RegKeyRoot = 0;
char *g_zGame_Options_RegKeyCurrentUser = 0;
char *g_zGame_Options_RegKeyGame = 0;
int g_zGame_Options_RegContextInitialized = 0;
zGame_OptionsRuntimeConfig g_zGame_Options_RuntimeConfigDefaults = {0};
int *ZOPT_VIDEO_FULLSCREEN = 0;
int *ZOPT_VIDEO_STRIDE = 0;
int *ZOPT_HUD_SW = 0;
int *ZOPT_HUD_HW = 0;
int *ZOPT_HUD_TYPE_SW = 0;
int *ZOPT_HUD_TYPE_HW = 0;
int *ZOPT_REPLICATE = 0;
int *ZOPT_NETWORK_ENABLED = 0;
int *g_zOpt_NetworkModemOption = 0;
int *g_zOpt_NetworkListenOption = 0;
int *g_zOpt_GameDifficultyOption = 0;
int *g_zOpt_WolPasswordFlagOption = 0;
int *ZOPT_EFFECTS_LEVEL_SW = 0;
int *ZOPT_EFFECTS_LEVEL_HW = 0;
int *ZOPT_OBJECT_LOD_SW = 0;
int *ZOPT_OBJECT_LOD_HW = 0;
int *ZOPT_MUTE_SOUND = 0;
float *ZOPT_SOUND_VOLUME = 0;
int *ZOPT_SOUND_LOD = 0;
int *ZOPT_TEXTURE_MEMORY_SW = 0;
int *ZOPT_TEXTURE_MEMORY_HW = 0;
zOptionEntryPartial *ZOPT_PLAYER_NAME = 0;
int *ZOPT_GFX_FLAGS_SW = 0;
int *ZOPT_GFX_FLAGS_HW = 0;
zOpt_ViewRectSection **g_zOpt_RenderSectionOption = 0;
zOpt_ViewRectSection **g_zOpt_DisplaySectionOption = 0;
zOpt_ViewRectSection **g_zOpt_WindowSectionOption = 0;
zOpt_CameraSection **g_zOpt_CameraSectionOption = 0;
int g_zOpt_HwMode = 0;
int *ZOPT_GAME_CONTROL_OPTIONS = 0;
zGame_OptionsRuntimeConfig g_zGame_Options_RuntimeConfig = {0};
}

RECOIL_STATIC_ASSERT(offsetof(zOptionEntryPartial, payloadOrBuffer) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zOptionEntryPartial, storageType) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zOptionEntryPartial, dataSize) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zOptionEntryPartial, name) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zOptionEntryPartial, registryScope) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zOptionEntryPartial, next) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zOptionEntryPartial) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zOpt_ViewRectSection, x) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zOpt_ViewRectSection, y) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zOpt_ViewRectSection, width) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zOpt_ViewRectSection, height) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zOpt_ViewRectSection, maxXInclusive) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zOpt_ViewRectSection, bitsPerPixel) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zOpt_ViewRectSection, target) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(zOpt_ViewRectSection) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zOpt_CameraSection, m_pCamera) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsRuntimeConfig, cpuClass) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsRuntimeConfig, defaultFlags) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsRuntimeConfig, soundHardwareMemKb) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(zGame_OptionsRuntimeConfig) == 0x30);

// Reimplements 0x4b3090: zGame_OptionsRuntimeConfig::CopyDefault
RECOIL_NOINLINE zGame_OptionsRuntimeConfig *RECOIL_THISCALL
zGame_OptionsRuntimeConfig::CopyDefault() {
    if (this == 0) {
        return &g_zGame_Options_RuntimeConfigDefaults;
    }

    memcpy(this, &g_zGame_Options_RuntimeConfigDefaults, sizeof(*this));
    return this;
}

// Reimplements 0x4b3160: zGame_OptionsRuntimeConfig::LoadCpuVendorString
RECOIL_NOINLINE RECOIL_NO_GS void RECOIL_THISCALL
zGame_OptionsRuntimeConfig::LoadCpuVendorString() {
    if (zSys::HasCpuidSupport() == 0) {
        return;
    }

    int cpuInfo[4];
    __cpuid(cpuInfo, 0);
    char vendor[0x0c];
    memcpy(&vendor[0], &cpuInfo[1], 4);
    memcpy(&vendor[4], &cpuInfo[3], 4);
    memcpy(&vendor[8], &cpuInfo[2], 4);
    strncpy(cpuVendor, vendor, 0x0c);
    cpuVendor[0x0c] = '\0';
}

// Reimplements 0x4b30b0: zGame_OptionsRuntimeConfig::InitFromSystem
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_THISCALL
zGame_OptionsRuntimeConfig::InitFromSystem() {
    LoadCpuVendorString();
    cpuClass = zSys::GetCpuClass();
    cpuMhz = zSys::GetCpuMhz();

    unsigned int probe = static_cast<unsigned int>(zSnd::HasMmxMixerSupport()) & 1u;
    unsigned int flags = defaultFlags;
    defaultFlags = ((flags ^ probe) & 1u) ^ flags;

    probe = (static_cast<unsigned int>(zSys::ReturnZeroStub()) & 1u) << 1;
    flags = defaultFlags;
    defaultFlags = (flags & ~2u) | probe;

    systemRamKb = zSys::GetTotalPhysKb();

    probe = (static_cast<unsigned int>(zSys::ReturnZeroStub()) & 1u) << 2;
    flags = defaultFlags;
    defaultFlags = (flags & ~4u) | probe;

    probe = (static_cast<unsigned int>(zVid::HasAcceptedHardwareRenderer()) & 1u) << 6;
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
const int ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON = 0x08;
const int ZOPT_GRAPHICS_MMX = 1;
const int ZOPT_GRAPHICS_TRANSPARENCY = 2;
const int ZOPT_GRAPHICS_LIGHTING = 4;
const int ZOPT_GRAPHICS_PERSPECTIVE = 8;
const int ZOPT_GRAPHICS_GLOBAL_LIGHT = 0x10;
const int ZOPT_GRAPHICS_ALL_VIDEO_BUFFER = 0x20;

template <typename T> T *OptionValuePointer(zOptionEntryPartial *entry) {
    return (T *)(entry);
}

int BuildGraphicsFlags(zReader::Node *profileRoot, const char *globalLightKey,
                                int globalLightDefault) {
    int flags = 0;
    if ((g_zGame_Options_RuntimeConfig.defaultFlags & 1u) != 0) {
        flags |= ZOPT_GRAPHICS_MMX;
    }
    if (zOpt::SelectProfileValueForSystem(profileRoot, "Transparency", 1) != 0) {
        flags |= ZOPT_GRAPHICS_TRANSPARENCY;
    }
    if (zOpt::SelectProfileValueForSystem(profileRoot, "Lighting", 1) != 0) {
        flags |= ZOPT_GRAPHICS_LIGHTING;
    }
    if (zOpt::SelectProfileValueForSystem(profileRoot, "Perspective", 1) != 0) {
        flags |= ZOPT_GRAPHICS_PERSPECTIVE;
    }
    if (zOpt::SelectProfileValueForSystem(profileRoot, globalLightKey, globalLightDefault) != 0) {
        flags |= ZOPT_GRAPHICS_GLOBAL_LIGHT;
    }
    if (zOpt::SelectProfileValueForSystem(profileRoot, "AllVideoBuffer", 0) != 0) {
        flags |= ZOPT_GRAPHICS_ALL_VIDEO_BUFFER;
    }

    return flags;
}

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

// Reimplements 0x4076f0: zGame::ReturnOnlyStub
RECOIL_NOINLINE void RECOIL_CDECL ReturnOnlyStub() {}

// Reimplements 0x4b3380: zGame::Options_FindOption
RECOIL_NOINLINE zOptionEntryPartial *RECOIL_FASTCALL Options_FindOption(const char *name) {
    for (zOptionEntryPartial *entry = g_zGame_Options_OptionListHead; entry != 0;
         entry = entry->next) {
        if (strcmp(name, entry->name) == 0) {
            return entry;
        }
    }

    return 0;
}

// Reimplements 0x4b2e80: zGame::Options_GetOrCreateOption
RECOIL_NOINLINE zOptionEntryPartial *RECOIL_FASTCALL Options_GetOrCreateOption(
    const char *name, int storageType, int dataSize, int registryScope) {
    zOptionEntryPartial *result = Options_FindOption(name);
    if (result != 0) {
        return result;
    }

    result = static_cast<zOptionEntryPartial *>(calloc(1, sizeof(zOptionEntryPartial)));
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
        result->payloadOrBuffer = (int)(calloc(1, dataSize));
        break;

    default:
        break;
    }

    result->next = g_zGame_Options_OptionListHead;
    g_zGame_Options_OptionListHead = result;
    return result;
}

// Reimplements 0x4b3260: zGame::Options_InitRegistryContext
RECOIL_NOINLINE void RECOIL_FASTCALL Options_InitRegistryContext(const char *regKeyRoot,
                                                                 const char *regKeyCurrentUser,
                                                                 const char *regKeyGame) {
    g_zGame_Options_RegKeyRoot = _strdup(regKeyRoot);
    g_zGame_Options_RegKeyCurrentUser = _strdup(regKeyCurrentUser);
    g_zGame_Options_RegKeyGame = _strdup(regKeyGame);
    g_zGame_Options_OptionListHead = 0;
    g_zGame_Options_RegContextInitialized = 1;
    g_zGame_Options_RuntimeConfigDefaults.InitFromSystem();
}

// Reimplements 0x4b2960: zGame::Options_LoadFromRegistry
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_CDECL Options_LoadFromRegistry() {
    const size_t subKeyLength = strlen("SOFTWARE\\") +
                                     strlen(g_zGame_Options_RegKeyRoot) + 1 +
                                     strlen(g_zGame_Options_RegKeyCurrentUser) + 1 +
                                     strlen(g_zGame_Options_RegKeyGame) + 1;
    char *const subKey =
        static_cast<char *>(_alloca((subKeyLength + 3u) & ~static_cast<size_t>(3u)));
    strcpy(subKey, "SOFTWARE\\");
    strcat(subKey, g_zGame_Options_RegKeyRoot);
    strcat(subKey, "\\");
    strcat(subKey, g_zGame_Options_RegKeyCurrentUser);
    strcat(subKey, "\\");
    strcat(subKey, g_zGame_Options_RegKeyGame);

    HKEY currentUserKey = 0;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, subKey, 0, KEY_READ, &currentUserKey) != ERROR_SUCCESS) {
        return 0;
    }

    HKEY localMachineKey = 0;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &localMachineKey) != ERROR_SUCCESS) {
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
            expectedSize = static_cast<DWORD>(entry->dataSize);
            payload = (BYTE *)(static_cast<unsigned int>(entry->payloadOrBuffer));
            break;

        default:
            continue;
        }

        DWORD valueType = 0;
        DWORD actualSize = 0;
        if (RegQueryValueExA(*key, entry->name, 0, &valueType, 0, &actualSize) ==
                ERROR_SUCCESS &&
            actualSize == expectedSize) {
            RegQueryValueExA(*key, entry->name, 0, &valueType, payload, &expectedSize);
        }
    }

    RegCloseKey(currentUserKey);
    RegCloseKey(localMachineKey);
    return 1;
}

// Reimplements 0x4b2bf0: zGame::Options_SaveToRegistry
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_CDECL Options_SaveToRegistry() {
    const size_t subKeyLength = strlen("SOFTWARE\\") +
                                     strlen(g_zGame_Options_RegKeyRoot) + 1 +
                                     strlen(g_zGame_Options_RegKeyCurrentUser) + 1 +
                                     strlen(g_zGame_Options_RegKeyGame) + 1;
    char *const subKey =
        static_cast<char *>(_alloca((subKeyLength + 3u) & ~static_cast<size_t>(3u)));
    strcpy(subKey, "SOFTWARE\\");
    strcat(subKey, g_zGame_Options_RegKeyRoot);
    strcat(subKey, "\\");
    strcat(subKey, g_zGame_Options_RegKeyCurrentUser);
    strcat(subKey, "\\");
    strcat(subKey, g_zGame_Options_RegKeyGame);

    DWORD disposition = 0;
    HKEY currentUserKey = 0;
    if (RegCreateKeyExA(HKEY_CURRENT_USER, subKey, 0, 0, REG_OPTION_RESERVED, KEY_WRITE,
                        0, &currentUserKey, &disposition) != ERROR_SUCCESS) {
        RegCloseKey(currentUserKey);
        return 0;
    }

    HKEY localMachineKey = 0;
    if (RegCreateKeyExA(HKEY_LOCAL_MACHINE, subKey, 0, 0, REG_OPTION_RESERVED, KEY_WRITE,
                        0, &localMachineKey, &disposition) != ERROR_SUCCESS) {
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

        if (key == 0 ||
            static_cast<unsigned int>(entry->storageType) > ZGAME_OPTION_STORAGE_MAX) {
            continue;
        }

        DWORD valueType = REG_BINARY;
        const BYTE *payload = (const BYTE *)(entry);
        if (entry->storageType == ZGAME_OPTION_INLINE_DWORD) {
            valueType = REG_DWORD;
        } else if (entry->storageType >= ZGAME_OPTION_STRING_BUFFER) {
            payload =
                (const BYTE *)(static_cast<unsigned int>(entry->payloadOrBuffer));
        }

        if (RegSetValueExA(*key, entry->name, 0, valueType, payload,
                           static_cast<DWORD>(entry->dataSize)) != ERROR_SUCCESS) {
            return 0;
        }
    }

    RegCloseKey(currentUserKey);
    RegCloseKey(localMachineKey);
    return 1;
}

// Reimplements 0x407e00: zGame::Options_SaveGameOptions
RECOIL_NOINLINE int RECOIL_CDECL Options_SaveGameOptions() {
    zInput::BindGroupList_Clear();
    zOpt::SetNetworkEnabled(0);
    zOpt::SetNetworkModemEnabled(0);
    return Options_SaveToRegistry();
}

// Reimplements 0x4b32c0: zGame::Options_ShutdownRegistryContext
RECOIL_NOINLINE void RECOIL_CDECL Options_ShutdownRegistryContext() {
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
            void *const payload =
                (void *)(static_cast<unsigned int>(entry->payloadOrBuffer));
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

// Reimplements 0x407700: zGame::Options_LoadGameOptions
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_CDECL Options_LoadGameOptions() {
    ResetOptionPointers();

    zReader::Node *const detailRoot = zReader::LoadNodeFromPath("detail.zrd", 0, 0);
    if (detailRoot == 0) {
        return 0;
    }

    g_zGame_Options_RuntimeConfig.CopyDefault();

    ZOPT_VIDEO_ACCELERATION = OptionValuePointer<int>(Options_GetOrCreateOption(
        "HWCardFlag", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_VIDEO_ACCELERATION != 0) {
        zVid::SetAccelerationOption(ZVID_HW_MODE_HARDWARE);
    }

    ZOPT_EFFECTS_LEVEL_SW = OptionValuePointer<int>(Options_GetOrCreateOption(
        "EffectsLevel_SW", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_EFFECTS_LEVEL_SW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetEffectsLevelForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, "EffectsLevel_SW", 1));
    }

    ZOPT_EFFECTS_LEVEL_HW = OptionValuePointer<int>(Options_GetOrCreateOption(
        "EffectsLevel_HW", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_EFFECTS_LEVEL_HW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetEffectsLevelForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, "EffectsLevel_HW", 0));
    }

    ZOPT_GFX_FLAGS_SW = OptionValuePointer<int>(Options_GetOrCreateOption(
        "GfxFlags_SW", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_GFX_FLAGS_SW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetGraphicsFlagsForCurrentHwMode(BuildGraphicsFlags(detailRoot, "GlobalLight_SW", 0));
    }

    ZOPT_GFX_FLAGS_HW = OptionValuePointer<int>(Options_GetOrCreateOption(
        "GfxFlags_HW", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_GFX_FLAGS_HW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetGraphicsFlagsForCurrentHwMode(BuildGraphicsFlags(detailRoot, "GlobalLight_HW", 1));
    }

    ZOPT_OBJECT_LOD_SW = OptionValuePointer<int>(Options_GetOrCreateOption(
        "ObjectLOD_SW", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_OBJECT_LOD_SW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetObjectLODForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, "ObjectLOD_SW", 0));
    }

    ZOPT_OBJECT_LOD_HW = OptionValuePointer<int>(Options_GetOrCreateOption(
        "ObjectLOD_HW", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_OBJECT_LOD_HW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetObjectLODForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, "ObjectLOD_HW", 0));
    }

    ZOPT_TEXTURE_MEMORY_SW = OptionValuePointer<int>(Options_GetOrCreateOption(
        "TextureMemory_SW", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_TEXTURE_MEMORY_SW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetTextureMemoryForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, "TextureMemory_SW", 0));
    }

    ZOPT_TEXTURE_MEMORY_HW = OptionValuePointer<int>(Options_GetOrCreateOption(
        "TextureMemory_HW", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_TEXTURE_MEMORY_HW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetTextureMemoryForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, "TextureMemory_HW", 0));
    }

    ZOPT_GAME_CONTROL_OPTIONS = OptionValuePointer<int>(Options_GetOrCreateOption(
        "GameCtlOptions", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_GAME_CONTROL_OPTIONS != 0) {
        zOpt::SetGameControlOptions(ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON);
    }

    g_zOpt_GameDifficultyOption = OptionValuePointer<int>(Options_GetOrCreateOption(
        "GameIntensity", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (g_zOpt_GameDifficultyOption != 0) {
        zOpt::SetGameDifficultyMode(1);
    }

    ZOPT_MUTE_SOUND = OptionValuePointer<int>(Options_GetOrCreateOption(
        "MuteSound", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_MUTE_SOUND != 0) {
        zOpt::SetMuteSoundOption(0);
    }

    ZOPT_SOUND_VOLUME = OptionValuePointer<float>(Options_GetOrCreateOption(
        "SoundVolume", ZGAME_OPTION_INLINE_BINARY4, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_SOUND_VOLUME != 0) {
        zOpt::SetSoundVolumeOption(1.0f);
    }

    ZOPT_SOUND_LOD = OptionValuePointer<int>(Options_GetOrCreateOption(
        "SoundLOD", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_SOUND_LOD != 0) {
        zOpt::SetSoundLODOption(zOpt::SelectProfileValueForSystem(detailRoot, "SoundLOD", 0));
    }

    ZOPT_AUDIO_API = OptionValuePointer<int>(Options_GetOrCreateOption(
        "SoundAPI", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_AUDIO_API != 0) {
        zSnd::SetAudioApiOption(1);
    }

    ZOPT_PLAYER_NAME = Options_GetOrCreateOption("PlayerName", ZGAME_OPTION_STRING_BUFFER, 0x16,
                                                 ZGAME_OPTION_SCOPE_USER);
    if (ZOPT_PLAYER_NAME != 0) {
        DWORD userNameSize = 0xfe;
        char userName[0x100];
        GetUserNameA(userName, &userNameSize);
        userName[userNameSize] = '\0';
        zOpt::SetPlayerName(userName);
    }

    ZOPT_SOUND_CDAUDIO = OptionValuePointer<int>(Options_GetOrCreateOption(
        "CDAudio", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_SOUND_CDAUDIO != 0) {
        zSnd::SetCDAudioOption(1);
    }

    ZOPT_VIDEO_FULLSCREEN = OptionValuePointer<int>(Options_GetOrCreateOption(
        "FullScreen", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_VIDEO_FULLSCREEN != 0) {
        zOpt::SetFullscreenOption(1);
    }

    ZOPT_HUD_SW = OptionValuePointer<int>(Options_GetOrCreateOption(
        "HUDFlag_SW", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_HUD_SW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetHudVisibilityOption(
            zOpt::SelectProfileValueForSystem(detailRoot, "HUDFlag_SW", 1));
    }

    ZOPT_HUD_HW = OptionValuePointer<int>(Options_GetOrCreateOption(
        "HUDFlag_HW", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_HUD_HW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetHudVisibilityOption(
            zOpt::SelectProfileValueForSystem(detailRoot, "HUDFlag_HW", 1));
    }

    ZOPT_HUD_TYPE_SW = OptionValuePointer<int>(Options_GetOrCreateOption(
        "HUDType_SW", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_HUD_TYPE_SW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetHudTypeForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, "HUDType_SW", 1));
    }

    ZOPT_HUD_TYPE_HW = OptionValuePointer<int>(Options_GetOrCreateOption(
        "HUDType_HW", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_HUD_TYPE_HW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetHudTypeForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, "HUDType_HW", 1));
    }

    ZOPT_HW_API = OptionValuePointer<int>(
        Options_GetOrCreateOption("HWAPI", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_HW_API != 0) {
        zVid::SetHwApiOption(1);
    }

    ZOPT_INPUT_JOYSTICK = OptionValuePointer<int>(Options_GetOrCreateOption(
        "Joystick", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_INPUT_JOYSTICK != 0) {
        zInp::SetJoystickOption(0);
    }

    g_zOpt_WolPasswordFlagOption = OptionValuePointer<int>(Options_GetOrCreateOption(
        "WOLPasswordFlag", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (g_zOpt_WolPasswordFlagOption != 0) {
        zOpt::SetWolPasswordFlag(1);
    }

    ZOPT_JOYSTICK_NUM_AXES = OptionValuePointer<int>(Options_GetOrCreateOption(
        "JoystickNumAxes", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_TRANSIENT));
    if (ZOPT_JOYSTICK_NUM_AXES != 0) {
        zInp::SetJoystickAxesCountOption(0);
    }

    ZOPT_JOYSTICK_NUM_BUTTONS = OptionValuePointer<int>(Options_GetOrCreateOption(
        "JoystickNumButtons", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_TRANSIENT));
    if (ZOPT_JOYSTICK_NUM_BUTTONS != 0) {
        zInp::SetJoystickButtonCountOption(0);
    }

    ZOPT_NETWORK_ENABLED = OptionValuePointer<int>(Options_GetOrCreateOption(
        "Network", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_TRANSIENT));
    if (ZOPT_NETWORK_ENABLED != 0) {
        zOpt::SetNetworkEnabled(0);
    }

    g_zOpt_NetworkModemOption = OptionValuePointer<int>(Options_GetOrCreateOption(
        "NetworkModem", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_TRANSIENT));
    if (g_zOpt_NetworkModemOption != 0) {
        zOpt::SetNetworkModemEnabled(0);
    }

    g_zOpt_NetworkListenOption = OptionValuePointer<int>(Options_GetOrCreateOption(
        "NetListen", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_TRANSIENT));
    if (g_zOpt_NetworkListenOption != 0) {
        zOpt::SetNetworkListenEnabled(0);
    }

    g_zOpt_CameraSectionOption = OptionValuePointer<zOpt_CameraSection *>(Options_GetOrCreateOption(
        "Camera", ZGAME_OPTION_HEAP_BUFFER, 0x0c, ZGAME_OPTION_SCOPE_TRANSIENT));
    g_zOpt_RenderSectionOption =
        OptionValuePointer<zOpt_ViewRectSection *>(Options_GetOrCreateOption(
            "Render", ZGAME_OPTION_HEAP_BUFFER, 0x28, ZGAME_OPTION_SCOPE_TRANSIENT));
    g_zOpt_DisplaySectionOption =
        OptionValuePointer<zOpt_ViewRectSection *>(Options_GetOrCreateOption(
            "Display", ZGAME_OPTION_HEAP_BUFFER, 0x28, ZGAME_OPTION_SCOPE_TRANSIENT));
    g_zOpt_WindowSectionOption =
        OptionValuePointer<zOpt_ViewRectSection *>(Options_GetOrCreateOption(
            "Window", ZGAME_OPTION_HEAP_BUFFER, 0x28, ZGAME_OPTION_SCOPE_TRANSIENT));
    ZOPT_REPLICATE = OptionValuePointer<int>(Options_GetOrCreateOption(
        "Replicate", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_TRANSIENT));

    ZOPT_VIDEO_MODE = OptionValuePointer<int>(
        Options_GetOrCreateOption("VMode", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER));
    if (ZOPT_VIDEO_MODE != 0) {
        zVid::SetVideoModeIndex(zOpt::SelectProfileValueForSystem(detailRoot, "VMode", 5));
    }

    ZOPT_VIDEO_STRIDE = OptionValuePointer<int>(Options_GetOrCreateOption(
        "VStride", ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_TRANSIENT));
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

const int ZOPT_GAME_CONTROL_THROTTLE = 0x01;
const int ZOPT_GAME_CONTROL_STEERING = 0x02;
const int ZOPT_GAME_CONTROL_CURSOR = 0x04;
const int ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON = 0x08;
const double ZOPT_COMPARE_TOLERANCE_PCT = 0.02;

int WrappedAbsDifference(int lhs, int rhs) {
    const unsigned int diff = static_cast<unsigned int>(lhs) - static_cast<unsigned int>(rhs);
    const unsigned int signMask = 0u - (diff >> 31);
    return static_cast<int>((diff ^ signMask) - signMask);
}

} // namespace

// Reimplements 0x407190: zOpt::LookupNamedValueAsInt
RECOIL_NOINLINE int RECOIL_FASTCALL LookupNamedValueAsInt(const char *key) {
    {
        int pairIndex1;
        for (pairIndex1 = 0; pairIndex1 < (int)(sizeof(g_zOpt_NamedScalarValues) / sizeof((g_zOpt_NamedScalarValues)[0])); ++pairIndex1) {
            const zOpt_NameInt32Pair & pair = (g_zOpt_NamedScalarValues)[pairIndex1];
        if (strcmp(pair.name, key) == 0) {
            return pair.value;
        }
    }
    }

    return 0;
}

// Reimplements 0x4071f0: zOpt::ReadScalarValueAsInt
RECOIL_NOINLINE int RECOIL_FASTCALL ReadScalarValueAsInt(zReader::Node *scalarValueNode) {
    if (scalarValueNode->type == zReader::ZRDR_NODE_INT) {
        return scalarValueNode->value.i32;
    }
    if (scalarValueNode->type == zReader::ZRDR_NODE_FLOAT) {
        return static_cast<int>(scalarValueNode->value.f32);
    }
    if (scalarValueNode->type == zReader::ZRDR_NODE_STRING) {
        return LookupNamedValueAsInt(scalarValueNode->value.str);
    }

    return 0;
}

// Reimplements 0x407470: zOpt::EvaluateProfileMetricCondition
RECOIL_NOINLINE int RECOIL_FASTCALL
EvaluateProfileMetricCondition(zReader::Node *metricConditionNode) {
    if (metricConditionNode->type == zReader::ZRDR_NODE_STRING) {
        return strcmp(metricConditionNode->value.str, "DEFAULT") == 0;
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

    if (strcmp(metricKey, "CPU_CLASS") == 0) {
        currentMetricValue = g_zGame_Options_RuntimeConfig.cpuClass;
    } else if (strcmp(metricKey, "CPU_MHZ") == 0) {
        currentMetricValue = g_zGame_Options_RuntimeConfig.cpuMhz;
    } else if (strcmp(metricKey, "VIDEO_KB") == 0) {
        currentMetricValue =
            static_cast<int>(g_zGame_Options_RuntimeConfig.soundHardwareMemKb);
    } else if (strcmp(metricKey, "RAM_KB") == 0) {
        currentMetricValue = static_cast<int>(g_zGame_Options_RuntimeConfig.systemRamKb);
    } else if (strcmp(metricKey, "HW_ACCEL") == 0) {
        currentMetricValue =
            static_cast<int>((g_zGame_Options_RuntimeConfig.defaultFlags >> 6) & 1u);
    } else {
        return 0;
    }

    return EvalIntCompareOp(opString, currentMetricValue, rhs);
}

// Reimplements 0x407680: zOpt::SelectProfileValueForSystem
RECOIL_NOINLINE int RECOIL_FASTCALL SelectProfileValueForSystem(
    zReader::Node *parentNode, const char *profileName, int defaultValue) {
    if (parentNode == 0) {
        return defaultValue;
    }

    zReader::Node *const profileRuleListNode = zReader_GetNamedNode(parentNode, profileName);
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

// Reimplements 0x407220: zOpt::EvalIntCompareOp
RECOIL_NOINLINE int RECOIL_FASTCALL EvalIntCompareOp(const char *opString,
                                                              int lhs, int rhs) {
    if (strcmp(opString, "==") == 0) {
        return lhs == rhs;
    }
    if (strcmp(opString, "<") == 0) {
        return lhs < rhs;
    }
    if (strcmp(opString, ">") == 0) {
        return lhs > rhs;
    }
    if (strcmp(opString, "<=") == 0) {
        return lhs <= rhs;
    }
    if (strcmp(opString, ">=") == 0) {
        return lhs >= rhs;
    }
    if (strcmp(opString, "!=") == 0) {
        return lhs != rhs;
    }
    if (strcmp(opString, "~=") == 0) {
        return static_cast<double>(WrappedAbsDifference(lhs, rhs)) <
               static_cast<double>(lhs) * ZOPT_COMPARE_TOLERANCE_PCT;
    }

    return 0;
}

// Reimplements 0x407e20: zOpt::SetGameControlOptions
RECOIL_NOINLINE void RECOIL_FASTCALL SetGameControlOptions(int value) {
    *ZOPT_GAME_CONTROL_OPTIONS = value;
}

// Reimplements 0x407e30: zOpt::SetThrottleMode
RECOIL_NOINLINE void RECOIL_FASTCALL SetThrottleMode(int enable) {
    if (enable != 0) {
        *ZOPT_GAME_CONTROL_OPTIONS |= ZOPT_GAME_CONTROL_THROTTLE;
    } else {
        *ZOPT_GAME_CONTROL_OPTIONS &= ~ZOPT_GAME_CONTROL_THROTTLE;
    }
}

// Reimplements 0x407e50: zOpt::GetThrottleMode
RECOIL_NOINLINE int RECOIL_CDECL GetThrottleMode() {
    return *ZOPT_GAME_CONTROL_OPTIONS & ZOPT_GAME_CONTROL_THROTTLE;
}

// Reimplements 0x407e60: zOpt::SetSteeringMode
RECOIL_NOINLINE void RECOIL_FASTCALL SetSteeringMode(int enable) {
    if (enable != 0) {
        *ZOPT_GAME_CONTROL_OPTIONS |= ZOPT_GAME_CONTROL_STEERING;
    } else {
        *ZOPT_GAME_CONTROL_OPTIONS &= ~ZOPT_GAME_CONTROL_STEERING;
    }
}

// Reimplements 0x407e80: zOpt::GetSteeringMode
RECOIL_NOINLINE int RECOIL_CDECL GetSteeringMode() {
    return (*ZOPT_GAME_CONTROL_OPTIONS >> 1) & 1;
}

// Reimplements 0x407e90: zOpt::SetCursorMode
RECOIL_NOINLINE void RECOIL_FASTCALL SetCursorMode(int enable) {
    if (enable != 0) {
        *ZOPT_GAME_CONTROL_OPTIONS |= ZOPT_GAME_CONTROL_CURSOR;
    } else {
        *ZOPT_GAME_CONTROL_OPTIONS &= ~ZOPT_GAME_CONTROL_CURSOR;
    }
}

// Reimplements 0x407eb0: zOpt::GetCursorMode
RECOIL_NOINLINE int RECOIL_CDECL GetCursorMode() {
    return (*ZOPT_GAME_CONTROL_OPTIONS >> 2) & 1;
}

// Reimplements 0x407ef0: zOpt::GetCameraModePlayerState
RECOIL_NOINLINE int RECOIL_CDECL GetCameraModePlayerState() {
    return ((~*ZOPT_GAME_CONTROL_OPTIONS & ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON) | 4) >> 2;
}

// Reimplements 0x407f10: zOpt::SetGameDifficultyMode
RECOIL_NOINLINE void RECOIL_FASTCALL SetGameDifficultyMode(int value) {
    *g_zOpt_GameDifficultyOption = value;
}

// Reimplements 0x407f20: zOpt::GetGameDifficultyMode
RECOIL_NOINLINE int RECOIL_CDECL GetGameDifficultyMode() {
    return *g_zOpt_GameDifficultyOption;
}

// Reimplements 0x407f30: zOpt::SetEffectsLevelForCurrentHwMode
RECOIL_NOINLINE void RECOIL_FASTCALL SetEffectsLevelForCurrentHwMode(int level) {
    *(g_zOpt_HwMode != 0 ? ZOPT_EFFECTS_LEVEL_HW : ZOPT_EFFECTS_LEVEL_SW) = level;

    if (level == 0) {
        zEffect::SetConditionalEffectLevel(2);
    } else if (level == 1) {
        zEffect::SetConditionalEffectLevel(1);
    } else if (level == 2) {
        zEffect::SetConditionalEffectLevel(0);
    }
}

// Reimplements 0x407f80: zOpt::GetEffectsLevelForCurrentHwMode
RECOIL_NOINLINE int RECOIL_CDECL GetEffectsLevelForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? ZOPT_EFFECTS_LEVEL_HW : ZOPT_EFFECTS_LEVEL_SW);
}

// Reimplements 0x407fa0: zOpt::SetObjectLODForCurrentHwMode
RECOIL_NOINLINE void RECOIL_FASTCALL SetObjectLODForCurrentHwMode(int level) {
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

    zClass_Camera::gwCameraSetClipDistance(camera, clipDistance);
}

// Reimplements 0x408030: zOpt::GetObjectLODForCurrentHwMode
RECOIL_NOINLINE int RECOIL_CDECL GetObjectLODForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? ZOPT_OBJECT_LOD_HW : ZOPT_OBJECT_LOD_SW);
}

// Reimplements 0x408060: zOpt::GetMuteSoundOption
RECOIL_NOINLINE int RECOIL_CDECL GetMuteSoundOption() {
    return *ZOPT_MUTE_SOUND;
}

// Reimplements 0x408050: zOpt::SetMuteSoundOption
RECOIL_NOINLINE void RECOIL_FASTCALL SetMuteSoundOption(int value) {
    *ZOPT_MUTE_SOUND = value;
    zSnd::ApplyMuteStateToActiveVoices(value);
}

// Reimplements 0x408070: zOpt::SetSoundVolumeOption
RECOIL_NOINLINE void RECOIL_FASTCALL SetSoundVolumeOption(float volume) {
    *ZOPT_SOUND_VOLUME = volume;
    zSnd::SetGlobalVolumeScale(volume);
}

// Reimplements 0x408090: zOpt::GetSoundVolumeOption
RECOIL_NOINLINE float RECOIL_CDECL GetSoundVolumeOption() {
    return *ZOPT_SOUND_VOLUME;
}

// Reimplements 0x4080c0: zOpt::SetSoundLODOption
RECOIL_NOINLINE void RECOIL_FASTCALL SetSoundLODOption(int value) {
    *ZOPT_SOUND_LOD = value;
}

// Reimplements 0x4080d0: zOpt::GetSoundLODOption
RECOIL_NOINLINE int RECOIL_CDECL GetSoundLODOption() {
    return *ZOPT_SOUND_LOD;
}

// Reimplements 0x4080e0: zOpt::SetTextureMemoryForCurrentHwMode
RECOIL_NOINLINE void RECOIL_FASTCALL SetTextureMemoryForCurrentHwMode(int value) {
    *(g_zOpt_HwMode != 0 ? ZOPT_TEXTURE_MEMORY_HW : ZOPT_TEXTURE_MEMORY_SW) = value;
}

// Reimplements 0x408100: zOpt::GetTextureMemoryForCurrentHwMode
RECOIL_NOINLINE int RECOIL_CDECL GetTextureMemoryForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? ZOPT_TEXTURE_MEMORY_HW : ZOPT_TEXTURE_MEMORY_SW);
}

// Reimplements 0x408120: zOpt::SetPlayerName
RECOIL_NOINLINE void RECOIL_FASTCALL SetPlayerName(const char *name) {
    char *const buffer = (char *)(ZOPT_PLAYER_NAME->payloadOrBuffer);
    const unsigned int dataSize = static_cast<unsigned int>(ZOPT_PLAYER_NAME->dataSize);
    const size_t nameLength = strlen(name);

    if (nameLength < dataSize) {
        memcpy(buffer, name, nameLength + 1);
    } else {
        strncpy(buffer, name, dataSize - 1);
        buffer[dataSize - 1] = '\0';
    }
}

// Reimplements 0x4081f0: zOpt::GetGraphicsFlagsForCurrentHwMode
RECOIL_NOINLINE int RECOIL_CDECL GetGraphicsFlagsForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? ZOPT_GFX_FLAGS_HW : ZOPT_GFX_FLAGS_SW);
}

// Reimplements 0x4081a0: zOpt::SetGraphicsFlagsForCurrentHwMode
RECOIL_NOINLINE void RECOIL_FASTCALL SetGraphicsFlagsForCurrentHwMode(int flags) {
    *(g_zOpt_HwMode != 0 ? ZOPT_GFX_FLAGS_HW : ZOPT_GFX_FLAGS_SW) = flags;

    zClass_NodePartial *const sunlight = zClass::FindByTypeAndName(6, "sunlight");
    if (sunlight != 0) {
        zClass_Class::gwNodeSetActive(sunlight, (flags & 0x10) != 0 ? 1 : 0);
    }
}

// Reimplements 0x4082d0: zOpt::SetHudTypeForCurrentHwMode
RECOIL_NOINLINE int RECOIL_FASTCALL SetHudTypeForCurrentHwMode(int hudType) {
    const int previous = HudUiMgr::ApplyHudModeSwitch(hudType);
    int *const option = g_zOpt_HwMode != 0 ? ZOPT_HUD_TYPE_HW : ZOPT_HUD_TYPE_SW;
    *option = hudType;
    return previous;
}

// Reimplements 0x408230: zOpt::SetNetworkEnabled
RECOIL_NOINLINE void RECOIL_FASTCALL SetNetworkEnabled(int value) {
    *ZOPT_NETWORK_ENABLED = value;
}

// Reimplements 0x408240: zOpt::SetNetworkModemEnabled
RECOIL_NOINLINE void RECOIL_FASTCALL SetNetworkModemEnabled(int value) {
    *g_zOpt_NetworkModemOption = value;
}

// Reimplements 0x408250: zOpt::SetNetworkListenEnabled
RECOIL_NOINLINE void RECOIL_FASTCALL SetNetworkListenEnabled(int value) {
    *g_zOpt_NetworkListenOption = value;
}

// Reimplements 0x408270: zOpt::GetNetworkModemEnabled
RECOIL_NOINLINE int RECOIL_CDECL GetNetworkModemEnabled() {
    return *g_zOpt_NetworkModemOption;
}

// Reimplements 0x408a10: zOpt::SetWolPasswordFlag
RECOIL_NOINLINE void RECOIL_FASTCALL SetWolPasswordFlag(int value) {
    *g_zOpt_WolPasswordFlagOption = value;
}

// Reimplements 0x408650: zOpt::GetDisplaySection
RECOIL_NOINLINE zOpt_ViewRectSection *RECOIL_CDECL GetDisplaySection() {
    return *g_zOpt_DisplaySectionOption;
}

// Reimplements 0x408690: zOpt::GetDisplaySectionBitsPerPixel
RECOIL_NOINLINE int RECOIL_CDECL GetDisplaySectionBitsPerPixel() {
    return (*g_zOpt_DisplaySectionOption)->bitsPerPixel;
}

// Reimplements 0x4086a0: zOpt::GetVideoStrideValue
RECOIL_NOINLINE int RECOIL_CDECL GetVideoStrideValue() {
    return *ZOPT_VIDEO_STRIDE;
}

// Reimplements 0x4086c0: zOpt::GetWindowSection
RECOIL_NOINLINE zOpt_ViewRectSection *RECOIL_CDECL GetWindowSection() {
    return *g_zOpt_WindowSectionOption;
}

// Reimplements 0x4086d0: zOpt::GetWindowSectionHeight
RECOIL_NOINLINE int RECOIL_CDECL GetWindowSectionHeight() {
    return (*g_zOpt_WindowSectionOption)->height;
}

// Reimplements 0x4082a0: zOpt::SetFullscreenOption
RECOIL_NOINLINE void RECOIL_FASTCALL SetFullscreenOption(int fullscreenOption) {
    *ZOPT_VIDEO_FULLSCREEN = fullscreenOption;
}

// Reimplements 0x408330: zOpt::GetFullscreenOption
RECOIL_NOINLINE int RECOIL_CDECL GetFullscreenOption() {
    return *ZOPT_VIDEO_FULLSCREEN;
}

// Reimplements 0x4082b0: zOpt::SetHudVisibilityOption
RECOIL_NOINLINE void RECOIL_FASTCALL SetHudVisibilityOption(int hudVisibility) {
    *(g_zOpt_HwMode != 0 ? ZOPT_HUD_HW : ZOPT_HUD_SW) = hudVisibility;
}

// Reimplements 0x408340: zOpt::GetHudVisibilityOption
RECOIL_NOINLINE int RECOIL_CDECL GetHudVisibilityOption() {
    return *(g_zOpt_HwMode != 0 ? ZOPT_HUD_HW : ZOPT_HUD_SW);
}

// Reimplements 0x408360: zOpt::GetHudTypeForCurrentHwMode
RECOIL_NOINLINE int RECOIL_CDECL GetHudTypeForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? ZOPT_HUD_TYPE_HW : ZOPT_HUD_TYPE_SW);
}

// Reimplements 0x408300: zOpt::SetReplicateMode
RECOIL_NOINLINE void RECOIL_FASTCALL SetReplicateMode(int replicateMode) {
    *ZOPT_REPLICATE = replicateMode;
}

// Reimplements 0x408380: zOpt::GetReplicateMode
RECOIL_NOINLINE int RECOIL_CDECL GetReplicateMode() {
    return *ZOPT_REPLICATE;
}

// Reimplements 0x408260: zOpt::GetNetworkEnabled
RECOIL_NOINLINE int RECOIL_CDECL GetNetworkEnabled() {
    return *ZOPT_NETWORK_ENABLED;
}

// Reimplements 0x4083d0: zOpt::ViewRectSection_SetPosition
RECOIL_NOINLINE void RECOIL_FASTCALL ViewRectSection_SetPosition(zOpt_ViewRectSection *section,
                                                                 int x, int y) {
    section->x = x;
    section->y = y;
    section->rightExclusive = x + section->width;
    section->bottomExclusive = y + section->height;
    section->maxXInclusive = section->rightExclusive - 1;
    section->maxYInclusive = section->bottomExclusive - 1;
}

// Reimplements 0x408400: zOpt::ViewRectSection_SetSize
RECOIL_NOINLINE void RECOIL_FASTCALL ViewRectSection_SetSize(zOpt_ViewRectSection *section,
                                                             int width,
                                                             int height) {
    section->width = width;
    section->height = height;
    section->rightExclusive = section->x + width;
    section->bottomExclusive = section->y + height;
    section->maxXInclusive = section->rightExclusive - 1;
    section->maxYInclusive = section->bottomExclusive - 1;
}

// Reimplements 0x408430: zOpt::ViewRectSection_ClampPointToInclusiveBounds
RECOIL_NOINLINE void RECOIL_FASTCALL
ViewRectSection_ClampPointToInclusiveBounds(zOpt_ViewRectSection *section, float *pointXY) {
    if (pointXY[0] < static_cast<float>(section->x)) {
        pointXY[0] = static_cast<float>(section->x);
    } else if (!(pointXY[0] <= static_cast<float>(section->maxXInclusive))) {
        pointXY[0] = static_cast<float>(section->maxXInclusive);
    }

    if (pointXY[1] < static_cast<float>(section->y)) {
        pointXY[1] = static_cast<float>(section->y);
    } else if (!(pointXY[1] <= static_cast<float>(section->maxYInclusive))) {
        pointXY[1] = static_cast<float>(section->maxYInclusive);
    }
}

// Reimplements 0x408530: zOpt::RenderSection_SetPosition
RECOIL_NOINLINE void RECOIL_FASTCALL RenderSection_SetPosition(int x, int y) {
    zOpt_ViewRectSection *section = *g_zOpt_RenderSectionOption;
    ViewRectSection_SetPosition(section, x, y);
    if (section->target != 0) {
        zClass_NodePartial * target = static_cast<zClass_NodePartial *>(section->target);
        zClass_Window::gwWindowSetResolution(target, section->width, section->height);
        zClass_Window::gwWindowSetSize(target, section->x, section->y);
    }
}

// Reimplements 0x408500: zOpt::RenderSection_SetSize
RECOIL_NOINLINE void RECOIL_FASTCALL RenderSection_SetSize(int width,
                                                           int height) {
    zOpt_ViewRectSection *section = *g_zOpt_RenderSectionOption;
    ViewRectSection_SetSize(section, width, height);
    if (section->target != 0) {
        zClass_Window::gwWindowSetResolution(static_cast<zClass_NodePartial *>(section->target),
                                             section->width, section->height);
    }
}

// Reimplements 0x408570: zOpt::RenderSection_SetTargetWindow
RECOIL_NOINLINE void RECOIL_FASTCALL RenderSection_SetTargetWindow(zClass_NodePartial *windowNode) {
    zOpt_ViewRectSection *section = *g_zOpt_RenderSectionOption;
    section->target = windowNode;
    if (windowNode != 0) {
        zClass_Window::gwWindowSetResolution(windowNode, section->width, section->height);
        zClass_Window::gwWindowSetSize(static_cast<zClass_NodePartial *>(section->target),
                                       section->x, section->y);
    }
}

// Reimplements 0x4085a0: zOpt::GetRenderSection
RECOIL_NOINLINE zOpt_ViewRectSection *RECOIL_CDECL GetRenderSection() {
    return *g_zOpt_RenderSectionOption;
}

// Reimplements 0x4085e0: zOpt::DisplaySection_SetPosition
RECOIL_NOINLINE void RECOIL_FASTCALL DisplaySection_SetPosition(int x, int y) {
    zOpt_ViewRectSection *section = *g_zOpt_DisplaySectionOption;
    ViewRectSection_SetPosition(section, x, y);
    if (section->target != 0) {
        zClass_NodePartial * target = static_cast<zClass_NodePartial *>(section->target);
        zClass_Display::gwDisplaySetSize(target, section->width, section->height);
        zClass_Display::gwDisplaySetPosition(target, section->x, section->y);
    }
}

// Reimplements 0x408620: zOpt::DisplaySection_SetSize
RECOIL_NOINLINE void RECOIL_FASTCALL DisplaySection_SetSize(int width,
                                                            int height) {
    zOpt_ViewRectSection *section = *g_zOpt_DisplaySectionOption;
    ViewRectSection_SetSize(section, width, height);
    if (section->target != 0) {
        zClass_Display::gwDisplaySetSize(static_cast<zClass_NodePartial *>(section->target),
                                         section->width, section->height);
    }
}

// Reimplements 0x4085b0: zOpt::DisplaySection_SetTargetDisplay
RECOIL_NOINLINE void RECOIL_FASTCALL
DisplaySection_SetTargetDisplay(zClass_NodePartial *displayNode) {
    zOpt_ViewRectSection *section = *g_zOpt_DisplaySectionOption;
    section->target = displayNode;
    if (displayNode != 0) {
        zClass_Display::gwDisplaySetSize(displayNode, section->width, section->height);
        zClass_Display::gwDisplaySetPosition(static_cast<zClass_NodePartial *>(section->target),
                                             section->x, section->y);
    }
}

// Reimplements 0x408680: zOpt::DisplaySection_SetBitsPerPixel
RECOIL_NOINLINE void RECOIL_FASTCALL DisplaySection_SetBitsPerPixel(int bitsPerPixel) {
    (*g_zOpt_DisplaySectionOption)->bitsPerPixel = bitsPerPixel;
}

// Reimplements 0x408700: zOpt::WindowSection_SetPosition
RECOIL_NOINLINE void RECOIL_FASTCALL WindowSection_SetPosition(int x, int y) {
    ViewRectSection_SetPosition(*g_zOpt_WindowSectionOption, x, y);
}

// Reimplements 0x4086e0: zOpt::WindowSection_SetSize
RECOIL_NOINLINE void RECOIL_FASTCALL WindowSection_SetSize(int width,
                                                           int height) {
    ViewRectSection_SetSize(*g_zOpt_WindowSectionOption, width, height);
}

// Reimplements 0x408480: zOpt::CameraSection_SetActiveCamera
RECOIL_NOINLINE void RECOIL_FASTCALL CameraSection_SetActiveCamera(zClass_NodePartial *camera) {
    zOpt_CameraSection *const cameraSection = *g_zOpt_CameraSectionOption;
    cameraSection->m_pCamera = camera;
    if (camera == 0) {
        return;
    }

    zOpt_ViewRectSection *const renderSection = *g_zOpt_RenderSectionOption;
    float fovX = 0.0f;
    float fovY = 0.0f;
    zClass_Camera::gwCameraGetFOV(camera, &fovX, &fovY);

    fovX =
        static_cast<float>(renderSection->width) * fovY / static_cast<float>(renderSection->height);
    zClass_Camera::gwCameraSetFOV(cameraSection->m_pCamera, fovX, fovY);
    zOpt::SetObjectLODForCurrentHwMode(zOpt::GetObjectLODForCurrentHwMode());
}
} // namespace zOpt

// Reimplements 0x4084e0: zOpt_CameraSection_GetActiveCamera
RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL zOpt_CameraSection_GetActiveCamera() {
    if (g_zOpt_CameraSectionOption == 0 || *g_zOpt_CameraSectionOption == 0) {
        return 0;
    }

    return (*g_zOpt_CameraSectionOption)->m_pCamera;
}

// Reimplements 0x408190: zOpt_GetPlayerName
RECOIL_NOINLINE char *RECOIL_CDECL zOpt_GetPlayerName() {
    return (char *)(ZOPT_PLAYER_NAME->payloadOrBuffer);
}

// Reimplements 0x408a20: zOpt_GetWolPasswordFlagValue
RECOIL_NOINLINE int RECOIL_CDECL zOpt_GetWolPasswordFlagValue() {
    return *g_zOpt_WolPasswordFlagOption;
}

// Reimplements 0x408660: zOpt_DisplaySection_GetWidth
RECOIL_NOINLINE int RECOIL_CDECL zOpt_DisplaySection_GetWidth() {
    return (*g_zOpt_DisplaySectionOption)->width;
}

// Reimplements 0x408670: zOpt_DisplaySection_GetHeight
RECOIL_NOINLINE int RECOIL_CDECL zOpt_DisplaySection_GetHeight() {
    return (*g_zOpt_DisplaySectionOption)->height;
}
