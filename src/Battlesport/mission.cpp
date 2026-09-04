#include "Battlesport/mission.h"

#include "Battlesport/hud_sensor_tracker.h"

namespace Mission {
/**
 *
 * Purpose: initialize the global HUD sensor objective tracker and register its
 * process-exit cleanup hook.
 */
void __cdecl InitObjectives() {
    HudSensorTracker::ConstructGlobal();
    HudSensorTracker::RegisterGlobalOnExit();
}
} // namespace Mission

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *MissionCrtInitializerFn)();
/* VC5 emits this mission-objectives startup callback as a direct .CRT$XCU row. */
#pragma data_seg(".CRT$XCU")
MissionCrtInitializerFn s_MissionCrtInit_Objectives =
    Mission::InitObjectives;
#pragma data_seg()
#endif
#include "Battlesport/hud_sensor_tracker.h"

#include "Battlesport/game_net.h"
#include "Battlesport/recoil_app.h"
#include "Battlesport/hud.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zInterp/zinterp.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "Battlesport/turret.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "GameZRecoil/zWeapon/zwep.h"

#include <math.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
struct HudSensorTrackerMissionData {
    int missionId;
    int missionFlags;
    int currentObjectiveIndex;
    int firstIncompleteObjectiveIndex;
    int completedObjectiveCount;
    int objectiveFlowState;
    float objectiveFlowDeadlineSecRaw;
    int missionStat0;
    int missionStat1;
    int missionStat2;
    int missionStat3;
    int weaponsFoundMask;
    int objectiveCompletedFlags[10];
    int difficultyMode;
};

RECOIL_STATIC_ASSERT(sizeof(HudSensorTrackerMissionData) == 0x5c);
}

extern "C" char g_HudSensorTracker_ZarSectionName_MissionData[0x0c];
extern "C" char g_HudSensorTracker_ObjectivesZrdPath[0x0e];
extern "C" const unsigned char g_HudSensorTracker_ObjectiveBlinkColorRedRgb24[4];
extern "C" const unsigned char g_HudSensorTracker_ObjectiveMarkerColorBlueRgb24[4];
extern "C" char g_HudSensorTracker_ZarHandlerName_MissionLate[0x0c];
extern "C" char g_HudSensorTracker_ZarHandlerName_Mission[0x08];
extern "C" char g_HudSensorTracker_LateMissionDataSectionName[0x10];
extern "C" const char g_HudSensorTracker_StartAnimsZrdPath[0x0e];
extern "C" char g_HudSensorTracker_DisplayNodeName[0x08];
extern "C" char g_HudSensorTracker_WindowNodeName[0x08];
extern "C" char g_HudSensorTracker_CameraNodeName[0x08];
extern "C" char g_HudSensorTracker_WorldNodeName[0x07];
extern "C" char g_HudSensorTracker_MissionSoundSetNameFmt[0x04];
extern "C" char g_HudSensorTracker_MissionGsFmt[0x07];
extern "C" char g_HudSensorTracker_MissionZbdGsFmt[0x0b];
extern "C" char g_HudSensorTracker_InitScriptPathFmt[0x13];
extern "C" const char g_HudSensorTracker_FindMissionObjectivesMsg[0x18];
extern "C" const char g_HudSensorTracker_DefaultAirdropCarrierNodeName[0x06];
extern "C" const char kHudSensorTrackerWeatherArchiveName[];
extern "C" const char kHudSensorTrackerAiArchiveName[];
extern "C" const char kHudSensorTrackerWeaponsArchiveName[];
extern "C" const char kHudSensorTrackerEffectsArchiveName[];
extern "C" const char kHudSensorTrackerPickupArchiveName[];
extern "C" int g_HudSensorTracker_ObjectiveCommandLocked;
extern "C" const char g_HudSensorTracker_MissionUnloadedMsg[0x14];
extern "C" const char g_HudSensorTracker_ClosingModelsMsg[0x0f];
extern "C" const char g_HudSensorTracker_ClosingClassMsg[0x0e];
extern "C" const char g_HudSensorTracker_LargeModelsCheckpointFmt[0x11];
extern "C" const char g_HudSensorTracker_ClosingAnimationsMsg[0x13];
extern "C" const char g_HudSensorTracker_ClosingEffectsMsg[0x10];
extern "C" const char g_HudSensorTracker_ClosingWeaponsMsg[0x10];
extern "C" const char g_HudSensorTracker_ClosingPlayerMsg[0x0f];
extern "C" const char g_HudSensorTracker_UnloadObjectivesMsg[0x12];
extern "C" const char g_HudSensorTracker_UnloadingMissionMsg[0x15];
extern "C" const char g_HudLoading_StopAllSoundsMsg[0x10];
extern "C" char g_HudSensorTracker_ObjectiveImageMissingFmt[0x2b];
extern "C" char g_HudSensorTracker_ObjectivesArrayOverflowFmt[0x36];
extern "C" char g_HudSensorTracker_ObjectiveNode_Autoplay[0x09];
extern "C" char g_HudSensorTracker_ObjectiveNodeNameFmt[0x0c];
extern "C" char g_HudSensorTracker_ObjectiveNode_FinalMission[0x0e];
extern "C" char g_HudSensorTracker_ObjectiveNode_ReviewDelay[0x0d];
extern "C" char g_HudSensorTracker_ObjectiveNode_ReadTime[0x0a];
extern "C" char g_HudSensorTracker_MissionImageSearchPathFmt[0x14];
extern "C" char g_HudSensorTracker_ObjectiveIncomingSfxName[0x0d];
extern "C" char g_HudSensorTracker_ObjectiveNode_ObjectiveSound[0x10];
extern "C" char g_HudSensorTracker_ObjectiveNode_ReadSound[0x0b];
extern "C" char g_HudSensorTracker_ObjectiveInactivationNodeMissingFmt[0x31];
extern "C" char g_HudSensorTracker_ObjectiveNode_Inactive[0x09];
extern "C" char g_HudSensorTracker_ObjectiveActivationNodeMissingFmt[0x2f];
extern "C" char g_HudSensorTracker_ObjectiveNode_Active[0x07];
extern "C" char g_HudSensorTracker_ObjectiveNode_ReviewSound[0x0d];
extern "C" char g_HudSensorTracker_ObjectivePanelThreeLineFmt[0x09];
extern "C" char g_HudUiWeaponStatsFmt_Basic[0x3f];
extern "C" char g_HudUiWeaponStatsFmt_Proximity[0x5b];
extern "C" char g_HudUiWeaponFeatureSuffix_Mine[0x06];
extern "C" char g_HudUiWeaponFeatureSuffix_Beam[0x06];
extern "C" char g_HudUiWeaponFeatureSuffix_LockOn[0x09];
extern "C" char g_HudUiWeaponFeatureSuffix_Tether[0x08];
extern "C" char g_HudUiWeaponFeatureSuffix_Multi[0x07];
extern "C" char g_HudUiWeaponFeatureSuffix_Thermal[0x09];
extern "C" char g_HudUiWeaponFeatureSuffix_Remote[0x08];
extern "C" char g_HudUiWeaponFeaturesLabel[0x0a];
extern "C" char g_HudWeatherFx_AlphaGradientNodeName[0x0f];
extern "C" char g_HudWeatherFx_WindVelocityNodeName[0x09];
extern "C" char g_HudWeatherFx_WindDirectionNodeName[0x09];
extern "C" char g_HudWeatherFx_TypeValue_Rain[0x05];
extern "C" char g_HudWeatherFx_TypeValue_Snow[0x05];
extern "C" char g_HudWeatherFx_TypeNodeName[0x05];
extern "C" char g_HudWeatherFx_ParticlesNodeName[0x0a];
extern "C" char g_HudWeatherFx_MissionNodeNameFmt[0x0a];
extern "C" char g_HudSensorTracker_MissionCppSourcePath[0x20];
extern "C" const char kHudSensorTrackerRaceCheckpointCountNodeName[];
extern "C" const char kHudSensorTrackerRaceCheckpointArchiveName[];
extern "C" const char kHudSensorTrackerRaceZrdrSearchPathFmt[];

/**
 * Original-source helper evidence: no standalone retail function exists;
 * observed in HudSensorTracker callers 0x417ee0, 0x4184e0, 0x4188f0,
 * 0x418d40, 0x4193c0, and 0x419470.
 * Evidence: repeated stores preserve the IEEE-754 float bit pattern in integer
 * timer/deadline fields consumed by the HUD objective and race checkpoint
 * runtime; original-source helper evidence is the repeated inlined store idiom.
 * Purpose: store second values in the integer-backed timer fields used by the
 * original HUD mission runtime layout.
 */
inline int FloatToRawSeconds(
    float value
) {
    int rawValue;
    memcpy(
        &rawValue,
        &value,
        sizeof(rawValue)
    );
    return rawValue;
}

/**
 * Original-source helper evidence: no standalone retail function exists;
 * observed in HudSensorTracker objective-flow callers 0x4184e0, 0x418c70,
 * 0x418d40, and race checkpoint caller 0x4193c0.
 * Evidence: repeated caller patterns reinterpret integer-backed HUD timer
 * fields as seconds values without changing their stored bit pattern.
 * Purpose: read integer-backed HUD timer fields as float seconds.
 */
inline float RawSecondsToFloat(
    int rawValue
) {
    float value;
    memcpy(
        &value,
        &rawValue,
        sizeof(value)
    );
    return value;
}

/**
 * Original-source helper evidence: no standalone retail function exists;
 * observed in HudSensorTracker pickup-info formatting caller 0x418940.
 * Evidence: repeated append-only feature-string construction is inlined into
 * the reconstructed pickup info source cluster.
 * Purpose: append one pickup feature label to the HUD feature text buffer.
 */
inline void AppendPickupFeature(
    char *featureText,
    const char *feature
) {
    strcat(
        featureText,
        feature
    );
}

/**
 * Original-source helper evidence: no standalone retail function exists;
 * observed in HudSensorTracker objective loader caller 0x418230.
 * Evidence: ACTIVE and INACTIVE path resolution share the same zClass lookup,
 * child-node walk, and original mission.cpp error reporting pattern.
 * Purpose: resolve an objective ZRD node path to a zClass node.
 */
inline zClass_NodePartial *ResolveObjectiveNodePath(
    zReader::Node *pathNode,
    int objectiveIndex,
    const char *missingFormat,
    int sourceLine
) {
    zReader::Node *const pathFields = pathNode->value.nodes;
    zClass_NodePartial *resolvedNode = zClass::FindByTypeAndName(
        6,
        pathFields[1].value.str
    );
    if (resolvedNode == 0) {
        zError::ReportOld(
            0x400,
            "D:\\Proj\\Battlesport\\mission.cpp",
            sourceLine,
            missingFormat,
            objectiveIndex,
            pathFields[1].value.str
        );
        return 0;
    }

    const int pathCount = pathFields[0].value.i32;
    for (int i = 2; i < pathCount; ++i) {
        resolvedNode = zClass_Class::FindNodeRecursiveByName(
            resolvedNode,
            pathFields[i].value.str
        );
    }

    return resolvedNode;
}

/**
 * Source model: global construction thunk for the CZRecoilFrame-owned
 * g_HudSensorTracker object.
 * Touched data: constructs the accepted zero-filled global data owner
 * g_HudSensorTracker.
 * Purpose: Construct and return the global HUD sensor tracker instance.
 */
HudSensorTracker *__cdecl HudSensorTracker::ConstructGlobal() {
    return g_HudSensorTracker.Constructor();
}

/**
 * Source model: global lifetime registration helper for the tracker singleton.
 * Touched data: registers ShutdownGlobal as the CRT atexit callback.
 * Purpose: Schedule HUD sensor tracker shutdown during process exit.
 */
void __cdecl HudSensorTracker::RegisterGlobalOnExit() {
    atexit(&HudSensorTracker::ShutdownGlobal);
}

/**
 * Source model: global destruction thunk for the CZRecoilFrame-owned
 * g_HudSensorTracker object.
 * Touched data: tears down g_HudSensorTracker through its member Shutdown path.
 * Purpose: Run tracker cleanup for the singleton registered with atexit.
 */
void __cdecl HudSensorTracker::ShutdownGlobal() {
    g_HudSensorTracker.Shutdown();
}

/**
 * Source model: member constructor for the global HudSensorTracker record; VC5
 * EH state only surrounds the three CString default constructors.
 * Touched data: initializes this HudSensorTracker instance and then resets its
 * mission state through ResetMissionState.
 * Purpose: Construct the tracker singleton storage before mission/map runtime use.
 */
HudSensorTracker * HudSensorTracker::Constructor() {
    InitNoBounds();
    missionDataPath.CString::CString();
    zbdPath.CString::CString();
    missionGsPath.CString::CString();
    fxPass3Obj = 0;
    hudScale = 1.0f;
    raceCheckpointMode = 0;
    hasPendingPlayerSave = 0;
    pendingPlayerSave.skipTimerResetOnStart = 0;
    ResetMissionState();
    return this;
}

/**
 * Source model: member serializer for the fixed MissionData ZAR payload.
 * Touched data: reads the tracker mission counters, ten objective completion
 * flags, and the provider-owned difficulty option.
 * Purpose: Serialize HUD mission state into the MissionData ZAR section.
 */
int HudSensorTracker::WriteMissionDataSection(
    zZbdSectionCallbackCtx *writer
) {
    HudSensorTrackerMissionData missionData = {0};
    missionData.missionId = missionId;
    missionData.missionFlags = missionFlags;
    missionData.currentObjectiveIndex = currentObjectiveIndex;
    missionData.firstIncompleteObjectiveIndex = firstIncompleteObjectiveIndex;
    missionData.completedObjectiveCount = completedObjectiveCount;
    missionData.objectiveFlowState = objectiveFlowState;
    missionData.objectiveFlowDeadlineSecRaw = objectiveFlowDeadlineSecRaw;
    missionData.missionStat0 = missionStat0;
    missionData.missionStat1 = missionStat1;
    missionData.missionStat2 = primaryGunDispatchCount;
    missionData.missionStat3 = missionStat3;
    missionData.weaponsFoundMask = weaponsFoundMask;

    {
        for (int index = 0; index < 10; ++index) {
            missionData.objectiveCompletedFlags[index] = objectiveSlots[index].completedFlag;
        }
    }

    missionData.difficultyMode = zOpt::GetGameDifficultyMode();
    return zUtil_ZAR::WriteSectionBlob(
        writer,
        g_HudSensorTracker_ZarSectionName_MissionData,
        &missionData,
        sizeof(missionData)
    );
}

/**
 * Source model: member restore helper for the fixed MissionData ZAR payload.
 * Touched data: updates tracker mission state, pending player-save state,
 * objective completion flags, and provider-owned difficulty option.
 * Purpose: Apply saved HUD mission state and reload mission resources when needed.
 */
int HudSensorTracker::ApplyMissionDataAndReload(
    void *,
    const char *,
    const void *missionDataBlob,
    unsigned int
) {
    const HudSensorTrackerMissionData *const missionData =
        (const HudSensorTrackerMissionData *)(missionDataBlob);

    pendingPlayerSave.skipTimerResetOnStart = 1;
    completedObjectiveCount = missionData->completedObjectiveCount;
    zOpt::SetGameDifficultyMode(missionData->difficultyMode);

    const int currentMissionId = missionId;
    if (currentMissionId == 0) {
        InitMissionIdAndFlags(
            missionData->missionId,
            missionData->missionFlags
        );
        zUtil::ZAR_RequestStopGlobal();
        return 1;
    }

    if (missionData->missionId != currentMissionId) {
        ShutdownMissionGameplaySystems();
        RecoilStateMainMenuTransition::ClearPausedAudioSnapshot();
        InitMissionIdAndFlags(
            missionData->missionId,
            missionData->missionFlags
        );
        zUtil_ZRDR_UnloadMountedArchives(0);
        zUtil::SetMissionZrdrPathsAndMountZbd(missionData->missionId);
        LoadObjectivesFromPath(g_HudSensorTracker_ObjectivesZrdPath);
        LoadMissionCoreResources();
        InitMissionGameplaySystems();
        if (zVid::GetAccelerationOption() != 0) {
            zClass_Camera::SetActiveCamera(0);
            zClass_Camera::SetObjectHseTestEnabled(0);
        }
    }

    completedObjectiveCount = missionData->completedObjectiveCount;
    currentObjectiveIndex = missionData->currentObjectiveIndex;
    firstIncompleteObjectiveIndex = missionData->firstIncompleteObjectiveIndex;
    objectiveFlowState = missionData->objectiveFlowState;
    objectiveFlowDeadlineSecRaw = missionData->objectiveFlowDeadlineSecRaw;
    missionStat0 = missionData->missionStat0;
    missionStat1 = missionData->missionStat1;
    primaryGunDispatchCount = missionData->missionStat2;
    missionStat3 = missionData->missionStat3;
    weaponsFoundMask = missionData->weaponsFoundMask;

    for (int objectiveIndex = 0; objectiveIndex < 10; ++objectiveIndex) {
        const int completedFlag = missionData->objectiveCompletedFlags[objectiveIndex];
        objectiveSlots[objectiveIndex].completedFlag = completedFlag;
        if (completedFlag != 0) {
            SetObjectiveMarkerEnabledAndColor(
                objectiveIndex,
                0,
                0
            );
            SetObjectiveMarkerColorBlink(
                objectiveIndex,
                g_HudSensorTracker_ObjectiveBlinkColorRedRgb24
            );
        }
    }

    return 1;
}

/**
 * Source model: member registration helper for mission ZAR save/restore callbacks.
 * Touched data: registers this tracker as callback context for Mission and
 * MissionLate sections.
 * Purpose: Install HUD mission save/load section handlers.
 */
void HudSensorTracker::RegisterMissionSectionHandlers() {
    zUtil_ZAR::RegisterSectionHandler(
        g_HudSensorTracker_ZarHandlerName_Mission,
        (zZbdSectionCallback)(&HudSensorTracker::ZarMission_SaveCallback),
        (zZbdSectionCallback)(&HudSensorTracker::ZarMission_RestoreCallback),
        0,
        this
    );
    zUtil_ZAR::RegisterSectionHandler(
        g_HudSensorTracker_ZarHandlerName_MissionLate,
        (zZbdSectionCallback)(&HudSensorTracker::ZarMissionLate_SaveCallback),
        (zZbdSectionCallback)(&HudSensorTracker::ZarMissionLate_RestoreCallback),
        0x7d0,
        this
    );
}

/**
 * Source model: static ZAR callback that receives HudSensorTracker as user data.
 * Touched data: serializes the callback tracker through WriteMissionDataSection.
 * Purpose: Forward Mission section save requests to the tracker serializer.
 */
int __fastcall HudSensorTracker::ZarMission_SaveCallback(
    zZbdSectionCallbackCtx *writer,
    HudSensorTracker *self
) {
    return self->WriteMissionDataSection(writer);
}

/**
 * Source model: static ZAR callback that receives HudSensorTracker as user data.
 * Touched data: restores the callback tracker through ApplyMissionDataAndReload.
 * Purpose: Forward Mission section restore payloads to the tracker restore helper.
 */
int __fastcall HudSensorTracker::ZarMission_RestoreCallback(
    void *reader,
    const char *token,
    const void *missionData,
    unsigned int dataSize,
    HudSensorTracker *self
) {
    self->ApplyMissionDataAndReload(
        reader,
        token,
        missionData,
        dataSize
    );
    return 1;
}

/**
 * Source model: static ZAR callback for the late mission restore marker section.
 * Touched data: writes a one-word LateMissionData marker payload.
 * Purpose: Emit the late mission marker section used during saved-game restore.
 */
void __fastcall HudSensorTracker::ZarMissionLate_SaveCallback(
    zZbdSectionCallbackCtx *writer,
    HudSensorTracker *
) {
    unsigned int lateMissionData = 1;
    zUtil_ZAR::WriteSectionBlob(
        writer,
        g_HudSensorTracker_LateMissionDataSectionName,
        &lateMissionData,
        sizeof(lateMissionData)
    );
}

/**
 * Source model: static ZAR callback that receives HudSensorTracker as user data.
 * Touched data: runs the callback tracker start-animation script after late restore.
 * Purpose: Resume mission start animations after late mission data is restored.
 */
void __fastcall HudSensorTracker::ZarMissionLate_RestoreCallback(
    void *,
    const char *,
    const void *,
    unsigned int,
    HudSensorTracker *self
) {
    self->RunStartAnimsFromZrd(
        g_HudSensorTracker_StartAnimsZrdPath,
        g_RecoilApp_LoadGameStartAnimStateName
    );
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\map.cpp.
 * Source model: HudSensorTracker mission reset method; weather FX teardown is
 * the recovered HudUiElement virtual visibility/deleting-destructor dispatch,
 * not a local table or raw slot helper.
 * Touched data: clears mission identity/path/world/objective fields and
 * releases the mission-owned pass-3 weather FX element from the global
 * zVideo pass-3 HUD container.
 * Purpose: reset per-mission HUD tracker state and delete any active weather
 * FX emitter.
 */
int HudSensorTracker::ResetMissionState() {
    missionLoaded = 0;
    missionId = 0;
    missionDataPath.Empty();
    zbdPath.Empty();
    HudUiElement *const fxElement = fxPass3Obj;
    worldNode = 0;
    missionFlags = 1;
    objectiveCount = 0;

    if (fxElement != 0) {
        fxElement->SetVisible(0);
        ((HudUiContainer *)(&g_zVideo_FxPass3ConfigLocal))->RemoveChild(fxElement);

        if (fxPass3Obj != 0) {
            delete fxPass3Obj;
        }

        fxPass3Obj = 0;
    }

    return 1;
}

/**
 * Provisional source-placement hypothesis: GameZRecoil/HudSensorTracker.cpp.
 * Purpose: store the current mission id and flags, clearing the ZBD override
 * path when a nonzero mission id is supplied.
 */
int HudSensorTracker::InitMissionIdAndFlags(
    int newMissionId,
    int flags
) {
    missionFlags = flags;
    missionId = newMissionId;
    if (newMissionId != 0) {
        zbdPath.Empty();
    }

    return 1;
}

/**
 * Purpose: apply the recovered HUD state change handled by HudSensorTracker::SetMissionId.
 */
int HudSensorTracker::SetMissionId(
    int newMissionId
) {
    missionId = newMissionId;
    if (newMissionId != 0) {
        zbdPath.Empty();
    }

    return 1;
}

/**
 * Provisional source-placement hypothesis: GameZRecoil/HudSensorTracker.cpp.
 * Purpose: replace or clear the explicit mission ZBD path override.
 */
int HudSensorTracker::SetZbdPath(
    const char *path
) {
    if (path != 0) {
        zbdPath = path;
    } else {
        zbdPath.Empty();
    }

    return 1;
}

/**
 * Purpose: Return the mission id currently owned by the HUD sensor tracker.
 */
int HudSensorTracker::GetMissionId() {
    return missionId;
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\map.cpp.
 * Purpose: load the core mission script/resources, bind world/camera/window
 * nodes, and activate the render/display/camera sections.
 */
int HudSensorTracker::LoadMissionCoreResources() {
    CString scriptPath;
    zImg::Init();

    if (missionId == 0) {
        missionId = 1;
    }

    raceCheckpointMode = LoadRaceCheckpointMeta();
    scriptPath.Format(
        g_HudSensorTracker_InitScriptPathFmt,
        missionId
    );
    g_zInterp_GlobalContext.RunScriptFile(scriptPath);

    zClass::Init();
    zModel::Init();

    if (((const char *)zbdPath)[0] == '\0') {
        if (missionFlags != 0) {
            zbdPath.Format(
                g_HudSensorTracker_MissionZbdGsFmt,
                missionId
            );
        } else {
            zbdPath.Format(
                g_HudSensorTracker_MissionGsFmt,
                missionId
            );
        }
    }

    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x102));
    g_zInterp_GlobalContext.RunScriptFile(zbdPath);

    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x10e));
    sprintf(
        g_HudSensor_MissionSoundSetName,
        g_HudSensorTracker_MissionSoundSetNameFmt,
        missionId
    );
    zSndSampleSet_InitByName(g_HudSensor_MissionSoundSetName);

    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x103));
    zImage::TexDir_LoadPendingEntries();

    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x104));
    worldNode = zClass::FindByTypeAndName(
        13,
        g_HudSensorTracker_WorldNodeName
    );
    cameraNode = zClass::FindByTypeAndName(
        8,
        g_HudSensorTracker_CameraNodeName
    );
    windowNode = zClass::FindByTypeAndName(
        14,
        g_HudSensorTracker_WindowNodeName
    );
    displayNode = zClass::FindByTypeAndName(
        15,
        g_HudSensorTracker_DisplayNodeName
    );

    zClass_Class::gwNodeUpdateAll();
    zClass::ProcessDeferredWork();
    zOpt::RenderSection_SetTargetWindow(windowNode);
    zOpt::DisplaySection_SetTargetDisplay(displayNode);
    zOpt::CameraSection_SetActiveCamera(cameraNode);

    missionLoaded = 1;
    return 1;
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\map.cpp.
 * Purpose: initialize mission HUD/gameplay systems, objective commands,
 * player runtime, networking, objectives, weather FX, map overlay, and final
 * HUD state refresh.
 */
int HudSensorTracker::InitMissionGameplaySystems() {
    missionStat0 = 0;
    missionStat1 = 0;
    primaryGunDispatchCount = 0;
    missionStat3 = 0;
    weaponsFoundMask = 0;
    g_Player_HudCounterValue = 0;
    g_OptCatalog_DamageFeedbackHitCount = 0;
    menuTransitionDelaySec = -1.0f;

    LoadMissionMapAndSfx(missionId);
    mapWorldNode = worldNode;

    zInputCommandCallbackFn objectiveCommandCallback =
        (zInputCommandCallbackFn)(HudSensorTracker::OnObjectiveCommand);
    zInput::BindMap_Current_SetCommandCallback(
        27,
        objectiveCommandCallback
    );
    if (zOpt::GetNetworkEnabled() == 0) {
        zInput::BindMap_Current_SetCommandCallback(
            28,
            objectiveCommandCallback
        );
        zInput::BindMap_Current_SetCommandCallback(
            29,
            objectiveCommandCallback
        );
        zInput::BindMap_Current_SetCommandCallback(
            24,
            objectiveCommandCallback
        );
        zInput::BindMap_Current_SetCommandCallback(
            25,
            objectiveCommandCallback
        );
    }
    zInput::BindMap_Current_SetCommandCallback(
        26,
        objectiveCommandCallback
    );

    Pickup::Init(
        worldNode,
        kHudSensorTrackerPickupArchiveName
    );
    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x106));
    zEffect::InitFromPath(
        worldNode,
        cameraNode,
        kHudSensorTrackerEffectsArchiveName
    );
    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x108));
    zEffect::SetWorldNode(worldNode);
    zEffect::SetResourceNode(effectResourceNode);
    zEffect_Anim::LoadAndInstantiate();
    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x105));
    zDEClient::LoadConfigResources(worldNode);
    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x109));
    zWeapon::LoadOptCatalogFromPath(
        worldNode,
        kHudSensorTrackerWeaponsArchiveName,
        zOpt::GetNetworkEnabled(),
        zWeapon_OptCatalog::LoadKillVerbString
    );
    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x10a));
    zTurret_System::LoadDefinitionsFromPath(
        worldNode,
        kHudSensorTrackerAiArchiveName
    );
    PickupAirdropSpawnRef::InitGlobalFromCarrierNodeName(g_HudSensorTracker_DefaultAirdropCarrierNodeName);
    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x10b));

    HudUiMgr::ActivateHud(
        (const HudUiRect *)zOpt::GetDisplaySection(),
        (const HudUiRect *)zOpt::GetWindowSection()
    );
    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x10c));
    Player::InitMissionRuntimeFromWorldAndCamera(
        worldNode,
        cameraNode
    );

    if (hasPendingPlayerSave != 0) {
        g_LocalPlayerSaveState->playerState->nanitePanelLevel =
            pendingPlayerSave.savedNanitePanelLevel;
        Player::ApplyMissionSaveData(&pendingPlayerSave.playerSaveData);
        g_PlayerStatusMeterRatio = 1.0f;
        hasPendingPlayerSave = 0;
    }

    Pickup::InitAndLoadPuppySpawns();
    if (zOpt::GetNetworkEnabled() != 0) {
        GameNet::RegisterGameplayHandlersAndOptCatalogCallbacks();
        Net::InitFromZrd();
    }

    HudUiLoadingCheckpoint::AdvanceAndLog(g_HudSensorTracker_FindMissionObjectivesMsg);
    LoadObjectivesFromZrd(g_HudSensorTracker_ObjectivesZrdPath);
    LoadMissionWeatherFx(kHudSensorTrackerWeatherArchiveName);
    zInput::Keyboard_ResetTransitionState();

    if (zOpt::GetNetworkEnabled() != 0 && GameNet::GetStatusBitAllowMaps() != 0) {
        MapOverlayRefToggle(1);
    }

    zClass_Camera::gwCameraSetFlagBit0(
        cameraNode,
        1
    );
    if (g_zVideo_ActiveRendererPath != 0) {
        zModel_MatlBuffer::ReleaseTextureSurfaces();
    }

    Player::RefreshHudFromState((zUtil_SaveGameState *)g_GameStateOrMapTable);
    return 1;
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\map.cpp.
 * Purpose: dispatch bound HUD objective/map command ids to the global
 * HudSensorTracker while honoring network map and objective-command gates.
 */
void __fastcall HudSensorTracker::OnObjectiveCommand(
    int commandId
) {
    switch (commandId) {
    case 0x1b:
        if (zOpt::GetNetworkEnabled() == 0 || GameNet::GetStatusBitAllowMaps() != 0) {
            g_HudSensorTracker.MapOverlayRefToggle(
                g_HudSensorTracker.mapScaleLerpActive == 0 ? 1 : 0
            );
        }
        break;

    case 0x1c:
        g_HudSensorTracker.MapZoomIn();
        break;

    case 0x1d:
        g_HudSensorTracker.MapZoomOut();
        break;

    case 0x18:
        if (g_HudSensorTracker_ObjectiveCommandLocked == 0) {
            g_HudSensorTracker.AdvanceObjectiveState();
        }
        break;

    case 0x19:
        if (g_HudSensorTracker_ObjectiveCommandLocked == 0) {
            g_HudSensorTracker.Command_ToggleObjectivePanel();
        }
        break;

    case 0x1a:
        if (g_HudSensorTracker_ObjectiveCommandLocked == 0) {
            g_HudSensorTracker.Command_ShowObjectivePickupInfo();
        }
        break;

    default:
        break;
    }
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\map.cpp.
 * Purpose: shut down mission gameplay/HUD systems, release mission resources,
 * clear core node refs, and return the tracker to reset mission state.
 */
int HudSensorTracker::ShutdownMissionGameplaySystems() {
    if (missionLoaded == 0) {
        return 1;
    }

    HudUiLoadingCheckpoint::AdvanceAndLog(g_HudSensorTracker_UnloadingMissionMsg);
    HudUiMgr::UpdateTargetReticleFromCursor(
        0,
        0,
        0.0f,
        0.0f
    );
    HudUiMgr::DisableHud();
    HudUiLoadingCheckpoint::AdvanceAndLog(0);
    HudUiAuxOverlay::ClearTextLines();

    HudUiLoadingCheckpoint::AdvanceAndLog(g_HudLoading_StopAllSoundsMsg);
    zSndPlayHandleSnapshot *const soundSnapshot = zSndPlayHandleSnapshot::CreateFromActiveSamples();
    soundSnapshot->StopAllIfPlaying();
    zClass_Camera::gwCameraSetFlagBit0(
        cameraNode,
        0
    );
    MapShutdownAndReset();

    HudUiLoadingCheckpoint::AdvanceAndLog(g_HudSensorTracker_UnloadObjectivesMsg);
    UnloadObjectives();

    HudUiLoadingCheckpoint::AdvanceAndLog(g_HudSensorTracker_ClosingPlayerMsg);
    GameNet::UnregisterGameplayPacketHandlers();
    Player::ShutdownMissionRuntime();
    PickupAirdropSpawnRef::ShutdownGlobal();
    zTurret_System::FreeAllRuntimes();

    HudUiLoadingCheckpoint::AdvanceAndLog(g_HudSensorTracker_ClosingWeaponsMsg);
    OptCatalog::ShutdownCore();

    HudUiLoadingCheckpoint::AdvanceAndLog(g_HudSensorTracker_ClosingEffectsMsg);
    zEffect::Reset();

    HudUiLoadingCheckpoint::AdvanceAndLog(g_HudSensorTracker_ClosingAnimationsMsg);
    zEffect_Anim::Shutdown();
    zDEClient::ShutdownGlobals();
    Pickup::Shutdown();
    zClass_Object3D_ModelRefLerpQueue::Reset();
    zOpt::RenderSection_SetTargetWindow(0);
    zOpt::DisplaySection_SetTargetDisplay(0);
    zOpt::CameraSection_SetActiveCamera(0);

    char loadingMessage[80];
    sprintf(
        loadingMessage,
        g_HudSensorTracker_LargeModelsCheckpointFmt,
        worldNode->listCountB
    );
    HudUiLoadingCheckpoint::AdvanceAndLog(loadingMessage);

    HudUiLoadingCheckpoint::AdvanceAndLog(g_HudSensorTracker_ClosingClassMsg);
    zClass::ShutdownCore();

    HudUiLoadingCheckpoint::AdvanceAndLog(g_HudSensorTracker_ClosingModelsMsg);
    zModel_Display::Shutdown();
    zImage::Shutdown();

    sprintf(
        g_HudSensor_MissionSoundSetName + 16,
        g_HudSensorTracker_MissionSoundSetNameFmt,
        missionId
    );
    zSndSampleSet_DestroyByName(g_HudSensor_MissionSoundSetName + 16);

    HudUiLoadingCheckpoint::AdvanceAndLog(g_HudSensorTracker_MissionUnloadedMsg);
    HudUiLoadingCheckpoint::AdvanceAndLog(0);

    displayNode = 0;
    windowNode = 0;
    cameraNode = 0;
    worldNode = 0;
    missionLoaded = 0;
    ResetMissionState();
    return 1;
}

/**
 * Retail literal-backed physical source block: D:\Proj\Battlesport\mission.cpp.
 * Purpose: reset loaded objective slots in single-player mode and free the
 * loaded objective ZRD tree.
 */
int HudSensorTracker::UnloadObjectives() {
    if (zOpt::GetNetworkEnabled() == 0) {
        {
            for (int index = 0; index < objectiveCount; ++index) {
                objectiveSlots[index].Reset();
            }
        }

        currentObjectiveIndex = -1;
        firstIncompleteObjectiveIndex = 0;
        objectiveCount = 0;
        completedObjectiveCount = 0;
    }

    if (objectivesRootNode != 0) {
        zReader::FreeLoadedTree(objectivesRootNode);
    }

    return 1;
}

/**
 * Source model: embedded HudSensorObjectiveSlot reset method for the objective
 * slot array owned by HudSensorTracker.
 * Touched data: no authored globals; releases only the slot-owned objective
 * image through the accepted zVid image provider helper.
 * Purpose: clear objective text/status fields and drop the retained objective
 * image reference.
 */
void HudSensorObjectiveSlot::Reset() {
    zVidImagePartial *const image = objectiveImage;
    objectiveTitle[0] = '\0';
    objectiveDesc[0] = '\0';
    objectiveSummary[0] = '\0';
    completedFlag = 0;
    if (image != 0) {
        zVid_Image::ReleaseIfNotDefault(image);
        objectiveImage = 0;
    }
}

/**
 * Retail literal-backed physical source block: D:\Proj\Battlesport\mission.cpp.
 * Purpose: load mission objective data, timing values, image resources, and objective slots from a ZRD path.
 */
int HudSensorTracker::LoadObjectivesFromPath(
    const char *path
) {
    zReader::Node *rootNode = zReader::LoadNodeFromPath(
        path,
        0,
        0
    );
    if (rootNode == 0) {
        zError::ReportOld(
            0x200,
            "D:\\Proj\\Battlesport\\mission.cpp",
            0x2c7,
            g_HudSensorTracker_ReadFileFailedFmt,
            path
        );
        return 1;
    }

    objectivesRootNode = rootNode;

    char imagesPath[0x40];
    sprintf(
        imagesPath,
        g_HudSensorTracker_MissionImageSearchPathFmt,
        missionId
    );
    zImage_InitMissionResources(imagesPath);

    objectiveReviewDelaySecRaw = 4.0f;
    objectiveReadTimeSecRaw = 4.0f;
    objectiveReadSoundDelaySecRaw = FloatToRawSeconds(2.0f);

    zReader::Node *readTimeNode = zReader_GetNamedNode(
        rootNode,
        g_HudSensorTracker_ObjectiveNode_ReadTime
    );
    if (readTimeNode != 0) {
        objectiveReadTimeSecRaw =
            (float)(readTimeNode->value.nodes[1].value.i32);
    }

    zReader::Node *reviewDelayNode = zReader_GetNamedNode(
        rootNode,
        g_HudSensorTracker_ObjectiveNode_ReviewDelay
    );
    if (reviewDelayNode != 0) {
        objectiveReviewDelaySecRaw =
            (float)(reviewDelayNode->value.nodes[1].value.i32);
    }

    if (zOpt::GetNetworkEnabled() != 0) {
        return 0;
    }

    zReader::Node *finalMissionNode = zReader_GetNamedNode(
        rootNode,
        g_HudSensorTracker_ObjectiveNode_FinalMission
    );
    if (finalMissionNode != 0) {
        finalMissionFlag = finalMissionNode->value.nodes[1].value.i32;
    } else {
        finalMissionFlag = 0;
    }

    int objectiveNumber;
    int lastObjectiveIndex = 0;
    for (objectiveNumber = 1; objectiveNumber != 0xb; ++objectiveNumber) {
        char objectiveName[0x20];
        sprintf(
            objectiveName,
            g_HudSensorTracker_ObjectiveNodeNameFmt,
            objectiveNumber
        );

        zReader::Node *objectiveNode = zReader_GetNamedNode(
            rootNode,
            objectiveName
        );
        if (objectiveNode == 0) {
            break;
        }

        lastObjectiveIndex = objectiveNumber - 1;
        HudSensorObjectiveSlot &slot = objectiveSlots[lastObjectiveIndex];
        zReader::Node *objectiveFields = objectiveNode->value.nodes;

        const char *imagePath = objectiveFields[1].value.str;
        slot.objectiveImage = zImage::TexDir_FindOrCreateByPath(imagePath);
        if (slot.objectiveImage == 0) {
            zError::ReportOld(
                0x800,
                "D:\\Proj\\Battlesport\\mission.cpp",
                0x2ff,
                g_HudSensorTracker_ObjectiveImageMissingFmt,
                objectiveNumber,
                imagePath
            );
            return 1;
        }

        strncpy(
            slot.objectiveTitle,
            zLoc::ResolveMessageKeyOrFallback(objectiveFields[2].value.str),
            0x100
        );
        slot.objectiveTitle[0xff] = '\0';

        strncpy(
            slot.objectiveDesc,
            zLoc::ResolveMessageKeyOrFallback(objectiveFields[3].value.str),
            0x100
        );
        slot.objectiveDesc[0xff] = '\0';

        strncpy(
            slot.objectiveSummary,
            zLoc::ResolveMessageKeyOrFallback(objectiveFields[4].value.str),
            0x100
        );
        slot.objectiveSummary[0xff] = '\0';

        slot.completedFlag = 0;
        if (zReader_GetNamedNode(
            objectiveNode,
            g_HudSensorTracker_ObjectiveNode_Autoplay
        ) != 0) {
            slot.autoplayFlag = 1;
        }
    }

    if (objectiveNumber == 0xb) {
        zError::ReportOld(
            0x400,
            "D:\\Proj\\Battlesport\\mission.cpp",
            0x2ee,
            g_HudSensorTracker_ObjectivesArrayOverflowFmt,
            objectiveNumber - 1
        );
    }

    currentObjectiveIndex = -1;
    firstIncompleteObjectiveIndex = 0;
    objectiveCount = lastObjectiveIndex + 1;
    completedObjectiveCount = 0;
    return 0;
}

/**
 * Retail literal-backed physical source block: D:\Proj\Battlesport\mission.cpp.
 * Purpose: bind objective sounds and node paths from the loaded objective ZRD,
 * then select the first incomplete objective for the HUD.
 */
int HudSensorTracker::LoadObjectivesFromZrd(
    const char *
) {
    zReader::Node *reviewSoundNode = zReader_GetNamedNode(
        objectivesRootNode,
        g_HudSensorTracker_ObjectiveNode_ReviewSound
    );
    if (reviewSoundNode != 0) {
        objectiveReviewSfx = zSnd::FindSampleByName(reviewSoundNode->value.nodes[1].value.str);
    }

    if (zOpt::GetNetworkEnabled() != 0) {
        return 0;
    }

    char objectiveName[0x20];
    int objectiveNumber = 1;
    sprintf(
        objectiveName,
        g_HudSensorTracker_ObjectiveNodeNameFmt,
        objectiveNumber
    );

    zReader::Node *objectiveNode = zReader_GetNamedNode(
        objectivesRootNode,
        objectiveName
    );
    while (objectiveNode != 0) {
        HudSensorObjectiveSlot &slot = objectiveSlots[objectiveNumber - 1];

        zReader::Node *activeNode = zReader_GetNamedNode(
            objectiveNode,
            g_HudSensorTracker_ObjectiveNode_Active
        );
        if (activeNode != 0) {
            slot.activationNode = ResolveObjectiveNodePath(
                activeNode,
                objectiveNumber - 1,
                g_HudSensorTracker_ObjectiveActivationNodeMissingFmt,
                0x355
            );
            slot.inactivationNode = 0;
        } else {
            zReader::Node *inactiveNode = zReader_GetNamedNode(
                objectiveNode,
                g_HudSensorTracker_ObjectiveNode_Inactive
            );
            if (inactiveNode != 0) {
                slot.activationNode = 0;
                slot.inactivationNode = ResolveObjectiveNodePath(
                    inactiveNode,
                    objectiveNumber - 1,
                    g_HudSensorTracker_ObjectiveInactivationNodeMissingFmt,
                    0x36b
                );
            } else {
                slot.activationNode = 0;
                slot.inactivationNode = 0;
            }
        }

        slot.objectiveReadFlag = 0;
        zReader::Node *readSoundNode = zReader_GetNamedNode(
            objectiveNode,
            g_HudSensorTracker_ObjectiveNode_ReadSound
        );
        if (readSoundNode != 0) {
            zReader::Node *const readSoundFields = readSoundNode->value.nodes;
            slot.readSoundSample = zSnd::FindSampleByName(readSoundFields[1].value.str);
            if (slot.readSoundSample != 0) {
                slot.readSoundSample->SetPlaybackEventHandler(OnObjectiveReadSoundEvent);
            }
            if (readSoundFields[0].value.i32 > 2) {
                objectiveReadSoundDelaySecRaw = readSoundFields[2].value.i32;
            }
        }

        ++objectiveNumber;
        sprintf(
            objectiveName,
            g_HudSensorTracker_ObjectiveNodeNameFmt,
            objectiveNumber
        );
        objectiveNode = zReader_GetNamedNode(
            objectivesRootNode,
            objectiveName
        );
    }

    zReader::Node *objectiveSoundNode = zReader_GetNamedNode(
        objectivesRootNode,
        g_HudSensorTracker_ObjectiveNode_ObjectiveSound
    );
    if (objectiveSoundNode != 0) {
        objectiveCompleteSfx = zSnd::FindSampleByName(objectiveSoundNode->value.nodes[1].value.str);
    }

    objectiveIncomingSfx = zSnd::FindSampleByName(g_HudSensorTracker_ObjectiveIncomingSfxName);
    firstIncompleteObjectiveIndex = FindAndHighlightFirstIncompleteObjective();
    return 0;
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\hud.cpp.
 * Purpose: advance objective review/readout flow and transition HUD sound
 * ducking state.
 */
void HudSensorTracker::AdvanceObjectiveState() {
    const int flowState = objectiveFlowState;
    if (flowState != 0x6b && flowState != 0x67 && flowState != 0x64) {
        if (objectiveUiMode != 1) {
            objectiveCompleteSfx->PlayA3DSimple(1.0f);
            SetObjectiveReviewVisible(1);
            return;
        }

        if (currentObjectiveReadSound != 0) {
            currentObjectiveReadSound->StopActiveVoicesIfPlaying();
        }
        objectiveCompleteSfx->PlayA3DSimple(1.0f);
        SetObjectiveReviewVisible(0);
        return;
    }

    HudUiMgrObjective::SetVisibleAndResetMeterFill(0);
    if (missionId == 1 && firstIncompleteObjectiveIndex == 0) {
        HudUi::PlayPowerupSfx(0);
    }

    HudSensorObjectiveSlot &firstIncompleteSlot = objectiveSlots[firstIncompleteObjectiveIndex];
    if (firstIncompleteObjectiveIndex == currentObjectiveIndex + 1) {
        SetObjectivePanelVisible(1);
        currentObjectiveReadSound = firstIncompleteSlot.readSoundSample;
        currentObjectiveReadSound->PlayA3DSimple(1.0f);
        objectiveFlowState = 0x68;
        objectiveFlowDeadlineSecRaw =
            objectiveReadTimeSecRaw + g_Time_UnscaledAccumulatedTimeSec;
    } else {
        currentObjectiveReadSound = firstIncompleteSlot.readSoundSample;
        currentObjectiveReadSound->PlayDirectSound(
            0,
            1.0f,
            0x3e7
        );
        objectiveFlowState = 0x69;
    }

    hudScale = zSnd::MulGlobalVolumeScaleAndGetPrev(0.600000024f);
    zSnd::SetFlag10PlaybackEnabled(0);
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\hud.cpp.
 * Purpose: show or hide the objective review panel and restore HUD sound state
 * when the review closes.
 */
int HudSensorTracker::SetObjectiveReviewVisible(
    int visible
) {
    objectiveFlowState = 0x65;
    if (visible != 0) {
        objectiveUiMode = 1;
        if (firstIncompleteObjectiveIndex < objectiveCount) {
            HudSensorObjectiveSlot &slot = objectiveSlots[firstIncompleteObjectiveIndex];
            HudUiMgrObjective::Show(
                slot.objectiveImage,
                slot.objectiveTitle,
                slot.objectiveDesc,
                0.0f
            );
        } else {
            HudSensorObjectiveSlot &slot = objectiveSlots[currentObjectiveIndex];
            HudUiMgrObjective::Show(
                slot.objectiveImage,
                zLoc::GetMessageString(0xf0f),
                0,
                0.0f
            );
        }

        return 1;
    }

    objectiveUiMode = 0;
    HudUiMgrObjective::Begin();
    zSnd::SetGlobalVolumeScale(g_HudSensorTracker.hudScale);
    zSnd::SetFlag10PlaybackEnabled(1);
    return 1;
}

/**
 * Purpose: Return the briefing text buffers and image pointer for one objective slot.
 */
int HudSensorTracker::GetObjectiveBriefingStringsAndImageRef(
    int objectiveIndex,
    char **outSummary,
    char **outDesc,
    zVidImagePartial **outImageRef
) {
    HudSensorObjectiveSlot &slot = objectiveSlots[objectiveIndex];
    *outSummary = slot.objectiveTitle;
    *outDesc = slot.objectiveDesc;
    *outImageRef = slot.objectiveImage;
    return 1;
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\map.cpp.
 * Purpose: play the objective review click and toggle the objective summary
 * panel.
 */
void HudSensorTracker::Command_ToggleObjectivePanel() {
    objectiveReviewSfx->PlayA3DSimple(1.0f);
    SetObjectivePanelVisible(objectiveUiMode != 2 ? 1 : 0);
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\hud.cpp.
 * Purpose: show or hide the objective summary panel and format the mission
 * status text displayed in it.
 */
void HudSensorTracker::SetObjectivePanelVisible(
    int visible
) {
    if (visible != 0) {
        objectiveUiMode = 2;

    float damageRatio = 1.0f;
    if (primaryGunDispatchCount > 0) {
        damageRatio =
            (float)(g_OptCatalog_DamageFeedbackHitCount) / (float)(primaryGunDispatchCount);
    }
    const int damagePercent = (int)(damageRatio * 100.0f);

    char objectiveLine[0x80];
    zLoc::FormatMessage(
        objectiveLine,
        0x40,
        0x116,
        completedObjectiveCount,
        objectiveCount,
        damagePercent
    );

    int cappedStat0 = missionStat0;
    if (cappedStat0 > missionStat1) {
        cappedStat0 = missionStat1;
    }

    char statLine[0x80];
    zLoc::FormatMessage(
        statLine,
        0x40,
        0x117,
        cappedStat0,
        missionStat1,
        missionStat3,
        weaponsFoundMask
    );

    const int elapsedSeconds = (int)(objectiveMeterSeconds);
    char timeLine[0x80];
    zLoc::FormatMessage(
        timeLine,
        0x40,
        0x118,
        elapsedSeconds / 60,
        elapsedSeconds % 60
    );

    sprintf(
        objectiveSummaryText,
        g_HudSensorTracker_ObjectivePanelThreeLineFmt,
        objectiveLine,
        statLine,
        timeLine
    );

    if (currentObjectiveIndex < 0) {
        HudSensorObjectiveSlot &firstSlot = objectiveSlots[0];
        HudUiMgrObjective::Show(
            firstSlot.objectiveImage,
            firstSlot.objectiveTitle,
            objectiveSummaryText,
            0.0f
        );
        return;
    }

        HudSensorObjectiveSlot &slot = objectiveSlots[currentObjectiveIndex];
        HudUiMgrObjective::Show(
            slot.objectiveImage,
            slot.objectiveSummary,
            objectiveSummaryText,
            0.0f
        );
        return;
    }

    objectiveUiMode = 0;
    HudUiMgrObjective::Begin();
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\map.cpp.
 * Purpose: play the objective review click and toggle pickup information for
 * the local player's active alternate weapon.
 */
void HudSensorTracker::Command_ShowObjectivePickupInfo() {
    objectiveReviewSfx->PlayA3DSimple(1.0f);

    const int visible = (objectiveUiMode == 3 || objectiveUiMode == 4) ? 0 : 1;
    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);
    ShowObjectivePickupInfo(
        visible,
        0,
        playerState->activeAltGunController->optCatalogEntry
    );
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\map.cpp.
 * Purpose: show or hide active-pickup information and format the weapon stat
 * text for the objective HUD panel.
 */
void HudSensorTracker::ShowObjectivePickupInfo(
    int visible,
    int startAutoAdvance,
    OptCatalogEntryDef *optEntry
) {
    if (visible != 0) {

    char featureText[0x40];
    strcpy(
        featureText,
        g_HudUiWeaponFeaturesLabel
    );

    const unsigned int flags = optEntry->flags;
    if ((flags & 0x00080000) != 0) {
        AppendPickupFeature(
            featureText,
            g_HudUiWeaponFeatureSuffix_Remote
        );
    }
    if ((flags & 0x00200000) != 0) {
        AppendPickupFeature(
            featureText,
            g_HudUiWeaponFeatureSuffix_Thermal
        );
    }
    if ((flags & 0x00010000) != 0) {
        AppendPickupFeature(
            featureText,
            g_HudUiWeaponFeatureSuffix_Multi
        );
    }
    if ((flags & 0x00100000) != 0) {
        AppendPickupFeature(
            featureText,
            g_HudUiWeaponFeatureSuffix_Tether
        );
    } else if ((flags & 0x00004000) != 0) {
        AppendPickupFeature(
            featureText,
            g_HudUiWeaponFeatureSuffix_LockOn
        );
    }
    if ((flags & 0x00000002) != 0) {
        AppendPickupFeature(
            featureText,
            g_HudUiWeaponFeatureSuffix_Beam
        );
    }
    if ((flags & 0x00002000) != 0) {
        AppendPickupFeature(
            featureText,
            g_HudUiWeaponFeatureSuffix_Mine
        );
    }

    if (featureText[0x0c] == '\0') {
        strcpy(
            featureText,
            "\n"
        );
    }

    char weaponStatsText[0x200];
    if (optEntry->impactProximity > 0.0f) {
        const int fireRatePerMinute =
            (int)(60.0f / optEntry->fireRateInterval + 0.5f);
        const int maxRange = (int)(optEntry->range);
        sprintf(
            weaponStatsText,
            g_HudUiWeaponStatsFmt_Proximity,
            fireRatePerMinute,
            maxRange,
            (double)(optEntry->damage),
            (int)(optEntry->impactProximity),
            featureText
        );
    } else {
        const int fireRatePerMinute =
            (int)(60.0f / optEntry->fireRateInterval + 0.5f);
        const int maxRange = (int)(optEntry->range);
        sprintf(
            weaponStatsText,
            g_HudUiWeaponStatsFmt_Basic,
            fireRatePerMinute,
            maxRange,
            (double)(optEntry->damage),
            featureText
        );
    }

    HudUiMgrObjective::Show(
        Pickup::FindOptMetaImageByOptEntry(optEntry),
        optEntry->description,
        weaponStatsText,
        0.0f
    );

    if (startAutoAdvance != 0) {
        objectiveUiMode = 4;
        objectiveFlowDeadlineSecRaw =
            objectiveReadTimeSecRaw + g_Time_UnscaledAccumulatedTimeSec;
        return;
    }

        objectiveUiMode = 3;
        return;
    }

    objectiveUiMode = 0;
    HudUiMgrObjective::Begin();
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\hud.cpp.
 * Purpose: find the first active incomplete objective and enable its blue map
 * marker.
 */
int HudSensorTracker::FindAndHighlightFirstIncompleteObjective() {
    int objectiveIndex = 0;
    while (objectiveIndex < objectiveCount && objectiveSlots[objectiveIndex].completedFlag != 0) {
        ++objectiveIndex;
    }

    if (objectiveIndex < objectiveCount) {
        SetObjectiveMarkerEnabledAndColor(
            objectiveIndex,
            1,
            g_HudSensorTracker_ObjectiveMarkerColorBlueRgb24
        );
    }

    return objectiveIndex;
}

/**
 * Source model: HudSensorTracker mission-start HUD/objective state reset
 * method; HUD-manager and option calls are external owner dependencies.
 * Touched data: mutates only this tracker and accepted HUD/option globals
 * through their typed APIs.
 * Purpose: reset objective/timer flow for a mission start and restore network
 * HUD presentation when multiplayer is active.
 */
void HudSensorTracker::ResetHudForMissionStart() {
    objectiveMeterSeconds = 0.0f;
    HudUiMgrObjective::SetVisibleAndResetMeterFill(0);

    if (zOpt::GetNetworkEnabled() != 0) {
        HudUiMgr::DisableHud();
        if (zOpt::GetHudVisibilityOption() != 0) {
            HudUiMgr::ApplyHudModeSwitch(zOpt::GetHudTypeForCurrentHwMode());
            HudUiMgr::EnableHud();
        }

        zUtil_PlayerStateStorage *const playerState =
            (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);
        HudUi::ShowTopMessageLine(
            playerState->activeAltGunController->optCatalogEntry->description,
            5.0f
        );
        HudUiTimerPanel::SetRunning(1);
        HudUiMgr::TriggerCurrentLayoutOnActivated();
        HudUiMgr::UpdateTargetReticleFromCursor(
            1,
            0,
            0.5f,
            0.5f
        );
        OptCatalog_SetDamageMaskEnabled(0);
        pendingPlayerSave.skipTimerResetOnStart = 0;
        return;
    }

    if (pendingPlayerSave.skipTimerResetOnStart == 0) {
        HudUiTimerPanel::SetElapsedSeconds(0.0f);
    }

    const float readSoundDelaySec = RawSecondsToFloat(objectiveReadSoundDelaySecRaw);
    objectiveFlowState = 0x64;
    objectiveUiMode = 0;
    currentObjectiveReadSound = 0;
    pendingPlayerSave.skipTimerResetOnStart = 0;
    objectiveFlowDeadlineSecRaw =
        readSoundDelaySec + g_Time_UnscaledAccumulatedTimeSec;
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\map.cpp.
 * Purpose: Advance mission objective UI state, timers, and post-read transitions.
 */
int HudSensorTracker::UpdateObjectiveFlow() {
    objectiveMeterSeconds = HudUiTimerPanel::GetSeconds();

    if (zOpt::GetNetworkEnabled() == 0) {
        firstIncompleteObjectiveIndex = FindAndHighlightFirstIncompleteObjective();

        if (menuTransitionDelaySec > 0.0f &&
            objectiveReadTimeSecRaw + menuTransitionDelaySec <=
                g_Time_AccumulatedTimeSec) {
            zUtil_PlayerStateStorage *const playerState =
                (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);
            if (playerState->lifecycleState == 4) {
                RecoilStateMainMenuTransition::QueueEnter(RECOIL_MAINMENU_ROUTE_INGAME);
            }

            menuTransitionDelaySec = -1.0f;
        }

        {
            for (int objectiveIndex = 0; objectiveIndex < objectiveCount; ++objectiveIndex) {
                HudSensorObjectiveSlot &slot = objectiveSlots[objectiveIndex];
                if (slot.completedFlag != 0) {
                    continue;
                }

                int completed = 0;
                if (slot.activationNode != 0 && (slot.activationNode->flags & 4) != 0) {
                    completed = 1;
                } else if (slot.inactivationNode != 0 && (slot.inactivationNode->flags & 4) == 0) {
                    completed = 1;
                }

                if (completed == 0) {
                    continue;
                }

                slot.completedFlag = 1;
                ++completedObjectiveCount;
                objectiveFlowState = 0x67;
                currentObjectiveIndex = objectiveIndex;
                objectiveFlowDeadlineSecRaw =
                    objectiveReviewDelaySecRaw +
                    g_Time_UnscaledAccumulatedTimeSec;
                SetObjectiveMarkerEnabledAndColor(
                    firstIncompleteObjectiveIndex,
                    0,
                    0
                );
                SetObjectiveMarkerColorBlink(
                    firstIncompleteObjectiveIndex,
                    g_HudSensorTracker_ObjectiveBlinkColorRedRgb24
                );

                if (finalMissionFlag != 0 && completedObjectiveCount == 5) {
                    g_HudSensorTracker_ObjectiveCommandLocked = 1;
                    AINet::AiFinalizeMode2State1ForAllPlayers();
                    zTurret_System::DisableTickCallback();
                    HudUiMgrObjective::SetVisibleAndResetMeterFill(0);
                    SetObjectivePanelVisible(1);
                    objectiveFlowState = 0x68;
                    objectiveFlowDeadlineSecRaw =
                        g_Time_UnscaledAccumulatedTimeSec + 60.0f;
                }

                break;
            }
        }

        switch (objectiveFlowState) {
        case 0x64:
        case 0x67:
            if (g_Time_UnscaledAccumulatedTimeSec >= objectiveFlowDeadlineSecRaw) {
                objectiveIncomingSfx->PlayA3DSimple(1.0f);
                HudUiMgrObjective::SetVisibleAndResetMeterFill(1);
                objectiveFlowState = 0x6b;
            }
            break;

        case 0x68:
            if (g_Time_UnscaledAccumulatedTimeSec >= objectiveFlowDeadlineSecRaw) {
                SetObjectivePanelVisible(0);
                objectiveFlowState = 0x69;
            }
            break;

        case 0x6b:
            if (objectiveSlots[firstIncompleteObjectiveIndex].autoplayFlag != 0) {
                AdvanceObjectiveState();
            }
            break;
        }
    }

    if (objectiveUiMode == 4 &&
        g_Time_UnscaledAccumulatedTimeSec >= objectiveFlowDeadlineSecRaw) {
        zUtil_PlayerStateStorage *const playerState =
            (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);
        ShowObjectivePickupInfo(
            0,
            1,
            playerState->activeAltGunController->optCatalogEntry
        );
    }

    return 1;
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\map.cpp.
 * Purpose: preserve the recovered HUD behavior for HudSensorTracker::SaveAndQueueMissionState.
 */
void HudSensorTracker::SaveAndQueueMissionState() {
    if (finalMissionFlag != 0) {
        g_RecoilApp_QuitAfterCredits = 1;
        return;
    }

    zUtil_PlayerStateStorage *const playerState = g_LocalPlayerSaveState->playerState;
    Player::BuildMissionSaveData(&pendingPlayerSave.playerSaveData);
    pendingPlayerSave.savedNanitePanelLevel = playerState->nanitePanelLevel;
    hasPendingPlayerSave = 1;
    QueueMissionFmvStateForMissionId(missionId + 1);
}

/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\recoilapp.cpp.
 * Purpose: queue the recovered HUD application-state transition for HudSensorTracker::QueueMissionFmvStateForMissionId.
 */
int HudSensorTracker::QueueMissionFmvStateForMissionId(
    int missionId
) {
    g_RecoilApp.m_missionFmvState.SetMissionId(missionId);
    g_RecoilApp.m_missionFmvState.m_skipMissionFmv = 0;
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_missionFmvState,
        0
    );
    return 1;
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\map.cpp.
 * Source model: mission weather FX loader creates the recovered
 * HudWeatherFxSnow/HudWeatherFxRain class owner and stores it as the
 * HudSensorTracker pass-3 element.
 * Purpose: parse Weather.zrd, construct the requested weather emitter, apply
 * optional tuning fields, and attach it to the global pass-3 HUD container.
 */
void HudSensorTracker::LoadMissionWeatherFx(
    const char *zrdPath
) {
    zReader::Node *rootNode = zReader::LoadNodeFromPath(
        zrdPath,
        0,
        0
    );
    if (rootNode == 0) {
        zError::ReportOld(
            0x200,
            "D:\\Proj\\Battlesport\\mission.cpp",
            0x5f6,
            g_HudSensorTracker_ReadFileFailedFmt,
            zrdPath
        );
        return;
    }

    char missionNodeName[0x40];
    sprintf(
        missionNodeName,
        g_HudWeatherFx_MissionNodeNameFmt,
        missionId
    );
    zReader::Node *missionNode = zReader_GetNamedNode(
        rootNode,
        missionNodeName
    );
    if (missionNode != 0) {
        int particleCount = 100;
        zReader::Node *particleNode = zReader_GetNamedNode(
            missionNode,
            g_HudWeatherFx_ParticlesNodeName
        );
        if (particleNode != 0) {
            particleCount = particleNode->value.i32;
        }

        zReader::Node *typeNode = zReader_GetNamedNode(
            missionNode,
            g_HudWeatherFx_TypeNodeName
        );
        if (typeNode != 0) {
            const char *const weatherType = typeNode->value.str;
            if (strcmp(
                weatherType,
                g_HudWeatherFx_TypeValue_Snow
            ) == 0) {
                fxPass3Obj = new HudWeatherFxSnow(particleCount);
            } else if (strcmp(
                weatherType,
                g_HudWeatherFx_TypeValue_Rain
            ) == 0) {
                fxPass3Obj = new HudWeatherFxRain(particleCount);
            }
        }

        if (fxPass3Obj != 0) {
            HudWeatherFx *const weatherFx = (HudWeatherFx *)(fxPass3Obj);

            zReader::Node *colorNode = zReader_GetNamedNode(
                missionNode,
                "COLOR"
            );
            if (colorNode != 0) {
                zReader::Node *const colorFields = colorNode->value.nodes;
                weatherFx->packedColor16 = zVid_PackColorRGB(
                    colorFields[1].value.i32,
                    colorFields[2].value.i32,
                    colorFields[3].value.i32
                );
            }

            zReader::Node *windDirNode = zReader_GetNamedNode(
                missionNode,
                g_HudWeatherFx_WindDirectionNodeName
            );
            if (windDirNode != 0) {
                weatherFx->windDirection = windDirNode->value.f32;
            }

            zReader::Node *windVelNode = zReader_GetNamedNode(
                missionNode,
                g_HudWeatherFx_WindVelocityNodeName
            );
            if (windVelNode != 0) {
                weatherFx->windVelocity = windVelNode->value.f32;
            }

            zReader::Node *gravityNode = zReader_GetNamedNode(
                missionNode,
                "GRAVITY"
            );
            if (gravityNode != 0) {
                weatherFx->gravity = gravityNode->value.f32;
            }

            zReader::Node *alphaGradientNode = zReader_GetNamedNode(
                missionNode,
                g_HudWeatherFx_AlphaGradientNodeName
            );
            if (alphaGradientNode != 0) {
                zReader::Node *const alphaFields = alphaGradientNode->value.nodes;
                weatherFx->alphaStartScale = alphaFields[1].value.f32;
                weatherFx->alphaEndScale = alphaFields[2].value.f32;
            }

            ((HudUiContainer *)(&g_zVideo_FxPass3ConfigLocal))->AddChild(fxPass3Obj);
        }
    }

    zReader::FreeLoadedTree(rootNode);
}

/**
 * Source model: mission-start helper on HudSensorTracker; animation lookup and
 * tree loading are accepted zEffect/zReader provider-source dependencies.
 * Touched data: reads network option state and mutates resolved zEffect
 * animation entries only through accepted zEffect animation APIs.
 * Purpose: run named mission start animations from a ZRD list outside network
 * mode.
 */
void HudSensorTracker::RunStartAnimsFromZrd(
    const char *zrdPath,
    const char *namedNodeName
) {
    if (zOpt::GetNetworkEnabled() != 0) {
        return;
    }

    zReader::Node *rootNode = zReader::LoadNodeFromPath(
        zrdPath,
        0,
        0
    );
    if (rootNode == 0) {
        zError::ReportOld(
            0x200,
            g_HudSensorTracker_MissionCppSourcePath,
            0x646,
            g_HudSensorTracker_ReadFileFailedFmt,
            zrdPath
        );
        return;
    }

    zReader::Node *startAnimList = zReader_GetNamedNode(
        rootNode,
        namedNodeName
    );
    if (startAnimList != 0) {
        zReader::Node *startAnimFields = startAnimList->value.nodes;
        const int startAnimCount = startAnimFields[0].value.i32 - 1;
        {
            for (int startAnimIndex = 0; startAnimIndex < startAnimCount; ++startAnimIndex) {
                zReader::Node *startAnimEntry = startAnimFields[startAnimIndex + 1].value.nodes;
                zEffectAnimEntry *effectAnim =
                    zEffectAnim::FindEntryByName(startAnimEntry[1].value.str);
                if (effectAnim != 0) {
                    zEffectAnim::ResetActivationPrereqCount(effectAnim);
                    zEffectAnim::SetVelocity_Thunk(
                        effectAnim,
                        0,
                        0.0f,
                        0.0f,
                        0.0f
                    );
                }
            }
        }
    }

    zReader::FreeLoadedTree(rootNode);
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\hud.cpp.
 * Purpose: handle objective read-sound events that open or close the review UI
 * and restore HUD sound state when playback completes.
 */
void __fastcall HudSensorTracker::OnObjectiveReadSoundEvent(
    int eventCode
) {
    if (eventCode == 2) {
        zSnd::SetGlobalVolumeScale(g_HudSensorTracker.hudScale);
        zSnd::SetFlag10PlaybackEnabled(1);
        return;
    }

    int visible;
    if (eventCode == 0) {
        visible = 1;
    } else if (eventCode == 1) {
        visible = 0;
    } else {
        return;
    }

    g_HudSensorTracker.SetObjectiveReviewVisible(visible);
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\map.cpp.
 * Purpose: load mission race checkpoint metadata and publish timer/count state
 * when a cp_count node is present.
 */
int HudSensorTracker::LoadRaceCheckpointMeta() {
    CString raceZrdrSearchPath;
    raceZrdrSearchPath.Format(
        kHudSensorTrackerRaceZrdrSearchPathFmt,
        missionId
    );

    int raceCheckpointMode = 0;
    zReader::Node *raceRoot = zReader::LoadNodeFromPath(
        kHudSensorTrackerRaceCheckpointArchiveName,
        raceZrdrSearchPath,
        0
    );
    if (raceRoot != 0) {
        zReader::Node *cpCountNode = zReader_GetNamedNode(
            raceRoot,
            kHudSensorTrackerRaceCheckpointCountNodeName
        );
        if (cpCountNode != 0) {
            raceCheckpointMode = 1;
            runtimeTimerSecRaw = FloatToRawSeconds(20.0f);
            checkpointCount = cpCountNode->value.nodes[1].value.i32;
        }

        zReader::FreeLoadedTree(raceRoot);
    }

    return raceCheckpointMode;
}

/**
 * Purpose: Store the runtime timer seconds payload and mission goal value.
 */
void HudSensorTracker::SetRuntimeTimerSecAndGoalValue(
    int timerSecRaw,
    int goalValue
) {
    runtimeGoalValue = goalValue;
    runtimeTimerSecRaw = timerSecRaw;
}

/**
 * Source model: HudSensorTracker lifetime cleanup method; CString destruction
 * and EH state are MFC/VC5 provider scaffolding, while map teardown remains the
 * accepted map shutdown/reset owner.
 * Touched data: no authored globals; clears owned mission path strings and
 * resets map runtime state through MapShutdownAndResetThunk.
 * Purpose: release mission/map path state when the tracker shuts down.
 */
void HudSensorTracker::Shutdown() {
    missionGsPath.~CString();
    zbdPath.~CString();
    missionDataPath.~CString();
    MapShutdownAndResetThunk();
}

#include "Battlesport/recoil_app.h"
#include "Battlesport/hud_ui_mp_exit_dialog.h"
#include "Battlesport/hud.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"

#include <new>
#include <stdio.h>
#include <string.h>
#include <windows.h>

extern "C" HWND g_RecoilApp_hWndMain;

/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-huduimpexitdialog
 * @recoil-artifact defines .data recoil:data:0x4f329c: g_HudUiMpExitDialog.
 * Purpose: preserve the recovered HUD global storage for g_HudUiMpExitDialog.
 */
HudUiMpExitDialog *g_HudUiMpExitDialog = 0;

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: capture and blur the current surface, load the MPEXIT dialog layout, and configure button or network-message state.
 */
void HudUiMpExitDialog::LoadLayout() {
    m_mpNewGameButtonMode = HudUiMgr::IsLocalPlayerFirstInStatsList();

    zVidImagePartial *const image = zVideo_buff_CaptureSurfaceToImage(1);
    m_capturedBackgroundImage = image;
    const int imageWidth = image->width;
    zVideo::Fx_SetSurfaceState(
        image->pixels,
        imageWidth,
        image->height,
        imageWidth * 2
    );
    zVideo::buff_BlurRegionByMode(
        0,
        3
    );
    zVideo::buff_BlurRegionByMode(
        0,
        3
    );
    zVideo::buff_BlurRegionByMode(
        0,
        3
    );

    HudScoreboard::SetScaleAndRebuild(0.0f);

    zReader::Node *const loadedSection = LoadFromZrd(
        "dialog.zrd",
        "MPEXIT",
        1
    );
    if (loadedSection != 0) {
        if (m_mpNewGameButtonMode >= 0) {
            BindWidgetByName(
                loadedSection,
                &m_mpNewGameButton,
                "MPNEWGAME"
            );
        }
        BindWidgetByName(
            loadedSection,
            &m_mpExitButton,
            "MPEXITBTN"
        );
        FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
    }

    SetChildFlags(0);
    if (m_mpNewGameButtonMode >= 0) {
        HudUiZrdWidget *const newGameButton = &m_mpNewGameButton;
        newGameButton->modeOrEnabled = m_mpNewGameButtonMode;
        newGameButton->RefreshState();
    } else {
        HudUiMgr::EnableTopAndChatStacks();
        g_HudUiTopMessageStack->SetXAll(zVideo::GetPrimarySurfaceWidth() / 2);
        if (zOpt::GetNetworkModemEnabled() != 0) {
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x25),
                300.0f
            );
        } else {
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x39),
                300.0f
            );
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x40),
                300.0f
            );
        }
    }

    m_fadeElapsedSeconds = 0.0f;
    SetEnabled(1);
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: disable and unload the multiplayer exit dialog presentation state and release its captured background image.
 */
void HudUiMpExitDialog::UnloadLayout() {
    SetEnabled(0);
    UpdateAll(0.0f);
    HudScoreboard::SetScaleAndRebuild(0.0f);
    g_HudUiTopMessageStack->Clear();
    if (m_capturedBackgroundImage != 0) {
        m_capturedBackgroundImage =
            (zVidImagePartial *)(unsigned int)zVid_Image::ReleaseIfNotDefault(
                m_capturedBackgroundImage
            );
    }
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: advance the multiplayer exit fade/update path and restore the captured background through the video postprocess pass.
 */
void HudUiMpExitDialog::Update(
    float deltaSeconds
) {
    if (m_mpNewGameButtonMode >= 0) {
        const float fadeElapsedSeconds = m_fadeElapsedSeconds + deltaSeconds;
        m_fadeElapsedSeconds = fadeElapsedSeconds;
        HudScoreboard::SetScaleAndRebuild(fadeElapsedSeconds < 1.0f ? fadeElapsedSeconds : 1.0f);
    }

    zVideo::RunPostprocessOnPrimaryBuffer();
    zVid_Image::BlitToActiveTarget(
        m_capturedBackgroundImage,
        0,
        0,
        0,
        0
    );
    HudUiBackgroundContainer::UpdateAll(deltaSeconds);

    if (m_mpNewGameButtonMode >= 0) {
        HudScoreboard::DispatchSetScale(deltaSeconds);
    } else {
        g_HudUiTopMessageStack->UpdateAll(g_Time_UnscaledDeltaTimeSec);
    }

    zVideo::Dispatch_UnlockPrimarySurfaceState();
    zOpt_ViewRectSection *const dstRect = zOpt::GetWindowSection();
    zOpt_ViewRectSection *const srcRect = zOpt::GetWindowSection();
    zVideo::AdjustSurfacesIfEnabled(
        (zVidRect32 *)srcRect,
        (zVidRect32 *)dstRect,
        0,
        1
    );
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: lazily construct the multiplayer exit dialog singleton and load its layout in software video mode.
 */
void RecoilApp_MpExitDialogState::OnEnter() {
    if (g_HudUiMpExitDialog == 0) {
        HudUiMpExitDialog *dialog = new HudUiMpExitDialog;
        g_HudUiMpExitDialog = dialog;
    }

    if (zVid::GetAccelerationOption() == 0) {
        g_HudUiMpExitDialog->LoadLayout();
    }
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: queue the intro FMV and multiplayer setup reconfiguration when the new-game button is activated.
 */
void HudUiMpExitDialog_NewGameButton::OnActivate() {
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_introFmvState,
        0
    );
    HudUiNetGameSetupOverlayOwner::QueueEnterWithReconfigureFlag(1);
    HudUiZrdWidget::OnActivate();
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: run the base widget activation and queue the leave-network state.
 */
void HudUiMpExitDialog_ExitButton::OnActivate() {
    HudUiZrdWidget::OnActivate();
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_leaveNetworkState,
        0
    );
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: destroy the exit and new-game child widgets before tearing down the background base.
 */
void HudUiMpExitDialog::Destructor() {
    m_mpExitButton.HudUiZrdWidget::~HudUiZrdWidget();
    m_mpNewGameButton.HudUiZrdWidget::~HudUiZrdWidget();
    this->HudUiBackground::~HudUiBackground();
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: configure render, sound, and input state before entering the multiplayer exit dialog.
 */
int RecoilApp_MpExitDialogState::OnTryBecomeCurrent() {
    zVideo::SetHalfResAdjustMode(0);
    HudUi::SetInvalidateMode(0);

    zRndr::SetFrameBufferRegion(
        zVideo::GetPrimarySurfacePixels(),
        zOpt::GetWindowSection(),
        zOpt::GetDisplaySectionBitsPerPixel(),
        zVideo::GetPrimarySurfacePitch()
    );

    zSndSampleSet_InitByName(g_HudUiDialogSampleSetName);
    zInput::BindMapContext_Push(0);
    zInput::BindMapCurrent_ResetAllBindings();

    if (zVid::GetAccelerationOption() != 0) {
        g_HudUiMpExitDialog->LoadLayout();
    }

    return 1;
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: unload, destroy, and clear the multiplayer exit dialog and restore input, sound, and scoreboard state.
 */
void RecoilApp_MpExitDialogState::OnDeactivate() {
    g_HudUiMpExitDialog->UnloadLayout();

    HudUiMpExitDialog *const dialog = g_HudUiMpExitDialog;
    if (dialog != 0) {
        delete dialog;
    }

    g_HudUiMpExitDialog = 0;
    zInput::BindMapContext_Pop();
    Sleep(1000);
    zSndSampleSet_DestroyByName(g_HudUiDialogSampleSetName);
    HudScoreboard::SetScaleAndRebuild(0.0f);
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: poll input, tick/update the dialog, and run the fatal timeout shutdown path after a long stalled fade.
 */
int RecoilApp_MpExitDialogState::OnUpdateShouldQuit() {
    zInput::PollActiveDevices(0);
    Time::Tick();

    HudUiMpExitDialog *const dialog = g_HudUiMpExitDialog;
    ((HudUiContainer *)dialog)->UpdateAll(g_FrameDeltaTimeSec);

    if (g_HudUiMpExitDialog->m_fadeElapsedSeconds > 600.0f) {
        char caption[128];
        char text[128];

        strcpy(
            caption,
            zLoc::GetMessageString(28)
        );
        strcpy(
            text,
            zLoc::GetMessageString(29)
        );
        zVideo_dd::FlipToGDIIfAttached();
        zSndSystem::Shutdown();
        zNetwork::ShutdownSessionRuntime();
        zVideo::ShutdownVideoSystem();
        printf(
            "%s: %s\n",
            caption,
            text
        );
        Sleep(1000);
        MessageBeep(MB_ICONHAND);
        MessageBoxA(
            g_RecoilApp_hWndMain,
            text,
            caption,
            MB_ICONHAND
        );
        zSys::ExitProcessWithCleanup(0);
    }

    return 0;
}

#include "recoil/Mfc42Abi.h"
#include "Battlesport/hud_ui_net_game_setup.h"

#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/recoil_app.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

extern "C" const char kClampedIntTextInputAcceptedRawKeyChars[];

namespace {
/**
 * Original helper evidence: no standalone retail function; repeated inlined
 * min/max clamp sequence in 0x419aa0 and shared world-button callers
 * 0x41a820/0x41a9c0 immediately before "%d" formatting.
 * Purpose: Clamp integer setup values to the active input bounds.
 */
inline int ClampInt(
    int value,
    int minValue,
    int maxValue
) {
    if (value < minValue) {
        value = minValue;
    }
    if (value > maxValue) {
        value = maxValue;
    }
    return value;
}

/**
 * Original helper evidence: no standalone retail function; repeated store to
 * modeOrEnabled followed by the ftable slot 0x78 RefreshState dispatch in
 * 0x419aa0 and world-button side-effect callers 0x41a820/0x41a9c0.
 * Purpose: Store the enabled flag and refresh the ZRD widget state.
 */
inline void SetZrdWidgetEnabled(
    HudUiZrdWidget *widget,
    int enabled
) {
    widget->modeOrEnabled = enabled;
    widget->RefreshState();
}

/**
 * Original helper evidence: no standalone retail function; repeated
 * constructor-lowered pattern in 0x419aa0 for time, kills, and max players:
 * min/max stores, clamped value, sprintf("%d"), then
 * HudUiNumericTextInput::Update.
 * Purpose: Initialize clamped integer text input bounds and visible text.
 */
inline void InitClampedInput(
    HudUiClampedIntTextInput *input,
    int minValue,
    int maxValue,
    int value
) {
    input->minValue = minValue;
    input->maxValue = maxValue;

    char valueText[20];
    sprintf(
        valueText,
        "%d",
        ClampInt(value, minValue, maxValue)
    );
    input->Update(valueText);
}

/**
 * Original helper evidence: no standalone retail function; repeated
 * constructor-local targetInput and stepDelta stores in 0x419aa0 for the
 * increment/decrement time, kills, and max players buttons.
 * Purpose: Bind a step button to its target clamped integer input.
 */
inline void ConfigureStepButton(
    HudUiClampedIntStepButton *button,
    HudUiClampedIntTextInput *targetInput,
    int stepDelta
) {
    button->targetInput = targetInput;
    button->stepDelta = stepDelta;
}

/**
 * Original helper evidence: no standalone retail function; repeated indirect
 * ftable slot 0x60 visibility dispatch in 0x419aa0 and world-button callers
 * 0x41a820/0x41a9c0.
 * Purpose: Dispatch a widget visibility change through its installed table.
 */
inline void SetWidgetVisible(
    HudUiWidget *widget,
    int visible
) {
    ((HudUiElement *)(widget))->SetVisible(visible);
}

} // namespace

/**
 * Original helper evidence: no standalone retail function; observed in caller
 * 0x419aa0 as compiler-emitted construction installing this concrete member
 * widget table.
 * Purpose: construct the launch button through its ZRD widget base.
 */
HudUiNetGameSetupPanel_LaunchButton::HudUiNetGameSetupPanel_LaunchButton()
    : HudUiZrdWidget() {
}

/**
 * Original helper evidence: no standalone retail function; observed in caller
 * 0x419aa0 as compiler-emitted construction installing this concrete member
 * widget table.
 * Purpose: construct the cancel button through its ZRD widget base.
 */
HudUiNetGameSetupPanel_CancelButton::HudUiNetGameSetupPanel_CancelButton()
    : HudUiZrdWidget() {
}

/**
 * Purpose: Initialize the network game setup panel controls and default session options.
 */
HudUiNetGameSetupPanel::HudUiNetGameSetupPanel(
    int reconfigureExistingSessionValue
) : HudUiBackground(),
    playButton(),
    cancelButton(),
    gameNameInput(),
    worldSelector(),
    nextWorldButton(),
    prevWorldButton(),
    timeLimitInput(),
    incTimeLimitButton(),
    decTimeLimitButton(),
    killsInput(),
    incKillsButton(),
    decKillsButton(),
    maxPlayersInput(),
    incMaxPlayersButton(),
    decMaxPlayersButton(),
    allowMapsToggle(),
    nameTagsToggle(),
    killsSwitch(0),
    lapsSwitch(0) {
    zReader::Node *const loadedSection =
        HudUiBackground::LoadFromZrd(
            "dialog.zrd",
            "MP_NEW_GAME",
            0
        );

    incTimeLimitButton.targetInput = 0;
    incTimeLimitButton.stepDelta = 1;
    decTimeLimitButton.targetInput = 0;
    decTimeLimitButton.stepDelta = 1;
    incKillsButton.targetInput = 0;
    incKillsButton.stepDelta = 1;
    decKillsButton.targetInput = 0;
    decKillsButton.stepDelta = 1;
    incMaxPlayersButton.targetInput = 0;
    incMaxPlayersButton.stepDelta = 1;
    decMaxPlayersButton.targetInput = 0;
    decMaxPlayersButton.stepDelta = 1;

    reconfigureExistingSession = reconfigureExistingSessionValue;

    if (loadedSection != 0) {
        HudUiBackground::BindPrimitiveNodeToElement(
            loadedSection,
            (HudUiElement *)(&killsSwitch),
            "KILLS_SWITCH"
        );
        HudUiBackground::BindPrimitiveNodeToElement(
            loadedSection,
            (HudUiElement *)(&lapsSwitch),
            "LAPS_SWITCH"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &playButton,
            "PLAY"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &cancelButton,
            "CANCEL"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &gameNameInput,
            "GAME_NAME"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &worldSelector,
            "WORLD"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &nextWorldButton,
            "INC_WORLD"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &prevWorldButton,
            "DEC_WORLD"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &timeLimitInput,
            "TIME_LIMIT"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &incTimeLimitButton,
            "INC_TIME_LIMIT"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &decTimeLimitButton,
            "DEC_TIME_LIMIT"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &killsInput,
            "KILLS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &incKillsButton,
            "INC_KILLS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &decKillsButton,
            "DEC_KILLS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &maxPlayersInput,
            "MAX_PLAYERS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &incMaxPlayersButton,
            "INC_MAX_PLAYERS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &decMaxPlayersButton,
            "DEC_MAX_PLAYERS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &allowMapsToggle,
            "ALLOW_MAPS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &nameTagsToggle,
            "NAME_TAGS"
        );
        HudUiBackground::FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
    }

    SetWidgetVisible(
        &killsSwitch,
        1
    );
    SetWidgetVisible(
        &lapsSwitch,
        0
    );
    worldSelector.SetIndexClamped(0);
    char *const playerName = zOpt_GetPlayerName();
    char playerNameText[24];
    sprintf(
        playerNameText,
        "%.21s",
        playerName
    );
    gameNameInput.Update(playerNameText);
    gameNameInput.AllocTextBuffer(21);

    const int enabledForNewSession = reconfigureExistingSession == 0 ? 1 : 0;
    SetZrdWidgetEnabled(
        &gameNameInput,
        enabledForNewSession
    );

    InitClampedInput(
        &timeLimitInput,
        5,
        360,
        15
    );

    InitClampedInput(
        &killsInput,
        1,
        99,
        10
    );

    if (zOpt::GetNetworkModemEnabled() != 0) {
        maxPlayersInput.modeOrEnabled = 0;
        maxPlayersInput.RefreshState();
        SetZrdWidgetEnabled(
            &incMaxPlayersButton,
            0
        );
        SetZrdWidgetEnabled(
            &decMaxPlayersButton,
            0
        );
    } else {
        InitClampedInput(
            &maxPlayersInput,
            2,
            8,
            8
        );
        SetZrdWidgetEnabled(
            &maxPlayersInput,
            enabledForNewSession
        );
        ConfigureStepButton(
            &incMaxPlayersButton,
            &maxPlayersInput,
            1
        );
        SetZrdWidgetEnabled(
            &incMaxPlayersButton,
            enabledForNewSession
        );
        ConfigureStepButton(
            &decMaxPlayersButton,
            &maxPlayersInput,
            -1
        );
        decMaxPlayersButton.modeOrEnabled = enabledForNewSession;
        decMaxPlayersButton.RefreshState();
    }

    currentFocusWidget = 0;

    ConfigureStepButton(
        &incTimeLimitButton,
        &timeLimitInput,
        1
    );
    ConfigureStepButton(
        &decTimeLimitButton,
        &timeLimitInput,
        -1
    );
    ConfigureStepButton(
        &incKillsButton,
        &killsInput,
        1
    );
    ConfigureStepButton(
        &decKillsButton,
        &killsInput,
        -1
    );

    allowMapsToggle.SetChecked(1);
    nameTagsToggle.SetChecked(0);
    SetChildFlags(0);
}

/**
 * Purpose: Leave the network setup state when the cancel button is activated.
 */
void HudUiNetGameSetupPanel_CancelButton::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_leaveNetworkState,
        0
    );
    HudUiZrdWidget::OnActivate();
}

/**
 * Purpose: Run typed numeric-input construction, allocate the requested digit
 * buffer, clear the display text, and leave keyboard input inactive.
 */
HudUiNumericTextInput * HudUiNumericTextInput::Constructor(
    unsigned int maxDigits
) {
    BaseConstructor();
    textInput.AllocTextBuffer(maxDigits);
    Update("");
    SetInputActive(0);
    return this;
}

/**
 * Purpose: constructs the numeric input base, allocates a digit buffer, clears
 * the display, disables capture, and initializes the signed 32-bit clamp range.
 */
HudUiClampedIntTextInput::HudUiClampedIntTextInput(
    unsigned int maxDigits
) {
    textInput.AllocTextBuffer(maxDigits + 1);
    Update("");
    SetInputActive(0);
    minValue = -2147483647 - 1;
    maxValue = 2147483647;
}

/**
 * Purpose: handle the recovered HUD event path for HudUiNumericTextInput::OnAcceptForwardToCommit.
 */
int HudUiNumericTextInput::OnAcceptForwardToCommit() {
    return CommitAndGetValue();
}

/**
 * Purpose: handle the recovered HUD event path for HudUiClampedIntTextInput::OnRawKeyboardChar.
 */
int HudUiClampedIntTextInput::OnRawKeyboardChar(
    int key
) {
    if (strchr(
        kClampedIntTextInputAcceptedRawKeyChars,
        key
    ) != 0) {
        textInput.DispatchKeyAction(key);
    }

    return 0;
}

/**
 * Purpose: preserve the recovered HUD behavior for HudUiClampedIntTextInput::CommitAndGetValue.
 */
int HudUiClampedIntTextInput::CommitAndGetValue() {
    char *const text = GetBuffer();
    int value;

    if (text == 0 || *text == 0) {
        value = minValue;
    } else {
        value = atoi(text);
    }

    if (value < minValue) {
        value = minValue;
    }

    if (value > maxValue) {
        value = maxValue;
    }

    int displayValue = value;
    if (displayValue < minValue) {
        displayValue = minValue;
    }

    if (displayValue > maxValue) {
        displayValue = maxValue;
    }

    char valueText[20];
    sprintf(
        valueText,
        "%d",
        displayValue
    );
    Update(valueText);
    return value;
}

/**
 * Binary Ninja source file D:\Proj\Battlesport\hud.cpp shows the target-input
 * guard, virtual commit slot, signed step/clamp, numeric text update, target
 * invalidate slot, then HudUiZrdWidget activation.
 * Purpose: commit the linked clamped integer input, apply this button's step,
 * clamp/display the result, invalidate the input, and run base activation.
 */
void HudUiClampedIntStepButton::OnActivate() {
    if (targetInput != 0) {
        int value = targetInput->CommitAndGetValue() + stepDelta;

        if (value < targetInput->minValue) {
            value = targetInput->minValue;
        }

        if (value > targetInput->maxValue) {
            value = targetInput->maxValue;
        }

        char valueText[20];
        sprintf(
            valueText,
            "%d",
            value
        );
        targetInput->Update(valueText);
        targetInput->Invalidate();
    }

    HudUiZrdWidget::OnActivate();
}

/**
 * Purpose: Tear down the panel-owned controls before destroying the background base.
 */
HudUiNetGameSetupPanel::~HudUiNetGameSetupPanel() {
}

/**
 * Purpose: Commit setup values and start or reconfigure the network game session.
 */
void HudUiNetGameSetupPanel_LaunchButton::OnActivate() {
    HudUiNetGameSetupPanel *const ownerPanel = (HudUiNetGameSetupPanel *)(owner);
    int statusFlags = 0;
    HudUiZrdWidget::OnActivate();

    if (ownerPanel->allowMapsToggle.checked != 0) {
        statusFlags = 1;
    }
    if (ownerPanel->nameTagsToggle.checked != 0) {
        statusFlags |= 2;
    }

    HudUiClampedIntTextInput *const killsInput = &ownerPanel->killsInput;
    if (zOpt::GetNetworkModemEnabled() == 0 && ownerPanel->reconfigureExistingSession == 0) {
        zNetworkSessionDescStatusFields statusFields;
        statusFields.eventCode = ownerPanel->worldSelector.selectedIndex + 1;
        statusFields.statusFlags = statusFlags;
        statusFields.valueOrTime = ownerPanel->timeLimitInput.CommitAndGetValue();
        statusFields.auxParam = killsInput->CommitAndGetValue();
        statusFields.maxPlayers = ownerPanel->maxPlayersInput.CommitAndGetValue();
        strcpy(
            statusFields.sessionNameBuf,
            ownerPanel->gameNameInput.GetBuffer()
        );

        if (zNetwork_DPlay::CreateSessionFromStatusFields(&statusFields) != 0) {
            zOpt::SetNetworkEnabled(1);
            zNetwork_DPlay::CreateLocalPlayerRecordAndRegister(zOpt_GetPlayerName());
        }
    } else {
        const int auxParam = killsInput->CommitAndGetValue();
        const int valueOrTime = ownerPanel->timeLimitInput.CommitAndGetValue();
        GameNet::SendPkt14_HudTimerAndFlagsSync(
            ownerPanel->worldSelector.selectedIndex + 1,
            statusFlags,
            valueOrTime,
            auxParam
        );
        if (zNetwork::IsHost() != 0) {
            GameNet::HostUpdateSessionDescStatusFields(
                ownerPanel->worldSelector.selectedIndex + 1,
                killsInput->CommitAndGetValue(),
                ownerPanel->timeLimitInput.CommitAndGetValue(),
                statusFlags
            );
        }

        GameNet::UnregisterGameplayPacketHandlers();
        GameNet::ResetRemotePlayersAndSpawnLists();
    }

    g_RecoilApp.m_skipIntroFmv = 1;
    GameNet::SetStatusBitsFromFlags(statusFlags);

    const int goalValue = ownerPanel->killsInput.CommitAndGetValue();
    const int timeLimitMinutes = ownerPanel->timeLimitInput.CommitAndGetValue();
    union TimerSecondsRaw {
        float seconds;
        int raw;
    } timerSeconds = {(float)(timeLimitMinutes) * 60.0f};
    g_HudSensorTracker.SetRuntimeTimerSecAndGoalValue(
        timerSeconds.raw,
        goalValue
    );

    CZRecoilFrame *const mainWnd = (CZRecoilFrame *)((unsigned int)(g_RecoilApp.GetMainWnd()));
    g_HudSensorTracker.InitMissionIdAndFlags(
        ownerPanel->worldSelector.selectedIndex + 7,
        mainWnd->m_useArchiveBanks
    );
    g_RecoilApp.QueueExitCurrentState(0);
}

/**
 * Purpose: Move network setup text focus to this numeric input, release the
 * previous raw-keyboard capture, refresh text and cursor state, then activate.
 */
void HudUiNetGameSetupTextInput::OnActivateFocusAndCursor() {
    HudUiNetGameSetupPanel *const ownerPanel =
        (HudUiNetGameSetupPanel *)HudUiZrdWidget::owner;
    HudUiNumericTextInput **const focusTextInputSlot =
        &ownerPanel->currentFocusWidget;
    HudUiNumericTextInput *const previousFocusTextInput = *focusTextInputSlot;

    if (previousFocusTextInput != 0) {
        previousFocusTextInput->CommitAndGetValue();
        previousFocusTextInput->SetRawKeyboardCapture(0);
    }

    *focusTextInputSlot = this;
    SetRawKeyboardCapture(1);
    Update(GetBuffer());
    textInput.SetCursorPosition((int)(strlen(GetBuffer())));
    HudUiNumericTextInput::OnActivate();
}

/**
 * Purpose: preserve the recovered HUD behavior for HudUiNetGameSetupOverlayOwner::StaticInitAndRegisterAtExit.
 */
void HudUiNetGameSetupTextInput::OnActivate() {
    OnActivateFocusAndCursor();
}

/**
 * Purpose: Advance the selected world and apply the related setup side effects.
 */
void HudUiNetGameSetupPanel_NextWorldButton::OnActivate() {
    HudUiNetGameSetupPanel *const ownerPanel = (HudUiNetGameSetupPanel *)(owner);
    ownerPanel->worldSelector.SetIndexClamped(ownerPanel->worldSelector.selectedIndex + 1);

    HudUiClampedIntTextInput *killsInput;
    if (ownerPanel->worldSelector.selectedIndex == 2) {
        SetWidgetVisible(
            &ownerPanel->killsSwitch,
            0
        );
        SetWidgetVisible(
            &ownerPanel->lapsSwitch,
            1
        );

        killsInput = &ownerPanel->killsInput;
        if (killsInput->CommitAndGetValue() == 1) {
            char valueText[20];
            int clampedValue = 2;
            if (killsInput->minValue > clampedValue) {
                clampedValue = killsInput->minValue;
            }
            if (clampedValue > killsInput->maxValue) {
                clampedValue = killsInput->maxValue;
            }
            sprintf(
                valueText,
                "%d",
                clampedValue
            );
            killsInput->Update(valueText);
        }
        killsInput->minValue = 2;
        killsInput->maxValue = 99;

        SetZrdWidgetEnabled(
            &ownerPanel->incTimeLimitButton,
            0
        );
        SetZrdWidgetEnabled(
            &ownerPanel->decTimeLimitButton,
            0
        );
    } else {
        SetWidgetVisible(
            &ownerPanel->killsSwitch,
            1
        );
        SetWidgetVisible(
            &ownerPanel->lapsSwitch,
            0
        );
        killsInput = &ownerPanel->killsInput;
        killsInput->minValue = 1;
        killsInput->maxValue = 99;

        SetZrdWidgetEnabled(
            &ownerPanel->timeLimitInput,
            1
        );
        SetZrdWidgetEnabled(
            &ownerPanel->incTimeLimitButton,
            1
        );
        SetZrdWidgetEnabled(
            &ownerPanel->decTimeLimitButton,
            1
        );
    }

    killsInput->Invalidate();
    ownerPanel->incKillsButton.Invalidate();
    ownerPanel->decKillsButton.Invalidate();
    HudUiZrdWidget::OnActivate();
}

/**
 * Purpose: Move to the previous world and apply the related setup side effects.
 */
void HudUiNetGameSetupPanel_PrevWorldButton::OnActivate() {
    HudUiNetGameSetupPanel *const ownerPanel = (HudUiNetGameSetupPanel *)(owner);
    ownerPanel->worldSelector.SetIndexClamped(ownerPanel->worldSelector.selectedIndex - 1);

    HudUiClampedIntTextInput *killsInput;
    if (ownerPanel->worldSelector.selectedIndex == 2) {
        SetWidgetVisible(
            &ownerPanel->killsSwitch,
            0
        );
        SetWidgetVisible(
            &ownerPanel->lapsSwitch,
            1
        );

        killsInput = &ownerPanel->killsInput;
        if (killsInput->CommitAndGetValue() == 1) {
            char valueText[20];
            int clampedValue = 2;
            if (killsInput->minValue > clampedValue) {
                clampedValue = killsInput->minValue;
            }
            if (clampedValue > killsInput->maxValue) {
                clampedValue = killsInput->maxValue;
            }
            sprintf(
                valueText,
                "%d",
                clampedValue
            );
            killsInput->Update(valueText);
        }
        killsInput->minValue = 2;
        killsInput->maxValue = 99;

        SetZrdWidgetEnabled(
            &ownerPanel->incTimeLimitButton,
            0
        );
        SetZrdWidgetEnabled(
            &ownerPanel->decTimeLimitButton,
            0
        );
    } else {
        SetWidgetVisible(
            &ownerPanel->killsSwitch,
            1
        );
        SetWidgetVisible(
            &ownerPanel->lapsSwitch,
            0
        );
        killsInput = &ownerPanel->killsInput;
        killsInput->minValue = 1;
        killsInput->maxValue = 99;

        SetZrdWidgetEnabled(
            &ownerPanel->timeLimitInput,
            1
        );
        SetZrdWidgetEnabled(
            &ownerPanel->incTimeLimitButton,
            1
        );
        SetZrdWidgetEnabled(
            &ownerPanel->decTimeLimitButton,
            1
        );
    }

    killsInput->Invalidate();
    ownerPanel->incKillsButton.Invalidate();
    ownerPanel->decKillsButton.Invalidate();
    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-huduinetgamesetupoverlayowner
 * @recoil-artifact defines .data recoil:data:0x4f32a0: g_HudUiNetGameSetupOverlayOwner.
 * Purpose: own the process-global multiplayer setup overlay state through its
 * natural C++ static lifetime.
 */
HudUiNetGameSetupOverlayOwner g_HudUiNetGameSetupOverlayOwner;

/**
 * @recoil-anchor recoil:anchor:battlesport-mission-huduinetgamesetupoverlayowner-constructor
 * @recoil-artifact defines .text recoil:function:0x41aba0: HudUiNetGameSetupOverlayOwner constructor.
 * BN source path: D:\Proj\Battlesport\HudUi.cpp.
 * Purpose: initialize the overlay owner state with no active setup panel and
 * no pending reconfigure request.
 */
HudUiNetGameSetupOverlayOwner::HudUiNetGameSetupOverlayOwner()
    : m_reconfigureExistingSession(0) {
    m_dialog = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-mission-huduinetgamesetupoverlayowner-destructor
 * @recoil-artifact defines .text recoil:function:0x41abe0: HudUiNetGameSetupOverlayOwner destructor.
 * BN source path: D:\Proj\Battlesport\HudUi.cpp.
 * Purpose: disable and delete any live multiplayer setup panel before clearing
 * the owner singleton's panel pointer.
 */
HudUiNetGameSetupOverlayOwner::~HudUiNetGameSetupOverlayOwner() {
    HudUiNetGameSetupPanel *panel = (HudUiNetGameSetupPanel *)m_dialog;
    if (panel != 0) {
        panel->SetEnabled(0);

        panel = (HudUiNetGameSetupPanel *)m_dialog;
        if (panel != 0) {
            delete panel;
        }

        m_dialog = 0;
    }
}

/**
 * BN source path: D:\Proj\Battlesport\HudUi.cpp.
 * Purpose: configure HUD video and dialog audio state, create and enable the
 * network setup panel, then start the menu CD track when enabled.
 */
int HudUiNetGameSetupOverlayOwner::OnTryBecomeCurrent() {
    zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);
    HudUi::SetInvalidateMode(0);

    const int pitchBytes = zVideo::GetPrimarySurfacePitch();
    const int bitsPerPixel = zOpt::GetDisplaySectionBitsPerPixel();
    zOpt_ViewRectSection *const activeRegionRect = zOpt::GetWindowSection();
    zRndr::SetFrameBufferRegion(
        zVideo::GetPrimarySurfacePixels(),
        activeRegionRect,
        bitsPerPixel,
        pitchBytes
    );

    zSndSampleSet_InitByName(g_HudUiDialogSampleSetName);

    HudUiNetGameSetupPanel *panel =
        (HudUiNetGameSetupPanel *) ::operator new(sizeof(HudUiNetGameSetupPanel));
    if (panel != 0) {
        panel = new (panel) HudUiNetGameSetupPanel(m_reconfigureExistingSession);
    }

    m_dialog = panel;
    panel->SetEnabled(1);

    if (zSnd::GetCDAudioOption() != 0) {
        zSndCd::PlayTrackWithMode(
            2,
            5
        );
    }

    return 1;
}

/**
 * BN source path: D:\Proj\Battlesport\HudUi.cpp.
 * Purpose: tear down dialog audio and the active setup panel while restoring
 * the primary surface after the multiplayer setup overlay exits.
 */
void HudUiNetGameSetupOverlayOwner::OnDeactivate() {
    Sleep(1000);
    zSndSampleSet_DestroyByName(g_HudUiDialogSampleSetName);

    HudUiNetGameSetupPanel *panel = (HudUiNetGameSetupPanel *)m_dialog;
    if (panel == 0) {
        return;
    }

    zVideo::RunPostprocessOnPrimaryBuffer();

    panel = (HudUiNetGameSetupPanel *)m_dialog;
    panel->SetEnabled(0);

    ((HudUiDialogController *)m_dialog)->BlitOwnedSurfaceToPrimary();
    zVideo::Dispatch_UnlockPrimarySurfaceState();

    panel = (HudUiNetGameSetupPanel *)m_dialog;
    if (panel != 0) {
        delete panel;
    }

    m_dialog = 0;
}

/**
 * BN source path: D:\Proj\GameZRecoil\zHud\HudUiNetGameSetup.cpp.
 * Purpose: store the requested reconfigure mode on the static overlay owner
 * and queue that owner as the next application state.
 */
void __fastcall HudUiNetGameSetupOverlayOwner::QueueEnterWithReconfigureFlag(
    int reconfigureExistingSession
) {
    g_HudUiNetGameSetupOverlayOwner.m_reconfigureExistingSession = reconfigureExistingSession;
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_HudUiNetGameSetupOverlayOwner,
        0
    );
}
#include "Battlesport/game_net.h"

#include "Battlesport/briefing.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/net_ui.h"
#include "Battlesport/recoil_app.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "Battlesport/mission.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "Battlesport/turret.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zVideo/zvid.h"

#if defined(_MSC_VER) && _MSC_VER < 1300 && !defined(_DEBUG)
/**
 * Original-source inline provider-boundary restore from MFC42 AFXCMN.INL:
 * _AFXCMN_INLINE CSpinButtonCtrl::CSpinButtonCtrl() { }.
 * No standalone Recoil-authored retail function exists; this source restores
 * the VC5/MFC42 common-control inline suppressed by Mfc42Abi.h so the config
 * dialog emits the retail local spin-control vftable reference.
 * Purpose: Construct embedded MFC42 spin-button controls with provider inline
 * behavior for NetSessionConfigDialog.
 */
inline CSpinButtonCtrl::CSpinButtonCtrl() {
}
#endif

#include <shellapi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_MSC_VER) || _MSC_VER >= 1300
#include <new>
#endif


/**
 * Provider-boundary accessor for imported MFC42 CDialog metadata; this does not reimplement
 * CDialog behavior.
 */
class NetSessionBrowserCDialogMessageMapAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *__stdcall GetMessageMap();
};

/**
 * Provider-boundary accessor for imported MFC42 CDialog metadata; this does not reimplement
 * CDialog behavior.
 */
class NetSessionConfigCDialogMessageMapAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *__stdcall GetMessageMap();
};

/**
 * Provider-boundary accessor for imported MFC42 protected window/dialog members; this does
 * not reimplement provider behavior.
 */
class GameNetMfcWndAccess : public CWnd {
  public:
    long CallDefault();
    void CallOnDestroy();
};

/**
 * Provider-boundary accessor for imported MFC42 protected dialog members; this does not
 * reimplement provider behavior.
 */
class GameNetMfcDialogAccess : public CDialog {
  public:
    void CallOnOK();
};

void __stdcall DDX_Control(
    CDataExchange *dataExchange,
    int controlId,
    CWnd &control
);
void __stdcall DDX_Text(
    CDataExchange *dataExchange,
    int controlId,
    CString &value
);
void __stdcall DDX_Text(
    CDataExchange *dataExchange,
    int controlId,
    unsigned int &value
);
void __stdcall DDX_Check(
    CDataExchange *dataExchange,
    int controlId,
    int &value
);
void __stdcall DDV_MaxChars(
    CDataExchange *dataExchange,
    const CString &value,
    int maxChars
);
void __stdcall DDV_MinMaxUInt(
    CDataExchange *dataExchange,
    unsigned int value,
    unsigned int minValue,
    unsigned int maxValue
);

RECOIL_STATIC_ASSERT(sizeof(CWnd) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CDialog) == 0x60);
RECOIL_STATIC_ASSERT(sizeof(CEdit) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CButton) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CListBox) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CComboBox) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CSpinButtonCtrl) == 0x40);

extern "C" {
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-hud-triplestringfmt
 * @recoil-artifact defines .data recoil:data:0x4dae34: g_Hud_TripleStringFmt.
 * Purpose: Stores the writable sprintf format used for multiplayer
 * kill-feed top-message lines.
 */
char g_Hud_TripleStringFmt[9] = "%s %s %s";
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-gamenetplayerrowlist
 * @recoil-artifact defines .data recoil:data:0x4f3f10: g_GameNetPlayerRowList.
 * Purpose: Owns the multiplayer player-row linked-list header for active
 * local and remote network participants.
 */
GameNetPlayerRowListState g_GameNetPlayerRowList = {0, 0, 0, 0};
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-gamenetspawnpointlist
 * @recoil-artifact defines .data recoil:data:0x4f3f78: g_GameNetSpawnPointList.
 * Purpose: Owns the multiplayer spawn-point linked-list header loaded from
 * net.zrd during network mission startup.
 */
GameNetSpawnPointListState g_GameNetSpawnPointList = {0, 0, 0, 0};
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-gamenetplayerrowstylecolors-00rrggbb
 * @recoil-artifact defines .data recoil:data:0x4dcd88: g_GameNetPlayerRowStyleColors_00RRGGBB.
 * Purpose: Maps network player color indices to HUD row and player tint
 * colors in packed 00RRGGBB order.
 */
unsigned int g_GameNetPlayerRowStyleColors_00RRGGBB[9] = {
    0x00000000,
    0x000000ff,
    0x0000ff00,
    0x00ff0000,
    0x00ff00ff,
    0x00ffff00,
    0x0000ffff,
    0x00ffffff,
    0x000040ff,
};
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-hudtimerpanelnetstate
 * @recoil-artifact defines .data recoil:data:0x4f3f20: g_HudTimerPanelNetState.
 * Purpose: Stores replicated multiplayer HUD timer state and resend/warning
 * flags shared by GameNet timer packet handlers.
 */
HudTimerPanelNetState g_HudTimerPanelNetState = {0};
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-netpkt0c-hudtimerstatusbitsbuf
 * @recoil-artifact defines .data recoil:data:0x4dce88: g_NetPkt0C_HudTimerStatusBitsBuf.
 * Purpose: Holds the reusable network packet buffer for HUD timer status-bit
 * replication.
 */
NetPkt0C_HudTimerStatusBits g_NetPkt0C_HudTimerStatusBitsBuf = {
    {0x0c, sizeof(NetPkt0C_HudTimerStatusBits), 0},
    0.0f,
    0,
    0,
    0,
};
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-netpkt0d-hudtimerpanelstatebuf
 * @recoil-artifact defines .data recoil:data:0x4dce78: g_NetPkt0D_HudTimerPanelStateBuf.
 * Purpose: Holds the reusable network packet buffer for HUD timer panel state
 * replication.
 */
NetPkt0D_HudTimerPanelState g_NetPkt0D_HudTimerPanelStateBuf = {
    {0x0d, sizeof(NetPkt0D_HudTimerPanelState), 0},
    0.0f,
    0,
    0,
};
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-netpkt14-hudtimerandflagssyncbuf
 * @recoil-artifact defines .data recoil:data:0x4dcfa0: g_NetPkt14_HudTimerAndFlagsSyncBuf.
 * Purpose: Holds the reusable network packet buffer for HUD timer and status
 * flag synchronization.
 */
NetPkt14_HudTimerAndFlagsSync g_NetPkt14_HudTimerAndFlagsSyncBuf = {
    {0x14, sizeof(NetPkt14_HudTimerAndFlagsSync), 0},
    0,
    0,
    0,
    0,
};
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-netpkt0a-optcatalogprocessruntimerelaybuf
 * @recoil-artifact defines .data recoil:data:0x4dcf80: g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.
 * Purpose: Holds the reusable packet buffer for OptCatalog runtime relay
 * removal events.
 */
NetPkt0A_RemoveRuntimeRelay g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf = {
    {0x0a, sizeof(NetPkt0A_RemoveRuntimeRelay), 0},
    0,
    0,
    {0.0f, 0.0f, 0.0f},
    0,
};
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-netpkt07-altgundispatchbuf
 * @recoil-artifact defines .data recoil:data:0x4dcf60: g_NetPkt07_AltGunDispatchBuf.
 * Purpose: Stores g NetPkt07 AltGunDispatchBuf data used by network_online.gamenet_pkt07_packet_buffer_data.
 */
NetPkt07_AltGunDispatch g_NetPkt07_AltGunDispatchBuf = {
    {0x07, sizeof(NetPkt07_AltGunDispatch), 0},
    0,
    0,
    0,
    {0.0f, 0.0f, 0.0f},
};
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-netpkt06-playerstatesnapshotbuf
 * @recoil-artifact defines .data recoil:data:0x4dcdb0: g_NetPkt06_PlayerStateSnapshotBuf.
 * Purpose: Holds the reusable local player-state snapshot buffer for packet 6
 * replication.
 */
NetPkt06_PlayerStateSnapshot g_NetPkt06_PlayerStateSnapshotBuf = {
    {0x06, 0, 0},
};
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-netpkt0f-cratereventrelaybuf
 * @recoil-artifact defines .data recoil:data:0x4dcea0: g_NetPkt0F_CraterEventRelayBuf.
 * Purpose: Holds the reusable crater packet used by zDEClient_Crater::Execute
 * before host relay or reliable send.
 */
NetPkt0F_CraterEvent g_NetPkt0F_CraterEventRelayBuf = {
    {0x0f, sizeof(NetPkt0F_CraterEvent), 0},
    0,
    0,
    {0.0f, 0.0f, 0.0f},
    0.0f,
};
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-netpkt0f-cratereventsendbuf
 * @recoil-artifact defines .data recoil:data:0x4dcec0: g_NetPkt0F_CraterEventSendBuf.
 * Purpose: Holds the reusable network packet buffer for crater feature
 * events.
 */
NetPkt0F_CraterEvent g_NetPkt0F_CraterEventSendBuf = {
    {0x0f, sizeof(NetPkt0F_CraterEvent), 0},
    0,
    0,
    {0.0f, 0.0f, 0.0f},
    0.0f,
};
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-netpkt10-qsandeventrelaybuf
 * @recoil-artifact defines .data recoil:data:0x4dcee0: g_NetPkt10_QSandEventRelayBuf.
 * Purpose: Holds the reusable quicksand packet used by GameNet local event
 * relay before host callback or reliable send.
 */
NetPkt10_QSandEvent g_NetPkt10_QSandEventRelayBuf = {
    {0x10, sizeof(NetPkt10_QSandEvent), 0},
    0,
    0,
    {0.0f, 0.0f, 0.0f},
    0.0f,
};
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-netpkt10-qsandeventsendbuf
 * @recoil-artifact defines .data recoil:data:0x4dcf00: g_NetPkt10_QSandEventSendBuf.
 * Purpose: Holds the reusable network packet buffer for quicksand feature
 * events.
 */
NetPkt10_QSandEvent g_NetPkt10_QSandEventSendBuf = {
    {0x10, sizeof(NetPkt10_QSandEvent), 0},
    0,
    0,
    {0.0f, 0.0f, 0.0f},
    0.0f,
};
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-gamenetonelapleftmessageshown
 * @recoil-artifact defines .data recoil:data:0x4f3f8c: g_GameNetOneLapLeftMessageShown.
 * Purpose: Caches whether the local race HUD has already shown the one-lap
 * remaining network message.
 */
int g_GameNetOneLapLeftMessageShown = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-gamenetstatus-allowmaps
 * @recoil-artifact defines .data recoil:data:0x4f3f88: g_GameNetStatus_AllowMaps.
 * Purpose: Stores the replicated session status bit that allows map use.
 */
int g_GameNetStatus_AllowMaps = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-gamenetstatus-nametags
 * @recoil-artifact defines .data recoil:data:0x4f3f90: g_GameNetStatus_NameTags.
 * Purpose: Stores the replicated session status bit that enables name tags.
 */
int g_GameNetStatus_NameTags = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-gamenetallplayerslaptargetcheckstarted
 * @recoil-artifact defines .data recoil:data:0x4f3fa0: g_GameNetAllPlayersLapTargetCheckStarted.
 * Purpose: Tracks whether the multiplayer race goal completion check has
 * started for the current session.
 */
int g_GameNetAllPlayersLapTargetCheckStarted = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-gamenetsuppresspkt13activationecho
 * @recoil-artifact defines .data recoil:data:0x4f3fa8: g_GameNetSuppressPkt13ActivationEcho.
 * Purpose: Suppresses local echo while replaying replicated effect animation
 * activation records.
 */
int g_GameNetSuppressPkt13ActivationEcho = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-gamenetpkt06initialsyncgate
 * @recoil-artifact defines .data recoil:data:0x4dcdac: g_GameNetPkt06InitialSyncGate.
 * Purpose: Blocks local pkt06 replication until the first multiplayer state
 * synchronization has been initialized.
 */
int g_GameNetPkt06InitialSyncGate = 1;
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-gamenetpkt06inputbit17latch
 * @recoil-artifact defines .data recoil:data:0x4f3f6c: g_GameNetPkt06InputBit17Latch.
 * Purpose: Accumulates local pkt06 input bit 17 until the next player-state
 * snapshot is sent.
 */
int g_GameNetPkt06InputBit17Latch = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-gamenetpkt06inputbit16latch
 * @recoil-artifact defines .data recoil:data:0x4f3f70: g_GameNetPkt06InputBit16Latch.
 * Purpose: Accumulates local pkt06 input bit 16 until the next player-state
 * snapshot is sent.
 */
int g_GameNetPkt06InputBit16Latch = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-gamenetpkt06nextsendtimesec
 * @recoil-artifact defines .data recoil:data:0x4f3f9c: g_GameNetPkt06NextSendTimeSec.
 * Purpose: Stores the next accumulated-time deadline for local pkt06 state
 * snapshot transmission.
 */
float g_GameNetPkt06NextSendTimeSec = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-gamenethosthudtimerinitflag
 * @recoil-artifact defines .data recoil:data:0x4f3f98: g_GameNetHostHudTimerInitFlag.
 * Purpose: Tracks host-side HUD timer initialization during multiplayer
 * mission startup.
 */
int g_GameNetHostHudTimerInitFlag = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-gamenethudtimertensecondwarningarmed
 * @recoil-artifact defines .data recoil:data:0x4dce70: g_GameNetHudTimerTenSecondWarningArmed.
 * Purpose: Arms the replicated HUD timer ten-second warning message.
 */
int g_GameNetHudTimerTenSecondWarningArmed = 1;
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-gamenethudtimerpendingsavereminderarmed
 * @recoil-artifact defines .data recoil:data:0x4dce74: g_GameNetHudTimerPendingSaveReminderArmed.
 * Purpose: Arms the replicated HUD timer pending-save reminder message.
 */
int g_GameNetHudTimerPendingSaveReminderArmed = 1;
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-gamenet-handlersregistered
 * @recoil-artifact defines .data recoil:data:0x4f3fa4: g_GameNet_HandlersRegistered.
 * Purpose: Records whether GameNet gameplay packet handlers and callback
 * hooks are registered.
 */
int g_GameNet_HandlersRegistered = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-netuitcpipproviderwarningshown
 * @recoil-artifact defines .data recoil:data:0x4f32b4: g_NetUiTcpIpProviderWarningShown.
 * Purpose: Records whether the Net UI has already shown the TCP/IP provider
 * warning for the current UI lifetime.
 */
int g_NetUiTcpIpProviderWarningShown = 0;
}

extern "C" HWND g_RecoilApp_hWndMain;

/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-netsessionconfigdialog-mapnamestrings
 * @recoil-artifact defines .data recoil:data:0x4f32d8: g_NetSessionConfigDialog_MapNameStrings.
 * Purpose: Stores the seven static CString objects used by the multiplayer
 * session configuration map list.
 */
unsigned int g_NetSessionConfigDialog_MapNameStringStorage[7] = {0};
CString *g_NetSessionConfigDialog_MapNameStrings =
    (CString *)&g_NetSessionConfigDialog_MapNameStringStorage[0];

namespace {
const UINT kNetSessionBrowserDialogResourceId = 136;
const int kNetSessionBrowserPlayerNameEditId = 1048;
const int kNetSessionBrowserOkButtonId = 1;
const int kNetSessionBrowserHelpButtonId = 1029;
const int kNetSessionBrowserCreateSessionButtonId = 1030;
const int kNetSessionBrowserSessionListId = 1040;
const int kNetSessionBrowserProviderComboId = 1114;
const int kNetSessionBrowserPlayerNameMaxChars = 21;
const unsigned int kNetSessionBrowserHelpCaptionMessageId = 25;
const unsigned int kNetSessionBrowserHelpNoAssociationMessageId = 32;
const unsigned int kNetSessionBrowserHelpNoDdeAssociationMessageId = 33;
const unsigned int kNetSessionBrowserHelpFileNotFoundMessageId = 34;
const unsigned int kNetSessionBrowserHelpAssociationIncompleteMessageId = 36;
const unsigned int kNetSessionBrowserPlayerNameRequiredMessageId = 23;
const unsigned int kNetSessionBrowserPlayerNameCaptionMessageId = 24;
const unsigned int kNetSessionBrowserNoProviderMessageId = 273;
const unsigned int kNetSessionBrowserTcpIpWarningCaptionMessageId = 18;
const unsigned int kNetSessionBrowserTcpIpWarningFormatMessageId = 38;
const unsigned int kNetSessionBrowserModemOkButtonMessageId = 53;
const unsigned int kNetSessionBrowserModemCreateButtonMessageId = 54;
const unsigned int kNetSessionBrowserJoinButtonMessageId = 55;
const unsigned int kNetSessionBrowserRefreshButtonMessageId = 56;
const UINT kNetSessionConfigDialogResourceId = 146;
const int kNetSessionConfigMaxPlayersSpinId = 1072;
const int kNetSessionConfigSessionNameEditId = 1115;
const int kNetSessionConfigMapComboId = 1116;
const int kNetSessionConfigValueLimitEditId = 1117;
const int kNetSessionConfigTimeLimitEditId = 1118;
const int kNetSessionConfigMaxPlayersEditId = 1119;
const int kNetSessionConfigTimeLimitSpinId = 1120;
const int kNetSessionConfigValueLimitSpinId = 1121;
const int kNetSessionConfigUnusedCheckboxId = 1122;
const int kNetSessionConfigMaxPlayersLabelId = 1125;
const int kNetSessionConfigSessionNameMaxChars = 80;
const unsigned int kNetSessionConfigLimitMax = 10000;
const unsigned int kNetSessionConfigMaxPlayersMin = 2;
const unsigned int kNetSessionConfigMaxPlayersMax = 8;
const unsigned int kNetSessionConfigMaxPlayersSpecialMapMessageId = 12352;
const unsigned int kNetSessionConfigMaxPlayersDefaultMessageId = 12353;
const int kNetSessionConfigSpecialMapIndex = 2;
const int kNetSessionConfigMapNameCount = 7;
const char kNetSessionConfigSessionNameFormat[] = "Exercise %03d";
const UINT kNetSessionConfigSpinSetRangeMessage = 1125;
const unsigned int kNetSessionConfigDefaultValueLimit = 5;
const unsigned int kNetSessionConfigDefaultTimeLimitMinutes = 10;
const unsigned int kNetSessionConfigDefaultMaxPlayers = 8;
const int kNetSessionBrowserModemEventCode = 256;
const int kNetSessionBrowserModemValueOrTime = 10;
const int kNetSessionBrowserModemAuxParam = 10;
const int kNetSessionBrowserModemMaxPlayers = 2;

} // namespace

/**
 * Provider-boundary accessor for imported MFC42 CDialog message-map metadata.
 * Purpose: Return the provider CDialog message map for the browser dialog chain.
 */
const AFX_MSGMAP *__stdcall NetSessionBrowserCDialogMessageMapAccessor::GetMessageMap() {
    return &CDialog::messageMap;
}

/**
 * Original helper evidence: no standalone retail function; MFC message-map
 * tables reference this base-map accessor for NetSessionBrowserDialog.
 * Purpose: Return the browser dialog base message map.
 */
const AFX_MSGMAP *__stdcall NetSessionBrowserDialog::GetBaseMessageMapForMfc() {
    return NetSessionBrowserCDialogMessageMapAccessor::GetMessageMap();
}

AFX_MSGMAP_ENTRY const NetSessionBrowserDialog::messageEntries[] = {
    {WM_COMMAND,
        CBN_CLOSEUP,
        kNetSessionBrowserProviderComboId,
        kNetSessionBrowserProviderComboId,
        12,
        (AFX_PMSG)&NetSessionBrowserDialog::ConnectSelectedProvider},
    {WM_COMMAND,
        BN_CLICKED,
        kNetSessionBrowserCreateSessionButtonId,
        kNetSessionBrowserCreateSessionButtonId,
        12,
        (AFX_PMSG)&NetSessionBrowserDialog::OnCreateSession},
    {WM_TIMER, 0, 0, 0, 13, (AFX_PMSG)&NetSessionBrowserDialog::OnTimer},
    {WM_DESTROY, 0, 0, 0, 12, (AFX_PMSG)&NetSessionBrowserDialog::OnDestroy},
    {WM_COMMAND,
        BN_CLICKED,
        kNetSessionBrowserHelpButtonId,
        kNetSessionBrowserHelpButtonId,
        12,
        (AFX_PMSG)&NetSessionBrowserDialog::OnHelpDocs},
    {0, 0, 0, 0, 0, 0},
};

const AFX_MSGMAP NetSessionBrowserDialog::messageMap = {
    &NetSessionBrowserDialog::GetBaseMessageMapForMfc,
    &NetSessionBrowserDialog::messageEntries[0],
};

/**
 * Provider-boundary accessor for imported MFC42 CDialog message-map metadata.
 * Purpose: Return the provider CDialog message map for the config dialog chain.
 */
const AFX_MSGMAP *__stdcall NetSessionConfigCDialogMessageMapAccessor::GetMessageMap() {
    return &CDialog::messageMap;
}

/**
 * Original helper evidence: no standalone retail function; MFC dialog methods
 * in this source file share the same protected CWnd::Default dispatch.
 * Purpose: Call the provider CWnd default handler.
 */
long GameNetMfcWndAccess::CallDefault() {
    return CWnd::Default();
}

/**
 * Original helper evidence: no standalone retail function; MFC dialog methods
 * in this source file share the same protected CWnd::OnDestroy dispatch.
 * Purpose: Call the provider CWnd destroy handler.
 */
void GameNetMfcWndAccess::CallOnDestroy() {
    CWnd::OnDestroy();
}

/**
 * Original helper evidence: no standalone retail function; MFC dialog methods
 * in this source file share the same protected CDialog::OnOK dispatch.
 * Purpose: Call the provider CDialog OK handler.
 */
void GameNetMfcDialogAccess::CallOnOK() {
    CDialog::OnOK();
}

/**
 * Original helper evidence: no standalone retail function; MFC message-map
 * tables reference this base-map accessor for NetSessionConfigDialog.
 * Purpose: Return the config dialog base message map.
 */
const AFX_MSGMAP *__stdcall NetSessionConfigDialog::GetBaseMessageMapForMfc() {
    return NetSessionConfigCDialogMessageMapAccessor::GetMessageMap();
}

AFX_MSGMAP_ENTRY const NetSessionConfigDialog::messageEntries[] = {
    {WM_DESTROY, 0, 0, 0, 12, (AFX_PMSG)&NetSessionConfigDialog::OnDestroy},
    {WM_COMMAND,
        CBN_SELCHANGE,
        kNetSessionConfigMapComboId,
        kNetSessionConfigMapComboId,
        12,
        (AFX_PMSG)&NetSessionConfigDialog::OnMapChanged},
    {0, 0, 0, 0, 0, 0},
};

const AFX_MSGMAP NetSessionConfigDialog::messageMap = {
    &NetSessionConfigDialog::GetBaseMessageMapForMfc,
    &NetSessionConfigDialog::messageEntries[0],
};

/**
 * Purpose: Construct the multiplayer session browser dialog and child controls.
 */
NetSessionBrowserDialog::NetSessionBrowserDialog(
    CWnd *parentWnd
) :
    CDialog(
        kNetSessionBrowserDialogResourceId,
        parentWnd
    ),
    m_playerNameEdit(),
    m_okButton(),
    m_createSessionButton(),
    m_sessionList(),
    m_providerCombo(),
    m_playerName()
{
    m_playerName = "";
}

/**
 * Purpose: Bind browser dialog controls and validate the player-name field.
 */
void NetSessionBrowserDialog::DoDataExchange(
    CDataExchange *dataExchange
) {
    DDX_Control(
        dataExchange,
        kNetSessionBrowserPlayerNameEditId,
        *((CWnd *)&m_playerNameEdit)
    );
    DDX_Control(
        dataExchange,
        kNetSessionBrowserOkButtonId,
        *((CWnd *)&m_okButton)
    );
    DDX_Control(
        dataExchange,
        kNetSessionBrowserCreateSessionButtonId,
        *((CWnd *)&m_createSessionButton)
    );
    DDX_Control(
        dataExchange,
        kNetSessionBrowserSessionListId,
        *((CWnd *)&m_sessionList)
    );
    DDX_Control(
        dataExchange,
        kNetSessionBrowserProviderComboId,
        *((CWnd *)&m_providerCombo)
    );
    DDX_Text(
        dataExchange,
        kNetSessionBrowserPlayerNameEditId,
        m_playerName
    );
    DDV_MaxChars(
        dataExchange,
        m_playerName,
        kNetSessionBrowserPlayerNameMaxChars
    );
}

/**
 * Purpose: Return the browser dialog MFC message map.
 */
const AFX_MSGMAP * NetSessionBrowserDialog::GetMessageMap() const {
    return &NetSessionBrowserDialog::messageMap;
}

/**
 * Purpose: Initialize the multiplayer session browser controls and providers.
 */
BOOL NetSessionBrowserDialog::OnInitDialog() {
    ((CDialog *)this)->CDialog::OnInitDialog();
    m_playerName = zOpt_GetPlayerName();
    m_shouldEnterHostSetup = FALSE;
    m_sessionCount = 0;

    zNetworkServiceProviderListVec *const providerList =
        zNetworkDPlay::RefreshAndGetServiceProviderList();
    int providerCount = 0;
    if (providerList->begin != 0) {
        providerCount = (int)(providerList->end - providerList->begin);
    }

    HWND providerComboHwnd = m_providerCombo.m_hWnd;
    int providerIndex;
    for (providerIndex = 0; providerIndex < providerCount; ++providerIndex) {
        zNetworkDPlayServiceProviderInfo *const providerInfo = providerList->begin[providerIndex];
        char *const displayName = providerInfo->displayName;
        if (strstr(displayName, g_zNetwork_ProviderName_Ipx) != 0 ||
            strstr(displayName, g_zNetwork_ProviderName_TcpIp) != 0 ||
            strstr(
                displayName,
                g_zNetwork_ProviderName_Modem
            ) != 0) {
            const LRESULT comboIndex =
                ::SendMessageA(
                    providerComboHwnd,
                    CB_ADDSTRING,
                    0,
                    (LPARAM)displayName
                );
            ::SendMessageA(
                providerComboHwnd,
                CB_SETITEMDATA,
                comboIndex,
                (LPARAM)providerInfo
            );
        }
    }

    const LRESULT noProviderIndex = ::SendMessageA(
        providerComboHwnd,
        CB_ADDSTRING,
        0,
        (LPARAM)zLoc::GetMessageString(kNetSessionBrowserNoProviderMessageId)
    );
    ::SendMessageA(
        providerComboHwnd,
        CB_SETITEMDATA,
        noProviderIndex,
        0
    );
    ::SendMessageA(
        providerComboHwnd,
        CB_SETCURSEL,
        0,
        0
    );
    ((CWnd *)&m_okButton)
        ->SetWindowTextA(zLoc::GetMessageString(kNetSessionBrowserJoinButtonMessageId));
    ((CWnd *)this)->UpdateData(FALSE);
    return TRUE;
}

/**
 * Purpose: Refresh and restore the visible DirectPlay session list.
 */
int NetSessionBrowserDialog::RefreshSessionList() {
    CString selectedSessionText;
    m_sessionCount = zNetwork_DPlay::EnumSessions();

    HWND sessionListHwnd = m_sessionList.m_hWnd;
    const int selectedIndex = (int)(::SendMessageA(
        sessionListHwnd,
        LB_GETCURSEL,
        0,
        0
    ));
    if (selectedIndex != LB_ERR) {
        ((CListBox *)&m_sessionList)->GetText(
            selectedIndex,
            selectedSessionText
        );
    }

    ::SendMessageA(
        sessionListHwnd,
        LB_RESETCONTENT,
        0,
        0
    );
    for (int index = 0; index < m_sessionCount; ++index) {
        int maxPlayers = 0;
        int currentPlayers = 0;
        zNetworkDPlay::GetEnumeratedSessionPlayerCountsByIndex(
            index,
            &currentPlayers,
            &maxPlayers
        );

        char sessionText[120];
        zLoc::FormatMessage(
            sessionText,
            sizeof(sessionText),
            0x112,
            zNetworkDPlay::GetEnumeratedSessionNameByIndex(index),
            maxPlayers,
            currentPlayers
        );

        const int rowIndex =
            (int)(::SendMessageA(
                sessionListHwnd,
                LB_ADDSTRING,
                0,
                (LPARAM)sessionText
            ));
        ::SendMessageA(
            sessionListHwnd,
            LB_SETITEMDATA,
            rowIndex,
            index
        );
    }

    if (selectedIndex != LB_ERR) {
        const int restoredIndex = (int)(::SendMessageA(
            sessionListHwnd,
            LB_FINDSTRINGEXACT,
            (WPARAM)-1,
            (LPARAM)((const char *)selectedSessionText)
        ));
        if (restoredIndex != LB_ERR) {
            ::SendMessageA(
                sessionListHwnd,
                LB_SETCURSEL,
                restoredIndex,
                0
            );
            ((CWnd *)&m_okButton)->EnableWindow(TRUE);
        }
    } else if (m_sessionCount > 0) {
        ::SendMessageA(
            sessionListHwnd,
            LB_SETCURSEL,
            0,
            0
        );
        ((CWnd *)&m_okButton)->EnableWindow(TRUE);
    } else if (zOpt::GetNetworkModemEnabled() == 0) {
        ((CWnd *)&m_okButton)->EnableWindow(FALSE);
    }

    return m_sessionCount;
}

/**
 * Purpose: Connect to the selected provider and update browser dialog actions.
 */
void NetSessionBrowserDialog::ConnectSelectedProvider() {
    ::KillTimer(
        m_hWnd,
        2
    );

    HWND providerComboHwnd = m_providerCombo.m_hWnd;
    const LRESULT selectedProviderIndex = ::SendMessageA(
        providerComboHwnd,
        CB_GETCURSEL,
        0,
        0
    );
    if (selectedProviderIndex == CB_ERR) {
        return;
    }

    zNetworkDPlayServiceProviderInfo *providerInfo = (zNetworkDPlayServiceProviderInfo
            *)(::SendMessageA(
                providerComboHwnd,
                CB_GETITEMDATA,
                selectedProviderIndex,
                0
            ));
    if (providerInfo == 0) {
        ((CWnd *)&m_okButton)->EnableWindow(FALSE);
        ((CWnd *)&m_createSessionButton)->EnableWindow(FALSE);
        return;
    }

    if (strstr(
        providerInfo->displayName,
        g_zNetwork_ProviderName_TcpIp
    ) != 0 && g_NetUiTcpIpProviderWarningShown == 0) {
        g_NetUiTcpIpProviderWarningShown = 1;

        char caption[256];
        strcpy(
            caption,
            zLoc::GetMessageString(kNetSessionBrowserTcpIpWarningCaptionMessageId)
        );

        char messageFormat[256];
        strcpy(
            messageFormat,
            zLoc::GetMessageString(kNetSessionBrowserTcpIpWarningFormatMessageId)
        );

        if (NetUi::VerifyWinsock2OrPromptContinue(
            caption,
            messageFormat
        ) == 0) {
            ::SendMessageA(
                providerComboHwnd,
                CB_SETCURSEL,
                0,
                0
            );
            ((CWnd *)&m_okButton)->EnableWindow(FALSE);
            ((CWnd *)&m_createSessionButton)->EnableWindow(FALSE);
            return;
        }
    }

    zNetworkDPlay::SelectServiceProviderAndInitConnection(providerInfo);
    if (strstr(
        providerInfo->displayName,
        g_zNetwork_ProviderName_Modem
    ) == 0) {
        if (RefreshSessionList() >= 0) {
            ::SetTimer(
                m_hWnd,
                2,
                1000,
                0
            );
        }

        ((CWnd *)&m_createSessionButton)->EnableWindow(TRUE);
        ((CWnd *)&m_okButton)
            ->SetWindowTextA(zLoc::GetMessageString(kNetSessionBrowserJoinButtonMessageId));
        ((CWnd *)&m_createSessionButton)
            ->SetWindowTextA(zLoc::GetMessageString(kNetSessionBrowserRefreshButtonMessageId));
        m_selectedProviderIsModem = FALSE;
    } else {
        ::SendMessageA(
            m_sessionList.m_hWnd,
            LB_RESETCONTENT,
            0,
            0
        );
        ((CWnd *)&m_okButton)->EnableWindow(TRUE);
        ((CWnd *)&m_okButton)
            ->SetWindowTextA(zLoc::GetMessageString(kNetSessionBrowserModemOkButtonMessageId));
        ((CWnd *)&m_createSessionButton)
            ->SetWindowTextA(zLoc::GetMessageString(kNetSessionBrowserModemCreateButtonMessageId));
        ((CWnd *)&m_createSessionButton)->EnableWindow(TRUE);
        m_selectedProviderIsModem = TRUE;
    }
}

/**
 * Purpose: Join or initialize the selected multiplayer provider/session.
 */
void NetSessionBrowserDialog::OnOK() {
    int canCloseDialog = FALSE;
    if (ValidatePlayerName() == 0) {
        return;
    }

    if (m_selectedProviderIsModem == 0) {
        zOpt::SetNetworkModemEnabled(FALSE);
        ::KillTimer(
            m_hWnd,
            2
        );

        HWND sessionListHwnd = m_sessionList.m_hWnd;
        const LRESULT selectedSessionRow = ::SendMessageA(
            sessionListHwnd,
            LB_GETCURSEL,
            0,
            0
        );
        if (selectedSessionRow != LB_ERR) {
            m_selectedSessionIndex =
                (int)(::SendMessageA(
                    sessionListHwnd,
                    LB_GETITEMDATA,
                    selectedSessionRow,
                    0
                ));
            canCloseDialog = TRUE;
        }
    } else {
        zOpt::SetNetworkModemEnabled(TRUE);
        if (RefreshSessionList() >= 0) {
            canCloseDialog = TRUE;
            m_selectedSessionIndex = 0;
        }
    }

    if (canCloseDialog != 0) {
        ((GameNetMfcDialogAccess *)this)->CallOnOK();
    }
}

/**
 * Purpose: Enter host setup or create a modem session from browser state.
 */
void NetSessionBrowserDialog::OnCreateSession() {
    if (ValidatePlayerName() == 0) {
        return;
    }

    if (m_selectedProviderIsModem == 0) {
        ::KillTimer(
            m_hWnd,
            2
        );
        m_shouldEnterHostSetup = TRUE;
    } else {
        zNetworkSessionDescStatusFields statusFields;
        statusFields.eventCode = kNetSessionBrowserModemEventCode;
        statusFields.statusFlags = 0;
        statusFields.valueOrTime = kNetSessionBrowserModemValueOrTime;
        statusFields.auxParam = kNetSessionBrowserModemAuxParam;
        statusFields.maxPlayers = kNetSessionBrowserModemMaxPlayers;
        strcpy(
            statusFields.sessionNameBuf,
            g_zNetwork_ModemSessionName
        );

        if (zNetwork_DPlay::CreateSessionFromStatusFields(&statusFields) != 0) {
            zOpt::SetNetworkEnabled(TRUE);
            zOpt::SetNetworkModemEnabled(TRUE);
            zNetwork_DPlay::CreateLocalPlayerRecordAndRegister(zOpt_GetPlayerName());
            m_shouldEnterHostSetup = TRUE;
        }
    }

    if (m_shouldEnterHostSetup != 0) {
        ((GameNetMfcDialogAccess *)this)->CallOnOK();
    }
}

/**
 * Purpose: Poll for updated DirectPlay sessions while the browser is open.
 */
void NetSessionBrowserDialog::OnTimer(
    UINT_PTR
) {
    RefreshSessionList();
    ((GameNetMfcWndAccess *)this)->CallDefault();
}

/**
 * Purpose: Forward browser dialog destruction and stop session polling.
 */
void NetSessionBrowserDialog::OnDestroy() {
    ((GameNetMfcWndAccess *)this)->CallOnDestroy();
    ::KillTimer(
        m_hWnd,
        2
    );
}

/**
 * Purpose: Trim, validate, and prompt for the multiplayer player name.
 */
int NetSessionBrowserDialog::ValidatePlayerName() {
    ((CWnd *)this)->UpdateData(TRUE);
    m_playerName.TrimLeft();
    m_playerName.TrimRight();
    ((CWnd *)this)->UpdateData(FALSE);

    if (!m_playerName.IsEmpty()) {
        return TRUE;
    }

    char caption[128];
    strcpy(
        caption,
        zLoc::GetMessageString(kNetSessionBrowserPlayerNameCaptionMessageId)
    );

    char messageText[128];
    strcpy(
        messageText,
        zLoc::GetMessageString(kNetSessionBrowserPlayerNameRequiredMessageId)
    );

    ((CWnd *)this)->MessageBoxA(
        messageText,
        caption,
        MB_ICONHAND
    );
    ((CWnd *)&m_playerNameEdit)->SetFocus();
    return FALSE;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-mission-net-session-browser-dialog-on-help-docs
 * @recoil-artifact defines .text recoil:function:0x41b780: NetSessionBrowserDialog::OnHelpDocs.
 * @recoil-artifact emits .text recoil:data:0x41b898: VC5-generated five-entry switch jump table.
 * @recoil-artifact emits .text recoil:data:0x41b8ac: VC5-generated FindExecutableA result classifier table.
 * Purpose: Open the bundled help document or show the matching shell error.
 */
void NetSessionBrowserDialog::OnHelpDocs() {
    char caption[128];
    strcpy(
        caption,
        zLoc::GetMessageString(kNetSessionBrowserHelpCaptionMessageId)
    );

    char executablePath[256];
    HINSTANCE findResult = FindExecutableA(
        "Docs\\Index.html",
        0,
        executablePath
    );
    const UINT resultCode = (UINT)((UINT_PTR)(findResult));
    if (resultCode <= 31) {
        switch (resultCode) {
        case 0:
            ((CWnd *)this)
                ->MessageBoxA(
                    zLoc::GetMessageString(kNetSessionBrowserHelpNoAssociationMessageId),
                    caption,
                    MB_ICONEXCLAMATION
                );
            return;

        case 2:
        case 3:
            ((CWnd *)this)
                ->MessageBoxA(
                    zLoc::GetMessageString(kNetSessionBrowserHelpFileNotFoundMessageId),
                    caption,
                    MB_ICONEXCLAMATION
                );
            return;

        case 11:
            ((CWnd *)this)
                ->MessageBoxA(
                    zLoc::GetMessageString(kNetSessionBrowserHelpAssociationIncompleteMessageId),
                    caption,
                    MB_ICONEXCLAMATION
                );
            return;

        case 31:
            ((CWnd *)this)
                ->MessageBoxA(
                    zLoc::GetMessageString(kNetSessionBrowserHelpNoDdeAssociationMessageId),
                    caption,
                    MB_ICONEXCLAMATION
                );
            return;

        default:
            break;
        }
    }

    ShellExecuteA(
        g_RecoilApp_hWndMain,
        "open",
        "Docs\\Index.html",
        0,
        0,
        SW_HIDE
    );
}

namespace Player {

/**
 * Source owner: battlesport_gameplay.player_remote_network_tick.
 * Purpose: Ticks a remote network player from received network state and updates its gameplay presentation.
 */
void __fastcall TickRemoteNetworkPlayer(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (playerState->netUpdateReceived != 0) {
        SampleGroundAndAlignRootToSurface(
            saveState,
            0
        );
        playerState->netUpdateReceived = 0;
    }

    playerState->fxOffsetWorld.x = playerState->worldPos.x + playerState->fxOffsetLocal.x;
    playerState->fxOffsetWorld.y = playerState->worldPos.y + playerState->fxOffsetLocal.y;
    playerState->fxOffsetWorld.z = playerState->worldPos.z + playerState->fxOffsetLocal.z;

    GameNet::UpdateRemotePlayerHudWidgetScreenPos(saveState);

    if (playerState->cameraTransitionTimer != 0) {
        playerState->worldPos = playerState->netReceivedPos;
        playerState->vehiclePitchRad = playerState->netReceivedAngles.x;
        playerState->restartYawRad = playerState->netReceivedAngles.y;
        playerState->vehicleRollRad = playerState->netReceivedAngles.z;
    } else {
        zMath::Vec3Lerp(
            &playerState->worldPos,
            &playerState->netReceivedPos,
            0.649999976f
        );
        playerState->vehiclePitchRad = playerState->netReceivedAngles.x;
        playerState->restartYawRad = playerState->netReceivedAngles.y;
        playerState->vehicleRollRad = playerState->netReceivedAngles.z;

        if (playerState->lifecycleState != kPlayerLifecycleDestroyed) {
            UpdateAltGunAimDirection(saveState);
            UpdateGunDispatchRequestsFromTriggerLatches(saveState);
            TickAltGunRuntimeState(saveState);
        }

        ResetDamageVisualsAndTimedStatus(saveState);
        if (ApplyDamageLocal(saveState) != 0) {
            playerState->cameraTransitionTimer = 1;
        }
    }

    zClass_Object3D::gwObject3DSetPosition(
        playerState->rootNode,
        playerState->worldPos.x,
        playerState->worldPos.y,
        playerState->worldPos.z
    );
    zClass_Object3D::gwObject3DSetRotation(
        playerState->rootNode,
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
}

/**
 * Provisional source-placement hypothesis: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::UpdateGunDispatchRequestsFromTriggerLatches from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateGunDispatchRequestsFromTriggerLatches(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (playerState->netInputBit16Latch == 0) {
        playerState->altGunDispatchRequested = 0;
    } else if ((playerState->activeAltGunController->optCatalogEntry->flags &
                   kOptCatalogFlagAltDispatchLatch) != 0) {
        playerState->altGunDispatchRequested = 1;
    }

    if (playerState->netInputBit17Latch == 0) {
        playerState->primaryGunDispatchRequested = 0;
        return;
    }

    PlayerGunFireController *const activePrimaryGunController =
        playerState->activePrimaryGunController;
    if (g_Player_TotalTimeSecScaled >= activePrimaryGunController->nextDispatchTime &&
        (playerState->altGunTransitionState & 0x180) == 0) {
        playerState->primaryGunDispatchRequested = 1;
        activePrimaryGunController->nextDispatchTime =
            activePrimaryGunController->dispatchRepeatDelay + g_Player_TotalTimeSecScaled;
    }
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: restore the respawned player model to a lit, visible, healthy
 * state and clear destroyed-state combat selection and damage state.
 * Source owner: Player damage-hit and destroyed-state callback subsystem, not
 * a C++ Player class owner.
 * Evidence: effect-animation callback ABI receives the save state, queues the
 * camera transition clear callback, resets the healthy node transform, runs
 * the destroyed-respawn async action callback when present, and restores
 * damage, selection, and health fields.
 */
void __fastcall DestroyedStateRespawnCallback(
    zEffectAnimEntry *,
    zUtil_SaveGameState *saveState,
    int
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zClass_Object3D::gwObject3DSetLitFlag(
        playerState->rootNode,
        1
    );
    zClass_Object3D::gwObject3DSetAlphaScale(
        playerState->rootNode,
        0.0f
    );
    zClass_Object3D_ModelRefLerpQueue::Add(
        playerState->rootNode,
        saveState,
        (void *)(&ClearRespawnTransitionFlagCallback),
        0.0f,
        1.0f,
        5.0f
    );

    zClass_NodePartial *const healthyNode =
        zClass_Class::FindNodeRecursiveByName(
            playerState->rootNode,
            g_Player_HealthySubNodeName
        );
    if (healthyNode != 0) {
        zClass_Object3D::gwObject3DSetPosition(
            healthyNode,
            0.0f,
            0.0f,
            0.0f
        );
        zClass_Object3D::gwObject3DSetRotation(
            healthyNode,
            0.0f,
            0.0f,
            0.0f
        );
    }

    if (playerState->destroyedRespawnAsyncHandle != 0) {
        zEffect_Anim::NodeActionCallback(
            playerState->destroyedRespawnAsyncHandle,
            playerState->rootNode
        );
    }

    ResetDamageStateAndTimedHitStatus(saveState);
    playerState->cachedAltSelectionCode = 0;
    playerState->statusMeterValue = playerState->masterCommonData->maxHealth;
    playerState->cachedPrimarySelectionCode = 0;
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: begin destroyed-state reset by restarting the player node action,
 * restoring damage/health visibility, queuing model fade-in, and refreshing
 * weapon, HUD, and alt-gun runtime state.
 * Source owner: Player damage-hit and destroyed-state callback subsystem, not
 * a C++ Player class owner.
 * Evidence: effect-animation callback ABI receives the save state and body
 * uses destroyed-respawn effect data, root-node visual flags, model-ref lerp
 * queue callback 0x41bca0, network respawn/drop handling, and weapon/HUD reset.
 */
void __fastcall DestroyedStateResetCallback(
    zEffectAnimEntry *,
    zUtil_SaveGameState *saveState,
    int
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zEffect_Anim::NodeActionCallback(
        playerState->destroyedRespawnFxEntry,
        playerState->rootNode
    );
    ResetDamageStateAndTimedHitStatus(saveState);

    playerState->statusMeterValue = playerState->masterCommonData->maxHealth;
    zClass_Object3D::gwObject3DSetLitFlag(
        playerState->rootNode,
        1
    );
    zClass_Object3D::gwObject3DSetAlphaScale(
        playerState->rootNode,
        0.0f
    );
    zClass_Object3D_ModelRefLerpQueue::Add(
        playerState->rootNode,
        saveState,
        (void *)(&DestroyedStateResetFinalizeCallback),
        0.0f,
        1.0f,
        1.0f
    );

    playerState->aiMode = 0;
    playerState->nextModeSwitchAllowedTime = 0.0f;
    playerState->autoTurnSign = 0;
    playerState->motionInput = 0;
    TransitionToMasterTypeTrack(
        saveState,
        0
    );
    GameNet::RespawnPlayerAndDropWeaponPickupIfAllowed(
        saveState,
        0
    );
    LoadWeaponBanksAndSelectDefaults(saveState);
    RefreshHudFromState(saveState);
    ResetAltGunDoorAnimationState(saveState);
    HudUiMgr::TriggerCurrentLayoutOnActivated();
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: finish the destroyed-state model fade-in and restore health,
 * lifecycle, and camera transition state after respawn reset.
 * Source owner: Player damage-hit and destroyed-state callback subsystem, not
 * a C++ Player class owner.
 * Evidence: model-ref lerp callback ABI receives the save state, compares the
 * nearest other player at the spawn point before optional network respawn/drop
 * handling, then calls the local finalize helper and restores player fields.
 */
void __fastcall DestroyedStateResetFinalizeCallback(
    zUtil_SaveGameState *saveState
) {
    zUtil_SaveGameState *nearestSaveState = saveState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterCommonData *const masterCommonData = playerState->masterCommonData;

    const float nearestDistanceSq = GameNet::GetNearestOtherPlayerDistanceToSpawnPoint(
        (GameNetSpawnPoint *)&playerState->worldPos,
        (GameNetPlayerSaveState **)&nearestSaveState
    );
    if (nearestDistanceSq < 20.0f && saveState->netPlayerRow->playerColorIndex <
                                         nearestSaveState->netPlayerRow->playerColorIndex) {
        GameNet::RespawnPlayerAndDropWeaponPickupIfAllowed(
            saveState,
            0
        );
    }

    DestroyedStateResetLocalFinalize();
    playerState->lifecycleState = kPlayerLifecycleLocal;
    playerState->statusMeterValue = masterCommonData->maxHealth;
    playerState->cameraTransitionTimer = 0;
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: clear the camera transition timer after the destroyed-state
 * respawn fade finishes.
 * Source owner: Player damage-hit and destroyed-state callback subsystem, not
 * a C++ Player class owner.
 * Evidence: model-ref lerp callback ABI receives the save state and the body
 * only clears the save-state player's camera transition timer.
 */
void __fastcall ClearRespawnTransitionFlagCallback(
    zUtil_SaveGameState *saveState
) {
    saveState->playerState->cameraTransitionTimer = 0;
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: finish the local destroyed-state reset by restoring active local
 * lifecycle, input/camera state, damage state, and pickup effect feedback.
 * Source owner: Player damage-hit and destroyed-state callback subsystem, not
 * a C++ Player class owner.
 * Evidence: helper is called by the destroyed reset finalize callback and
 * touches only the active global save-state player's destroyed lifecycle,
 * steering/camera restoration, damage reset, and pickup effect dispatch.
 */
void __cdecl DestroyedStateResetLocalFinalize() {
    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->lifecycleState == kPlayerLifecycleInactive) {
        playerState->lifecycleState = kPlayerLifecycleLocal;
        zOpt::SetSteeringMode(g_PlayerPrevSteeringMode);
        ApplyCameraState(g_PlayerPrevCameraState);
        ResetMouseControlStateAndRecenterCursor(saveState);
        ResetDamageStateAndTimedHitStatus(saveState);
    }

    Pickup::ApplyEffect(
        0x386,
        0,
        saveState
    );
}

} // namespace Player

#include "Battlesport/hud_ui_net_exit_panel.h"

#include "Battlesport/recoil_app.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zInput/zinput.h"

#include <new>

extern "C" {
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-huduinetexitpanel
 * @recoil-artifact defines .data recoil:data:0x4f32c0: g_HudUiNetExitPanel.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: hold the process-global network exit panel singleton.
 */
HudUiNetExitPanel *g_HudUiNetExitPanel = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-huduinetexitpanel-savedinputfocus
 * @recoil-artifact defines .data recoil:data:0x4f32bc: g_HudUiNetExitPanel_SavedInputFocus.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: preserve the prior HUD input focus while the network exit panel owns input capture.
 */
HudUiElement *g_HudUiNetExitPanel_SavedInputFocus = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-mission-huduinetexitpanel-constructor
 * @recoil-artifact defines .text recoil:function:0x41bd80: HudUiNetExitPanel constructor.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: initialize the network exit panel, bind its exit and resume widgets, and capture input focus state.
 */
HudUiNetExitPanel::HudUiNetExitPanel() {
    resumeWidget.previewInputCaptureActive = 0;

    exitWidget.previewInputCaptureActive = 0;

    zReader::Node *const loadedSection = LoadFromZrd(
        "dialog.zrd",
        "NETEXIT",
        1
    );
    if (loadedSection != 0) {
        BindWidgetByName(
            loadedSection,
            &exitWidget,
            "EXIT"
        );
        BindWidgetByName(
            loadedSection,
            &resumeWidget,
            "RESUME"
        );
        FreeLoadedTreeRoots((int)((unsigned int)(loadedSection)));
    }

    if (zInp::GetJoystickOption() == 0) {
        g_HudUiNetExitPanel_SavedInputFocus = GetInputFocus();
        SetInputFocus(0);
    }

    SetChildFlags(0);
    HudUiContainer *const panelContainer = this;
    panelContainer->SetEnabled(0);
}

/**
 * Original helper evidence: no standalone retail function; recovered from address-backed callers in this source file.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: forward panel enabled-state changes through the HudUiBackground base implementation.
 */
void HudUiNetExitPanel::SetEnabled(
    int enabled
) {
    HudUiBackground::SetEnabled(enabled);
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: queue the leave-network app state when the exit button is activated.
 */
void HudUiNetExitPanel_ExitButton::OnActivate() {
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_leaveNetworkState,
        0
    );
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: let VC5 emit the panel's virtual derived-to-base destruction path
 * after the two embedded network-exit widgets.
 */
HudUiNetExitPanel::~HudUiNetExitPanel() {
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: close the preview, hide the network exit panel, and dispatch normal ZRD activation.
 */
void HudUiNetExitPanel_ResumeWidget::OnActivate() {
    HidePreview();
    g_HudUiNetExitPanel->SetEnabled(0);
    HudUiMgr::TriggerCurrentLayoutOnActivated();
    HudUiZrdWidget::OnActivate();
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: push preview input capture, restore saved focus for mouse mode, and show the resume preview.
 */
void HudUiNetExitPanel_ResumeWidget::OnShowPreview() {
    if (previewInputCaptureActive == 0) {
        zInput::BindMapContext_Push(0);
        zInput::BindMapCurrent_SetMouseBinding(
            1,
            0
        );

        if (zInp::GetJoystickOption() == 0) {
            HudUiMgr::UpdateTargetReticleFromCursor(
                0,
                0,
                0.0f,
                0.0f
            );

            HudUiElement *const focus = g_HudUiNetExitPanel_SavedInputFocus;
            if (focus != 0) {
                ((HudUiBackgroundContainer *)(owner))->SetInputFocus(focus);
            }
        }

        previewInputCaptureActive = 1;
    }

    HudUiZrdWidget::ShowPreview();
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: pop preview input capture, save current focus for mouse mode, and hide the resume preview.
 */
void HudUiNetExitPanel_ResumeWidget::OnHidePreview() {
    if (previewInputCaptureActive != 0) {
        zInput::BindMapContext_Pop();

        if (zInp::GetJoystickOption() == 0) {
            HudUiMgr::UpdateTargetReticleFromCursor(
                1,
                0,
                0.0f,
                0.0f
            );
            HudUiBackgroundContainer *const backgroundOwner = (HudUiBackgroundContainer *)(owner);
            g_HudUiNetExitPanel_SavedInputFocus = backgroundOwner->GetInputFocus();
            backgroundOwner->SetInputFocus(0);
        }

        previewInputCaptureActive = 0;
    }

    HudUiZrdWidget::HidePreview();
}

/**
 * @recoil-anchor recoil:anchor:battlesport-mission-huduinetexitpanel-createglobal
 * @recoil-artifact defines .text recoil:function:0x41c000: HudUiNetExitPanel::CreateGlobal.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: allocate and construct the process-global network exit panel singleton.
 */
HudUiNetExitPanel *__cdecl HudUiNetExitPanel::CreateGlobal() {
    g_HudUiNetExitPanel = new HudUiNetExitPanel;
    return g_HudUiNetExitPanel;
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: enable the process-global network exit panel.
 */
void __cdecl HudUiNetExitPanel::Show() {
    g_HudUiNetExitPanel->SetEnabled(1);
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: tick the process-global network exit panel with the frame delta.
 */
int __cdecl HudUiNetExitPanel::Tick() {
    g_HudUiNetExitPanel->UpdateAll(g_FrameDeltaTimeSec);
    return 0;
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: destroy and release the process-global network exit panel singleton.
 */
void __cdecl HudUiNetExitPanel::DestroyGlobal() {
    HudUiNetExitPanel *const panel = g_HudUiNetExitPanel;
    if (panel != 0) {
        delete panel;
        g_HudUiNetExitPanel = 0;
    }
}
#include "Battlesport/ai_property_dlg.h"

namespace {
const int kAiPropertyDlgBehaviorComboId = 1089;
const int kAiPropertyDlgFirstPropertyLabelId = 1107;
const int kAiPropertyDlgSecondPropertyLabelId = 1108;
} // namespace

/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-aipropertydlg-labelunused
 * @recoil-artifact defines .data recoil:data:0x4db604: g_AiPropertyDlg_LabelUnused.
 * Purpose: Supplies the AI property dialog label used when a behavior mode has no property.
 */
char g_AiPropertyDlg_LabelUnused[] = "Unused";

/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-aipropertydlg-labelmovement
 * @recoil-artifact defines .data recoil:data:0x4db60c: g_AiPropertyDlg_LabelMovement.
 * Purpose: Supplies the AI property dialog label for movement behavior properties.
 */
char g_AiPropertyDlg_LabelMovement[] = "Movement";

/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-aipropertydlg-labelattackrange
 * @recoil-artifact defines .data recoil:data:0x4db618: g_AiPropertyDlg_LabelAttackRange.
 * Purpose: Supplies the AI property dialog label for attack range properties.
 */
char g_AiPropertyDlg_LabelAttackRange[] = "Attack Range";

/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-aipropertydlg-labelmaxpursuitrange
 * @recoil-artifact defines .data recoil:data:0x4db628: g_AiPropertyDlg_LabelMaxPursuitRange.
 * Purpose: Supplies the AI property dialog label for maximum pursuit range properties.
 */
char g_AiPropertyDlg_LabelMaxPursuitRange[] = "Max Pursuit Rng";

/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-aipropertydlg-labelminpursuitrange
 * @recoil-artifact defines .data recoil:data:0x4db638: g_AiPropertyDlg_LabelMinPursuitRange.
 * Purpose: Supplies the AI property dialog label for minimum pursuit range properties.
 */
char g_AiPropertyDlg_LabelMinPursuitRange[] = "Min Pursuit Rng";

/**
 * Provider-boundary accessor for imported MFC42 CDialog metadata; this does not reimplement
 * CDialog behavior.
 */
class AiPropertyDlgCDialogMessageMapAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *__stdcall GetMessageMap();
};

/**
 * MFC provider-boundary accessor for imported CDialog message-map metadata.
 * Purpose: Exposes CDialog::messageMap through the callback shape expected by the derived map.
 */
const AFX_MSGMAP *__stdcall AiPropertyDlgCDialogMessageMapAccessor::GetMessageMap() {
    return &CDialog::messageMap;
}

/**
 * MFC provider-boundary accessor for AiPropertyDlg's base message-map callback.
 * Purpose: Returns the provider-owned CDialog base message map for MFC dispatch chaining.
 */
const AFX_MSGMAP *__stdcall AiPropertyDlg::GetBaseMessageMapForMfc() {
    return AiPropertyDlgCDialogMessageMapAccessor::GetMessageMap();
}

AFX_MSGMAP_ENTRY const AiPropertyDlg::messageEntries[] = {
    {WM_DESTROY, 0, 0, 0, 12, (AFX_PMSG)&AiPropertyDlg::OnDestroy},
    {WM_COMMAND,
        CBN_SELCHANGE,
        kAiPropertyDlgBehaviorComboId,
        kAiPropertyDlgBehaviorComboId,
        12,
        (AFX_PMSG)&AiPropertyDlg::OnSelChange},
    {0, 0, 0, 0, 0, 0},
};

const AFX_MSGMAP AiPropertyDlg::messageMap = {
    &AiPropertyDlg::GetBaseMessageMapForMfc,
    &AiPropertyDlg::messageEntries[0],
};

/**
 * MFC provider-boundary message-map accessor for AiPropertyDlg.
 * Purpose: Returns the authored dialog message-map table used by MFC command routing.
 */
const AFX_MSGMAP * AiPropertyDlg::GetMessageMap() const {
    return &AiPropertyDlg::messageMap;
}

/**
 * Purpose: Saves combo-box selections when the AI property dialog closes and hides the cursor.
 */
void AiPropertyDlg::OnDestroy() {
    CWnd::OnDestroy();

    const LRESULT selectedPropertyComboIndex =
        ::SendMessageA(
            m_propertyCombo.m_hWnd,
            CB_GETCURSEL,
            0,
            0
        );
    m_selectedPropertyIndex =
        ::SendMessageA(
            m_propertyCombo.m_hWnd,
            CB_GETITEMDATA,
            selectedPropertyComboIndex,
            0
        );

    const LRESULT selectedBehaviorComboIndex =
        ::SendMessageA(
            m_behaviorCombo.m_hWnd,
            CB_GETCURSEL,
            0,
            0
        );
    m_selectedBehaviorIndex =
        ::SendMessageA(
            m_behaviorCombo.m_hWnd,
            CB_GETITEMDATA,
            selectedBehaviorComboIndex,
            0
        );

    ::ShowCursor(FALSE);
}

/**
 * Purpose: Updates the selected AI behavior and refreshes the property labels.
 */
void AiPropertyDlg::OnSelChange() {
    const LRESULT selectedBehaviorComboIndex =
        ::SendMessageA(
            m_behaviorCombo.m_hWnd,
            CB_GETCURSEL,
            0,
            0
        );
    m_selectedBehaviorIndex =
        ::SendMessageA(
            m_behaviorCombo.m_hWnd,
            CB_GETITEMDATA,
            selectedBehaviorComboIndex,
            0
        );
    UpdatePropertyLabels();
}

/**
 * Purpose: Chooses the two property label strings for the currently selected AI behavior.
 */
void AiPropertyDlg::UpdatePropertyLabels() {
    CString firstLabel;
    CString secondLabel;

    switch ((unsigned int)m_selectedBehaviorIndex) {
    case 0:
        firstLabel = g_AiPropertyDlg_LabelMinPursuitRange;
        secondLabel = g_AiPropertyDlg_LabelMaxPursuitRange;
        break;

    case 1:
        firstLabel = g_AiPropertyDlg_LabelAttackRange;
        secondLabel = g_AiPropertyDlg_LabelUnused;
        break;

    case 2:
        firstLabel = g_AiPropertyDlg_LabelAttackRange;
        secondLabel = g_AiPropertyDlg_LabelMovement;
        break;

    case 3:
    case 4:
    case 5:
        firstLabel = g_AiPropertyDlg_LabelUnused;
        secondLabel = g_AiPropertyDlg_LabelUnused;
        break;

    default:
        break;
    }

    ((CWnd *)this)->SetDlgItemTextA(
        kAiPropertyDlgFirstPropertyLabelId,
        firstLabel
    );
    ((CWnd *)this)->SetDlgItemTextA(
        kAiPropertyDlgSecondPropertyLabelId,
        secondLabel
    );
}
#include "Battlesport/hud.h"
#include "GameZRecoil/include/opt_catalog.h"

#include <new>
#include <stdlib.h>

/* Complete HudUiNewGamePanel family body for the later mission.cpp host. */
/* Include exactly once after AiPropertyDlg and before NetSessionConfigDialog. */

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Start the new game through the owning panel before normal widget activation.
 */
void HudUiNewGamePanel_StartButton::OnActivate() {
    HudUiNewGamePanel *const panel = (HudUiNewGamePanel *)(owner);
    if (panel != 0) {
        panel->StartGameFromFields();
    }

    HudUiZrdWidget::OnActivate();
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Construct the panel, bind its ZRD widgets, and load the player name.
 */
HudUiNewGamePanel::HudUiNewGamePanel()
    : HudUiBackground() {
    zReader::Node *const loadedSection =
        HudUiBackground::LoadFromZrd(
            "dialog.zrd",
            "NEWGAMEPANEL",
            0
        );
    if (loadedSection != 0) {
        HudUiBackground::BindWidgetByName(loadedSection, &backWidget, "BACK");
        HudUiBackground::BindWidgetByName(loadedSection, &startWidget, "START");
        HudUiBackground::BindWidgetByName(loadedSection, &nameInput, "NAME");
        HudUiBackground::BindWidgetByName(loadedSection, &intensity, "INTENSITY");
        HudUiBackground::FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
    }

    nameInput.Update(zOpt_GetPlayerName());
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Refresh and activate the player-name input with raw keyboard capture.
 */
void HudUiNewGamePanel_NameInput::OnActivate() {
    textInput.AllocTextBuffer(21);
    HudUiNumericTextInput::Update(zOpt_GetPlayerName());
    HudUiNumericTextInput::OnActivate();
    HudUiNumericTextInput::SetRawKeyboardCapture(1);
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Tear down the panel through ordinary reverse C++ member/base cleanup.
 */
HudUiNewGamePanel::~HudUiNewGamePanel() {
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Reflect the saved game difficulty in the panel selector.
 */
void HudUiNewGamePanel::SyncIntensityFromDifficulty() {
    intensity.SetSelectedIndex(zOpt::GetGameDifficultyMode());
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Commit new-game options and queue mission FMV startup.
 */
void HudUiNewGamePanel::StartGameFromFields() {
    HudCheat::ClearNanitePanelCheatSentinel();
    zOpt::SetPlayerName(nameInput.GetBuffer());
    zOpt::SetGameDifficultyMode(intensity.selectedIndex);
    g_RecoilApp.m_missionFmvState.SetMissionId(1);
    g_RecoilApp.QueueExitCurrentState(1);
    g_RecoilApp.QueueExitCurrentState(1);
    g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_missionFmvState, 0);
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Create, enable, and retain the new-game panel for the overlay state.
 */
int HudUiNewGamePanelOverlayOwner::OnTryBecomeCurrent() {
    HudUiNewGamePanel *const panel = new HudUiNewGamePanel;
    m_dialog = panel;
    panel->SyncIntensityFromDifficulty();
    ((HudUiNewGamePanel *)m_dialog)->SetEnabled(1);
    return 1;
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: construct the global new-game overlay owner and register its exit cleanup.
 */
void __cdecl HudUiNewGamePanelOverlayOwner::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: return the ordinary static-storage new-game overlay owner.
 */
HudUiNewGamePanelOverlayOwner *__cdecl HudUiNewGamePanelOverlayOwner::StaticInit() {
    return &g_HudUiNewGamePanelOverlayOwner;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-mission-g-huduinewgamepaneloverlayowner
 * @recoil-artifact defines .data recoil:data:0x4f32c8: g_HudUiNewGamePanelOverlayOwner.
 * Purpose: Own the ordinary static-storage new-game overlay state object.
 */
HudUiNewGamePanelOverlayOwner g_HudUiNewGamePanelOverlayOwner;

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Disable and destroy the active panel owned by this app state.
 */
HudUiNewGamePanelOverlayOwner::~HudUiNewGamePanelOverlayOwner() {
    HudUiNewGamePanel *panel = (HudUiNewGamePanel *)m_dialog;
    if (panel != 0) {
        panel->SetEnabled(0);

        panel = (HudUiNewGamePanel *)m_dialog;
        if (panel != 0) {
            delete panel;
        }

        m_dialog = 0;
    }
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: register the global new-game overlay owner destructor for process exit.
 */
void __cdecl HudUiNewGamePanelOverlayOwner::RegisterAtExit() {
    atexit(AtExitDestructor);
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: run process-exit cleanup for the global new-game overlay owner.
 */
void __cdecl HudUiNewGamePanelOverlayOwner::AtExitDestructor() {
    g_HudUiNewGamePanelOverlayOwner.~HudUiNewGamePanelOverlayOwner();
}

/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Queue the global overlay owner as the next app state.
 */
void __cdecl HudUiNewGamePanelOverlayOwner::QueueEnter() {
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_HudUiNewGamePanelOverlayOwner,
        0
    );
}

/**
 * Purpose: Construct the multiplayer session configuration dialog controls.
 */
NetSessionConfigDialog::NetSessionConfigDialog(
    CWnd *parentWnd
) :
    CDialog(
        kNetSessionConfigDialogResourceId,
        parentWnd
    ),
    m_maxPlayersSpin(),
    m_valueLimitSpin(),
    m_timeLimitSpin(),
    m_mapCombo(),
    m_sessionName()
{
    m_sessionName = "";
    m_valueLimit = 0;
    m_timeLimitMinutes = 0;
    m_maxPlayers = 0;
    m_unusedCheckboxEnabled = 0;
}

/**
 * Purpose: Bind config dialog controls and validate session numeric fields.
 */
void NetSessionConfigDialog::DoDataExchange(
    CDataExchange *dataExchange
) {
    DDX_Control(
        dataExchange,
        kNetSessionConfigMaxPlayersSpinId,
        (CWnd &)m_maxPlayersSpin
    );
    DDX_Control(
        dataExchange,
        kNetSessionConfigValueLimitSpinId,
        (CWnd &)m_valueLimitSpin
    );
    DDX_Control(
        dataExchange,
        kNetSessionConfigTimeLimitSpinId,
        (CWnd &)m_timeLimitSpin
    );
    DDX_Control(
        dataExchange,
        kNetSessionConfigMapComboId,
        (CWnd &)m_mapCombo
    );
    DDX_Text(
        dataExchange,
        kNetSessionConfigSessionNameEditId,
        m_sessionName
    );
    DDV_MaxChars(
        dataExchange,
        m_sessionName,
        kNetSessionConfigSessionNameMaxChars
    );
    DDX_Text(
        dataExchange,
        kNetSessionConfigValueLimitEditId,
        m_valueLimit
    );
    DDV_MinMaxUInt(
        dataExchange,
        m_valueLimit,
        0,
        kNetSessionConfigLimitMax
    );
    DDX_Text(
        dataExchange,
        kNetSessionConfigTimeLimitEditId,
        m_timeLimitMinutes
    );
    DDV_MinMaxUInt(
        dataExchange,
        m_timeLimitMinutes,
        0,
        kNetSessionConfigLimitMax
    );
    DDX_Text(
        dataExchange,
        kNetSessionConfigMaxPlayersEditId,
        m_maxPlayers
    );
    DDV_MinMaxUInt(
        dataExchange,
        m_maxPlayers,
        kNetSessionConfigMaxPlayersMin,
        kNetSessionConfigMaxPlayersMax
    );
    DDX_Check(
        dataExchange,
        kNetSessionConfigUnusedCheckboxId,
        m_unusedCheckboxEnabled
    );
}

/**
 * Purpose: Return the config dialog MFC message map.
 */
const AFX_MSGMAP * NetSessionConfigDialog::GetMessageMap() const {
    return &NetSessionConfigDialog::messageMap;
}

namespace Mission {
/**
 * Purpose: Construct and register cleanup for multiplayer map name strings.
 */
void __cdecl RegisterMultiplayerMaps() {
    NetSessionConfigDialog::InitMapNameStrings();
    NetSessionConfigDialog::RegisterMapNameCleanup();
}
} // namespace Mission

/**
 * Purpose: Construct the seven multiplayer map-name CString entries.
 */
void NetSessionConfigDialog::InitMapNameStrings() {
    new (&g_NetSessionConfigDialog_MapNameStrings[0]) CString("RiverWorks");
    new (&g_NetSessionConfigDialog_MapNameStrings[1]) CString("Crater Chaos");
    new (&g_NetSessionConfigDialog_MapNameStrings[2]) CString("Beach Rally");
    new (&g_NetSessionConfigDialog_MapNameStrings[3]) CString("Clone City");
    new (&g_NetSessionConfigDialog_MapNameStrings[4]) CString("Frozen Tundra");
    new (&g_NetSessionConfigDialog_MapNameStrings[5]) CString("Poison Valley");
    new (&g_NetSessionConfigDialog_MapNameStrings[6]) CString("New Clone City");
}

/**
 * Purpose: Register process-exit cleanup for multiplayer map-name strings.
 */
void NetSessionConfigDialog::RegisterMapNameCleanup() {
    atexit(&NetSessionConfigDialog::CleanupMapNameStringsOnExit);
}

/**
 * Purpose: Destroy the static multiplayer map-name CString entries.
 */
void __cdecl NetSessionConfigDialog::CleanupMapNameStringsOnExit() {
    for (int index = kNetSessionConfigMapNameCount - 1; index >= 0; --index) {
        g_NetSessionConfigDialog_MapNameStrings[index].~CString();
    }
}

/**
 * Purpose: Initialize multiplayer session config fields, maps, and spin ranges.
 */
BOOL NetSessionConfigDialog::OnInitDialog() {
    ((CDialog *)this)->CDialog::OnInitDialog();

    m_sessionName.Format(
        kNetSessionConfigSessionNameFormat,
        m_defaultExerciseOrdinal
    );

    for (int mapIndex = 0; mapIndex < kNetSessionConfigMapNameCount; ++mapIndex) {
        const LRESULT comboItemIndex = ::SendMessageA(
            m_mapCombo.m_hWnd,
            CB_ADDSTRING,
            0,
            (LPARAM)((const char *)g_NetSessionConfigDialog_MapNameStrings[mapIndex])
        );
        ::SendMessageA(
            m_mapCombo.m_hWnd,
            CB_SETITEMDATA,
            comboItemIndex,
            mapIndex
        );
    }

    ::SendMessageA(
        m_mapCombo.m_hWnd,
        CB_SETCURSEL,
        0,
        0
    );
    ::SendMessageA(
        m_timeLimitSpin.m_hWnd,
        kNetSessionConfigSpinSetRangeMessage,
        0,
        MAKELPARAM(360, 0)
    );
    ::SendMessageA(
        m_valueLimitSpin.m_hWnd,
        kNetSessionConfigSpinSetRangeMessage,
        0,
        MAKELPARAM(100, 0)
    );

    LPARAM maxPlayersRange =
        MAKELPARAM(
            kNetSessionConfigMaxPlayersMax,
            kNetSessionConfigMaxPlayersMin
        );
    if (zOpt::GetNetworkModemEnabled() != 0) {
        maxPlayersRange =
            MAKELPARAM(
                kNetSessionConfigMaxPlayersMin,
                kNetSessionConfigMaxPlayersMin
            );
    }
    ::SendMessageA(
        m_maxPlayersSpin.m_hWnd,
        kNetSessionConfigSpinSetRangeMessage,
        0,
        maxPlayersRange
    );

    m_valueLimit = kNetSessionConfigDefaultValueLimit;
    m_timeLimitMinutes = kNetSessionConfigDefaultTimeLimitMinutes;
    m_maxPlayers = kNetSessionConfigDefaultMaxPlayers;
    m_unusedCheckboxEnabled = 1;
    ((CWnd *)this)->UpdateData(FALSE);
    ((CWnd *)this)
        ->SetDlgItemTextA(
            kNetSessionConfigMaxPlayersLabelId,
            zLoc::GetMessageString(kNetSessionConfigMaxPlayersDefaultMessageId)
        );
    return TRUE;
}

/**
 * Purpose: Persist the selected map index as the config dialog closes.
 */
void NetSessionConfigDialog::OnDestroy() {
    ((GameNetMfcWndAccess *)this)->CallOnDestroy();
    const LRESULT selectedMapComboIndex = ::SendMessageA(
        m_mapCombo.m_hWnd,
        CB_GETCURSEL,
        0,
        0
    );
    m_selectedMapIndex =
        (int) ::SendMessageA(
            m_mapCombo.m_hWnd,
            CB_GETITEMDATA,
            selectedMapComboIndex,
            0
        );
}

/**
 * Purpose: Track map selection and refresh max-player label text.
 */
void NetSessionConfigDialog::OnMapChanged() {
    const LRESULT selectedMapComboIndex = ::SendMessageA(
        m_mapCombo.m_hWnd,
        CB_GETCURSEL,
        0,
        0
    );
    const LRESULT selectedMapIndex =
        ::SendMessageA(
            m_mapCombo.m_hWnd,
            CB_GETITEMDATA,
            selectedMapComboIndex,
            0
        );
    m_selectedMapIndex = (int)selectedMapIndex;

    unsigned int messageId = kNetSessionConfigMaxPlayersSpecialMapMessageId;
    if (selectedMapIndex != kNetSessionConfigSpecialMapIndex) {
        messageId = kNetSessionConfigMaxPlayersDefaultMessageId;
    }
    ((CWnd *)this)
        ->SetDlgItemTextA(
            kNetSessionConfigMaxPlayersLabelId,
            zLoc::GetMessageString(messageId)
        );
}
