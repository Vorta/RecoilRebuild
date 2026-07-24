#include "Battlesport/player.h"
#include "Battlesport/game_net.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" std::uint32_t g_HudUi_InvalidateMask;

namespace {

template <typename T>
zZbdSectionCallback TestZbdCallbackPtr(T callback) {
    static_assert(sizeof(T) == sizeof(zZbdSectionCallback));
    union {
        T callback;
        zZbdSectionCallback raw;
    } value = {};
    value.callback = callback;
    return value.raw;
}

template <typename T>
T &TestFieldAt(void *base, std::size_t offset) {
    return *reinterpret_cast<T *>(static_cast<unsigned char *>(base) + offset);
}

zZbdManager MakePlayerZbdManager(zZbdSectionHandlerNode &sentinel) {
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    sentinel.sectionHandler = {};

    zZbdManager manager = {};
    manager.sectionHandlerListSentinel = &sentinel;
    return manager;
}

void ClearPlayerRegisteredHandlers(zZbdSectionHandlerNode &sentinel) {
    zZbdSectionHandlerNode *node = sentinel.next;
    while (node != &sentinel) {
        zZbdSectionHandlerNode *const next = node->next;
        delete node;
        node = next;
    }
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
}

bool Vec3Equals(const zVec3 &value, const zVec3 &expected) {
    return value.x == expected.x && value.y == expected.y && value.z == expected.z;
}

bool FloatNear(float actual, float expected) {
    return actual > expected - 0.0001f && actual < expected + 0.0001f;
}

void InitPlayerZarShieldWidget(HudUiShieldMessageWidget &shield) {
    new (&shield.widget) HudUiWidget(0);
    new (&shield.percentTextPanel) HudUiPanelSimple;
    new (&shield.meter) HudUiShieldMeterCandidate;
    shield.meter.fillPixelsMax = 20;
    shield.meter.points[1].y = 100.0f;
}

void RestoreVehicleReadGlobals(
    zUtil_SaveGameState *oldHead,
    zInput_GameStateOrMapTablePartial *oldGameState,
    float oldAccumulatedTime,
    const zTag4Partial &oldVariantTagCurrent,
    const zTag4Partial &oldVariantCurrent,
    int oldBuiltinTexturePackCount,
    zVidTexturePackEntry *oldBuiltinTexturePacks,
    int oldTexDirEntryCount
) {
    g_PlayerSaveStateListHead = oldHead;
    g_GameStateOrMapTable = oldGameState;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    g_VariantTag_Current = oldVariantTagCurrent;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_zVid_BuiltinTexturePackCount = oldBuiltinTexturePackCount;
    g_zVid_BuiltinTexturePacks = oldBuiltinTexturePacks;
    g_zImage_TexDirEntryCount = oldTexDirEntryCount;
}

}  // namespace

extern "C" int player_zar_read_mission_save_data_section_smoke(void) {
    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable =
        g_GameStateOrMapTable;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    HudUiCounterTextPanel *const oldObjectiveCounter = g_HudUiMgrObjectiveCounterTextPanel;
    HudLayoutBase *const oldCurrentLayout = g_HudUiMgrCurrentLayout;
    const zTag4Partial oldLastValidCameraVariantTag = g_Player_LastValidCameraVariantTag;
    const int oldActiveWeaponMessageIndex = g_HudUiMgrActiveWeaponMessageIndex;
    const int oldActiveWeaponSideIndex = g_HudUiMgrActiveWeaponSideIndex;
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;
    HudUiNanitePanel oldNanitePanel = g_HudUiMgrNanitePanel;
    HudUiCounter oldModeCounters[4] = {};
    HudUiMessage oldMessages[10] = {};
    for (int index = 0; index < 10; ++index) {
        oldMessages[index] = g_HudUiMgrMessages[index];
    }
    for (int index = 0; index < 4; ++index) {
        oldModeCounters[index] = g_HudUiMgrModeCounters[index];
    }
    PlayerNodeFlagRestoreEntry *const oldRestoreBegin = g_PlayerNodeFlagRestoreEntriesBegin;
    PlayerNodeFlagRestoreEntry *const oldRestoreEnd = g_PlayerNodeFlagRestoreEntriesEnd;
    PlayerNodeFlagRestoreEntry *const oldRestoreCapacity =
        g_PlayerNodeFlagRestoreEntriesCapacityEnd;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    PlayerMasterModalData flyModalData = {};
    PlayerModalState flyModal = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial cameraNode = {};
    zClass_CameraDataPartial cameraData = {};
    HudUiShieldMessageWidget shield = {};
    HudUiCounterTextPanel counter = {};
    zVidImagePartial messageImages[10][4] = {};
    zVidImagePartial modeImages[6] = {};
    zClass_NodePartial restoreNode = {};
    PlayerNodeFlagRestoreEntry restoreEntries[1] = {};

    rootNode.classId = 5;
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    commonData.maxHealth = 100.0f;
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    playerState.statusMeterValue = 25.0f;
    playerState.nanitePanelLevel = 1;
    playerState.lifecycleState = 1;

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        PlayerAltWeaponBank &bank = playerState.altWeaponBanks[bankIndex];
        bank.controllerA.weaponBankIndex = bankIndex;
        bank.controllerA.weaponSideIndex = 0;
        bank.controllerB.weaponBankIndex = bankIndex;
        bank.controllerB.weaponSideIndex = 1;

        HudUiMessage &message = g_HudUiMgrMessages[bankIndex];
        std::memset(&message, 0, sizeof(message));
        new (&message) HudUiMessage;
        message.variantImages[0] = &messageImages[bankIndex][0];
        message.variantImages[1] = &messageImages[bankIndex][1];
        message.variantImages[4] = &messageImages[bankIndex][2];
        message.sideImageSwaps[0] = &messageImages[bankIndex][2];
        message.sideImageSwaps[1] = &messageImages[bankIndex][3];
    }

    playerState.activeAltGunController = &playerState.altWeaponBanks[3].controllerB;
    playerState.activePrimaryGunController = &playerState.altWeaponBanks[4].controllerA;

    flyModalData.masterType = 1;
    flyModal.masterModalData = &flyModalData;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &flyModal;
    saveState.modalStateListHead = &flyModal;

    InitPlayerZarShieldWidget(shield);

    std::memset(&g_HudUiMgrNanitePanel, 0, sizeof(g_HudUiMgrNanitePanel));
    new (&g_HudUiMgrNanitePanel) HudUiNanitePanel;
    for (int index = 1; index < 4; ++index) {
        std::memset(&g_HudUiMgrModeCounters[index], 0, sizeof(g_HudUiMgrModeCounters[index]));
        new (&g_HudUiMgrModeCounters[index]) HudUiCounter;
        TestFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[index], 0xbc) =
            &modeImages[(index - 1) * 2];
        TestFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[index], 0xc0) =
            &modeImages[(index - 1) * 2 + 1];
    }

    new (&counter) HudUiCounterTextPanel;

    restoreEntries[0].node = &restoreNode;
    restoreEntries[0].wasPickable = 1;
    g_PlayerNodeFlagRestoreEntriesBegin = restoreEntries;
    g_PlayerNodeFlagRestoreEntriesEnd = restoreEntries + 1;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = restoreEntries + 1;

    PlayerMissionSaveData saveData = {};
    saveData.size = sizeof(saveData);
    saveData.altWeaponBankIndex = 3;
    saveData.altWeaponSideIndex = 1;
    saveData.primaryWeaponBankIndex = 4;
    saveData.primaryWeaponSideIndex = 0;
    saveData.hudCounterValue = 12;
    saveData.playerStatusMeterRatio = 0.25f;
    saveData.amphibUnlocked = 1;
    saveData.hoverUnlocked = 1;
    saveData.subUnlocked = 0;
    saveData.playerMasterType = 1;
    saveData.cameraTarget = {1.0f, 2.0f, 3.0f};
    saveData.cameraPosition = {4.0f, 5.0f, 6.0f};
    saveData.lastValidCameraVariantTag.count = 2;
    saveData.lastValidCameraVariantTag.tags[0] = 9;
    saveData.lastValidCameraVariantTag.tags[1] = 10;
    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        saveData.weaponBank[bankIndex].selectedSide = bankIndex & 1;
        saveData.weaponBank[bankIndex].sides[0].enabled = 1;
        saveData.weaponBank[bankIndex].sides[0].ammoOrCharge = 3.0f;
        saveData.weaponBank[bankIndex].sides[1].enabled = 1;
        saveData.weaponBank[bankIndex].sides[1].ammoOrCharge = 4.0f;
    }

    g_LocalPlayerSaveState = &saveState;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    g_MainCamera = &cameraNode;
    g_HudUiMgrShieldMessageWidget = &shield;
    g_HudUiMgrObjectiveCounterTextPanel = &counter;
    g_HudUiMgrCurrentLayout = nullptr;
    g_HudUiMgrActiveWeaponMessageIndex = 0;
    g_HudUiMgrActiveWeaponSideIndex = 0;
    g_HudUi_InvalidateMask = 0x80;

    Player::ZAR_ReadMissionSaveDataSection(nullptr, nullptr, &saveData, sizeof(saveData),
                                           nullptr);

    int failure = 0;
    if (g_Player_LastValidCameraVariantTag.count != 2 ||
        g_Player_LastValidCameraVariantTag.tags[0] != 9 ||
        g_Player_LastValidCameraVariantTag.tags[1] != 10) {
        failure = 1;
    } else if (playerState.activeAltGunController !=
                   &playerState.altWeaponBanks[3].controllerB ||
               playerState.activePrimaryGunController !=
                   &playerState.altWeaponBanks[4].controllerA) {
        failure = 2;
    } else if (g_HudUiMgrNanitePanel.visibleCount != 1) {
        failure = 3;
    } else if (shield.meter.points[0].y != 95.0f) {
        failure = 4;
    } else if ((restoreNode.flags & 0x20) == 0) {
        failure = 5;
    }

    g_LocalPlayerSaveState = oldLocalSaveState;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_MainCamera = oldMainCamera;
    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_HudUiMgrObjectiveCounterTextPanel = oldObjectiveCounter;
    g_HudUiMgrCurrentLayout = oldCurrentLayout;
    g_Player_LastValidCameraVariantTag = oldLastValidCameraVariantTag;
    g_HudUiMgrActiveWeaponMessageIndex = oldActiveWeaponMessageIndex;
    g_HudUiMgrActiveWeaponSideIndex = oldActiveWeaponSideIndex;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    g_HudUiMgrNanitePanel = oldNanitePanel;
    for (int index = 0; index < 10; ++index) {
        g_HudUiMgrMessages[index] = oldMessages[index];
    }
    for (int index = 0; index < 4; ++index) {
        g_HudUiMgrModeCounters[index] = oldModeCounters[index];
    }
    g_PlayerNodeFlagRestoreEntriesBegin = oldRestoreBegin;
    g_PlayerNodeFlagRestoreEntriesEnd = oldRestoreEnd;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = oldRestoreCapacity;
    return failure;
}

extern "C" int player_refresh_hud_from_state_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState localSaveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage localPlayerState = {};
    PlayerMasterCommonData localCommonData = {};
    HudUiShieldMessageWidget shield = {};
    zVidImagePartial images[40] = {};

    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;
    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    HudUiNanitePanel oldNanitePanel = g_HudUiMgrNanitePanel;
    HudUiCounter oldModeCounters[4] = {};
    HudUiMessage oldMessages[10] = {};
    const int oldActiveWeaponMessageIndex = g_HudUiMgrActiveWeaponMessageIndex;
    const int oldActiveWeaponSideIndex = g_HudUiMgrActiveWeaponSideIndex;
    for (int index = 0; index < 10; ++index) {
        oldMessages[index] = g_HudUiMgrMessages[index];
    }
    for (int index = 0; index < 4; ++index) {
        oldModeCounters[index] = g_HudUiMgrModeCounters[index];
    }

    saveState.playerState = &playerState;
    localSaveState.playerState = &localPlayerState;
    localPlayerState.masterCommonData = &localCommonData;
    localCommonData.maxHealth = 100.0f;
    playerState.statusMeterValue = 25.0f;
    playerState.nanitePanelLevel = 3;
    playerState.amphibUnlocked = 1;
    playerState.hoverUnlocked = 0;
    playerState.subUnlocked = 1;

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        HudUiMessage &message = g_HudUiMgrMessages[bankIndex];
        std::memset(&message, 0, sizeof(message));
        new (&message) HudUiMessage;
        message.variantImages[0] = &images[bankIndex * 4 + 0];
        message.variantImages[1] = &images[bankIndex * 4 + 1];
        message.variantImages[4] = &images[bankIndex * 4 + 2];
        message.sideImageSwaps[0] = &images[bankIndex * 4 + 2];
        message.sideImageSwaps[1] = &images[bankIndex * 4 + 3];
    }

    playerState.altWeaponBanks[0].controllerA.flags = 4;
    playerState.altWeaponBanks[0].controllerA.ammoOrCharge = 5.0f;
    playerState.altWeaponBanks[0].controllerB.flags = 4;
    playerState.altWeaponBanks[0].controllerB.ammoOrCharge = 7.0f;

    playerState.altWeaponBanks[1].controllerB.flags = 4;
    playerState.altWeaponBanks[1].controllerB.ammoOrCharge = 9.0f;

    playerState.altWeaponBanks[2].controllerA.ammoOrCharge = 4.0f;

    playerState.altWeaponBanks[3].controllerA.flags = 4;
    playerState.altWeaponBanks[3].controllerB.flags = 4;
    playerState.altWeaponBanks[3].controllerB.ammoOrCharge = 11.0f;

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        playerState.altWeaponBanks[bankIndex].controllerA.weaponBankIndex = bankIndex;
        playerState.altWeaponBanks[bankIndex].controllerA.weaponSideIndex = 0;
        playerState.altWeaponBanks[bankIndex].controllerB.weaponBankIndex = bankIndex;
        playerState.altWeaponBanks[bankIndex].controllerB.weaponSideIndex = 1;
    }
    playerState.activeAltGunController = &playerState.altWeaponBanks[3].controllerB;
    playerState.activePrimaryGunController = &playerState.altWeaponBanks[1].controllerB;

    InitPlayerZarShieldWidget(shield);
    g_HudUiMgrShieldMessageWidget = &shield;

    std::memset(&g_HudUiMgrNanitePanel, 0, sizeof(g_HudUiMgrNanitePanel));
    new (&g_HudUiMgrNanitePanel) HudUiNanitePanel;
    zVidImagePartial counterImages[6] = {};
    for (int index = 1; index < 4; ++index) {
        std::memset(&g_HudUiMgrModeCounters[index], 0, sizeof(g_HudUiMgrModeCounters[index]));
        new (&g_HudUiMgrModeCounters[index]) HudUiCounter;
        TestFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[index], 0xbc) =
            &counterImages[(index - 1) * 2];
        TestFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[index], 0xc0) =
            &counterImages[(index - 1) * 2 + 1];
    }

    g_GameStateOrMapTable =
        reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&localSaveState);
    g_HudUi_InvalidateMask = 0x80;

    Player::RefreshHudFromState(&saveState);

    const bool shieldOk =
        shield.meter.color565 == (zVid_PackColorRGB(255, 255, 0) & 0xffffu) &&
        shield.meter.points[0].y == 95.0f && shield.meter.points[3].y == 95.0f &&
        std::strcmp(&TestFieldAt<char>(&shield.percentTextPanel, 0x34), "25") == 0;
    const bool naniteOk = g_HudUiMgrNanitePanel.visibleCount == 3;
    const bool bank0Ok =
        playerState.altWeaponBanks[0].selectedSide == 0 &&
        g_HudUiMgrMessages[0].image == &images[0] &&
        g_HudUiMgrMessages[0].widget.image == &images[3] &&
        std::strcmp(&TestFieldAt<char>(&g_HudUiMgrMessages[0].panel, 0x34), "5") == 0;
    const bool bank1Ok =
        playerState.altWeaponBanks[1].selectedSide == 1 &&
        g_HudUiMgrMessages[1].image == &images[6] &&
        g_HudUiMgrMessages[1].widget.image == nullptr &&
        std::strcmp(&TestFieldAt<char>(&g_HudUiMgrMessages[1].panel, 0x34), "9") == 0;
    const bool bank2Ok =
        g_HudUiMgrMessages[2].image == nullptr &&
        std::strcmp(&TestFieldAt<char>(&g_HudUiMgrMessages[2].panel, 0x34), "4") == 0;
    const bool activeOk =
        playerState.altWeaponBanks[3].selectedSide == 1 &&
        g_HudUiMgrMessages[3].image == &images[14] &&
        std::strcmp(&TestFieldAt<char>(&g_HudUiMgrMessages[3].panel, 0x34), "11") == 0 &&
        g_HudUiMgrActiveWeaponMessageIndex == 3 && g_HudUiMgrActiveWeaponSideIndex == 1;
    const bool modesOk =
        reinterpret_cast<HudUiWidget *>(&g_HudUiMgrModeCounters[1])->image ==
            &counterImages[1] &&
        reinterpret_cast<HudUiWidget *>(&g_HudUiMgrModeCounters[2])->image ==
            &counterImages[2] &&
        reinterpret_cast<HudUiWidget *>(&g_HudUiMgrModeCounters[3])->image ==
            &counterImages[5];

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_HudUiMgrNanitePanel = oldNanitePanel;
    g_HudUiMgrActiveWeaponMessageIndex = oldActiveWeaponMessageIndex;
    g_HudUiMgrActiveWeaponSideIndex = oldActiveWeaponSideIndex;
    for (int index = 0; index < 10; ++index) {
        g_HudUiMgrMessages[index] = oldMessages[index];
    }
    for (int index = 0; index < 4; ++index) {
        g_HudUiMgrModeCounters[index] = oldModeCounters[index];
    }

    if (!shieldOk) {
        return 1;
    }
    if (!naniteOk) {
        return 2;
    }
    if (!bank0Ok) {
        return 3;
    }
    if (!bank1Ok) {
        return 4;
    }
    if (!bank2Ok) {
        return 5;
    }
    if (!activeOk) {
        return 6;
    }
    if (!modesOk) {
        return 7;
    }
    return 0;
}

extern "C" int player_zar_write_mission_save_data_section_smoke(void) {
    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    GetTempPathA(sizeof(tempPath), tempPath);
    GetTempFileNameA(tempPath, "pms", 0, tempFile);

    HANDLE const file =
        CreateFileA(tempFile, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 1;
    }

    zZbdManager manager = {};
    manager.indexArchive.hFile = file;

    zZbdSectionHandler handler = {};
    handler.sectionName = "Player";
    zZbdSectionCallbackCtx callbackCtx = {};
    callbackCtx.manager = &manager;
    callbackCtx.sectionHandler = &handler;

    zClass_NodePartial rootNode = {};
    std::strcpy(rootNode.name, "local_player");
    zClass_NodePartial cameraNode = {};
    zClass_CameraDataPartial cameraData = {};
    PlayerMasterModalData modalData = {};
    PlayerModalState modalState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    OptCatalogEntryDef hitSource = {};

    playerState.rootNode = &rootNode;
    playerState.activeAltGunController = &playerState.altWeaponBanks[1].controllerA;
    playerState.activePrimaryGunController = &playerState.altWeaponBanks[2].controllerB;
    playerState.altWeaponBanks[1].controllerA.weaponBankIndex = 1;
    playerState.altWeaponBanks[1].controllerA.weaponSideIndex = 0;
    playerState.altWeaponBanks[2].controllerB.weaponBankIndex = 2;
    playerState.altWeaponBanks[2].controllerB.weaponSideIndex = 1;
    playerState.timedHitStatus.runtimeFlags = 1;
    playerState.timedHitStatus.hitSource = &hitSource;
    playerState.timedHitStatus.nextUpdateTime = 100.0f;
    hitSource.ordinalIndex = 88;

    modalData.masterType = 66;
    modalState.masterModalData = &modalData;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    cameraData.targetOrEuler = {1.0f, 2.0f, 3.0f};
    cameraData.posOffset = {4.0f, 5.0f, 6.0f};

    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    const zTag4Partial oldLastValidCameraVariantTag = g_Player_LastValidCameraVariantTag;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    g_LocalPlayerSaveState = &saveState;
    g_MainCamera = &cameraNode;
    g_Player_LastValidCameraVariantTag.count = 2;
    g_Player_LastValidCameraVariantTag.tags[0] = 7;
    g_Player_LastValidCameraVariantTag.tags[1] = 8;
    g_Player_LastValidCameraVariantTag.tags[2] = 0xff;
    g_Time_AccumulatedTimeSec = 90.0f;

    const int result = Player::ZAR_WriteMissionSaveDataSection(&callbackCtx, nullptr);

    PlayerMissionSaveData readBack = {};
    DWORD read = 0;
    SetFilePointer(file, 0, nullptr, FILE_BEGIN);
    ReadFile(file, &readBack, sizeof(readBack), &read, nullptr);

    const bool ok = result == 1 && read == sizeof(readBack) &&
                    manager.indexArchive.recordCount == 1 &&
                    manager.indexArchive.records != nullptr &&
                    std::strcmp(manager.indexArchive.records[0].name,
                                "Player/local_player") == 0 &&
                    readBack.size == sizeof(PlayerMissionSaveData) &&
                    readBack.primaryWeaponBankIndex == 2 &&
                    readBack.primaryWeaponSideIndex == 1 &&
                    readBack.playerMasterType == 66 &&
                    Vec3Equals(readBack.cameraTarget, cameraData.targetOrEuler) &&
                    Vec3Equals(readBack.cameraPosition, cameraData.posOffset) &&
                    readBack.lastValidCameraVariantTag.count == 2 &&
                    readBack.lastValidCameraVariantTag.tags[0] == 7 &&
                    readBack.lastValidCameraVariantTag.tags[1] == 8 &&
                    readBack.lastValidCameraVariantTag.tags[2] == 0xff &&
                    readBack.timedHitStatus.savedHitSourceEntryId == 88 &&
                    readBack.timedHitStatus.nextUpdateTime == 10.0f;

    g_LocalPlayerSaveState = oldLocalSaveState;
    g_MainCamera = oldMainCamera;
    g_Player_LastValidCameraVariantTag = oldLastValidCameraVariantTag;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    std::free(manager.indexArchive.records);
    manager.indexArchive.records = nullptr;
    CloseHandle(file);
    return ok ? 0 : 2;
}

extern "C" int player_zar_register_sections_smoke(void) {
    zZbdManager *const oldManager = g_zUtil_ZbdManager;
    const int oldRuntimeInputFlags = g_Player_RuntimeInputFlags;

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakePlayerZbdManager(sentinel);
    g_zUtil_ZbdManager = &manager;
    g_Player_RuntimeInputFlags = 99;

    Player::ZAR_RegisterSections();

    zZbdSectionHandlerNode *const vehicleNode = sentinel.next;
    zZbdSectionHandlerNode *const playerNode =
        vehicleNode != &sentinel ? vehicleNode->next : &sentinel;
    const bool ok =
        g_Player_RuntimeInputFlags == 0 && manager.sectionHandlerCount == 2 &&
        vehicleNode != &sentinel && playerNode != &sentinel && playerNode->next == &sentinel &&
        std::strcmp(vehicleNode->sectionHandler.sectionName, "VehicleList") == 0 &&
        vehicleNode->sectionHandler.onPreLoad ==
            TestZbdCallbackPtr(&Player::ZAR_WriteVehicleListSection) &&
        vehicleNode->sectionHandler.onDataReady ==
            TestZbdCallbackPtr(&Player::ZAR_ReadVehicleListSection) &&
        vehicleNode->sectionHandler.sortOrder == 100 &&
        vehicleNode->sectionHandler.userData == nullptr &&
        std::strcmp(playerNode->sectionHandler.sectionName, "Player") == 0 &&
        playerNode->sectionHandler.onPreLoad ==
            TestZbdCallbackPtr(&Player::ZAR_WriteMissionSaveDataSection) &&
        playerNode->sectionHandler.onDataReady ==
            TestZbdCallbackPtr(&Player::ZAR_ReadMissionSaveDataSection) &&
        playerNode->sectionHandler.sortOrder == 200 &&
        playerNode->sectionHandler.userData == nullptr;

    ClearPlayerRegisteredHandlers(sentinel);
    g_zUtil_ZbdManager = oldManager;
    g_Player_RuntimeInputFlags = oldRuntimeInputFlags;
    return ok ? 0 : 1;
}

extern "C" int player_zar_write_vehicle_list_section_smoke(void) {
    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    GetTempPathA(sizeof(tempPath), tempPath);
    GetTempFileNameA(tempPath, "pvl", 0, tempFile);

    HANDLE const file =
        CreateFileA(tempFile, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 1;
    }

    zZbdManager manager = {};
    manager.indexArchive.hFile = file;

    zZbdSectionHandler handler = {};
    handler.sectionName = "VehicleList";
    zZbdSectionCallbackCtx callbackCtx = {};
    callbackCtx.manager = &manager;
    callbackCtx.sectionHandler = &handler;

    zClass_NodePartial rootNode = {};
    std::strcpy(rootNode.name, "local_vehicle");

    PlayerMasterModalData modalData = {};
    modalData.masterType = 77;
    PlayerModalState modalState = {};
    modalState.masterModalData = &modalData;

    zUtil_PlayerStateStorage playerState = {};
    playerState.rootNode = &rootNode;
    playerState.vehicleRotationAngles = {1.0f, 2.0f, 3.0f};
    playerState.worldPos = {4.0f, 5.0f, 6.0f};
    playerState.aiNetId = 1001;
    playerState.aiTopLevelState = 11;
    playerState.aiSavedTopLevelState = 12;
    playerState.aiReturnTopLevelState = 13;
    playerState.aiAttackRadiusSq = 14.0f;
    playerState.aiRestoreDistanceSq = 15.0f;
    playerState.aiRestoreTarget = {16.0f, 17.0f, 18.0f};
    playerState.aiDynamicOffsetDir = {19.0f, 20.0f, 21.0f};
    playerState.aiActivationRadiusSq = 22.0f;
    playerState.aiTickSuppressed = 23;
    playerState.recentHitFlag = 24;
    playerState.recentHitMarkerHandle = 25;
    playerState.aiActive = 26;
    playerState.aiPathCursorAdvanceRequested = 27;
    playerState.aiCurrentSteeringSubstate = 28;
    playerState.aiReturnSteeringSubstate = 29;
    playerState.masterType = 30;
    playerState.statusMeterScaled = 31.0f;
    playerState.statusMeterValue = 32.0f;
    playerState.nanitePanelLevel = 33;

    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    zUtil_SaveGameState *const oldHead = g_PlayerSaveStateListHead;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    g_PlayerSaveStateListHead = &saveState;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);

    const int result = Player::ZAR_WriteVehicleListSection(&callbackCtx, nullptr);

    PlayerVehicleListSaveEntry readBack = {};
    DWORD read = 0;
    SetFilePointer(file, 0, nullptr, FILE_BEGIN);
    ReadFile(file, &readBack, sizeof(readBack), &read, nullptr);

    const bool ok = result == 1 && read == sizeof(readBack) &&
                    manager.indexArchive.recordCount == 1 &&
                    manager.indexArchive.records != nullptr &&
                    std::strcmp(manager.indexArchive.records[0].name,
                                "VehicleList/local_vehicle") == 0 &&
                    readBack.size == 128 &&
                    Vec3Equals(readBack.vehicleRotationAngles,
                               playerState.vehicleRotationAngles) &&
                    Vec3Equals(readBack.worldPos, playerState.worldPos) &&
                    readBack.aiNetId == playerState.aiNetId &&
                    readBack.aiTopLevelState == playerState.aiTopLevelState &&
                    readBack.aiSavedTopLevelState == playerState.aiSavedTopLevelState &&
                    readBack.aiReturnTopLevelState == playerState.aiReturnTopLevelState &&
                    readBack.aiAttackRadiusSq == playerState.aiAttackRadiusSq &&
                    readBack.aiRestoreDistanceSq == playerState.aiRestoreDistanceSq &&
                    Vec3Equals(readBack.aiRestoreTarget, playerState.aiRestoreTarget) &&
                    Vec3Equals(readBack.aiDynamicOffsetDir, playerState.aiDynamicOffsetDir) &&
                    readBack.aiActivationRadiusSq == playerState.aiActivationRadiusSq &&
                    readBack.aiTickSuppressed == playerState.aiTickSuppressed &&
                    readBack.aiAlertFlag == playerState.recentHitFlag &&
                    readBack.aiStateMarkerHandle == playerState.recentHitMarkerHandle &&
                    readBack.aiActive == playerState.aiActive &&
                    readBack.aiPathCursorAdvanceRequested ==
                        playerState.aiPathCursorAdvanceRequested &&
                    readBack.aiCurrentSteeringSubstate ==
                        playerState.aiCurrentSteeringSubstate &&
                    readBack.aiReturnSteeringSubstate ==
                        playerState.aiReturnSteeringSubstate &&
                    readBack.masterType == playerState.masterType &&
                    readBack.statusMeterScaled == playerState.statusMeterScaled &&
                    readBack.statusMeterValue == playerState.statusMeterValue &&
                    readBack.nanitePanelLevel == playerState.nanitePanelLevel &&
                    readBack.localMasterType == modalData.masterType;

    g_PlayerSaveStateListHead = oldHead;
    g_GameStateOrMapTable = oldGameState;
    std::free(manager.indexArchive.records);
    manager.indexArchive.records = nullptr;
    CloseHandle(file);
    return ok ? 0 : 2;
}

extern "C" int player_zar_read_vehicle_list_section_smoke(void) {
    zUtil_SaveGameState *const oldHead = g_PlayerSaveStateListHead;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const int oldBuiltinTexturePackCount = g_zVid_BuiltinTexturePackCount;
    zVidTexturePackEntry *const oldBuiltinTexturePacks = g_zVid_BuiltinTexturePacks;
    const int oldTexDirEntryCount = g_zImage_TexDirEntryCount;

    zClass_NodePartial skippedRoot = {};
    std::strcpy(skippedRoot.name, "skip_vehicle");
    skippedRoot.classId = 5;
    zUtil_PlayerStateStorage skippedPlayer = {};
    skippedPlayer.rootNode = &skippedRoot;
    skippedPlayer.aiNetId = -100;
    zUtil_SaveGameState skippedSaveState = {};
    skippedSaveState.playerState = &skippedPlayer;

    zClass_Object3DDataPartial rootObject = {};
    zClass_NodePartial rootNode = {};
    std::strcpy(rootNode.name, "target_vehicle");
    rootNode.classId = 5;
    rootNode.classData = &rootObject;

    zClass_Object3DDataPartial healthyObject = {};
    healthyObject.rotation = {9.0f, 8.0f, 7.0f};
    healthyObject.localMatrix[9] = 6.0f;
    healthyObject.localMatrix[10] = 5.0f;
    healthyObject.localMatrix[11] = 4.0f;
    zClass_NodePartial healthyNode = {};
    std::strcpy(healthyNode.name, "healthy");
    healthyNode.flags = 0x01;
    healthyNode.classId = 5;
    healthyNode.classData = &healthyObject;
    zClass_NodePartial *children[1] = {&healthyNode};
    rootNode.listCountB = 1;
    rootNode.listB = children;

    PlayerMasterModalData modalData = {};
    modalData.masterType = 77;
    PlayerModalState modalState = {};
    modalState.masterModalData = &modalData;

    AINetNode currentPathNode = {};
    zUtil_PlayerStateStorage playerState = {};
    playerState.rootNode = &rootNode;
    playerState.projectileSpawnVel = {1.0f, 2.0f, 3.0f};
    playerState.localVel = {4.0f, 5.0f, 6.0f};
    playerState.yawRotatedLocalVel = {7.0f, 8.0f, 9.0f};
    playerState.lifecycleState = 1;
    playerState.aiMode2AttackDwell = 2.5f;
    playerState.aiUnknown_0f7c = reinterpret_cast<int>(&currentPathNode);
    playerState.aiCurrentPathNeighborIndex = 9;
    playerState.restartYawRad = 1.25f;
    playerState.variantTag.count = 3;
    playerState.variantTag.tags[0] = 11;
    playerState.variantTag.tags[1] = 12;
    playerState.variantTag.tags[2] = 13;

    zUtil_SaveGameState targetSaveState = {};
    targetSaveState.playerState = &playerState;
    targetSaveState.primaryModalState = &modalState;
    skippedSaveState.next = &targetSaveState;

    PlayerVehicleListSaveEntry saveData = {};
    saveData.size = 128;
    saveData.vehicleRotationAngles = {10.0f, 11.0f, 12.0f};
    saveData.worldPos = {13.0f, 14.0f, 15.0f};
    saveData.aiNetId = 1001;
    saveData.aiTopLevelState = 21;
    saveData.aiSavedTopLevelState = 22;
    saveData.aiReturnTopLevelState = 23;
    saveData.aiAttackRadiusSq = 24.0f;
    saveData.aiRestoreDistanceSq = 25.0f;
    saveData.aiRestoreTarget = {26.0f, 27.0f, 28.0f};
    saveData.aiDynamicOffsetDir = {29.0f, 30.0f, 31.0f};
    saveData.aiActivationRadiusSq = 32.0f;
    saveData.aiTickSuppressed = 33;
    saveData.aiAlertFlag = 34;
    saveData.aiStateMarkerHandle = 35;
    saveData.aiActive = 36;
    saveData.aiPathCursorAdvanceRequested = 37;
    saveData.aiCurrentSteeringSubstate = 38;
    saveData.aiReturnSteeringSubstate = 39;
    saveData.masterType = 1;
    saveData.statusMeterScaled = 40.0f;
    saveData.statusMeterValue = 41.0f;
    saveData.nanitePanelLevel = 42;

    g_PlayerSaveStateListHead = &skippedSaveState;
    g_GameStateOrMapTable =
        reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&targetSaveState);
    g_Time_AccumulatedTimeSec = 100.0f;
    zVidTexturePackEntry builtinPack = {};
    builtinPack.fileHandle = std::tmpfile();
    if (builtinPack.fileHandle == nullptr) {
        RestoreVehicleReadGlobals(oldHead, oldGameState, oldAccumulatedTime,
                                  oldVariantTagCurrent, oldVariantCurrent,
                                  oldBuiltinTexturePackCount, oldBuiltinTexturePacks,
                                  oldTexDirEntryCount);
        return 3;
    }
    g_zVid_BuiltinTexturePackCount = 1;
    g_zVid_BuiltinTexturePacks = &builtinPack;
    g_zImage_TexDirEntryCount = 0;

    Player::ZAR_ReadVehicleListSection(nullptr, "target_vehicle", &saveData,
                                       sizeof(saveData), nullptr);

    const bool ok =
        skippedPlayer.aiNetId == -100 &&
        Vec3Equals(playerState.projectileSpawnVel, zVec3_Make(0.0f, 0.0f, 0.0f)) &&
        Vec3Equals(playerState.localVel, zVec3_Make(0.0f, 0.0f, 0.0f)) &&
        Vec3Equals(playerState.yawRotatedLocalVel, zVec3_Make(0.0f, 0.0f, 0.0f)) &&
        Vec3Equals(playerState.worldPos, saveData.worldPos) &&
        Vec3Equals(playerState.vehicleRotationAngles, saveData.vehicleRotationAngles) &&
        Vec3Equals(playerState.aiRestoreTarget, saveData.aiRestoreTarget) &&
        Vec3Equals(playerState.aiDynamicOffsetDir, saveData.aiDynamicOffsetDir) &&
        playerState.aiNetId == saveData.aiNetId &&
        playerState.aiTopLevelState == saveData.aiTopLevelState &&
        playerState.aiSavedTopLevelState == saveData.aiSavedTopLevelState &&
        playerState.aiReturnTopLevelState == saveData.aiReturnTopLevelState &&
        playerState.aiAttackRadiusSq == saveData.aiAttackRadiusSq &&
        playerState.aiRestoreDistanceSq == saveData.aiRestoreDistanceSq &&
        playerState.aiActivationRadiusSq == saveData.aiActivationRadiusSq &&
        playerState.aiTickSuppressed == saveData.aiTickSuppressed &&
        playerState.recentHitFlag == saveData.aiAlertFlag &&
        playerState.recentHitMarkerHandle == saveData.aiStateMarkerHandle &&
        playerState.aiActive == saveData.aiActive &&
        playerState.aiPathCursorAdvanceRequested == saveData.aiPathCursorAdvanceRequested &&
        playerState.aiCurrentSteeringSubstate == saveData.aiCurrentSteeringSubstate &&
        playerState.aiReturnSteeringSubstate == saveData.aiReturnSteeringSubstate &&
        playerState.lifecycleState == saveData.masterType &&
        playerState.statusMeterScaled == saveData.statusMeterScaled &&
        playerState.statusMeterValue == saveData.statusMeterValue &&
        playerState.nanitePanelLevel == saveData.nanitePanelLevel &&
        FloatNear(playerState.aiStateUntilTime, 100.0f) &&
        FloatNear(playerState.aiHideTime0, 100.0f) &&
        FloatNear(playerState.aiHideTime1, 100.0f) &&
        FloatNear(playerState.unknown_0fa4, 100.0f) &&
        FloatNear(playerState.aiStateStartTime, 100.0f) &&
        FloatNear(playerState.aiStateEndTime, 102.5f) &&
        FloatNear(playerState.unknown_0fd0, 100.0f) &&
        playerState.aiCurrentPathNode == &currentPathNode &&
        playerState.aiCurrentPathNeighborIndex == 0 &&
        (rootNode.flags & 0x04) != 0 && rootNode.nodeType == 0xff &&
        playerState.variantTag.count == 0 && playerState.variantTag.tags[0] == 0xff &&
        playerState.variantTag.tags[1] == 0xff && playerState.variantTag.tags[2] == 0xff;

    if (builtinPack.fileHandle != nullptr) {
        std::fclose(builtinPack.fileHandle);
    }
    RestoreVehicleReadGlobals(oldHead, oldGameState, oldAccumulatedTime,
                              oldVariantTagCurrent, oldVariantCurrent,
                              oldBuiltinTexturePackCount, oldBuiltinTexturePacks,
                              oldTexDirEntryCount);

    return ok ? 0 : 2;
}
