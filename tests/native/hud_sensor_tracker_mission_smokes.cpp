// Checked-in focused native smoke translation unit, formerly extracted from player_tests.cpp.
// Emits the HudSensorTracker mission-start smoke without broad player tests.

#include "Battlesport/game_net.h"
#include "Battlesport/briefing.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/ai_net.h"
#include "Battlesport/hud.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <windows.h>

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "GameZRecoil/zHud/zhud_ui.h"

#include <cstdint>
#include <new>

extern "C" int g_Player_MissionInitFirstRunFlag;

void MakeAinetReaderStringNode(zReader::Node &node, const char *value) {
    node.type = zReader::ZRDR_NODE_STRING;
    node.value.str = const_cast<char *>(value);
}

void MakeAinetReaderIntNode(zReader::Node &node, int value) {
    node.type = zReader::ZRDR_NODE_INT;
    node.value.i32 = value;
}

void MakeAinetReaderFloatNode(zReader::Node &node, float value) {
    node.type = zReader::ZRDR_NODE_FLOAT;
    node.value.f32 = value;
}

void MakeAinetReaderArrayNode(zReader::Node &node, zReader::Node *payload, int count) {
    payload[0].type = zReader::ZRDR_NODE_INT;
    payload[0].value.i32 = count;
    node.type = zReader::ZRDR_NODE_ARRAY;
    node.value.nodes = payload;
}

bool WriteAinetZrdU32(std::FILE *file, unsigned int value) {
    return std::fwrite(&value, sizeof(value), 1, file) == 1;
}

bool WriteAinetZrdNode(std::FILE *file, const zReader::Node &node) {
    if (!WriteAinetZrdU32(file, static_cast<unsigned int>(node.type))) {
        return false;
    }

    switch (node.type) {
    case zReader::ZRDR_NODE_INT:
    case zReader::ZRDR_NODE_FLOAT:
        return WriteAinetZrdU32(file, node.value.u32);
    case zReader::ZRDR_NODE_STRING: {
        const unsigned int length = static_cast<unsigned int>(std::strlen(node.value.str));
        return WriteAinetZrdU32(file, length) &&
               std::fwrite(node.value.str, 1, length, file) == length;
    }
    case zReader::ZRDR_NODE_ARRAY: {
        const int count = node.value.nodes[0].value.i32;
        if (!WriteAinetZrdU32(file, static_cast<unsigned int>(count))) {
            return false;
        }
        for (int index = 1; index < count; ++index) {
            if (!WriteAinetZrdNode(file, node.value.nodes[index])) {
                return false;
            }
        }
        return true;
    }
    }

    return false;
}

struct AinetZrdArchiveEntry {
    const char *name;
    const zReader::Node *root;
};

bool MountAinetZrdArchive(const char *path, const AinetZrdArchiveEntry *entries, int entryCount,
                          zIndexArchive &archive, zZarFileRecord *records,
                          zArchiveListNode &archiveNode, zArchiveList &archiveList) {
    std::FILE *const file = std::fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }

    bool ok = true;
    for (int index = 0; index < entryCount; ++index) {
        const long offset = std::ftell(file);
        if (offset < 0 || !WriteAinetZrdNode(file, *entries[index].root)) {
            ok = false;
            break;
        }
        const long endOffset = std::ftell(file);
        if (endOffset < offset) {
            ok = false;
            break;
        }

        records[index] = {};
        records[index].fileOffset = static_cast<unsigned int>(offset);
        records[index].fileSize = static_cast<unsigned int>(endOffset - offset);
        std::strcpy(records[index].name, entries[index].name);
    }

    if (std::fclose(file) != 0 || !ok) {
        std::remove(path);
        return false;
    }

    archive = {};
    archive.hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, nullptr);
    if (archive.hFile == INVALID_HANDLE_VALUE) {
        std::remove(path);
        return false;
    }

    archive.recordCount = static_cast<unsigned int>(entryCount);
    archive.records = records;

    archiveNode = {};
    archiveNode.payload = &archive;
    archiveNode.next = &archiveNode;
    archiveNode.prev = &archiveNode;

    archiveList = {};
    archiveList.count = 1;
    archiveList.head = &archiveNode;
    g_zArchive_MountedList = &archiveList;
    return true;
}

bool FloatNear(float actual, float expected) {
    return std::fabs(actual - expected) < 0.0001f;
}

extern "C" int hud_sensor_tracker_init_mission_gameplay_systems_smoke(void) {
    char oldCurrentDir[MAX_PATH] = {};
    if (GetCurrentDirectoryA(sizeof(oldCurrentDir), oldCurrentDir) != 0) {
        const char *const rootCandidates[] = {"..\\..\\..\\..", "."};
        for (int index = 0; index < 2; ++index) {
            char supportPath[MAX_PATH] = {};
            std::snprintf(supportPath, sizeof(supportPath), "%s\\support\\zbd",
                          rootCandidates[index]);
            const DWORD attrs = GetFileAttributesA(supportPath);
            if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                SetCurrentDirectoryA(rootCandidates[index]);
                break;
            }
        }
    }

    zUtil_SaveGameState *const oldSaveHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState *const oldSaveTail = g_PlayerSaveStateListTail;
    const int oldSaveAux = g_PlayerSaveStateListAux;
    const int oldSaveCount = g_PlayerSaveStateCount;
    zUtil_SaveGameState *const oldPlayer2SaveState = g_Player2SaveState;
    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zUtil_SaveGameState *const oldCurrentSaveState = g_CurrentPlayerSaveState;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    PlayerMasterCommonData *const oldCommonHead = g_PlayerMasterCommonDataHead;
    PlayerMasterCommonData *const oldCommonTail = g_PlayerMasterCommonDataTail;
    const int oldCommonAux = g_PlayerMasterCommonDataListAux;
    const int oldCommonCount = g_PlayerMasterCommonDataCount;
    PlayerMasterModalData *const oldModalHead = g_PlayerMasterModalDataHead;
    PlayerMasterModalData *const oldModalTail = g_PlayerMasterModalDataTail;
    const int oldModalAux = g_PlayerMasterModalDataListAux;
    const int oldModalCount = g_PlayerMasterModalDataCount;
    const int oldMissionInitFirstRun = g_Player_MissionInitFirstRunFlag;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    HudUiTextStack4 *const oldChatStack = g_HudUiChatMessageStack;
    const HudUiPanel oldTopPanel1 = g_Player_TopMsgPanel1;
    const HudUiPanel oldTopPanel2 = g_Player_TopMsgPanel2;
    const Player_UnderwaterFxPass3Ui oldUnderwaterFxPass3Ui =
        g_Player_UnderwaterFxPass3Ui;
    const Player_ProjectileCameraFxPass3Ui oldState7FxPass3Ui =
        g_Player_State7FxPass3Ui;
    HudUiContainer *const fxContainer =
        reinterpret_cast<HudUiContainer *>(&g_zVideo_FxPass3ConfigLocal);
    const HudUiContainer oldFxContainer = *fxContainer;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    zClass_NodePartial *const oldHorizonNode = g_Player_HorizonNode;
    const int oldHorizonEnabled = g_Player_HorizonNodeFollowCameraEnabled;
    const int oldRuntimeInputFlags = g_Player_RuntimeInputFlags;
    const int oldLocalControlEnabled = g_Player_LocalControlEnabled;
    const int oldNextOrdinal = g_Player_NextOrdinal;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    const float oldTotalTime = g_Player_TotalTimeSecScaled;
    const float oldCameraZone = g_Player_CameraZone;
    const float oldCameraZoneInvRange = g_Player_CameraZoneInvRange;
    const float oldNominalGravity = g_Player_NominalGravity;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    zClass_NodeFreeListSlot *const oldNodeArray = g_zClass_NodeArray;
    const int oldFreeHead = g_zClass_NodeFreeHeadIndex;
    const int oldActiveNodeCount = g_zClass_ActiveNodeCount;
    zClass_TypeListLink *const oldFreeLinkHead = g_zClass_TypeList_FreeLinkHead;
    zClass_TypeListLink *const oldPendingFreeHead = g_zClass_NodeList_PendingFreeHead;
    const int oldDeferredProcessing = g_zClass_DeferredProcessingEnabled;
    const int oldLiveLinkCount = g_zClass_TypeList_LiveLinkCount;
    const int oldPeakLiveLinkCount = g_zClass_TypeList_PeakLiveLinkCount;
    int *const oldNetworkEnabled = g_zGame_Options_PointerCache.networkEnabled;
    int *const oldGameControlOptions = g_zGame_Options_PointerCache.gameControlOptions;
    int *const oldDifficultyOption = g_zGame_Options_PointerCache.gameDifficulty;
    int *const oldReplicate = g_zGame_Options_PointerCache.replicate;
    zArchiveList *const oldMountedList = g_zArchive_MountedList;
    zInput_BindMapContext *const oldBindMap = g_zInput_BindMap_Current;
    zOpt_ViewRectSection **const oldDisplayOption = g_zGame_Options_PointerCache.displaySection;
    zOpt_ViewRectSection **const oldWindowOption = g_zGame_Options_PointerCache.windowSection;
    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    HudLayoutBase *const oldCurrentLayout = g_HudUiMgrCurrentLayout;
    const HudUiRect oldHudRect = g_HudUiMgrHudRect;
    const HudUiRect oldViewRect = g_HudUiMgrViewRect;
    const HudUiMgrSensorBlock oldSensorBlock = g_HudUiMgrSensorBlock;
    const HudUiRect oldSensorFxRect = g_HudUiMgrSensorFxRect;
    const int oldSensorFxViewportWidth = g_HudUiMgrSensorFxViewportWidth;
    const int oldSensorFxViewportHeight = g_HudUiMgrSensorFxViewportHeight;
    const HudUiNanitePanel oldNanitePanel = g_HudUiMgrNanitePanel;
    HudUiMessage oldMessages[10];
    std::memcpy(oldMessages, g_HudUiMgrMessages, sizeof(oldMessages));
    HudUiCounter oldModeCounters[4];
    std::memcpy(oldModeCounters, g_HudUiMgrModeCounters, sizeof(oldModeCounters));
    const int oldActiveWeaponMessageIndex = g_HudUiMgrActiveWeaponMessageIndex;
    const int oldActiveWeaponSideIndex = g_HudUiMgrActiveWeaponSideIndex;
    const int oldActiveModeCounterIndex = g_HudUiMgrActiveModeCounterIndex;
    const HudUiWidget oldObjectiveWidget = g_HudUiMgrObjectiveWidget;
    const int oldObjectivePhase = g_HudUiMgrObjectivePhase;
    zClass_NodePartial *const oldEffectWorld = g_zEffect_World;
    zClass_NodePartial *const oldEffectResource = g_zEffect_ResourceNode;
    zEffectAnimEntry *const oldEffectEntries = g_zEffectAnim_EntryList;
    const short oldEffectCount = g_zEffectAnim_EntryCount;
    const int oldEffectInstantiated = g_zEffectAnim_EntriesInstantiated;
    char oldEffectZbdFilename[sizeof(g_zEffectAnim_ZbdFilename)] = {};
    std::memcpy(oldEffectZbdFilename, g_zEffectAnim_ZbdFilename,
                sizeof(g_zEffectAnim_ZbdFilename));
    zClass_NodePartial *const oldThermalGlowFreeList = g_OptCatalogThermalGlowFreeList;
    const int oldHudCounterValue = g_Player_HudCounterValue;
    const int oldDamageFeedbackHitCount = g_OptCatalog_DamageFeedbackHitCount;
    zClass_NodePartial *const oldHudWorldNode = g_HudSensorTracker.worldNode;
    const int oldHudMissionId = g_HudSensorTracker.missionId;
    const int oldHudRaceCheckpointMode = g_HudSensorTracker.raceCheckpointMode;
    const int oldHudMissionStat1 = g_HudSensorTracker.missionStat1;
    zModel_MaterialSlot *const oldMatlPool = g_zModel_MatlPool;
    const int oldMatlPoolCapacity = g_zModel_MatlPoolCapacity;
    const int oldMatlPoolInUseCount = g_zModel_MatlPoolInUseCount;
    const int oldMatlFreeHeadIndex = g_zModel_MatlFreeHeadIndex;
    const int oldMatlActiveHeadIndex = g_zModel_MatlActiveHeadIndex;
    zModel_MaterialPartial *const oldMatlReuseCache = g_zModel_MatlReuseCache;
    const int oldTexDirEntryCount = g_zImage_TexDirEntryCount;
    zImage_TexDirEntryPartial *const oldTexDirEntries =
        static_cast<zImage_TexDirEntryPartial *>(
            std::calloc(0x1000, sizeof(zImage_TexDirEntryPartial)));
    if (oldTexDirEntries == nullptr) {
        return 3;
    }
    std::memcpy(oldTexDirEntries, g_zImage_TexDirEntries,
                sizeof(zImage_TexDirEntryPartial) * 0x1000);
    const int oldZdeclientRebuildBltRect = g_zDEClient_RebuildBltRectOnReload;
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const int oldBuiltinTexturePackCount = g_zVid_BuiltinTexturePackCount;
    zVidTexturePackEntry *const oldBuiltinTexturePacks = g_zVid_BuiltinTexturePacks;
    const int oldTexturePackCount = g_zVid_TexturePackCount;
    zVidTexturePackEntry *const oldTexturePacks = g_zVid_TexturePacks;
    const zSndSampleSetRegistry oldSndRegistry = g_zSnd_SampleSetRegistry;
    const int oldSndInitialized = g_zSnd_IsInitialized;
    const int oldActiveBackend = g_zSnd_ActiveBackend;
    const unsigned int oldLoadingMaxIndex = g_HudUiLoadingCheckpointMaxIndex;
    const unsigned int oldLoadingCurrentIndex = g_HudUiLoadingCheckpointCurrentIndex;
    const float oldLoadingCurrentProgress = g_HudUiLoadingCheckpointCurrentProgress;
    HudUiBriefingRuntime *const oldBriefingRuntime = g_Briefing_Runtime;

    zClass_TypeListLink *oldTypeHeads[16] = {};
    zClass_TypeListLink *oldTypeTails[16] = {};
    int oldTypeDirty[16] = {};
    for (int i = 0; i < 16; ++i) {
        oldTypeHeads[i] = zClass_TypeList::Head(i);
        oldTypeTails[i] = zClass_TypeList::Tail(i);
        oldTypeDirty[i] = zClass_TypeList::PendingRemovalDirty(i);
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }

    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempPath), tempPath) == 0 ||
        GetTempFileNameA(tempPath, "hsg", 0, tempFile) == 0) {
        return 1;
    }

    zReader::Node playerRoot = {};
    zReader::Node playerItems[3] = {};
    zReader::Node cameraZoneItems[2] = {};
    MakeAinetReaderArrayNode(playerRoot, playerItems, 3);
    MakeAinetReaderStringNode(playerItems[1], "camera_zone");
    MakeAinetReaderArrayNode(playerItems[2], cameraZoneItems, 2);
    MakeAinetReaderFloatNode(cameraZoneItems[1], 0.75f);

    zReader::Node vehicleRoot = {};
    zReader::Node vehicleItems[3] = {};
    zReader::Node stealthItems[5] = {};
    zReader::Node commonItems[1] = {};
    zReader::Node modalItems[3] = {};
    zReader::Node modeItems[2] = {};
    MakeAinetReaderArrayNode(vehicleRoot, vehicleItems, 3);
    MakeAinetReaderStringNode(vehicleItems[1], "stealth");
    MakeAinetReaderArrayNode(vehicleItems[2], stealthItems, 5);
    MakeAinetReaderStringNode(stealthItems[1], "common_mode");
    MakeAinetReaderArrayNode(stealthItems[2], commonItems, 1);
    MakeAinetReaderStringNode(stealthItems[3], "basic");
    MakeAinetReaderArrayNode(stealthItems[4], modalItems, 3);
    MakeAinetReaderStringNode(modalItems[1], "mode");
    MakeAinetReaderArrayNode(modalItems[2], modeItems, 2);
    MakeAinetReaderStringNode(modeItems[1], "basic");

    zReader::Node declientRoot = {};
    zReader::Node declientItems[3] = {};
    zReader::Node craterItems[13] = {};
    MakeAinetReaderArrayNode(declientRoot, declientItems, 3);
    MakeAinetReaderStringNode(declientItems[1], "CRATER");
    MakeAinetReaderArrayNode(declientItems[2], craterItems, 13);
    MakeAinetReaderStringNode(craterItems[1], "POINTS");
    MakeAinetReaderIntNode(craterItems[2], 7);
    MakeAinetReaderStringNode(craterItems[3], "SLOPE");
    MakeAinetReaderFloatNode(craterItems[4], 0.0f);
    MakeAinetReaderStringNode(craterItems[5], "DEPTH");
    MakeAinetReaderFloatNode(craterItems[6], 4.0f);
    MakeAinetReaderStringNode(craterItems[7], "RADIUS");
    MakeAinetReaderFloatNode(craterItems[8], 20.0f);
    MakeAinetReaderStringNode(craterItems[9], "DEFAULT_TEXTURE");
    MakeAinetReaderStringNode(craterItems[10], "crater_base.tga");
    MakeAinetReaderStringNode(craterItems[11], "DEFAULT_ANIM");
    MakeAinetReaderStringNode(craterItems[12], "crater_fx");

    const AinetZrdArchiveEntry entries[] = {
        {"player.zrd", &playerRoot},
        {"vehicle.zrd", &vehicleRoot},
        {"declient.zrd", &declientRoot},
    };
    zIndexArchive archive = {};
    zZarFileRecord records[3] = {};
    zArchiveListNode archiveNode = {};
    zArchiveList archiveList = {};
    if (!MountAinetZrdArchive(tempFile, entries, 3, archive, records, archiveNode,
                              archiveList)) {
        return 2;
    }

    HudUiTopMessageStack topStack = {};
    topStack.Constructor();
    g_HudUiTopMessageStack = &topStack;
    g_HudUiChatMessageStack = nullptr;
    std::memset(&g_Player_TopMsgPanel1, 0, sizeof(g_Player_TopMsgPanel1));
    std::memset(&g_Player_TopMsgPanel2, 0, sizeof(g_Player_TopMsgPanel2));
    g_Player_TopMsgPanel1.ConstructorDefault("", 0, 0);
    g_Player_TopMsgPanel2.ConstructorDefault("", 0, 0);
    std::memset(&g_Player_UnderwaterFxPass3Ui, 0, sizeof(g_Player_UnderwaterFxPass3Ui));
    std::memset(&g_Player_State7FxPass3Ui, 0, sizeof(g_Player_State7FxPass3Ui));
    g_Player_UnderwaterFxPass3Ui.Constructor();
    g_Player_State7FxPass3Ui.Constructor();
    std::memset(fxContainer, 0, sizeof(*fxContainer));

    HudUiShieldMessageWidget shield = {};
    new (&shield.widget) HudUiWidget(0);
    reinterpret_cast<HudUiPanel *>(&shield.percentTextPanel)->ConstructorDefault("", 0, 0);
    new (&shield.meter) HudUiShieldMeterCandidate();
    shield.meter.fillPixelsMax = 100;
    shield.meter.points[1].y = 100.0f;
    g_HudUiMgrSensorBlock = {};
    g_HudUiMgrShieldMessageWidget = &shield;
    g_HudUiMgrCurrentLayout = nullptr;
    g_HudUiMgrSensorBlock.sensorParam = 1.0f;
    g_HudUiMgrSensorFxRect = {0, 0, 100, 80};
    g_HudUiMgrSensorFxViewportWidth = 100;
    g_HudUiMgrSensorFxViewportHeight = 80;
    std::memset(&g_HudUiMgrNanitePanel, 0, sizeof(g_HudUiMgrNanitePanel));
    static_cast<HudUiTripletPanel *>(&g_HudUiMgrNanitePanel)->Constructor();
    for (HudUiMessage &message : g_HudUiMgrMessages) {
        message.Constructor();
    }
    for (HudUiCounter &counter : g_HudUiMgrModeCounters) {
        new (&counter) HudUiCounter();
    }
    g_HudUiMgrObjectiveWidget.Constructor(0);
    g_HudUiMgrObjectivePhase = 0;
    g_HudUiMgrActiveWeaponMessageIndex = 0;
    g_HudUiMgrActiveWeaponSideIndex = 0;
    g_HudUiMgrActiveModeCounterIndex = 0;

    zClass_NodePartial worldNode = {};
    zClass_WorldDataPartial worldData = {};
    worldNode.classId = 2;
    worldNode.classData = &worldData;
    std::strcpy(worldNode.name, "world1");
    zClass_NodePartial cameraNode = {};
    zClass_CameraDataPartial cameraData = {};
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    cameraData.frustumWidth = 1.25f;
    cameraData.frustumHeight = 0.75f;
    zClass_NodePartial resourceNode = {};

    zClass_NodeFreeListSlot slots[32] = {};
    for (int i = 0; i < 31; ++i) {
        slots[i].freeTag = i + 1;
    }
    slots[31].freeTag = 0x00ffffff;
    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_DeferredProcessingEnabled = 1;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    int networkEnabled = 0;
    int gameControlOptions = 0;
    int difficultyOption = 1;
    int replicate = 0;
    g_zGame_Options_PointerCache.networkEnabled = &networkEnabled;
    g_zGame_Options_PointerCache.gameControlOptions = &gameControlOptions;
    g_zGame_Options_PointerCache.gameDifficulty = &difficultyOption;
    g_zGame_Options_PointerCache.replicate = &replicate;

    zOpt_ViewRectSection display = {};
    display.rightExclusive = 640;
    display.bottomExclusive = 480;
    display.width = 640;
    display.height = 480;
    display.bitsPerPixel = 16;
    zOpt_ViewRectSection window = {};
    window.rightExclusive = 640;
    window.bottomExclusive = 480;
    window.width = 640;
    window.height = 480;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;
    g_zGame_Options_PointerCache.displaySection = &displayPtr;
    g_zGame_Options_PointerCache.windowSection = &windowPtr;

    zInput_BindMapContext context = {};
    context.InitCommandMap(30);
    for (int commandId = 24; commandId <= 29; ++commandId) {
        char label[0x20];
        std::sprintf(label, "Command %d", commandId);
        context.SetBindingRecord(commandId, label, 0x20 + commandId, 0, 0, 0);
    }
    g_zInput_BindMap_Current = &context;

    zUtil_SaveGameState fakeLocalSave = {};
    zUtil_PlayerStateStorage fakeLocalState = {};
    PlayerMasterCommonData fakeCommon = {};
    PlayerGunFireController fakeAlt = {};
    PlayerGunFireController fakePrimary = {};
    fakeLocalSave.playerState = &fakeLocalState;
    fakeLocalState.masterCommonData = &fakeCommon;
    fakeLocalState.activeAltGunController = &fakeAlt;
    fakeLocalState.activePrimaryGunController = &fakePrimary;
    fakeLocalState.statusMeterValue = 25.0f;
    fakeLocalState.nanitePanelLevel = 2;
    fakeCommon.maxHealth = 100.0f;
    fakeCommon.invMaxHealth = 0.01f;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&fakeLocalSave);

    g_PlayerSaveStateListHead = nullptr;
    g_PlayerSaveStateListTail = nullptr;
    g_PlayerSaveStateListAux = 0;
    g_PlayerSaveStateCount = 0;
    g_Player2SaveState = nullptr;
    g_LocalPlayerSaveState = nullptr;
    g_CurrentPlayerSaveState = nullptr;
    g_PlayerMasterCommonDataHead = nullptr;
    g_PlayerMasterCommonDataTail = nullptr;
    g_PlayerMasterCommonDataListAux = 0;
    g_PlayerMasterCommonDataCount = 0;
    g_PlayerMasterModalDataHead = nullptr;
    g_PlayerMasterModalDataTail = nullptr;
    g_PlayerMasterModalDataListAux = 0;
    g_PlayerMasterModalDataCount = 0;
    g_Player_MissionInitFirstRunFlag = 1;
    g_Player_HorizonNode = nullptr;
    g_Player_HorizonNodeFollowCameraEnabled = 0;
    g_Player_RuntimeInputFlags = 0;
    g_Player_LocalControlEnabled = 0;
    g_Player_NextOrdinal = 1;
    g_Time_AccumulatedTimeSec = 9.5f;
    g_Player_TotalTimeSecScaled = 9.5f;
    g_Player_CameraZone = 0.5f;
    g_Player_CameraZoneInvRange = 2.0f;
    g_Player_NominalGravity = 0.0f;
    g_PlayerStatusMeterRatio = 0.0f;
    g_HudSensorTracker.worldNode = &worldNode;
    g_HudSensorTracker.missionId = 2;
    g_HudSensorTracker.raceCheckpointMode = 0;
    g_HudSensorTracker.missionStat1 = 0;
    g_zEffect_World = nullptr;
    g_zEffect_ResourceNode = nullptr;
    g_zEffectAnim_EntryList = nullptr;
    g_zEffectAnim_EntryCount = 0;
    g_zEffectAnim_EntriesInstantiated = 1;
    g_zEffectAnim_ZbdFilename[0] = '\0';
    g_OptCatalogThermalGlowFreeList = nullptr;
    g_Player_HudCounterValue = 123;
    g_OptCatalog_DamageFeedbackHitCount = 456;
    zModel_MaterialSlot materialSlots[8] = {};
    for (int i = 0; i < 8; ++i) {
        materialSlots[i].prevPoolIndex = static_cast<short>(i - 1);
        materialSlots[i].nextPoolIndex = static_cast<short>(i + 1);
    }
    materialSlots[0].prevPoolIndex = -1;
    materialSlots[7].nextPoolIndex = -1;
    g_zModel_MatlPool = materialSlots;
    g_zModel_MatlPoolCapacity = 8;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = 0;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_MatlReuseCache = nullptr;
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    std::strcpy(g_zImage_TexDirEntries[0].baseName, "crater_base.tga");
    g_zImage_TexDirEntries[0].loadState = 1;
    g_zImage_TexDirEntryCount = 1;
    g_zDEClient_RebuildBltRectOnReload = 0;
    zVidTexturePackEntry texturePacks[2] = {};
    texturePacks[0].fileHandle = stdout;
    texturePacks[1].fileHandle = stdout;
    g_zVideo_ActiveRendererPath = 0;
    g_zVid_BuiltinTexturePackCount = 1;
    g_zVid_BuiltinTexturePacks = &texturePacks[0];
    g_zVid_TexturePackCount = 1;
    g_zVid_TexturePacks = &texturePacks[1];
    zSndSample samples[1] = {};
    samples[0].replayFields.sampleId = "snd_incoming";
    samples[0].primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(0x1000);
    zSndSampleSet sampleSet = {};
    sampleSet.sampleCount = 1;
    sampleSet.samples = samples;
    g_zSnd_SampleSetRegistry.clear();
    g_zSnd_SampleSetRegistry.push_back(&sampleSet);
    g_zSnd_IsInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    HudUiLoadingCheckpoint::InitTable();
    g_Briefing_Runtime = nullptr;

    HudSensorTracker tracker = {};
    tracker.missionId = 2;
    tracker.worldNode = &worldNode;
    tracker.cameraNode = &cameraNode;
    tracker.effectResourceNode = &resourceNode;
    tracker.missionStat0 = 1;
    tracker.missionStat1 = 2;
    tracker.primaryGunDispatchCount = 3;
    tracker.missionStat3 = 4;
    tracker.weaponsFoundMask = 5;
    tracker.menuTransitionDelaySec = 2.0f;

    const int result = tracker.InitMissionGameplaySystems();
    if (oldCurrentDir[0] != '\0') {
        SetCurrentDirectoryA(oldCurrentDir);
    }
    const zInputCommandCallbackFn objectiveCallback =
        (zInputCommandCallbackFn)(HudSensorTracker::OnObjectiveCommand);
    const bool callbacksOk =
        context.m_commandCallbacks[24] == objectiveCallback &&
        context.m_commandCallbacks[25] == objectiveCallback &&
        context.m_commandCallbacks[26] == objectiveCallback &&
        context.m_commandCallbacks[27] == objectiveCallback &&
        context.m_commandCallbacks[28] == objectiveCallback &&
        context.m_commandCallbacks[29] == objectiveCallback;
    int verifyResult = 0;
    if (result != 1) {
        verifyResult = 10;
    } else if (tracker.missionStat0 != 0 || tracker.missionStat1 != 0 ||
               tracker.primaryGunDispatchCount != 0 || tracker.missionStat3 != 0 ||
               tracker.weaponsFoundMask != 0) {
        verifyResult = 11;
    } else if (g_Player_HudCounterValue != 0 || g_OptCatalog_DamageFeedbackHitCount != 0 ||
               !FloatNear(tracker.menuTransitionDelaySec, -1.0f)) {
        verifyResult = 12;
    } else if (tracker.mapWorldNode != &worldNode) {
        verifyResult = 13;
    } else if (g_zEffect_World != &worldNode) {
        verifyResult = 17;
    } else if (g_zEffect_ResourceNode != &resourceNode) {
        verifyResult = 18;
    } else if (!callbacksOk) {
        verifyResult = 19;
    } else if (g_Player_RuntimeDiScene != &worldNode || g_MainCamera != &cameraNode) {
        verifyResult = 14;
    } else if (shield.viewportResetFrame != -1 || shield.state != 0 ||
               g_HudUiMgrSensorBlock.state != 1) {
        verifyResult = 15;
    }

    Player::ShutdownMissionRuntime();
    zClass_NodePartial *lightNode = g_OptCatalogThermalGlowFreeList;
    while (lightNode != nullptr) {
        zClass_NodePartial *const next = lightNode->callbackContext;
        std::free(lightNode->classData);
        lightNode->classData = nullptr;
        lightNode = next;
    }
    for (int i = 0; i < 32; ++i) {
        std::free(slots[i].node.classData);
        slots[i].node.classData = nullptr;
    }
    context.FreeNonOwnedBuffers();
    if (archive.hFile != INVALID_HANDLE_VALUE && archive.hFile != nullptr) {
        CloseHandle(archive.hFile);
    }
    DeleteFileA(tempFile);

    g_PlayerSaveStateListHead = oldSaveHead;
    g_PlayerSaveStateListTail = oldSaveTail;
    g_PlayerSaveStateListAux = oldSaveAux;
    g_PlayerSaveStateCount = oldSaveCount;
    g_Player2SaveState = oldPlayer2SaveState;
    g_LocalPlayerSaveState = oldLocalSaveState;
    g_CurrentPlayerSaveState = oldCurrentSaveState;
    g_GameStateOrMapTable = oldGameState;
    g_PlayerMasterCommonDataHead = oldCommonHead;
    g_PlayerMasterCommonDataTail = oldCommonTail;
    g_PlayerMasterCommonDataListAux = oldCommonAux;
    g_PlayerMasterCommonDataCount = oldCommonCount;
    g_PlayerMasterModalDataHead = oldModalHead;
    g_PlayerMasterModalDataTail = oldModalTail;
    g_PlayerMasterModalDataListAux = oldModalAux;
    g_PlayerMasterModalDataCount = oldModalCount;
    g_Player_MissionInitFirstRunFlag = oldMissionInitFirstRun;
    g_HudUiTopMessageStack = oldTopStack;
    g_HudUiChatMessageStack = oldChatStack;
    g_Player_TopMsgPanel1 = oldTopPanel1;
    g_Player_TopMsgPanel2 = oldTopPanel2;
    g_Player_UnderwaterFxPass3Ui = oldUnderwaterFxPass3Ui;
    g_Player_State7FxPass3Ui = oldState7FxPass3Ui;
    *fxContainer = oldFxContainer;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_MainCamera = oldMainCamera;
    g_Player_HorizonNode = oldHorizonNode;
    g_Player_HorizonNodeFollowCameraEnabled = oldHorizonEnabled;
    g_Player_RuntimeInputFlags = oldRuntimeInputFlags;
    g_Player_LocalControlEnabled = oldLocalControlEnabled;
    g_Player_NextOrdinal = oldNextOrdinal;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    g_Player_TotalTimeSecScaled = oldTotalTime;
    g_Player_CameraZone = oldCameraZone;
    g_Player_CameraZoneInvRange = oldCameraZoneInvRange;
    g_Player_NominalGravity = oldNominalGravity;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    g_zClass_NodeArray = oldNodeArray;
    g_zClass_NodeFreeHeadIndex = oldFreeHead;
    g_zClass_ActiveNodeCount = oldActiveNodeCount;
    g_zClass_TypeList_FreeLinkHead = oldFreeLinkHead;
    g_zClass_NodeList_PendingFreeHead = oldPendingFreeHead;
    g_zClass_DeferredProcessingEnabled = oldDeferredProcessing;
    g_zClass_TypeList_LiveLinkCount = oldLiveLinkCount;
    g_zClass_TypeList_PeakLiveLinkCount = oldPeakLiveLinkCount;
    g_zGame_Options_PointerCache.networkEnabled = oldNetworkEnabled;
    g_zGame_Options_PointerCache.gameControlOptions = oldGameControlOptions;
    g_zGame_Options_PointerCache.gameDifficulty = oldDifficultyOption;
    g_zGame_Options_PointerCache.replicate = oldReplicate;
    g_zArchive_MountedList = oldMountedList;
    g_zInput_BindMap_Current = oldBindMap;
    g_zGame_Options_PointerCache.displaySection = oldDisplayOption;
    g_zGame_Options_PointerCache.windowSection = oldWindowOption;
    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_HudUiMgrCurrentLayout = oldCurrentLayout;
    g_HudUiMgrHudRect = oldHudRect;
    g_HudUiMgrViewRect = oldViewRect;
    g_HudUiMgrSensorBlock = oldSensorBlock;
    g_HudUiMgrSensorFxRect = oldSensorFxRect;
    g_HudUiMgrSensorFxViewportWidth = oldSensorFxViewportWidth;
    g_HudUiMgrSensorFxViewportHeight = oldSensorFxViewportHeight;
    g_HudUiMgrNanitePanel = oldNanitePanel;
    std::memcpy(g_HudUiMgrMessages, oldMessages, sizeof(oldMessages));
    std::memcpy(g_HudUiMgrModeCounters, oldModeCounters, sizeof(oldModeCounters));
    g_HudUiMgrActiveWeaponMessageIndex = oldActiveWeaponMessageIndex;
    g_HudUiMgrActiveWeaponSideIndex = oldActiveWeaponSideIndex;
    g_HudUiMgrActiveModeCounterIndex = oldActiveModeCounterIndex;
    g_HudUiMgrObjectiveWidget = oldObjectiveWidget;
    g_HudUiMgrObjectivePhase = oldObjectivePhase;
    g_zEffect_World = oldEffectWorld;
    g_zEffect_ResourceNode = oldEffectResource;
    g_zEffectAnim_EntryList = oldEffectEntries;
    g_zEffectAnim_EntryCount = oldEffectCount;
    g_zEffectAnim_EntriesInstantiated = oldEffectInstantiated;
    std::memcpy(g_zEffectAnim_ZbdFilename, oldEffectZbdFilename,
                sizeof(g_zEffectAnim_ZbdFilename));
    g_OptCatalogThermalGlowFreeList = oldThermalGlowFreeList;
    g_Player_HudCounterValue = oldHudCounterValue;
    g_OptCatalog_DamageFeedbackHitCount = oldDamageFeedbackHitCount;
    g_HudSensorTracker.worldNode = oldHudWorldNode;
    g_HudSensorTracker.missionId = oldHudMissionId;
    g_HudSensorTracker.raceCheckpointMode = oldHudRaceCheckpointMode;
    g_HudSensorTracker.missionStat1 = oldHudMissionStat1;
    g_zModel_MatlPool = oldMatlPool;
    g_zModel_MatlPoolCapacity = oldMatlPoolCapacity;
    g_zModel_MatlPoolInUseCount = oldMatlPoolInUseCount;
    g_zModel_MatlFreeHeadIndex = oldMatlFreeHeadIndex;
    g_zModel_MatlActiveHeadIndex = oldMatlActiveHeadIndex;
    g_zModel_MatlReuseCache = oldMatlReuseCache;
    std::memcpy(g_zImage_TexDirEntries, oldTexDirEntries,
                sizeof(zImage_TexDirEntryPartial) * 0x1000);
    std::free(oldTexDirEntries);
    g_zImage_TexDirEntryCount = oldTexDirEntryCount;
    g_zDEClient_RebuildBltRectOnReload = oldZdeclientRebuildBltRect;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVid_BuiltinTexturePackCount = oldBuiltinTexturePackCount;
    g_zVid_BuiltinTexturePacks = oldBuiltinTexturePacks;
    g_zVid_TexturePackCount = oldTexturePackCount;
    g_zVid_TexturePacks = oldTexturePacks;
    g_zSnd_SampleSetRegistry = oldSndRegistry;
    g_zSnd_IsInitialized = oldSndInitialized;
    g_zSnd_ActiveBackend = oldActiveBackend;
    g_HudUiLoadingCheckpointMaxIndex = oldLoadingMaxIndex;
    g_HudUiLoadingCheckpointCurrentIndex = oldLoadingCurrentIndex;
    g_HudUiLoadingCheckpointCurrentProgress = oldLoadingCurrentProgress;
    g_Briefing_Runtime = oldBriefingRuntime;
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = oldTypeHeads[i];
        zClass_TypeList::Tail(i) = oldTypeTails[i];
        zClass_TypeList::PendingRemovalDirty(i) = oldTypeDirty[i];
    }

    return verifyResult;
}

extern "C" int hud_sensor_tracker_parse_checkpoint_number_from_node_smoke(void) {
    zClass_NodePartial node = {};
    zClass_NodePartial contextNode = {};
    node.flags = 0x200000;
    node.callbackContext = &contextNode;
    contextNode.auxFlags = 2;
    std::strcpy(contextNode.name, "checkpoint42");

    if (HudSensorTracker::ParseCheckpointNumberFromNode(&node) != 42) {
        return 1;
    }

    node.flags = 0;
    if (HudSensorTracker::ParseCheckpointNumberFromNode(&node) != 0) {
        return 2;
    }

    node.flags = 0x200000;
    contextNode.auxFlags = 0;
    if (HudSensorTracker::ParseCheckpointNumberFromNode(&node) != 0) {
        return 3;
    }

    contextNode.auxFlags = 2;
    std::strcpy(contextNode.name, "checkpoint-5");
    if (HudSensorTracker::ParseCheckpointNumberFromNode(&node) != 0) {
        return 4;
    }

    std::strcpy(contextNode.name, "checkpoint");
    return HudSensorTracker::ParseCheckpointNumberFromNode(&node) == 0 ? 0 : 5;
}
