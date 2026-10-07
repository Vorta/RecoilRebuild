#include "recoil/Mfc42Abi.h"
#include "player.h"

#include "Battlesport/ai_net.h"
#include "Battlesport/game_net.h"
#include "Battlesport/pickup.h"
#include "Battlesport/turret.h"
#include "Battlesport/wol_api.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "hud.h"
#include "hud_sensor_tracker.h"
#include "opt_catalog.h"

#include <algorithm>
#include <atlbase.h>
#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern char g_HudUiCounterText_PlayerLabel[];

/**
 * Purpose: remember the most recent mover node accepted from movers.zrd.
 */
extern "C" CZNodePartial* g_Mover_LastLoadedNode = 0;

extern "C" {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-hudcountervalue
 * @recoil-artifact defines .data recoil:data:0x4f3764: g_Player_HudCounterValue.
 * BN types this as a zero-filled .data int restored by
 * Player::ApplyMissionSaveData, accumulated by AddScaledHudCounterValue, and
 * mirrored into mission-save/HUD objective counter paths.
 * Purpose: Stores the local mission objective HUD counter value.
 */
int g_Player_HudCounterValue = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-localcontrolenabled
 * @recoil-artifact defines .data recoil:data:0x4f36b0: g_Player_LocalControlEnabled.
 * BN types this as a zero-filled .data int seeded from the network option by
 * Player::InitMissionRuntimeFromWorldAndCamera and toggled by local-control
 * input paths.
 * Purpose: Gates local player command handling during mission runtime.
 */
int g_Player_LocalControlEnabled = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-runtimeinputflags
 * @recoil-artifact defines .data recoil:data:0x4f36a0: g_Player_RuntimeInputFlags.
 * BN types this as a zero-filled .data int reset by zZarRegisterSections and
 * read by local gameplay input/runtime-control paths.
 * Purpose: Stores player runtime input mode flags for the current mission.
 */
int g_Player_RuntimeInputFlags = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-camerazone
 * @recoil-artifact defines .data recoil:data:0x4f36f0: g_Player_CameraZone.
 * BN types this as a zero-filled .data float overwritten from player.zrd
 * camera_zone tuning, or by the mission-runtime default, before camera input
 * reads it.
 * Purpose: Stores the camera dead-zone threshold for player aim/camera input.
 */
float g_Player_CameraZone = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-camerazoneinvrange
 * @recoil-artifact defines .data recoil:data:0x4f36f4: g_Player_CameraZoneInvRange.
 * BN types this as a zero-filled .data float paired with g_Player_CameraZone
 * and seeded during mission-runtime player.zrd tuning load.
 * Purpose: Stores the reciprocal scale for input outside the camera dead zone.
 */
float g_Player_CameraZoneInvRange = 0.0f;
/**
 * Storage group: Player ZRD runtime tuning globals.
 * BN types these as independent zero-filled .data globals written by
 * Player::InitMissionRuntimeFromWorldAndCamera from player.zrd nodes:
 * camera, underwater-camera, gravity/sink, slope, and heat/cold option tuning.
 * Purpose: Stores mission runtime tuning loaded from player.zrd.
 */
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-maxcamyawrate
 * @recoil-artifact defines .data recoil:data:0x4f36f8: g_Player_MaxCamYawRate.
 * Purpose: stores the plan-tracked g_Player_MaxCamYawRate gameplay data symbol.
 */
float g_Player_MaxCamYawRate = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mousepushx
 * @recoil-artifact defines .data recoil:data:0x4f36fc: g_Player_MousePushX.
 * Purpose: stores the plan-tracked g_Player_MousePushX gameplay data symbol.
 */
float g_Player_MousePushX = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mousepushy
 * @recoil-artifact defines .data recoil:data:0x4f3700: g_Player_MousePushY.
 * Purpose: stores the plan-tracked g_Player_MousePushY gameplay data symbol.
 */
float g_Player_MousePushY = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-cameraelastic
 * @recoil-artifact defines .data recoil:data:0x4f3704: g_Player_CameraElastic.
 * Purpose: stores the plan-tracked g_Player_CameraElastic gameplay data symbol.
 */
float g_Player_CameraElastic = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-maxcamtetheranglerad
 * @recoil-artifact defines .data recoil:data:0x4f3708: g_Player_MaxCamTetherAngleRad.
 * Purpose: stores the plan-tracked g_Player_MaxCamTetherAngleRad gameplay data symbol.
 */
float g_Player_MaxCamTetherAngleRad = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-fpcamelevationrate
 * @recoil-artifact defines .data recoil:data:0x4f370c: g_Player_FpCamElevationRate.
 * Purpose: stores the plan-tracked g_Player_FpCamElevationRate gameplay data symbol.
 */
float g_Player_FpCamElevationRate = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-fpcamelevationmax
 * @recoil-artifact defines .data recoil:data:0x4f3710: g_Player_FpCamElevationMax.
 * Purpose: stores the plan-tracked g_Player_FpCamElevationMax gameplay data symbol.
 */
float g_Player_FpCamElevationMax = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-fpcamelevationmin
 * @recoil-artifact defines .data recoil:data:0x4f3714: g_Player_FpCamElevationMin.
 * Purpose: stores the plan-tracked g_Player_FpCamElevationMin gameplay data symbol.
 */
float g_Player_FpCamElevationMin = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-underwatercamdistance
 * @recoil-artifact defines .data recoil:data:0x4f371c: g_Player_UnderwaterCamDistance.
 * Purpose: stores the plan-tracked g_Player_UnderwaterCamDistance gameplay data symbol.
 */
float g_Player_UnderwaterCamDistance = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-underwatercamheight
 * @recoil-artifact defines .data recoil:data:0x4f3720: g_Player_UnderwaterCamHeight.
 * Purpose: stores the plan-tracked g_Player_UnderwaterCamHeight gameplay data symbol.
 */
float g_Player_UnderwaterCamHeight = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-underwatercamstepcount
 * @recoil-artifact defines .data recoil:data:0x4f3724: g_Player_UnderwaterCamStepCount.
 * Purpose: stores the plan-tracked g_Player_UnderwaterCamStepCount gameplay data symbol.
 */
int g_Player_UnderwaterCamStepCount = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-underwatercamfar
 * @recoil-artifact defines .data recoil:data:0x4f3728: g_Player_UnderwaterCamFar.
 * Purpose: stores the plan-tracked g_Player_UnderwaterCamFar gameplay data symbol.
 */
float g_Player_UnderwaterCamFar = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-underwatercampackedcolor
 * @recoil-artifact defines .data recoil:data:0x4f372c: g_Player_UnderwaterCamPackedColor.
 * Purpose: stores the plan-tracked g_Player_UnderwaterCamPackedColor gameplay data symbol.
 */
unsigned int g_Player_UnderwaterCamPackedColor = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-underwatercamalpha
 * @recoil-artifact defines .data recoil:data:0x4f3730: g_Player_UnderwaterCamAlpha.
 * Purpose: stores the plan-tracked g_Player_UnderwaterCamAlpha gameplay data symbol.
 */
float g_Player_UnderwaterCamAlpha = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-cameraheadingdotabs
 * @recoil-artifact defines .data recoil:data:0x4da398: g_Player_CameraHeadingDotAbs.
 * Purpose: stores the plan-tracked g_Player_CameraHeadingDotAbs gameplay data symbol.
 */
float g_Player_CameraHeadingDotAbs = 1.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-cameraheadinglerpbasewhenflagclear
 * @recoil-artifact defines .data recoil:data:0x4da39c: g_Player_CameraHeadingLerpBaseWhenFlagClear.
 * Purpose: stores the plan-tracked g_Player_CameraHeadingLerpBaseWhenFlagClear gameplay data symbol.
 */
float g_Player_CameraHeadingLerpBaseWhenFlagClear = 3.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-cameraheadinglerpbasewhenflagset
 * @recoil-artifact defines .data recoil:data:0x4da3a0: g_Player_CameraHeadingLerpBaseWhenFlagSet.
 * Purpose: stores the plan-tracked g_Player_CameraHeadingLerpBaseWhenFlagSet gameplay data symbol.
 */
float g_Player_CameraHeadingLerpBaseWhenFlagSet = 2.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-localplayersavestate
 * @recoil-artifact defines .data recoil:data:0x4f36a4: g_LocalPlayerSaveState.
 * BN types this as a zero-filled .data zUtil_SaveGameState pointer written
 * during mission-runtime initialization and read by the mission save/load
 * payload, local-control, camera-anchor, and HUD/gameplay paths.
 * Purpose: Points at the active local player's save-state record.
 */
zUtil_SaveGameState* g_LocalPlayerSaveState = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player2savestate
 * @recoil-artifact defines .data recoil:data:0x4f3770: g_Player2SaveState.
 * BN types this as a zero-filled .data zUtil_SaveGameState pointer assigned to
 * the stealth save-state created during mission-runtime bootstrap.
 * Purpose: Holds the hidden second-player/stealth save-state record.
 */
zUtil_SaveGameState* g_Player2SaveState = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-currentplayersavestate
 * @recoil-artifact defines .data recoil:data:0x4f36a8: g_CurrentPlayerSaveState.
 * Purpose: stores the plan-tracked g_CurrentPlayerSaveState gameplay data symbol.
 */
zUtil_SaveGameState* g_CurrentPlayerSaveState = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-lastvalidcameravarianttag
 * @recoil-artifact defines .data recoil:data:0x4f3718: g_Player_LastValidCameraVariantTag.
 * BN types this as a zero-filled .data zTag4 copied into and out of the
 * Player ZAR mission-save section as one packed 32-bit value.
 * Purpose: Remembers the last camera variant tag valid for mission save/load.
 */
zTag4Partial g_Player_LastValidCameraVariantTag = { 0 };
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-thirdpersoncamerasideprobeoffsetscale
 * @recoil-artifact defines .data recoil:data:0x4da3a4: g_Player_ThirdPersonCameraSideProbeOffsetScale.
 * Purpose: stores the plan-tracked g_Player_ThirdPersonCameraSideProbeOffsetScale gameplay data symbol.
 */
float g_Player_ThirdPersonCameraSideProbeOffsetScale = 1.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-cameravariantupdatedthistick
 * @recoil-artifact defines .data recoil:data:0x4e5cc0: g_Player_CameraVariantUpdatedThisTick.
 * Purpose: stores the plan-tracked g_Player_CameraVariantUpdatedThisTick gameplay data symbol.
 */
int g_Player_CameraVariantUpdatedThisTick = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-rebuildcameradirflatfromcurrenttarget
 * @recoil-artifact defines .data recoil:data:0x4e5cd8: g_Player_RebuildCameraDirFlatFromCurrentTarget.
 * Purpose: stores the plan-tracked g_Player_RebuildCameraDirFlatFromCurrentTarget gameplay data symbol.
 */
int g_Player_RebuildCameraDirFlatFromCurrentTarget = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nextordinal
 * @recoil-artifact defines .data recoil:data:0x4f3a94: g_Player_NextOrdinal.
 * Purpose: stores the plan-tracked g_Player_NextOrdinal gameplay data symbol.
 */
int g_Player_NextOrdinal = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-aimode2state1finalized
 * @recoil-artifact defines .data recoil:data:0x4f36ac: g_Player_AiMode2State1Finalized.
 * BN types this as a zero-filled .data int written by
 * AINet::AiFinalizeMode2State1ForAllPlayers and read by the Mode2 State1 AI
 * steering/latch helpers.
 * Purpose: Latches completion of the Mode2 State1 saved-state finalization pass.
 */
int g_Player_AiMode2State1Finalized = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-healthysubnodename
 * @recoil-artifact defines .data recoil:data:0x4db5ec: g_Player_HealthySubNodeName.
 * Purpose: Names the shared healthy child node used by player, pickup, and
 * turret paths.
 */
char g_Player_HealthySubNodeName[8] = "healthy";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-totaltimesecscaled
 * @recoil-artifact defines .data recoil:data:0x4f3760: g_Player_TotalTimeSecScaled.
 * Purpose: Stores the accumulated player-frame time used by gameplay timers.
 */
float g_Player_TotalTimeSecScaled = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-deltatime
 * @recoil-artifact defines .data recoil:data:0x4f3ac4: g_Player_DeltaTime.
 * BN types this as a zero-filled player timing float consumed by force-feedback
 * pitch filtering.
 * Purpose: Stores the current player-frame delta time used by input effects.
 */
float g_Player_DeltaTime = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-invdeltatime
 * @recoil-artifact defines .data recoil:data:0x4f3aac: g_Player_InvDeltaTime.
 * BN types this as a zero-filled player timing reciprocal float.
 * Purpose: Stores the inverse player-frame delta time shared with zInput code.
 */
float g_Player_InvDeltaTime = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-deltatimescaled001
 * @recoil-artifact defines .data recoil:data:0x4f3abc: g_Player_DeltaTimeScaled001.
 * BN types this as a zero-filled player timing float adjacent to the delta-time
 * globals.
 * Purpose: Stores the 0.01-scaled player-frame delta time shared with zInput.
 */
float g_Player_DeltaTimeScaled001 = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playerpendingcheckpointnumber
 * @recoil-artifact defines .data recoil:data:0x4f3a98: g_PlayerPendingCheckpointNumber.
 * Purpose: stores the plan-tracked g_PlayerPendingCheckpointNumber gameplay data symbol.
 */
int g_PlayerPendingCheckpointNumber = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-checkpoint-nodenamefmt
 * @recoil-artifact defines .data recoil:data:0x4dc4e0: g_Checkpoint_NodeNameFmt.
 * BN types this as a writable 13-byte .data string referenced only by
 * Checkpoint::InstantiateNamedObjects.
 * Purpose: Formats checkpoint node names as checkpoint1..checkpointN.
 */
char g_Checkpoint_NodeNameFmt[13] = "checkpoint%d";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playerstatusmeterratio
 * @recoil-artifact defines .data recoil:data:0x4f3754: g_PlayerStatusMeterRatio.
 * Purpose: Stores g PlayerStatusMeterRatio data used by battlesport_gameplay.player_damage_runtime_globals.
 */
float g_PlayerStatusMeterRatio = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nominalgravity
 * @recoil-artifact defines .data recoil:data:0x4f3ac8: g_Player_NominalGravity.
 * Purpose: Stores g Player NominalGravity data used by battlesport_gameplay.player_nominal_gravity_global.
 */
float g_Player_NominalGravity = 0.0f;
/**
 * Storage group: Player ZRD runtime tuning globals.
 * BN types these as independent zero-filled .data floats written by
 * Player::InitMissionRuntimeFromWorldAndCamera from player.zrd gravity,
 * sink-rate, and slope nodes, with defaults derived there when nodes are
 * absent.
 * Purpose: Stores terrain and gravity tuning loaded from player.zrd.
 */
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-watergravity
 * @recoil-artifact defines .data recoil:data:0x4f3ab8: g_Player_WaterGravity.
 * Purpose: stores the plan-tracked g_Player_WaterGravity gameplay data symbol.
 */
float g_Player_WaterGravity = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-quicksandgravity
 * @recoil-artifact defines .data recoil:data:0x4f3ac0: g_Player_QuicksandGravity.
 * Purpose: stores the plan-tracked g_Player_QuicksandGravity gameplay data symbol.
 */
float g_Player_QuicksandGravity = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-quicksandsinkrate
 * @recoil-artifact defines .data recoil:data:0x4f376c: g_Player_QuicksandSinkRate.
 * Purpose: stores the plan-tracked g_Player_QuicksandSinkRate gameplay data symbol.
 */
float g_Player_QuicksandSinkRate = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-lavasinkrate
 * @recoil-artifact defines .data recoil:data:0x4f3698: g_Player_LavaSinkRate.
 * Purpose: stores the plan-tracked g_Player_LavaSinkRate gameplay data symbol.
 */
float g_Player_LavaSinkRate = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-maxslope
 * @recoil-artifact defines .data recoil:data:0x4f3338: g_Player_MaxSlope.
 * Purpose: stores the plan-tracked g_Player_MaxSlope gameplay data symbol.
 */
float g_Player_MaxSlope = 0.0f;
/**
 * Storage group: Player ZRD runtime tuning option pointers.
 * BN types these as zero-filled .data OptCatalogEntryDef pointers resolved
 * from player.zrd `make_hot` and `make_cold` option names during
 * Player::InitMissionRuntimeFromWorldAndCamera.
 * Purpose: Caches heat/cold gameplay option catalog entries for player damage paths.
 */
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-makehotoptentry
 * @recoil-artifact defines .data recoil:data:0x4f3734: g_Player_MakeHotOptEntry.
 * Purpose: stores the plan-tracked g_Player_MakeHotOptEntry gameplay data symbol.
 */
OptCatalogEntryDef* g_Player_MakeHotOptEntry = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-makecoldoptentry
 * @recoil-artifact defines .data recoil:data:0x4f3738: g_Player_MakeColdOptEntry.
 * Purpose: stores the plan-tracked g_Player_MakeColdOptEntry gameplay data symbol.
 */
OptCatalogEntryDef* g_Player_MakeColdOptEntry = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-bftsplashanimentry
 * @recoil-artifact defines .data recoil:data:0x4f3740: g_Player_BftSplashAnimEntry.
 * BN types this as a zero-filled .data zEffectAnimEntry pointer cached from
 * the "bftsplash" animation during mission-runtime bootstrap.
 * Purpose: Caches the battle-force splash animation entry for gameplay FX.
 */
zEffectAnimEntry* g_Player_BftSplashAnimEntry = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-activedebugscriptasyncentry
 * @recoil-artifact defines .data recoil:data:0x4f3a90: g_Player_ActiveDebugScriptAsyncEntry.
 * Purpose: stores the plan-tracked g_Player_ActiveDebugScriptAsyncEntry gameplay data symbol.
 */
zEffectAnimEntry* g_Player_ActiveDebugScriptAsyncEntry = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-horizonnodefollowcameraenabled
 * @recoil-artifact defines .data recoil:data:0x4f3768: g_Player_HorizonNodeFollowCameraEnabled.
 * Purpose: Stores g Player HorizonNodeFollowCameraEnabled data used by
 * battlesport_gameplay.player_horizon_follow_globals.
 */
int g_Player_HorizonNodeFollowCameraEnabled = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-horizonnode
 * @recoil-artifact defines .data recoil:data:0x4f36c0: g_Player_HorizonNode.
 * Purpose: Stores g Player HorizonNode data used by battlesport_gameplay.player_horizon_follow_globals.
 */
CZNodePartial* g_Player_HorizonNode = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playerprevcamerastate
 * @recoil-artifact defines .data recoil:data:0x4f36d0: g_PlayerPrevCameraState.
 * Purpose: Stores g PlayerPrevCameraState data used by battlesport_gameplay.player_damage_runtime_globals.
 */
int g_PlayerPrevCameraState = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playerprevsteeringmode
 * @recoil-artifact defines .data recoil:data:0x4f36d4: g_PlayerPrevSteeringMode.
 * Purpose: Stores g PlayerPrevSteeringMode data used by battlesport_gameplay.player_damage_runtime_globals.
 */
int g_PlayerPrevSteeringMode = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-savedsteeringmode
 * @recoil-artifact defines .data recoil:data:0x4e5cc4: g_Player_SavedSteeringMode.
 * Purpose: stores the plan-tracked g_Player_SavedSteeringMode gameplay data symbol.
 */
int g_Player_SavedSteeringMode = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-coptersndnode1
 * @recoil-artifact defines .data recoil:data:0x4f36c4: g_Player_CopterSndNode1.
 * Purpose: stores the plan-tracked g_Player_CopterSndNode1 gameplay data symbol.
 */
CZNodePartial* g_Player_CopterSndNode1 = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-coptersndnode2
 * @recoil-artifact defines .data recoil:data:0x4f36c8: g_Player_CopterSndNode2.
 * Purpose: stores the plan-tracked g_Player_CopterSndNode2 gameplay data symbol.
 */
CZNodePartial* g_Player_CopterSndNode2 = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-coptersndsample
 * @recoil-artifact defines .data recoil:data:0x4f36cc: g_Player_CopterSndSample.
 * Purpose: stores the plan-tracked g_Player_CopterSndSample gameplay data symbol.
 */
zSndSample* g_Player_CopterSndSample = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playerrecenthitfxanimentry
 * @recoil-artifact defines .data recoil:data:0x4f373c: g_PlayerRecentHitFxAnimEntry.
 * Purpose: Stores g PlayerRecentHitFxAnimEntry data used by battlesport_gameplay.player_damage_runtime_globals.
 */
zEffectAnimEntry* g_PlayerRecentHitFxAnimEntry = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-localfxoffsetworldptr
 * @recoil-artifact defines .data recoil:data:0x779aa8: g_Player_LocalFxOffsetWorldPtr.
 * Purpose: stores the plan-tracked g_Player_LocalFxOffsetWorldPtr gameplay data symbol.
 */
zVec3* g_Player_LocalFxOffsetWorldPtr = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playersavestatelistauxptr
 * @recoil-artifact defines .data recoil:data:0x4dc264: g_PlayerSaveStateListAuxPtr.
 * Retail word is 0x4f3a78; no incoming use establishes this independent
 * source object's identity. BN retains an automatic pointer interpretation,
 * but its former save-state-aux symbol and accepted owner claims were withdrawn.
 * Purpose: retain the candidate initialized pointer while ownership is unresolved.
 */
int* g_PlayerSaveStateListAuxPtr = &g_PlayerSaveStateList.listAux;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-missioninitfirstrunflag
 * @recoil-artifact defines .data recoil:data:0x4dc268: g_Player_MissionInitFirstRunFlag.
 * BN types this as an initialized .data int with value 1, cleared after the
 * first mission-runtime HUD top-message panel registration.
 * Purpose: Ensures one-time attachment of player top-message HUD panels.
 */
int g_Player_MissionInitFirstRunFlag = 1;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-sourcefile-playercpp
 * @recoil-artifact defines .data recoil:data:0x4dc26c: g_Player_SourceFile_PlayerCpp.
 * BN types this as a writable player.cpp diagnostic source-file literal
 * referenced by ApplyMissionSaveData and zZarReadVehicleListSection.
 * Purpose: Stores the Player source-file path used by save/ZAR diagnostics.
 */
char g_Player_SourceFile_PlayerCpp[31] = "D:\\Proj\\Battlesport\\player.cpp";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-savedatamodifiedmsg
 * @recoil-artifact defines .data recoil:data:0x4dc28c: g_Player_SaveDataModifiedMsg.
 * BN types this as a writable diagnostic literal referenced by
 * ApplyMissionSaveData when a Player save payload has an unexpected size.
 * Purpose: Reports incompatible Player mission-save data.
 */
char g_Player_SaveDataModifiedMsg[72] = "Player save data structure has been modified. Cannot use this save set.";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-savevehiclelistsectionname
 * @recoil-artifact defines .data recoil:data:0x4dc2d4: g_Player_SaveVehicleListSectionName.
 * BN types this as a writable ZAR section-name literal referenced by
 * zZarRegisterSections for the VehicleList callbacks.
 * Purpose: Names the Player VehicleList ZAR section.
 */
char g_Player_SaveVehicleListSectionName[12] = "VehicleList";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-vehiclesavedatamodifiedmsg
 * @recoil-artifact defines .data recoil:data:0x4dc2e0: g_Player_VehicleSaveDataModifiedMsg.
 * BN types this as a writable diagnostic literal referenced by
 * zZarReadVehicleListSection when a VehicleList payload has an unexpected size.
 * Purpose: Reports incompatible VehicleList save data.
 */
char g_Player_VehicleSaveDataModifiedMsg[73]
    = "Vehicle save data structure has been modified. Cannot use this save set.";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-aivarchivemissingmsg
 * @recoil-artifact defines .data recoil:data:0x4dc368: g_Player_AivArchiveMissingMsg.
 * BN types this as a writable diagnostic literal referenced by
 * Player::InitMissionRuntimeFromWorldAndCamera when aiv.zrd is missing.
 * Purpose: Reports that the mission AIV archive could not be loaded.
 */
char g_Player_AivArchiveMissingMsg[0x15] = "Cannot find aiv.zrd!";
/**
 * Storage group: Player mission/player.zrd writable literals.
 * BN types these as writable .data char arrays used by mission runtime
 * bootstrap, player.zrd tuning, vehicle/common/modal loaders, and copter
 * sound-node caching.
 * Purpose: Stores Player mission runtime and player.zrd literal names.
 */
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-confignode-basic
 * @recoil-artifact defines .data recoil:data:0x4dc380: g_Player_ConfigNode_Basic.
 * Purpose: stores the plan-tracked g_Player_ConfigNode_Basic gameplay data symbol.
 */
char g_Player_ConfigNode_Basic[6] = "basic";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-confignode-commonmode
 * @recoil-artifact defines .data recoil:data:0x4dc388: g_Player_ConfigNode_CommonMode.
 * Purpose: stores the plan-tracked g_Player_ConfigNode_CommonMode gameplay data symbol.
 */
char g_Player_ConfigNode_CommonMode[12] = "common_mode";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-confignode-stealth
 * @recoil-artifact defines .data recoil:data:0x4dc394: g_Player_ConfigNode_Stealth.
 * Purpose: stores the plan-tracked g_Player_ConfigNode_Stealth gameplay data symbol.
 */
char g_Player_ConfigNode_Stealth[8] = "stealth";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-displayname-stealth
 * @recoil-artifact defines .data recoil:data:0x4dc39c: g_Player_DisplayName_Stealth.
 * Purpose: stores the plan-tracked g_Player_DisplayName_Stealth gameplay data symbol.
 */
char g_Player_DisplayName_Stealth[8] = "Stealth";

/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-lowshieldsndname
 * @recoil-artifact defines .data recoil:data:0x4dc3b0: g_Player_LowShieldSndName.
 * Purpose: stores the plan-tracked g_Player_LowShieldSndName gameplay data symbol.
 */
char g_Player_LowShieldSndName[15] = "low_shield_snd";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-burninganimname
 * @recoil-artifact defines .data recoil:data:0x4dc3c0: g_Player_BurningAnimName.
 * Purpose: stores the plan-tracked g_Player_BurningAnimName gameplay data symbol.
 */
char g_Player_BurningAnimName[13] = "burning_anim";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-makecold
 * @recoil-artifact defines .data recoil:data:0x4dc3d0: g_Player_ConfigKey_MakeCold.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_MakeCold gameplay data symbol.
 */
char g_Player_ConfigKey_MakeCold[10] = "make_cold";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-makehot
 * @recoil-artifact defines .data recoil:data:0x4dc3dc: g_Player_ConfigKey_MakeHot.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_MakeHot gameplay data symbol.
 */
char g_Player_ConfigKey_MakeHot[9] = "make_hot";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-maxslope
 * @recoil-artifact defines .data recoil:data:0x4dc3e8: g_Player_ConfigKey_MaxSlope.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_MaxSlope gameplay data symbol.
 */
char g_Player_ConfigKey_MaxSlope[10] = "max_slope";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-lavasink
 * @recoil-artifact defines .data recoil:data:0x4dc3f4: g_Player_ConfigKey_LavaSink.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_LavaSink gameplay data symbol.
 */
char g_Player_ConfigKey_LavaSink[10] = "lava_sink";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-quicksandsink
 * @recoil-artifact defines .data recoil:data:0x4dc400: g_Player_ConfigKey_QuicksandSink.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_QuicksandSink gameplay data symbol.
 */
char g_Player_ConfigKey_QuicksandSink[11] = "qsand_sink";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-quicksandgravity
 * @recoil-artifact defines .data recoil:data:0x4dc40c: g_Player_ConfigKey_QuicksandGravity.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_QuicksandGravity gameplay data symbol.
 */
char g_Player_ConfigKey_QuicksandGravity[12] = "qsd_gravity";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-watergravity
 * @recoil-artifact defines .data recoil:data:0x4dc418: g_Player_ConfigKey_WaterGravity.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_WaterGravity gameplay data symbol.
 */
char g_Player_ConfigKey_WaterGravity[12] = "wat_gravity";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-normalgravity
 * @recoil-artifact defines .data recoil:data:0x4dc424: g_Player_ConfigKey_NormalGravity.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_NormalGravity gameplay data symbol.
 */
char g_Player_ConfigKey_NormalGravity[12] = "nom_gravity";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-maxcamtetherangle
 * @recoil-artifact defines .data recoil:data:0x4dc430: g_Player_ConfigKey_MaxCamTetherAngle.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_MaxCamTetherAngle gameplay data symbol.
 */
char g_Player_ConfigKey_MaxCamTetherAngle[21] = "max_cam_tether_angle";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-cameraelastic
 * @recoil-artifact defines .data recoil:data:0x4dc448: g_Player_ConfigKey_CameraElastic.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_CameraElastic gameplay data symbol.
 */
char g_Player_ConfigKey_CameraElastic[15] = "camera_elastic";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-underwatercam
 * @recoil-artifact defines .data recoil:data:0x4dc458: g_Player_ConfigKey_UnderwaterCam.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_UnderwaterCam gameplay data symbol.
 */
char g_Player_ConfigKey_UnderwaterCam[15] = "underwater_cam";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-firstpersoncamelevationlimit
 * @recoil-artifact defines .data recoil:data:0x4dc468: g_Player_ConfigKey_FirstPersonCamElevationLimit.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_FirstPersonCamElevationLimit gameplay data symbol.
 */
char g_Player_ConfigKey_FirstPersonCamElevationLimit[14] = "fp_cam_el_lim";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-firstpersoncamelevationrate
 * @recoil-artifact defines .data recoil:data:0x4dc478: g_Player_ConfigKey_FirstPersonCamElevationRate.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_FirstPersonCamElevationRate gameplay data symbol.
 */
char g_Player_ConfigKey_FirstPersonCamElevationRate[15] = "fp_cam_el_rate";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-mousepush
 * @recoil-artifact defines .data recoil:data:0x4dc488: g_Player_ConfigKey_MousePush.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_MousePush gameplay data symbol.
 */
char g_Player_ConfigKey_MousePush[11] = "mouse_push";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-maxcamyawrate
 * @recoil-artifact defines .data recoil:data:0x4dc494: g_Player_ConfigKey_MaxCamYawRate.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_MaxCamYawRate gameplay data symbol.
 */
char g_Player_ConfigKey_MaxCamYawRate[17] = "max_cam_yaw_rate";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-camerazone
 * @recoil-artifact defines .data recoil:data:0x4dc4a8: g_Player_ConfigKey_CameraZone.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_CameraZone gameplay data symbol.
 */
char g_Player_ConfigKey_CameraZone[12] = "camera_zone";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configarchivename
 * @recoil-artifact defines .data recoil:data:0x4dc4b4: g_Player_ConfigArchiveName.
 * Purpose: stores the plan-tracked g_Player_ConfigArchiveName gameplay data symbol.
 */
char g_Player_ConfigArchiveName[11] = "player.zrd";

/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-horizon
 * @recoil-artifact defines .data recoil:data:0x4dc4cc: g_Player_NodeName_Horizon.
 * Purpose: stores the plan-tracked g_Player_NodeName_Horizon gameplay data symbol.
 */
char g_Player_NodeName_Horizon[8] = "horizon";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-shadow
 * @recoil-artifact defines .data recoil:data:0x4dc4f0: g_Player_NodeName_Shadow.
 * Purpose: Player init-state node name for the shadow/mode-variant node.
 */
char g_Player_NodeName_Shadow[7] = "shadow";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-doorright
 * @recoil-artifact defines .data recoil:data:0x4dc4f8: g_Player_NodeName_DoorRight.
 * Purpose: Player init-state node name for the right door node.
 */
char g_Player_NodeName_DoorRight[10] = "doorright";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-doorleft
 * @recoil-artifact defines .data recoil:data:0x4dc504: g_Player_NodeName_DoorLeft.
 * Purpose: Player init-state node name for the left door node.
 */
char g_Player_NodeName_DoorLeft[9] = "doorleft";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-turret
 * @recoil-artifact defines .data recoil:data:0x4dc510: g_Player_NodeName_Turret.
 * Purpose: Shared Player/GameNet node name for the turret node.
 */
char g_Player_NodeName_Turret[7] = "turret";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-body
 * @recoil-artifact defines .data recoil:data:0x4dc518: g_Player_NodeName_Body.
 * Purpose: Player init-state node name for the body node.
 */
char g_Player_NodeName_Body[5] = "body";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-target
 * @recoil-artifact defines .data recoil:data:0x4dc520: g_Player_NodeName_Target.
 * Purpose: Player init-state node name for the target node.
 */
char g_Player_NodeName_Target[7] = "target";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-bft
 * @recoil-artifact defines .data recoil:data:0x4dc528: g_Player_NodeName_Bft.
 * Purpose: Shared Player/GameNet node name for BFT state lookup.
 */
char g_Player_NodeName_Bft[4] = "bft";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-bft00
 * @recoil-artifact defines .data recoil:data:0x4dc52c: g_Player_NodeName_Bft00.
 * Purpose: Player bootstrap node name for the initial BFT actor.
 */
char g_Player_NodeName_Bft00[7] = "bft_00";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-subt
 * @recoil-artifact defines .data recoil:data:0x4dc538: g_Player_NodeName_Subt.
 * Purpose: Player init-state animation/effect name for the subt node.
 */
char g_Player_NodeName_Subt[5] = "subt";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-bftbubbleeffectname
 * @recoil-artifact defines .data recoil:data:0x4dc540: g_Player_BftBubbleEffectName.
 * Purpose: Player init-state effect name for the BFT bubble animation.
 */
char g_Player_BftBubbleEffectName[12] = "bft_bubble1";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-napalmvehicleeffectname
 * @recoil-artifact defines .data recoil:data:0x4dc54c: g_Player_NapalmVehicleEffectName.
 * Purpose: Shared Player/zTurret effect name for napalm vehicle animation.
 */
char g_Player_NapalmVehicleEffectName[15] = "napalm_vehicle";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-shockvehicleeffectname
 * @recoil-artifact defines .data recoil:data:0x4dc55c: g_Player_ShockVehicleEffectName.
 * Purpose: Player init-state effect name for shock vehicle animation.
 */
char g_Player_ShockVehicleEffectName[14] = "shock_vehicle";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-shattervehicleeffectname
 * @recoil-artifact defines .data recoil:data:0x4dc56c: g_Player_ShatterVehicleEffectName.
 * Purpose: Player init-state effect name for shatter vehicle animation.
 */
char g_Player_ShatterVehicleEffectName[16] = "shatter_vehicle";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-bftexhausttrailname
 * @recoil-artifact defines .data recoil:data:0x4dc57c: g_Player_BftExhaustTrailName.
 * Purpose: Player init-state trail name for BFT exhaust.
 */
char g_Player_BftExhaustTrailName[18] = "bft_exhaust_trail";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-boatwaketrailname
 * @recoil-artifact defines .data recoil:data:0x4dc590: g_Player_BoatWakeTrailName.
 * Purpose: Player init-state trail name for boat wake.
 */
char g_Player_BoatWakeTrailName[16] = "boat_wake_trail";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-regenskinnodename
 * @recoil-artifact defines .data recoil:data:0x4dc5a0: g_Player_RegenSkinNodeName.
 * Purpose: Player init-state effect name for the regen skin node.
 */
char g_Player_RegenSkinNodeName[11] = "regen_skin";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mastercommondatamissingfmt
 * @recoil-artifact defines .data recoil:data:0x4dc5ac: g_Player_MasterCommonDataMissingFmt.
 * Purpose: Diagnostic emitted when Player master common data is missing.
 */
char g_Player_MasterCommonDataMissingFmt[39] = "Cannot find Master Common Data for %s!";
/**
 * Storage group: Player modal-bind writable literals.
 * BN types these as writable .data char arrays used by modal-state node
 * binding and model-derived support/collision point construction; the
 * intervening Bft99 and shared path-join literals belong to separate owners.
 * Purpose: Stores Player modal binding and modal point-builder literal names.
 */
char g_Player_CollisionPointsMissingFmt[37] = "Cannot find collision points for %s!";
char g_Player_SupportPointsMissingFmt[35] = "Cannot find support points for %s!";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-effectnodename-dustright
 * @recoil-artifact defines .data recoil:data:0x4dc620: g_Player_EffectNodeName_DustRight.
 * Purpose: stores the plan-tracked g_Player_EffectNodeName_DustRight gameplay data symbol.
 */
char g_Player_EffectNodeName_DustRight[7] = "dust_r";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-effectnodename-dustleft
 * @recoil-artifact defines .data recoil:data:0x4dc628: g_Player_EffectNodeName_DustLeft.
 * Purpose: stores the plan-tracked g_Player_EffectNodeName_DustLeft gameplay data symbol.
 */
char g_Player_EffectNodeName_DustLeft[7] = "dust_l";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-effectnodename-splashright
 * @recoil-artifact defines .data recoil:data:0x4dc630: g_Player_EffectNodeName_SplashRight.
 * Purpose: stores the plan-tracked g_Player_EffectNodeName_SplashRight gameplay data symbol.
 */
char g_Player_EffectNodeName_SplashRight[9] = "splash_r";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-effectnodename-splashleft
 * @recoil-artifact defines .data recoil:data:0x4dc63c: g_Player_EffectNodeName_SplashLeft.
 * Purpose: stores the plan-tracked g_Player_EffectNodeName_SplashLeft gameplay data symbol.
 */
char g_Player_EffectNodeName_SplashLeft[9] = "splash_l";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-effectnodename-wake
 * @recoil-artifact defines .data recoil:data:0x4dc648: g_Player_EffectNodeName_Wake.
 * Purpose: stores the plan-tracked g_Player_EffectNodeName_Wake gameplay data symbol.
 */
char g_Player_EffectNodeName_Wake[5] = "wake";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-effectnodename-caustic1
 * @recoil-artifact defines .data recoil:data:0x4dc650: g_Player_EffectNodeName_Caustic1.
 * Purpose: stores the plan-tracked g_Player_EffectNodeName_Caustic1 gameplay data symbol.
 */
char g_Player_EffectNodeName_Caustic1[9] = "caustic1";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-props
 * @recoil-artifact defines .data recoil:data:0x4dc65c: g_Player_NodeName_Props.
 * Purpose: stores the plan-tracked g_Player_NodeName_Props gameplay data symbol.
 */
char g_Player_NodeName_Props[6] = "props";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-lefttracks
 * @recoil-artifact defines .data recoil:data:0x4dc664: g_Player_NodeName_LeftTracks.
 * Purpose: stores the plan-tracked g_Player_NodeName_LeftTracks gameplay data symbol.
 */
char g_Player_NodeName_LeftTracks[8] = "ltracks";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-righttracks
 * @recoil-artifact defines .data recoil:data:0x4dc66c: g_Player_NodeName_RightTracks.
 * Purpose: stores the plan-tracked g_Player_NodeName_RightTracks gameplay data symbol.
 */
char g_Player_NodeName_RightTracks[8] = "rtracks";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-chassis
 * @recoil-artifact defines .data recoil:data:0x4dc674: g_Player_NodeName_Chassis.
 * Purpose: stores the plan-tracked g_Player_NodeName_Chassis gameplay data symbol.
 */
char g_Player_NodeName_Chassis[8] = "chassis";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-leftmorphs
 * @recoil-artifact defines .data recoil:data:0x4dc67c: g_Player_NodeName_LeftMorphs.
 * Purpose: stores the plan-tracked g_Player_NodeName_LeftMorphs gameplay data symbol.
 */
char g_Player_NodeName_LeftMorphs[12] = "left_morphs";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-rightmorphs
 * @recoil-artifact defines .data recoil:data:0x4dc688: g_Player_NodeName_RightMorphs.
 * Purpose: stores the plan-tracked g_Player_NodeName_RightMorphs gameplay data symbol.
 */
char g_Player_NodeName_RightMorphs[13] = "right_morphs";
char g_Player_MasterModalDataMissingFmt[38] = "Cannot find Master Modal Data for %s!";

/**
 * Storage group: Player master ZRD record-loader writable literals.
 * BN types these as writable .data char arrays used by
 * Player::LoadMasterCommonDataFromNode and
 * Player::LoadMasterModalDataFromNode for common-mode, modal, sound, FX,
 * wave, movement, collision, platform, and master-type ZRD record lookups.
 * Purpose: Stores Player master ZRD record-loader literal names.
 */
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-weapons
 * @recoil-artifact defines .data recoil:data:0x4dc6e8: g_Player_NodeName_Weapons.
 * Purpose: Names the common-mode weapons record.
 */
char g_Player_NodeName_Weapons[8] = "weapons";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-pickups
 * @recoil-artifact defines .data recoil:data:0x4dc6f0: g_Player_NodeName_Pickups.
 * Purpose: Names the common-mode pickups record.
 */
char g_Player_NodeName_Pickups[8] = "pickups";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-health
 * @recoil-artifact defines .data recoil:data:0x4dc6f8: g_Player_NodeName_Health.
 * Purpose: Names the common-mode health record.
 */
char g_Player_NodeName_Health[7] = "health";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-trackswitch
 * @recoil-artifact defines .data recoil:data:0x4dc700: g_Player_NodeName_TrackSwitch.
 * Purpose: Names the common-mode track-switch record.
 */
char g_Player_NodeName_TrackSwitch[13] = "track_switch";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-cameraudswing
 * @recoil-artifact defines .data recoil:data:0x4dc710: g_Player_NodeName_CameraUdSwing.
 * Purpose: Names the common-mode camera swing record.
 */
char g_Player_NodeName_CameraUdSwing[16] = "camera_ud_swing";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-aimy
 * @recoil-artifact defines .data recoil:data:0x4dc720: g_Player_NodeName_AimY.
 * Purpose: Names the common-mode aim-yaw record.
 */
char g_Player_NodeName_AimY[5] = "aimy";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-camback
 * @recoil-artifact defines .data recoil:data:0x4dc728: g_Player_NodeName_CamBack.
 * Purpose: Names the common-mode camera-back record.
 */
char g_Player_NodeName_CamBack[8] = "camback";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-startanims
 * @recoil-artifact defines .data recoil:data:0x4dc730: g_Player_NodeName_StartAnims.
 * Purpose: Names the common-mode start-animations record.
 */
char g_Player_NodeName_StartAnims[12] = "start_anims";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-activation
 * @recoil-artifact defines .data recoil:data:0x4dc73c: g_Player_NodeName_Activation.
 * Purpose: Names the common-mode activation record.
 */
char g_Player_NodeName_Activation[11] = "activation";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-pinging
 * @recoil-artifact defines .data recoil:data:0x4dc748: g_Player_NodeName_Pinging.
 * Purpose: Names the common-mode pinging sound record.
 */
char g_Player_NodeName_Pinging[8] = "pinging";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-weaponselect
 * @recoil-artifact defines .data recoil:data:0x4dc750: g_Player_NodeName_WeaponSelect.
 * Purpose: Names the common-mode weapon-select sound record.
 */
char g_Player_NodeName_WeaponSelect[14] = "weapon_select";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-weaponup
 * @recoil-artifact defines .data recoil:data:0x4dc760: g_Player_NodeName_WeaponUp.
 * Purpose: Names the common-mode weapon-up sound record.
 */
char g_Player_NodeName_WeaponUp[10] = "weapon_up";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-sounds
 * @recoil-artifact defines .data recoil:data:0x4dc76c: g_Player_NodeName_Sounds.
 * Purpose: Names the master record sounds child.
 */
char g_Player_NodeName_Sounds[7] = "sounds";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-nanite
 * @recoil-artifact defines .data recoil:data:0x4dc774: g_Player_NodeName_Nanite.
 * Purpose: Names the common-mode nanite record.
 */
char g_Player_NodeName_Nanite[7] = "nanite";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-volumescale
 * @recoil-artifact defines .data recoil:data:0x4dc77c: g_Player_NodeName_VolumeScale.
 * Purpose: Names the modal sound volume scale record.
 */
char g_Player_NodeName_VolumeScale[13] = "volume_scale";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-pitchscale
 * @recoil-artifact defines .data recoil:data:0x4dc78c: g_Player_NodeName_PitchScale.
 * Purpose: Names the modal sound pitch scale record.
 */
char g_Player_NodeName_PitchScale[12] = "pitch_scale";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-land
 * @recoil-artifact defines .data recoil:data:0x4dc798: g_Player_NodeName_Land.
 * Purpose: Names the modal land sound record.
 */
char g_Player_NodeName_Land[5] = "land";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-collide
 * @recoil-artifact defines .data recoil:data:0x4dc7a0: g_Player_NodeName_Collide.
 * Purpose: Names the modal collide sound record.
 */
char g_Player_NodeName_Collide[8] = "collide";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-external
 * @recoil-artifact defines .data recoil:data:0x4dc7a8: g_Player_NodeName_External.
 * Purpose: Names the modal external-engine sound record.
 */
char g_Player_NodeName_External[9] = "external";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-engine
 * @recoil-artifact defines .data recoil:data:0x4dc7b4: g_Player_NodeName_Engine.
 * Purpose: Names the modal engine sound record.
 */
char g_Player_NodeName_Engine[7] = "engine";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-skid
 * @recoil-artifact defines .data recoil:data:0x4dc7bc: g_Player_NodeName_Skid.
 * Purpose: Names the modal skid sound record.
 */
char g_Player_NodeName_Skid[5] = "skid";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-idle
 * @recoil-artifact defines .data recoil:data:0x4dc7c4: g_Player_NodeName_Idle.
 * Purpose: Names the modal idle sound record.
 */
char g_Player_NodeName_Idle[5] = "idle";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-s2aanims
 * @recoil-artifact defines .data recoil:data:0x4dc7cc: g_Player_NodeName_S2AAnims.
 * Purpose: Names the sub-to-amphib FX list record.
 */
char g_Player_NodeName_S2AAnims[10] = "s2a_anims";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-a2sanims
 * @recoil-artifact defines .data recoil:data:0x4dc7d8: g_Player_NodeName_A2SAnims.
 * Purpose: Names the amphib-to-sub FX list record.
 */
char g_Player_NodeName_A2SAnims[10] = "a2s_anims";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-h2aanims
 * @recoil-artifact defines .data recoil:data:0x4dc7e4: g_Player_NodeName_H2AAnims.
 * Purpose: Names the hover-to-amphib FX list record.
 */
char g_Player_NodeName_H2AAnims[10] = "h2a_anims";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-a2hanims
 * @recoil-artifact defines .data recoil:data:0x4dc7f0: g_Player_NodeName_A2HAnims.
 * Purpose: Names the amphib-to-hover FX list record.
 */
char g_Player_NodeName_A2HAnims[10] = "a2h_anims";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-h2tanims
 * @recoil-artifact defines .data recoil:data:0x4dc7fc: g_Player_NodeName_H2TAnims.
 * Purpose: Names the hover-to-track FX list record.
 */
char g_Player_NodeName_H2TAnims[10] = "h2t_anims";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-t2hanims
 * @recoil-artifact defines .data recoil:data:0x4dc808: g_Player_NodeName_T2HAnims.
 * Purpose: Names the track-to-hover FX list record.
 */
char g_Player_NodeName_T2HAnims[10] = "t2h_anims";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-a2tanims
 * @recoil-artifact defines .data recoil:data:0x4dc814: g_Player_NodeName_A2TAnims.
 * Purpose: Names the amphib-to-track FX list record.
 */
char g_Player_NodeName_A2TAnims[10] = "a2t_anims";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-t2aanims
 * @recoil-artifact defines .data recoil:data:0x4dc820: g_Player_NodeName_T2AAnims.
 * Purpose: Names the track-to-amphib FX list record.
 */
char g_Player_NodeName_T2AAnims[10] = "t2a_anims";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-collisiondamage
 * @recoil-artifact defines .data recoil:data:0x4dc82c: g_Player_NodeName_CollisionDamage.
 * Purpose: Names the modal collision damping record.
 */
char g_Player_NodeName_CollisionDamage[12] = "collision_d";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-chassisroll
 * @recoil-artifact defines .data recoil:data:0x4dc838: g_Player_NodeName_ChassisRoll.
 * Purpose: Names the modal chassis roll record.
 */
char g_Player_NodeName_ChassisRoll[10] = "chas_roll";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-chassispitch
 * @recoil-artifact defines .data recoil:data:0x4dc844: g_Player_NodeName_ChassisPitch.
 * Purpose: Names the modal chassis pitch record.
 */
char g_Player_NodeName_ChassisPitch[11] = "chas_pitch";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-chassissmooth
 * @recoil-artifact defines .data recoil:data:0x4dc850: g_Player_NodeName_ChassisSmooth.
 * Purpose: Names the modal chassis smoothing record.
 */
char g_Player_NodeName_ChassisSmooth[12] = "chas_smooth";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-modealt
 * @recoil-artifact defines .data recoil:data:0x4dc85c: g_Player_NodeName_ModeAlt.
 * Purpose: Names the modal alternate-mode transition record.
 */
char g_Player_NodeName_ModeAlt[9] = "mode_alt";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-subwave
 * @recoil-artifact defines .data recoil:data:0x4dc868: g_Player_ConfigKey_SubWave.
 * Purpose: Names the modal submarine wave record.
 */
char g_Player_ConfigKey_SubWave[9] = "sub_wave";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-hoverwave
 * @recoil-artifact defines .data recoil:data:0x4dc874: g_Player_ConfigKey_HoverWave.
 * Purpose: Names the modal hover wave record.
 */
char g_Player_ConfigKey_HoverWave[11] = "hover_wave";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-amphibwave
 * @recoil-artifact defines .data recoil:data:0x4dc880: g_Player_ConfigKey_AmphibWave.
 * Purpose: Names the modal amphib wave record.
 */
char g_Player_ConfigKey_AmphibWave[12] = "amphib_wave";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-gunpitch
 * @recoil-artifact defines .data recoil:data:0x4dc88c: g_Player_ConfigKey_GunPitch.
 * Purpose: Names the modal gun pitch record.
 */
char g_Player_ConfigKey_GunPitch[10] = "gun_pitch";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-mass
 * @recoil-artifact defines .data recoil:data:0x4dc898: g_Player_ConfigKey_Mass.
 * Purpose: Names the modal mass record.
 */
char g_Player_ConfigKey_Mass[5] = "mass";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-altcontrol
 * @recoil-artifact defines .data recoil:data:0x4dc8a0: g_Player_ConfigKey_AltControl.
 * Purpose: Names the modal alternate-control record.
 */
char g_Player_ConfigKey_AltControl[12] = "alt_control";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-acceldamping
 * @recoil-artifact defines .data recoil:data:0x4dc8ac: g_Player_ConfigKey_AccelDamping.
 * Purpose: Names the modal acceleration damping record.
 */
char g_Player_ConfigKey_AccelDamping[10] = "a_damping";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-ratedamping
 * @recoil-artifact defines .data recoil:data:0x4dc8b8: g_Player_ConfigKey_RateDamping.
 * Purpose: Names the modal rate damping record.
 */
char g_Player_ConfigKey_RateDamping[13] = "rate_damping";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-turndamping
 * @recoil-artifact defines .data recoil:data:0x4dc8c8: g_Player_ConfigKey_TurnDamping.
 * Purpose: Names the modal turn damping record.
 */
char g_Player_ConfigKey_TurnDamping[13] = "turn_damping";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-turns
 * @recoil-artifact defines .data recoil:data:0x4dc8d8: g_Player_ConfigKey_Turns.
 * Purpose: Names the modal turn-rate record.
 */
char g_Player_ConfigKey_Turns[6] = "turns";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-lavaslowdown
 * @recoil-artifact defines .data recoil:data:0x4dc8e0: g_Player_ConfigKey_LavaSlowdown.
 * Purpose: Names the modal lava slowdown record.
 */
char g_Player_ConfigKey_LavaSlowdown[14] = "lava_slowdown";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-quicksandslowdown
 * @recoil-artifact defines .data recoil:data:0x4dc8f0: g_Player_ConfigKey_QuicksandSlowdown.
 * Purpose: Names the modal quicksand slowdown record.
 */
char g_Player_ConfigKey_QuicksandSlowdown[19] = "quicksand_slowdown";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-stopping
 * @recoil-artifact defines .data recoil:data:0x4dc904: g_Player_ConfigKey_Stopping.
 * Purpose: Names the modal stopping-force record.
 */
char g_Player_ConfigKey_Stopping[9] = "stopping";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-friction
 * @recoil-artifact defines .data recoil:data:0x4dc910: g_Player_ConfigKey_Friction.
 * Purpose: Names the modal friction record.
 */
char g_Player_ConfigKey_Friction[9] = "friction";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-rates
 * @recoil-artifact defines .data recoil:data:0x4dc91c: g_Player_ConfigKey_Rates.
 * Purpose: Names the modal acceleration-rate record.
 */
char g_Player_ConfigKey_Rates[6] = "rates";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-collision
 * @recoil-artifact defines .data recoil:data:0x4dc924: g_Player_ConfigKey_Collision.
 * Purpose: Names the modal collision probe list record.
 */
char g_Player_ConfigKey_Collision[10] = "collision";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-platform
 * @recoil-artifact defines .data recoil:data:0x4dc930: g_Player_ConfigKey_Platform.
 * Purpose: Names the modal platform probe list record.
 */
char g_Player_ConfigKey_Platform[9] = "platform";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configvalue-mastertypeunknown
 * @recoil-artifact defines .data recoil:data:0x4dc93c: g_Player_ConfigValue_MasterTypeUnknown.
 * Purpose: Stores the fallback modal master type name.
 */
char g_Player_ConfigValue_MasterTypeUnknown[8] = "unknown";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configvalue-mastertypefly
 * @recoil-artifact defines .data recoil:data:0x4dc944: g_Player_ConfigValue_MasterTypeFly.
 * Purpose: Stores the fly modal master type name.
 */
char g_Player_ConfigValue_MasterTypeFly[4] = "fly";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configvalue-mastertypesub
 * @recoil-artifact defines .data recoil:data:0x4dc948: g_Player_ConfigValue_MasterTypeSub.
 * Purpose: Stores the sub modal master type name.
 */
char g_Player_ConfigValue_MasterTypeSub[4] = "sub";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configvalue-mastertypeamphib
 * @recoil-artifact defines .data recoil:data:0x4dc94c: g_Player_ConfigValue_MasterTypeAmphib.
 * Purpose: Stores the amphib modal master type name.
 */
char g_Player_ConfigValue_MasterTypeAmphib[7] = "amphib";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configvalue-mastertypehover
 * @recoil-artifact defines .data recoil:data:0x4dc954: g_Player_ConfigValue_MasterTypeHover.
 * Purpose: Stores the hover modal master type name.
 */
char g_Player_ConfigValue_MasterTypeHover[6] = "hover";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configvalue-mastertypetrack
 * @recoil-artifact defines .data recoil:data:0x4dc95c: g_Player_ConfigValue_MasterTypeTrack.
 * Purpose: Stores the track modal master type name.
 */
char g_Player_ConfigValue_MasterTypeTrack[6] = "track";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-confignode-mode
 * @recoil-artifact defines .data recoil:data:0x4dc964: g_Player_ConfigNode_Mode.
 * Purpose: Names the modal mode record.
 */
char g_Player_ConfigNode_Mode[5] = "mode";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-aivparentdir
 * @recoil-artifact defines .data recoil:data:0x4e5b50: g_Player_AivParentDir.
 * BN types this as a zero-filled char[0x104] buffer written by
 * zReader::BuildResolvedParentDir after aiv.zrd is loaded; final-data
 * evidence places the source symbol in player.obj BSS while the retail range
 * straddles the PE raw/zero-fill boundary.
 * Purpose: Stores the resolved parent directory for AIV-relative player data.
 */
char g_Player_AivParentDir[0x104];
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-aivzrdpath
 * @recoil-artifact defines .data recoil:data:0x4dc32c: g_Player_AivZrdPath.
 * Purpose: names the player AIV archive loaded during mission bootstrap.
 */
char g_Player_AivZrdPath[8] = "aiv.zrd";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-vehiclearchivename-easy
 * @recoil-artifact defines .data recoil:data:0x4dc334: g_Player_VehicleArchiveName_Easy.
 * Purpose: names the easy-difficulty vehicle archive selected for AIV loads.
 */
char g_Player_VehicleArchiveName_Easy[17] = "vehicle_easy.zrd";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-vehiclearchivename-hard
 * @recoil-artifact defines .data recoil:data:0x4dc348: g_Player_VehicleArchiveName_Hard.
 * Purpose: names the hard-difficulty vehicle archive selected for AIV loads.
 */
char g_Player_VehicleArchiveName_Hard[17] = "vehicle_hard.zrd";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-vehiclearchivename-default
 * @recoil-artifact defines .data recoil:data:0x4dc35c: g_Player_VehicleArchiveName_Default.
 * Purpose: names the fallback vehicle archive selected for AIV loads.
 */
char g_Player_VehicleArchiveName_Default[12] = "vehicle.zrd";
}

extern const zVec3 g_Player_ConstZeroVec3;
extern const zVec3 kPlayerDefaultAltGunAimOrigin;

namespace {
/**
 * Original inline helper; no standalone retail function exists. Observed in address-backed callers 0x4386c0, 0x4289f0,
 * 0x42c0d0, 0x42c2e0, 0x427440, 0x427ec0, 0x43a600, and 0x43a900 as a VC5-era int-bits smoothing idiom. Purpose:
 * reinterpret an IEEE-754 bit pattern as float.
 */
#define PLAYER_FLOAT_FROM_BITS(bits) (*(const float*)&(bits))
/**
 * Original inline helper; no standalone retail function exists.
 * Observed in address-backed callers 0x425a20 Player::TickLocalPlayerControls, 0x428520 Player::UpdateMasterTypeSub,
 * 0x426770 Player::UpdateMasterTypeTrack, 0x427440 Player::UpdateMasterTypeHoverFromModalProbe. Purpose: clamp a
 * caller-owned scalar without emitting an out-of-line helper under the retail translation unit's /Ob0 compile mode.
 */
#define PLAYER_CLAMP_SIGNED(value, limit)                                                                              \
    do {                                                                                                               \
        if ((value) > (limit)) {                                                                                       \
            (value) = (limit);                                                                                         \
        } else if ((value) < -(limit)) {                                                                               \
            (value) = -(limit);                                                                                        \
        }                                                                                                              \
    } while (0)

enum PlayerMasterTypeId {
    kPlayerMasterTypeFly = 1,
    kPlayerMasterTypeSub = 2,
    kPlayerMasterTypeTrack = 3,
    kPlayerMasterTypeHover = 4,
    kPlayerMasterTypeAmphib = 5
};
const int kPlayerAiMode2TopSteering = 1;
const int kPlayerAiMode2SteerDirectTarget = 0;
const int kPlayerAiMode2SteerOffsetTarget = 1;
const int kPlayerAiMode2SteerDynamicOffsetTarget = 2;
const int kPlayerAiMode2SteerPathFollow = 3;
const int kPlayerAiMode2SteerTurnInPlace = 5;
const int kPlayerAiMode2SteerAutoTurn = 6;
const float kPlayerAiAltGunAttackForwardMin = 0.75f;
const float kPlayerAiAltGunStatusMinScale = 0.5f;
const int kPlayerAiTopPathFollow = 0;
const int kPlayerAiTopTurnTowardTarget = 2;
const int kPlayerAiTopTurnOnlyTowardTarget = 3;
const int kPlayerAiTopPathSteering = 4;
const int kPlayerAiTopAutoTurn = 5;
const int kPlayerNodeFlagNetworkBftCloneSource = 1 << 22;
const float kPlayerMinFrameDeltaSec = 0.00499999989f;
const float kPlayerDeltaTimeScaled001Factor = 0.00999999978f;
const int kPlayerNanitePanelDisabledSentinel = 123456789;
const float kPlayerAltAmmoDisabledSentinel = 123456792.0f;
const float kPlayerRecentHitAlertSec = 5.0f;
const int kPlayerMissionSaveLegacySize = 0x124;
const unsigned int kPlayerGunControllerAvailableFlag = 0x04;
const unsigned int kPlayerGunControllerDualMountFlag = 0x02;
const unsigned int kPlayerGunControllerRecoilFlag = 0x01;
const unsigned int kOptCatalogFlagLockOnTargetRef = 0x4000;
const unsigned int kPlayerOptCatalogFlagTetherGuided = 1u << 20;
const unsigned int kOptCatalogFlagReload = 1u << 18;
const unsigned int kOptCatalogFlagCreateTrail = 0x02;
const int kCheckpointNodeAuxFlagTracked = 0x02;
const int kCheckpointNodePickableFlag = 0x40000;
const int kCheckpointNodeContextFlag = 0x200000;
const unsigned int kPlayerTimedHitStatusActiveFlag = 0x01;
const unsigned int kOptCatalogFlagBypassDamageProtection = 0x200;
const unsigned int kOptCatalogFlagRecordsRecentHit = 0x1000;
const unsigned int kOptCatalogFlagAppliesTimedHitStatus = 0x200000;
const unsigned int kOptCatalogFlagBlockedInSub = 0x1000;
const unsigned int kOptCatalogFlagNoSubUse = 0x02;
const int kPlayerTickCameraStateProjectileAttached = 7;
const int kPlayerTickCameraStateRestorePrevious = 8;
const float kPlayerDefaultActivationRange = 100.0f;
const float kPlayerDefaultReturnRange = 250.0f;
const float kPlayerDefaultNotPursuitDwellTime = 3.0f;
const float kPlayerDefaultMaxHealth = 100.0f;
const float kPlayerDefaultAiAttackRadiusSq = 1500.0f;
const float kPlayerDefaultAiAttackDwellTime = 10.0f;
const float kPlayerAiInitialStateDelaySec = 10.0f;
const float kPlayerAiPathFollowMinThrottle = 0.25f;
const float kPlayerAiPathFollowAdvanceDistance = 10.0f;
const float kPlayerAiForwardPathAdvanceDistance = 5.0f;
const float kPlayerAiSyntheticPathRebuildDistanceSq = 400.0f;
const float kPlayerAiSyntheticPathWidth = 10.0f;
const float kPlayerAiSyntheticPathRebuildDelaySec = 1.0f;
const float kPlayerAiAttackLosTargetYOffset = 1.5f;
const float kPlayerAiDynamicOffsetBackUpDistance = 10.0f;
const float kPlayerCameraState2TargetYOffset = 150.0f;
const double kPlayerRadiansToDegrees = 57.29577951308;

struct HitOwnerSaveStateLinkPartial {
    unsigned char unknown_00[0x04];
    zUtil_SaveGameState* ownerSaveState;
};

struct HitOwnerOrContextPartial {
    unsigned char unknown_00[0x40];
    HitOwnerSaveStateLinkPartial* ownerLink;
};

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in callers 0x41fe90, 0x422170, and 0x4226d0 as repeated direct
 * zReader node-array value loads. BN shows the caller bodies fold this access
 * into field reads instead of calling a helper target.
 * Purpose: return the child array backing a type-4 ZRD record node.
 */
#define PlayerZrdArrayBase(node) ((node)->value.nodes)

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in callers 0x41fe90, 0x422170, and 0x4226d0 as repeated reads of
 * the first child node's integer count before ZRD array loops and modal-count
 * calculations. The pattern is a source-level ZRD array accessor, not a
 * separate retail callee.
 * Purpose: return the stored element count for a ZRD array node.
 */
#define PlayerZrdArrayCount(node) (PlayerZrdArrayBase(node)[0].value.i32)

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in callers 0x41fe90, 0x422170, and 0x4226d0 as repeated string
 * fetches from indexed ZRD child records before strcpy, sound lookup, pickup
 * lookup, and FX lookup operations. BN shows inline child-array field reads at
 * those call sites.
 * Purpose: return a string field from an indexed ZRD array element.
 */
#define PlayerZrdArrayString(node, index) (PlayerZrdArrayBase(node)[index].value.str)

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in callers 0x41fe90 and 0x422170 as repeated integer fetches from
 * indexed ZRD child records for tuning fields, counts, capacities, gates, and
 * weapon specs. BN shows the loads inlined into the caller bodies.
 * Purpose: return an integer field from an indexed ZRD array element.
 */
#define PlayerZrdArrayInt(node, index) (PlayerZrdArrayBase(node)[index].value.i32)

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in callers 0x41fe90, 0x422170, and 0x4226d0 as repeated float
 * fetches from indexed ZRD child records for camera, common-mode, modal, wave,
 * and sound-scale data. BN shows direct float loads at those caller sites.
 * Purpose: return a float field from an indexed ZRD array element.
 */
#define PlayerZrdArrayFloat(node, index) PlayerZrdArrayBase(node)[index].value.f32

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in caller 0x41fe90 while selecting each vehicle modal child node
 * from the loaded vehicle ZRD array. BN shows address arithmetic over the
 * child-node array, with no separate helper target.
 * Purpose: return the indexed child node from a ZRD array node.
 */
#define PlayerZrdArrayNode(node, index) (&PlayerZrdArrayBase(node)[index])

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in callers 0x422170 and 0x4226d0 where string fields from ZRD
 * records are copied into fixed Player master-data name buffers. BN shows
 * strlen/memcpy strcpy expansions around direct child-array string loads.
 * Purpose: copy an indexed ZRD string field into a destination buffer.
 */
#define PlayerCopyZrdArrayString(dest, node, index) strcpy((dest), PlayerZrdArrayString((node), (index)))

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in callers 0x422170 and 0x4226d0 as repeated named child lookups
 * followed by first-string extraction and zSnd::FindSampleByName. BN shows the
 * full pattern repeated for common and modal sound records.
 * Purpose: resolve one optional named ZRD sound sample into a Player data slot.
 */
#define PlayerLoadSoundSample(parentNode, name, outSample)                                                             \
    do {                                                                                                               \
        zReader::Node* const playerSoundNode = zRdrGetNode((parentNode), (name));                                      \
        if (playerSoundNode != 0) {                                                                                    \
            *(outSample) = zSnd::FindSampleByName(PlayerZrdArrayString(playerSoundNode, 1));                           \
        }                                                                                                              \
    } while (0)

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in caller 0x4226d0 as the same count-and-copy loop for platform and
 * collision point lists. BN shows direct child-array vector loads and no
 * separate retail function target for the repeated loop.
 * Purpose: copy a ZRD list of xyz point records into a modal point buffer.
 */
#define PlayerLoadModalPointList(node, points, outCount)                                                               \
    do {                                                                                                               \
        zReader::Node* const playerPointListNode = (node);                                                             \
        if (playerPointListNode != 0) {                                                                                \
            const int playerPointCount = PlayerZrdArrayCount(playerPointListNode) - 1;                                 \
            *(outCount) = playerPointCount;                                                                            \
            for (int playerPointIndex = 0; playerPointIndex < playerPointCount; ++playerPointIndex) {                  \
                zReader::Node* const playerPointCoords                                                                 \
                    = PlayerZrdArrayBase(playerPointListNode)[playerPointIndex + 1].value.nodes;                       \
                (points)[playerPointIndex].x = playerPointCoords[1].value.f32;                                         \
                (points)[playerPointIndex].y = playerPointCoords[2].value.f32;                                         \
                (points)[playerPointIndex].z = playerPointCoords[3].value.f32;                                         \
            }                                                                                                          \
        } else {                                                                                                       \
            *(outCount) = 0;                                                                                           \
        }                                                                                                              \
    } while (0)

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in caller 0x4226d0 as eight repeated transition-FX list loads with
 * the same optional named-node lookup, two-entry cap, and
 * zEffectAnim::FindEntryByName dispatch. BN shows each loop body inlined.
 * Purpose: load up to two named transition FX entries from a modal ZRD list.
 */
#define PlayerLoadModalFxList(modalNode, name, entries)                                                                \
    do {                                                                                                               \
        zReader::Node* const playerFxListNode = zRdrGetNode((modalNode), (name));                                      \
        if (playerFxListNode != 0) {                                                                                   \
            int playerFxCount = PlayerZrdArrayCount(playerFxListNode);                                                 \
            if (playerFxCount >= 2) {                                                                                  \
                playerFxCount = 2;                                                                                     \
            }                                                                                                          \
            for (int playerFxIndex = 0; playerFxIndex < playerFxCount; ++playerFxIndex) {                              \
                (entries)[playerFxIndex]                                                                               \
                    = zEffectAnim::FindEntryByName(PlayerZrdArrayString(playerFxListNode, playerFxIndex + 1));         \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in caller 0x4226d0 as three repeated seven-float wave blocks for
 * amphib, hover, and sub modal records. BN shows direct field stores into the
 * same modal wave parameter slots rather than a separate helper call.
 * Purpose: overwrite modal hover wave parameters from a named ZRD record.
 */
#define PlayerLoadModalWaveParams(modalData, modalNode, name)                                                          \
    do {                                                                                                               \
        zReader::Node* const playerWaveNode = zRdrGetNode((modalNode), (name));                                        \
        if (playerWaveNode != 0) {                                                                                     \
            (modalData)->hoverPitchWaveBaseRate = PlayerZrdArrayFloat(playerWaveNode, 1);                              \
            (modalData)->hoverPitchWaveSpeedRate = PlayerZrdArrayFloat(playerWaveNode, 2);                             \
            (modalData)->hoverPitchWaveAmplitude = PlayerZrdArrayFloat(playerWaveNode, 3);                             \
            (modalData)->hoverRollWaveBaseRate = PlayerZrdArrayFloat(playerWaveNode, 4);                               \
            (modalData)->hoverRollWaveSpeedRate = PlayerZrdArrayFloat(playerWaveNode, 5);                              \
            (modalData)->hoverRollWaveAmplitude = PlayerZrdArrayFloat(playerWaveNode, 6);                              \
            (modalData)->hoverRollYawCoupleScale = PlayerZrdArrayFloat(playerWaveNode, 7);                             \
        }                                                                                                              \
    } while (0)

/**
 * Original inline helper; no standalone retail function exists.
 * Observed expanded inline three times at 0x41fe90
 * Player::InitMissionRuntimeFromWorldAndCamera as repeated
 * [gwObject3DInit, gwNodeSetPriority(2), gwNodeSetActionCallback] triples.
 * Purpose: register one player init action callback node without emitting an
 * out-of-line helper under the retail translation unit's /Ob0 compile mode.
 */
#define PLAYER_INIT_ACTION_CALLBACK_NODE(callback)                                                                     \
    do {                                                                                                               \
        CZNodePartial* playerInitActionNode = CZObject3D::gwObject3DInit();                                            \
        CZClass::gwNodeSetPriority(playerInitActionNode, 2);                                                           \
        CZClass::gwNodeSetActionCallback(playerInitActionNode, (callback));                                            \
    } while (0)

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x426770 Player::UpdateMasterTypeTrack, 0x42d5c0
 * Player::ApplyEnvironmentProbeResult. Purpose: provide the recovered cache attachment local offset helper for the
 * Player/Pickup gameplay source cluster.
 */
#define PLAYER_CACHE_ATTACHMENT_LOCAL_OFFSET(playerState)                                                              \
    do {                                                                                                               \
        const float playerAttachmentDx = (playerState)->worldPos.x - (playerState)->environmentAttachmentMatrix.posX;  \
        const float playerAttachmentDy = (playerState)->worldPos.y - (playerState)->environmentAttachmentMatrix.posY;  \
        const float playerAttachmentDz = (playerState)->worldPos.z - (playerState)->environmentAttachmentMatrix.posZ;  \
        const zMat4x3* const playerAttachmentMatrix = &(playerState)->environmentAttachmentMatrix;                     \
        (playerState)->fxOffsetLocal.x = playerAttachmentDx * playerAttachmentMatrix->xx                               \
            + playerAttachmentDy * playerAttachmentMatrix->xy + playerAttachmentDz * playerAttachmentMatrix->xz;       \
        (playerState)->fxOffsetLocal.y = playerAttachmentDx * playerAttachmentMatrix->yx                               \
            + playerAttachmentDy * playerAttachmentMatrix->yy + playerAttachmentDz * playerAttachmentMatrix->yz;       \
        (playerState)->fxOffsetLocal.z = playerAttachmentDx * playerAttachmentMatrix->zx                               \
            + playerAttachmentDy * playerAttachmentMatrix->zy + playerAttachmentDz * playerAttachmentMatrix->zz;       \
    } while (0)

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x41f1d0 Player::ApplyMissionSaveData.
 * Purpose: provide the recovered player saved weapon controller helper for
 * the Player/Pickup gameplay source cluster.
 */
PlayerGunFireController* PlayerSavedWeaponController(PlayerAltWeaponBank* bank, int sideIndex)
{
    return sideIndex == 0 ? &bank->controllerA : &bank->controllerB;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x41f1d0 Player::ApplyMissionSaveData.
 * Purpose: provide the recovered player restore saved weapon side helper for
 * the Player/Pickup gameplay source cluster.
 */
void PlayerRestoreSavedWeaponSide(PlayerGunFireController* controller, const PlayerMissionSaveWeaponSide* savedSide)
{
    controller->flags &= ~kPlayerGunControllerAvailableFlag;
    if ((savedSide->enabled & 1) != 0) {
        controller->flags |= kPlayerGunControllerAvailableFlag;
    }
    controller->ammoOrCharge = savedSide->ammoOrCharge;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x41f1d0 Player::ApplyMissionSaveData.
 * Purpose: provide the recovered player refresh saved weapon bank hud helper for
 * the Player/Pickup gameplay source cluster.
 */
void PlayerRefreshSavedWeaponBankHud(int bankIndex, PlayerAltWeaponBank* bank)
{
    PlayerGunFireController* const selectedController = PlayerSavedWeaponController(bank, bank->selectedSide);

    HudUiMessage::SetValueIfOwnerMatches(bankIndex, bank->selectedSide, selectedController->ammoOrCharge);
    if ((selectedController->flags & kPlayerGunControllerAvailableFlag) != 0) {
        HudUiMessage::SelectVariantDisplay(bankIndex, bank->selectedSide);
    } else {
        HudUiMessage::ClearDisplay(bankIndex);
    }
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x41f1d0 Player::ApplyMissionSaveData.
 * Purpose: provide the recovered player refresh previous weapon controller hud helper for
 * the Player/Pickup gameplay source cluster.
 */
void PlayerRefreshPreviousWeaponControllerHud(PlayerGunFireController* controller)
{
    if ((controller->flags & kPlayerGunControllerAvailableFlag) != 0) {
        HudUiMessage::SelectVariantDisplay(controller->weaponBankIndex, controller->weaponSideIndex);
    } else {
        HudUiMessage::ClearDisplay(controller->weaponBankIndex);
    }
}
} // namespace

namespace zVehicle {

} // namespace zVehicle

namespace Player_TopMsgPanel1 {

} // namespace Player_TopMsgPanel1

namespace Player_TopMsgPanel2 {

} // namespace Player_TopMsgPanel2

namespace PlayerNodeFlagRestore {

} // namespace PlayerNodeFlagRestore

namespace Player {

} // namespace Player

namespace zMath {

} // namespace zMath

namespace Player {

} // namespace Player

namespace Player {

} // namespace Player

#include "GameZRecoil/zCom/zCom.h"

namespace Checkpoint {

} // namespace Checkpoint

namespace Player {
/**
 * Original inline helper; no standalone retail function exists.
 * Observed in address-backed callers 0x424010 PlayerPendingContact::SelectPreferred, 0x4251f0
 * Player::CollectPendingCollisionContactsForQuadProbe, 0x426770 Player::UpdateMasterTypeTrack, 0x428d60
 * Player::ProbeModalSampleHeights. Purpose: transform a point without emitting an out-of-line helper under the retail
 * translation unit's /Ob0 compile mode.
 */
#define PLAYER_TRANSFORM_POINT_BY_MATRIX(result, point, matrix)                                                        \
    do {                                                                                                               \
        (result).x = (point).x * (matrix).xx + (point).y * (matrix).yx + (point).z * (matrix).zx + (matrix).posX;      \
        (result).y = (point).x * (matrix).xy + (point).y * (matrix).yy + (point).z * (matrix).zy + (matrix).posY;      \
        (result).z = (point).x * (matrix).xz + (point).y * (matrix).yz + (point).z * (matrix).zz + (matrix).posZ;      \
    } while (0)

enum { kPlayerMasterTypeSub = 2, kPlayerMaxModalProbePoints = 4, kPlayerEnvProbeBasePointOffset = 15 };

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x405c90 Player::ApplyCameraState.
 * Purpose: provide the recovered set state7 fx pass3 visible helper for
 * the Player/Pickup gameplay source cluster.
 */
void SetState7FxPass3Visible(int visible)
{
    g_Player_State7FxPass3Ui.SetVisible(visible);
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x4236b0 Player::BuildPendingContactQueues.
 * Purpose: provide the recovered build modal and root probe world caches helper for
 * the Player/Pickup gameplay source cluster.
 */
void BuildModalAndRootProbeWorldCaches(
    zUtil_PlayerStateStorage* playerState,
    const PlayerMasterModalData* masterModalData
)
{
    for (int i = 0; i < masterModalData->probePointCount; ++i) {
        PLAYER_TRANSFORM_POINT_BY_MATRIX(
            playerState->modalProbeWorldByIndex[i],
            masterModalData->probePoints[i],
            playerState->motionBasis
        );
        PLAYER_TRANSFORM_POINT_BY_MATRIX(
            playerState->rootProbeWorldByIndex[i],
            masterModalData->probePoints[i],
            playerState->previousTransform
        );
    }
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x424270 Player::ResolvePendingCollisionContact.
 * Purpose: provide the recovered vec3 length helper for
 * the Player/Pickup gameplay source cluster.
 */
float Vec3Length(const zVec3& vec)
{
    return (float)(sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z));
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x424270 Player::ResolvePendingCollisionContact.
 * Purpose: provide the recovered vec3 dot xz helper for
 * the Player/Pickup gameplay source cluster.
 */
float Vec3DotXZ(const zVec3& a, const zVec3& b)
{
    return a.x * b.x + a.z * b.z;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x424270 Player::ResolvePendingCollisionContact.
 * Purpose: provide the recovered vec3 cross helper for
 * the Player/Pickup gameplay source cluster.
 */
zVec3 Vec3Cross(const zVec3& a, const zVec3& b)
{
    zVec3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}

/*
 * Mission-order Player callback bodies compile from mission.cpp; retain the
 * original player.cpp address provenance for focused source guards.
 */

} // namespace Player

namespace PlayerPickupContact {

} // namespace PlayerPickupContact

namespace Player {

/**
 * Original-source static helper; no standalone retail function exists.
 * Observed in caller 0x41fd20 Player::DestroySaveGameState.
 * Evidence: the caller contains the HUD sensor track-list unlink sequence inline.
 * Purpose: Remove a HUD sensor track node from the global mission track list.
 */
#define RemoveTrackNode(trackNode)                                                                                     \
    do {                                                                                                               \
        HudUiMgrSensorTrackNode* const playerRemovedTrackNode = (trackNode);                                           \
        if (g_HudUiMgrSensor_TrackList.count != 0) {                                                                   \
            HudUiMgrSensorTrackNode* playerTrackCursor = g_HudUiMgrSensor_TrackList.head;                              \
            if (playerRemovedTrackNode == playerTrackCursor) {                                                         \
                --g_HudUiMgrSensor_TrackList.count;                                                                    \
                g_HudUiMgrSensor_TrackList.head = playerRemovedTrackNode->next;                                        \
                if (g_HudUiMgrSensor_TrackList.head == 0) {                                                            \
                    g_HudUiMgrSensor_TrackList.trackListAux = 0;                                                       \
                    g_HudUiMgrSensor_TrackList.tail = 0;                                                               \
                }                                                                                                      \
            } else {                                                                                                   \
                while (playerTrackCursor != 0) {                                                                       \
                    HudUiMgrSensorTrackNode* const playerTrackNext = playerTrackCursor->next;                          \
                    if (playerTrackNext == playerRemovedTrackNode) {                                                   \
                        --g_HudUiMgrSensor_TrackList.count;                                                            \
                        playerTrackCursor->next = playerRemovedTrackNode->next;                                        \
                        if (g_HudUiMgrSensor_TrackList.tail == playerRemovedTrackNode) {                               \
                            g_HudUiMgrSensor_TrackList.tail = playerTrackCursor;                                       \
                        }                                                                                              \
                        break;                                                                                         \
                    }                                                                                                  \
                    playerTrackCursor = playerTrackNext;                                                               \
                }                                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

/**
 * Original inline helper; no standalone retail function exists.
 * Observed expanded inline at 0x41fd20 Player::DestroySaveGameState and
 * 0x41fb80 Player::ShutdownMissionRuntime as real list-draining loops whose
 * internal cases reconverge before shared cleanup.
 * Purpose: shape reference for save-state unlink loops; kept textually where
 * callers expand it.
 */

/**
 * Original-source static helper; no standalone retail function exists.
 * Observed in caller 0x41fb80 Player::ShutdownMissionRuntime.
 * Evidence: the caller contains the remaining HUD sensor track-node deletion loop inline.
 * Purpose: Delete leftover HUD sensor track nodes and clear the global track list.
 */
#define DeleteRemainingTrackNodes()                                                                                    \
    do {                                                                                                               \
        HudUiMgrSensorTrackNode* playerTrackNode = g_HudUiMgrSensor_TrackList.head;                                    \
        while (playerTrackNode != 0) {                                                                                 \
            HudUiMgrSensorTrackNode* const playerTrackNext = playerTrackNode->next;                                    \
            ::operator delete(playerTrackNode);                                                                        \
            playerTrackNode = playerTrackNext;                                                                         \
        }                                                                                                              \
        memset(&g_HudUiMgrSensor_TrackList, 0, sizeof(g_HudUiMgrSensor_TrackList));                                    \
    } while (0)

/**
 * Original-source static helper; no standalone retail function exists.
 * Observed in caller 0x41fb80 Player::ShutdownMissionRuntime.
 * Evidence: the caller contains the weapon-spec deletion and list-clear sequence inline.
 * Purpose: Delete all weapon specs owned by one PlayerMasterCommonData record.
 */
#define DeleteWeaponSpecs(commonData)                                                                                  \
    do {                                                                                                               \
        PlayerMasterCommonData* const playerCommonData = (commonData);                                                 \
        PlayerMasterWeaponSpec* playerWeaponSpec = playerCommonData->weaponSpecHead;                                   \
        while (playerWeaponSpec != 0) {                                                                                \
            PlayerMasterWeaponSpec* const playerWeaponSpecNext = playerWeaponSpec->next;                               \
            ::operator delete(playerWeaponSpec);                                                                       \
            playerWeaponSpec = playerWeaponSpecNext;                                                                   \
        }                                                                                                              \
        playerCommonData->weaponSpecListAux = 0;                                                                       \
        playerCommonData->weaponSpecTail = 0;                                                                          \
        playerCommonData->weaponSpecHead = 0;                                                                          \
        playerCommonData->weaponSpecCount = 0;                                                                         \
    } while (0)

} // namespace Player

/* Governed authored-order insertion point: keep selected retail bodies below. */
/**
 * @recoil-anchor recoil:anchor:battlesport-player-common-data-list-global
 * @recoil-artifact defines .data recoil:data:0x4f3a68: Common player-data list.
 * @recoil-artifact emits .text recoil:function:0x41ea90: Native global lifecycle contribution 1.
 * @recoil-artifact emits .data recoil:logical-data:0x4f3a68:aux: List auxiliary state field.
 * @recoil-artifact emits .data recoil:logical-data:0x4f3a6c:head: List head field.
 * @recoil-artifact emits .data recoil:logical-data:0x4f3a70:tail: List tail field.
 * @recoil-artifact emits .data recoil:logical-data:0x4f3a74:count: List count field.
 * Purpose: Own the common-data list and initialize its empty state at startup.
 * Aggregate field identities are under live review.
 */
CPlayerMasterCommonDataList g_PlayerMasterCommonDataList;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-modal-data-list-global
 * @recoil-artifact defines .data recoil:data:0x4f3688: Modal player-data list.
 * @recoil-artifact emits .text recoil:function:0x41eac0: Native global lifecycle contribution 1.
 * @recoil-artifact emits .data recoil:logical-data:0x4f3688:aux: List auxiliary state field.
 * @recoil-artifact emits .data recoil:logical-data:0x4f368c:head: List head field.
 * @recoil-artifact emits .data recoil:logical-data:0x4f3690:tail: List tail field.
 * @recoil-artifact emits .data recoil:logical-data:0x4f3694:count: List count field.
 * Purpose: Own the modal-data list and establish its empty startup state.
 * Aggregate field identities are under live review.
 */
CPlayerMasterModalDataList g_PlayerMasterModalDataList;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-underwater-pass3-global
 * @recoil-artifact defines .data recoil:data:0x4f3778: Underwater pass-3 overlay.
 * @recoil-artifact emits .text recoil:function:0x41eaf0: Native global lifecycle contribution 1.
 * @recoil-artifact emits .text recoil:function:0x41eb00: Native global lifecycle contribution 2.
 * @recoil-artifact emits .text recoil:function:0x41eb10: Native global lifecycle contribution 3.
 * @recoil-artifact emits .text recoil:function:0x41eb20: Native global lifecycle contribution 4.
 * Purpose: Own the underwater HUD effect for the process lifetime, with native
 * C++ initialization and destruction. CRT emission is under live investigation.
 */
CPlayerUnderwaterFxPass3Ui g_Player_UnderwaterFxPass3Ui;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-underwaterfxpass3ui-constructor
 * @recoil-artifact defines .text recoil:function:0x41eb30: CPlayerUnderwaterFxPass3Ui::CPlayerUnderwaterFxPass3Ui.
 * @recoil-match byte
 *
 * Purpose: Construct the underwater pass-3 HUD overlay.
 */
CPlayerUnderwaterFxPass3Ui::CPlayerUnderwaterFxPass3Ui()
    : zVideoFxPass3Element(0, 0)
{
}
/**
 * @recoil-anchor recoil:anchor:battlesport-player-projectile-pass3-global
 * @recoil-artifact defines .data recoil:data:0x4f3650: Projectile-camera pass-3 overlay.
 * @recoil-artifact emits .text recoil:function:0x41eb50: Native global lifecycle contribution 1.
 * @recoil-artifact emits .text recoil:function:0x41eb60: Native global lifecycle contribution 2.
 * @recoil-artifact emits .text recoil:function:0x41eb70: Native global lifecycle contribution 3.
 * @recoil-artifact emits .text recoil:function:0x41eb80: Native global lifecycle contribution 4.
 * Purpose: Own the projectile-camera overlay for the process lifetime.
 */
CPlayerProjectileCameraFxPass3Ui g_Player_State7FxPass3Ui;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-projectilecamerafxpass3ui-constructor
 * @recoil-artifact defines .text recoil:function:0x41eb90: CPlayerProjectileCameraFxPass3Ui::CPlayerProjectileCameraFxPass3Ui.
 * @recoil-match byte
 *
 * Purpose: Construct the projectile-camera pass-3 HUD overlay.
 */
CPlayerProjectileCameraFxPass3Ui::CPlayerProjectileCameraFxPass3Ui()
    : zVideoFxPass3Element(0, 0)
{
}
/**
 * @recoil-anchor recoil:anchor:battlesport-player-sensor-track-list-global
 * @recoil-artifact defines .data recoil:data:0x4f3340: Sensor track list.
 * @recoil-artifact emits .text recoil:function:0x41ebd0: Native global lifecycle contribution 1.
 * Purpose: Own the initially empty sensor tracking list. Native startup
 * emission and Player translation-unit placement are under live verification.
 */
HudUiMgrSensorTrackList g_HudUiMgrSensor_TrackList;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-save-state-list-global
 * @recoil-artifact defines .data recoil:data:0x4f3a78: Player save-state list.
 * @recoil-artifact emits .text recoil:function:0x41ec00: Native global lifecycle contribution 1.
 * @recoil-artifact emits .data recoil:logical-data:0x4f3a78:aux: List auxiliary state field.
 * @recoil-artifact emits .data recoil:logical-data:0x4f3a7c:head: List head field.
 * @recoil-artifact emits .data recoil:logical-data:0x4f3a80:tail: List tail field.
 * @recoil-artifact emits .data recoil:logical-data:0x4f3a84:count: List count field.
 * Purpose: Own the list of live save-state records with an empty startup state.
 * Explicit teardown retains responsibility for deleting the linked records.
 */
CPlayerSaveStateList g_PlayerSaveStateList;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-top-message-panel1-global
 * @recoil-artifact defines .data recoil:data:0x4f37b0: First top-message panel.
 * @recoil-artifact emits .text recoil:function:0x41ec30: Native global lifecycle contribution 1.
 * @recoil-artifact emits .text recoil:function:0x41ec40: Native global lifecycle contribution 2.
 * @recoil-artifact emits .text recoil:function:0x41ec60: Native global lifecycle contribution 3.
 * @recoil-artifact emits .text recoil:function:0x41ec70: Native global lifecycle contribution 4.
 * Purpose: Own the first top-message HUD panel for the process lifetime.
 */
HudUiPanel g_Player_TopMsgPanel1;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-top-message-panel2-global
 * @recoil-artifact defines .data recoil:data:0x4f33a8: Second top-message panel.
 * @recoil-artifact emits .text recoil:function:0x41ec80: Native global lifecycle contribution 1.
 * @recoil-artifact emits .text recoil:function:0x41ec90: Native global lifecycle contribution 2.
 * @recoil-artifact emits .text recoil:function:0x41ecb0: Native global lifecycle contribution 3.
 * @recoil-artifact emits .text recoil:function:0x41ecc0: Native global lifecycle contribution 4.
 * Purpose: Own the second top-message HUD panel for the process lifetime.
 */
HudUiPanel g_Player_TopMsgPanel2;
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-recordnodeflagsforrestore
 * @recoil-artifact defines .text recoil:function:0x41ecd0: Player::RecordNodeFlagsForRestore.
 * @recoil-match byte
 *
 * Purpose: Save the node pickability flags for later restoration.
 */
void __fastcall RecordNodeFlagsForRestore(CZNodePartial* node)
{
    PlayerNodeFlagRestoreEntry value;
    value.node = node;
    CZClass::gwNodeGetCellPickable(node, &value.wasCellPickable);
    CZClass::gwNodeGetRaycastable(node, &value.wasRaycastable);
    CZClass::gwNodeGetPickable(node, &value.wasPickable);
    g_PlayerNodeFlagRestoreEntries.push_back(value);
}
} // namespace Player
/**
 * @recoil-anchor recoil:anchor:battlesport-player-node-flag-restore-vector
 * @recoil-artifact defines .data recoil:data:0x4f3a58: g_PlayerNodeFlagRestoreEntries.
 *
 * Purpose: Retain node flag snapshots until their original values are restored.
 */
PlayerNodeFlagRestoreEntryVector g_PlayerNodeFlagRestoreEntries;
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-restorerecordednodeflags
 * @recoil-artifact defines .text recoil:function:0x41efa0: Player::RestoreRecordedNodeFlags.
 * @recoil-match byte
 *
 * Purpose: Restore the saved node pickability flags from each copied record.
 */
void __cdecl RestoreRecordedNodeFlags()
{
    PlayerNodeFlagRestoreEntryVector::iterator entry = g_PlayerNodeFlagRestoreEntries.begin();
    while (entry != g_PlayerNodeFlagRestoreEntries.end()) {
        const PlayerNodeFlagRestoreEntry value = *entry;
        CZNodePartial* const node = value.node;
        if (value.wasCellPickable != 0) {
            CZClass::gwNodeSetCellPickable(node, 1);
        }
        if (value.wasRaycastable != 0) {
            CZClass::gwNodeSetRaycastable(node, 1);
        }
        if (value.wasPickable != 0) {
            CZClass::gwNodeSetPickable(node, 1);
        }
        ++entry;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-buildmissionsavedata
 * @recoil-artifact defines .text recoil:function:0x41f010: Player::BuildMissionSaveData
 * @recoil-match byte
 *
 * Purpose: copy the live local-player mission state into the save-section payload.
 */
void __fastcall BuildMissionSaveData(PlayerMissionSaveData* outData)
{
    zUtil_SaveGameState* const localSaveState = g_LocalPlayerSaveState;
    zUtil_PlayerStateStorage* const playerState = localSaveState->playerState;
    PlayerMasterModalData* const masterModalData = localSaveState->primaryModalState->masterModalData;

    outData->size = sizeof(PlayerMissionSaveData);
    {
        for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
            outData->weaponBank[bankIndex].selectedSide = playerState->altWeaponBanks[bankIndex].selectedSide;
            {
                for (int sideIndex = 0; sideIndex < 2; ++sideIndex) {
                    const PlayerGunFireController* const controller
                        = &playerState->altWeaponBanks[bankIndex].controllerA + sideIndex;
                    outData->weaponBank[bankIndex].sides[sideIndex].enabled
                        = ((unsigned int)controller->flags >> 2) & 1;
                    outData->weaponBank[bankIndex].sides[sideIndex].ammoOrCharge = controller->ammoOrCharge;
                }
            }
        }
    }

    outData->altWeaponBankIndex = playerState->activeAltGunController->weaponBankIndex;
    outData->altWeaponSideIndex = playerState->activeAltGunController->weaponSideIndex;
    outData->primaryWeaponBankIndex = playerState->activePrimaryGunController->weaponBankIndex;
    outData->primaryWeaponSideIndex = playerState->activePrimaryGunController->weaponSideIndex;
    outData->playerStatusMeterRatio = g_PlayerStatusMeterRatio;
    outData->hudCounterValue = g_Player_HudCounterValue;
    outData->amphibUnlocked = playerState->amphibUnlocked;
    outData->hoverUnlocked = playerState->hoverUnlocked;
    outData->subUnlocked = playerState->subUnlocked;
    // Retail repeats the three unlock-flag stores (retail 0x41f0ce-0x41f103); keep the duplicate assignments.
    outData->amphibUnlocked = playerState->amphibUnlocked;
    outData->hoverUnlocked = playerState->hoverUnlocked;
    outData->subUnlocked = playerState->subUnlocked;
    outData->aiMode = playerState->aiMode;
    outData->nextModeSwitchAllowedTime = playerState->nextModeSwitchAllowedTime;
    outData->motionInput = playerState->motionInput;
    outData->autoTurnSign = playerState->autoTurnSign;
    outData->bankInput = playerState->bankInput;
    outData->playerMasterType = masterModalData->masterType;

    CZCamera::gwCameraGetTarget(
        g_MainCamera,
        &outData->cameraTarget.x,
        &outData->cameraTarget.y,
        &outData->cameraTarget.z
    );
    CZCamera::gwCameraGetPosition(
        g_MainCamera,
        &outData->cameraPosition.x,
        &outData->cameraPosition.y,
        &outData->cameraPosition.z
    );

    memcpy(&outData->timedHitStatus, &playerState->timedHitStatus, sizeof(outData->timedHitStatus));

    if ((playerState->timedHitStatus.runtimeFlags & 1) != 0) {
        outData->timedHitStatus.nextUpdateTime -= g_Time_AccumulatedTimeSec;
        outData->timedHitStatus.lightNode = 0;
        outData->timedHitStatus.savedHitSourceEntryId = playerState->timedHitStatus.hitSource->ordinalIndex;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-applymissionsavedata
 * @recoil-artifact defines .text recoil:function:0x41f1d0: Player::ApplyMissionSaveData
 *
 *
 * Purpose: restore the live local-player mission state from the save-section payload.
 */
void __fastcall ApplyMissionSaveData(PlayerMissionSaveData* saveData)
{
    zUtil_PlayerStateStorage* const playerState = g_LocalPlayerSaveState->playerState;
    int hasTimedHitStatus = 1;
    if (saveData->size != sizeof(PlayerMissionSaveData)) {
        hasTimedHitStatus = 0;
        if (saveData->size != kPlayerMissionSaveLegacySize) {
            zError::ReportOld(0x200, g_Player_SourceFile_PlayerCpp, 0xd1, g_Player_SaveDataModifiedMsg);
            return;
        }
    }

    PlayerGunFireController* const oldAltController = playerState->activeAltGunController;
    PlayerGunFireController* const oldPrimaryController = playerState->activePrimaryGunController;

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        playerState->altWeaponBanks[bankIndex].selectedSide = saveData->weaponBank[bankIndex].selectedSide;
        for (int sideIndex = 0; sideIndex < 2; ++sideIndex) {
            PlayerGunFireController* const controller = &playerState->altWeaponBanks[bankIndex].controllerA + sideIndex;
            controller->flags = (controller->flags & ~kPlayerGunControllerAvailableFlag)
                | ((saveData->weaponBank[bankIndex].sides[sideIndex].enabled & 1) << 2);
            controller->ammoOrCharge = saveData->weaponBank[bankIndex].sides[sideIndex].ammoOrCharge;
        }

        HudUiMessage::SetValueIfOwnerMatches(
            bankIndex,
            playerState->altWeaponBanks[bankIndex].selectedSide,
            (&playerState->altWeaponBanks[bankIndex].controllerA + playerState->altWeaponBanks[bankIndex].selectedSide)
                ->ammoOrCharge
        );
        if ((unsigned char)((unsigned int)(&playerState->altWeaponBanks[bankIndex].controllerA
                                + playerState->altWeaponBanks[bankIndex].selectedSide)
                                ->flags
                >> 2)
            & 1) {
            HudUiMessage::SelectVariantDisplay(bankIndex, playerState->altWeaponBanks[bankIndex].selectedSide);
        } else {
            HudUiMessage::ClearDisplay(bankIndex);
        }
    }

    PlayerGunFireController* const newAltController
        = &playerState->altWeaponBanks[saveData->altWeaponBankIndex].controllerA + saveData->altWeaponSideIndex;
    playerState->activeAltGunController = newAltController;
    if (newAltController != oldAltController) {
        ApplyAltWeaponSwitch(g_LocalPlayerSaveState, oldAltController, newAltController);
        if ((unsigned char)((unsigned int)(&playerState->altWeaponBanks[oldAltController->weaponBankIndex].controllerA
                                + oldAltController->weaponSideIndex)
                                ->flags
                >> 2)
            & 1) {
            HudUiMessage::SelectVariantDisplay(oldAltController->weaponBankIndex, oldAltController->weaponSideIndex);
        } else {
            HudUiMessage::ClearDisplay(oldAltController->weaponBankIndex);
        }
    } else {
        ApplyAltWeaponSwitch(g_LocalPlayerSaveState, 0, newAltController);
    }
    HudUiMessage::UpdateSelectedWeaponDisplay(
        playerState->activeAltGunController->weaponBankIndex,
        playerState->activeAltGunController->weaponSideIndex,
        playerState->activeAltGunController->ammoOrCharge
    );

    PlayerGunFireController* const newPrimaryController
        = &playerState->altWeaponBanks[saveData->primaryWeaponBankIndex].controllerA + saveData->primaryWeaponSideIndex;
    playerState->activePrimaryGunController = newPrimaryController;
    if (newPrimaryController != oldPrimaryController) {
        ApplyPrimaryWeaponSwitch(g_LocalPlayerSaveState, oldPrimaryController, newPrimaryController);
        if ((unsigned char)((unsigned int)(&playerState->altWeaponBanks[oldPrimaryController->weaponBankIndex]
                                               .controllerA
                                + oldPrimaryController->weaponSideIndex)
                                ->flags
                >> 2)
            & 1) {
            HudUiMessage::SelectVariantDisplay(
                oldPrimaryController->weaponBankIndex,
                oldPrimaryController->weaponSideIndex
            );
        } else {
            HudUiMessage::ClearDisplay(oldPrimaryController->weaponBankIndex);
        }
    }
    HudUiMessage::UpdateSelectedWeaponDisplay(
        playerState->activePrimaryGunController->weaponBankIndex,
        playerState->activePrimaryGunController->weaponSideIndex,
        playerState->activePrimaryGunController->ammoOrCharge
    );

    HudUiMgrSensor::SetShieldMessageRatio(
        playerState->statusMeterValue / g_LocalPlayerSaveState->playerState->masterCommonData->maxHealth
    );
    HudUiMgr::SetNanitePanelCount(playerState->nanitePanelLevel);

    g_PlayerStatusMeterRatio = saveData->playerStatusMeterRatio;
    g_Player_HudCounterValue = saveData->hudCounterValue;
    playerState->amphibUnlocked = saveData->amphibUnlocked;
    playerState->hoverUnlocked = saveData->hoverUnlocked;
    playerState->subUnlocked = saveData->subUnlocked;
    playerState->aiMode = saveData->aiMode;
    playerState->nextModeSwitchAllowedTime = saveData->nextModeSwitchAllowedTime;
    playerState->motionInput = saveData->motionInput;
    playerState->autoTurnSign = saveData->autoTurnSign;
    playerState->bankInput = saveData->bankInput;

    HudUiMgrObjective::RefreshCounterText(g_Player_HudCounterValue);
    ApplyMasterTypeTransition(g_LocalPlayerSaveState, saveData->playerMasterType, 1);
    playerState->primaryGunGateUntilTime = 0.0f;

    CZCamera::gwCameraSetTarget(
        g_MainCamera,
        saveData->cameraTarget.x,
        saveData->cameraTarget.y,
        saveData->cameraTarget.z
    );
    CZCamera::gwCameraSetPosition(
        g_MainCamera,
        saveData->cameraPosition.x,
        saveData->cameraPosition.y,
        saveData->cameraPosition.z
    );

    ((zUtil_SaveGameState*)g_GameStateOrMapTable)->playerState->timedHitStatus.ClearLightAndReset();
    playerState->damageProtectionActive = 0;
    if (hasTimedHitStatus != 0) {
        memcpy(&playerState->timedHitStatus, &saveData->timedHitStatus, sizeof(saveData->timedHitStatus));
        playerState->timedHitStatus.lightParentNode = playerState->rootNode;

        if ((playerState->timedHitStatus.runtimeFlags & kPlayerTimedHitStatusActiveFlag) != 0) {
            // The copied save record holds the hit-source entry id in the runtime hitSource slot.
            OptCatalogEntryDef* const hitSource = OptCatalog::FindEntryById((int)playerState->timedHitStatus.hitSource);
            playerState->timedHitStatus.hitSource = hitSource;
            HitSource::UpdateTimedStatus(hitSource, &playerState->timedHitStatus, 0.0f);
            playerState->timedHitStatus.nextUpdateTime
                = g_Time_AccumulatedTimeSec + playerState->timedHitStatus.nextUpdateTime;
        }
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-zar-registersections
 * @recoil-artifact defines .text recoil:function:0x41f5b0: Player::zZarRegisterSections
 * @recoil-match byte
 *
 * BN evidence: resets g_Player_RuntimeInputFlags and registers VehicleList and
 * Player callbacks through zUtil_ZAR::RegisterSectionHandler with sort orders 100
 * and 200.
 * Purpose: install Player-owned ZAR section callbacks for save/load.
 */
void __fastcall zZarRegisterSections()
{
    g_Player_RuntimeInputFlags = 0;
    zUtil_ZAR::RegisterSectionHandler(
        g_Player_SaveVehicleListSectionName,
        (zZbdSectionCallback)(&zZarWriteVehicleListSection),
        (zZbdSectionCallback)(&zZarReadVehicleListSection),
        100,
        0
    );
    zUtil_ZAR::RegisterSectionHandler(
        g_HudUiCounterText_PlayerLabel,
        (zZbdSectionCallback)(&zZarWriteMissionSaveDataSection),
        (zZbdSectionCallback)(&zZarReadMissionSaveDataSection),
        200,
        0
    );
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-zar-writemissionsavedatasection
 * @recoil-artifact defines .text recoil:function:0x41f5f0: Player::zZarWriteMissionSaveDataSection
 * @recoil-match byte
 *
 * BN evidence: __fastcall ZAR pre-load callback; builds PlayerMissionSaveData,
 * copies g_Player_LastValidCameraVariantTag as one packed zTag4 value, and writes
 * a 0x140-byte blob under the local player's root-node name.
 * Purpose: serialize local-player mission state into the Player ZAR section.
 */
int __fastcall zZarWriteMissionSaveDataSection(zZbdSectionCallbackCtx* writer, void*)
{
    PlayerMissionSaveData missionData;
    zUtil_PlayerStateStorage* const playerState = g_LocalPlayerSaveState->playerState;

    BuildMissionSaveData(&missionData);
    missionData.lastValidCameraVariantTag = g_Player_LastValidCameraVariantTag;
    return zUtil_ZAR::WriteSectionBlob(writer, playerState->rootNode->name, &missionData, sizeof(missionData));
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-zar-readmissionsavedatasection
 * @recoil-artifact defines .text recoil:function:0x41f640: Player::zZarReadMissionSaveDataSection
 * @recoil-match byte
 *
 * BN evidence: __fastcall ZAR data-ready callback; applies PlayerMissionSaveData,
 * copies lastValidCameraVariantTag to g_Player_LastValidCameraVariantTag, refreshes
 * HUD/layout state, and restores recorded node flags.
 * Purpose: restore local-player mission state from the Player ZAR section.
 */
void __fastcall zZarReadMissionSaveDataSection(
    zZbdSectionCallbackCtx*,
    const char*,
    PlayerMissionSaveData* saveData,
    unsigned int,
    void*
)
{
    zUtil_PlayerStateStorage* const playerState = g_LocalPlayerSaveState->playerState;

    ApplyMissionSaveData(saveData);
    g_Player_LastValidCameraVariantTag = saveData->lastValidCameraVariantTag;

    if (playerState->lifecycleState == kPlayerLifecycleInactive) {
        zEffect_Anim::NodeActionCallback(playerState->destroyedRespawnFxEntry, playerState->rootNode);
    }

    RefreshHudFromState((zUtil_SaveGameState*)(g_GameStateOrMapTable));
    HudUiMgr::TriggerCurrentLayoutOnActivated();
    RestoreRecordedNodeFlags();
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-zar-writevehiclelistsection
 * @recoil-artifact defines .text recoil:function:0x41f6a0: Player::zZarWriteVehicleListSection
 * @recoil-match byte
 *
 * BN evidence: __fastcall ZAR pre-load callback; walks g_PlayerSaveStateList.head,
 * fills the 0x80-byte PlayerVehicleListSaveEntry from typed player-state fields,
 * and writes each blob under the player's root-node name.
 * Purpose: serialize all active player vehicle records into the VehicleList ZAR section.
 */
int __fastcall zZarWriteVehicleListSection(zZbdSectionCallbackCtx* writer, void*)
{
    int writeOk = 1;
    zUtil_SaveGameState* saveState = g_PlayerSaveStateList.head;
    while (saveState != 0 && writeOk != 0) {
        zUtil_PlayerStateStorage* const playerState = saveState->playerState;
        PlayerVehicleListSaveEntry vehicleRecord;
        vehicleRecord.size = 128;
        vehicleRecord.worldPos = playerState->worldPos;
        vehicleRecord.vehicleRotationAngles = playerState->vehicleRotationAngles;
        vehicleRecord.aiNetId = playerState->aiNetId;
        vehicleRecord.aiTopLevelState = playerState->aiTopLevelState;
        vehicleRecord.aiSavedTopLevelState = playerState->aiSavedTopLevelState;
        vehicleRecord.aiReturnTopLevelState = playerState->aiReturnTopLevelState;
        vehicleRecord.aiAttackRadiusSq = playerState->aiAttackRadiusSq;
        vehicleRecord.aiRestoreDistanceSq = playerState->aiRestoreDistanceSq;
        vehicleRecord.aiRestoreTarget = playerState->aiRestoreTarget;
        vehicleRecord.aiDynamicOffsetDir = playerState->aiDynamicOffsetDir;
        vehicleRecord.aiActivationRadiusSq = playerState->aiActivationRadiusSq;
        vehicleRecord.aiTickSuppressed = playerState->aiTickSuppressed;
        vehicleRecord.aiAlertFlag = playerState->recentHitFlag;
        vehicleRecord.aiStateMarkerHandle = playerState->recentHitMarkerHandle;
        vehicleRecord.aiActive = playerState->aiActive;
        vehicleRecord.aiPathCursorAdvanceRequested = playerState->aiPathCursorAdvanceRequested;
        vehicleRecord.aiCurrentSteeringSubstate = playerState->aiCurrentSteeringSubstate;
        vehicleRecord.aiReturnSteeringSubstate = playerState->aiReturnSteeringSubstate;
        vehicleRecord.masterType = playerState->masterType;
        vehicleRecord.statusMeterScaled = playerState->statusMeterScaled;
        vehicleRecord.statusMeterValue = playerState->statusMeterValue;
        vehicleRecord.nanitePanelLevel = playerState->nanitePanelLevel;
        if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
            vehicleRecord.localMasterType = saveState->primaryModalState->masterModalData->masterType;
        }

        writeOk
            = zUtil_ZAR::WriteSectionBlob(writer, playerState->rootNode->name, &vehicleRecord, sizeof(vehicleRecord));
        saveState = saveState != 0 ? saveState->next : 0;
    }

    return writeOk;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-zar-readvehiclelistsection
 * @recoil-artifact defines .text recoil:function:0x41f850: Player::zZarReadVehicleListSection
 * @recoil-match byte
 *
 * BN evidence: __fastcall ZAR data-ready callback; validates the 0x80-byte
 * VehicleList record, finds the save state by root-node token, restores pose,
 * AI, status, visual, and lifecycle fields, and refreshes node state.
 * Purpose: restore one player vehicle record from the VehicleList ZAR section.
 */
void __fastcall zZarReadVehicleListSection(
    zZbdSectionCallbackCtx*,
    const char* sectionToken,
    PlayerVehicleListSaveEntry* saveData,
    unsigned int,
    void*
)
{
    if (saveData->size != 128) {
        zError::ReportOld(0x200, g_Player_SourceFile_PlayerCpp, 419, g_Player_VehicleSaveDataModifiedMsg);
        return;
    }

    zUtil_SaveGameState* saveState = g_PlayerSaveStateList.head;
    zUtil_PlayerStateStorage* playerState;
    for (;;) {
        if (saveState == 0) {
            return;
        }

        playerState = saveState->playerState;
        if (strcmp(playerState->rootNode->name, sectionToken) == 0) {
            break;
        }
        saveState = saveState != 0 ? saveState->next : 0;
    }

    const bool restoreHealthyNode
        = playerState->lifecycleState == kPlayerLifecycleInactive && saveData->masterType != kPlayerLifecycleInactive;

    playerState->projectileSpawnVel.x = playerState->projectileSpawnVel.y = playerState->projectileSpawnVel.z = 0.0f;
    playerState->localVel.x = playerState->localVel.y = playerState->localVel.z = 0.0f;
    playerState->yawRotatedLocalVel.x = playerState->yawRotatedLocalVel.y = playerState->yawRotatedLocalVel.z = 0.0f;
    playerState->worldPos = saveData->worldPos;
    playerState->vehicleRotationAngles = saveData->vehicleRotationAngles;
    playerState->aiNetId = saveData->aiNetId;
    playerState->aiTopLevelState = saveData->aiTopLevelState;
    playerState->aiSavedTopLevelState = saveData->aiSavedTopLevelState;
    playerState->aiReturnTopLevelState = saveData->aiReturnTopLevelState;

    playerState->aiStateUntilTime = g_Time_AccumulatedTimeSec;
    playerState->aiHideTime0 = g_Time_AccumulatedTimeSec;
    playerState->aiHideTime1 = g_Time_AccumulatedTimeSec;
    playerState->unknown_0fa4 = g_Time_AccumulatedTimeSec;
    playerState->aiStateStartTime = g_Time_AccumulatedTimeSec;
    playerState->aiStateEndTime = playerState->aiMode2AttackDwell + g_Time_AccumulatedTimeSec;

    playerState->aiAttackRadiusSq = saveData->aiAttackRadiusSq;
    playerState->aiRestoreDistanceSq = saveData->aiRestoreDistanceSq;
    playerState->aiRestoreTarget = saveData->aiRestoreTarget;
    playerState->aiDynamicOffsetDir = saveData->aiDynamicOffsetDir;
    playerState->unknown_0fd0 = g_Time_AccumulatedTimeSec;
    playerState->aiActivationRadiusSq = saveData->aiActivationRadiusSq;
    playerState->aiTickSuppressed = saveData->aiTickSuppressed;
    playerState->recentHitFlag = saveData->aiAlertFlag;
    playerState->recentHitMarkerHandle = saveData->aiStateMarkerHandle;
    playerState->aiActive = saveData->aiActive;
    playerState->aiPathCursorAdvanceRequested = saveData->aiPathCursorAdvanceRequested;
    playerState->aiCurrentSteeringSubstate = saveData->aiCurrentSteeringSubstate;
    playerState->aiReturnSteeringSubstate = saveData->aiReturnSteeringSubstate;
    playerState->lifecycleState = saveData->masterType;
    playerState->statusMeterScaled = saveData->statusMeterScaled;
    playerState->statusMeterValue = saveData->statusMeterValue;
    playerState->nanitePanelLevel = saveData->nanitePanelLevel;

    SetWorldPoseAndRestartAnchor(saveState, &playerState->worldPos, playerState->restartYawRad);

    if (saveState != (zUtil_SaveGameState*)g_GameStateOrMapTable) {
        TickMasterTypeAndForceFeedback(saveState);
    }

    AINet::AiDiscardNegativeBranchPathNodes(saveState);
    playerState->aiCurrentPathNode = (AINetNode*)playerState->aiUnknown_0f7c;
    playerState->aiCurrentPathNeighborIndex = 0;

    if (saveState != (zUtil_SaveGameState*)g_GameStateOrMapTable && restoreHealthyNode != 0) {
        CZNodePartial* const healthyNode
            = CZClass::FindNodeRecursiveByName(playerState->rootNode, g_Player_HealthySubNodeName);
        if (healthyNode != 0) {
            CZObject3D::gwObject3DSetPosition(healthyNode, 0.0f, 0.0f, 0.0f);
            CZObject3D::gwObject3DSetRotation(healthyNode, 0.0f, 0.0f, 0.0f);
        }

        if (playerState->destroyedRespawnAsyncHandle != 0) {
            zEffect_Anim::NodeActionCallback(playerState->destroyedRespawnAsyncHandle, 0);
        } else {
            zEffect_Anim::NodeActionCallback(playerState->destroyedRespawnFxEntry, playerState->rootNode);
        }
    }

    if (playerState->lifecycleState == kPlayerLifecycleInactive) {
        CZClass::gwNodeSetActive(playerState->rootNode, 0);
    } else {
        CZClass::gwNodeSetActive(playerState->rootNode, 1);
    }
    CZNode::LoadFlagBit8MaterialImagesAndTexturePack(playerState->rootNode);
    zTag4::Clear(&playerState->variantTag);
    CZClass::gwNodeSetNodeType(playerState->rootNode, playerState->variantTag.tags[0]);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-shutdownmissionruntime
 * @recoil-artifact defines .text recoil:function:0x41fb80: Player::ShutdownMissionRuntime
 * @recoil-match byte
 *
 * Source file: D:\Proj\Battlesport\player.cpp.
 * Purpose: Clear mission-owned player runtime lists, AI net state, and pass-3 UI links.
 */
void __fastcall ShutdownMissionRuntime()
{
    while (1) {
        zUtil_SaveGameState* const saveStateHead = g_PlayerSaveStateList.head;
        if (saveStateHead == 0) {
            break;
        }
        DestroySaveGameState(saveStateHead);
    }

    HudUiMgrSensorTrackNode* trackNode = g_HudUiMgrSensor_TrackList.head;
    while (trackNode != 0) {
        HudUiMgrSensorTrackNode* const next = trackNode != 0 ? trackNode->next : 0;
        ::operator delete(trackNode);
        trackNode = next;
    }

    g_HudUiMgrSensor_TrackList.trackListAux = 0;
    g_HudUiMgrSensor_TrackList.tail = 0;
    g_HudUiMgrSensor_TrackList.head = 0;
    g_HudUiMgrSensor_TrackList.count = 0;

    zUtil_SaveGameState* saveState = g_PlayerSaveStateList.head;
    while (saveState != 0) {
        zUtil_SaveGameState* const next = saveState != 0 ? saveState->next : 0;
        if (saveState != 0) {
            saveState->FreeOwnedResources();
            ::operator delete(saveState);
        }
        saveState = next;
    }

    g_PlayerSaveStateList.listAux = 0;
    g_PlayerSaveStateList.tail = 0;
    g_PlayerSaveStateList.head = 0;
    g_PlayerSaveStateList.count = 0;

    PlayerMasterCommonData* commonData;
    for (commonData = g_PlayerMasterCommonDataList.head; commonData != 0;
        commonData = commonData != 0 ? commonData->next : 0) {
        PlayerMasterWeaponSpec* weaponSpec = commonData->weaponSpecHead;
        while (weaponSpec != 0) {
            PlayerMasterWeaponSpec* const next = weaponSpec != 0 ? weaponSpec->next : 0;
            ::operator delete(weaponSpec);
            weaponSpec = next;
        }

        commonData->weaponSpecListAux = 0;
        commonData->weaponSpecTail = 0;
        commonData->weaponSpecHead = 0;
        commonData->weaponSpecCount = 0;
    }

    commonData = g_PlayerMasterCommonDataList.head;
    while (commonData != 0) {
        PlayerMasterCommonData* const next = commonData != 0 ? commonData->next : 0;
        ::operator delete(commonData);
        commonData = next;
    }

    g_PlayerMasterCommonDataList.listAux = 0;
    g_PlayerMasterCommonDataList.tail = 0;
    g_PlayerMasterCommonDataList.head = 0;
    g_PlayerMasterCommonDataList.count = 0;

    PlayerMasterModalData* modalData = g_PlayerMasterModalDataList.head;
    while (modalData != 0) {
        PlayerMasterModalData* const next = modalData != 0 ? modalData->next : 0;
        ::operator delete(modalData);
        modalData = next;
    }

    g_PlayerMasterModalDataList.listAux = 0;
    g_PlayerMasterModalDataList.tail = 0;
    g_PlayerMasterModalDataList.head = 0;
    g_PlayerMasterModalDataList.count = 0;

    AINet::FreeAll();
    g_Player_NextOrdinal = 0;
    g_GameStateOrMapTable = 0;
    ((HudUiContainer*)(&g_zVideo_FxPass3ConfigLocal))->RemoveChild(&g_Player_UnderwaterFxPass3Ui);
    ((HudUiContainer*)(&g_zVideo_FxPass3ConfigLocal))->RemoveChild(&g_Player_State7FxPass3Ui);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-destroysavegamestate
 * @recoil-artifact defines .text recoil:function:0x41fd20: Player::DestroySaveGameState
 * @recoil-match byte
 *
 * Source file: D:\Proj\Battlesport\player.cpp.
 * Purpose: Tear down a mission save state, its sensor track node, and owned resources.
 */
void __fastcall DestroySaveGameState(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    FreeAltWeaponTrailRuntimeStates(saveState);
    CZNode::ClearDamageHandler(playerState->rootNode);

    HudUiMgrSensorTrackNode* const trackNode = (HudUiMgrSensorTrackNode*)(playerState->rootNode->callbackContext);
    if (trackNode != 0) {
        if (g_HudUiMgrSensor_TrackList.count != 0) {
            if (trackNode == g_HudUiMgrSensor_TrackList.head) {
                --g_HudUiMgrSensor_TrackList.count;
                g_HudUiMgrSensor_TrackList.head = trackNode->next;
                if (g_HudUiMgrSensor_TrackList.head == 0) {
                    g_HudUiMgrSensor_TrackList.trackListAux = 0;
                    g_HudUiMgrSensor_TrackList.tail = 0;
                }
            } else {
                for (HudUiMgrSensorTrackNode* cursor = g_HudUiMgrSensor_TrackList.head; cursor != 0;
                    cursor = cursor->next) {
                    if (cursor->next == trackNode) {
                        --g_HudUiMgrSensor_TrackList.count;
                        cursor->next = trackNode->next;
                        if (g_HudUiMgrSensor_TrackList.tail == trackNode) {
                            g_HudUiMgrSensor_TrackList.tail = cursor;
                        }
                        break;
                    }
                }
            }
        }
        free(trackNode);
    }

    if (saveState != 0 && g_PlayerSaveStateList.count != 0) {
        if (saveState == g_PlayerSaveStateList.head) {
            --g_PlayerSaveStateList.count;
            g_PlayerSaveStateList.head = saveState->next;
            if (g_PlayerSaveStateList.head == 0) {
                g_PlayerSaveStateList.listAux = 0;
                g_PlayerSaveStateList.tail = 0;
            }
        } else {
            for (zUtil_SaveGameState* cursor = g_PlayerSaveStateList.head; cursor != 0; cursor = cursor->next) {
                if (cursor->next == saveState) {
                    --g_PlayerSaveStateList.count;
                    cursor->next = saveState->next;
                    if (g_PlayerSaveStateList.tail == saveState) {
                        g_PlayerSaveStateList.tail = cursor;
                    }
                    break;
                }
            }
        }
    }

    if (saveState != 0) {
        saveState->FreeOwnedResources();
        ::operator delete(saveState);
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-getaivzrdpath
 * @recoil-artifact defines .text recoil:function:0x41fe40: Player::GetAivZrdPath.
 * @recoil-match byte
 *
 * Purpose: return the static player AIV archive path used by mission bootstrap.
 */
const char* __cdecl GetAivZrdPath()
{
    return g_Player_AivZrdPath;
}
} // namespace Player
namespace zVehicle {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-zvehicle-selectzrdbydifficulty
 * @recoil-artifact defines .text recoil:function:0x41fe50: zVehicle::SelectZrdByDifficulty.
 * @recoil-match byte
 *
 * Purpose: select the difficulty-specific vehicle ZRD archive, falling back to the default archive when the selected
 * path is unavailable.
 */
const char* __fastcall SelectZrdByDifficulty(const char* extraSearchPath)
{
    const char* filename;
    switch (zOpt::GetGameDifficultyMode()) {
    case 0:
        filename = g_Player_VehicleArchiveName_Easy;
        break;
    case 2:
        filename = g_Player_VehicleArchiveName_Hard;
        break;
    default:
        filename = g_Player_VehicleArchiveName_Default;
        break;
    }
    if (zReader::FindFile(filename, extraSearchPath) == 0) {
        filename = g_Player_VehicleArchiveName_Default;
    }

    return filename;
}
} // namespace zVehicle
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-initmissionruntimefromworldandcamera
 * @recoil-artifact defines .text recoil:function:0x41fe90: Player::InitMissionRuntimeFromWorldAndCamera
 *
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: initialize mission player runtime from world/camera nodes, attach
 * one-time HUD panels, load player/vehicle tuning, create the stealth
 * save-state, and continue into AIV/local-player bootstrap when aiv.zrd loads.
 * Source owner: battlesport_gameplay.player_mission_runtime_bootstrap.
 * BN evidence: current assembly writes the first-run HUD gate, world/camera
 * globals, camera-zone defaults and player.zrd overrides, runtime input flags,
 * stealth save-state pointer, AIV parent-dir buffer, and the missing-aiv.zrd
 * early return path at 0x420870.
 */
void __fastcall InitMissionRuntimeFromWorldAndCamera(CZNodePartial* worldNode, CZNodePartial* cameraNode)
{
    if (g_Player_MissionInitFirstRunFlag != 0) {
        g_HudUiTopMessageStack->AddChild((HudUiElement*)(&g_Player_TopMsgPanel1));
        g_HudUiTopMessageStack->AddChild((HudUiElement*)(&g_Player_TopMsgPanel2));
        g_Player_MissionInitFirstRunFlag = 0;
    }

    if (zOpt::GetNetworkEnabled() == 0) {
        AINet::LoadAllFromZrd();
    }

    g_Player_TopMsgPanel1.SetTextFmt(zLoc::GetMessageString(0x909));
    ((HudUiElement*)(&g_Player_TopMsgPanel1))->x = 55;
    ((HudUiElement*)(&g_Player_TopMsgPanel1))->y = 66;
    ((HudUiElement*)(&g_Player_TopMsgPanel1))->Invalidate();
    ((HudUiElement*)(&g_Player_TopMsgPanel1))->SetVisible(0);

    g_Player_TopMsgPanel2.SetTextFmt(zLoc::GetMessageString(0x910));
    ((HudUiElement*)(&g_Player_TopMsgPanel2))->x = 55;
    ((HudUiElement*)(&g_Player_TopMsgPanel2))->y = 66;
    ((HudUiElement*)(&g_Player_TopMsgPanel2))->Invalidate();
    ((HudUiElement*)(&g_Player_TopMsgPanel2))->SetVisible(0);

    ((HudUiContainer*)(&g_zVideo_FxPass3ConfigLocal))->AddChild(&g_Player_UnderwaterFxPass3Ui);
    ((HudUiElement*)(&g_Player_UnderwaterFxPass3Ui))->SetVisible(0);
    ((HudUiContainer*)(&g_zVideo_FxPass3ConfigLocal))->AddChild(&g_Player_State7FxPass3Ui);
    ((HudUiElement*)(&g_Player_State7FxPass3Ui))->SetVisible(0);

    g_Player_RuntimeDiScene = worldNode;
    g_MainCamera = cameraNode;
    g_Player_HorizonNode = CZClass::FindSubNodeByName(g_HudSensorTracker.worldNode, g_Player_NodeName_Horizon);

    float fovX;
    float fovY;
    CZCamera::gwCameraGetFOV(g_MainCamera, &fovX, &fovY);
    CZCamera::gwCameraSetPosition(g_MainCamera, 0.0f, 0.0f, 0.0f);
    CZCamera::gwCameraSetTarget(g_MainCamera, 0.0f, 0.0f, 0.0f);
    if (g_Player_HorizonNode != 0) {
        g_Player_HorizonNodeFollowCameraEnabled = 1;
        CZObject3D::gwObject3DSetPosition(g_Player_HorizonNode, 0.0f, 0.0f, 0.0f);
    }

    memset(&g_VariantTag_Current, 0, sizeof(g_VariantTag_Current));
    zTag4::Clear(&g_VariantTag_Current);
    g_Variant_CurrentTag = g_VariantTag_Current;

    PLAYER_INIT_ACTION_CALLBACK_NODE((void*)(&TickAllPlayers));
    PLAYER_INIT_ACTION_CALLBACK_NODE((void*)(&PickupRespawnQueue::Update));
    PLAYER_INIT_ACTION_CALLBACK_NODE((void*)(&CZObject3DModelRefLerpQueue::Update));

    g_Player_LocalControlEnabled = zOpt::GetNetworkEnabled();
    g_Player_TotalTimeSecScaled = g_Time_AccumulatedTimeSec;
    g_Player_CameraZone = 0.899999976f;
    g_Player_CameraZoneInvRange = 10.0f;
    g_Player_CopterSndNode1 = 0;
    g_Player_CopterSndNode2 = 0;
    g_Player_BftSplashAnimEntry = zEffectAnim::FindEntryByName("bftsplash");

    zReader::Node* playerRoot = zReader::Load(g_Player_ConfigArchiveName, 0, 0);
    {
        zReader::Node* const root = playerRoot;
        zReader::Node* node = zRdrGetNode(root, g_Player_ConfigKey_CameraZone);
        if (node != 0) {
            const float cameraZone = PlayerZrdArrayFloat(node, 1);
            if (cameraZone > 0.0f && cameraZone < 1.0f) {
                g_Player_CameraZone = cameraZone;
                g_Player_CameraZoneInvRange = 1.0f / (1.0f - cameraZone);
            }
        }

        node = zRdrGetNode(root, g_Player_ConfigKey_MaxCamYawRate);
        g_Player_MaxCamYawRate = node != 0 ? PlayerZrdArrayFloat(node, 1) : 2.0f;

        node = zRdrGetNode(root, g_Player_ConfigKey_MousePush);
        if (node != 0) {
            g_Player_MousePushX = PlayerZrdArrayFloat(node, 1);
            g_Player_MousePushY = PlayerZrdArrayFloat(node, 2);
        } else {
            g_Player_MousePushX = 0.00200000009f;
            g_Player_MousePushY = 0.00999999978f;
        }

        node = zRdrGetNode(root, g_Player_ConfigKey_FirstPersonCamElevationRate);
        g_Player_FpCamElevationRate = node != 0 ? PlayerZrdArrayFloat(node, 1) : 5.0f;

        node = zRdrGetNode(root, g_Player_ConfigKey_FirstPersonCamElevationLimit);
        if (node != 0) {
            g_Player_FpCamElevationMin = PlayerZrdArrayFloat(node, 1);
            g_Player_FpCamElevationMax = PlayerZrdArrayFloat(node, 2);
        } else {
            g_Player_FpCamElevationMin = -0.75f;
            g_Player_FpCamElevationMax = 1.0f;
        }

        node = zRdrGetNode(root, g_Player_ConfigKey_UnderwaterCam);
        if (node != 0) {
            int rBits;
            int gBits;
            int bBits;
            g_Player_UnderwaterCamDistance = PlayerZrdArrayFloat(node, 1);
            g_Player_UnderwaterCamHeight = PlayerZrdArrayFloat(node, 2);
            g_Player_UnderwaterCamStepCount = PlayerZrdArrayInt(node, 3);
            g_Player_UnderwaterCamFar = PlayerZrdArrayFloat(node, 4);
            zVideo::PixelPackGetRgbBits(&rBits, &gBits, &bBits);
            g_Player_UnderwaterCamPackedColor = (PlayerZrdArrayInt(node, 5) >> (8 - rBits) << (bBits + gBits))
                + (PlayerZrdArrayInt(node, 6) >> (8 - gBits) << bBits) + (PlayerZrdArrayInt(node, 7) >> (8 - bBits));
            g_Player_UnderwaterCamAlpha = PlayerZrdArrayFloat(node, 8);
        } else {
            g_Player_UnderwaterCamDistance = 5.0f;
            g_Player_UnderwaterCamHeight = 4.0f;
            g_Player_UnderwaterCamStepCount = 12;
            g_Player_UnderwaterCamFar = 100.0f;
            g_Player_UnderwaterCamPackedColor = 0x1f5;
            g_Player_UnderwaterCamAlpha = 0.5f;
        }

        node = zRdrGetNode(root, g_Player_ConfigKey_CameraElastic);
        if (node != 0) {
            g_Player_CameraElastic = PlayerZrdArrayFloat(node, 1);
        }

        node = zRdrGetNode(root, g_Player_ConfigKey_MaxCamTetherAngle);
        if (node != 0) {
            g_Player_MaxCamTetherAngleRad = PlayerZrdArrayFloat(node, 1) * 0.01745329251994;
        }

        node = zRdrGetNode(root, g_Player_ConfigKey_NormalGravity);
        g_Player_NominalGravity = node != 0 ? PlayerZrdArrayFloat(node, 1) : 28.0f;

        node = zRdrGetNode(root, g_Player_ConfigKey_WaterGravity);
        g_Player_WaterGravity = node != 0 ? PlayerZrdArrayFloat(node, 1) : g_Player_NominalGravity * 0.333333343f;

        node = zRdrGetNode(root, g_Player_ConfigKey_QuicksandGravity);
        g_Player_QuicksandGravity = node != 0 ? PlayerZrdArrayFloat(node, 1) : g_Player_NominalGravity * 0.166666672f;

        node = zRdrGetNode(root, g_Player_ConfigKey_QuicksandSink);
        g_Player_QuicksandSinkRate = node != 0 ? PlayerZrdArrayFloat(node, 1) : 0.899999976f;

        node = zRdrGetNode(root, g_Player_ConfigKey_LavaSink);
        g_Player_LavaSinkRate = node != 0 ? PlayerZrdArrayFloat(node, 1) : 0.600000024f;

        node = zRdrGetNode(root, g_Player_ConfigKey_MaxSlope);
        g_Player_MaxSlope = node != 0 ? PlayerZrdArrayFloat(node, 1) : 0.707000017f;

        node = zRdrGetNode(root, g_Player_ConfigKey_MakeHot);
        if (node != 0) {
            g_Player_MakeHotOptEntry = OptCatalog::FindEntryByName(PlayerZrdArrayString(node, 1));
        }

        node = zRdrGetNode(root, g_Player_ConfigKey_MakeCold);
        if (node != 0) {
            g_Player_MakeColdOptEntry = OptCatalog::FindEntryByName(PlayerZrdArrayString(node, 1));
        }

        node = zRdrGetNode(root, g_Player_BurningAnimName);
        if (node != 0) {
            g_PlayerRecentHitFxAnimEntry = zEffectAnim::FindEntryByName(PlayerZrdArrayString(node, 1));
        }

        node = zRdrGetNode(root, g_Player_LowShieldSndName);
        if (node != 0) {
            g_Hud_LowMeterBeepSample = zSnd::FindSampleByName(PlayerZrdArrayString(node, 1));
            g_Hud_LowMeterBeepInterval = PlayerZrdArrayFloat(node, 2);
            g_Hud_LowMeterLoopSample = zSnd::FindSampleByName(PlayerZrdArrayString(node, 3));
        }

        g_PlayerStatusMeterRatio = 1.0f;
        g_Hud_LowMeterNextBeepTime = 0.0f;
        g_Player_CopterSndSample = zSnd::FindSampleByName("snd_chopper");
    }
    zReader::Free(playerRoot);

    g_Player_RuntimeInputFlags = 3;
    zEffectAnimEntry* asyncEntry = zEffectAnim::FindNextAsyncEntry(0);
    while (asyncEntry != 0) {
        zEffectAnimEntry::SetOnStateDoneCallback(asyncEntry, (void*)(&AsyncCommandCallback), 0);
        asyncEntry = zEffectAnim::FindNextAsyncEntry(asyncEntry);
    }

    zReader::Node* vehicleRoot = zReader::Load(zVehicle::SelectZrdByDifficulty(0), 0, 0);
    zReader::Node* const vehicleList = PlayerZrdArrayNode(vehicleRoot, 1);
    const int vehicleCount = (PlayerZrdArrayCount(vehicleList) - 1) / 2;
    for (int vehicleIndex = 0; vehicleIndex < vehicleCount; ++vehicleIndex) {
        char vehicleName[0x14];
        strcpy(vehicleName, PlayerZrdArrayString(vehicleList, vehicleIndex * 2 + 1));

        PlayerMasterCommonData* const commonData
            = (PlayerMasterCommonData*)(::operator new(sizeof(PlayerMasterCommonData)));
        memset(commonData, 0, sizeof(PlayerMasterCommonData));
        if (commonData != 0) {
            commonData->next = 0;
            if (g_PlayerMasterCommonDataList.count == 0) {
                g_PlayerMasterCommonDataList.head = commonData;
            } else {
                g_PlayerMasterCommonDataList.tail->next = commonData;
            }
            g_PlayerMasterCommonDataList.tail = commonData;
            commonData->next = 0;
            ++g_PlayerMasterCommonDataList.count;
        }
        zReader::Node* const vehicleNode = zRdrGetNode(vehicleRoot, vehicleName);
        LoadMasterCommonDataFromNode(commonData, vehicleNode, vehicleName);

        for (int modalIndex = 0; modalIndex < commonData->modalCount; ++modalIndex) {
            PlayerMasterModalData* const modalData
                = (PlayerMasterModalData*)(::operator new(sizeof(PlayerMasterModalData)));
            memset(modalData, 0, sizeof(PlayerMasterModalData));
            if (modalData != 0) {
                modalData->next = 0;
                if (g_PlayerMasterModalDataList.count == 0) {
                    g_PlayerMasterModalDataList.head = modalData;
                } else {
                    g_PlayerMasterModalDataList.tail->next = modalData;
                }
                g_PlayerMasterModalDataList.tail = modalData;
                modalData->next = 0;
                ++g_PlayerMasterModalDataList.count;
            }
            zReader::Node* const modalNode = PlayerZrdArrayNode(vehicleNode, modalIndex * 2 + 4);
            LoadMasterModalDataFromNode(modalData, modalNode, vehicleName);
            strcpy(commonData->modalNames[modalIndex], modalData->modeName);
        }
    }

    zUtil_SaveGameState* const stealthSaveState = new zUtil_SaveGameState;
    if (stealthSaveState != 0) {
        stealthSaveState->next = 0;
        if (g_PlayerSaveStateList.count == 0) {
            g_PlayerSaveStateList.head = stealthSaveState;
        } else {
            g_PlayerSaveStateList.tail->next = stealthSaveState;
        }
        g_PlayerSaveStateList.tail = stealthSaveState;
        stealthSaveState->next = 0;
        ++g_PlayerSaveStateList.count;
    }
    zUtil_PlayerStateStorage* const stealthPlayerState = stealthSaveState->playerState;
    memset(stealthPlayerState, 0, sizeof(*stealthPlayerState));
    PlayerModalState* const stealthModalState = (PlayerModalState*)zUtilSaveGameStateListAllocAppend(stealthSaveState);
    g_Player2SaveState = stealthSaveState;
    stealthPlayerState->rootNode = CZObject3D::gwObject3DInit();
    CZClass::gwNodeSetName(stealthPlayerState->rootNode, g_Player_DisplayName_Stealth);
    CZObject3D::gwObject3DSetPosition(stealthPlayerState->rootNode, 500.0f, 50.0f, 500.0f);
    CZObject3D::gwObject3DSetRotation(stealthPlayerState->rootNode, 0.0f, 0.0f, 0.0f);
    CZClass::gwNodeSetPriority(stealthPlayerState->rootNode, 1);
    CZClass::gwNodeSetRaycastable(stealthPlayerState->rootNode, 0);
    CZClass::gwNodeSetCellPickable(stealthPlayerState->rootNode, 0);
    zRdrGetNode(zRdrGetNode(vehicleRoot, g_Player_ConfigNode_Stealth), g_Player_ConfigNode_CommonMode);
    InitStateFromNameAndMasterCommonData(stealthSaveState, g_Player_ConfigNode_Stealth, g_Player_ConfigNode_Stealth);
    BindModalStateFromMasterModalData(
        stealthSaveState,
        stealthModalState,
        g_Player_ConfigNode_Stealth,
        g_Player_ConfigNode_Basic
    );
    InitSpawnStateFromPrimaryModalData(stealthSaveState);
    stealthSaveState->firstSaveState->playerState->projectileSpawnVel.x = 0.0f;
    stealthPlayerState->cameraState = zOpt::GetCameraModePlayerState();

    zReader::Node* aivRoot = zReader::Load(GetAivZrdPath(), 0, 0);
    if (aivRoot == 0) {
        zError::ReportOld(0x800, "D:\\Proj\\Battlesport\\player.cpp", 0x399, g_Player_AivArchiveMissingMsg);
        return;
    }

    zReader::BuildResolvedParentDir(GetAivZrdPath(), g_Player_AivParentDir);
    zReader::Node* const aivList = PlayerZrdArrayNode(aivRoot, 1);
    int aivCount = (PlayerZrdArrayCount(aivList) - 1) / 2;
    if (zOpt::GetNetworkEnabled() != 0) {
        aivCount = 1;
    }

    for (int aivIndex = 0; aivIndex < aivCount; ++aivIndex) {
        char aivName[0x1c];
        char vehicleName[0x14];
        strcpy(aivName, PlayerZrdArrayString(aivList, aivIndex * 2 + 1));
        ExtractVehicleNameFromAivName(aivName, vehicleName);

        if (zRdrGetNode(vehicleRoot, vehicleName) != 0) {
            zReader::Node* const aivNode = zRdrGetNode(aivRoot, aivName);
            if (aivNode != 0) {
                zReader::Node* const spawnNode = PlayerZrdArrayNode(aivNode, 2);
                zVec3 spawnPos;
                spawnPos.x = PlayerZrdArrayFloat(spawnNode, 1);
                spawnPos.y = PlayerZrdArrayFloat(spawnNode, 2);
                spawnPos.z = PlayerZrdArrayFloat(spawnNode, 3);
                CreateFromNamesAtPose(
                    &spawnPos,
                    PlayerZrdArrayFloat(aivNode, 3),
                    PlayerZrdArrayInt(aivNode, 1),
                    vehicleName,
                    aivName
                );
            }
        }
    }

    zReader::Free(vehicleRoot);
    zReader::Free(aivRoot);

    zUtil_SaveGameState* const headSaveState = g_PlayerSaveStateList.head;
    headSaveState->playerState->lifecycleState = kPlayerLifecycleInactive;
    zUtil_SaveGameState* const localSaveState = headSaveState != 0 ? headSaveState->next : 0;
    g_LocalPlayerSaveState = localSaveState;
    g_CurrentPlayerSaveState = localSaveState;
    localSaveState->playerState->cameraTickEnabled = 1;
    localSaveState->playerState->transitionDamageSuppressed = 0;
    g_VariantTag_Current = localSaveState->playerState->variantTag;
    g_Player_LastValidCameraVariantTag = localSaveState->playerState->variantTag;
    g_Variant_CurrentTag = localSaveState->playerState->variantTag;
    zEffect::SetConditionalRefPos(&localSaveState->playerState->worldPos);
    localSaveState->playerState->lifecycleState = kPlayerLifecycleLocal;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial*)localSaveState;

    if (zOpt::GetNetworkEnabled() != 0) {
        localSaveState->playerState->amphibUnlocked = IsMissionProbeType1EnabledById(g_HudSensorTracker.GetMissionId());
    } else {
        if (g_HudSensorTracker.GetMissionId() == 6) {
            localSaveState->playerState->subUnlocked = 1;
        }
        if (g_HudSensorTracker.GetMissionId() >= 4) {
            localSaveState->playerState->hoverUnlocked = 1;
        }
        if (g_HudSensorTracker.GetMissionId() >= 3) {
            localSaveState->playerState->amphibUnlocked = 1;
        }
    }

    stealthPlayerState->worldPos = localSaveState->playerState->worldPos;
    CZObject3D::gwObject3DSetPosition(
        stealthPlayerState->rootNode,
        stealthPlayerState->worldPos.x,
        stealthPlayerState->worldPos.y,
        stealthPlayerState->worldPos.z
    );
    stealthPlayerState->vehicleRotationAngles = localSaveState->playerState->vehicleRotationAngles;
    CZObject3D::gwObject3DSetRotation(
        stealthPlayerState->rootNode,
        stealthPlayerState->vehiclePitchRad,
        stealthPlayerState->restartYawRad,
        stealthPlayerState->vehicleRollRad
    );

    AINet::BuildAiPeerRingsByAiNetId();
    zInput::BindMapCurrentSetCommandCallback(15, (zInputCommandCallbackFn)(HandlePrimaryWeaponVariantToggleInput));
    zInput::BindMapCurrentSetCommandCallback(16, (zInputCommandCallbackFn)(HandleAltWeaponBankSelectInput));
    zInput::BindMapCurrentSetCommandCallback(17, (zInputCommandCallbackFn)(HandleAltWeaponBankSelectInput));
    zInput::BindMapCurrentSetCommandCallback(18, (zInputCommandCallbackFn)(HandleAltWeaponBankSelectInput));
    zInput::BindMapCurrentSetCommandCallback(19, (zInputCommandCallbackFn)(HandleAltWeaponBankSelectInput));
    zInput::BindMapCurrentSetCommandCallback(20, (zInputCommandCallbackFn)(HandleAltWeaponBankSelectInput));
    zInput::BindMapCurrentSetCommandCallback(21, (zInputCommandCallbackFn)(HandleAltWeaponBankSelectInput));
    zInput::BindMapCurrentSetCommandCallback(22, (zInputCommandCallbackFn)(HandleAltWeaponBankSelectInput));
    zInput::BindMapCurrentSetCommandCallback(23, (zInputCommandCallbackFn)(HandleAltWeaponBankSelectInput));
    RegisterGameplayCommandCallbacksAndCreateFfEffects();
    CZNode::MaskExtraFlagsRecursive(g_Player_RuntimeDiScene, 0);
    zReader::LoadMoversFromZrd();
    if (g_HudSensorTracker.raceCheckpointMode != 0) {
        Checkpoint::InstantiateNamedObjects();
    }
}
} // namespace Player
namespace zReader {
/**
 * @recoil-anchor recoil:anchor:battlesport.player.z-reader-load-movers-from-zrd
 * @recoil-artifact defines .text recoil:function:0x420be0: zReader::LoadMoversFromZrd.
 * @recoil-match byte
 *
 * Purpose: load mover definitions from the current ZRD tree.
 */
void __fastcall LoadMoversFromZrd()
{
    Node* const treeRoot = Load("movers.zrd", 0, 0);
    if (treeRoot == 0) {
        return;
    }

    const int moverCount = treeRoot->value.nodes[1].value.nodes[0].value.i32 - 1;
    for (int i = 0; i < moverCount; ++i) {
        CZNodePartial* const mover
            = CZClass::FindByTypeAndName(6, treeRoot->value.nodes[1].value.nodes[i + 1].value.str);
        if (mover != 0) {
            CZNode::PropagateExtraFlagsRecursive(mover, 1);
            CZNode::SetContextRecursive(mover, mover, 0x200000);
            g_Mover_LastLoadedNode = mover;
        }
    }

    Free(treeRoot);
}
} // namespace zReader
namespace Checkpoint {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-checkpoint-instantiatenamedobjects
 * @recoil-artifact defines .text recoil:function:0x420c60: Checkpoint::InstantiateNamedObjects
 * @recoil-match byte
 *
 * Purpose: Resolves checkpoint nodes by name and recursively stamps their race
 * checkpoint flags and callback context.
 */
void __fastcall InstantiateNamedObjects()
{
    CString searchName;
    const int checkpointCount = g_HudSensorTracker.checkpointCount;

    for (int checkpointIndex = 0; checkpointIndex < checkpointCount; ++checkpointIndex) {
        searchName.Format(g_Checkpoint_NodeNameFmt, checkpointIndex + 1);
        CZNodePartial* const checkpointNode = CZClass::FindByTypeAndName(6, (const char*)searchName);
        if (checkpointNode != 0) {
            CZNode::PropagateExtraFlagsRecursive(checkpointNode, kCheckpointNodeAuxFlagTracked);
            CZNode::PropagateFlagsRecursive(checkpointNode, kCheckpointNodePickableFlag);
            CZNode::SetContextRecursive(checkpointNode, checkpointNode, kCheckpointNodeContextFlag);
        }
    }
}
} // namespace Checkpoint
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-initstatefromnameandmastercommondata
 * @recoil-artifact defines .text recoil:function:0x420d10: Player::InitStateFromNameAndMasterCommonData
 * @recoil-match byte
 *
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: bind a save-state record to master common data by name and
 * initialize the player's common bootstrap state.
 * Source owner: Player save-state/bootstrap record-global subsystem, not a
 * C++ Player class.
 */
void __fastcall InitStateFromNameAndMasterCommonData(
    zUtil_SaveGameState* saveState,
    const char* objectName,
    const char* masterCommonDataName
)
{
    zUtil_SaveGameState* const localSaveState = GetSaveStateListHead();
    zUtil_PlayerStateStorage* const localPlayerState = localSaveState->playerState;
    GetSaveStateListHead();

    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterCommonData* commonData = g_PlayerMasterCommonDataList.head;
    while (commonData != 0) {
        if (strcmp(commonData->vehicleName, masterCommonDataName) == 0) {
            playerState->masterCommonData = commonData;
            break;
        }
        commonData = commonData != 0 ? commonData->next : 0;
    }

    if (playerState->masterCommonData == 0) {
        char errorText[0x80];
        sprintf(errorText, g_Player_MasterCommonDataMissingFmt, objectName);
        zError::ReportOld(0x800, g_Player_SourceFile_PlayerCpp, 0x46d, errorText);
    }

    playerState->playerOrdinal = g_Player_NextOrdinal;
    ++g_Player_NextOrdinal;
    if (playerState->playerOrdinal == 1) {
        g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial*)saveState;
    }

    CZObject3D::gwObject3DGetPosition(
        playerState->rootNode,
        &playerState->worldPos.x,
        &playerState->worldPos.y,
        &playerState->worldPos.z
    );
    CZObject3D::gwObject3DGetRotation(
        playerState->rootNode,
        &playerState->vehiclePitchRad,
        &playerState->restartYawRad,
        &playerState->vehicleRollRad
    );
    playerState->poseCache = playerState->vehicleRotationAngles;
    playerState->angVel = g_Player_ConstZeroVec3;

    zMath::MatBuildEulerRotation3x3(
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        playerState->vehicleRollRad,
        &playerState->motionBasis
    );
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    playerState->previousTransform = playerState->motionBasis;
    playerState->steerBasisRef.x = playerState->motionBasis.yx;
    playerState->steerBasisRef.y = playerState->motionBasis.yy;
    playerState->steerBasisRef.z = playerState->motionBasis.yz;
    playerState->steerBasisRaw.x = -playerState->motionBasis.zx;
    playerState->steerBasisRaw.y = -playerState->motionBasis.zy;
    playerState->steerBasisRaw.z = -playerState->motionBasis.zz;
    playerState->steerBasisNorm = playerState->steerBasisRaw;
    playerState->steerBasisNorm.y = 0.0f;
    zMath::Vec3NormalizeXZ(&playerState->steerBasisNorm, &playerState->steerBasisNorm);
    playerState->cameraDirFlat = playerState->steerBasisNorm;

    AINet* aiNet;
    if (playerState->aiNetId != 0 && (aiNet = AINet::FindByNetId(playerState->aiNetId)) != 0) {
        playerState->lifecycleState = kPlayerLifecycleAi;
    } else {
        playerState->lifecycleState = kPlayerLifecycleInactive;
    }
    if (playerState->lifecycleState == kPlayerLifecycleAi) {
        playerState->aiNet = aiNet;
        switch (aiNet->aiType) {
        case AINET_TYPE_ST:
            playerState->aiTopLevelState = kPlayerAiTopPathFollow;
            break;
        case AINET_TYPE_HI:
            playerState->aiTopLevelState = kPlayerAiTopTurnTowardTarget;
            break;
        case AINET_TYPE_FI:
            playerState->aiTopLevelState = kPlayerAiTopTurnOnlyTowardTarget;
            break;
        case AINET_TYPE_DE:
            playerState->aiTopLevelState = kPlayerAiTopPathSteering;
            break;
        }

        playerState->aiCurrentSteeringSubstate = aiNet->attackStrategy;
        playerState->aiHideTime0 = aiNet->hideTime0;
        playerState->aiHideTime1 = aiNet->hideTime1;
        playerState->aiCurrentPathNode = AINet::FindNearestNode(&playerState->worldPos, aiNet->nodeListHead);
        playerState->aiHomePathNode = playerState->aiCurrentPathNode;

        CZNodePartial* const healthyNode
            = CZClass::FindSubNodeByName(playerState->rootNode, g_Player_HealthySubNodeName);
        if (healthyNode != 0) {
            CZClass::gwNodeSetCellPickable(healthyNode, 0);
        }

        if (aiNet->activateRadius != 0.0f) {
            playerState->aiActivationRadiusSq = aiNet->activateRadius * aiNet->activateRadius;
        }
        if (aiNet->attackRadius != 0.0f) {
            playerState->aiAttackRadiusSq = aiNet->attackRadius * aiNet->attackRadius;
        } else {
            playerState->aiAttackRadiusSq = 1500.0f;
        }
        if (aiNet->attackDwell != 0.0f) {
            playerState->aiMode2AttackDwell = aiNet->attackDwell;
        } else {
            playerState->aiMode2AttackDwell = 10.0f;
        }
        if (aiNet->notPursuitDwell != 0.0f) {
            playerState->aiNotPursuitDwell = aiNet->notPursuitDwell;
        }
        if (aiNet->returnRange != 0.0f) {
            playerState->aiRestoreDistanceSq = aiNet->returnRange * aiNet->returnRange;
        }

        saveState->aiPeerRingNext = saveState;
        playerState->aiStateUntilTime = g_Time_AccumulatedTimeSec + 10.0f;
        playerState->aiStateStartTime = playerState->aiStateUntilTime;
    }

    playerState->regenSkinFxEntry = zEffectAnim::FindEntryByName(g_Player_RegenSkinNodeName);
    playerState->masterTypeTransitionToAmphibNodeAction = zEffectAnim::FindEntryByName(g_Player_BoatWakeTrailName);
    playerState->masterTypeTransitionToTrackNodeAction = zEffectAnim::FindEntryByName(g_Player_BftExhaustTrailName);
    playerState->shatterVehicleFxEntry = zEffectAnim::FindEntryByName(g_Player_ShatterVehicleEffectName);
    playerState->shockVehicleFxEntry = zEffectAnim::FindEntryByName(g_Player_ShockVehicleEffectName);
    playerState->napalmVehicleFxEntry = zEffectAnim::FindEntryByName(g_Player_NapalmVehicleEffectName);
    playerState->masterTypeTransitionToSubNodeAction = zEffectAnim::FindEntryByName(g_Player_BftBubbleEffectName);
    playerState->subTransitionFxEntry = zEffectAnim::FindEntryByName(g_Player_NodeName_Subt);

    playerState->destroyedRespawnFxEntry
        = zEffectAnim::FindEntryByName(strstr(objectName, "net") != 0 ? g_Player_NodeName_Bft00 : objectName);
    if (strstr(objectName, "net") != 0 || strstr(objectName, g_Player_NodeName_Bft) != 0) {
        playerState->masterTypeTransitionToTrackLightHandle = zEffectAnim::SetVelocityThunk(
            playerState->masterTypeTransitionToTrackNodeAction,
            playerState->rootNode,
            0.0f,
            0.0f,
            0.0f
        );
    }
    if (commonData->startAnimsName != 0) {
        zEffectAnimEntry* const startAnims = zEffectAnim::FindEntryByName(commonData->startAnimsName);
        zEffectAnim::SetVelocityThunk(startAnims, playerState->rootNode, 0.0f, 0.0f, 0.0f);
    }

    playerState->cameraState = zOpt::GetCameraModePlayerState();
    playerState->cameraLerpActive = 0;
    playerState->thirdPersonYawOffset = 0.0f;
    playerState->cameraBackOffset = commonData->cameraBackOffset;
    playerState->cameraBack1 = commonData->camback1;
    playerState->cameraBack2 = commonData->camback2;
    playerState->cameraYOffset = commonData->aimYawRate;
    playerState->cameraYOffset = commonData->aimYawMax;
    playerState->cameraState2TargetOffset.x = 0.0f;
    playerState->cameraState2TargetOffset.y = 150.0f;
    playerState->cameraState2TargetOffset.z = 0.0f;
    playerState->unknown_00d4 = 0;
    playerState->unknown_00d8 = 0;
    playerState->unknown_00dc = 0;
    playerState->unknown_00e0 = 0;
    playerState->altGunAimOrigin = kPlayerDefaultAltGunAimOrigin;
    playerState->activeAltBankIndex = 1;
    playerState->autoTurnActive = 0;
    playerState->cameraTransitionTimer = 0;
    playerState->cameraTransitionBlend = 1.0f;

    CZNodePartial* const targetNode = CZClass::FindSubNodeByName(playerState->rootNode, g_Player_NodeName_Target);
    if (targetNode != 0) {
        CZObject3D::gwObject3DGetPosition(
            targetNode,
            &playerState->fxOffsetLocal.x,
            &playerState->fxOffsetLocal.y,
            &playerState->fxOffsetLocal.z
        );
        CZClass::gwNodeSetActive(targetNode, 0);
    } else {
        playerState->fxOffsetLocal.x = 0.0f;
        playerState->fxOffsetLocal.y = 0.0f;
        playerState->fxOffsetLocal.z = 0.0f;
    }
    playerState->fxOffsetWorld.x = playerState->worldPos.x + playerState->fxOffsetLocal.x;
    playerState->fxOffsetWorld.y = playerState->worldPos.y + playerState->fxOffsetLocal.y;
    playerState->fxOffsetWorld.z = playerState->worldPos.z + playerState->fxOffsetLocal.z;

    playerState->bodyNode = CZClass::FindSubNodeByName(playerState->rootNode, g_Player_NodeName_Body);
    playerState->turretNode = CZClass::FindSubNodeByName(playerState->rootNode, g_Player_NodeName_Turret);
    playerState->doorLeftNode = CZClass::FindSubNodeByName(playerState->rootNode, g_Player_NodeName_DoorLeft);
    playerState->doorRightNode = CZClass::FindSubNodeByName(playerState->rootNode, g_Player_NodeName_DoorRight);
    playerState->modeVariantNode = CZClass::FindSubNodeByName(playerState->rootNode, g_Player_NodeName_Shadow);

    CacheGunHardpointsAndDetachDisplays(saveState, 1);

    playerState->statusMeterValue = commonData->maxHealth;
    playerState->statusMeterScaled = 1.0f;
    playerState->damageProtectionActive = 0;
    playerState->queuedFixedDamageFlag = 0;
    playerState->recentHitValid = 0;
    playerState->recentHitLightHandle = 0;
    playerState->nanitePanelLevel = 0;

    if (playerState != localPlayerState) {
        HudUiMgrSensorTrackNode* const context = HudUiMgrSensor::TrackListAdd(HUD_SENSOR_TRACK_KIND_PLAYER, saveState);
        CZNode::SetContextRecursive(playerState->rootNode, (CZNodePartial*)context, 0x100000);
    }

    LoadWeaponBanksAndSelectDefaults(saveState);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-bindmodalstatefrommastermodaldata
 * @recoil-artifact defines .text recoil:function:0x421470: Player::BindModalStateFromMasterModalData
 * @recoil-match byte
 *
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: bind a modal state to matching master modal data, cache its model
 * nodes, and populate support/collision probe points when needed.
 * Source owner: Player save-state/bootstrap record-global subsystem, not a
 * C++ Player class.
 */
void __fastcall BindModalStateFromMasterModalData(
    zUtil_SaveGameState* saveState,
    PlayerModalState* modalState,
    const char* objectName,
    const char* modalName
)
{
    GetSaveStateListHead();
    GetSaveStateListHead();

    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterCommonData* const commonData = playerState->masterCommonData;
    PlayerMasterModalData* masterModalData;
    for (masterModalData = g_PlayerMasterModalDataList.head; masterModalData != 0;
        masterModalData = masterModalData != 0 ? masterModalData->next : 0) {
        if (strcmp(masterModalData->modalName, commonData->vehicleName) == 0
            && strcmp(modalName, masterModalData->modeName) == 0) {
            modalState->masterModalData = masterModalData;
            break;
        }
    }

    if (modalState->masterModalData == 0) {
        char errorText[0x100];
        sprintf(errorText, g_Player_MasterModalDataMissingFmt, objectName);
        zError::ReportOld(0x800, g_Player_SourceFile_PlayerCpp, 0x5c3, errorText);
    }

    modalState->nodeRightMorphs = CZClass::FindSubNodeByName(playerState->rootNode, g_Player_NodeName_RightMorphs);
    modalState->nodeLeftMorphs = CZClass::FindSubNodeByName(playerState->rootNode, g_Player_NodeName_LeftMorphs);
    modalState->modalNode = CZClass::FindSubNodeByName(playerState->rootNode, g_Player_NodeName_Chassis);
    modalState->nodeRTracks = CZClass::FindSubNodeByName(playerState->rootNode, g_Player_NodeName_RightTracks);
    modalState->nodeLTracks = CZClass::FindSubNodeByName(playerState->rootNode, g_Player_NodeName_LeftTracks);
    modalState->nodeProps = CZClass::FindSubNodeByName(playerState->rootNode, g_Player_NodeName_Props);
    modalState->nodeCaustic1 = CZClass::FindSubNodeByName(playerState->rootNode, g_Player_EffectNodeName_Caustic1);
    modalState->nodeWake = CZClass::FindSubNodeByName(playerState->rootNode, g_Player_EffectNodeName_Wake);
    modalState->nodeSplashL = CZClass::FindSubNodeByName(playerState->rootNode, g_Player_EffectNodeName_SplashLeft);
    modalState->nodeSplashR = CZClass::FindSubNodeByName(playerState->rootNode, g_Player_EffectNodeName_SplashRight);
    modalState->nodeDustL = CZClass::FindSubNodeByName(playerState->rootNode, g_Player_EffectNodeName_DustLeft);
    modalState->nodeDustR = CZClass::FindSubNodeByName(playerState->rootNode, g_Player_EffectNodeName_DustRight);

    modalState->chassisRollFilterState = 0.0f;
    modalState->chassisPitchFilterState = 0.0f;
    modalState->modalStateCode = 4;

    if (BuildSupportPointsFromModel(saveState, playerState->rootNode) == 0
        && masterModalData->platformPointCount == 0) {
        char errorText[0x100];
        sprintf(errorText, g_Player_SupportPointsMissingFmt, objectName);
        zError::ReportOld(0x800, g_Player_SourceFile_PlayerCpp, 0x5df, errorText);
    }

    if (masterModalData->probePointCount == 0 && BuildCollisionPointsFromModel(saveState, playerState->rootNode) == 0) {
        char errorText[0x100];
        sprintf(errorText, g_Player_CollisionPointsMissingFmt, objectName);
        zError::ReportOld(0x800, g_Player_SourceFile_PlayerCpp, 0x5e6, errorText);
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-initspawnstatefromprimarymodaldata
 * @recoil-artifact defines .text recoil:function:0x421790: Player::InitSpawnStateFromPrimaryModalData
 * @recoil-match byte
 *
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: reset spawn-time state from the primary modal data, build world
 * probe-point caches, and align the root node to the sampled surface.
 * Source owner: Player save-state/bootstrap record-global subsystem, not a
 * C++ Player class.
 */
void __fastcall InitSpawnStateFromPrimaryModalData(zUtil_SaveGameState* saveState)
{
    GetSaveStateListHead();
    GetSaveStateListHead();

    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;

    playerState->spawnStateInitialized = 0;
    playerState->gravityAccel = g_Player_NominalGravity;
    playerState->primaryGunGateUntilTime = 0.0f;
    playerState->primaryFireSlotIndex = 0;
    playerState->altFireSlotIndex = 0;

    for (int i = 0; i < masterModalData->probePointCount; ++i) {
        playerState->rootProbeWorldByIndex[i].x = masterModalData->probePoints[i].x + playerState->worldPos.x;
        playerState->rootProbeWorldByIndex[i].y = masterModalData->probePoints[i].y + playerState->worldPos.y;
        playerState->rootProbeWorldByIndex[i].z = masterModalData->probePoints[i].z + playerState->worldPos.z;
    }

    SampleGroundAndAlignRootToSurface(saveState, 1);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-samplegroundandalignroottosurface
 * @recoil-artifact defines .text recoil:function:0x421830: Player::SampleGroundAndAlignRootToSurface
 * @recoil-match byte
 *
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: sample ground under the player, update the active variant tag, and
 * optionally pitch/roll the root node to the selected surface normal.
 * Source owner: Player save-state/bootstrap record-global subsystem, not a
 * C++ Player class.
 */
void __fastcall SampleGroundAndAlignRootToSurface(zUtil_SaveGameState* saveState, int updateRotation)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;

    zTag4::Clear(&playerState->variantTag);
    g_Variant_CurrentTag = playerState->variantTag;
    CZClass::gwNodeSetNodeType(playerState->rootNode, playerState->variantTag.tags[0]);
    CZClass::gwNodeSetCellPickable(playerState->rootNode, 0);

    PlayerProbeSampleCandidateBuffer candidateBuffer;
    CZDisplayInstance::BuildPickCandidateListBelowPoint(
        g_Player_RuntimeDiScene,
        playerState->worldPos.x,
        500.0f,
        playerState->worldPos.z,
        &candidateBuffer
    );

    int bestCandidateIndex;
    int selectedImpactSlot;
    float taggedHeight;
    SelectProbeSampleHeightFromCandidates(
        &candidateBuffer,
        playerState->worldPos.y,
        &bestCandidateIndex,
        4.0f,
        playerState->amphibUnlocked == 0,
        &selectedImpactSlot,
        &taggedHeight
    );

    CZClass::gwNodeSetCellPickable(playerState->rootNode, 1);

    if (candidateBuffer.candidateCount > 0) {
        playerState->variantTag = candidateBuffer.entries[bestCandidateIndex].variantTag;

        CZNodePartial* const worldChild
            = CZClass::gwNodeGetWorldChild(candidateBuffer.entries[bestCandidateIndex].node);
        int nodeType;
        if (worldChild != 0) {
            nodeType = worldChild->nodeType;
        } else {
            const zTag4Partial candidateTag = candidateBuffer.entries[bestCandidateIndex].variantTag;
            nodeType = candidateTag.tags[0];
        }
        CZClass::gwNodeSetNodeType(playerState->rootNode, nodeType);

        if (updateRotation == 0) {
            return;
        }
        playerState->steerBasisRef = candidateBuffer.entries[bestCandidateIndex].surfaceNormal;
        zVec3 yawRelativeNormal = playerState->steerBasisRef;
        RebuildSteerBasisRawFromRef(saveState);
        zMath::Vec3RotateY(&yawRelativeNormal, &playerState->steerBasisRef, -playerState->restartYawRad);

        const float pitchAngleRad = (float)(asin(yawRelativeNormal.z));
        playerState->vehiclePitchRad = pitchAngleRad;
        const float rollAngleRad = (float)(asin(-yawRelativeNormal.x));
        playerState->vehicleRollRad = rollAngleRad;
        if (pitchAngleRad > 0.523599982f) {
            playerState->vehiclePitchRad = 0.523599982f;
        } else if (pitchAngleRad < -0.523599982f) {
            playerState->vehiclePitchRad = -0.523599982f;
        }

        CZObject3D::gwObject3DSetRotation(
            playerState->rootNode,
            playerState->vehiclePitchRad,
            playerState->restartYawRad,
            rollAngleRad
        );
    } else {
        CZClass::gwNodeSetNodeType(playerState->rootNode, 0xff);
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-clonetype6nodefromtemplateandrename
 * @recoil-artifact defines .text recoil:function:0x421a40: Player::CloneType6NodeFromTemplateAndRename
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: clone a type-6 template node into the runtime scene and give it a
 * new active runtime name.
 * Source owner: Player namespace bootstrap/save-state node creation cluster.
 * BN evidence: finds a type-6 template by name, uses network-enabled for both
 * clone options, clones the node, inserts it into g_Player_RuntimeDiScene,
 * renames it, activates it, and returns the clone or null. BN HLIL currently
 * folds the AddChildAtGrid status branch because the callee decompiles as
 * returning zero; assembly keeps the failure gate before rename.
 */
CZNodePartial* __fastcall CloneType6NodeFromTemplateAndRename(const char* templateName, const char* newName)
{
    CZNodePartial* const source = CZClass::FindByTypeAndName(6, templateName);
    CZNodePartial* child = 0;
    if (source != 0) {
        if (zOpt::GetNetworkEnabled() != 0) {
            child = CZUtil::CopyNodeWithCloneOptions(source, 1, 1);
        } else {
            child = CZUtil::CopyNodeWithCloneOptions(source, 0, 0);
        }
        if (child != 0) {
            if (CZWorld::AddChildAtGrid(g_Player_RuntimeDiScene, child) == 0) {
                if (CZClass::gwNodeSetName(child, newName) == 0) {
                    CZClass::gwNodeSetActive(child, 1);
                    return child;
                }
            }
        }
    }
    return 0;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-createfromnamesatpose
 * @recoil-artifact defines .text recoil:function:0x421ab0: Player::CreateFromNamesAtPose
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: create and link a player save state from template/object names at
 * the requested spawn pose.
 * Source owner: Player namespace bootstrap/save-state node creation cluster.
 * BN evidence: handles the network bft_00 special clone/rename path, sets the
 * 0x400000 clone-source node flag, allocates and appends a zUtil save state,
 * applies pose/yaw and aiNetId, links the root node, initializes common data,
 * registers local/net hit callbacks, binds modal states, fills destroyed
 * respawn FX, initializes spawn state, and increments the HUD mission stat for
 * non-local states. BN shows the object-name strcmp as an MSVC sbb/sbb idiom
 * and may render the 0x400000 flag as the image base symbol in HLIL.
 */
int __fastcall CreateFromNamesAtPose(
    const zVec3* spawnPos,
    float yawDeg,
    int aiNetId,
    const char* templateName,
    const char* objectName
)
{
    const int objectIsBft00 = strcmp(objectName, g_Player_NodeName_Bft00) == 0;
    CZNodePartial* rootNode;

    if (zOpt::GetNetworkEnabled() != 0 && objectIsBft00 != 0) {
        rootNode = CZClass::FindByTypeAndName(6, g_Player_NodeName_Bft00);
        if (rootNode != 0) {
            CZNodePartial* const networkClone = CZUtil::CopyNodeWithCloneOptions(rootNode, 1, 1);
            if (networkClone != 0) {
                CZClass::gwNodeSetName(networkClone, "bft_99");
            }

            rootNode->flags |= kPlayerNodeFlagNetworkBftCloneSource;
        }
    } else {
        rootNode = CZClass::FindByTypeAndName(6, objectName);
        if (rootNode == 0) {
            rootNode = CloneType6NodeFromTemplateAndRename(templateName, objectName);
        }
    }

    if (rootNode == 0) {
        return 0;
    }

    zUtil_SaveGameState* const saveState = new zUtil_SaveGameState;
    if (saveState != 0) {
        saveState->next = 0;
        if (g_PlayerSaveStateList.count == 0) {
            g_PlayerSaveStateList.head = saveState;
        } else {
            g_PlayerSaveStateList.tail->next = saveState;
        }
        g_PlayerSaveStateList.tail = saveState;
        saveState->next = 0;
        ++g_PlayerSaveStateList.count;
    }

    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    if (spawnPos != 0) {
        CZObject3D::gwObject3DSetPosition(rootNode, spawnPos->x, spawnPos->y, spawnPos->z);
        CZObject3D::gwObject3DSetRotation(rootNode, 0.0f, (float)(yawDeg * 0.01745329251994), 0.0f);
        playerState->aiNetId = aiNetId;
    }

    if (rootNode->listCountA == 0) {
        CZClass::AddChild(g_Player_RuntimeDiScene, rootNode);
    }

    playerState->rootNode = rootNode;
    InitStateFromNameAndMasterCommonData(saveState, objectName, templateName);

    if (objectIsBft00 != 0) {
        CZNode::SetDamageHitCallback(saveState, playerState->rootNode, (void*)(&EnterDestroyedState));
        g_OptCatalogDamageFeedbackTrackedNode = playerState->rootNode;
        g_Player_LocalFxOffsetWorldPtr = &playerState->fxOffsetWorld;
        CZCamera::SetTargetNode(playerState->rootNode);
        g_HudSensorTracker.SetTrackedSaveState(saveState);
        if (zOpt::GetNetworkEnabled() == 0 && OptCatalogIsDamageMaskEnabled() != 0) {
            CZNode::SetMaterialFlagBit9ForFlagBit0EntriesRecursive(playerState->rootNode, 1);
        }
    } else {
        if (strstr(objectName, "net") != 0) {
            CZNode::SetDamageHitCallback(
                saveState,
                playerState->rootNode,
                (void*)(&HitCallbackRecordNetContextAndTimedStatus)
            );
        } else {
            CZNode::SetDamageHitCallback(
                saveState,
                playerState->rootNode,
                (void*)(&HitCallbackRecordContextAndTimedStatus)
            );
        }
    }

    const int modalCount = saveState->playerState->masterCommonData->modalCount;
    for (int i = 0; i < modalCount; ++i) {
        PlayerModalState* const modalState = (PlayerModalState*)zUtilSaveGameStateListAllocAppend(saveState);
        BindModalStateFromMasterModalData(
            saveState,
            modalState,
            objectName,
            saveState->playerState->masterCommonData->modalNames[i]
        );
    }

    if (playerState->destroyedRespawnFxEntry == 0) {
        playerState->destroyedRespawnFxEntry = zEffectAnim::FindEntryByName(templateName);
    }

    InitSpawnStateFromPrimaryModalData(saveState);
    if (saveState != (zUtil_SaveGameState*)g_GameStateOrMapTable) {
        ++g_HudSensorTracker.missionStat1;
    }

    return 1;
}
} // namespace Player
namespace CZNode {
/**
 * @recoil-anchor recoil:anchor:battlesport.player.cznode-mask-extra-flags-recursive
 * @recoil-artifact defines .text recoil:function:0x421d60: CZNode::MaskExtraFlagsRecursive.
 * @recoil-match byte
 *
 * BN evidence: fastcall self/mask, auxFlags at 0x28, signed
 * listCountB at 0x5c, listB at 0x60, recursive self-call only, and no
 * global data references.
 * Purpose: AND a mask into auxFlags across a node's child-list subtree.
 */
void __fastcall MaskExtraFlagsRecursive(CZNodePartial* self, int mask)
{
    self->auxFlags &= mask;

    for (int i = 0; i < self->listCountB; ++i) {
        MaskExtraFlagsRecursive(self->listB[i], mask);
    }
}
} // namespace CZNode
namespace CZNode {
/**
 * @recoil-anchor recoil:anchor:battlesport.player.cznode-propagate-extra-flags-recursive
 * @recoil-artifact defines .text recoil:function:0x421da0: CZNode::PropagateExtraFlagsRecursive.
 * @recoil-match byte
 *
 * BN evidence: fastcall self/flags, auxFlags at 0x28, signed
 * listCountB at 0x5c, listB at 0x60, recursive self-call only, and no
 * global data references.
 * Purpose: OR auxFlags into each node in a child-list subtree.
 */
void __fastcall PropagateExtraFlagsRecursive(CZNodePartial* self, int flags)
{
    self->auxFlags |= flags;

    for (int i = 0; i < self->listCountB; ++i) {
        PropagateExtraFlagsRecursive(self->listB[i], flags);
    }
}
} // namespace CZNode
namespace CZNode {
/**
 * @recoil-anchor recoil:anchor:battlesport.player.cznode-propagate-flags-recursive
 * @recoil-artifact defines .text recoil:function:0x421de0: CZNode::PropagateFlagsRecursive.
 * @recoil-match byte
 *
 * BN evidence: fastcall self/flags, flags at 0x24, signed listCountB at
 * 0x5c, listB at 0x60, recursive self-call only, and no global data
 * references.
 * Purpose: OR normal node flags into each node in a child-list subtree.
 */
void __fastcall PropagateFlagsRecursive(CZNodePartial* self, int flags)
{
    self->flags |= flags;

    for (int i = 0; i < self->listCountB; ++i) {
        PropagateFlagsRecursive(self->listB[i], flags);
    }
}
} // namespace CZNode
namespace zReader {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-zreader-buildresolvedparentdir
 * @recoil-artifact defines .text recoil:function:0x421e20: zReader::BuildResolvedParentDir.
 * @recoil-match byte
 *
 * Purpose: build the parent directory for the currently resolved ZRDR path.
 */
int __fastcall BuildResolvedParentDir(const char* filename, char* outParentDir)
{
    char fullPath[0x104];
    filename = FindFile(filename, 0);
    _fullpath(fullPath, filename, sizeof(fullPath));
    char drive[3];
    char dir[0x100];
    char baseName[0x100];
    char ext[0x100];
    _splitpath(fullPath, drive, dir, baseName, ext);
    return sprintf(outParentDir, "%s%s", drive, dir);
}
} // namespace zReader
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-createfromnamesatposegetstate
 * @recoil-artifact defines .text recoil:function:0x421ea0: Player::CreateFromNamesAtPoseGetState
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: create a player from names and return the newly appended save-state
 * tail.
 * Source owner: Player namespace bootstrap/save-state node creation cluster.
 * BN evidence: calls CreateFromNamesAtPose(spawnPos, 0, yawDeg, templateName,
 * objectName), then returns g_PlayerSaveStateList.tail on success and null on
 * failure. BN leaves the MSVC neg/sbb/and success-mask expression. Retail
 * keeps templateName in edx until after yawDeg is loaded into eax.
 */
zUtil_SaveGameState* __fastcall
CreateFromNamesAtPoseGetState(const zVec3* spawnPos, const char* templateName, float yawDeg, const char* objectName)
{
    const char* const object = objectName;
    const int created = CreateFromNamesAtPose(spawnPos, yawDeg, 0, templateName, object);
    // Always capture tail so VC5 emits retail's neg/sbb/and success mask.
    zUtil_SaveGameState* const tail = g_PlayerSaveStateList.tail;
    return created == 0 ? 0 : tail;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-buildcollisionpointsfrommodel
 * @recoil-artifact defines .text recoil:function:0x421ed0: Player::BuildCollisionPointsFromModel
 * @recoil-match byte
 *
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: read collide00..collide11 nodes from the model, deactivate them,
 * and store the reordered probe points in the modal data.
 * Source owner: Player save-state/bootstrap record-global subsystem, not a
 * C++ Player class.
 */
int __fastcall BuildCollisionPointsFromModel(zUtil_SaveGameState* saveState, CZNodePartial* modelNode)
{
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;
    zVec3 collisionPoints[12];

    for (int i = 0; i < 12; ++i) {
        char nodeName[0x50];
        sprintf(nodeName, "collide%02d", i);
        CZNodePartial* const collisionNode = CZClass::FindSubNodeByName(modelNode, nodeName);
        if (collisionNode == 0) {
            return 0;
        }

        CZObject3D::gwObject3DGetPosition(
            collisionNode,
            &collisionPoints[i].x,
            &collisionPoints[i].y,
            &collisionPoints[i].z
        );
        CZClass::gwNodeSetActive(collisionNode, 0);
    }

    masterModalData->probePoints[0] = collisionPoints[0];
    masterModalData->probePoints[1] = collisionPoints[1];
    masterModalData->probePoints[2] = collisionPoints[2];
    masterModalData->probePoints[3] = collisionPoints[6];
    masterModalData->probePoints[4] = collisionPoints[7];
    masterModalData->probePoints[5] = collisionPoints[8];
    masterModalData->probePoints[6] = collisionPoints[3];
    masterModalData->probePoints[7] = collisionPoints[4];
    masterModalData->probePoints[8] = collisionPoints[5];
    masterModalData->probePoints[9] = collisionPoints[9];
    masterModalData->probePoints[10] = collisionPoints[10];
    masterModalData->probePoints[11] = collisionPoints[11];
    masterModalData->probePointCount = 12;
    return 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-buildsupportpointsfrommodel
 * @recoil-artifact defines .text recoil:function:0x4220f0: Player::BuildSupportPointsFromModel
 * @recoil-match byte
 *
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: read support00..support03 nodes from the model, deactivate them,
 * and cache their positions in modal probe-point slots 15..18.
 * Source owner: Player save-state/bootstrap record-global subsystem, not a
 * C++ Player class.
 */
int __fastcall BuildSupportPointsFromModel(zUtil_SaveGameState* saveState, CZNodePartial* modelNode)
{
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;

    for (int i = 0; i < 4; ++i) {
        char nodeName[0x50];
        sprintf(nodeName, "support%02d", i);
        CZNodePartial* const supportNode = CZClass::FindSubNodeByName(modelNode, nodeName);
        if (supportNode == 0) {
            return 0;
        }

        zVec3* const supportPoint = &masterModalData->probePoints[15 + i];
        CZObject3D::gwObject3DGetPosition(supportNode, &supportPoint->x, &supportPoint->y, &supportPoint->z);
        CZClass::gwNodeSetActive(supportNode, 0);
    }

    return 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-loadmastercommondatafromnode
 * @recoil-artifact defines .text recoil:function:0x422170: Player::LoadMasterCommonDataFromNode.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Source owner: battlesport_gameplay.player_master_zrd_record_loaders.
 * BN evidence: current decompilation shows fastcall ECX=PlayerMasterCommonData,
 * EDX=vehicle zReader node, stack vehicleName, direct type-4 child-array field
 * reads for the ZRD records, optional common_mode child lookups, sample/pickup
 * provider calls, and PlayerMasterWeaponSpec allocation/linking.
 * Purpose: load one vehicle common-mode master data record from vehicle ZRD.
 */
void __fastcall
LoadMasterCommonDataFromNode(PlayerMasterCommonData* commonData, zReader::Node* vehicleNode, const char* vehicleName)
{
    strcpy(commonData->vehicleName, vehicleName);

    commonData->modalCount = ((PlayerZrdArrayCount(vehicleNode) - 1) / 2) - 1;

    zReader::Node* const commonModeNode = zRdrGetNode(vehicleNode, g_Player_ConfigNode_CommonMode);
    zReader::Node* node = zRdrGetNode(commonModeNode, g_Player_NodeName_Nanite);
    if (node != 0) {
        commonData->naniteBuildRate = PlayerZrdArrayInt(node, 1);
        commonData->naniteMaxLevel = PlayerZrdArrayInt(node, 2);
    } else {
        commonData->naniteBuildRate = 0;
        commonData->naniteMaxLevel = 0;
    }

    zReader::Node* const soundsNode = zRdrGetNode(commonModeNode, g_Player_NodeName_Sounds);
    if (soundsNode != 0) {
        PlayerLoadSoundSample(soundsNode, g_Player_NodeName_WeaponUp, &commonData->sfxWeaponUp[0]);
        PlayerLoadSoundSample(soundsNode, g_Player_NodeName_WeaponSelect, &commonData->sfxWeaponUp[2]);
        PlayerLoadSoundSample(soundsNode, g_Player_NodeName_Pinging, &commonData->sfxWeaponUp[3]);
    }

    node = zRdrGetNode(commonModeNode, g_Player_NodeName_Activation);
    if (node != 0) {
        const float activationRange = PlayerZrdArrayFloat(node, 1);
        commonData->activationRangeSq = activationRange * activationRange;
    } else {
        commonData->activationRangeSq = 100.0f * 100.0f;
    }

    node = zRdrGetNode(commonModeNode, "not_pursuit_dwell");
    if (node != 0) {
        commonData->notPursuitDwellTime = PlayerZrdArrayFloat(node, 1);
    } else {
        commonData->notPursuitDwellTime = 3.0f;
    }

    node = zRdrGetNode(commonModeNode, "return_range");
    if (node != 0) {
        const float returnRange = PlayerZrdArrayFloat(node, 1);
        commonData->returnRangeSq = returnRange * returnRange;
    } else {
        commonData->returnRangeSq = 250.0f * 250.0f;
    }

    node = zRdrGetNode(commonModeNode, g_Player_NodeName_StartAnims);
    if (node != 0) {
        PlayerCopyZrdArrayString(commonData->startAnimsName, node, 1);
    }

    node = zRdrGetNode(commonModeNode, g_Player_NodeName_CamBack);
    if (node != 0) {
        commonData->cameraBackOffset.x = PlayerZrdArrayBase(node)[1].value.nodes[1].value.f32;
        commonData->cameraBackOffset.y = PlayerZrdArrayBase(node)[1].value.nodes[2].value.f32;
        commonData->cameraBackOffset.z = PlayerZrdArrayBase(node)[1].value.nodes[3].value.f32;
        commonData->camback1.x = PlayerZrdArrayBase(node)[2].value.nodes[1].value.f32;
        commonData->camback1.y = PlayerZrdArrayBase(node)[2].value.nodes[2].value.f32;
        commonData->camback1.z = PlayerZrdArrayBase(node)[2].value.nodes[3].value.f32;
        commonData->camback2.x = PlayerZrdArrayBase(node)[3].value.nodes[1].value.f32;
        commonData->camback2.y = PlayerZrdArrayBase(node)[3].value.nodes[2].value.f32;
        commonData->camback2.z = PlayerZrdArrayBase(node)[3].value.nodes[3].value.f32;
    } else {
        commonData->cameraBackOffset.x = 0.0f;
        commonData->cameraBackOffset.y = 4.0f;
        commonData->cameraBackOffset.z = 9.0f;
        commonData->camback1.x = 0.0f;
        commonData->camback1.y = 3.5f;
        commonData->camback1.z = 2.25f;
        commonData->camback2.x = 0.0f;
        commonData->camback2.y = 2.25f;
        commonData->camback2.z = 2.25f;
    }

    node = zRdrGetNode(commonModeNode, g_Player_NodeName_AimY);
    if (node != 0) {
        commonData->aimYawRate = PlayerZrdArrayFloat(node, 1);
        commonData->aimYawMax = PlayerZrdArrayFloat(node, 2);
    } else {
        commonData->aimYawRate = 3.0f;
        commonData->aimYawMax = 2.0f;
    }

    node = zRdrGetNode(commonModeNode, g_Player_NodeName_CameraUdSwing);
    if (node != 0) {
        commonData->cameraUdSwing[0] = PlayerZrdArrayFloat(node, 1);
        commonData->cameraUdSwing[1] = PlayerZrdArrayFloat(node, 2);
        commonData->cameraUdSwing[2] = PlayerZrdArrayFloat(node, 3);
        commonData->cameraUdSwing[3] = PlayerZrdArrayFloat(node, 4);
    } else {
        commonData->cameraUdSwing[0] = 5.5f;
        commonData->cameraUdSwing[1] = 2.5f;
        commonData->cameraUdSwing[2] = 0.0f;
        commonData->cameraUdSwing[3] = 0.0f;
    }

    node = zRdrGetNode(commonModeNode, g_Player_NodeName_TrackSwitch);
    if (node != 0) {
        commonData->trackSwitchDist0 = PlayerZrdArrayFloat(node, 1);
        commonData->trackSwitchDist1 = PlayerZrdArrayFloat(node, 2);
        commonData->trackSwitchDist2 = PlayerZrdArrayFloat(node, 3);
    } else {
        commonData->trackSwitchDist0 = 10000.0f;
        commonData->trackSwitchDist1 = 10000.0f;
        commonData->trackSwitchDist2 = 10000.0f;
    }

    zReader::Node* const healthNode = zRdrGetNode(commonModeNode, g_Player_NodeName_Health);
    if (healthNode != 0) {
        if (zOpt::GetNetworkEnabled() != 0) {
            commonData->maxHealth = PlayerZrdArrayFloat(healthNode, 2);
        } else {
            commonData->maxHealth = PlayerZrdArrayFloat(healthNode, 1);
        }
    } else {
        commonData->maxHealth = 100.0f;
    }
    commonData->invMaxHealth = 1.0f / commonData->maxHealth;

    zReader::Node* const pickupsNode = zRdrGetNode(commonModeNode, g_Player_NodeName_Pickups);
    if (pickupsNode != 0) {
        PickupType::FindByLogicalName(PlayerZrdArrayString(pickupsNode, 1), &commonData->pickupType);
        commonData->pickupCapacity = PlayerZrdArrayInt(pickupsNode, 2);
    } else {
        commonData->pickupType = 0;
        commonData->pickupCapacity = 0;
    }

    zReader::Node* const weaponsNode = zRdrGetNode(commonModeNode, g_Player_NodeName_Weapons);
    if (weaponsNode == 0) {
        return;
    }

    commonData->weaponNodeCount = PlayerZrdArrayCount(weaponsNode) - 1;
    for (int index = 0; index < commonData->weaponNodeCount; ++index) {
        PlayerMasterWeaponSpec* const weaponSpec
            = (PlayerMasterWeaponSpec*)(::operator new(sizeof(PlayerMasterWeaponSpec)));
        memset(weaponSpec, 0, sizeof(PlayerMasterWeaponSpec));
        if (weaponSpec != 0) {
            weaponSpec->next = 0;
            if (commonData->weaponSpecCount == 0) {
                commonData->weaponSpecHead = weaponSpec;
            } else {
                commonData->weaponSpecTail->next = weaponSpec;
            }
            commonData->weaponSpecTail = weaponSpec;
            weaponSpec->next = 0;
            ++commonData->weaponSpecCount;
        }

        strcpy(weaponSpec->optCatalogName, PlayerZrdArrayBase(weaponsNode)[index + 1].value.nodes[1].value.str);
        weaponSpec->missionRequirementOrGateId = PlayerZrdArrayBase(weaponsNode)[index + 1].value.nodes[2].value.i32;
        weaponSpec->mountLayoutFlags = PlayerZrdArrayBase(weaponsNode)[index + 1].value.nodes[3].value.i32;
        weaponSpec->startAmmoOrCharge = (float)(PlayerZrdArrayBase(weaponsNode)[index + 1].value.nodes[4].value.i32);
        weaponSpec->dispatchRepeatDelay = PlayerZrdArrayBase(weaponsNode)[index + 1].value.nodes[5].value.f32;
        weaponSpec->aiAttackRangeMin = PlayerZrdArrayBase(weaponsNode)[index + 1].value.nodes[6].value.f32;
        weaponSpec->aiAttackRangeMax = PlayerZrdArrayBase(weaponsNode)[index + 1].value.nodes[7].value.f32;
        weaponSpec->fireSlotRecoilFlags = PlayerZrdArrayBase(weaponsNode)[index + 1].value.nodes[8].value.i32;
        weaponSpec->initialHardpointSelectState = PlayerZrdArrayBase(weaponsNode)[index + 1].value.nodes[9].value.i32;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-loadmastermodaldatafromnode
 * @recoil-artifact defines .text recoil:function:0x4226d0: Player::LoadMasterModalDataFromNode.
 *
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Source owner: battlesport_gameplay.player_master_zrd_record_loaders.
 * BN evidence: current decompilation shows fastcall ECX=PlayerMasterModalData,
 * EDX=modal zReader node, stack modalName, direct type-4 child-array field
 * reads for modal scalar/list records, two-character mode dispatch, repeated
 * point/FX/wave/sound loader patterns, and no authored globals touched.
 * Purpose: load one modal master data record from a vehicle modal ZRD node.
 */
void __fastcall
LoadMasterModalDataFromNode(PlayerMasterModalData* modalData, zReader::Node* modalNode, const char* modalName)
{
    strcpy(modalData->modalName, modalName);

    zReader::Node* node = zRdrGetNode(modalNode, g_Player_ConfigNode_Mode);
    if (node != 0) {
        PlayerCopyZrdArrayString(modalData->modeName, node, 1);
        if (strncmp(modalData->modeName, g_Player_ConfigNode_Basic, 2) == 0) {
            modalData->masterType = 0;
        } else if (strncmp(modalData->modeName, g_Player_ConfigValue_MasterTypeTrack, 2) == 0) {
            modalData->masterType = 3;
        } else if (strncmp(modalData->modeName, g_Player_ConfigValue_MasterTypeHover, 2) == 0) {
            modalData->masterType = 4;
        } else if (strncmp(modalData->modeName, g_Player_ConfigValue_MasterTypeAmphib, 2) == 0) {
            modalData->masterType = 5;
        } else if (strncmp(modalData->modeName, g_Player_ConfigValue_MasterTypeSub, 2) == 0) {
            modalData->masterType = 2;
        } else if (strncmp(modalData->modeName, g_Player_ConfigValue_MasterTypeFly, 2) == 0) {
            modalData->masterType = 1;
        }
    } else {
        strcpy(modalData->modeName, g_Player_ConfigValue_MasterTypeUnknown);
        modalData->masterType = 0;
    }

    PlayerLoadModalPointList(
        zRdrGetNode(modalNode, g_Player_ConfigKey_Platform),
        &modalData->probePoints[15],
        &modalData->platformPointCount
    );
    PlayerLoadModalPointList(
        zRdrGetNode(modalNode, g_Player_ConfigKey_Collision),
        modalData->probePoints,
        &modalData->probePointCount
    );

    node = zRdrGetNode(modalNode, g_Player_ConfigKey_Rates);
    if (node != 0) {
        modalData->accelRate = PlayerZrdArrayFloat(node, 1);
        modalData->maxSpeed = PlayerZrdArrayFloat(node, 2);
    } else {
        modalData->accelRate = 10.0f;
        modalData->maxSpeed = 30.0f;
    }

    node = zRdrGetNode(modalNode, g_Player_ConfigKey_Friction);
    if (node != 0) {
        modalData->frictionStatic = PlayerZrdArrayFloat(node, 1);
        modalData->frictionDynamic = PlayerZrdArrayFloat(node, 2);
        modalData->frictionSlide = PlayerZrdArrayFloat(node, 3);
    } else {
        modalData->frictionStatic = 10000.0f;
        modalData->frictionDynamic = 10.0f;
        modalData->frictionSlide = 0.0f;
    }

    node = zRdrGetNode(modalNode, g_Player_ConfigKey_Stopping);
    if (node != 0) {
        modalData->stoppingForce = PlayerZrdArrayFloat(node, 1);
    } else {
        modalData->stoppingForce = 8.0f;
    }

    if (modalData->frictionDynamic > modalData->frictionStatic) {
        modalData->frictionDynamic = modalData->frictionStatic * 0.899999976f;
    }

    node = zRdrGetNode(modalNode, g_Player_ConfigKey_QuicksandSlowdown);
    if (node != 0) {
        modalData->quicksandSlowdown = PlayerZrdArrayFloat(node, 1);
    } else {
        modalData->quicksandSlowdown = 0.899999976f;
    }

    node = zRdrGetNode(modalNode, g_Player_ConfigKey_LavaSlowdown);
    if (node != 0) {
        modalData->lavaSlowdown = PlayerZrdArrayFloat(node, 1);
    } else {
        modalData->lavaSlowdown = 0.800000012f;
    }

    node = zRdrGetNode(modalNode, g_Player_ConfigKey_Turns);
    if (node != 0) {
        modalData->yawAccel = PlayerZrdArrayFloat(node, 1);
        modalData->yawRateMax = PlayerZrdArrayFloat(node, 2);
    } else {
        modalData->yawAccel = 0.600000024f;
        modalData->yawRateMax = 2.0f;
    }

    node = zRdrGetNode(modalNode, g_Player_ConfigKey_TurnDamping);
    if (node != 0) {
        modalData->yawDamping = PlayerZrdArrayFloat(node, 1);
    } else {
        modalData->yawDamping = 30.0f;
    }

    node = zRdrGetNode(modalNode, g_Player_ConfigKey_RateDamping);
    if (node != 0) {
        modalData->rateDampingAccel = PlayerZrdArrayFloat(node, 1);
        modalData->rateDampingDecel = PlayerZrdArrayFloat(node, 2);
    } else {
        modalData->rateDampingAccel = 30.0f;
        modalData->rateDampingDecel = 30.0f;
    }

    node = zRdrGetNode(modalNode, g_Player_ConfigKey_AccelDamping);
    if (node != 0) {
        modalData->aDamping = PlayerZrdArrayFloat(node, 1);
    } else {
        modalData->aDamping = 8.0f;
    }

    node = zRdrGetNode(modalNode, g_Player_ConfigKey_AltControl);
    if (node != 0) {
        modalData->hoverLiftDampingRate = PlayerZrdArrayFloat(node, 1);
        modalData->hoverLiftScale = PlayerZrdArrayFloat(node, 2);
        modalData->hoverNormalLerpRate = PlayerZrdArrayFloat(node, 3);
    } else {
        modalData->hoverLiftDampingRate = -10.0f;
        modalData->hoverLiftScale = 0.800000012f;
        modalData->hoverNormalLerpRate = -3.0f;
    }

    node = zRdrGetNode(modalNode, g_Player_ConfigKey_Mass);
    if (node != 0) {
        modalData->mass = PlayerZrdArrayFloat(node, 1);
    } else {
        modalData->mass = 1.0f;
    }
    modalData->invMass = 1.0f / modalData->mass;

    node = zRdrGetNode(modalNode, g_Player_ConfigKey_GunPitch);
    if (node != 0) {
        modalData->gunPitchMin = PlayerZrdArrayFloat(node, 1);
        modalData->gunPitchRate = PlayerZrdArrayFloat(node, 2);
    } else {
        modalData->gunPitchMin = -0.2588f;
        modalData->gunPitchRate = 0.5f;
    }

    PlayerLoadModalWaveParams(modalData, modalNode, g_Player_ConfigKey_AmphibWave);
    PlayerLoadModalWaveParams(modalData, modalNode, g_Player_ConfigKey_HoverWave);
    PlayerLoadModalWaveParams(modalData, modalNode, g_Player_ConfigKey_SubWave);

    node = zRdrGetNode(modalNode, g_Player_NodeName_ModeAlt);
    if (node != 0) {
        modalData->modeAltTransitionTime = PlayerZrdArrayFloat(node, 1);
    } else {
        modalData->modeAltTransitionTime = 2.0f;
    }

    node = zRdrGetNode(modalNode, g_Player_NodeName_ChassisSmooth);
    if (node != 0) {
        modalData->chassisSmoothFactor = (float)(fabs(PlayerZrdArrayFloat(node, 1)));
    } else {
        modalData->chassisSmoothFactor = 0.0f;
    }

    node = zRdrGetNode(modalNode, g_Player_NodeName_ChassisPitch);
    if (node != 0) {
        modalData->chassisPitchRate = PlayerZrdArrayFloat(node, 1);
        modalData->chassisPitchMax = PlayerZrdArrayFloat(node, 2);
        modalData->chassisPitchDamping = (float)(fabs(PlayerZrdArrayFloat(node, 3)));
    } else {
        modalData->chassisPitchRate = 0.0f;
        modalData->chassisPitchMax = 0.0f;
        modalData->chassisPitchDamping = 0.0f;
    }

    node = zRdrGetNode(modalNode, g_Player_NodeName_ChassisRoll);
    if (node != 0) {
        modalData->chassisRollRate = PlayerZrdArrayFloat(node, 1);
        modalData->chassisRollMax = PlayerZrdArrayFloat(node, 2);
        modalData->chassisRollDamping = (float)(fabs(PlayerZrdArrayFloat(node, 3)));
    } else {
        // Retail code clears the pitch slots here when chas_roll is absent.
        modalData->chassisPitchRate = 0.0f;
        modalData->chassisPitchMax = 0.0f;
        modalData->chassisPitchDamping = 0.0f;
    }

    node = zRdrGetNode(modalNode, g_Player_NodeName_CollisionDamage);
    if (node != 0) {
        modalData->collisionDampingA = PlayerZrdArrayFloat(node, 1);
        modalData->collisionDampingB = PlayerZrdArrayFloat(node, 2);
    } else {
        modalData->collisionDampingA = 0.5f;
        modalData->collisionDampingB = 0.150000006f;
    }

    PlayerLoadModalFxList(modalNode, g_Player_NodeName_T2AAnims, modalData->fxList_fromTrackToAmphib);
    PlayerLoadModalFxList(modalNode, g_Player_NodeName_A2TAnims, modalData->fxList_fromAmphibToTrack);
    PlayerLoadModalFxList(modalNode, g_Player_NodeName_T2HAnims, modalData->fxList_fromTrackToHover);
    PlayerLoadModalFxList(modalNode, g_Player_NodeName_H2TAnims, modalData->fxList_fromHoverToTrack);
    PlayerLoadModalFxList(modalNode, g_Player_NodeName_A2HAnims, modalData->fxList_fromAmphibToHover);
    PlayerLoadModalFxList(modalNode, g_Player_NodeName_H2AAnims, modalData->fxList_fromHoverToAmphib);
    PlayerLoadModalFxList(modalNode, g_Player_NodeName_A2SAnims, modalData->fxList_fromAmphibToSub);
    PlayerLoadModalFxList(modalNode, g_Player_NodeName_S2AAnims, modalData->fxList_fromSubToAmphib);

    zReader::Node* const soundsNode = zRdrGetNode(modalNode, g_Player_NodeName_Sounds);
    if (soundsNode == 0) {
        return;
    }

    PlayerLoadSoundSample(soundsNode, g_Player_NodeName_Idle, &modalData->sfxEngine[2]);
    PlayerLoadSoundSample(soundsNode, g_Player_NodeName_Skid, &modalData->sfxEngine[3]);
    PlayerLoadSoundSample(soundsNode, g_Player_NodeName_Engine, &modalData->sfxEngine[0]);
    PlayerLoadSoundSample(soundsNode, g_Player_NodeName_External, &modalData->sfxEngine[1]);
    PlayerLoadSoundSample(soundsNode, g_Player_NodeName_Collide, &modalData->sfxCollide);
    PlayerLoadSoundSample(soundsNode, g_Player_NodeName_Land, &modalData->sfxLand);

    node = zRdrGetNode(soundsNode, g_Player_NodeName_PitchScale);
    if (node != 0) {
        modalData->sfxPitchScale = PlayerZrdArrayFloat(node, 1);
    }

    node = zRdrGetNode(soundsNode, g_Player_NodeName_VolumeScale);
    if (node != 0) {
        modalData->sfxVolumeScale = PlayerZrdArrayFloat(node, 1);
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-extractvehiclenamefromaivname
 * @recoil-artifact defines .text recoil:function:0x423150: Player::ExtractVehicleNameFromAivName.
 * @recoil-match byte
 *
 *
 * Purpose: copy the vehicle-name prefix from an AIV name until the numeric
 * suffix separator.
 */
void __fastcall ExtractVehicleNameFromAivName(const char* aivName, char* outVehicleName)
{
    int outLen = 0;
    outVehicleName[0] = '\0';

    // Write the next-byte terminator before the for-loop advances the index.
    // VC5 then retains the input base and output index across isdigit,
    // reproducing the retail loop; the cursor form compiles differently.
    for (; aivName[outLen] != '\0'; ++outLen) {
        if (aivName[outLen] == '_' && isdigit(aivName[outLen + 1]) != 0) {
            break;
        }

        outVehicleName[outLen] = aivName[outLen];
        outVehicleName[outLen + 1] = '\0';
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-refreshhudfromstate
 * @recoil-artifact defines .text recoil:function:0x4231b0: Player::RefreshHudFromState.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: refresh the HUD weapon, health, mode, damage, and status displays
 * from the current player save-state fields.
 */
void __fastcall RefreshHudFromState(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    zUtil_PlayerStateStorage* const localPlayerState = ((zUtil_SaveGameState*)(g_GameStateOrMapTable))->playerState;
    HudUiMgrSensor::SetShieldMessageRatio(
        playerState->statusMeterValue / localPlayerState->masterCommonData->maxHealth
    );
    HudUiMgr::SetNanitePanelCount(playerState->nanitePanelLevel);

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        PlayerAltWeaponBank& bank = playerState->altWeaponBanks[bankIndex];
        PlayerGunFireController& left = bank.controllerA;
        PlayerGunFireController& right = bank.controllerB;

        if ((unsigned char)((unsigned int)(left.flags) >> 2) & 1) {
            if (left.ammoOrCharge != 0.0f) {
                HudUiMessage::SelectVariantDisplay(bankIndex, 0);
                HudUiMessage::SetValueIfOwnerMatches(bankIndex, 0, left.ammoOrCharge);
                bank.selectedSide = 0;
                if ((unsigned char)((unsigned int)(right.flags) >> 2) & 1) {
                    HudUiMessage::ApplySideImageSwap(bankIndex, 1);
                }
            } else if ((right.flags & 4) != 0 && right.ammoOrCharge != 0.0f) {
                HudUiMessage::SelectVariantDisplay(bankIndex, 1);
                HudUiMessage::SetValueIfOwnerMatches(bankIndex, 1, right.ammoOrCharge);
                bank.selectedSide = 1;
                HudUiMessage::ApplySideImageSwap(bankIndex, 0);
            }
        } else if ((unsigned char)((unsigned int)(right.flags) >> 2) & 1) {
            HudUiMessage::SelectVariantDisplay(bankIndex, 1);
            HudUiMessage::SetValueIfOwnerMatches(bankIndex, 1, right.ammoOrCharge);
            bank.selectedSide = 1;
        } else {
            HudUiMessage::ClearDisplay(bankIndex);
            if (left.ammoOrCharge != 0.0f) {
                HudUiMessage::SetValueIfOwnerMatches(bankIndex, 0, left.ammoOrCharge);
            } else if (right.ammoOrCharge != 0.0f) {
                HudUiMessage::SetValueIfOwnerMatches(bankIndex, 0, right.ammoOrCharge);
            }
        }
    }

    HudUiMessage::UpdateSelectedWeaponDisplay(0, 0, 0.0f);

    PlayerGunFireController* const activeAltGunController = playerState->activeAltGunController;
    HudUiMessage::UpdateSelectedWeaponDisplay(
        activeAltGunController->weaponBankIndex,
        activeAltGunController->weaponSideIndex,
        activeAltGunController->ammoOrCharge
    );

    PlayerGunFireController* const activePrimaryGunController = playerState->activePrimaryGunController;
    HudUiMessage::UpdateSelectedWeaponDisplay(
        activePrimaryGunController->weaponBankIndex,
        activePrimaryGunController->weaponSideIndex,
        activePrimaryGunController->ammoOrCharge
    );

    HudUiMgr::SetModeCounterState(1, playerState->amphibUnlocked != 0 ? 1 : 0);
    HudUiMgr::SetModeCounterState(2, playerState->hoverUnlocked != 0 ? 1 : 0);
    HudUiMgr::SetModeCounterState(3, playerState->subUnlocked != 0 ? 1 : 0);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-ismissionprobetype1enabledbyid
 * @recoil-artifact defines .text recoil:function:0x423380: Player::IsMissionProbeType1EnabledById
 * @recoil-match byte
 *
 * Purpose: identify the mission probe ids that enable type-1 mission probe handling.
 * Source owner: standalone mission probe type predicate leaf, not the Player
 * C++ class.
 * Evidence: retail body is a pure integer predicate over ids 9, 11, 12, and
 * 13 with no calls, globals, object state, or table dispatch.
 */
int __fastcall IsMissionProbeType1EnabledById(int missionId)
{
    if (missionId == 9) {
        return 1;
    }
    if (missionId == 11) {
        return 1;
    }
    if (missionId == 12) {
        return 1;
    }
    return missionId == 13;
}
} // namespace Player
/**
 * @recoil-anchor recoil:anchor:battlesport.player.cplayer-underwater-fx-pass3-ui-apply-pass3
 * @recoil-artifact defines .text recoil:function:0x423440: CPlayerUnderwaterFxPass3Ui::ApplyPass3.
 * @recoil-match byte
 *
 * Purpose: applies the underwater blue-tint pass to the active pass-3 input
 * rectangle through the recovered ApplyPass3 virtual slot.
 */
void CPlayerUnderwaterFxPass3Ui::ApplyPass3()
{
    zVideo_FxSurface::ApplyBlueTintRect((zVidRect32*)(clipRectOrNull));
}
/**
 * @recoil-anchor recoil:anchor:battlesport.player.cplayer-projectile-camera-fx-pass3-ui-apply-pass3
 * @recoil-artifact defines .text recoil:function:0x423450: CPlayerProjectileCameraFxPass3Ui::ApplyPass3.
 * @recoil-match byte
 *
 * Purpose: applies the projectile-camera green-mask pass to the active pass-3
 * input rectangle through the recovered ApplyPass3 virtual slot.
 */
void CPlayerProjectileCameraFxPass3Ui::ApplyPass3()
{
    zVideo_FxSurface::ApplyGreenMaskRect((zVidRect32*)(clipRectOrNull));
}
