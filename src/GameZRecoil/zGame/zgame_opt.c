#include "recoil/Mfc42Abi.h"

#include "zgame.h"

#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zclass.h"

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
 * Reimplements data 0x4e5d00..0x4e5d98:
 * g_zGame_Options_PointerCache.
 * Purpose: caches the option-value and section pointers populated by game
 * option loading as one ordered record.
 */
zGame_OptionsPointerCache g_zGame_Options_PointerCache = {0};
/**
 * Reimplements data 0x4e5dcc: g_zOpt_HwMode.
 * Purpose: selects the current software or hardware option-value members.
 */
int g_zOpt_HwMode = 0;
zGame_OptionsRuntimeConfig g_zGame_Options_RuntimeConfig = {0};

}

RECOIL_STATIC_ASSERT(sizeof(zGame_OptionsPointerCache) == 0x98);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, effectsLevelSw) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, effectsLevelHw) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, gfxFlagsSw) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, gfxFlagsHw) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, objectLodSw) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, objectLodHw) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, textureMemorySw) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, textureMemoryHw) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, hudVisibilitySw) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, hudVisibilityHw) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, hudTypeSw) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, hudTypeHw) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, videoMode) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, audioApi) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, soundLod) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, gameControlOptions) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, muteSound) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, soundVolume) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, gameDifficulty) == 0x48);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, playerName) == 0x4c);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, cdAudio) == 0x50);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, videoFullscreen) == 0x54);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, videoAcceleration) == 0x58);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, hardwareApi) == 0x5c);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, inputJoystick) == 0x60);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, joystickNumAxes) == 0x64);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, joystickNumButtons) == 0x68);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, replicate) == 0x6c);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, videoStride) == 0x70);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, networkEnabled) == 0x74);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, networkListen) == 0x78);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, cameraSection) == 0x7c);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, renderSection) == 0x80);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, displaySection) == 0x84);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, windowSection) == 0x88);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, unusedOption) == 0x8c);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, networkModem) == 0x90);
RECOIL_STATIC_ASSERT(offsetof(zGame_OptionsPointerCache, wolPasswordFlag) == 0x94);

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

namespace zGame {
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

} // namespace zGame

namespace zSnd {

/**
 * Reimplements 0x4b2f50: zSnd::AcquireCachedDirectSound.
 * Purpose: return the matching cached DirectSound device or create and cache a
 * new device for the requested GUID.
 */
LPDIRECTSOUND __fastcall AcquireCachedDirectSound(
    const GUID *deviceGuid
) {
    LPDIRECTSOUND cached = g_zSnd_CachedDirectSound;
    if (cached != 0) {
        if (deviceGuid == g_zSnd_CachedDirectSoundGuid) {
            return cached;
        }

        cached->Release();
        g_zSnd_CachedDirectSound = 0;
    }

    if (DirectSoundCreate(
        (LPGUID)(deviceGuid),
        &g_zSnd_CachedDirectSound,
        0
    ) != DS_OK) {
        return 0;
    }

    g_zSnd_CachedDirectSoundGuid = deviceGuid;
    return g_zSnd_CachedDirectSound;
}

/**
 * Reimplements 0x4b2fa0: zSnd::ReleaseCachedDirectSound.
 * Purpose: release and clear the cached DirectSound device when present.
 */
void ReleaseCachedDirectSound() {
    LPDIRECTSOUND cached = g_zSnd_CachedDirectSound;
    if (cached != 0) {
        cached->Release();
        g_zSnd_CachedDirectSound = 0;
    }
}

/**
 * Reimplements 0x4b2fc0: zSnd::CachedDirectSound_GetCaps.
 * Purpose: initialize the DirectSound caps structure size and query the cached
 * DirectSound device.
 */
HRESULT __fastcall CachedDirectSound_GetCaps(
    DSCAPS *caps
) {
    caps->dwSize = sizeof(DSCAPS);
    return g_zSnd_CachedDirectSound->GetCaps(caps);
}

} // namespace zSnd

namespace zSys {

/**
 * Reimplements 0x4b2fe0: zSys::HasCpuidSupportRuntimeOptions.
 * Purpose: repeats the EFLAGS ID-bit probe for runtime option setup callers;
 * VC5 C++ cannot express the required EFLAGS toggle and register-preservation
 * sequence, so this documented raw-assembly CPU-probe exception keeps that
 * sequence local.
 */
int HasCpuidSupportRuntimeOptions() {
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
    int changedFlags = 0;
    __asm {
        push ebx
        push ecx
        push edx
        pushfd
        pop eax
        mov ecx, eax
        xor eax, 0200000h
        push eax
        popfd
        pushfd
        pop eax
        xor eax, ecx
        mov dword ptr [changedFlags], eax
        pop edx
        pop ecx
        pop ebx
    }
    return changedFlags != 0 ? 1 : 0;
#else
    return HasCpuidSupport() != 0 ? 1 : 0;
#endif
}

} // namespace zSys

namespace zCpu {

/**
 * Reimplements 0x4b3020: zCpu::HasMmxSupport.
 * Purpose: probe CPUID feature bit 23 for MMX support; VC5 C++ has no CPUID
 * intrinsic, so this documented raw-assembly CPU-probe exception emits the
 * opcode locally.
 */
int HasMmxSupport() {
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
    int result;
    __asm {
        push ebx
        push ecx
        push edx
        mov eax, 1
        _emit 0x0f
        _emit 0xa2
        test edx, 0800000h
        jne recoil_zgame_cpu_mmx_support_done
        xor eax, eax
    recoil_zgame_cpu_mmx_support_done:
        mov dword ptr [result], eax
        pop edx
        pop ecx
        pop ebx
    }
    return result != 0 ? 1 : 0;
#else
    int cpuInfo[4] = {0};
    __cpuid(
        cpuInfo,
        1
    );
    return (cpuInfo[3] & 0x800000) != 0 ? 1 : 0;
#endif
}

} // namespace zCpu

namespace zSys {

/**
 * Reimplements 0x4b3050: zSys::CheckCpuSignatureMask.
 * Purpose: read CPUID leaf 1 and test the optimized-path signature mask; VC5
 * C++ has no CPUID intrinsic, so this documented raw-assembly CPU-probe
 * exception emits the opcode while preserving the retail register shape.
 */
int CheckCpuSignatureMask() {
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
    unsigned int cpuidSignature;
    __asm {
        xor esi, esi
        push ebx
        push ecx
        push edx
        mov eax, 1
        _emit 0x0f
        _emit 0xa2
        mov dword ptr [cpuidSignature], eax
        pop edx
        pop ecx
        pop ebx
        mov eax, dword ptr [cpuidSignature]
        and eax, 0630h
        cmp eax, 0630h
        jne recoil_zgame_cpu_signature_done
        mov esi, 1
    recoil_zgame_cpu_signature_done:
        mov eax, esi
    }
#else
    int cpuInfo[4] = {0};
    __cpuid(
        cpuInfo,
        1
    );
    return (cpuInfo[0] & 0x630) == 0x630 ? 1 : 0;
#endif
}

} // namespace zSys

/**
 * Reimplements 0x4b3090: zGame_OptionsRuntimeConfig::CopyDefault.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame.cpp.
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
 * Reimplements 0x4b30b0: zGame_OptionsRuntimeConfig::InitFromSystem.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame.cpp.
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

/**
 * Reimplements 0x4b3160: zGame_OptionsRuntimeConfig::LoadCpuVendorString.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: load the CPUID vendor string into the runtime configuration when
 * available; VC5 C++ has no CPUID intrinsic, so this documented raw-assembly
 * CPU-probe exception emits the opcode and stores its fixed-register result
 * directly.
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

namespace zSys {

/**
 * Reimplements 0x4b31b0: zSys::GetCpuClass.
 * Purpose: return the low-word CPU class from the recovered CPU detection packet.
 */
int GetCpuClass() {
    return DetectCpuClassAndFeatures() & 0xffff;
}

} // namespace zSys

namespace zSnd {

/**
 * Reimplements 0x4b31f0: zSnd::HasMmxMixerSupport.
 * Purpose: report MMX mixer availability only when CPUID probing is available.
 */
int HasMmxMixerSupport() {
    if (zSys::HasCpuidSupportRuntimeOptions() == 0) {
        return 0;
    }

    return zCpu::HasMmxSupport();
}

} // namespace zSnd

namespace zSys {

/**
 * Reimplements 0x4b3210: zSys::ReturnZeroStub.
 * Purpose: return zero for callers that need a stable legacy system stub.
 */
int ReturnZeroStub() {
    return 0;
}

} // namespace zSys

namespace zVid {

/**
 * Reimplements 0x4b3220: zVid::HasAcceptedHardwareRenderer.
 * Purpose: report whether the cached renderer list contains an accepted entry.
 */
int HasAcceptedHardwareRenderer() {
    return GetAcceptedHardwareRendererCount_Cached() > 0 ? 1 : 0;
}

} // namespace zVid

namespace zSys {

/**
 * Reimplements 0x4b3230: zSys::GetTotalPhysKb.
 * Purpose: read Windows memory status and return physical memory in kilobytes.
 */
RECOIL_NO_GS unsigned int GetTotalPhysKb() {
    MEMORYSTATUS status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);
    return status.dwTotalPhys >> 10;
}

} // namespace zSys

namespace zGame {

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

} // namespace zGame

namespace zOpt {


























} // namespace zOpt


namespace zOpt {




















} // namespace zOpt


namespace zOpt {









} // namespace zOpt



namespace zOpt {









} // namespace zOpt
