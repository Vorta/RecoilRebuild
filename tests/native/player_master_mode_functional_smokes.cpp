#include "Battlesport/game_net.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <windows.h>

#include <cmath>
#include <cstdint>
#include <cstring>

#include <dsound.h>

namespace {
bool FloatNear(
    float actual,
    float expected
) {
    return std::fabs(actual - expected) < 0.0001f;
}

float PlayerDampingFactor(
    float rate,
    float deltaTime
) {
    const int bits = static_cast<int>(-rate * deltaTime * 12102200.0f) + 0x3f800000;
    float factor = 0.0f;
    std::memcpy(&factor, &bits, sizeof(factor));
    return factor;
}

void InitObjectPositionNode(
    zClass_NodePartial *node,
    zClass_Object3DDataPartial *data,
    float x,
    float y,
    float z
) {
    std::memset(node, 0, sizeof(*node));
    std::memset(data, 0, sizeof(*data));
    node->classId = 5;
    node->classData = data;
    node->flags = 1;
    data->localMatrix[9] = x;
    data->localMatrix[10] = y;
    data->localMatrix[11] = z;
}

class TestDirectSoundBuffer final : public IDirectSoundBuffer {
  public:
    int playCount = 0;
    int stopCount = 0;

    HRESULT __stdcall QueryInterface(
        REFIID,
        LPVOID *objectOut
    ) override {
        if (objectOut != nullptr) {
            *objectOut = nullptr;
        }
        return E_NOINTERFACE;
    }

    ULONG __stdcall AddRef() override {
        return 1;
    }

    ULONG __stdcall Release() override {
        return 1;
    }

    HRESULT __stdcall GetCaps(
        LPDSBCAPS caps
    ) override {
        if (caps != nullptr) {
            std::memset(caps, 0, sizeof(*caps));
            caps->dwSize = sizeof(*caps);
        }
        return DS_OK;
    }

    HRESULT __stdcall GetCurrentPosition(
        LPDWORD playCursor,
        LPDWORD writeCursor
    ) override {
        if (playCursor != nullptr) {
            *playCursor = 0;
        }
        if (writeCursor != nullptr) {
            *writeCursor = 0;
        }
        return DS_OK;
    }

    HRESULT __stdcall GetFormat(
        LPWAVEFORMATEX,
        DWORD,
        LPDWORD bytesWritten
    ) override {
        if (bytesWritten != nullptr) {
            *bytesWritten = 0;
        }
        return DS_OK;
    }

    HRESULT __stdcall GetVolume(
        LPLONG volume
    ) override {
        if (volume != nullptr) {
            *volume = 0;
        }
        return DS_OK;
    }

    HRESULT __stdcall GetPan(
        LPLONG pan
    ) override {
        if (pan != nullptr) {
            *pan = 0;
        }
        return DS_OK;
    }

    HRESULT __stdcall GetFrequency(
        LPDWORD frequency
    ) override {
        if (frequency != nullptr) {
            *frequency = 0;
        }
        return DS_OK;
    }

    HRESULT __stdcall GetStatus(
        LPDWORD status
    ) override {
        if (status != nullptr) {
            *status = 0;
        }
        return DS_OK;
    }

    HRESULT __stdcall Initialize(
        LPDIRECTSOUND,
        LPCDSBUFFERDESC
    ) override {
        return DS_OK;
    }

    HRESULT __stdcall Lock(
        DWORD,
        DWORD,
        LPVOID *,
        LPDWORD,
        LPVOID *,
        LPDWORD,
        DWORD
    ) override {
        return DSERR_UNSUPPORTED;
    }

    HRESULT __stdcall Play(
        DWORD,
        DWORD,
        DWORD
    ) override {
        ++playCount;
        return DS_OK;
    }

    HRESULT __stdcall SetCurrentPosition(
        DWORD
    ) override {
        return DS_OK;
    }

    HRESULT __stdcall SetFormat(
        LPCWAVEFORMATEX
    ) override {
        return DS_OK;
    }

    HRESULT __stdcall SetVolume(
        LONG
    ) override {
        return DS_OK;
    }

    HRESULT __stdcall SetPan(
        LONG
    ) override {
        return DS_OK;
    }

    HRESULT __stdcall SetFrequency(
        DWORD
    ) override {
        return DS_OK;
    }

    HRESULT __stdcall Stop() override {
        ++stopCount;
        return DS_OK;
    }

    HRESULT __stdcall Unlock(
        LPVOID,
        DWORD,
        LPVOID,
        DWORD
    ) override {
        return DS_OK;
    }

    HRESULT __stdcall Restore() override {
        return DS_OK;
    }
};

zSndBuffer *SoundBackendBuffer(
    TestDirectSoundBuffer &buffer
) {
    return reinterpret_cast<zSndBuffer *>(static_cast<IDirectSoundBuffer *>(&buffer));
}
} // namespace

extern "C" int player_auto_switch_to_next_usable_alt_weapon_smoke(
    void
) {
    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    HudUiTextStack4 *const oldTopMessageStack = g_HudUiTopMessageStack;
    const int oldActiveWeaponMessageIndex = g_HudUiMgrActiveWeaponMessageIndex;
    const int oldActiveWeaponSideIndex = g_HudUiMgrActiveWeaponSideIndex;
    HudUiMessage oldMessages[10] = {};
    for (int i = 0; i < 10; ++i) {
        oldMessages[i] = g_HudUiMgrMessages[i];
    }

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalData.masterType = 1;

    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState = static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&playerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    g_LocalPlayerSaveState = &saveState;

    HudUiTopMessageStack topStack = {};
    topStack.Constructor();
    topStack.enabled = 0;
    g_HudUiTopMessageStack = &topStack;

    zVidImagePartial images[10][5] = {};
    for (int i = 0; i < 10; ++i) {
        HudUiMessage &message = g_HudUiMgrMessages[i];
        message = HudUiMessage{};
        message.variantImages[3] = &images[i][3];
        message.variantImages[4] = &images[i][4];
    }
    g_HudUiMgrMessages[4].panel.activeSideIndex = 1;
    g_HudUiMgrMessages[6].panel.activeSideIndex = 1;
    g_HudUiMgrActiveWeaponMessageIndex = 0;
    g_HudUiMgrActiveWeaponSideIndex = 0;

    OptCatalogEntryDef entryA = {};
    entryA.description = const_cast<char *>("auto-a");
    OptCatalogEntryDef entryB = {};
    entryB.description = const_cast<char *>("auto-b");

    auto reset_state = [&]() {
        playerState = {};
        saveState.playerState = &playerState;
        saveState.primaryModalState = &modalState;
        playerState.altGunTransitionState = 1;
        modalData.masterType = 1;
    };
    auto arm_controller = [&](PlayerGunFireController &controller, OptCatalogEntryDef *entry, int bankIndex, int sideIndex, float ammo) {
        controller.optCatalogEntry = entry;
        controller.weaponBankIndex = bankIndex;
        controller.weaponSideIndex = sideIndex;
        controller.flags = 4;
        controller.ammoOrCharge = ammo;
    };
    auto cleanup = [&]() {
        g_LocalPlayerSaveState = oldLocalSaveState;
        g_GameStateOrMapTable = oldGameStateOrMapTable;
        g_HudUiTopMessageStack = oldTopMessageStack;
        g_HudUiMgrActiveWeaponMessageIndex = oldActiveWeaponMessageIndex;
        g_HudUiMgrActiveWeaponSideIndex = oldActiveWeaponSideIndex;
        for (int i = 0; i < 10; ++i) {
            g_HudUiMgrMessages[i] = oldMessages[i];
        }
    };

    reset_state();
    PlayerAltWeaponBank &sameBank = playerState.altWeaponBanks[4];
    sameBank.selectedSide = 0;
    arm_controller(sameBank.controllerA, &entryA, 4, 0, 0.0f);
    arm_controller(sameBank.controllerB, &entryB, 4, 1, 5.0f);
    playerState.activeAltGunController = &sameBank.controllerA;
    playerState.activeAltBankIndex = 4;
    Player::AutoSwitchToNextUsableAltWeapon(&saveState);
    if (playerState.activeAltGunController != &sameBank.controllerB) {
        cleanup();
        return 1;
    }
    if (playerState.activeAltBankIndex != 4) {
        cleanup();
        return 11;
    }
    if (playerState.cachedAltSelectionCode != 401) {
        cleanup();
        return 12;
    }

    reset_state();
    PlayerAltWeaponBank &activeBank = playerState.altWeaponBanks[5];
    activeBank.selectedSide = 0;
    arm_controller(activeBank.controllerA, &entryA, 5, 0, 0.0f);
    arm_controller(activeBank.controllerB, &entryB, 5, 1, 0.0f);
    playerState.activeAltGunController = &activeBank.controllerB;
    playerState.activeAltBankIndex = 5;
    PlayerAltWeaponBank &lowerBank = playerState.altWeaponBanks[4];
    lowerBank.selectedSide = 0;
    arm_controller(lowerBank.controllerA, &entryA, 4, 0, 6.0f);
    Player::AutoSwitchToNextUsableAltWeapon(&saveState);
    if (playerState.activeAltGunController != &lowerBank.controllerA || playerState.activeAltBankIndex != 4 || playerState.cachedAltSelectionCode != 400) {
        cleanup();
        return 2;
    }

    reset_state();
    PlayerAltWeaponBank &activeBankUp = playerState.altWeaponBanks[4];
    activeBankUp.selectedSide = 0;
    arm_controller(activeBankUp.controllerA, &entryA, 4, 0, 0.0f);
    arm_controller(activeBankUp.controllerB, &entryB, 4, 1, 0.0f);
    playerState.activeAltGunController = &activeBankUp.controllerA;
    playerState.activeAltBankIndex = 4;
    PlayerAltWeaponBank &upperBank = playerState.altWeaponBanks[6];
    upperBank.selectedSide = 1;
    arm_controller(upperBank.controllerA, &entryA, 6, 0, 0.0f);
    arm_controller(upperBank.controllerB, &entryB, 6, 1, 7.0f);
    Player::AutoSwitchToNextUsableAltWeapon(&saveState);
    const bool upwardOk = playerState.activeAltGunController == &upperBank.controllerB && playerState.activeAltBankIndex == 6 && playerState.cachedAltSelectionCode == 601;

    cleanup();
    return upwardOk ? 0 : 3;
}

extern "C" int player_ensure_master_type_loop_sfx_handle_smoke(
    void
) {
    void *const oldVolumeScalePtr = g_zSnd_GlobalVolumeScalePtr;
    const int oldInitialized = g_zSnd_IsInitialized;
    const int oldPreInitialized = g_zSnd_PreInitialized;
    const int oldBackend = g_zSnd_ActiveBackend;
    const int oldMuteDepth = g_zSnd_MuteDepth;
    const int oldPlaybackEnabled = g_zSnd_Flag10PlaybackEnabled;

    TestDirectSoundBuffer directSoundBuffer;

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 0.5f;
    sample.primaryVoice.backendBuffer = SoundBackendBuffer(directSoundBuffer);

    PlayerMasterCommonData commonData = {};
    zUtil_PlayerStateStorage playerState = {};
    playerState.masterCommonData = &commonData;
    playerState.worldPos = {2.0f, 3.0f, 4.0f};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;

    zSndPlayHandle existingHandle = {};
    playerState.modeLoopSfxHandle[1] = &existingHandle;
    commonData.sfxWeaponUp[1] = &sample;
    saveState.EnsureMasterTypeLoopSfxHandle(1, 0.25f);
    if (playerState.modeLoopSfxHandle[1] != &existingHandle || directSoundBuffer.playCount != 0) {
        g_zSnd_GlobalVolumeScalePtr = oldVolumeScalePtr;
        g_zSnd_IsInitialized = oldInitialized;
        g_zSnd_PreInitialized = oldPreInitialized;
        g_zSnd_ActiveBackend = oldBackend;
        g_zSnd_MuteDepth = oldMuteDepth;
        g_zSnd_Flag10PlaybackEnabled = oldPlaybackEnabled;
        return 1;
    }

    saveState.EnsureMasterTypeLoopSfxHandle(2, 0.5f);
    if (playerState.modeLoopSfxHandle[2] != nullptr || directSoundBuffer.playCount != 0) {
        g_zSnd_GlobalVolumeScalePtr = oldVolumeScalePtr;
        g_zSnd_IsInitialized = oldInitialized;
        g_zSnd_PreInitialized = oldPreInitialized;
        g_zSnd_ActiveBackend = oldBackend;
        g_zSnd_MuteDepth = oldMuteDepth;
        g_zSnd_Flag10PlaybackEnabled = oldPlaybackEnabled;
        return 2;
    }

    commonData.sfxWeaponUp[2] = &sample;
    saveState.EnsureMasterTypeLoopSfxHandle(2, 0.75f);

    const bool ok = playerState.modeLoopSfxHandle[2] == &sample.primaryVoice && sample.primaryVoice.ownerSample == &sample && directSoundBuffer.playCount == 1;

    g_zSnd_GlobalVolumeScalePtr = oldVolumeScalePtr;
    g_zSnd_IsInitialized = oldInitialized;
    g_zSnd_PreInitialized = oldPreInitialized;
    g_zSnd_ActiveBackend = oldBackend;
    g_zSnd_MuteDepth = oldMuteDepth;
    g_zSnd_Flag10PlaybackEnabled = oldPlaybackEnabled;
    return ok ? 0 : 3;
}

extern "C" int player_handle_alt_weapon_bank_select_smoke(
    void
) {
    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    HudUiTextStack4 *const oldTopMessageStack = g_HudUiTopMessageStack;
    void *const oldVolumeScalePtr = g_zSnd_GlobalVolumeScalePtr;
    const int oldInitialized = g_zSnd_IsInitialized;
    const int oldPreInitialized = g_zSnd_PreInitialized;
    const int oldBackend = g_zSnd_ActiveBackend;
    const int oldMuteDepth = g_zSnd_MuteDepth;
    const int oldPlaybackEnabled = g_zSnd_Flag10PlaybackEnabled;
    const HudUiMessage oldMessage0 = g_HudUiMgrMessages[0];
    const HudUiMessage oldMessage3 = g_HudUiMgrMessages[3];

    TestDirectSoundBuffer directSoundBuffer;

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 0.5f;
    sample.primaryVoice.backendBuffer = SoundBackendBuffer(directSoundBuffer);

    PlayerMasterCommonData commonData = {};
    commonData.sfxWeaponUp[0] = &sample;
    PlayerMasterModalData modalData = {};
    modalData.masterType = 1;
    PlayerModalState modalState = {};
    modalState.masterModalData = &modalData;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    playerState.masterCommonData = &commonData;
    playerState.altGunTransitionState = 1;
    playerState.activeAltBankIndex = 2;

    OptCatalogEntryDef entryA = {};
    entryA.description = const_cast<char *>("alt-a");
    OptCatalogEntryDef entryB = {};
    entryB.description = const_cast<char *>("alt-b");
    PlayerAltWeaponBank &activeBank = playerState.altWeaponBanks[2];
    activeBank.controllerA.optCatalogEntry = &entryA;
    activeBank.controllerA.weaponBankIndex = 2;
    activeBank.controllerA.weaponSideIndex = 0;
    activeBank.controllerA.flags = 4;
    activeBank.controllerA.ammoOrCharge = 3.0f;
    playerState.activeAltGunController = &activeBank.controllerA;

    PlayerAltWeaponBank &targetBank = playerState.altWeaponBanks[3];
    targetBank.selectedSide = 1;
    targetBank.controllerA.optCatalogEntry = &entryA;
    targetBank.controllerA.weaponBankIndex = 3;
    targetBank.controllerA.weaponSideIndex = 0;
    targetBank.controllerA.flags = 4;
    targetBank.controllerA.ammoOrCharge = 0.0f;
    targetBank.controllerB.optCatalogEntry = &entryB;
    targetBank.controllerB.weaponBankIndex = 3;
    targetBank.controllerB.weaponSideIndex = 1;
    targetBank.controllerB.flags = 4;
    targetBank.controllerB.ammoOrCharge = 5.0f;

    HudUiTopMessageStack topStack = {};
    topStack.Constructor();
    topStack.enabled = 0;
    g_HudUiTopMessageStack = &topStack;

    HudUiMessage &message = g_HudUiMgrMessages[3];
    message = HudUiMessage{};
    HudUiMessage &previousMessage = g_HudUiMgrMessages[0];
    previousMessage = HudUiMessage{};
    zVidImagePartial images[5] = {};
    message.variantImages[3] = &images[3];
    message.variantImages[4] = &images[4];
    message.panel.activeSideIndex = 1;

    g_LocalPlayerSaveState = &saveState;
    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState = static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&playerState));
    g_GameStateOrMapTable = &gameStateOrMap;

    Player::HandleAltWeaponBankSelectInput(0x11);
    int result = 0;
    if (playerState.activeAltGunController != &targetBank.controllerB) {
        result = 1;
    } else if (playerState.activeAltBankIndex != 3) {
        result = 2;
    } else if (targetBank.selectedSide != 1) {
        result = 3;
    } else if (playerState.cachedAltSelectionCode != 301) {
        result = 4;
    } else if (playerState.modeLoopSfxHandle[0] != &sample.primaryVoice) {
        result = 5;
    } else if (directSoundBuffer.playCount != 1) {
        result = 6;
    } else if (g_HudUiMgrMessages[3].image != &images[4]) {
        result = 7;
    } else if (std::strcmp(message.panel.textBuffer, "5") != 0) {
        result = 8;
    }

    playerState.altGunTransitionState = 1;
    Player::HandleAltWeaponBankSelectInput(0x11);
    const bool failureOk = playerState.activeAltGunController == &targetBank.controllerB && targetBank.selectedSide == 1 && directSoundBuffer.playCount == 1;

    if (result == 0 && !failureOk) {
        result = 9;
    }

    g_LocalPlayerSaveState = oldLocalSaveState;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_HudUiTopMessageStack = oldTopMessageStack;
    g_zSnd_GlobalVolumeScalePtr = oldVolumeScalePtr;
    g_zSnd_IsInitialized = oldInitialized;
    g_zSnd_PreInitialized = oldPreInitialized;
    g_zSnd_ActiveBackend = oldBackend;
    g_zSnd_MuteDepth = oldMuteDepth;
    g_zSnd_Flag10PlaybackEnabled = oldPlaybackEnabled;
    g_HudUiMgrMessages[0] = oldMessage0;
    g_HudUiMgrMessages[3] = oldMessage3;

    return result;
}

extern "C" int player_handle_primary_weapon_variant_toggle_smoke(
    void
) {
    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    HudUiTextStack4 *const oldTopMessageStack = g_HudUiTopMessageStack;
    void *const oldVolumeScalePtr = g_zSnd_GlobalVolumeScalePtr;
    const int oldInitialized = g_zSnd_IsInitialized;
    const int oldPreInitialized = g_zSnd_PreInitialized;
    const int oldBackend = g_zSnd_ActiveBackend;
    const int oldMuteDepth = g_zSnd_MuteDepth;
    const int oldPlaybackEnabled = g_zSnd_Flag10PlaybackEnabled;
    const HudUiMessage oldMessage1 = g_HudUiMgrMessages[1];

    TestDirectSoundBuffer directSoundBuffer;

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 0.5f;
    sample.primaryVoice.backendBuffer = SoundBackendBuffer(directSoundBuffer);

    PlayerMasterCommonData commonData = {};
    commonData.sfxWeaponUp[2] = &sample;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;

    OptCatalogEntryDef entryA = {};
    entryA.description = const_cast<char *>("primary-a");
    OptCatalogEntryDef entryB = {};
    entryB.description = const_cast<char *>("primary-b");

    PlayerAltWeaponBank &bank = playerState.altWeaponBanks[1];
    bank.controllerA.optCatalogEntry = &entryA;
    bank.controllerA.weaponBankIndex = 1;
    bank.controllerA.weaponSideIndex = 0;
    bank.controllerA.flags = 0;
    bank.controllerA.ammoOrCharge = 0.0f;
    bank.controllerB.optCatalogEntry = &entryB;
    bank.controllerB.weaponBankIndex = 1;
    bank.controllerB.weaponSideIndex = 1;

    zClass_NodePartial nodeA = {};
    zClass_NodePartial nodeB = {};
    nodeA.classId = 5;
    nodeA.flags = 0x04;
    nodeB.classId = 5;
    bank.controllerA.attachNodePrimary = &nodeA;
    bank.controllerB.attachNodePrimary = &nodeB;

    HudUiTopMessageStack topStack = {};
    topStack.Constructor();
    topStack.enabled = 0;
    g_HudUiTopMessageStack = &topStack;

    HudUiMessage &message = g_HudUiMgrMessages[1];
    message = HudUiMessage{};
    zVidImagePartial images[5] = {};
    message.variantImages[3] = &images[3];
    message.variantImages[4] = &images[4];

    g_LocalPlayerSaveState = &saveState;
    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState = static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&playerState));
    g_GameStateOrMapTable = &gameStateOrMap;

    auto cleanup = [&]() {
        g_LocalPlayerSaveState = oldLocalSaveState;
        g_GameStateOrMapTable = oldGameStateOrMapTable;
        g_HudUiTopMessageStack = oldTopMessageStack;
        g_zSnd_GlobalVolumeScalePtr = oldVolumeScalePtr;
        g_zSnd_IsInitialized = oldInitialized;
        g_zSnd_PreInitialized = oldPreInitialized;
        g_zSnd_ActiveBackend = oldBackend;
        g_zSnd_MuteDepth = oldMuteDepth;
        g_zSnd_Flag10PlaybackEnabled = oldPlaybackEnabled;
        g_HudUiMgrMessages[1] = oldMessage1;
    };

    playerState.activePrimaryGunController = &bank.controllerA;
    bank.controllerB.flags = 0;
    bank.controllerB.ammoOrCharge = 7.0f;
    Player::HandlePrimaryWeaponVariantToggleInput(123);
    if (playerState.activePrimaryGunController != &bank.controllerA || playerState.modeLoopSfxHandle[2] != nullptr || directSoundBuffer.playCount != 0) {
        cleanup();
        return 1;
    }

    bank.controllerB.flags = 4;
    bank.controllerB.ammoOrCharge = 0.0f;
    Player::HandlePrimaryWeaponVariantToggleInput(456);
    if (playerState.activePrimaryGunController != &bank.controllerA || playerState.modeLoopSfxHandle[2] != nullptr || directSoundBuffer.playCount != 0) {
        cleanup();
        return 2;
    }

    bank.controllerB.ammoOrCharge = 7.0f;
    message.panel.activeSideIndex = 1;
    Player::HandlePrimaryWeaponVariantToggleInput(789);
    const bool sideBOk = playerState.activePrimaryGunController == &bank.controllerB && playerState.cachedPrimarySelectionCode == 101 &&
                         playerState.primaryHardpointSelectState == 2 && playerState.modeLoopSfxHandle[2] == &sample.primaryVoice && sample.primaryVoice.ownerSample == &sample &&
                         g_HudUiMgrMessages[1].image == &images[4] && std::strcmp(message.panel.textBuffer, "7") == 0 && (nodeA.flags & 0x04) == 0 && (nodeB.flags & 0x04) != 0 &&
                         directSoundBuffer.playCount == 1;
    if (!sideBOk) {
        cleanup();
        return 3;
    }

    message.panel.activeSideIndex = 0;
    Player::HandlePrimaryWeaponVariantToggleInput(321);
    const bool sideAOk = playerState.activePrimaryGunController == &bank.controllerA && playerState.cachedPrimarySelectionCode == 100 &&
                         g_HudUiMgrMessages[1].image == &images[3] && std::strcmp(message.panel.textBuffer, "0") == 0 && (nodeA.flags & 0x04) != 0 && (nodeB.flags & 0x04) == 0 &&
                         directSoundBuffer.playCount == 2;

    cleanup();
    return sideAOk ? 0 : 4;
}

extern "C" int player_is_alt_weapon_allowed_in_current_master_mode_smoke(
    void
) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    OptCatalogEntryDef entry = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;

    modalData.masterType = 2;
    entry.flags = 0;
    if (Player::IsAltWeaponAllowedInCurrentMasterMode(&saveState, &entry) != 1) {
        return 1;
    }

    entry.flags = 0x1000;
    if (Player::IsAltWeaponAllowedInCurrentMasterMode(&saveState, &entry) != 0) {
        return 2;
    }

    entry.flags = 0x02;
    if (Player::IsAltWeaponAllowedInCurrentMasterMode(&saveState, &entry) != 0) {
        return 3;
    }

    modalData.masterType = 1;
    entry.flags = 0x1002;
    return Player::IsAltWeaponAllowedInCurrentMasterMode(&saveState, &entry) == 1 ? 0 : 4;
}

extern "C" int player_stop_master_type_loop_sfx_handle_smoke(
    void
) {
    TestDirectSoundBuffer directSoundBuffer;

    const int oldInitialized = g_zSnd_IsInitialized;
    const int oldPreInitialized = g_zSnd_PreInitialized;
    const int oldBackend = g_zSnd_ActiveBackend;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zSndPlayHandle handle = {};
    handle.backendBuffer = SoundBackendBuffer(directSoundBuffer);
    saveState.playerState = &playerState;
    playerState.modeLoopSfxHandle[2] = &handle;

    saveState.StopMasterTypeLoopSfxHandle(2);
    const bool stopped = playerState.modeLoopSfxHandle[2] == nullptr && directSoundBuffer.stopCount == 1;

    saveState.StopMasterTypeLoopSfxHandle(1);
    const bool nullSlotOk = playerState.modeLoopSfxHandle[1] == nullptr && directSoundBuffer.stopCount == 1;

    g_zSnd_IsInitialized = oldInitialized;
    g_zSnd_PreInitialized = oldPreInitialized;
    g_zSnd_ActiveBackend = oldBackend;

    return stopped && nullSlotOk ? 0 : 1;
}

extern "C" int player_stop_modal_loop_sfx_handle_smoke(
    void
) {
    zUtil_SaveGameState saveState = {};
    PlayerModalState modalState = {};
    zSndPlayHandle handle = {};
    saveState.primaryModalState = &modalState;
    modalState.modalSfxHandle[2] = &handle;

    saveState.StopModalLoopSfxHandle(2);
    if (modalState.modalSfxHandle[2] != nullptr) {
        return 1;
    }

    saveState.StopModalLoopSfxHandle(1);
    return modalState.modalSfxHandle[1] == nullptr ? 0 : 2;
}

extern "C" int player_tick_master_type_and_force_feedback_smoke(
    void
) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const int oldConditionalEnabled = g_zEffect_ConditionalRefPosEnabled;
    const float oldConditionalX = g_zEffect_ConditionalRefPosX;
    const float oldConditionalY = g_zEffect_ConditionalRefPosY;
    const float oldConditionalZ = g_zEffect_ConditionalRefPosZ;
    int *const oldJoystickOption = g_zGame_Options_PointerCache.inputJoystick;
    const int oldForceFeedbackCaps = g_zInput_JoystickCaps_ForceFeedback;
    zInput_FFEffectSet *const oldEffectSet = g_zInputFfEffectSet;

    Player::TickMasterTypeAndForceFeedback(nullptr);

    zUtil_SaveGameState inactiveSave = {};
    zUtil_PlayerStateStorage inactiveState = {};
    PlayerModalState inactiveModalState = {};
    PlayerMasterModalData inactiveModalData = {};
    inactiveSave.playerState = &inactiveState;
    inactiveSave.primaryModalState = &inactiveModalState;
    inactiveModalState.masterModalData = &inactiveModalData;
    inactiveState.lifecycleState = 4;
    inactiveState.damageProtectionActive = 1;
    inactiveState.throttleInput = 1.0f;
    Player::TickMasterTypeAndForceFeedback(&inactiveSave);
    if (!FloatNear(inactiveState.throttleInput, 1.0f)) {
        g_GameStateOrMapTable = oldGameStateOrMapTable;
        return 1;
    }

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    int joystickEnabled = 0;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalData.masterType = 6;
    playerState.lifecycleState = 1;
    playerState.damageProtectionActive = 1;
    playerState.throttleInput = 1.0f;
    playerState.steeringInput = 2.0f;
    playerState.subVerticalInput = 3.0f;
    playerState.subPitchInput = 4.0f;
    playerState.worldPos = {5.0f, 6.0f, 7.0f};
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    g_zGame_Options_PointerCache.inputJoystick = &joystickEnabled;
    g_zInput_JoystickCaps_ForceFeedback = 0;
    g_zInputFfEffectSet = nullptr;
    g_zEffect_ConditionalRefPosEnabled = 0;

    Player::TickMasterTypeAndForceFeedback(&saveState);
    const bool ok = playerState.throttleInput == 0.0f && playerState.steeringInput == 0.0f && playerState.subVerticalInput == 0.0f && playerState.subPitchInput == 0.0f &&
                    g_zEffect_ConditionalRefPosEnabled == 1 && FloatNear(g_zEffect_ConditionalRefPosX, 5.0f) && FloatNear(g_zEffect_ConditionalRefPosY, 6.0f) &&
                    FloatNear(g_zEffect_ConditionalRefPosZ, 7.0f);

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_zEffect_ConditionalRefPosEnabled = oldConditionalEnabled;
    g_zEffect_ConditionalRefPosX = oldConditionalX;
    g_zEffect_ConditionalRefPosY = oldConditionalY;
    g_zEffect_ConditionalRefPosZ = oldConditionalZ;
    g_zGame_Options_PointerCache.inputJoystick = oldJoystickOption;
    g_zInput_JoystickCaps_ForceFeedback = oldForceFeedbackCaps;
    g_zInputFfEffectSet = oldEffectSet;

    return ok ? 0 : 2;
}

extern "C" int player_update_master_type_amphib_from_modal_probe_smoke(
    void
) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldInvDeltaTime = g_Player_InvDeltaTime;
    const float oldFrameDeltaTimeSec = g_FrameDeltaTimeSec;
    const float oldAccumulatedTimeSec = g_Time_AccumulatedTimeSec;
    const zVec3 oldAmphibBasisUpRef = g_Player_AmphibBasisUpRef;
    const float oldAmphibSteerBasisLerpRate = g_Player_AmphibSteerBasisLerpRate;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&identityMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage globalPlayerState = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial globalRootNode = {};
    playerState.rootNode = &rootNode;
    globalPlayerState.rootNode = &globalRootNode;
    playerState.motionBasis = identityMatrix;
    playerState.steerBasisNorm = {0.0f, 0.0f, -1.0f};
    playerState.steerBasisRef = {0.0f, 1.0f, 0.0f};
    playerState.worldPos = {5.0f, 1.0f, 7.0f};
    playerState.projectileSpawnVel = {3.0f, -2.0f, 4.0f};
    playerState.localVel = {6.0f, -1.0f, 8.0f};
    playerState.restartYawRad = 0.0f;
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x42;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;

    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState = static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&globalPlayerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    g_Player_DeltaTime = 0.25f;
    g_Player_InvDeltaTime = 4.0f;
    g_FrameDeltaTimeSec = 0.25f;
    g_Time_AccumulatedTimeSec = 12.0f;
    g_Player_AmphibBasisUpRef = {0.0f, 1.0f, 0.0f};
    g_Player_AmphibSteerBasisLerpRate = 0.0f;
    g_VariantTag_Current.count = 0;
    g_VariantTag_Current.tags[0] = 0xff;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;

    PlayerMasterModalData masterModalData = {};
    masterModalData.masterType = 1;
    masterModalData.yawRateMax = 3.5f;
    masterModalData.modeAltTransitionTime = 2.0f;
    masterModalData.probePoints[0] = {0.5f, 0.0f, 0.5f};
    masterModalData.probePoints[1] = {0.0f, 0.0f, 0.0f};
    masterModalData.probePoints[2] = {0.0f, 0.0f, 1.0f};
    masterModalData.probePoints[3] = {1.0f, 0.0f, 0.0f};

    PlayerModalState modalState = {};
    modalState.masterModalData = &masterModalData;
    modalState.modalStateCode = 4;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    zVec3 probeFaceVertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};
    std::int32_t faceIndices[3] = {0, 1, 2};
    zModel_MaterialPartial materialPayload = {};
    materialPayload.userTag = 1;
    zModel_PickFaceUvData faceUvData = {};
    zModel_PickFaceEntry faceEntry = {};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = faceIndices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload = static_cast<zModel_PickFaceScenePayload *>(static_cast<void *>(&materialPayload));
    faceEntry.variantTag.count = 1;
    faceEntry.variantTag.tags[0] = 0x42;
    zModel_PickFaceData faceData = {};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = probeFaceVertices;

    zClass_Object3DDataPartial objectData = {};
    objectData.flags = 8;
    zClass_NodePartial objectNode = {};
    objectNode.flags = 0x11c;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    objectNode.cachedBounds[0] = 0.0f;
    objectNode.cachedBounds[1] = 0.0f;
    objectNode.cachedBounds[2] = 0.0f;
    objectNode.cachedBounds[3] = 1.0f;
    objectNode.cachedBounds[4] = 1.0f;
    objectNode.cachedBounds[5] = 1.0f;
    zClass_NodePartial *areaChildren[1] = {&objectNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = areaRows;
    zClass_NodePartial world = {};
    world.classData = &worldData;
    g_Player_RuntimeDiScene = &world;

    Player::UpdateMasterTypeAmphib_FromModalProbe(&saveState);

    const bool ok = playerState.yawVelocityLimit == 3.5f && playerState.amphibProbeCoverageFailed == 0 && FloatNear(playerState.worldPos.y, 2.0f) &&
                    FloatNear(playerState.localVel.y, 4.0f) && FloatNear(playerState.projectileSpawnVel.y, 4.0f) && FloatNear(playerState.steerBasisRef.x, 0.0f) &&
                    FloatNear(playerState.steerBasisRef.y, 1.0f) && FloatNear(playerState.steerBasisRef.z, 0.0f) && FloatNear(playerState.steerBasisRaw.x, 0.0f) &&
                    FloatNear(playerState.steerBasisRaw.y, 0.0f) && FloatNear(playerState.steerBasisRaw.z, -1.0f) && FloatNear(playerState.motionBasis.xx, 1.0f) &&
                    FloatNear(playerState.motionBasis.yy, 1.0f) && FloatNear(playerState.motionBasis.zz, 1.0f) && FloatNear(playerState.motionBasis.posY, 2.0f) &&
                    FloatNear(playerState.vehiclePitchRad, 0.0f) && FloatNear(playerState.vehicleRollRad, 0.0f);

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_InvDeltaTime = oldInvDeltaTime;
    g_FrameDeltaTimeSec = oldFrameDeltaTimeSec;
    g_Time_AccumulatedTimeSec = oldAccumulatedTimeSec;
    g_Player_AmphibBasisUpRef = oldAmphibBasisUpRef;
    g_Player_AmphibSteerBasisLerpRate = oldAmphibSteerBasisLerpRate;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;

    return ok ? 0 : 1;
}

extern "C" int player_update_master_type_amphib_smoke(
    void
) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldInvDeltaTime = g_Player_InvDeltaTime;
    const float oldFrameDeltaTimeSec = g_FrameDeltaTimeSec;
    const float oldAccumulatedTimeSec = g_Time_AccumulatedTimeSec;
    const zVec3 oldAmphibBasisUpRef = g_Player_AmphibBasisUpRef;
    const float oldAmphibSteerBasisLerpRate = g_Player_AmphibSteerBasisLerpRate;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&identityMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage globalPlayerState = {};
    zClass_Object3DDataPartial rootData = {};
    zClass_Object3DDataPartial wakeData = {};
    zClass_Object3DDataPartial splashLData = {};
    zClass_Object3DDataPartial splashRData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial globalRootNode = {};
    zClass_NodePartial wakeNode = {};
    zClass_NodePartial splashLNode = {};
    zClass_NodePartial splashRNode = {};
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    wakeNode.classId = 5;
    wakeNode.classData = &wakeData;
    splashLNode.classId = 5;
    splashLNode.classData = &splashLData;
    splashRNode.classId = 5;
    splashRNode.classData = &splashRData;
    playerState.rootNode = &rootNode;
    globalPlayerState.rootNode = &globalRootNode;
    playerState.motionBasis = identityMatrix;
    playerState.previousTransform = identityMatrix;
    playerState.localVel = {0.5f, 0.0f, 0.5f};
    playerState.throttleInputCopy = 1.0f;
    playerState.worldPos = {0.0f, 1.0f, 0.0f};
    playerState.fxOffsetLocal = {0.25f, 0.5f, 0.75f};
    playerState.restartYawRad = 0.0f;
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x42;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;

    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState = static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&globalPlayerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    g_Player_DeltaTime = 0.25f;
    g_Player_InvDeltaTime = 4.0f;
    g_FrameDeltaTimeSec = 0.25f;
    g_Time_AccumulatedTimeSec = 12.0f;
    g_Player_AmphibBasisUpRef = {0.0f, 1.0f, 0.0f};
    g_Player_AmphibSteerBasisLerpRate = 0.0f;
    g_VariantTag_Current.count = 0;
    g_VariantTag_Current.tags[0] = 0xff;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;

    PlayerMasterModalData masterModalData = {};
    masterModalData.masterType = 1;
    masterModalData.maxSpeed = 10.0f;
    masterModalData.yawRateMax = 3.5f;
    masterModalData.modeAltTransitionTime = 2.0f;
    masterModalData.probePoints[0] = {0.5f, 0.0f, 0.5f};
    masterModalData.probePoints[1] = {0.0f, 0.0f, 0.0f};
    masterModalData.probePoints[2] = {0.0f, 0.0f, 1.0f};
    masterModalData.probePoints[3] = {1.0f, 0.0f, 0.0f};

    PlayerModalState modalState = {};
    modalState.masterModalData = &masterModalData;
    modalState.modalStateCode = 4;
    modalState.nodeWake = &wakeNode;
    modalState.nodeSplashL = &splashLNode;
    modalState.nodeSplashR = &splashRNode;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    zVec3 probeFaceVertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 10.0f}, {10.0f, 0.0f, 0.0f}};
    std::int32_t faceIndices[3] = {0, 1, 2};
    zModel_MaterialPartial materialPayload = {};
    materialPayload.userTag = 1;
    zModel_PickFaceUvData faceUvData = {};
    zModel_PickFaceEntry faceEntry = {};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = faceIndices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload = static_cast<zModel_PickFaceScenePayload *>(static_cast<void *>(&materialPayload));
    faceEntry.variantTag.count = 1;
    faceEntry.variantTag.tags[0] = 0x42;
    zModel_PickFaceData faceData = {};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = probeFaceVertices;

    zClass_Object3DDataPartial objectData = {};
    objectData.flags = 8;
    zClass_NodePartial objectNode = {};
    objectNode.flags = 0x11c;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    objectNode.cachedBounds[0] = 0.0f;
    objectNode.cachedBounds[1] = 0.0f;
    objectNode.cachedBounds[2] = 0.0f;
    objectNode.cachedBounds[3] = 10.0f;
    objectNode.cachedBounds[4] = 1.0f;
    objectNode.cachedBounds[5] = 10.0f;
    zClass_NodePartial *areaChildren[1] = {&objectNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 10.0f;
    worldData.areaCellSizeZ = 10.0f;
    worldData.areaInvSizeX = 0.1f;
    worldData.areaInvSizeZ = 0.1f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = areaRows;
    zClass_NodePartial world = {};
    world.classData = &worldData;
    g_Player_RuntimeDiScene = &world;

    Player::UpdateMasterTypeAmphib(&saveState);
    int result = 0;
    if (!FloatNear(playerState.projectileSpawnVel.x, 0.5f)) {
        result = FloatNear(playerState.projectileSpawnVel.x, 0.0f) ? 16 : 2;
    } else if (!FloatNear(playerState.projectileSpawnVel.y, 4.0f)) {
        result = FloatNear(playerState.projectileSpawnVel.y, 0.0f) ? 17 : (FloatNear(playerState.projectileSpawnVel.y, 8.0f) ? 18 : 3);
    } else if (!FloatNear(playerState.projectileSpawnVel.z, 0.5f)) {
        result = FloatNear(playerState.projectileSpawnVel.z, 0.0f) ? 15 : 4;
    } else if (!FloatNear(playerState.worldPos.x, 0.125f)) {
        result = 5;
    } else if (!FloatNear(playerState.worldPos.y, 2.0f)) {
        result = 6;
    } else if (!FloatNear(playerState.worldPos.z, 0.125f)) {
        result = 7;
    } else if (!FloatNear(playerState.motionBasis.posX, 0.125f) || !FloatNear(playerState.motionBasis.posY, 2.0f) || !FloatNear(playerState.motionBasis.posZ, 0.125f)) {
        result = 8;
    } else if (!FloatNear(playerState.fxOffsetWorld.x, 0.375f) || !FloatNear(playerState.fxOffsetWorld.y, 2.5f) || !FloatNear(playerState.fxOffsetWorld.z, 0.875f)) {
        result = 9;
    } else if (!FloatNear(playerState.bankBasis.x, 0.0f) || !FloatNear(playerState.bankBasis.y, 0.0f) || !FloatNear(playerState.bankBasis.z, -1.0f)) {
        result = 10;
    } else if (!FloatNear(playerState.previousTransform.posX, rootData.localMatrix[9]) || !FloatNear(playerState.previousTransform.posY, rootData.localMatrix[10]) ||
               !FloatNear(playerState.previousTransform.posZ, rootData.localMatrix[11])) {
        result = 11;
    } else if (!FloatNear(wakeData.scale.x, 0.05f) || !FloatNear(splashLData.scale.y, 0.05f) || !FloatNear(splashRData.scale.z, 0.05f)) {
        result = 12;
    }

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_InvDeltaTime = oldInvDeltaTime;
    g_FrameDeltaTimeSec = oldFrameDeltaTimeSec;
    g_Time_AccumulatedTimeSec = oldAccumulatedTimeSec;
    g_Player_AmphibBasisUpRef = oldAmphibBasisUpRef;
    g_Player_AmphibSteerBasisLerpRate = oldAmphibSteerBasisLerpRate;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;

    return result;
}

extern "C" int player_update_master_type_basic_smoke(
    void
) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldPlayerDeltaTimeScaled001 = g_Player_DeltaTimeScaled001;

    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage globalPlayerState = {};
    PlayerMasterModalData modalData = {};
    PlayerModalState modalState = {};
    zUtil_SaveGameState saveState = {};
    zClass_Object3DDataPartial objectData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial globalRootNode = {};
    zClass_WorldDataPartial worldData = {};
    zClass_NodePartial worldNode = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalState.modalStateCode = 0;

    rootNode.classId = 5;
    rootNode.classData = &objectData;
    playerState.rootNode = &rootNode;
    globalPlayerState.rootNode = &globalRootNode;
    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState = static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&globalPlayerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    worldNode.classData = &worldData;
    g_Player_RuntimeDiScene = &worldNode;

    objectData.scale = {1.0f, 1.0f, 1.0f};
    objectData.localMatrix[0] = 1.0f;
    objectData.localMatrix[4] = 1.0f;
    objectData.localMatrix[8] = 1.0f;

    modalData.maxSpeed = 100.0f;
    modalData.yawRateMax = 12.0f;
    modalData.accelRate = 0.0f;
    modalData.yawAccel = 0.0f;
    modalData.yawDamping = 0.0f;
    modalData.rateDampingAccel = 0.0f;
    modalData.rateDampingDecel = 0.0f;
    modalData.modeAltTransitionTime = 7.0f;

    playerState.worldPos = {10.0f, 5.0f, 20.0f};
    playerState.localVel = {2.0f, 3.0f, 4.0f};
    playerState.throttleInputCopy = 1.0f;
    playerState.fxOffsetLocal = {1.0f, 2.0f, 3.0f};
    playerState.cameraState = 0;
    g_Player_DeltaTime = 0.5f;
    g_Player_DeltaTimeScaled001 = 0.0f;

    Player::UpdateMasterTypeBasic(&saveState);

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_DeltaTimeScaled001 = oldPlayerDeltaTimeScaled001;

    if (!FloatNear(playerState.worldPos.x, 11.0f) || !FloatNear(playerState.worldPos.y, 7.0f) || !FloatNear(playerState.worldPos.z, 22.0f) ||
        !FloatNear(playerState.projectileSpawnVel.x, 2.0f) || !FloatNear(playerState.projectileSpawnVel.y, 3.0f) || !FloatNear(playerState.projectileSpawnVel.z, 4.0f) ||
        playerState.axisClampRuntime != 100.0f || playerState.yawVelocityLimit != 12.0f || playerState.vehiclePitchRad != 0.0f || playerState.vehicleRollRad != 0.0f) {
        return 1;
    }

    if (!FloatNear(playerState.fxOffsetWorld.x, 12.0f) || !FloatNear(playerState.fxOffsetWorld.y, 9.0f) || !FloatNear(playerState.fxOffsetWorld.z, 25.0f)) {
        return 2;
    }
    if (!FloatNear(playerState.previousTransform.posX, objectData.localMatrix[9]) || !FloatNear(playerState.previousTransform.posY, objectData.localMatrix[10]) ||
        !FloatNear(playerState.previousTransform.posZ, objectData.localMatrix[11])) {
        return 4;
    }

    return FloatNear(playerState.bankBasis.x, 0.0f) && FloatNear(playerState.bankBasis.y, 0.0f) && FloatNear(playerState.bankBasis.z, -1.0f) &&
                   playerState.cachedPitchRad == playerState.vehiclePitchRad && playerState.cachedYawRad == playerState.restartYawRad &&
                   playerState.cachedRollRad == playerState.vehicleRollRad
               ? 0
               : 3;
}

extern "C" int player_update_master_type_hover_from_modal_probe_smoke(
    void
) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldFrameDeltaTimeSec = g_FrameDeltaTimeSec;
    const float oldAccumulatedTimeSec = g_Time_AccumulatedTimeSec;
    const float oldMaxSlope = g_Player_MaxSlope;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&identityMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage globalPlayerState = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial globalRootNode = {};
    zClass_NodePartial hoverVariantNode = {};
    playerState.rootNode = &rootNode;
    playerState.modeVariantNode = &hoverVariantNode;
    hoverVariantNode.classId = 5;
    globalPlayerState.rootNode = &globalRootNode;
    playerState.motionBasis = identityMatrix;
    playerState.previousTransform = identityMatrix;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.steerBasisRef = {0.0f, 1.0f, 0.0f};
    playerState.gravityAccel = 10.0f;
    playerState.slipSfxActive = 1;
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x42;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;

    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState = static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&globalPlayerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    g_Player_DeltaTime = 0.25f;
    g_FrameDeltaTimeSec = 0.25f;
    g_Time_AccumulatedTimeSec = 0.0f;
    g_Player_MaxSlope = 0.5f;
    g_VariantTag_Current.count = 0;
    g_VariantTag_Current.tags[0] = 0xff;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;

    PlayerMasterModalData masterModalData = {};
    masterModalData.masterType = 1;
    masterModalData.yawRateMax = 2.5f;
    masterModalData.modeAltTransitionTime = 2.0f;
    masterModalData.probePoints[0] = {0.5f, 0.0f, 0.5f};
    masterModalData.probePoints[1] = {0.0f, 0.0f, 0.0f};
    masterModalData.probePoints[2] = {0.0f, 0.0f, 1.0f};
    masterModalData.probePoints[3] = {1.0f, 0.0f, 0.0f};
    masterModalData.probePoints[15] = {0.5f, 3.0f, 0.5f};
    masterModalData.probePoints[16] = {0.0f, 3.0f, 0.0f};
    masterModalData.probePoints[17] = {0.0f, 3.0f, 1.0f};
    masterModalData.probePoints[18] = {1.0f, 3.0f, 0.0f};

    PlayerModalState modalState = {};
    modalState.masterModalData = &masterModalData;
    modalState.modalStateCode = 4;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    zVec3 probeFaceVertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 10.0f}, {10.0f, 0.0f, 0.0f}};
    std::int32_t faceIndices[3] = {0, 1, 2};
    zModel_MaterialPartial materialPayload = {};
    materialPayload.userTag = 4;
    zModel_PickFaceUvData faceUvData = {};
    zModel_PickFaceEntry faceEntry = {};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = faceIndices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload = static_cast<zModel_PickFaceScenePayload *>(static_cast<void *>(&materialPayload));
    faceEntry.variantTag.count = 1;
    faceEntry.variantTag.tags[0] = 0x42;
    zModel_PickFaceData faceData = {};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = probeFaceVertices;

    zClass_Object3DDataPartial objectData = {};
    objectData.flags = 8;
    zClass_NodePartial objectNode = {};
    objectNode.flags = 0x11c;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    objectNode.cachedBounds[0] = 0.0f;
    objectNode.cachedBounds[1] = 0.0f;
    objectNode.cachedBounds[2] = 0.0f;
    objectNode.cachedBounds[3] = 10.0f;
    objectNode.cachedBounds[4] = 1.0f;
    objectNode.cachedBounds[5] = 10.0f;
    zClass_NodePartial *areaChildren[1] = {&objectNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 10.0f;
    worldData.areaCellSizeZ = 10.0f;
    worldData.areaInvSizeX = 0.1f;
    worldData.areaInvSizeZ = 0.1f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = areaRows;
    zClass_NodePartial world = {};
    world.classData = &worldData;
    g_Player_RuntimeDiScene = &world;

    Player::UpdateMasterTypeHover_FromModalProbe(&saveState);

    int result = 0;
    if (playerState.yawVelocityLimit != 2.5f) {
        result = 2;
    } else if (!FloatNear(playerState.steerBasisRef.x, 0.0f) || !FloatNear(playerState.steerBasisRef.y, 1.0f) || !FloatNear(playerState.steerBasisRef.z, 0.0f)) {
        result = 3;
    } else if (!FloatNear(playerState.localVel.y, 5.0f)) {
        result = 4;
    } else if (!FloatNear(playerState.projectileSpawnVel.y, 5.0f)) {
        result = 5;
    } else if (!FloatNear(playerState.vehiclePitchRad, 0.0f) || !FloatNear(playerState.vehicleRollRad, 0.0f)) {
        result = 6;
    } else if (!FloatNear(modalState.transformedProbePointWorldByIndex[0].y, 3.0f)) {
        result = 8;
    } else if ((hoverVariantNode.flags & 0x04) == 0) {
        result = 7;
    }

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_FrameDeltaTimeSec = oldFrameDeltaTimeSec;
    g_Time_AccumulatedTimeSec = oldAccumulatedTimeSec;
    g_Player_MaxSlope = oldMaxSlope;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;

    return result;
}

extern "C" int player_update_master_type_hover_smoke(
    void
) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldFrameDeltaTimeSec = g_FrameDeltaTimeSec;
    const float oldAccumulatedTimeSec = g_Time_AccumulatedTimeSec;
    const float oldMaxSlope = g_Player_MaxSlope;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&identityMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage globalPlayerState = {};
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial globalRootNode = {};
    zClass_NodePartial hoverVariantNode = {};
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    hoverVariantNode.classId = 5;
    playerState.rootNode = &rootNode;
    playerState.modeVariantNode = &hoverVariantNode;
    globalPlayerState.rootNode = &globalRootNode;
    playerState.motionBasis = identityMatrix;
    playerState.previousTransform = identityMatrix;
    playerState.steerBasisRef = {0.0f, 1.0f, 0.0f};
    playerState.localVel = {0.0f, 0.0f, 0.0f};
    playerState.worldPos = {1.0f, 0.0f, 2.0f};
    playerState.fxOffsetLocal = {0.25f, 0.5f, 0.75f};
    playerState.gravityAccel = 10.0f;
    playerState.slipSfxActive = 1;
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x42;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;

    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState = static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&globalPlayerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    g_Player_DeltaTime = 0.25f;
    g_FrameDeltaTimeSec = 0.25f;
    g_Time_AccumulatedTimeSec = 0.0f;
    g_Player_MaxSlope = 0.5f;
    g_VariantTag_Current.count = 0;
    g_VariantTag_Current.tags[0] = 0xff;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;

    PlayerMasterModalData masterModalData = {};
    masterModalData.masterType = 1;
    masterModalData.maxSpeed = 12.0f;
    masterModalData.yawRateMax = 2.5f;
    masterModalData.modeAltTransitionTime = 2.0f;
    masterModalData.probePoints[0] = {0.5f, 0.0f, 0.5f};
    masterModalData.probePoints[1] = {0.0f, 0.0f, 0.0f};
    masterModalData.probePoints[2] = {0.0f, 0.0f, 1.0f};
    masterModalData.probePoints[3] = {1.0f, 0.0f, 0.0f};
    masterModalData.probePoints[15] = {0.5f, 3.0f, 0.5f};
    masterModalData.probePoints[16] = {0.0f, 3.0f, 0.0f};
    masterModalData.probePoints[17] = {0.0f, 3.0f, 1.0f};
    masterModalData.probePoints[18] = {1.0f, 3.0f, 0.0f};

    PlayerModalState modalState = {};
    modalState.masterModalData = &masterModalData;
    modalState.modalStateCode = 4;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    zVec3 probeFaceVertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};
    std::int32_t faceIndices[3] = {0, 1, 2};
    zModel_MaterialPartial materialPayload = {};
    materialPayload.userTag = 4;
    zModel_PickFaceUvData faceUvData = {};
    zModel_PickFaceEntry faceEntry = {};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = faceIndices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload = static_cast<zModel_PickFaceScenePayload *>(static_cast<void *>(&materialPayload));
    faceEntry.variantTag.count = 1;
    faceEntry.variantTag.tags[0] = 0x42;
    zModel_PickFaceData faceData = {};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = probeFaceVertices;

    zClass_Object3DDataPartial objectData = {};
    objectData.flags = 8;
    zClass_NodePartial objectNode = {};
    objectNode.flags = 0x11c;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    objectNode.cachedBounds[0] = 0.0f;
    objectNode.cachedBounds[1] = 0.0f;
    objectNode.cachedBounds[2] = 0.0f;
    objectNode.cachedBounds[3] = 1.0f;
    objectNode.cachedBounds[4] = 1.0f;
    objectNode.cachedBounds[5] = 1.0f;
    zClass_NodePartial *areaChildren[1] = {&objectNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = areaRows;
    zClass_NodePartial world = {};
    world.classData = &worldData;
    g_Player_RuntimeDiScene = &world;

    Player::UpdateMasterTypeHover(&saveState);

    const bool ok = FloatNear(playerState.axisClampRuntime, 12.0f) && FloatNear(playerState.yawVelocityLimit, 2.5f) && FloatNear(playerState.projectileSpawnVel.y, 5.0f) &&
                    FloatNear(playerState.worldPos.x, 1.0f) && FloatNear(playerState.worldPos.y, 0.0f) && FloatNear(playerState.worldPos.z, 2.0f) &&
                    FloatNear(playerState.fxOffsetWorld.x, 1.25f) && FloatNear(playerState.fxOffsetWorld.y, 0.5f) && FloatNear(playerState.fxOffsetWorld.z, 2.75f) &&
                    FloatNear(playerState.bankBasis.z, -1.0f) && FloatNear(playerState.cachedPitchRad, playerState.vehiclePitchRad) &&
                    FloatNear(playerState.cachedYawRad, playerState.restartYawRad) && FloatNear(playerState.cachedRollRad, playerState.vehicleRollRad) &&
                    (hoverVariantNode.flags & 0x04) != 0;

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_FrameDeltaTimeSec = oldFrameDeltaTimeSec;
    g_Time_AccumulatedTimeSec = oldAccumulatedTimeSec;
    g_Player_MaxSlope = oldMaxSlope;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;

    return ok ? 0 : 1;
}

extern "C" int player_update_master_type_sub_smoke(
    void
) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldPlayerDeltaTimeScaled001 = g_Player_DeltaTimeScaled001;
    const float oldAccumulatedTimeSec = g_Time_AccumulatedTimeSec;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&identityMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage globalPlayerState = {};
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial globalRootNode = {};
    zClass_NodePartial attachmentNode = {};
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    playerState.rootNode = &rootNode;
    globalPlayerState.rootNode = &globalRootNode;
    playerState.motionBasis = identityMatrix;
    playerState.previousTransform = identityMatrix;
    playerState.worldPos = {0.0f, -1.0f, 0.0f};
    playerState.localVel = {1.0f, 0.0f, 1.0f};
    playerState.throttleInputCopy = 1.0f;
    playerState.fxOffsetLocal = {0.25f, 0.5f, 0.75f};
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x42;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;

    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState = static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&globalPlayerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    g_Player_DeltaTime = 0.25f;
    g_Player_DeltaTimeScaled001 = 0.0f;
    g_Time_AccumulatedTimeSec = 12.0f;
    g_VariantTag_Current.count = 0;
    g_VariantTag_Current.tags[0] = 0xff;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;

    PlayerMasterModalData masterModalData = {};
    masterModalData.masterType = 2;
    masterModalData.maxSpeed = 10.0f;
    masterModalData.yawRateMax = 3.5f;
    masterModalData.modeAltTransitionTime = 2.0f;
    masterModalData.probePoints[0] = {0.5f, 0.0f, 0.5f};
    masterModalData.probePoints[1] = {0.0f, 0.0f, 0.0f};
    masterModalData.probePoints[2] = {0.0f, 0.0f, 1.0f};
    masterModalData.probePoints[3] = {1.0f, 0.0f, 0.0f};
    masterModalData.probePoints[15] = {0.5f, 3.0f, 0.5f};
    masterModalData.probePoints[16] = {0.0f, 3.0f, 0.0f};
    masterModalData.probePoints[17] = {0.0f, 3.0f, 1.0f};
    masterModalData.probePoints[18] = {1.0f, 3.0f, 0.0f};

    PlayerModalState modalState = {};
    modalState.masterModalData = &masterModalData;
    modalState.modalStateCode = 4;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    zVec3 probeFaceVertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 10.0f}, {10.0f, 0.0f, 0.0f}};
    std::int32_t faceIndices[3] = {0, 1, 2};
    zModel_MaterialPartial materialPayload = {};
    materialPayload.userTag = 1;
    zModel_PickFaceUvData faceUvData = {};
    zModel_PickFaceEntry faceEntry = {};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = faceIndices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload = static_cast<zModel_PickFaceScenePayload *>(static_cast<void *>(&materialPayload));
    faceEntry.variantTag.count = 1;
    faceEntry.variantTag.tags[0] = 0x42;
    zModel_PickFaceData faceData = {};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = probeFaceVertices;

    zClass_Object3DDataPartial objectData = {};
    objectData.flags = 8;
    zClass_NodePartial objectNode = {};
    objectNode.flags = 0x11c;
    objectNode.auxFlags = 1;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    objectNode.callbackContext = &attachmentNode;
    objectNode.cachedBounds[0] = 0.0f;
    objectNode.cachedBounds[1] = 0.0f;
    objectNode.cachedBounds[2] = 0.0f;
    objectNode.cachedBounds[3] = 10.0f;
    objectNode.cachedBounds[4] = 1.0f;
    objectNode.cachedBounds[5] = 10.0f;
    zClass_NodePartial *areaChildren[1] = {&objectNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 10.0f;
    worldData.areaCellSizeZ = 10.0f;
    worldData.areaInvSizeX = 0.1f;
    worldData.areaInvSizeZ = 0.1f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = areaRows;
    zClass_NodePartial world = {};
    world.classData = &worldData;
    g_Player_RuntimeDiScene = &world;

    Player::UpdateMasterTypeSub(&saveState);

    int result = 0;
    if (!FloatNear(playerState.axisClampRuntime, 10.0f)) {
        result = 1;
    } else if (!FloatNear(playerState.projectileSpawnVel.x, 1.0f)) {
        result = 21;
    } else if (!FloatNear(playerState.projectileSpawnVel.y, 0.0f)) {
        result = 22;
    } else if (!FloatNear(playerState.projectileSpawnVel.z, 1.0f)) {
        result = 23;
    } else if (!FloatNear(playerState.worldPos.x, 0.25f) || !FloatNear(playerState.worldPos.y, 2.0f) || !FloatNear(playerState.worldPos.z, 0.25f)) {
        result = 3;
    } else if (!FloatNear(playerState.motionBasis.posX, 0.25f) || !FloatNear(playerState.motionBasis.posY, 2.0f) || !FloatNear(playerState.motionBasis.posZ, 0.25f)) {
        result = 4;
    } else if (!FloatNear(playerState.fxOffsetWorld.x, 0.5f) || !FloatNear(playerState.fxOffsetWorld.y, 2.5f) || !FloatNear(playerState.fxOffsetWorld.z, 1.0f)) {
        result = 5;
    } else if (!FloatNear(playerState.bankBasis.x, 0.0f) || !FloatNear(playerState.bankBasis.y, 0.0f) || !FloatNear(playerState.bankBasis.z, -1.0f)) {
        result = 6;
    } else if (!FloatNear(playerState.cachedPitchRad, playerState.vehiclePitchRad) || !FloatNear(playerState.cachedYawRad, playerState.restartYawRad) ||
               !FloatNear(playerState.cachedRollRad, playerState.vehicleRollRad)) {
        result = 7;
    } else if (!FloatNear(playerState.previousTransform.posX, rootData.localMatrix[9]) || !FloatNear(playerState.previousTransform.posY, rootData.localMatrix[10]) ||
               !FloatNear(playerState.previousTransform.posZ, rootData.localMatrix[11])) {
        result = 8;
    } else if (!FloatNear(playerState.subModeProbeBestHeight, 0.0f) || playerState.probeImpactSlot1SeenFlag != 1 || playerState.selectedProbeSample.node != &objectNode) {
        result = 9;
    }

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_DeltaTimeScaled001 = oldPlayerDeltaTimeScaled001;
    g_Time_AccumulatedTimeSec = oldAccumulatedTimeSec;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;

    return result;
}

extern "C" int player_update_master_type_track_smoke(
    void
) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const float oldDeltaTime = g_Player_DeltaTime;
    const float oldInvDeltaTime = g_Player_InvDeltaTime;

    g_GameStateOrMapTable = nullptr;
    g_Player_DeltaTime = 0.1f;
    g_Player_InvDeltaTime = 10.0f;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    PlayerMasterCommonData commonData = {};
    zClass_NodePartial rootNode = {};
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial modeVariantNode = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalData.masterType = 2;
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    playerState.modeVariantNode = &modeVariantNode;
    InitObjectPositionNode(&rootNode, &rootData, 0.0f, 0.0f, 0.0f);
    rootData.localMatrix[0] = 1.0f;
    rootData.localMatrix[4] = 1.0f;
    rootData.localMatrix[8] = 1.0f;

    playerState.airborneFlag = 1;
    playerState.airborneFlagPrev = 0;
    playerState.projectileSpawnVel = {10.0f, 2.0f, 20.0f};
    playerState.worldPos = {1.0f, 5.0f, 3.0f};
    playerState.fxOffsetLocal = {0.5f, 1.0f, 1.5f};

    Player::UpdateMasterTypeTrack(&saveState);

    const float damping = PlayerDampingFactor(0.200000003f, 0.1f);
    const float expectedX = 10.0f * damping;
    const float expectedZ = 20.0f * damping;
    const bool ok = playerState.airborneFlagPrev == 1 && FloatNear(playerState.projectileSpawnVel.x, expectedX) && FloatNear(playerState.projectileSpawnVel.y, 2.0f) &&
                    FloatNear(playerState.projectileSpawnVel.z, expectedZ) && FloatNear(playerState.worldPos.x, 1.0f + expectedX * 0.1f) &&
                    FloatNear(playerState.worldPos.y, 5.0f) && FloatNear(playerState.worldPos.z, 3.0f + expectedZ * 0.1f) &&
                    FloatNear(playerState.motionBasis.posX, playerState.worldPos.x) && FloatNear(playerState.motionBasis.posY, playerState.worldPos.y) &&
                    FloatNear(playerState.motionBasis.posZ, playerState.worldPos.z) && FloatNear(playerState.fxOffsetWorld.x, playerState.worldPos.x + 0.5f) &&
                    FloatNear(playerState.fxOffsetWorld.y, playerState.worldPos.y + 1.0f) && FloatNear(playerState.fxOffsetWorld.z, playerState.worldPos.z + 1.5f) &&
                    FloatNear(playerState.previousTransform.posX, rootData.localMatrix[9]) && FloatNear(playerState.previousTransform.posY, rootData.localMatrix[10]) &&
                    FloatNear(playerState.previousTransform.posZ, rootData.localMatrix[11]);

    g_Player_InvDeltaTime = oldInvDeltaTime;
    g_Player_DeltaTime = oldDeltaTime;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    return ok ? 0 : 1;
}

extern "C" int player_update_sub_mode_water_probe_state_smoke(
    void
) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldPlayerDeltaTimeScaled001 = g_Player_DeltaTimeScaled001;
    const float oldAccumulatedTimeSec = g_Time_AccumulatedTimeSec;
    const int oldHorizonFollow = g_Player_HorizonNodeFollowCameraEnabled;
    const Player_UnderwaterFxPass3Ui oldUnderwaterFx = g_Player_UnderwaterFxPass3Ui;
    const std::uint32_t oldHudInvalidateMask = g_HudUi_InvalidateMask;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&identityMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    g_Player_UnderwaterFxPass3Ui.SetVisible(0);

    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage globalPlayerState = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial globalRootNode = {};
    zClass_NodePartial attachmentNode = {};
    playerState.rootNode = &rootNode;
    globalPlayerState.rootNode = &globalRootNode;
    playerState.motionBasis = identityMatrix;
    playerState.worldPos = {0.0f, -1.0f, 0.0f};
    playerState.localVel.z = 4.0f;
    playerState.vehiclePitchRad = 0.25f;
    playerState.vehicleRollRad = 0.5f;
    playerState.underwaterFxEnabled = 1;
    playerState.cameraTarget = {0.0f, -0.5f, 0.0f};
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x42;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;

    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    gameStateOrMap.playerState = static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&globalPlayerState));
    g_GameStateOrMapTable = &gameStateOrMap;
    g_Player_DeltaTime = 0.25f;
    g_Player_DeltaTimeScaled001 = 0.0f;
    g_Time_AccumulatedTimeSec = 12.0f;
    g_Player_HorizonNodeFollowCameraEnabled = 1;
    g_VariantTag_Current.count = 0;
    g_VariantTag_Current.tags[0] = 0xff;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;

    PlayerMasterModalData masterModalData = {};
    masterModalData.masterType = 2;
    masterModalData.yawRateMax = 7.0f;
    masterModalData.modeAltTransitionTime = 2.0f;
    masterModalData.probePoints[0] = {0.5f, 0.0f, 0.5f};
    masterModalData.probePoints[1] = {0.0f, 0.0f, 0.0f};
    masterModalData.probePoints[2] = {0.0f, 0.0f, 1.0f};
    masterModalData.probePoints[3] = {1.0f, 0.0f, 0.0f};
    masterModalData.probePoints[15] = {0.5f, 3.0f, 0.5f};
    masterModalData.probePoints[16] = {0.0f, 3.0f, 0.0f};
    masterModalData.probePoints[17] = {0.0f, 3.0f, 1.0f};
    masterModalData.probePoints[18] = {1.0f, 3.0f, 0.0f};

    PlayerModalState modalState = {};
    modalState.masterModalData = &masterModalData;
    modalState.modalStateCode = 4;
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    zVec3 probeFaceVertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};
    std::int32_t faceIndices[3] = {0, 1, 2};
    zModel_MaterialPartial materialPayload = {};
    materialPayload.userTag = 1;
    zModel_PickFaceUvData faceUvData = {};
    zModel_PickFaceEntry faceEntry = {};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = faceIndices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload = static_cast<zModel_PickFaceScenePayload *>(static_cast<void *>(&materialPayload));
    faceEntry.variantTag.count = 1;
    faceEntry.variantTag.tags[0] = 0x42;
    zModel_PickFaceData faceData = {};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = probeFaceVertices;

    zClass_Object3DDataPartial objectData = {};
    objectData.flags = 8;
    zClass_NodePartial objectNode = {};
    objectNode.flags = 0x11c;
    objectNode.auxFlags = 1;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    objectNode.callbackContext = &attachmentNode;
    objectNode.cachedBounds[0] = 0.0f;
    objectNode.cachedBounds[1] = 0.0f;
    objectNode.cachedBounds[2] = 0.0f;
    objectNode.cachedBounds[3] = 1.0f;
    objectNode.cachedBounds[4] = 1.0f;
    objectNode.cachedBounds[5] = 1.0f;
    zClass_NodePartial *areaChildren[1] = {&objectNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = areaRows;
    zClass_NodePartial world = {};
    world.classData = &worldData;
    g_Player_RuntimeDiScene = &world;

    Player::UpdateSubModeWaterProbeState(&saveState);

    int result = 0;
    const float expectedRollDamping = -(PlayerDampingFactor(1.0f, 0.25f) * 0.5f);
    if (!FloatNear(playerState.yawVelocityLimit, 7.0f)) {
        result = 1;
    } else if (!FloatNear(playerState.subModeProbeBestHeight, 0.0f)) {
        result = 2;
    } else if (!FloatNear(playerState.worldPos.y, 2.0f) || !FloatNear(playerState.motionBasis.posY, 2.0f)) {
        result = 3;
    } else if (!FloatNear(playerState.angVelRoll, expectedRollDamping)) {
        result = 4;
    } else if (!FloatNear(playerState.vehiclePitchRad, 0.25f) || !FloatNear(playerState.vehicleRollRad, 0.5f)) {
        result = 5;
    } else if ((g_Player_UnderwaterFxPass3Ui.flags & 0x10u) != 0 || g_Player_HorizonNodeFollowCameraEnabled != 0) {
        result = 6;
    } else if (playerState.probeImpactSlot1SeenFlag != 1 || playerState.selectedProbeSample.node != &objectNode || playerState.variantTag.tags[0] != 0x42) {
        result = 7;
    }

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_DeltaTimeScaled001 = oldPlayerDeltaTimeScaled001;
    g_Time_AccumulatedTimeSec = oldAccumulatedTimeSec;
    g_Player_HorizonNodeFollowCameraEnabled = oldHorizonFollow;
    g_Player_UnderwaterFxPass3Ui = oldUnderwaterFx;
    g_HudUi_InvalidateMask = oldHudInvalidateMask;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;

    return result;
}
