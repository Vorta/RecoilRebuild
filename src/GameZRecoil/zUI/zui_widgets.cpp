#include "recoil/Mfc42Abi.h"

#include "GameZRecoil/zHud/zhud_ui.h"

#include "Battlesport/briefing.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud_ui_net_game_setup.h"
#include "Battlesport/recoil_state_credits.h"
#include "Battlesport/hud.h"
#include "Battlesport/player.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zClass/cls_stubs.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zVideo/zvid_fx_pass3.h"

#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "Battlesport/turret.h"
#include "GameZRecoil/zUtil/zbd.h"

#include <cctype>
#include <cstdarg>
#include <math.h>
#include <new>
#if defined(_MSC_VER) && _MSC_VER < 1200
#include <vector>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>


namespace {
const int ZOPT_GRAPHICS_PERSPECTIVE = 8;
const int ZOPT_GRAPHICS_GLOBAL_LIGHT = 0x10;
const int ZVID_HW_MODE_SOFTWARE = 0;
const float ZSND_CD_VOLUME_TO_NORMALIZED = 1.52590219e-05f;
const float ZSND_CD_NORMALIZED_TO_VOLUME = 65535.0f;

struct HudReticleAttachStatePartial {
    unsigned char unknown_00[0x0c];
    zClass_NodePartial *projectileNode;
};

struct HudReticleAltGunControllerPartial {
    OptCatalogEntryDef *optCatalogEntry;
    unsigned char unknown_04[0x24];
    HudReticleAttachStatePartial *attachState;
};

struct HudReticlePlayerStatePartial {
    unsigned char unknown_000[0x58c];
    int cameraState;
    unsigned char unknown_590[0x54];
    HudReticleAltGunControllerPartial *activeAltGunController;
    unsigned char unknown_5e8[0x8e8];
    zClass_NodePartial *rootNode;
};

RECOIL_STATIC_ASSERT(offsetof(HudReticleAttachStatePartial, projectileNode) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(HudReticleAltGunControllerPartial, attachState) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(HudReticlePlayerStatePartial, cameraState) == 0x58c);
RECOIL_STATIC_ASSERT(offsetof(HudReticlePlayerStatePartial, activeAltGunController) == 0x5e4);
RECOIL_STATIC_ASSERT(offsetof(HudReticlePlayerStatePartial, rootNode) == 0xed0);

} // namespace

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-ztimedtask-activecount
 * @recoil-artifact defines .data recoil:data:0x56bd30: g_zTimedTask_ActiveCount.
 * Purpose: preserve the recovered HUD global storage for g_zTimedTask_ActiveCount.
 */
int g_zTimedTask_ActiveCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-ztimedtask-activehead
 * @recoil-artifact defines .data recoil:data:0x56bd34: g_zTimedTask_ActiveHead.
 * Purpose: preserve the recovered HUD global storage for g_zTimedTask_ActiveHead.
 */
zTimedTask *g_zTimedTask_ActiveHead = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-ztimedtask-activetail
 * @recoil-artifact defines .data recoil:data:0x56bd38: g_zTimedTask_ActiveTail.
 * Purpose: preserve the recovered HUD global storage for g_zTimedTask_ActiveTail.
 */
zTimedTask *g_zTimedTask_ActiveTail = 0;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcmdmousedebounceframes
 * @recoil-artifact defines .data recoil:data:0x4e5e00: g_HudCmdMouseDebounceFrames.
 * Purpose: preserve the recovered HUD global storage for g_HudCmdMouseDebounceFrames.
 */
int g_HudCmdMouseDebounceFrames = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduiwidget-exclusivedrawimage
 * @recoil-artifact defines .data recoil:data:0x56bd1c: g_HudUiWidget_ExclusiveDrawImage.
 * Purpose: preserve the recovered HUD global storage for g_HudUiWidget_ExclusiveDrawImage.
 */
zVidImagePartial *g_HudUiWidget_ExclusiveDrawImage = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduimgrsensor-roundrobintrackindex
 * @recoil-artifact defines .data recoil:data:0x4dd1d8: g_HudUiMgrSensor_RoundRobinTrackIndex.
 * Owner data: four-byte signed index initialized to -1 (ff ff ff ff).
 * Purpose: seed the sensor-target round-robin candidate selector before
 * HudUiMgrSensor::UpdateMarkersAndProgressFromVariantTag advances it.
 */
int g_HudUiMgrSensor_RoundRobinTrackIndex = -1;
HudUiRect g_HudUiMgrSensor_FxRectScratch = {0};

#undef g_HudUiMgr
#undef g_HudLayoutHW
#undef g_HudLayoutSW
#undef g_HudCmdDialogState

union HudUiSensorWindowStorage {
    unsigned long align;
    unsigned char bytes[sizeof(CWnd)];
};
RECOIL_STATIC_ASSERT(sizeof(HudUiSensorWindowStorage) == sizeof(CWnd));


/*
 * Retail HUD UI storage and CRT initialization are not the same sequence.
 * Keep these definitions in the recovered storage run; VC5's compiler-emitted
 * static-lifetime helpers for HudLayoutSW and HudLayoutHW, together with the
 * HudUiMgr initialization and provider CString/CWnd constructors, form the
 * retail CRT pass:
 * CString, HudLayoutSW, HudLayoutHW, HudUiMgr, then CWnd.
 */
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduisensorwindow
 * @recoil-artifact defines .data recoil:data:0x4e5e90: g_HudUiSensorWindow.
 * Source model: zero-initialized provider CWnd storage; the explicit HUD CRT
 * row constructs it and registers the provider destructor.
 * Purpose: preserve the recovered HUD global storage for g_HudUiSensorWindow.
 */
HudUiSensorWindowStorage g_HudUiSensorWindow = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduimgr
 * @recoil-artifact defines .data recoil:data:0x4e5ed0: g_HudUiMgr.
 * Source model: zero-initialized HudUiMgrData storage; HudUiMgr::StaticInit
 * constructs the typed manager through the explicit CRT row.
 * Purpose: preserve the recovered HUD global storage for g_HudUiMgr.
 */
HudUiMgrDataStorage g_HudUiMgr = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudlayoutsw
 * @recoil-artifact defines .data recoil:data:0x4eda68: g_HudLayoutSW.
 * Owner data: typed 236-byte singleton with compiler-owned static lifetime.
 * Retail startup constructs the software layout before the hardware layout.
 * Purpose: own the global software HUD layout instance.
 */
HudLayoutSW g_HudLayoutSW;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudlayouthw
 * @recoil-artifact defines .data recoil:data:0x4ed718: g_HudLayoutHW.
 * Owner data: typed 844-byte singleton with compiler-owned static lifetime.
 * Purpose: own the global hardware HUD layout instance.
 */
HudLayoutHW g_HudLayoutHW;

#define g_HudUiSensorWindow \
    (*(CWnd *)&g_HudUiSensorWindow)
#define g_HudUiMgr \
    (*(HudUiMgrData *)&g_HudUiMgr)

HudUiRect g_HudUiMgrSensorFxRect = {0};
int g_HudUiMgrSensorFxViewportWidth = 0;
int g_HudUiMgrSensorFxViewportHeight = 0;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduisensorwindowplayback
 * @recoil-artifact defines .data recoil:data:0x4edb70: g_HudUiSensorWindowPlayback.
 * Purpose: preserve the recovered HUD global storage for g_HudUiSensorWindowPlayback.
 */
zFMV_Playback *g_HudUiSensorWindowPlayback = 0;

// Moved HUD runtime bodies live in src/Battlesport/hud_runtime_layer_body.h
// and are included by src/Battlesport/hud.cpp for physical HUD order.

extern "C" {
HudUiMgrSensorTrackList g_HudUiMgrSensor_TrackList = {0};
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduichatmessagestack
 * @recoil-artifact defines .data recoil:data:0x56bd20: g_HudUiChatMessageStack.
 * Purpose: preserve the recovered HUD global storage for g_HudUiChatMessageStack.
 */
HudUiTextStack4 *g_HudUiChatMessageStack = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduitopmessagestack
 * @recoil-artifact defines .data recoil:data:0x56bd24: g_HudUiTopMessageStack.
 * Purpose: preserve the recovered HUD global storage for g_HudUiTopMessageStack.
 */
HudUiTextStack4 *g_HudUiTopMessageStack = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudui-auxoverlayenabled
 * @recoil-artifact defines .data recoil:data:0x4f3aa8: g_HudUi_AuxOverlayEnabled.
 * Purpose: preserve the recovered HUD global storage for g_HudUi_AuxOverlayEnabled.
 */
int g_HudUi_AuxOverlayEnabled = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcmddialogstate
 * @recoil-artifact defines .data recoil:data:0x4e5df0: g_HudCmdDialogState.
 * BN identifies 0x4e5df0 as an eight-byte BSS HudCmdDialogState object. VC5 emits the
 * 0x40bc20/0x40bc30/0x40bc40/0x40bc50 static init and at-exit thunks from this global object.
 * Purpose: preserve the recovered HUD global storage for g_HudCmdDialogState.
 */
HudCmdDialogStateStorage g_HudCmdDialogState = {0};

#define g_HudCmdDialogState \
    (*(HudCmdDialogState *)&g_HudCmdDialogState)

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-resolutioncyclenodename
 * @recoil-artifact defines .data recoil:data:0x4dac00: g_HudUiOptionsPanel_ResolutionCycleNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD resolution selector node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_ResolutionCycleNodeName[] = "RESOLUTION_CYCLE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-musicvolumewidgetnodename
 * @recoil-artifact defines .data recoil:data:0x4dac14: g_HudUiOptionsPanel_MusicVolumeWidgetNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD music-volume widget node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_MusicVolumeWidgetNodeName[] = "MUSIC_VOLUME";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-musicenabletogglenodename
 * @recoil-artifact defines .data recoil:data:0x4dac24: g_HudUiOptionsPanel_MusicEnableToggleNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD music-enable toggle node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_MusicEnableToggleNodeName[] = "MUSIC_ENABLE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-soundvolumewidgetnodename
 * @recoil-artifact defines .data recoil:data:0x4dac34: g_HudUiOptionsPanel_SoundVolumeWidgetNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD sound-volume widget node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_SoundVolumeWidgetNodeName[] = "SOUND_VOLUME";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-soundqualityselectornodename
 * @recoil-artifact defines .data recoil:data:0x4dac44: g_HudUiOptionsPanel_SoundQualitySelectorNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD sound-quality selector node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_SoundQualitySelectorNodeName[] = "SOUND_QUALITY";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-soundactivetogglenodename
 * @recoil-artifact defines .data recoil:data:0x4dac54: g_HudUiOptionsPanel_SoundActiveToggleNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD sound-active toggle node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_SoundActiveToggleNodeName[] = "SOUND_ACTIVE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-effectszrdnodename
 * @recoil-artifact defines .data recoil:data:0x4dac64: g_EffectsZrdNodeName
 * (BN: g_HudUiOptionsPanel_EffectsNodeName).
 * Shared data owner: effects_weapons.shared_effects_zrd_node_name; this is
 * not HudOptionsDialog-owned data.
 * Purpose: name the shared EFFECTS ZRD node consumed by HudOptionsDialog and
 */
char g_EffectsZrdNodeName[8] = "EFFECTS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-texturememoryselectornodename
 * @recoil-artifact defines .data recoil:data:0x4dac6c: g_HudUiOptionsPanel_TextureMemorySelectorNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD texture-memory selector node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_TextureMemorySelectorNodeName[] = "TEXTURE_MEMORY";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-objectdetailselectornodename
 * @recoil-artifact defines .data recoil:data:0x4dac7c: g_HudUiOptionsPanel_ObjectDetailSelectorNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD object-detail selector node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_ObjectDetailSelectorNodeName[] = "OBJECT_DETAIL";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-fullhudtogglenodename
 * @recoil-artifact defines .data recoil:data:0x4dac8c: g_HudUiOptionsPanel_FullHudToggleNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD full-HUD toggle node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_FullHudToggleNodeName[] = "FULLHUD";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-perspectivetogglenodename
 * @recoil-artifact defines .data recoil:data:0x4dac94: g_HudUiOptionsPanel_PerspectiveToggleNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD perspective toggle node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_PerspectiveToggleNodeName[] = "PERSPECTIVE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-lightingtogglenodename
 * @recoil-artifact defines .data recoil:data:0x4daca0: g_HudUiOptionsPanel_LightingToggleNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD lighting toggle node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_LightingToggleNodeName[] = "LIGHTING";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-sectionname
 * @recoil-artifact defines .data recoil:data:0x4dacac: g_HudUiOptionsPanel_SectionName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD options-panel section loaded by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_SectionName[] = "OPTIONSPANEL";
extern char g_HudFontName_Arial[];
/**
 * Storage group:
 * hud_ui.hud_ui_mgr_ensure_hud_loaded_literals.
 * Source model: writable HUD ZRD key/source-path string globals shared by
 * HudUiMgr::EnsureHudLoaded and matching reader paths in zTurret/zImage.
 * Purpose: name the HUD layout sections and diagnostics consumed while the
 * HUD singleton loads its ZRD tree.
 */
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-modes
 * @recoil-artifact defines .data recoil:data:0x4dad2c: g_HudCfgKey_Modes.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Modes.
 */
char g_HudCfgKey_Modes[6] = "MODES";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-weapon
 * @recoil-artifact defines .data recoil:data:0x4dad34: g_HudCfgKey_Weapon.
 * Purpose: preserve the recovered shared WEAPON reader-key storage.
 */
char g_HudCfgKey_Weapon[7] = "WEAPON";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-target
 * @recoil-artifact defines .data recoil:data:0x4dad3c: g_HudCfgKey_Target.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Target.
 */
char g_HudCfgKey_Target[7] = "TARGET";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-shield
 * @recoil-artifact defines .data recoil:data:0x4dad44: g_HudCfgKey_Shield.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Shield.
 */
char g_HudCfgKey_Shield[7] = "SHIELD";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduiblankspaces8
 * @recoil-artifact defines .data recoil:data:0x4dad4c: g_HudUiBlankSpaces8.
 * Purpose: preserve the recovered HUD global storage for g_HudUiBlankSpaces8.
 */
char g_HudUiBlankSpaces8[9] = "        ";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-stats
 * @recoil-artifact defines .data recoil:data:0x4dad58: g_HudCfgKey_Stats.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Stats.
 */
char g_HudCfgKey_Stats[6] = "STATS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-reticule
 * @recoil-artifact defines .data recoil:data:0x4dad60: g_HudCfgKey_Reticule.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Reticule.
 */
char g_HudCfgKey_Reticule[9] = "RETICULE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-objective
 * @recoil-artifact defines .data recoil:data:0x4dad6c: g_HudCfgKey_Objective.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Objective.
 */
char g_HudCfgKey_Objective[10] = "OBJECTIVE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-sensor
 * @recoil-artifact defines .data recoil:data:0x4dad78: g_HudCfgKey_Sensor.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Sensor.
 */
char g_HudCfgKey_Sensor[7] = "SENSOR";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-nanite
 * @recoil-artifact defines .data recoil:data:0x4dad80: g_HudCfgKey_Nanite.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Nanite.
 */
char g_HudCfgKey_Nanite[7] = "NANITE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-ammo
 * @recoil-artifact defines .data recoil:data:0x4dad88: g_HudCfgKey_Ammo.
 * Purpose: preserve the recovered shared AMMO reader-key storage.
 */
char g_HudCfgKey_Ammo[5] = "AMMO";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-strings
 * @recoil-artifact defines .data recoil:data:0x4dad90: g_HudCfgKey_Strings.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Strings.
 */
char g_HudCfgKey_Strings[8] = "STRINGS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-objectivedescription
 * @recoil-artifact defines .data recoil:data:0x4dad98: g_HudCfgKey_ObjectiveDescription.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_ObjectiveDescription.
 */
char g_HudCfgKey_ObjectiveDescription[16] = "OBJ_DESCRIPTION";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-objectivesummary
 * @recoil-artifact defines .data recoil:data:0x4dada8: g_HudCfgKey_ObjectiveSummary.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_ObjectiveSummary.
 */
char g_HudCfgKey_ObjectiveSummary[12] = "OBJ_SUMMARY";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-fonts
 * @recoil-artifact defines .data recoil:data:0x4dadb4: g_HudCfgKey_Fonts.
 * Purpose: preserve the recovered shared FONTS reader-key storage.
 */
char g_HudCfgKey_Fonts[6] = "FONTS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hud-imagesearchpath-hud
 * @recoil-artifact defines .data recoil:data:0x4dadbc: g_Hud_ImageSearchPath_Hud.
 * Purpose: preserve the recovered HUD global storage for g_Hud_ImageSearchPath_Hud.
 */
char g_Hud_ImageSearchPath_Hud[26] = "..\\data\\common\\images\\hud";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hud-sourcefile-hudcpp
 * @recoil-artifact defines .data recoil:data:0x4dadd8: g_Hud_SourceFile_HudCpp.
 * Purpose: preserve the recovered HUD global storage for g_Hud_SourceFile_HudCpp.
 */
char g_Hud_SourceFile_HudCpp[28] = "D:\\Proj\\Battlesport\\hud.cpp";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudsensortracker-readfilefailedfmt
 * @recoil-artifact defines .data recoil:data:0x4dadf4: g_HudSensorTracker_ReadFileFailedFmt.
 * Shared data owner: hud_ui.shared_zrd_read_failed_format_literal.
 * Purpose: provide the shared failed-read diagnostic format used by HUD,
 * mission, pickup, turret, image, and opt-catalog ZRD load paths.
 */
char g_HudSensorTracker_ReadFileFailedFmt[18] = "Failed to read %s";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x6-0x4db428
 * @recoil-artifact defines .data recoil:data:0x4db428: g_HudZrd_Key_Color.
 * Shared data owner: hud_ui.background_primitive_zrd_key_literals.
 * Purpose: name shared COLOR ZRD records consumed by HUD primitive binding.
 */
char g_HudZrd_Key_Color[0x6] = "COLOR";
/**
 * Storage group:
 * hud_ui.hud_ui_zrd_widget_base_zrd_key_literals.
 * Source model: writable HUD ZRD key string globals consumed by the base
 * HudUiZrdWidget loaders. BN shows the six char[] objects in this order with
 * the address-aligned padding between ACTIVATE/DISABLE, RATE/FLASH,
 * FLASH/LABEL, LABEL/ROLLOVER, and ROLLOVER/BITMAP.
 * Purpose: name the optional activation, disable, rollover, label, and flash
 * records in a recovered HudUiZrdWidget section.
 */
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x9-0x4e46d0
 * @recoil-artifact defines .data recoil:data:0x4e46d0: g_HudZrd_Key_Activate.
 * Purpose: preserve the recovered HUD global storage for g_HudZrd_Key_Activate.
 */
char g_HudZrd_Key_Activate[0x9] = "ACTIVATE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x8-0x4e46dc
 * @recoil-artifact defines .data recoil:data:0x4e46dc: g_HudZrd_Key_Disable.
 * Purpose: preserve the recovered HUD global storage for g_HudZrd_Key_Disable.
 */
char g_HudZrd_Key_Disable[0x8] = "DISABLE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x5
 * @recoil-artifact defines .data recoil:data:0x4e46e4: g_HudZrd_Key_Rate.
 * Purpose: preserve the recovered HUD global storage for g_HudZrd_Key_Rate.
 */
char g_HudZrd_Key_Rate[0x5] = "RATE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x6-0x4e46ec
 * @recoil-artifact defines .data recoil:data:0x4e46ec: g_HudZrd_Key_Flash.
 * Purpose: preserve the recovered HUD global storage for g_HudZrd_Key_Flash.
 */
char g_HudZrd_Key_Flash[0x6] = "FLASH";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x6-0x4e46f4
 * @recoil-artifact defines .data recoil:data:0x4e46f4: g_HudZrd_Key_Label.
 * Purpose: preserve the recovered HUD global storage for g_HudZrd_Key_Label.
 */
char g_HudZrd_Key_Label[0x6] = "LABEL";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x9-0x4e46fc
 * @recoil-artifact defines .data recoil:data:0x4e46fc: g_HudZrd_Key_Rollover.
 * Purpose: preserve the recovered HUD global storage for g_HudZrd_Key_Rollover.
 */
char g_HudZrd_Key_Rollover[0x9] = "ROLLOVER";
RECOIL_STATIC_ASSERT(sizeof(g_HudZrd_Key_Activate) == 0x9);
RECOIL_STATIC_ASSERT(sizeof(g_HudZrd_Key_Disable) == 0x8);
RECOIL_STATIC_ASSERT(sizeof(g_HudZrd_Key_Rate) == 0x5);
RECOIL_STATIC_ASSERT(sizeof(g_HudZrd_Key_Flash) == 0x6);
RECOIL_STATIC_ASSERT(sizeof(g_HudZrd_Key_Label) == 0x6);
RECOIL_STATIC_ASSERT(sizeof(g_HudZrd_Key_Rollover) == 0x9);
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduicycleselectorwidget-zrdkey-bitmap
 * @recoil-artifact defines .data recoil:data:0x4e4708: g_HudUiCycleSelectorWidget_ZrdKey_Bitmap.
 * Shared data owner: hud_ui.cycle_selector_shared_zrd_key_literals.
 * Purpose: name the shared BITMAP ZRD record consumed by HUD widget loaders.
 */
char g_HudUiCycleSelectorWidget_ZrdKey_Bitmap[] = "BITMAP";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x9-0x4e4710
 * @recoil-artifact defines .data recoil:data:0x4e4710: g_HudZrd_Key_Position.
 * Shared data owner: hud_ui.background_primitive_zrd_key_literals.
 * Purpose: name shared POSITION ZRD records consumed by HUD widget loaders.
 */
char g_HudZrd_Key_Position[0x9] = "POSITION";
/**
 * Storage group:
 * hud_ui.hud_ui_check_toggle_zrd_key_literals.
 * Source model: writable HUD ZRD key string globals consumed by
 * HudUiCheckToggleWidget::LoadFromZrd. BN shows DISABLE_SEL and
 * DISABLE_UNSEL in address order before the shared TEXT key, then CHECKED
 * immediately after TEXT with one aligned padding byte before CYCLE.
 * Purpose: name the check-toggle checked/disabled ZRD variant records.
 */
char g_HudUiZrdKey_DisableSel[0xb] = {
    'D', 'I', 'S', 'A', 'B', 'L', 'E', '_', 'S', 'E', 'L'
};
char g_HudUiZrdKey_DisableUnsel[0xd] = {
    'D', 'I', 'S', 'A', 'B', 'L', 'E', '_', 'U', 'N', 'S', 'E', 'L'
};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduicycleselectorwidget-zrdkey-text
 * @recoil-artifact defines .data recoil:data:0x4e4738: g_HudUiCycleSelectorWidget_ZrdKey_Text.
 * Shared data owner: hud_ui.cycle_selector_shared_zrd_key_literals.
 * BN exposes four TEXT bytes followed by aligned zero padding before CHECKED;
 * keep the pool slot contiguous so C-string lookups see the terminator.
 * Purpose: name shared TEXT ZRD records consumed by toggle and cycle widgets.
 */
char g_HudUiCycleSelectorWidget_ZrdKey_Text[8] = {'T', 'E', 'X', 'T'};
char g_HudUiZrdKey_Checked[0x7] = {'C', 'H', 'E', 'C', 'K', 'E', 'D'};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduicycleselectorwidget-zrdkey-cycle
 * @recoil-artifact defines .data recoil:data:0x4e4748: g_HudUiCycleSelectorWidget_ZrdKey_Cycle.
 * Shared data owner: hud_ui.cycle_selector_shared_zrd_key_literals.
 * Purpose: name the CYCLE ZRD array loaded by HudUiCycleSelectorWidget.
 */
char g_HudUiCycleSelectorWidget_ZrdKey_Cycle[] = "CYCLE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduicycleselectorwidget-zrdkey-textoffset
 * @recoil-artifact defines .data recoil:data:0x4e4750: g_HudUiCycleSelectorWidget_ZrdKey_TextOffset.
 * Shared data owner: hud_ui.cycle_selector_shared_zrd_key_literals.
 * Purpose: name the TEXTOFFSET ZRD array loaded by HudUiCycleSelectorWidget.
 */
char g_HudUiCycleSelectorWidget_ZrdKey_TextOffset[] = "TEXTOFFSET";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduicycleselectorwidget-zrdkey-font
 * @recoil-artifact defines .data recoil:data:0x4e475c: g_HudUiCycleSelectorWidget_ZrdKey_Font.
 * Shared data owner: hud_ui.cycle_selector_shared_zrd_key_literals.
 * Purpose: name shared FONT ZRD records consumed by HUD widget loaders.
 */
char g_HudUiCycleSelectorWidget_ZrdKey_Font[] = "FONT";
/**
 * Storage group:
 * hud_ui.zhud_background_config_zrd_key_literals.
 * Source model: writable HudUiBackground ZRD key string globals consumed by
 * HudUiBackground::LoadZrdAndSection. BN shows the keys in this order, with
 * VC5 char-array alignment padding between adjacent slots.
 * Purpose: name the background resource, cursor, capture, and sound records
 * in a recovered background ZRD section.
 */
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x12-0x4e47c0
 * @recoil-artifact defines .data recoil:data:0x4e47c0: zHudCfgKey_BACKGROUND_SOUNDS.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_BACKGROUND_SOUNDS.
 */
char zHudCfgKey_BACKGROUND_SOUNDS[0x12] = "BACKGROUND_SOUNDS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x8-0x4e47d4
 * @recoil-artifact defines .data recoil:data:0x4e47d4: zHudCfgKey_CAPTURE.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_CAPTURE.
 */
char zHudCfgKey_CAPTURE[0x8] = "CAPTURE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x7
 * @recoil-artifact defines .data recoil:data:0x4e47dc: zHudCfgKey_CURSOR.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_CURSOR.
 */
char zHudCfgKey_CURSOR[0x7] = "CURSOR";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x10
 * @recoil-artifact defines .data recoil:data:0x4e47e4: zHudCfgKey_BACKGROUND_TEXT.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_BACKGROUND_TEXT.
 */
char zHudCfgKey_BACKGROUND_TEXT[0x10] = "BACKGROUND_TEXT";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x12-0x4e47f4
 * @recoil-artifact defines .data recoil:data:0x4e47f4: zHudCfgKey_BACKGROUND_VIDEOS.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_BACKGROUND_VIDEOS.
 */
char zHudCfgKey_BACKGROUND_VIDEOS[0x12] = "BACKGROUND_VIDEOS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x12-0x4e4808
 * @recoil-artifact defines .data recoil:data:0x4e4808: zHudCfgKey_BACKGROUND_IMAGES.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_BACKGROUND_IMAGES.
 */
char zHudCfgKey_BACKGROUND_IMAGES[0x12] = "BACKGROUND_IMAGES";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x12-0x4e4824
 * @recoil-artifact defines .data recoil:data:0x4e4824: zHudCfgKey_SHARED_IMAGE_PATH.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_SHARED_IMAGE_PATH.
 */
char zHudCfgKey_SHARED_IMAGE_PATH[0x12] = "SHARED_IMAGE_PATH";
RECOIL_STATIC_ASSERT(sizeof(zHudCfgKey_BACKGROUND_SOUNDS) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(zHudCfgKey_CAPTURE) == 0x8);
RECOIL_STATIC_ASSERT(sizeof(zHudCfgKey_CURSOR) == 0x7);
RECOIL_STATIC_ASSERT(sizeof(zHudCfgKey_BACKGROUND_TEXT) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zHudCfgKey_BACKGROUND_VIDEOS) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(zHudCfgKey_BACKGROUND_IMAGES) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(zHudCfgKey_SHARED_IMAGE_PATH) == 0x12);
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x8-0x4e4838
 * @recoil-artifact defines .data recoil:data:0x4e4838: g_HudUiZrdToken_Buttons.
 * Data owner: hud_ui.hud_ui_background_buttons_zrd_key_literal.
 * Purpose: name the BUTTONS child table consumed by
 */
char g_HudUiZrdToken_Buttons[0x8] = "BUTTONS";
RECOIL_STATIC_ASSERT(sizeof(g_HudUiZrdToken_Buttons) == 0x8);
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x9-0x4e4840
 * @recoil-artifact defines .data recoil:data:0x4e4840: g_HudUiZrdToken_EndPointAbsolute.
 * Data owner: hud_ui.background_primitive_zrd_key_literals.
 * Purpose: name the absolute endpoint ZRD record consumed by HUD primitive binding.
 */
char g_HudUiZrdToken_EndPointAbsolute[0x9] = "ENDP_ABS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x9-0x4e484c
 * @recoil-artifact defines .data recoil:data:0x4e484c: g_HudUiZrdToken_EndPointRelative.
 * Data owner: hud_ui.background_primitive_zrd_key_literals.
 * Purpose: name the relative endpoint ZRD record consumed by HUD primitive binding.
 */
char g_HudUiZrdToken_EndPointRelative[0x9] = "ENDP_REL";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x9-0x4e4858
 * @recoil-artifact defines .data recoil:data:0x4e4858: g_HudUiZrdToken_WordWrap.
 * Data owner: hud_ui.background_primitive_zrd_key_literals.
 * Purpose: name the WORDWRAP ZRD record consumed by HUD primitive binding.
 */
char g_HudUiZrdToken_WordWrap[0x9] = "WORDWRAP";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-xb
 * @recoil-artifact defines .data recoil:data:0x4e4864: g_HudUiBackground_ZrdKey_Primitives.
 * Data owner: hud_ui.background_primitive_zrd_key_literals.
 * Purpose: name the PRIMITIVES ZRD container consumed by HUD primitive binding.
 */
char g_HudUiBackground_ZrdKey_Primitives[0xb] = "PRIMITIVES";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduifillbitmap-zrdkey-fillbitmap
 * @recoil-artifact defines .data recoil:data:0x4e4764: g_HudUiFillBitmap_ZrdKey_FillBitmap.
 * Data owner: hud_ui.hud_ui_fill_bitmap_zrd_key_literals.
 * Purpose: name the FILLBITMAP ZRD record consumed by HudUiFillBitmap.
 */
char g_HudUiFillBitmap_ZrdKey_FillBitmap[] = "FILLBITMAP";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduizrdwidgetex17c-item-zrdkey-mouserect
 * @recoil-artifact defines .data recoil:data:0x4e4770: g_HudUiZrdWidgetEx17C_Item_ZrdKey_MouseRect.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the optional mouse-rectangle ZRD child loaded by HudUiZrdWidgetEx17C_Item.
 */
char g_HudUiZrdWidgetEx17C_Item_ZrdKey_MouseRect[] = "MOUSERECT";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduizrdtoken-radio
 * @recoil-artifact defines .data recoil:data:0x4e477c: g_HudUiZrdToken_Radio.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the RADIO ZRD child array loaded by HudUiZrdWidgetEx17C.
 */
char g_HudUiZrdToken_Radio[] = "RADIO";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudzrd-key-sound
 * @recoil-artifact defines .data recoil:data:0x4dc17c: g_HudZrd_Key_Sound.
 * Shared data owner: hud_ui.shared_zrd_sound_key_literal.
 * Purpose: name the shared SOUND ZRD record consumed by HUD widget, pickup,
 * and opt-catalog sound loaders.
 */
char g_HudZrd_Key_Sound[6] = "SOUND";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduimessage-clearspecialtoken165
 * @recoil-artifact defines .data recoil:data:0x4dae08: g_HudUiMessage_ClearSpecialToken165.
 * Data owner: hud_ui.hud_ui_message_clear_special_token_literal.
 * Exact extent is the writable 4-byte .data object a5 00 00 00 referenced by
 * HudUiMessage::SetValueIfOwnerMatches and
 * Purpose: provide the special one-byte token string used to clear HUD message
 * panel text when the sentinel float value is passed.
 */
char g_HudUiMessage_ClearSpecialToken165[4] = "\xa5";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudlayout-typeisectionname
 * @recoil-artifact defines .data recoil:data:0x4dae0c: g_HudLayout_TypeISectionName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the TYPEI HUD layout section loaded from the HUD ZRD root.
 */
char g_HudLayout_TypeISectionName[] = "TYPEI";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudlayout-typeiisectionname
 * @recoil-artifact defines .data recoil:data:0x4dae14: g_HudLayout_TypeIISectionName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the TYPEII HUD layout section loaded from the HUD ZRD root.
 */
char g_HudLayout_TypeIISectionName[] = "TYPEII";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduiblankspaces3
 * @recoil-artifact defines .data recoil:data:0x4dae1c: g_HudUiBlankSpaces3.
 * Data owner: hud_ui.hud_ui_message_layout_literals.
 * Exact extent is the writable 4-byte .data object 20 20 20 00 referenced by
 * Purpose: provide the initial blank weapon-message panel text.
 */
char g_HudUiBlankSpaces3[4] = "   ";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hud-checkpointoverflowmsg
 * @recoil-artifact defines .data recoil:data:0x4dae20: g_Hud_CheckpointOverflowMsg.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable checkpoint-overflow diagnostic text used by
 */
char g_Hud_CheckpointOverflowMsg[20] = "Checkpoint overflow";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduimessage-nodename
 * @recoil-artifact defines .data recoil:data:0x4dae40: g_HudUiMessage_NodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the objective HUD message node used for chat and save/load status prompts.
 */
char g_HudUiMessage_NodeName[8] = "Message";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduimessage-separatorcolon
 * @recoil-artifact defines .data recoil:data:0x4dae48: g_HudUiMessage_SeparatorColon.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: separate the local player name from chat text when composing HUD messages.
 */
char g_HudUiMessage_SeparatorColon[2] = ":";




#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *HudUiSensorWindowCrtInitializerFn)();
#pragma data_seg(".CRT$XCU")
/* VC5 emits this HUD sensor window startup callback as a direct .CRT$XCU row. */
HudUiSensorWindowCrtInitializerFn s_HudUiSensorWindowCrtInit =
    HudUiSensorWindow::StaticInitAndRegisterAtExit;
#pragma data_seg()
#endif

/**
 * Original inline helper; no standalone retail function exists. BN vtable
 * evidence at 0x4ce968, 0x4ce988, and 0x4ce9a8 points this slot at the
 * shared no-op body 0x404e80.
 * Original source name: HudLayoutBase::LayoutPreUpdate.
 * Purpose: preserve the typed HudLayoutBase virtual source model without
 * introducing a production FTable scaffold.
 */
void HudLayoutBase::LayoutPreUpdate() {
}

/**
 * Original inline helper; no standalone retail function exists.
 * Original source name: HudLayoutBase::OnActivated.
 * Purpose: provide the default layout activation hook for derived HUD layouts.
 */
void HudLayoutBase::OnActivated() {
}

namespace HudLayout {
} // namespace HudLayout

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x0c
 * @recoil-artifact defines .data recoil:data:0x4e4870: g_HudUi_InvalidateMask.
 * Purpose: Stores g HudUi InvalidateMask data used by hud_ui.invalidate_mask_global.
 */
unsigned int g_HudUi_InvalidateMask = 0x0c;
}

namespace {
const char kNumericTextInputAcceptedRawKeyChars[] = "0123456789.-\x1b\r\x08\x7f\x02\x06";
extern "C" const char kClampedIntTextInputAcceptedRawKeyChars[] = "0123456789\x1b\r\x08\x7f\x02\x06";

#if defined(_MSC_VER) && _MSC_VER < 1200
// VC5 misparses explicit function-template calls such as FieldAt<unsigned int>(...).
// Keep the same call-site spelling for first-pass VC5 verification without changing
// modern compiler codegen.
template <typename T> class FieldAt {
  public:
    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4135f0 HudLayoutHW::Disable callers.
 * Purpose: preserve the recovered HUD behavior for FieldAt.
 */
FieldAt(
        void *base,
        size_t offset
    )
        : address((T *)((unsigned char *)(base) + offset)) {}

    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4135f0 HudLayoutHW::Disable callers.
 * Purpose: preserve the recovered HUD behavior for FieldAt.
 */
FieldAt(
        const void *base,
        size_t offset
    )
        : address((T *)((const unsigned char *)(base) + offset)) {}

    operator T &() {
        return *address;
    }

    T *operator&() const {
        return address;
    }

    FieldAt &operator=(
        const T &value
    ) {
        *address = value;
        return *this;
    }

    FieldAt &operator|=(
        const T &value
    ) {
        *address |= value;
        return *this;
    }

    FieldAt &operator+=(
        const T &value
    ) {
        *address += value;
        return *this;
    }

    FieldAt &operator-=(
        const T &value
    ) {
        *address -= value;
        return *this;
    }

  private:
    T *address;
};
#else
template <typename T>
/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4135f0 HudLayoutHW::Disable callers.
 * Purpose: preserve the recovered HUD behavior for FieldAt.
 */
T &FieldAt(
    void *base,
    size_t offset
) {
    return *(T *)((unsigned char *)(base) + offset);
}

template <typename T>
/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x40d220 HudUiListMenuEntry::CompareSortKey callers.
 * Purpose: preserve the recovered HUD behavior for FieldAt.
 */
const T &FieldAt(
    const void *base,
    size_t offset
) {
    return *(const T *)((const unsigned char *)(base) + offset);
}
#endif


const float kHudUiMessageClearSpecialTokenValue = 123456792.0f;

/**
 * Recovered original inline/static helper with no standalone retail function.
 * Observed in callers 0x4b5630, 0x4b5740, 0x4b5860, and 0x4b5900 as the
 * same HudUiPanelPtrVector begin/end loop dispatching HudUiElement::SetVisible.
 * Purpose: apply a visibility state to every panel in a recovered panel-vector
 * member while preserving the original HudUiZrdWidget source pattern.
 */
inline void HudUiSetPanelVectorVisible(
    HudUiPanelPtrVector &panels,
    int visible
) {
    for (HudUiPanelPtrVector::iterator it = panels.begin(); it != panels.end(); ++it) {
        (*it)->SetVisible(visible);
    }
}

struct HudUiZrdDeleteChildIfPresent {
    HudUiPanel * operator()(HudUiPanel *childWidgetOrNull) {
        return (HudUiPanel *)(HudUiZrdWidget::DeleteChildIfPresent(
            childWidgetOrNull
        ));
    }
};

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x414670 HudUiTripletEntries::GetCount callers.
 * Purpose: preserve the recovered HUD behavior for ZrdArrayBase.
 */
inline zReader::Node *ZrdArrayBase(
    zReader::Node *node
) {
    if (node == 0 || node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    return node->value.nodes;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x414670 HudUiTripletEntries::GetCount callers.
 * Purpose: preserve the recovered HUD behavior for ZrdArrayCount.
 */
inline int ZrdArrayCount(
    zReader::Node *arrayBase
) {
    return arrayBase != 0 ? arrayBase[0].value.i32 : 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x414670 HudUiTripletEntries::GetCount callers.
 * Purpose: preserve the recovered HUD behavior for ZrdArrayItem.
 */
inline zReader::Node *ZrdArrayItem(
    zReader::Node *arrayBase,
    int index
) {
    return arrayBase != 0 ? &arrayBase[index] : 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x414670 HudUiTripletEntries::GetCount callers.
 * Purpose: preserve the recovered HUD behavior for ZrdArrayString.
 */
inline const char *ZrdArrayString(
    zReader::Node *arrayBase,
    int index
) {
    zReader::Node *const item = ZrdArrayItem(
        arrayBase,
        index
    );
    return item != 0 && item->type == zReader::ZRDR_NODE_STRING ? item->value.str : 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x414670 HudUiTripletEntries::GetCount callers.
 * Purpose: preserve the recovered HUD behavior for ZrdArrayInt.
 */
inline int ZrdArrayInt(
    zReader::Node *arrayBase,
    int index,
    int fallback
) {
    zReader::Node *const item = ZrdArrayItem(
        arrayBase,
        index
    );
    return item != 0 && item->type == zReader::ZRDR_NODE_INT ? item->value.i32 : fallback;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x414670 HudUiTripletEntries::GetCount callers.
 * Purpose: preserve the recovered HUD behavior for ZrdArrayFloat.
 */
inline float ZrdArrayFloat(
    zReader::Node *arrayBase,
    int index,
    float fallback
) {
    zReader::Node *const item = ZrdArrayItem(
        arrayBase,
        index
    );
    if (item == 0) {
        return fallback;
    }

    if (item->type == zReader::ZRDR_NODE_FLOAT) {
        return item->value.f32;
    }

    if (item->type == zReader::ZRDR_NODE_INT) {
        return (float)(item->value.i32);
    }

    return fallback;
}

} // namespace

namespace {

struct HudUiListSelectorItemArrayHeader {
    int count;
};

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x413d30 HudUiLayoutNode::ApplyImageWidget callers.
 * Purpose: preserve the recovered HUD behavior for HudUiZrdOwnerFontStyle.
 */
inline const HudFontStyle *HudUiZrdOwnerFontStyle(
    const HudUiBackground *owner,
    int styleIndex
) {
    const HudFontStyle *const style = &owner->fontStyles[styleIndex];
    return style->validMarker != 0 ? style : 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x413d30 HudUiLayoutNode::ApplyImageWidget callers.
 * Purpose: apply the recovered HUD layout or option state handled by ApplyHudFontStyleToPanel.
 */
inline void ApplyHudFontStyleToPanel(
    HudUiPanel *panel,
    const HudFontStyle *style
) {
    if (style == 0) {
        return;
    }

    panel->SetFont(
        style->fontName,
        style->fontSize,
        style->fontWeight,
        0,
        0,
        0,
        2
    );
    panel->alignMode = style->alignMode;
    panel->textColor0 = style->textColor;
    panel->textColor1 = style->textColor;
    panel->textDirty = 1;
    panel->shadowEnabled = style->shadowEnabled;
    panel->shadowOffsetX = 1;
    panel->shadowOffsetY = 1;
    panel->bkMode = style->bkMode;
    panel->bkColor = style->bkColor;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x413d30 HudUiLayoutNode::ApplyImageWidget callers.
 * Purpose: apply the recovered HUD layout or option state handled by ApplyHudFontStyleTextOnly.
 */
inline void ApplyHudFontStyleTextOnly(
    HudUiPanel *panel,
    const HudFontStyle *style
) {
    if (style == 0) {
        return;
    }

    panel->SetFont(
        style->fontName,
        style->fontSize,
        style->fontWeight,
        0,
        0,
        0,
        2
    );
    panel->textColor0 = style->textColor;
    panel->textColor1 = style->textColor;
    panel->textDirty = 1;
    panel->shadowEnabled = style->shadowEnabled;
    panel->shadowOffsetX = 1;
    panel->shadowOffsetY = 1;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x413d30 HudUiLayoutNode::ApplyImageWidget callers.
 * Purpose: preserve the recovered HUD behavior for DeleteHudUiListSelectorItemArray.
 */
inline void DeleteHudUiListSelectorItemArray(
    HudUiListSelectorItem *items
) {
    if (items == 0) {
        return;
    }

    HudUiListSelectorItemArrayHeader *const header =
        ((HudUiListSelectorItemArrayHeader *)(items)) - 1;
    const int count = header->count;
    {
        for (int index = 0; index < count; ++index) {
            ((HudUiPanel *)(&items[index]))->~HudUiPanel();
        }
    }

    ::operator delete(header);
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x413d30 HudUiLayoutNode::ApplyImageWidget callers.
 * Purpose: preserve the recovered HUD behavior for CreateHudZrdTextPanel.
 */
inline HudUiPanel *CreateHudZrdTextPanel(
    HudUiZrdWidget *widget,
    zReader::Node *textNode,
    int visible
) {
    zReader::Node *const textBase = ZrdArrayBase(textNode);
    if (textBase == 0) {
        return 0;
    }

    HudUiTransitionTextPanel *const transitionPanel =
        (HudUiTransitionTextPanel *)(::operator new(sizeof(HudUiTransitionTextPanel)));
    new (transitionPanel) HudUiTransitionTextPanel;

    HudUiPanel *const panel = (HudUiPanel *)(transitionPanel);
    const char *const key = ZrdArrayString(
        textBase,
        1
    );
    const char *const text = key != 0 ? zLoc::ResolveMessageKeyOrFallback(key) : "";
    panel->SetTextFmt(text != 0 ? text : "");

    HudUiElement *const element = (HudUiElement *)(transitionPanel);
    element->SetPos(
        widget->originX + ZrdArrayInt(
            textBase,
            2,
            0
        ),
        widget->originY + ZrdArrayInt(textBase, 3, 0)
    );

    const int styleIndex = ZrdArrayInt(
        textBase,
        4,
        0
    );
    ApplyHudFontStyleTextOnly(
        panel,
        HudUiZrdOwnerFontStyle(widget->owner, styleIndex)
    );

    element->SetVisible(visible);
    ((HudUiContainer *)(widget->owner))->AddChild(element);
    return panel;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x41ebd0 HudUiMgrSensor::TrackList_Reset callers.
 * Purpose: apply the recovered HUD layout or option state handled by ApplyHudZrdFlashSection.
 */
inline void ApplyHudZrdFlashSection(
    zReader::Node *parentNode,
    HudUiPanelPtrVector &panels
) {
    zReader::Node *const flashNode = zReader_GetNamedNode(
        parentNode,
        g_HudZrd_Key_Flash
    );
    if (flashNode == 0) {
        return;
    }

    float flashRate = 0.0f;
    zReader::ReadNamedFloat(
        flashNode,
        g_HudZrd_Key_Rate,
        &flashRate
    );

    unsigned int flashColor = 0;
    zReader::Node *const colorNode = zReader_GetNamedNode(
        flashNode,
        "COLOR"
    );
    zReader::Node *const colorBase = ZrdArrayBase(colorNode);
    if (colorBase != 0) {
        const unsigned int red = (unsigned int)(ZrdArrayInt(
            colorBase,
            1,
            0
        )) & 0xffu;
        const unsigned int green = (unsigned int)(ZrdArrayInt(
            colorBase,
            2,
            0
        )) & 0xffu;
        const unsigned int blue = (unsigned int)(ZrdArrayInt(
            colorBase,
            3,
            0
        )) & 0xffu;
        flashColor = red | (green << 8) | (blue << 16);
    }

    if (flashRate == 0.0f) {
        return;
    }

    for (HudUiPanelPtrVector::iterator it = panels.begin();
         it != panels.end();
         ++it) {
        ((HudUiTransitionTextPanel *)(*it))->SetFlashColorAndRate(
            flashColor,
            flashRate
        );
    }
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x41ebd0 HudUiMgrSensor::TrackList_Reset callers.
 * Purpose: load the recovered HUD data handled by LoadHudZrdBitmap.
 */
inline void LoadHudZrdBitmap(
    zReader::Node *parentNode,
    const char *sectionName,
    zVidImagePartial **outImage
) {
    zReader::Node *const bitmapNode = zReader_GetNamedNode(
        parentNode,
        sectionName
    );
    zReader::Node *const bitmapBase = ZrdArrayBase(bitmapNode);
    const char *const path = ZrdArrayString(
        bitmapBase,
        1
    );
    if (path != 0) {
        *outImage = zImage::TexDir_FindOrCreateByPath(path);
    }
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x41ebd0 HudUiMgrSensor::TrackList_Reset callers.
 * Purpose: load the recovered HUD data handled by LoadHudZrdSound.
 */
inline void LoadHudZrdSound(
    zReader::Node *parentNode,
    zSndSample **outSound,
    float *outScale
) {
    zReader::Node *const soundNode = zReader_GetNamedNode(
        parentNode,
        g_HudZrd_Key_Sound
    );
    zReader::Node *const soundBase = ZrdArrayBase(soundNode);
    const char *const name = ZrdArrayString(
        soundBase,
        1
    );
    if (name == 0) {
        return;
    }

    *outScale = ZrdArrayCount(soundBase) >= 3 ? ZrdArrayFloat(
        soundBase,
        2,
        1.0f
    ) : 1.0f;
    *outSound = zSnd::FindSampleByName(name);
}

/*
 * Retail 0x4b59f0 contains these source constructs expanded at each state
 * section.  They are macros rather than callable helpers because the retail
 * body has no intervening helper calls and VC5 /Ob1 does not inline the
 * complete nested label loader when it is expressed as an ordinary function.
 */
#define HUD_ZRD_LOAD_BITMAP(parentNode_, sectionName_, outImage_) \
    do { \
        zReader::Node *const hudBitmapNode_ = \
            zReader_GetNamedNode((parentNode_), (sectionName_)); \
        if (hudBitmapNode_ != 0) { \
            zReader::Node *const hudBitmapBase_ = hudBitmapNode_->value.nodes; \
            const char *const hudBitmapPath_ = hudBitmapBase_[1].value.str; \
            if (hudBitmapPath_ != 0) { \
                (outImage_) = zImage::TexDir_FindOrCreateByPath(hudBitmapPath_); \
            } \
        } \
    } while (0)

#define HUD_ZRD_LOAD_SOUND(parentNode_, outSound_, outScale_) \
    do { \
        zReader::Node *const hudSoundNode_ = \
            zReader_GetNamedNode((parentNode_), g_HudZrd_Key_Sound); \
        if (hudSoundNode_ != 0) { \
            zReader::Node *const hudSoundBase_ = hudSoundNode_->value.nodes; \
            const char *const hudSoundName_ = hudSoundBase_[1].value.str; \
            if (hudSoundName_ != 0) { \
                (outScale_) = hudSoundBase_[0].value.i32 >= 3 \
                    ? hudSoundBase_[2].value.f32 \
                    : 1.0f; \
                (outSound_) = zSnd::FindSampleByName(hudSoundName_); \
            } \
        } \
    } while (0)

#define HUD_ZRD_INSERT_LABEL_COUNTED(panels_, panel_) \
    (panels_).insert((panels_).end(), 1, (panel_))

#define HUD_ZRD_INSERT_LABEL_NATURAL(panels_, panel_) \
    (panels_).insert((panels_).end(), (panel_))

#define HUD_ZRD_APPEND_LABEL_WITH_INSERT( \
    widget_, panels_, labelSpecBase_, originX_, originY_, insert_) \
    do { \
        HudUiPanel *hudPanel_; \
        hudPanel_ = (HudUiPanel *)(new HudUiTransitionTextPanel); \
        HudUiElement *hudElement_; \
        hudElement_ = (HudUiElement *)(hudPanel_); \
        hudElement_->flags = (hudElement_->flags & 0x10u) | 0x02u; \
        const char *const hudLabelKey_ = (labelSpecBase_)[1].value.str; \
        const char *const hudLabelText_ = hudLabelKey_ != 0 \
            ? zLoc::ResolveMessageKeyOrFallback(hudLabelKey_) \
            : ""; \
        hudPanel_->SetTextFmt(hudLabelText_ != 0 ? hudLabelText_ : ""); \
        hudElement_->SetPos( \
            (originX_) + (labelSpecBase_)[2].value.i32, \
            (originY_) + (labelSpecBase_)[3].value.i32 \
        ); \
        const int hudStyleIndex_ = (labelSpecBase_)[4].value.i32; \
        const HudFontStyle *hudStyle_ = \
            &((widget_)->owner->fontStyles[hudStyleIndex_]); \
        if (hudStyle_->validMarker == 0) { \
            hudStyle_ = 0; \
        } \
        if (hudStyle_ != 0) { \
            hudPanel_->alignMode = hudStyle_->alignMode; \
            hudPanel_->SetFont( \
                hudStyle_->fontName, \
                hudStyle_->fontSize, \
                hudStyle_->fontWeight, \
                0, \
                0, \
                0, \
                2 \
            ); \
            hudPanel_->textColor0 = hudStyle_->textColor; \
            hudPanel_->textColor1 = hudStyle_->textColor; \
            hudPanel_->textDirty = 1; \
            hudPanel_->shadowEnabled = hudStyle_->shadowEnabled; \
            hudPanel_->shadowOffsetX = 1; \
            hudPanel_->shadowOffsetY = 1; \
            hudPanel_->bkMode = hudStyle_->bkMode; \
            hudPanel_->bkColor = hudStyle_->bkColor; \
        } \
        hudElement_->SetVisible(1); \
        ((HudUiContainer *)((widget_)->owner))->AddChild(hudElement_); \
        insert_((panels_), hudPanel_); \
    } while (0)

#define HUD_ZRD_APPEND_LABEL(widget_, panels_, labelSpecBase_, originX_, originY_) \
    HUD_ZRD_APPEND_LABEL_WITH_INSERT( \
        widget_, panels_, labelSpecBase_, originX_, originY_, \
        HUD_ZRD_INSERT_LABEL_COUNTED)

#define HUD_ZRD_APPEND_LABEL_NATURAL( \
    widget_, panels_, labelSpecBase_, originX_, originY_) \
    HUD_ZRD_APPEND_LABEL_WITH_INSERT( \
        widget_, panels_, labelSpecBase_, originX_, originY_, \
        HUD_ZRD_INSERT_LABEL_NATURAL)

#define HUD_ZRD_LOAD_LABEL_SECTION_WITH_APPEND( \
    widget_, parentNode_, panels_, append_) \
    do { \
        zReader::Node *const hudLabelNode_ = \
            zReader_GetNamedNode((parentNode_), g_HudZrd_Key_Label); \
        if (hudLabelNode_ != 0) { \
            zReader::Node *const hudLabelBase_ = hudLabelNode_->value.nodes; \
            const int hudOriginX_ = (widget_)->originX; \
            const int hudOriginY_ = (widget_)->originY; \
            if (hudLabelBase_[1].type == zReader::ZRDR_NODE_ARRAY) { \
                const int hudLabelCount_ = hudLabelBase_[0].value.i32; \
                for (int hudLabelIndex_ = 1; \
                    hudLabelIndex_ <= hudLabelCount_ - 1; \
                    ++hudLabelIndex_) { \
                    zReader::Node *const hudLabelSpecBase_ = \
                        hudLabelBase_[hudLabelIndex_].value.nodes; \
                    append_( \
                        (widget_), \
                        (panels_), \
                        hudLabelSpecBase_, \
                        hudOriginX_, \
                        hudOriginY_ \
                    ); \
                } \
            } else { \
                append_( \
                    (widget_), \
                    (panels_), \
                    hudLabelBase_, \
                    hudOriginX_, \
                    hudOriginY_ \
                ); \
            } \
        } \
    } while (0)

#define HUD_ZRD_LOAD_LABEL_SECTION(widget_, parentNode_, panels_) \
    HUD_ZRD_LOAD_LABEL_SECTION_WITH_APPEND( \
        widget_, parentNode_, panels_, HUD_ZRD_APPEND_LABEL)

#define HUD_ZRD_LOAD_LABEL_SECTION_NATURAL(widget_, parentNode_, panels_) \
    HUD_ZRD_LOAD_LABEL_SECTION_WITH_APPEND( \
        widget_, parentNode_, panels_, HUD_ZRD_APPEND_LABEL_NATURAL)

#define HUD_ZRD_APPLY_FLASH_SECTION(parentNode_, panels_) \
    do { \
        zReader::Node *const hudFlashNode_ = \
            zReader_GetNamedNode((parentNode_), g_HudZrd_Key_Flash); \
        if (hudFlashNode_ != 0) { \
            float hudFlashRate_ = 0.0f; \
            zReader::Node *const hudRateNode_ = \
                zReader_GetNamedNode(hudFlashNode_, g_HudZrd_Key_Rate); \
            if (hudRateNode_ != 0) { \
                hudFlashRate_ = hudRateNode_->value.nodes[1].value.f32; \
            } \
            unsigned int hudFlashColor_ = 0; \
            zReader::Node *const hudColorNode_ = \
                zReader_GetNamedNode(hudFlashNode_, "COLOR"); \
            if (hudColorNode_ != 0) { \
                zReader::Node *const hudColorBase_ = hudColorNode_->value.nodes; \
                const unsigned int hudRed_ = \
                    (unsigned int)(hudColorBase_[1].value.i32) & 0xffu; \
                const unsigned int hudGreen_ = \
                    (unsigned int)(hudColorBase_[2].value.i32) & 0xffu; \
                const unsigned int hudBlue_ = \
                    (unsigned int)(hudColorBase_[3].value.i32) & 0xffu; \
                hudFlashColor_ = \
                    hudRed_ | (hudGreen_ << 8) | (hudBlue_ << 16); \
            } \
            if (hudFlashRate_ != 0.0f) { \
                for (HudUiPanelPtrVector::iterator hudPanelIt_ = \
                         (panels_).begin(); \
                     hudPanelIt_ != (panels_).end(); \
                     ++hudPanelIt_) { \
                    ((HudUiTransitionTextPanel *)(*hudPanelIt_))-> \
                        SetFlashColorAndRate(hudFlashColor_, hudFlashRate_); \
                } \
            } \
        } \
    } while (0)

inline void LoadHudZrdExpandedLabelSection(
    HudUiZrdWidget *widget,
    zReader::Node *parentNode,
    HudUiPanelPtrVector &panels
) {
    HUD_ZRD_LOAD_LABEL_SECTION(widget, parentNode, panels);
}

inline int LoadHudZrdExpandedLabelArray(
    HudUiZrdWidget *widget,
    zReader::Node *labelBase,
    HudUiPanelPtrVector &panels
) {
    if (labelBase[1].type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    const int originX = widget->originX;
    const int originY = widget->originY;
    const int labelCount = labelBase[0].value.i32;
    for (int labelIndex = 1; labelIndex <= labelCount - 1; ++labelIndex) {
        zReader::Node *const labelSpecBase =
            labelBase[labelIndex].value.nodes;
        HUD_ZRD_APPEND_LABEL(
            widget,
            panels,
            labelSpecBase,
            originX,
            originY
        );
    }
    return 1;
}

} // namespace

namespace HudUiMgrSensor {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-tracklist-add
 * @recoil-artifact defines .text recoil:function:0x438920: HudUiMgrSensor::TrackList_Add.
 * Purpose: append one payload-bearing sensor tracking node to the recovered
 * global track-list owner while preserving its head, tail, and count fields.
 */
HudUiMgrSensorTrackNode *__fastcall TrackList_Add(
    int trackKind,
    void *payload
) {
    HudUiMgrSensorTrackNode *const trackNode =
        (HudUiMgrSensorTrackNode *)(malloc(sizeof(HudUiMgrSensorTrackNode)));
    trackNode->trackKind = 0;
    trackNode->payload = 0;
    trackNode->next = 0;

    if (trackNode != 0) {
        trackNode->next = 0;
        if (g_HudUiMgrSensor_TrackList.count == 0) {
            g_HudUiMgrSensor_TrackList.head = trackNode;
        } else {
            g_HudUiMgrSensor_TrackList.tail->next = trackNode;
        }

        g_HudUiMgrSensor_TrackList.tail = trackNode;
        trackNode->next = 0;
        ++g_HudUiMgrSensor_TrackList.count;
    }

    trackNode->trackKind = trackKind;
    trackNode->payload = payload;
    return trackNode;
}


} // namespace HudUiMgrSensor

namespace HudUiMgrTarget {
} // namespace HudUiMgrTarget

namespace HudUiMgrObjective {
/**
 * Recovered original helper with no standalone retail function. Observed in
 * caller 0x411ac0: HudUiMgrObjective::StartHide.
 * Evidence basis: repeated objective phase runtime update of the widget right
 * edge after slide-position changes.
 * Purpose: refresh the cached objective widget right edge from its current
 * center position and borrowed image width.
 */
static void HudUiMgrObjective_UpdateWidgetRightX() {
    const zVidImagePartial *const image = g_HudUiMgrObjectiveWidget.image;
    const int width = image != 0 ? image->width : 0;
    g_HudUiMgrObjectiveWidgetRightX = g_HudUiMgrObjectiveWidget.GetCenterX() + width;
}

/**
 * Recovered original helper with no standalone retail function. Observed in
 * caller 0x411ac0: HudUiMgrObjective::StartHide.
 * Evidence basis: repeated phase animation sequence updates the objective bar
 * slide edge, invalidates the bar, moves the widget, and recomputes meter X
 * points as one source-level operation.
 * Purpose: apply the objective panel slide X position and dependent meter
 * geometry.
 */
static void HudUiMgrObjective_SetSlidePosition(
    float slideX
) {
    g_HudUiMgrObjectiveBar.points[2].x = slideX;
    g_HudUiMgrObjectiveBar.points[3].x = slideX;
    g_HudUiMgrObjectiveBar.Invalidate();
    ((HudUiElement *)(&g_HudUiMgrObjectiveWidget))->SetX((int)(slideX)-1);
    HudUiMgrObjective::UpdateMeterXPoints();
}

/**
 * Recovered original helper with no standalone retail function. Observed in
 * caller 0x411ac0: HudUiMgrObjective::StartHide.
 * Evidence basis: phase-3 animation branches share the same hardware-HUD dirty
 * rectangle gate through zOpt::GetHudTypeForCurrentHwMode.
 * Purpose: update the hardware HUD objective dirty rectangle only for the
 * hardware perspective HUD mode.
 */
static void HudUiMgrObjective_UpdateHwDirtyRectIfNeeded() {
    if (zOpt::GetHudTypeForCurrentHwMode() == 2) {
        g_HudLayoutHW.UpdateObjectiveDirtyRect();
    }
}

/**
 * Recovered original helper with no standalone retail function. Observed in
 * caller 0x411ac0: HudUiMgrObjective::StartHide.
 * Evidence basis: phase-1 and phase-3 animation branches share the sensor
 * image null guard, mirrored fade-to-noise calculation, visibility update, and
 * zVid::DrawNoiseRect call sequence.
 * Purpose: draw objective sensor transition noise while optionally revealing or
 * hiding the sensor rectangle when the fade passes the midpoint.
 */
static void HudUiMgrObjective_DrawSensorNoise(
    float fade,
    int visibleWhenCovered
) {
    if (g_HudUiMgrObjectiveSensorRect.image == 0) {
        return;
    }

    float noise = fade + fade;
    if (noise < 1.0f) {
        zVid::DrawNoiseRect(
            (zVidRect32 *)(&g_HudUiMgrSensorBlock.sensorRectRaw),
            (double)(noise)
        );
        return;
    }

    g_HudUiMgrObjectiveSensorRect.SetVisible(visibleWhenCovered);
    zVid::DrawNoiseRect(
        (zVidRect32 *)(&g_HudUiMgrSensorBlock.sensorRectRaw),
        (double)(2.0f - noise)
    );
}

} // namespace HudUiMgrObjective

namespace {
/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4137c0 HudUiAuxOverlay::ClearTextLines callers.
 * Purpose: preserve the recovered HUD behavior for HudUiZrdPayload.
 */
zReader::Node *HudUiZrdPayload(
    zReader::Node *node
) {
    return node != 0 && node->type == zReader::ZRDR_NODE_ARRAY ? node->value.nodes : 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4137c0 HudUiAuxOverlay::ClearTextLines callers.
 * Purpose: preserve the recovered HUD behavior for HudUiZrdStringAt.
 */
const char *HudUiZrdStringAt(
    zReader::Node *payload,
    int index
) {
    return payload != 0 ? payload[index].value.str : 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4137c0 HudUiAuxOverlay::ClearTextLines callers.
 * Purpose: preserve the recovered HUD behavior for HudUiZrdIntAt.
 */
int HudUiZrdIntAt(
    zReader::Node *payload,
    int index
) {
    return payload != 0 ? payload[index].value.i32 : 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x40d7e0 HudUiMgr::Constructor callers.
 * Purpose: preserve the recovered HUD behavior for HudUiSetFontFromRect.
 */
void HudUiSetFontFromRect(
    HudUiPanel *panel,
    const HudUiRect &fontSpec
) {
    panel->SetFont(
        (const char *)(fontSpec.left),
        fontSpec.right,
        fontSpec.bottom,
        fontSpec.top,
        0,
        0,
        2
    );
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x40d7e0 HudUiMgr::Constructor callers.
 * Purpose: preserve the recovered HUD behavior for HudUiSetPanelClipWithSource.
 */
void HudUiSetPanelClipWithSource(
    HudUiPanel *panel,
    void *source,
    const HudUiRect *clipRect
) {
    panel->SetClip(
        source,
        clipRect
    );
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x40d7e0 HudUiMgr::Constructor callers.
 * Purpose: preserve the recovered HUD behavior for HudUiApplyStatsTripletInt3.
 */
void HudUiApplyStatsTripletInt3(
    zReader::Node *payload,
    int nodeIndex,
    int &outX,
    int &outY,
    int *outZ = 0
) {
    HudUiLayoutNode::ReadInt3(
        &payload[nodeIndex],
        &outX,
        &outY,
        outZ
    );
}
} // namespace


/**
 * Purpose: run the recovered HudUiWidget::DestructorCore teardown path.
 */
void HudUiWidget::DestructorCore() {
    this->~HudUiWidget();
}


namespace HudUiMgr {

} // namespace HudUiMgr

HudUiWidget::HudUiWidget(
    unsigned int initAlignFlags
) : HudUiElement(0, 0) {
    alignFlags = initAlignFlags;
    image = 0;
    ownsImage = 0;
    bltClipRectOrNull = 0;
    *((unsigned short *)(&imageStateWord)) = 0;
    dirtyRectCount = 0;

    {
        int dirtyRectIndex;
        for (dirtyRectIndex = 0; dirtyRectIndex < 4; ++dirtyRectIndex) {
            dirtyRects[dirtyRectIndex].framesRemaining = 0;
        }
    }
}

HudUiWidget::~HudUiWidget() {
    ReleaseImageIfOwned();
}

void HudUiWidget::ReleaseImageIfOwned() {
    if (image != 0 && ownsImage != 0) {
        zVid_Image::ReleaseIfNotDefault(image);
        image = 0;
    }

    ownsImage = 0;
}

void HudUiWidget::SetPos(
    int newX,
    int newY
) {
    if (alignFlags != 0 && image != 0) {
        x = newX - (image->width / 2);
        y = newY - (image->height / 2);
    } else {
        x = newX;
        y = newY;
    }

    Invalidate();
}

zVidImagePartial * HudUiWidget::SetImageByPathOwned(
    const char *imagePath
) {
    if (imagePath == 0) {
        return 0;
    }

    ReleaseImageIfOwned();
    image = zImage::TexDir_FindOrCreateByPath(imagePath);
    if (image != 0) {
        ownsImage = 1;
    }

    Invalidate();
    return image;
}

zVidImagePartial * HudUiWidget::SetImageBorrowedAndInvalidate(
    zVidImagePartial *newImage
) {
    ownsImage = 0;
    image = newImage;
    Invalidate();
    return newImage;
}

void HudUiWidget::InvalidateRect(
    const HudUiRect *dirtyRect
) {
    if (image == 0) {
        return;
    }

    HudUiRectDirty *slot = 0;
    {
        for (int index = 0; index < 4; ++index) {
            if (dirtyRects[index].framesRemaining == 0) {
                slot = &dirtyRects[index];
                break;
            }
        }
    }

    if (slot == 0) {
        return;
    }

    slot->srcLeft = dirtyRect->left;
    slot->srcTop = dirtyRect->top;
    slot->srcRight = dirtyRect->right;
    slot->srcBottom = dirtyRect->bottom;

    if (slot->srcLeft < x) {
        slot->srcLeft = x;
    }

    const int imageRight = image->width + x;
    if (slot->srcRight > imageRight) {
        slot->srcBottom = imageRight;
    }

    if (slot->srcTop < y) {
        slot->srcTop = y;
    }

    const int imageBottom = image->height + y;
    if (slot->srcBottom > imageBottom) {
        slot->srcBottom = imageBottom;
    }

    if (slot->srcRight <= slot->srcLeft || slot->srcBottom <= slot->srcTop) {
        return;
    }

    ++dirtyRectCount;
    slot->framesRemaining = (g_HudUi_InvalidateMask == 0x0c ? 1u : 0u) + 1u;
    slot->drawX = slot->srcLeft;
    slot->drawY = slot->srcTop;

    slot->srcLeft -= GetCenterX();
    slot->srcRight -= GetCenterX();
    slot->srcTop -= GetCenterY();
    slot->srcBottom -= GetCenterY();
    Invalidate();
}

void HudUiWidget::Draw() {
    if (image == 0) {
        return;
    }

    if (dirtyRectCount != 0) {
        int dirtyRectIndex;
        for (dirtyRectIndex = 0; dirtyRectIndex < 4; ++dirtyRectIndex) {
            HudUiRectDirty &dirtyRect = dirtyRects[dirtyRectIndex];
            if (dirtyRect.framesRemaining == 0) {
                continue;
            }

            zVid_Image::BlitToActiveTarget(
                image,
                dirtyRect.drawX,
                dirtyRect.drawY,
                0,
                (zVidRect32 *)(&dirtyRect.srcLeft)
            );

            --dirtyRect.framesRemaining;
            if (dirtyRect.framesRemaining == 0) {
                --dirtyRectCount;
            }
        }
        return;
    }

    if (g_HudUiWidget_ExclusiveDrawImage != 0 && g_HudUiWidget_ExclusiveDrawImage != image) {
        return;
    }

    DrawBase();

    zVid_Image::BlitToActiveTarget(
        image,
        x,
        y,
        0,
        (zVidRect32 *)(bltClipRectOrNull)
    );
}

int HudUiWidget::HitTest(
    int px,
    int py
) {
    if ((flags & 0x10) != 0) {
        return 0;
    }

    HudUiRect *const bounds = GetBoundsRectOrNull();
    if (bounds == 0) {
        return 0;
    }

    return px >= bounds->left && px <= bounds->right && py >= bounds->top && py <= bounds->bottom
               ? 1
               : 0;
}

/**
 * Purpose: Initializes the common HUD element position, links, timer, invalidation state, and blit source.
 */
HudUiElement::HudUiElement(
    int initX,
    int initY
) {
    HudUiElement *const element = this;
    parent = 0;
    next = 0;
    timer = 0.0f;
    element->x = initX;
    element->y = initY;
    element->Invalidate();

    flags = 0;
    state = 0;
    HudUiElement::SetBltSourceAndClipRect(
        0,
        0
    );
}

/**
 * Purpose: initialize the recovered HudUiElement::Constructor state.
 */
HudUiElement * HudUiElement::Constructor(
    int initX,
    int initY
) {
    new (this) HudUiElement(
        initX,
        initY
    );
    return this;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduielement-huduielement
 * @recoil-artifact defines .text recoil:function:0x4b40c0: HudUiElement::HudUiElement(const HudUiElement &).
 * Purpose: initialize a HUD element from another element while clearing owner links.
 */
HudUiElement::HudUiElement(
    const HudUiElement &source
) {
    next = 0;
    parent = 0;
    flags = source.flags;
    state = source.state;
    timer = source.timer;
    x = source.x;
    y = source.y;
    bltSource = source.bltSource;
    clipRect = source.clipRect;
}

/**
 * Purpose: copy another HUD element's runtime fields while preserving dispatch identity.
 */
HudUiElement & HudUiElement::operator=(
    const HudUiElement &source
) {
    next = 0;
    parent = 0;
    flags = source.flags;
    state = source.state;
    timer = source.timer;
    x = source.x;
    y = source.y;
    bltSource = source.bltSource;
    clipRect = source.clipRect;
    return *this;
}

/**
 * Purpose: reset the HudUiElement virtual table during class destruction.
 *
 * Evidence: the definition is kept inline in zhud_ui.h so VC5 can inline the
 * base table reset into derived destructors while still emitting the standalone
 * element destructor COMDAT when the address-backed symbol is required.
 */

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduielement-invalidate
 * @recoil-artifact defines .text recoil:function:0x4b4180: HudUiElement::Invalidate.
 * Purpose: mark the element dirty by OR-ing the current HUD invalidation mask into its flags.
 */
void HudUiElement::Invalidate() {
    flags |= g_HudUi_InvalidateMask;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduielement-setbltsourceandcliprect
 * @recoil-artifact defines .text recoil:function:0x4b4190: HudUiElement::SetBltSourceAndClipRect.
 * Purpose: apply the recovered HUD state change handled by HudUiElement::SetBltSourceAndClipRect.
 */
void HudUiElement::SetBltSourceAndClipRect(
    void *bltSourceOrNull,
    const HudUiRect *rectOrNull
) {
    bltSource = bltSourceOrNull;
    SetClipRect(rectOrNull);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduielement-setcliprect
 * @recoil-artifact defines .text recoil:function:0x4b41b0: HudUiElement::SetClipRect.
 * Purpose: replace the element clip rectangle when a source rectangle is supplied.
 * Binary Ninja: 0x4b41b0 returns immediately for a null argument; otherwise it
 * copies the four HudUiRect fields into the clipRect member at offset 0x20.
 */
void HudUiElement::SetClipRect(
    const HudUiRect *rect
) {
    if (rect == 0) {
        return;
    }

    clipRect = *rect;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduielement-update
 * @recoil-artifact defines .text recoil:function:0x4b41e0: HudUiElement::Update.
 * Purpose: dispatch visible or hidden dirty drawing and hide the element when its timer expires.
 */
void HudUiElement::Update(
    float deltaSeconds
) {
    unsigned int currentFlags = flags;

    if ((currentFlags & 0x10) == 0) {
        if ((currentFlags & 0x02) == 0) {
            Draw();
        } else if ((currentFlags & 0x04) != 0) {
            Draw();
            currentFlags = flags & ~0x04u;
            flags = currentFlags;
        } else if ((currentFlags & 0x08) != 0) {
            Draw();
            currentFlags = flags & ~0x08u;
            flags = currentFlags;
        }

        if ((flags & 0x01) != 0) {
            timer -= deltaSeconds;
            if (timer <= 0.0f) {
                SetVisible(0);
            }
        }
    } else if ((currentFlags & 0x02) != 0) {
        if ((currentFlags & 0x04) != 0) {
            DrawBase();
            flags &= ~0x04u;
        } else if ((currentFlags & 0x08) != 0) {
            DrawBase();
            flags &= ~0x08u;
        }
    }
}

/**
 * Recovered original helper slot with no standalone HudUiElement retail function.
 * Binary Ninja vtables for HudUiElement/HudUiZrdWidget/HudUiPanel and
 * HudUiNumericTextInput place a one-argument no-op provider target at slot
 * +0x28 between Update and GetBoundsRectOrNull.
 * Purpose: preserve the source-faithful HudUiElement virtual order used by
 * retail input/update dispatch without recreating table data under src/.
 */
void HudUiElement::OnUpdateIdle(
    float
) {}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduielement-settimer
 * @recoil-artifact defines .text recoil:function:0x4b4280: HudUiElement::SetTimer.
 * Purpose: set the element timer and update the timed-visible flag state.
 */
void HudUiElement::SetTimer(
    float duration
) {
    timer = duration;

    if (duration >= 0.0f) {
        flags |= 0x01u;
    } else {
        flags = (flags & ~0x01u) | 0x10u;
    }
}

/**
 * Purpose: apply the recovered HUD state change handled by HudUiPanel::SetClip.
 */

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduielement-gettextrect
 * @recoil-artifact defines .text recoil:function:0x4b42c0: HudUiElement::GetTextRect.
 * Purpose: fill a degenerate rectangle from the element position.
 * Binary Ninja: 0x4b42c0 dispatches the HudUiElement virtual GetCenterX and GetCenterY
 * methods from the base text-rectangle slot, then writes right/left and
 * bottom/top in that order.
 */
void HudUiElement::GetTextRect(
    HudUiRect *outRect
) {
    const int rectX = GetCenterX();
    outRect->right = rectX;
    outRect->left = rectX;

    const int rectY = GetCenterY();
    outRect->bottom = rectY;
    outRect->top = rectY;
}

HudUiTextInput::HudUiTextInput(
    int bufferSize
) {
    HudUiTextInput *const input = this;
    input->cursor = 0;
    input->buffer = 0;
    input->capacity = 0;
    input->AllocTextBuffer(bufferSize);

    {
        for (int code = 0; code < 0x100; ++code) {
            if (isprint(code) != 0) {
                input->keyActionMap[code] = 0;
            } else {
                input->keyActionMap[code] = 1;
            }
        }
    }

    input->keyActionMap[0x20] = 0;
    input->keyActionMap[0x2e] = 0;
    input->keyActionMap[0x1b] = 2;
    input->keyActionMap[0x0d] = 3;
    input->keyActionMap[0x08] = 4;
    input->keyActionMap[0x7f] = 5;
    input->keyActionMap[0x02] = 6;
    input->keyActionMap[0x06] = 7;
}

HudUiTextInput::~HudUiTextInput() {
    char *const ownedBuffer = buffer;
    ::operator delete(ownedBuffer);
}

void HudUiTextInput::AllocTextBuffer(
    int bufferSize
) {
    char *const newBuffer = (char *)(::operator new(bufferSize));
    char *const oldBuffer = buffer;
    if (oldBuffer != 0) {
        int copyCount = capacity;
        if (bufferSize < copyCount) {
            copyCount = bufferSize;
        }

        strncpy(
            newBuffer,
            oldBuffer,
            copyCount
        );
    }

    capacity = bufferSize;
    buffer = newBuffer;
}

void HudUiTextInput::SetContents(
    const char *source
) {
    strncpy(
        buffer,
        source,
        capacity
    );
    buffer[capacity - 1] = '\0';
    SetCursorPosition((int)(cursor));
}

char * HudUiTextInput::GetBuffer() {
    return buffer;
}

void HudUiTextInput::SetCursorPosition(
    int position
) {
    cursor =
        (position < (int)(strlen(buffer)))
            ? (unsigned int)(position)
            : (unsigned int)(strlen(buffer));
}

void HudUiTextInput::DispatchKeyAction(
    int key
) {
    const int keyIndex = (signed char)(key);
    const int action = (signed char)(keyActionMap[keyIndex]);

    switch (action) {
    case 0:
        OnPrintableKey(key);
        break;
    case 1:
        OnIgnoredKey(key);
        break;
    case 2:
        OnCancel();
        break;
    case 3:
        OnAccept();
        break;
    case 4:
        OnBackspace();
        break;
    case 5:
        OnDeleteForward();
        break;
    case 6:
        OnMoveCursorLeft();
        break;
    case 7:
        OnMoveCursorRight();
        break;
    default:
        break;
    }
}

void HudUiTextInput::InsertCharAtCursor(
    int ch
) {
    const int textLength = (int)(strlen(buffer));
    if (textLength < (int)(capacity)-1) {
        ShiftTextRight(
            1,
            (int)(cursor)
        );
        buffer[cursor] = (char)(ch);
        ++cursor;
    } else {
        OnOverflow();
    }
}

void HudUiTextInput::BackspaceDeleteChar() {
    if ((int)(cursor) > 0) {
        --cursor;
        ShiftTextLeft(
            1,
            (int)(cursor)
        );
    }
}

void HudUiTextInput::DeleteCharForward() {
    ShiftTextLeft(
        1,
        (int)(cursor)
    );
}

void HudUiTextInput::MoveCursorLeft() {
    if ((int)(cursor) > 0) {
        --cursor;
    }
}

void HudUiTextInput::MoveCursorRight() {
    const int textLength = (int)(strlen(buffer));
    if ((int)(cursor) < textLength) {
        ++cursor;
    }
}

int HudUiTextInput::ShiftTextRight(
    int count,
    int startPos
) {
    int index = (int)(strlen(buffer)) + count;
    if (index >= (int)(capacity)) {
        return 0;
    }

    while (index > startPos) {
        buffer[index] = buffer[index - count];
        --index;
    }

    return 1;
}

int HudUiTextInput::ShiftTextLeft(
    int count,
    int startPos
) {
    const int textLength = (int)(strlen(buffer));
    {
        for (int index = startPos; index < textLength; ++index) {
            buffer[index] = buffer[index + count];
        }
    }

    return 1;
}

HudUiSliderBorder::HudUiSliderBorder() {
    originX = 0;
    originY = 0;
    halfWidth = 1;
    height = 10;
    blinkEnabled = 0;
    blinkPeriodSec = 0.35f;
    blinkDirSign = 1;
    blinkTimeRemainingSec = 0.0f;

    SetPoint(0, -1, 0);
    SetPoint(1, halfWidth, 0);
    SetPoint(2, halfWidth, 1);
    SetPoint(3, 0, 1);
    SetPoint(4, 0, height - 1);
    SetPoint(5, halfWidth, height - 1);
    SetPoint(6, halfWidth, height);
    SetPoint(7, -halfWidth, height);
    SetPoint(8, -halfWidth, height - 1);
    SetPoint(9, 0, height - 1);
    SetPoint(10, 0, 1);
    SetPoint(11, -halfWidth, 1);
    SetPoint(12, -halfWidth, 0);
}

void HudUiSliderBorder::Update(
    float deltaSeconds
) {
    if ((flags & 0x10) != 0) {
        return;
    }

    if (blinkEnabled != 0) {
        const float nextTime = blinkTimeRemainingSec - deltaSeconds;
        blinkTimeRemainingSec = nextTime;
        if (nextTime < 0.0f) {
            blinkDirSign = -blinkDirSign;
            blinkTimeRemainingSec = blinkPeriodSec;
        }
    }

    if (blinkEnabled == 0 || blinkDirSign == 1) {
        HudUiPolyline::Draw();
    }
}

void HudUiSliderBorder::SetBounds(
    int newOriginX,
    int newOriginY,
    int newHalfWidth,
    int newHeight
) {
    originX = newOriginX;
    originY = newOriginY;
    halfWidth = newHalfWidth;
    height = newHeight;

    SetPoint(0, originX - halfWidth, originY);
    SetPoint(1, originX + halfWidth, originY);
    SetPoint(2, originX + halfWidth, originY + 1);
    SetPoint(3, originX, originY + 1);
    SetPoint(4, originX, originY + height - 1);
    SetPoint(5, originX + halfWidth, originY + height - 1);
    SetPoint(6, originX + halfWidth, originY + height);
    SetPoint(7, originX - halfWidth, originY + height);
    SetPoint(8, originX - halfWidth, originY + height - 1);
    SetPoint(9, originX, originY + height - 1);
    SetPoint(10, originX, originY + 1);
    SetPoint(11, originX - halfWidth, originY + 1);
    SetPoint(12, originX - halfWidth, originY);
}

HudUiNumericTextInput::HudUiNumericTextInput()
    : HudUiZrdWidget(),
      textInput(0x100),
      sliderBorder() {
    sliderBorder.sliderVisibleWhenInputActive = 0;
    sliderBorder.rawKeyFilterEnabled = 0;
    sliderBorder.inputActive = 1;
    sliderBorder.caretHalfWidth = 0;

    HudUiElement *sliderElement = &sliderBorder;
    sliderElement->SetVisible(1);
    HudUiNumericTextInput *ownerSelf = this;
    textInput.owner = ownerSelf;
    HudUiElement *element = ownerSelf;
    element->SetVisible(1);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduinumerictextinput-huduinumerictextinput
 * @recoil-artifact defines .text recoil:function:0x4b4ac0: HudUiNumericTextInput::~HudUiNumericTextInput.
 * Binary Ninja shows VC5 destructor codegen: derived vtable restore, raw
 * keyboard capture release, embedded HudUiOwnedTextInput teardown, then
 * HudUiZrdWidget cleanup with EH state transitions.
 * Purpose: Disable raw keyboard capture before C++ member/base destruction.
 */
HudUiNumericTextInput::~HudUiNumericTextInput() {
    SetRawKeyboardCapture(0);
}

int __fastcall HudUiNumericTextInput::RawKeyboardCallback(
    int key,
    HudUiNumericTextInput *callbackCtx
) {
    if (callbackCtx != 0) {
        return callbackCtx->OnRawKeyboardChar(key);
    }

    return 0;
}

int HudUiNumericTextInput::OnRawKeyboardChar(
    int key
) {
    if (sliderBorder.rawKeyFilterEnabled != 0) {
        if (strchr(
            kNumericTextInputAcceptedRawKeyChars,
            key
        ) == 0) {
            return 0;
        }
        textInput.DispatchKeyAction(key);
        return 0;
    }

    textInput.DispatchKeyAction(key);
    return 0;
}

int HudUiNumericTextInput::SetInputActive(
    int active
) {
    HudUiPanel *firstLabelPanel = 0;
    const int previousActive = sliderBorder.inputActive;
    sliderBorder.inputActive = active;

    const int labelPanelCount = (int)(labelPanels.size());
    unsigned char labelPanelsEmpty = labelPanelCount == 0;
    if (labelPanelsEmpty == 0) {
        firstLabelPanel = labelPanels[0];
    }

    if (active != 0) {
        SetVisible(1);
        sliderBorder.SetVisible(1);
        if (firstLabelPanel != 0) {
            firstLabelPanel->SetVisible(1);
        }
    } else {
        SetVisible(0);
        if (firstLabelPanel != 0) {
            firstLabelPanel->SetVisible(0);
        }
        sliderBorder.SetVisible(0);
    }

    return previousActive;
}

void HudUiNumericTextInput::SetRawKeyboardCapture(
    int enable
) {
    const char enableByte = (char)(enable);
    if (enableByte == sliderBorder.sliderVisibleWhenInputActive) {
        return;
    }

    sliderBorder.sliderVisibleWhenInputActive = enableByte;
    if (enableByte != 0) {
        zInput::Keyboard_SetRawEventCallback(
            (void *)(&HudUiNumericTextInput::RawKeyboardCallback),
            this
        );
    } else {
        zInput::Keyboard_SetRawEventCallback(
            0,
            0
        );
    }
}

void HudUiNumericTextInput::OnActivate() {
    sliderBorder.inputActive = 1;
    HudUiZrdWidget::OnActivate();
}

RECOIL_NO_GS void HudUiNumericTextInput::UpdateCaptureUiAndClip(
    float deltaSeconds
) {
    if ((flags & 0x10) != 0) {
        labelPanels[0]->SetVisible(0);
        labelPanels[0]->Invalidate();
        sliderBorder.SetVisible(0);
        sliderBorder.Invalidate();
        return;
    }

    if (sliderBorder.sliderVisibleWhenInputActive != 0) {
        labelPanels[0]->SetVisible(1);
        char *const buffer = textInput.GetBuffer();

        if (labelPanels.begin() != labelPanels.end()) {
            const ptrdiff_t panelCount = labelPanels.end() - labelPanels.begin();
            if (panelCount != 0) {
                labelPanels[0]->SetText(buffer);
            }
        }

        RECT textRect = {0};
        textRect.left = labelPanels[0]->GetCenterX();
        textRect.top = labelPanels[0]->GetCenterY();
        textRect.right = labelPanels[0]->GetCenterX();
        textRect.bottom = labelPanels[0]->GetCenterY();

        if (labelPanels[0]->MeasureTextPrefixRect(
            (int)(textInput.cursor),
            &textRect
        ) != 0) {
            const unsigned int textColor = labelPanels[0]->textColor0;
            const unsigned int packedColor = zVid_PackColorRGB(
                                                 (unsigned char)(textColor),
                                                 (unsigned char)(textColor >> 8),
                                                 (unsigned char)(textColor >> 16)
                                             ) &
                                             0xffffu;
            sliderBorder.color565 = (int)(packedColor);
            sliderBorder.SetBounds(
                textRect.right,
                textRect.top,
                sliderBorder.caretHalfWidth,
                textRect.bottom - textRect.top
            );
            sliderBorder.SetVisible(1);
        }

        Invalidate();
    } else {
        sliderBorder.SetVisible(0);
    }

    HudUiElement::Update(deltaSeconds);
    sliderBorder.Update(deltaSeconds);
}

void HudUiNumericTextInput::AllocTextBuffer(
    unsigned int bufferSize
) {
    textInput.AllocTextBuffer(bufferSize);
}

void HudUiNumericTextInput::Update(
    const char *text
) {
    textInput.SetContents(text);
    textInput.SetCursorPosition((int)(strlen(text)));
    char *const buffer = textInput.GetBuffer();

    if (labelPanels.size() != 0) {
        HudUiPanel *const firstPanel = labelPanels[0];
        firstPanel->SetText(buffer);
    }

    Invalidate();
}

char * HudUiNumericTextInput::GetBuffer() {
    return textInput.GetBuffer();
}

HudUiZrdWidget::HudUiZrdWidget() : HudUiWidget(0) {
    modeOrEnabled = 1;
    originY = 0;
    originX = 0;
    owner = 0;
    defaultImage = 0;
    rolloverImage = 0;
    disabledImage = 0;
    rolloverSound = 0;
    rolloverSoundScale = 1.0f;
    rolloverPlayHandle = 0;
    activateImage = 0;
    activateSound = 0;
    activateSoundScale = 1.0f;
    activatePlayHandle = 0;

    labelPanels.clear();
    rolloverLabelPanels.clear();
    activateLabelPanels.clear();
    (void)labelPanels.empty();
    (void)rolloverLabelPanels.empty();

    *((unsigned short *)(&imageStateWord)) = 1;
    HudUiElement *element = this;
    element->Invalidate();
    unsigned int visibleFlag = (unsigned char)(flags);
    flags = (visibleFlag & 0x10u) | 0x02u;
}

HudUiZrdWidget::~HudUiZrdWidget() {
    {
        HudUiZrdDeleteChildIfPresent deleteChildIfPresent;
        HudUiZrdDeleteChildIfPresent deleteChildIfPresentCopy(
            deleteChildIfPresent
        );
        std::transform(
            labelPanels.begin(),
            labelPanels.end(),
            labelPanels.begin(),
            deleteChildIfPresentCopy
        );
    }

    {
        HudUiPanelPtrVector::iterator rolloverIt = rolloverLabelPanels.begin();
        HudUiPanelPtrVector::iterator rolloverOut = rolloverLabelPanels.begin();
        HudUiPanelPtrVector::iterator rolloverEnd = rolloverLabelPanels.end();
        while (rolloverIt != rolloverEnd) {
            if (*rolloverIt != 0) {
                delete (*rolloverIt);
            }

            *rolloverOut = 0;
            ++rolloverIt;
            ++rolloverOut;
        }
    }

    {
        HudUiPanelPtrVector::iterator activateIt = activateLabelPanels.begin();
        HudUiPanelPtrVector::iterator activateOut = activateLabelPanels.begin();
        HudUiPanelPtrVector::iterator activateEnd = activateLabelPanels.end();
        while (activateIt != activateEnd) {
            if (*activateIt != 0) {
                delete (*activateIt);
            }

            *activateOut = 0;
            ++activateIt;
            ++activateOut;
        }
    }

    typedef HudUiPanelPtrVector::iterator (
        HudUiPanelPtrVector::*ErasePanelRangeMethod
    )(
        HudUiPanelPtrVector::iterator,
        HudUiPanelPtrVector::iterator
    );
    ErasePanelRangeMethod erasePanelRange =
        (ErasePanelRangeMethod)(&HudUiPanelPtrVector::erase);
    (labelPanels.*erasePanelRange)(labelPanels.begin(), labelPanels.end());
    (rolloverLabelPanels.*erasePanelRange)(
        rolloverLabelPanels.begin(),
        rolloverLabelPanels.end()
    );
    (activateLabelPanels.*erasePanelRange)(
        activateLabelPanels.begin(),
        activateLabelPanels.end()
    );

    if (defaultImage != 0 && defaultImage != image) {
        defaultImage = (zVidImagePartial *)(unsigned int)zVid_Image::ReleaseIfNotDefault(defaultImage);
    }

    if (activateImage != 0 && activateImage != image) {
        activateImage = (zVidImagePartial *)(unsigned int)zVid_Image::ReleaseIfNotDefault(activateImage);
    }

    if (rolloverImage != 0 && rolloverImage != image) {
        rolloverImage = (zVidImagePartial *)(unsigned int)zVid_Image::ReleaseIfNotDefault(rolloverImage);
    }

    if (disabledImage != 0 && disabledImage != image) {
        disabledImage = (zVidImagePartial *)(unsigned int)zVid_Image::ReleaseIfNotDefault(disabledImage);
    }

    if (image != 0 && ownsImage == 0) {
        zVid_Image::ReleaseIfNotDefault(image);
    }
}

void *__stdcall HudUiZrdWidget::DeleteChildIfPresent(
    void *childWidgetOrNull
) {
    if (childWidgetOrNull != 0) {
        delete ((HudUiElement *)(childWidgetOrNull));
    }

    return 0;
}

void HudUiZrdWidget::Invalidate() {
    HudUiElement::Invalidate();

    HudUiPanelPtrVector::iterator panel = labelPanels.begin();
    if (panel == labelPanels.end()) {
        return;
    }

    while (panel != labelPanels.end()) {
        HudUiPanel *const label = *panel;
        label->Invalidate();
        ++panel;
    }
}

HudUiRect * HudUiZrdWidget::GetBoundsRectOrNull() {
    if (modeOrEnabled == 0) {
        return 0;
    }

    if (image != 0) {
        boundsRect.left = x;
        boundsRect.top = y;
        boundsRect.right = x + image->width;
        boundsRect.bottom = y + image->height;
        return &boundsRect;
    }

    HudUiPanelPtrVector::iterator panelIt = labelPanels.begin();
    if (panelIt == labelPanels.end()) {
        return 0;
    }

    boundsRect.top = labelPanels[0]->GetCenterY();
    boundsRect.bottom =
        boundsRect.top + labelPanels[0]->QueryTextHeight();

    while (panelIt != labelPanels.end()) {
        boundsRect.bottom += (*panelIt)->QueryTextHeight();

        const int alignMode = (*panelIt)->alignMode;
        switch (alignMode) {
        case 0:
            boundsRect.left = labelPanels[0]->GetCenterX();
            if ((*panelIt)->GetCenterX() + (*panelIt)->QueryTextWidth() >
                boundsRect.right) {
                boundsRect.right =
                    (*panelIt)->GetCenterX() + (*panelIt)->QueryTextWidth();
            }
            break;

        case 1:
            if ((*panelIt)->GetCenterX() - ((*panelIt)->QueryTextWidth() / 2) <
                boundsRect.left) {
                boundsRect.left =
                    (*panelIt)->GetCenterX() - ((*panelIt)->QueryTextWidth() / 2);
            }

            if ((*panelIt)->GetCenterX() + ((*panelIt)->QueryTextWidth() / 2) >
                boundsRect.right) {
                boundsRect.right =
                    (*panelIt)->GetCenterX() + ((*panelIt)->QueryTextWidth() / 2);
            }
            break;

        case 2:
            boundsRect.right = labelPanels[0]->GetCenterX();
            if ((*panelIt)->GetCenterX() - (*panelIt)->QueryTextWidth() >
                boundsRect.left) {
                boundsRect.left = boundsRect.left;
            } else {
                boundsRect.left =
                    (*panelIt)->GetCenterX() - (*panelIt)->QueryTextWidth();
            }
            break;
        }

        ++panelIt;
    }

    boundsRect.bottom -= labelPanels[0]->QueryTextHeight();
    return &boundsRect;
}

void HudUiZrdWidget::ShowPreview() {
    if (rolloverImage != 0) {
        if (defaultImage == 0) {
            defaultImage = image;
        }

        SetImageBorrowedAndInvalidate(rolloverImage);
    }

    if (rolloverSound != 0) {
        rolloverPlayHandle = rolloverSound->PlayA3DSimple(rolloverSoundScale);
    }

    if (rolloverLabelPanels.begin() != rolloverLabelPanels.end()) {
        HudUiSetPanelVectorVisible(labelPanels, 0);
        HudUiSetPanelVectorVisible(activateLabelPanels, 0);
        HudUiSetPanelVectorVisible(rolloverLabelPanels, 1);
        return;
    }

    HudUiSetPanelVectorVisible(labelPanels, 1);
    HudUiSetPanelVectorVisible(activateLabelPanels, 0);
}

void HudUiZrdWidget::RefreshState() {
    for (HudUiPanelPtrVector::iterator rolloverIt = rolloverLabelPanels.begin();
        rolloverIt != rolloverLabelPanels.end();
        ++rolloverIt) {
        (*rolloverIt)->SetVisible(0);
    }

    for (HudUiPanelPtrVector::iterator activateIt = activateLabelPanels.begin();
        activateIt != activateLabelPanels.end();
        ++activateIt) {
        (*activateIt)->SetVisible(0);
    }

    if (modeOrEnabled != 0) {
        for (HudUiPanelPtrVector::iterator labelIt = labelPanels.begin();
            labelIt != labelPanels.end(); ++labelIt) {
            (*labelIt)->SetVisible(1);
        }
        for (HudUiPanelPtrVector::iterator disabledIt = disabledLabelPanels.begin();
            disabledIt != disabledLabelPanels.end(); ++disabledIt) {
            (*disabledIt)->SetVisible(0);
        }
        SetImageBorrowedAndInvalidate(defaultImage);
        Invalidate();
        return;
    }

    for (HudUiPanelPtrVector::iterator labelIt2 = labelPanels.begin();
        labelIt2 != labelPanels.end(); ++labelIt2) {
        (*labelIt2)->SetVisible(0);
    }
    for (HudUiPanelPtrVector::iterator disabledIt2 = disabledLabelPanels.begin();
        disabledIt2 != disabledLabelPanels.end(); ++disabledIt2) {
        (*disabledIt2)->SetVisible(1);
    }

    SetImageBorrowedAndInvalidate(disabledImage);
    Invalidate();
}

void HudUiZrdWidget::HidePreview() {
    if (defaultImage != 0) {
        SetImageBorrowedAndInvalidate(defaultImage);
    }

    if (rolloverPlayHandle != 0) {
        rolloverPlayHandle = 0;
    }

    for (HudUiPanelPtrVector::iterator rolloverIt = rolloverLabelPanels.begin();
        rolloverIt != rolloverLabelPanels.end();
        ++rolloverIt) {
        (*rolloverIt)->SetVisible(0);
    }
    for (HudUiPanelPtrVector::iterator activateIt = activateLabelPanels.begin();
        activateIt != activateLabelPanels.end();
        ++activateIt) {
        (*activateIt)->SetVisible(0);
    }
    for (HudUiPanelPtrVector::iterator labelIt = labelPanels.begin();
        labelIt != labelPanels.end(); ++labelIt) {
        (*labelIt)->SetVisible(1);
    }
}

void HudUiZrdWidget::OnActivate() {
    zInput::ResetAllTransitionState();

    if (activateImage != 0) {
        SetImageBorrowedAndInvalidate(activateImage);
    }

    if (rolloverPlayHandle != 0) {
        rolloverPlayHandle->StopIfActive();
        rolloverPlayHandle = 0;
    }

    if (activateSound != 0) {
        activatePlayHandle = activateSound->PlayA3DSimple(activateSoundScale);
    }

    HudUiSetPanelVectorVisible(rolloverLabelPanels, 0);

    if (activateLabelPanels.begin() != activateLabelPanels.end()) {
        HudUiSetPanelVectorVisible(activateLabelPanels, 1);
        HudUiSetPanelVectorVisible(labelPanels, 0);
        return;
    }

    HudUiSetPanelVectorVisible(labelPanels, 1);
}

/**
 * Purpose: attach the widget to its owning dialog and load its base ZRD
 * position, bitmap, sound, and label state.
 */
int HudUiZrdWidget::LoadFromZrd(
    zReader::Node *zrdSection,
    HudUiBackground *ownerDialog
) {
    owner = ownerDialog;
    SetVisible(1);
    ((HudUiContainer *)(ownerDialog))->AddChild(this);

    originX = ownerDialog->uiOriginX;
    originY = ownerDialog->uiOriginY;
    if (zrdSection == 0) {
        return 0;
    }

    zReader::Node *const positionNode = zReader_GetNamedNode(
        zrdSection,
        g_HudZrd_Key_Position
    );
    if (positionNode != 0) {
        zReader::Node *const positionBase = positionNode->value.nodes;
        originX += positionBase[1].value.i32;
        originY += positionBase[2].value.i32;
    }

    int widgetX = originX;
    int widgetY = originY;
    zReader::Node *const bitmapNode = zReader_GetNamedNode(
        zrdSection,
        g_HudUiCycleSelectorWidget_ZrdKey_Bitmap
    );
    if (bitmapNode != 0) {
        zReader::Node *const bitmapBase = bitmapNode->value.nodes;
        const char *const bitmapPath = bitmapBase[1].value.str;
        if (bitmapPath != 0) {
            defaultImage = SetImageByPathOwned(bitmapPath);
            if (bitmapBase[0].value.i32 >= 4) {
                widgetX += bitmapBase[2].value.i32;
                widgetY += bitmapBase[3].value.i32;
            }
        }
    }

    SetPos(
        widgetX,
        widgetY
    );

    void *const clipSource = ownerDialog->capturedCompositeImage;
    if (clipSource != 0) {
        HudUiRect *const bounds = GetBoundsRectOrNull();
        if (bounds != 0) {
            SetBltSourceAndClipRect(
                clipSource,
                bounds
            );
        }
    }

    zReader::Node *const rolloverNode = zReader_GetNamedNode(
        zrdSection,
        g_HudZrd_Key_Rollover
    );
    if (rolloverNode != 0) {
        HUD_ZRD_LOAD_BITMAP(
            rolloverNode,
            g_HudUiCycleSelectorWidget_ZrdKey_Bitmap,
            rolloverImage
        );
        HUD_ZRD_LOAD_SOUND(
            rolloverNode,
            rolloverSound,
            rolloverSoundScale
        );
        LoadHudZrdExpandedLabelSection(
            this,
            rolloverNode,
            rolloverLabelPanels
        );
        HUD_ZRD_APPLY_FLASH_SECTION(
            rolloverNode,
            rolloverLabelPanels
        );
    }

    zReader::Node *const disableNode = zReader_GetNamedNode(
        zrdSection,
        g_HudZrd_Key_Disable
    );
    if (disableNode != 0) {
        HUD_ZRD_LOAD_BITMAP(
            disableNode,
            g_HudUiCycleSelectorWidget_ZrdKey_Bitmap,
            disabledImage
        );
        HUD_ZRD_LOAD_SOUND(
            disableNode,
            disabledSound,
            disabledSoundScale
        );
        LoadHudZrdExpandedLabelSection(
            this,
            disableNode,
            disabledLabelPanels
        );
        HUD_ZRD_APPLY_FLASH_SECTION(
            disableNode,
            disabledLabelPanels
        );
    }

    zReader::Node *const activateNode = zReader_GetNamedNode(
        zrdSection,
        g_HudZrd_Key_Activate
    );
    if (activateNode != 0) {
        HUD_ZRD_LOAD_BITMAP(
            activateNode,
            g_HudUiCycleSelectorWidget_ZrdKey_Bitmap,
            activateImage
        );
        HUD_ZRD_LOAD_SOUND(
            activateNode,
            activateSound,
            activateSoundScale
        );
        LoadHudZrdExpandedLabelSection(
            this,
            activateNode,
            activateLabelPanels
        );
        HUD_ZRD_APPLY_FLASH_SECTION(
            activateNode,
            activateLabelPanels
        );
    }

    zReader::Node *const baseLabelNode = zReader_GetNamedNode(
        zrdSection,
        g_HudZrd_Key_Label
    );
    if (baseLabelNode != 0) {
        zReader::Node *const baseLabel = baseLabelNode->value.nodes;
        if (!LoadHudZrdExpandedLabelArray(this, baseLabel, labelPanels)) {
            HUD_ZRD_APPEND_LABEL(
                this,
                labelPanels,
                baseLabel,
                originX,
                originY
            );
        }
    }
    HUD_ZRD_APPLY_FLASH_SECTION(
        zrdSection,
        labelPanels
    );
    return 1;
}

/**
 * Function modeled here:
 * Purpose: initialize the toggle widget's unchecked, checked, label, and
 * disabled-state members over the ZRD widget base.
 */
HudUiCheckToggleWidget::HudUiCheckToggleWidget() : HudUiZrdWidget() {
    checked = 0;
    uncheckedImage = 0;
    checkedImage = 0;
    checkedLabelPanel = 0;
    disabledCheckedImage = 0;
    disabledCheckedFallbackImage = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduichecktogglewidget-huduichecktogglewidget
 * @recoil-artifact defines .text recoil:function:0x4b7020: HudUiCheckToggleWidget::~HudUiCheckToggleWidget.
 * Purpose: Restore the unchecked image, delete owned checked state, and tear down the ZRD widget base.
 */
HudUiCheckToggleWidget::~HudUiCheckToggleWidget() {
    SetImageBorrowedAndInvalidate(uncheckedImage);

    if (checkedImage != 0) {
        ::operator delete(checkedImage);
        checkedImage = 0;
    }

    if (checkedLabelPanel != 0) {
        delete checkedLabelPanel;
        checkedLabelPanel = 0;
    }
}

/**
 * Provider boundary 0x40cf30: VC5 compiler/EH cleanup forwarding thunk.
 * Source compatibility wrapper for recovered callers that historically named
 * the destructor body DestructorCore. The physical row is not a standalone
 * authored body.
 * Purpose: Run the check-toggle destructor body.
 */
void HudUiCheckToggleWidget::DestructorCore() {
    this->HudUiCheckToggleWidget::~HudUiCheckToggleWidget();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduichecktogglewidget-getboundsrectornull
 * @recoil-artifact defines .text recoil:function:0x4b70b0: HudUiCheckToggleWidget::GetBoundsRectOrNull.
 * Purpose: return the recovered HUD value exposed by HudUiCheckToggleWidget::GetBoundsRectOrNull.
 */
HudUiRect * HudUiCheckToggleWidget::GetBoundsRectOrNull() {
    return &boundsRect;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduichecktogglewidget-refreshstate
 * @recoil-artifact defines .text recoil:function:0x4b70c0: HudUiCheckToggleWidget::RefreshState.
 * Purpose: preserve the recovered HUD behavior for HudUiCheckToggleWidget::RefreshState.
 */
void HudUiCheckToggleWidget::RefreshState() {
    HudUiSetPanelVectorVisible(
        rolloverLabelPanels,
        0
    );
    HudUiSetPanelVectorVisible(
        activateLabelPanels,
        0
    );

    zVidImagePartial *image = 0;
    if (modeOrEnabled != 0) {
        HudUiSetPanelVectorVisible(
            labelPanels,
            1
        );
        HudUiSetPanelVectorVisible(
            disabledLabelPanels,
            0
        );

        if (checked != 0) {
            image = checkedImage;
            if (image == 0) {
                image = uncheckedImage;
                if (image != 0) {
                    SetImageBorrowedAndInvalidate(image);
                    Invalidate();
                    return;
                }
            }
        }
    } else {
        HudUiSetPanelVectorVisible(
            labelPanels,
            0
        );
        HudUiSetPanelVectorVisible(
            disabledLabelPanels,
            1
        );

        if (checked != 0) {
            image = disabledCheckedImage;
            if (image == 0) {
                image = disabledCheckedFallbackImage;
            }
        }
    }

    if (image != 0) {
        SetImageBorrowedAndInvalidate(image);
    }
    Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduichecktogglewidget-showpreview
 * @recoil-artifact defines .text recoil:function:0x4b7210: HudUiCheckToggleWidget::ShowPreview.
 * Purpose: preserve the recovered HUD behavior for HudUiCheckToggleWidget::ShowPreview.
 */
void HudUiCheckToggleWidget::ShowPreview() {
    if (modeOrEnabled == 0 || checked != 0) {
        return;
    }

    if (rolloverSound != 0) {
        rolloverPlayHandle = rolloverSound->PlayA3DSimple(rolloverSoundScale);
    }

    HudUiZrdWidget::ShowPreview();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduichecktogglewidget-hidepreview
 * @recoil-artifact defines .text recoil:function:0x4b7250: HudUiCheckToggleWidget::HidePreview.
 * Purpose: preserve the recovered HUD behavior for HudUiCheckToggleWidget::HidePreview.
 */
void HudUiCheckToggleWidget::HidePreview() {
    if (modeOrEnabled == 0 || checked != 0) {
        return;
    }

    if (rolloverPlayHandle != 0) {
        rolloverPlayHandle->StopIfActive();
        rolloverPlayHandle = 0;
    }

    HudUiZrdWidget::HidePreview();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduichecktogglewidget-onactivate
 * @recoil-artifact defines .text recoil:function:0x4b7290: HudUiCheckToggleWidget::OnActivate.
 * Purpose: handle the recovered HUD event path for HudUiCheckToggleWidget::OnActivate.
 */
void HudUiCheckToggleWidget::OnActivate() {
    if (modeOrEnabled == 0) {
        return;
    }

    SetChecked(checked == 0 ? 1 : 0);
    HudUiZrdWidget::OnActivate();
}

/**
 * Purpose: handle the recovered HUD event path for HudUiCheckToggleWidget::OnActivateThunk.
 */
void HudUiCheckToggleWidget::OnActivateThunk() {
    OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduichecktogglewidget-setchecked
 * @recoil-artifact defines .text recoil:function:0x4b72c0: HudUiCheckToggleWidget::SetChecked.
 * Purpose: apply the recovered HUD state change handled by
 */
int HudUiCheckToggleWidget::SetChecked(
    int newChecked
) {
    const int previousChecked = checked;
    checked = newChecked;

    if (newChecked != 0) {
        if (checkedImage != 0) {
            SetImageBorrowedAndInvalidate(checkedImage);
        }

        if (checkedLabelPanel != 0) {
            checkedLabelPanel->SetVisible(1);
            Invalidate();
            return previousChecked;
        }
    } else {
        if (uncheckedImage != 0) {
            SetImageBorrowedAndInvalidate(uncheckedImage);
        }

        if (checkedLabelPanel != 0) {
            checkedLabelPanel->SetVisible(0);
        }
    }

    Invalidate();
    return previousChecked;
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduichecktogglewidget-loadfromzrd
 * @recoil-artifact defines .text recoil:function:0x4b7340: HudUiCheckToggleWidget::LoadFromZrd.
 * Purpose: load the base widget and its checked-state bitmap, label, and
 * disabled fallback resources from the owning ZRD section.
 */
int HudUiCheckToggleWidget::LoadFromZrd(
    zReader::Node *zrdSection,
    HudUiBackground *ownerDialog
) {
    HudUiZrdWidget::LoadFromZrd(
        zrdSection,
        ownerDialog
    );
    uncheckedImage = image;

    zReader::Node *const checkedNode = zReader_GetNamedNode(
        zrdSection,
        g_HudUiZrdKey_Checked
    );
    if (checkedNode != 0) {
        LoadHudZrdBitmap(
            checkedNode,
            g_HudUiCycleSelectorWidget_ZrdKey_Bitmap,
            &checkedImage
        );
        zReader::Node *const textNode = zReader_GetNamedNode(
            checkedNode,
            g_HudUiCycleSelectorWidget_ZrdKey_Text
        );
        if (textNode != 0) {
            checkedLabelPanel = CreateHudZrdTextPanel(
                this,
                textNode,
                0
            );
        }
    }

    zReader::Node *const disabledUnselectedNode = zReader_GetNamedNode(
        zrdSection,
        g_HudUiZrdKey_DisableUnsel
    );
    if (disabledUnselectedNode != 0) {
        LoadHudZrdBitmap(
            disabledUnselectedNode,
            g_HudUiCycleSelectorWidget_ZrdKey_Bitmap,
            &disabledCheckedFallbackImage
        );
        zReader::Node *const textNode = zReader_GetNamedNode(
            disabledUnselectedNode,
            g_HudUiCycleSelectorWidget_ZrdKey_Text
        );
        if (textNode != 0) {
            checkedLabelPanel = CreateHudZrdTextPanel(
                this,
                textNode,
                0
            );
        }

        HUD_ZRD_LOAD_LABEL_SECTION_NATURAL(
            this,
            zrdSection,
            disabledLabelPanels
        );
    }

    zReader::Node *const disabledSelectedNode = zReader_GetNamedNode(
        zrdSection,
        g_HudUiZrdKey_DisableSel
    );
    if (disabledSelectedNode != 0) {
        LoadHudZrdBitmap(
            disabledSelectedNode,
            g_HudUiCycleSelectorWidget_ZrdKey_Bitmap,
            &disabledCheckedImage
        );
        zReader::Node *const textNode = zReader_GetNamedNode(
            disabledSelectedNode,
            g_HudUiCycleSelectorWidget_ZrdKey_Text
        );
        if (textNode != 0) {
            checkedLabelPanel = CreateHudZrdTextPanel(
                this,
                textNode,
                0
            );
        }
    }

    if (uncheckedImage != 0) {
        boundsRect.left = x;
        boundsRect.top = y;
        boundsRect.right = x + uncheckedImage->width;
        boundsRect.bottom = y + uncheckedImage->height;
    } else if (labelPanels.begin() != labelPanels.end()) {
        HudUiPanelPtrVector::iterator panelIt = labelPanels.begin();
        HudUiPanel *const firstPanel = *panelIt;
        boundsRect.top = firstPanel->GetCenterY();
        boundsRect.left = firstPanel->GetCenterX();
        boundsRect.bottom = firstPanel->QueryTextHeight() + boundsRect.top;

        while (panelIt != labelPanels.end()) {
            HudUiPanel *const panel = *panelIt;
            boundsRect.bottom += panel->QueryTextHeight();

            if (panel->QueryTextWidth() + boundsRect.left > boundsRect.right) {
                boundsRect.right =
                    panel->QueryTextWidth() + boundsRect.left;
            }

            ++panelIt;
        }

        boundsRect.bottom -= firstPanel->QueryTextHeight();
    }

    return 1;
}

/**
 * Purpose: preserve the recovered HUD behavior for HudUiCycleSelectorWidget::HudUiCycleSelectorWidget.
 */
HudUiCycleSelectorWidget::HudUiCycleSelectorWidget() : HudUiZrdWidget() {
    selectedIndex = 0;
    itemCount = 0;
    for (int i = 0; i < 20; ++i) {
        entriesA[i] = 0;
        entriesB[i] = 0;
    }

    firstIndex = 0;
    visibleCount = 20;
    fontStyleRef = 0;
    textOffsetY = 0;
    textOffsetX = 0;
}

/**
 * Purpose: initialize the recovered HudUiCycleSelectorWidget::Constructor state.
 */
HudUiCycleSelectorWidget * HudUiCycleSelectorWidget::Constructor() {
    new (this) HudUiCycleSelectorWidget;
    return this;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicycleselectorwidget-huduicycleselectorwidget
 * @recoil-artifact defines .text recoil:function:0x4b7de0: HudUiCycleSelectorWidget::~HudUiCycleSelectorWidget.
 * Purpose: Delete paired selector entry widgets and tear down the ZRD widget base.
 */
HudUiCycleSelectorWidget::~HudUiCycleSelectorWidget() {
    for (int i = 0; i < 20; ++i) {
        if (entriesA[i] != 0) {
            delete entriesA[i];
            entriesA[i] = 0;
        }

        if (entriesB[i] != 0) {
            delete entriesB[i];
            entriesB[i] = 0;
        }
    }
}

/**
 * Provider boundary 0x40cf40: VC5 compiler/EH cleanup forwarding thunk.
 * Source compatibility wrapper for recovered callers that historically named
 * the destructor body DestructorCore. The physical row is not a standalone
 * authored body.
 * Purpose: Run the cycle-selector destructor body.
 */
void HudUiCycleSelectorWidget::DestructorCore() {
    this->HudUiCycleSelectorWidget::~HudUiCycleSelectorWidget();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicycleselectorwidget-update
 * @recoil-artifact defines .text recoil:function:0x4b7e60: HudUiCycleSelectorWidget::Update.
 * Purpose: advance the recovered HUD update path for HudUiCycleSelectorWidget::Update.
 */
void HudUiCycleSelectorWidget::Update(
    float deltaSeconds
) {
    for (int i = 0; i < itemCount; ++i) {
        Invalidate();

        if (entriesA[i] != 0) {
            if (i == selectedIndex) {
                entriesA[i]->SetVisible(1);
            } else {
                entriesA[i]->SetVisible(0);
            }
        }

        if (entriesB[i] != 0) {
            if (i == selectedIndex) {
                entriesB[i]->SetVisible(1);
            } else {
                entriesB[i]->SetVisible(0);
            }
        }
    }

    HudUiElement::Update(deltaSeconds);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicycleselectorwidget-advanceselectionandactivate
 * @recoil-artifact defines .text recoil:function:0x4b7ee0: HudUiCycleSelectorWidget::AdvanceSelectionAndActivate.
 * Purpose: Advance the selected cycle entry, wrap at the visible/item limit,
 * and run the base ZRD activation path.
 */
void HudUiCycleSelectorWidget::AdvanceSelectionAndActivate() {
    const int nextIndex = selectedIndex + 1;
    selectedIndex = nextIndex;

    int endIndex = visibleCount;
    if (endIndex >= itemCount) {
        endIndex = itemCount;
    }

    if (nextIndex >= endIndex) {
        selectedIndex = firstIndex;
    }

    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicycleselectorwidget-setindexclamped
 * @recoil-artifact defines .text recoil:function:0x4b7f20: HudUiCycleSelectorWidget::SetIndexClamped.
 *
 * Purpose: clamp a requested cycle-selector index and return the previous
 * selected index.
 *
 * Evidence: BN assembly at 0x4b7f20 loads selectedIndex into eax before the
 * clamp branches, compares the requested index against firstIndex, itemCount,
 * and visibleCount, writes selectedIndex to the clamped value, and returns with
 * eax preserved.
 */
int HudUiCycleSelectorWidget::SetIndexClamped(
    int index
) {
    const int previousIndex = selectedIndex;

    if (index < firstIndex) {
        selectedIndex = firstIndex;
        return previousIndex;
    }

    if (index >= itemCount) {
        selectedIndex = itemCount - 1;
        return previousIndex;
    }

    if (index >= visibleCount) {
        selectedIndex = visibleCount - 1;
        return previousIndex;
    }

    selectedIndex = index;
    return previousIndex;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicycleselectorwidget-setvisiblerange
 * @recoil-artifact defines .text recoil:function:0x4b7f80: HudUiCycleSelectorWidget::SetVisibleRange.
 * Purpose: set the visible selector range and clamp the selected entry into it.
 */
void HudUiCycleSelectorWidget::SetVisibleRange(
    int first,
    int last
) {
    if (first >= 0 && first < itemCount) {
        firstIndex = first;
    }

    if (last >= first && last < itemCount) {
        visibleCount = last;
    }

    if (selectedIndex < first) {
        selectedIndex = first;
    }

    if (selectedIndex >= last) {
        selectedIndex = last - 1;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicycleselectorwidget-addtextentry
 * @recoil-artifact defines .text recoil:function:0x4b7fd0: HudUiCycleSelectorWidget::AddTextEntry.
 *
 * Purpose: create a hidden transition text-panel entry, position it with the
 * selector text offset, and attach it to the owning HUD background container.
 *
 * Evidence: BN assembly at 0x4b7fd0 grows itemCount/visibleCount, allocates a
 * 0x2c0 HudUiTransitionTextPanel, stores it in entriesA[index], dispatches
 * SetTextFmt/SetPos/SetVisible, and adds it through the owner container.
 */
void HudUiCycleSelectorWidget::AddTextEntry(
    int index,
    const char *text,
    int posX,
    int posY
) {
    if (index >= itemCount) {
        int newCount = index + 1;
        if (newCount >= 20) {
            newCount = 20;
        }

        itemCount = newCount;
        if (newCount > visibleCount) {
            visibleCount = newCount;
        }
    }

    if (index > visibleCount) {
        return;
    }

    HudUiTransitionTextPanel *const transitionPanel = new HudUiTransitionTextPanel;

    entriesA[index] = (HudUiWidget *)(transitionPanel);
    ((HudUiPanel *)(entriesA[index]))->SetTextFmt(text);

    entriesA[index]->SetPos(
        textOffsetX + posX,
        textOffsetY + posY
    );
    entriesA[index]->SetVisible(0);
    ((HudUiContainer *)(owner))->AddChild(entriesA[index]);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicycleselectorwidget-applyfontstyleforentry
 * @recoil-artifact defines .text recoil:function:0x4b8100: HudUiCycleSelectorWidget::ApplyFontStyleForEntry.
 *
 * Purpose: grow the selector entry range when needed, validate the owning
 * background font style, and copy that style onto the text-panel entry.
 *
 * Evidence: BN assembly at 0x4b8100 selects owner->fontStyles[styleIndex] at
 * HudUiBackground offset 0x1cec, masks the style pointer to null when
 * validMarker is clear, calls the entry SetFont slot, and copies text color,
 * shadow, alignment, background mode, and background color fields.
 */
void HudUiCycleSelectorWidget::ApplyFontStyleForEntry(
    int index,
    int styleIndex
) {
    if (index >= itemCount) {
        int newCount = index + 1;
        if (newCount >= 20) {
            newCount = 20;
        }

        itemCount = newCount;
        if (newCount > visibleCount) {
            visibleCount = newCount;
        }
    }

    if (index > visibleCount) {
        return;
    }

    const HudFontStyle *const style =
        owner->fontStyles[styleIndex].validMarker != 0 ?
        &owner->fontStyles[styleIndex] :
        0;
    if (style == 0) {
        return;
    }

    HudUiPanel *panel = (HudUiPanel *)(entriesA[index]);
    panel->SetFont(
        style->fontName,
        style->fontSize,
        style->fontWeight,
        0,
        0,
        0,
        2
    );

    panel = (HudUiPanel *)(entriesA[index]);
    const unsigned int textColor = style->textColor;
    panel->textColor0 = textColor;
    panel->textColor1 = textColor;
    panel->textDirty = 1;

    panel = (HudUiPanel *)(entriesA[index]);
    panel->shadowEnabled = style->shadowEnabled;
    panel->shadowOffsetX = 1;
    panel->shadowOffsetY = 1;

    panel = (HudUiPanel *)(entriesA[index]);
    panel->alignMode = style->alignMode;

    panel = (HudUiPanel *)(entriesA[index]);
    const unsigned int backgroundColor = style->bkColor;
    const int backgroundMode = style->bkMode;
    panel->bkMode = backgroundMode;
    panel->bkColor = backgroundColor;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicycleselectorwidget-addbitmapentry
 * @recoil-artifact defines .text recoil:function:0x4b8200: HudUiCycleSelectorWidget::AddBitmapEntry.
 * Purpose: preserve the recovered HUD behavior for HudUiCycleSelectorWidget::AddBitmapEntry.
 */
void HudUiCycleSelectorWidget::AddBitmapEntry(
    int index,
    const char *imagePath,
    int posX,
    int posY
) {
    if (index > itemCount) {
        int newCount = index + 1;
        if (newCount >= 20) {
            newCount = 20;
        }

        itemCount = newCount;
        if (newCount > visibleCount) {
            visibleCount = newCount;
        }
    }

    if (index > visibleCount) {
        return;
    }

    HudUiWidget *const bitmapWidget = (HudUiWidget *)(::operator new(sizeof(HudUiWidget)));
    bitmapWidget->Constructor(0);
    entriesB[index] = bitmapWidget;
    bitmapWidget->SetImageByPathOwned(imagePath);
    bitmapWidget->SetPos(
        posX,
        posY
    );
    bitmapWidget->SetVisible(0);
    ((HudUiContainer *)(owner))->AddChild((HudUiElement *)(bitmapWidget));
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicycleselectorwidget-loadfromzrd
 * @recoil-artifact defines .text recoil:function:0x4b82e0: HudUiCycleSelectorWidget::LoadFromZrd.
 * Purpose: load the recovered HUD data handled by HudUiCycleSelectorWidget::LoadFromZrd.
 */
int HudUiCycleSelectorWidget::LoadFromZrd(
    zReader::Node *zrdSection,
    HudUiBackground *ownerDialog
) {
    HudUiZrdWidget::LoadFromZrd(
        zrdSection,
        ownerDialog
    );

    zReader::Node *const fontNode = zReader_GetNamedNode(
        zrdSection,
        g_HudUiCycleSelectorWidget_ZrdKey_Font
    );
    if (fontNode != 0) {
        fontStyleRef = (void *)((unsigned int)(fontNode->value.u32));
    }

    zReader::Node *const textOffsetNode = zReader_GetNamedNode(
        zrdSection,
        g_HudUiCycleSelectorWidget_ZrdKey_TextOffset
    );
    zReader::Node *const textOffsetBase = ZrdArrayBase(textOffsetNode);
    if (textOffsetBase != 0) {
        textOffsetX = ZrdArrayInt(
            textOffsetBase,
            1,
            textOffsetX
        );
        textOffsetY = ZrdArrayInt(
            textOffsetBase,
            2,
            textOffsetY
        );
    }

    zReader::Node *const cycleNode = zReader_GetNamedNode(
        zrdSection,
        g_HudUiCycleSelectorWidget_ZrdKey_Cycle
    );
    zReader::Node *const cycleBase = ZrdArrayBase(cycleNode);
    if (cycleBase == 0) {
        return 1;
    }

    int count = ZrdArrayCount(cycleBase) - 1;
    if (count >= 20) {
        count = 20;
    }

    itemCount = count;
    if (count > visibleCount) {
        visibleCount = count;
    }

    {
        for (int index = 0; index < itemCount; ++index) {
            zReader::Node *const entryNode = ZrdArrayItem(
                cycleBase,
                index + 1
            );

            zReader::Node *const textNode = zReader_GetNamedNode(
                entryNode,
                g_HudUiCycleSelectorWidget_ZrdKey_Text
            );
            zReader::Node *const textBase = ZrdArrayBase(textNode);
            if (textBase != 0) {
                const char *const key = ZrdArrayString(
                    textBase,
                    1
                );
                const char *const text = key != 0 ? zLoc::ResolveMessageKeyOrFallback(key) : "";
                AddTextEntry(
                    index,
                    text != 0 ? text : "",
                    originX + ZrdArrayInt(
                        textBase,
                        2,
                        0
                    ),
                    originY + ZrdArrayInt(textBase, 3, 0)
                );
                ApplyFontStyleForEntry(
                    index,
                    ZrdArrayInt(textBase, 4, 0)
                );
            }

            zReader::Node *const bitmapNode = zReader_GetNamedNode(
                entryNode,
                g_HudUiCycleSelectorWidget_ZrdKey_Bitmap
            );
            zReader::Node *const bitmapBase = ZrdArrayBase(bitmapNode);
            if (bitmapBase != 0) {
                int bitmapX = originX;
                int bitmapY = originY;
                if (ZrdArrayCount(bitmapBase) >= 4) {
                    bitmapX += ZrdArrayInt(
                        bitmapBase,
                        2,
                        0
                    );
                    bitmapY += ZrdArrayInt(
                        bitmapBase,
                        3,
                        0
                    );
                }

                AddBitmapEntry(
                    index,
                    ZrdArrayString(
                        bitmapBase,
                        1
                    ),
                    bitmapX,
                    bitmapY
                );
            }
        }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduifillbitmap-huduifillbitmap-0x4b8450
 * @recoil-artifact defines .text recoil:function:0x4b8450: HudUiFillBitmap::HudUiFillBitmap.
 * Purpose: preserve the recovered HUD behavior for HudUiFillBitmap::HudUiFillBitmap.
 */
HudUiFillBitmap::HudUiFillBitmap() : HudUiZrdWidget() {
    normalizedValue = 0.0f;
    previewImage = 0;
    fillImage = 0;
    previewRect.right = 0;
    previewRect.left = 0;
    previewRect.bottom = 0;
    previewRect.top = 0;
    fillRect.right = 0;
    fillRect.left = 0;
    fillRect.bottom = 0;
    fillRect.top = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduifillbitmap-huduifillbitmap-0x4b84d0
 * @recoil-artifact defines .text recoil:function:0x4b84d0: HudUiFillBitmap::~HudUiFillBitmap.
 * Purpose: Release distinct preview/fill images and tear down the ZRD widget base.
 */
HudUiFillBitmap::~HudUiFillBitmap() {
    if (previewImage != 0 && previewImage != image) {
        previewImage =
            (zVidImagePartial *)(unsigned int)zVid_Image::ReleaseIfNotDefault(
                previewImage
            );
    }

    if (fillImage != 0 && fillImage != image) {
        fillImage =
            (zVidImagePartial *)(unsigned int)zVid_Image::ReleaseIfNotDefault(
                fillImage
            );
    }
}

/**
 * Provider boundary 0x40cf50: VC5 compiler/EH cleanup forwarding thunk.
 * Source compatibility wrapper for recovered callers that historically named
 * the destructor body DestructorCore. The physical row is not a standalone
 * authored body.
 * Purpose: Run the fill-bitmap destructor body.
 */
void HudUiFillBitmap::DestructorCore() {
    this->HudUiFillBitmap::~HudUiFillBitmap();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduifillbitmap-draw
 * @recoil-artifact defines .text recoil:function:0x4b8520: HudUiFillBitmap::Draw.
 * Purpose: preserve the recovered HUD behavior for HudUiFillBitmap::Draw.
 */
void HudUiFillBitmap::Draw() {
    if (previewImage == 0 || fillImage == 0) {
        return;
    }

    DrawBase();

    if (fillRect.left != fillRect.right) {
        zVid_Image::BlitToActiveTarget(
            fillImage,
            x + fillOffsetX,
            y + fillOffsetY,
            0,
            (zVidRect32 *)(&fillRect)
        );
    }

    if (previewRect.left != previewRect.right) {
        zVid_Image::BlitToActiveTarget(
            previewImage,
            x + previewOffsetX,
            y + previewOffsetY,
            0,
            (zVidRect32 *)(&previewRect)
        );
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduifillbitmap-loadfromzrd
 * @recoil-artifact defines .text recoil:function:0x4b85c0: HudUiFillBitmap::LoadFromZrd.
 * Purpose: load the recovered HUD data handled by HudUiFillBitmap::LoadFromZrd.
 */
int HudUiFillBitmap::LoadFromZrd(
    zReader::Node *zrdSection,
    HudUiBackground *ownerDialog
) {
    HudUiZrdWidget::LoadFromZrd(
        zrdSection,
        ownerDialog
    );

    zReader::Node *const fillBitmapNode = zReader_GetNamedNode(
        zrdSection,
        g_HudUiFillBitmap_ZrdKey_FillBitmap
    );
    zReader::Node *const fillBitmapBase = ZrdArrayBase(fillBitmapNode);
    if (fillBitmapBase != 0) {
        fillImage = zImage::TexDir_FindOrCreateByPath(ZrdArrayString(
            fillBitmapBase,
            1
        ));
        Invalidate();

        int posX = originX;
        int posY = originY;
        if (ZrdArrayCount(fillBitmapBase) >= 4) {
            posX += ZrdArrayInt(
                fillBitmapBase,
                2,
                0
            );
            posY += ZrdArrayInt(
                fillBitmapBase,
                3,
                0
            );
        }

        SetPos(
            posX,
            posY
        );
        previewImage = image;
        Invalidate();
    }

    SetNormalizedValueAndRebuild(0.0f);
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduifillbitmap-updatenormalizedfromcursor
 * @recoil-artifact defines .text recoil:function:0x4b8650: HudUiFillBitmap::UpdateNormalizedFromCursor.
 * Purpose: update the normalized fill value from the owner cursor and activate the widget.
 */
void HudUiFillBitmap::UpdateNormalizedFromCursor() {
    const int cursorX = owner->mouseState.cursorClientX;
    const int relativeX = cursorX - GetCenterX();
    const int imageWidth = image != 0 ? image->width : 0;
    SetNormalizedValueAndRebuild((float)(relativeX) / (float)(imageWidth));
    // Retail emits a direct base call here. Virtual redispatch would re-enter
    // the sound/music option override and recurse until stack exhaustion.
    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduifillbitmap-setnormalizedvalueandrebuild
 * @recoil-artifact defines .text recoil:function:0x4b86b0: HudUiFillBitmap::SetNormalizedValueAndRebuild.
 * Purpose: apply the recovered HUD state change handled by HudUiFillBitmap::SetNormalizedValueAndRebuild.
 */
void HudUiFillBitmap::SetNormalizedValueAndRebuild(
    float value
) {
    if (fillImage == 0) {
        return;
    }

    normalizedValue = value;
    Invalidate();

    const int fillWidth = fillImage->width;
    const int fillHeight = fillImage->height;
    const int filledWidth = (int)((float)(fillWidth)*value);

    fillRect.left = 0;
    fillRect.top = 0;
    fillRect.right = filledWidth;
    fillRect.bottom = fillHeight;
    fillOffsetX = 0;
    fillOffsetY = 0;

    if (previewImage == 0) {
        return;
    }

    previewRect.left = filledWidth;
    previewRect.top = 0;
    previewRect.right = previewImage->width;
    previewRect.bottom = fillHeight;
    previewOffsetX = filledWidth;
    previewOffsetY = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduizrdwidgetex17c-item-huduizrdwidgetex17c-item
 * @recoil-artifact defines .text recoil:function:0x4b8760: HudUiZrdWidgetEx17C_Item::HudUiZrdWidgetEx17C_Item.
 * Purpose: preserve the recovered HUD behavior for HudUiZrdWidgetEx17C_Item::HudUiZrdWidgetEx17C_Item.
 */
HudUiZrdWidgetEx17C_Item::HudUiZrdWidgetEx17C_Item() : HudUiZrdWidget() {
    selected = 0;
    selectedImage = 0;
    unselectedImage = 0;
    ownerSelector = 0;
    mouseRectValid = 0;
}

/**
 * Purpose: run the recovered HudUiZrdWidgetEx17C_Item::DestructorCore teardown path.
 */
HudUiZrdWidgetEx17C_Item * HudUiZrdWidgetEx17C_Item::Constructor() {
    new (this) HudUiZrdWidgetEx17C_Item;
    return this;
}

/**
 * Purpose: run the recovered HudUiZrdWidgetEx17C_Item::DestructorCore teardown path.
 */
void HudUiZrdWidgetEx17C_Item::DestructorCore() {
    HudUiZrdWidget::DestructorCore();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduizrdwidgetex17c-item-showpreviewifnotselected
 * @recoil-artifact defines .text recoil:function:0x4b87d0: HudUiZrdWidgetEx17C_Item::ShowPreviewIfNotSelected.
 * Purpose: preserve the recovered HUD behavior for HudUiZrdWidgetEx17C_Item::ShowPreviewIfNotSelected.
 */
void HudUiZrdWidgetEx17C_Item::ShowPreviewIfNotSelected() {
    if (selected == 0) {
        HudUiZrdWidget::ShowPreview();
    }
}

/**
 * Purpose: preserve the recovered HUD behavior for HudUiZrdWidgetEx17C_Item::HidePreviewIfNotSelected.
 */
void HudUiZrdWidgetEx17C_Item::ShowPreview() {
    ShowPreviewIfNotSelected();
}

/**
 * Purpose: preserve the recovered HUD behavior for HudUiZrdWidgetEx17C_Item::HidePreviewIfNotSelected.
 */
void HudUiZrdWidgetEx17C_Item::HidePreviewIfNotSelected() {
    if (selected == 0) {
        HudUiZrdWidget::HidePreview();
    }
}

/**
 * Purpose: handle the recovered HUD event path for HudUiZrdWidgetEx17C_Item::OnActivateSelectSelf.
 */
void HudUiZrdWidgetEx17C_Item::HidePreview() {
    HidePreviewIfNotSelected();
}

/**
 * Purpose: handle the recovered HUD event path for HudUiZrdWidgetEx17C_Item::OnActivateSelectSelf.
 */
void HudUiZrdWidgetEx17C_Item::OnActivateSelectSelf() {
    ownerSelector->SetSelectedIndex(itemIndex);
    ownerSelector->OnActivate();
    HudUiZrdWidget::OnActivate();

    {
        for (int index = 0; index < ownerSelector->optionCount; ++index) {
            HudUiZrdWidgetEx17C_Item *const option = ownerSelector->options[index];
            option->HidePreviewIfNotSelected();
        }
    }
}

/**
 * Purpose: load the recovered HUD data handled by HudUiZrdWidgetEx17C_Item::LoadFromZrd.
 */
void HudUiZrdWidgetEx17C_Item::OnActivate() {
    OnActivateSelectSelf();
}

/**
 * Purpose: load the recovered HUD data handled by HudUiZrdWidgetEx17C_Item::LoadFromZrd.
 */
int HudUiZrdWidgetEx17C_Item::LoadFromZrd(
    zReader::Node *zrdSection,
    HudUiBackground *ownerDialog
) {
    HudUiZrdWidget::LoadFromZrd(
        zrdSection,
        ownerDialog
    );

    unselectedImage = image;
    unselectedRolloverImage = rolloverImage;
    selectedImage = activateImage;
    selectedRolloverImage = activateImage;

    boundsRect.top = GetCenterY();
    boundsRect.left = GetCenterX();

    if (image != 0) {
        boundsRect.bottom = boundsRect.top + image->width;
        boundsRect.right = boundsRect.left + image->height;
    } else {
        boundsRect.bottom = boundsRect.top;
        boundsRect.right = boundsRect.left;
    }

    if (unselectedImage != 0) {
        boundsRect.top = y;
        boundsRect.left = x;
        boundsRect.bottom = y + unselectedImage->height;
        boundsRect.right = x + unselectedImage->width;
    } else if (labelPanels.begin() != labelPanels.end()) {
        HudUiPanelPtrVector::iterator panelIt = labelPanels.begin();
        HudUiPanel *const firstPanel = *panelIt;
        boundsRect.top = firstPanel->GetCenterY();
        boundsRect.left = firstPanel->GetCenterX();
        boundsRect.bottom = firstPanel->QueryTextHeight() + boundsRect.top;

        while (panelIt != labelPanels.end()) {
            HudUiPanel *const panel = *panelIt;
            boundsRect.bottom += panel->QueryTextHeight();

            if (panel->textDirty != 0) {
                panel->RebuildTextRect();
            }

            const int right = panel->textWidthPx + boundsRect.left;
            if (right > boundsRect.right) {
                boundsRect.right = right;
            }

            ++panelIt;
        }

        boundsRect.bottom -= firstPanel->QueryTextHeight();
    }

    mouseRect = boundsRect;
    mouseRectValid = 1;

    zReader::Node *const mouseRectNode = zReader_GetNamedNode(
        zrdSection,
        g_HudUiZrdWidgetEx17C_Item_ZrdKey_MouseRect
    );
    zReader::Node *const mouseRectBase = ZrdArrayBase(mouseRectNode);
    if (mouseRectBase != 0) {
        mouseRect.top += ZrdArrayInt(
            mouseRectBase,
            1,
            0
        );
        mouseRect.left += ZrdArrayInt(
            mouseRectBase,
            2,
            0
        );
        mouseRect.bottom = mouseRect.top + ZrdArrayInt(
            mouseRectBase,
            3,
            0
        );
        mouseRect.right = mouseRect.left + ZrdArrayInt(
            mouseRectBase,
            4,
            0
        );
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduizrdwidgetex17c-item-setselected
 * @recoil-artifact defines .text recoil:function:0x4b8a90: HudUiZrdWidgetEx17C_Item::SetSelected
 * Source file evidence: BN labels the source as HudUiZrdWidgetEx17C_Item.cpp.
 * Purpose: Record the option-item selected state and refresh the displayed image pair when enabled.
 */
void HudUiZrdWidgetEx17C_Item::SetSelected(
    int selectedValue
) {
    selected = selectedValue;
    if (modeOrEnabled == 0) {
        return;
    }

    if (selectedValue != 0) {
        defaultImage = selectedImage;
        rolloverImage = selectedRolloverImage;
    } else {
        defaultImage = unselectedImage;
        rolloverImage = unselectedRolloverImage;
    }

    SetImageBorrowedAndInvalidate(defaultImage);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduizrdwidgetex17c-item-getmouserectorbounds
 * @recoil-artifact defines .text recoil:function:0x4b8af0: HudUiZrdWidgetEx17C_Item::GetMouseRectOrBounds.
 * Purpose: return the recovered HUD value exposed by HudUiZrdWidgetEx17C_Item::GetMouseRectOrBounds.
 */
HudUiRect * HudUiZrdWidgetEx17C_Item::GetMouseRectOrBounds() {
    if (mouseRectValid != 0) {
        return &mouseRect;
    }
    return HudUiZrdWidget::GetBoundsRectOrNull();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduizrdwidgetex17c-huduizrdwidgetex17c
 * @recoil-artifact defines .text recoil:function:0x4b8b10: HudUiZrdWidgetEx17C::HudUiZrdWidgetEx17C.
 * Purpose: preserve the recovered HUD behavior for HudUiZrdWidgetEx17C::HudUiZrdWidgetEx17C.
 */
HudUiZrdWidgetEx17C::HudUiZrdWidgetEx17C() : HudUiZrdWidget() {
    optionCount = 0;

    {
        int optionIndex;
        for (optionIndex = 0; optionIndex < 10; ++optionIndex) {
            options[optionIndex] = 0;
        }
    }
}

/**
 * Source file evidence: BN labels the source as HudUiZrdWidgetEx17C.cpp.
 * Purpose: Delete owned option-selector items and clear their slots before compiler-generated base cleanup.
 */
HudUiZrdWidgetEx17C::~HudUiZrdWidgetEx17C() {

    {
        int optionIndex;
        for (optionIndex = 0; optionIndex < 10; ++optionIndex) {
            HudUiZrdWidgetEx17C_Item *option = options[optionIndex];
            if (option != 0) {
                delete option;
                options[optionIndex] = 0;
            }
        }
    }
}

/**
 * Purpose: run the recovered HudUiZrdWidgetEx17C destructor through the compatibility name.
 */
HudUiZrdWidgetEx17C * HudUiZrdWidgetEx17C::Constructor() {
    new (this) HudUiZrdWidgetEx17C;
    return this;
}

/**
 * HudUiZrdWidgetEx17C::DestructorCore compatibility wrapper.
 * No standalone retail function; source compatibility wrapper for recovered
 * callers that historically named the destructor body DestructorCore in this
 * reconstruction.
 * Purpose: Run the option-selector destructor body.
 */
void HudUiZrdWidgetEx17C::DestructorCore() {
    this->HudUiZrdWidgetEx17C::~HudUiZrdWidgetEx17C();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduizrdwidgetex17c-loadfromzrd
 * @recoil-artifact defines .text recoil:function:0x4b8be0: HudUiZrdWidgetEx17C::LoadFromZrd.
 * Purpose: load the recovered HUD data handled by HudUiZrdWidgetEx17C::LoadFromZrd.
 */
int HudUiZrdWidgetEx17C::LoadFromZrd(
    zReader::Node *zrdSection,
    HudUiBackground *ownerDialog
) {
    owner = ownerDialog;

    zReader::Node *const radioNode = zReader_GetNamedNode(
        zrdSection,
        g_HudUiZrdToken_Radio
    );
    zReader::Node *const radioBase = ZrdArrayBase(radioNode);
    if (radioBase != 0) {
        optionCount = ZrdArrayCount(radioBase) - 1;
        if (optionCount >= 10) {
            optionCount = 10;
        }

        {
            for (int index = 0; index < optionCount; ++index) {
                HudUiZrdWidgetEx17C_Item *const option =
                    new HudUiZrdWidgetEx17C_Item;
                options[index] = option;

                option->LoadFromZrd(
                    &radioBase[index + 1],
                    ownerDialog
                );
                option->ownerSelector = this;
                option->itemIndex = index;
            }
        }
    }

    SetSelectedIndex(0);
    return 1;
}

/**
 * Source model note: Source-faithful helper recovered from address-backed callers in this
 * source file.
 * Purpose: apply the recovered HUD state change handled by HudUiZrdWidgetEx17C::SetSelectedIndex.
 */
void HudUiZrdWidgetEx17C::SetVisible(
    int childIndex
) {
    EnableChildAtIndex(childIndex);
}

/**
 * Source file evidence: BN labels the source as HudUiZrdWidgetEx17C.cpp.
 * Purpose: Store the selected option index and update every loaded option item's selected state.
 */
int HudUiZrdWidgetEx17C::SetSelectedIndex(
    int index
) {
    selectedIndex = index;
    {
        for (int optionIndex = 0; optionIndex < 10; ++optionIndex) {
            HudUiZrdWidgetEx17C_Item *const option = options[optionIndex];
            if (option != 0) {
                option->SetSelected(optionIndex == index ? 1 : 0);
            }
        }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-hudcmdbindbuttonbase-hudcmdbindbuttonbase
 * @recoil-artifact defines .text recoil:function:0x4b8d30: HudCmdBindButtonBase::HudCmdBindButtonBase.
 * Purpose: preserve the recovered HUD behavior for HudCmdBindButtonBase::HudCmdBindButtonBase.
 */
HudCmdBindButtonBase::HudCmdBindButtonBase() :
    HudUiCheckToggleWidget()
{
    bindingSlotTotalCount = 0;
    bindingSlotPanels = 0;
    visibleListOffsetX = 0.0f;
    visibleListOffsetY = 0.0f;
    overflowListOffsetX = 0.0f;
    overflowListOffsetY = 0.0f;
    bindingSlotSpacing = 0xf;
    selectedBindingIndex = -1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-hudcmdbindbuttonbase-loadfromzrd
 * @recoil-artifact defines .text recoil:function:0x4b8de0: HudCmdBindButtonBase::LoadFromZrd.
 * Purpose: load binding button fonts, spacing, offsets, slot counts, and
 * child panel setup from a ZRD node.
 * Touched data: uses accepted ZRD-key literal owner
 * hud_ui.hudcmd_bind_button_base_zrd_key_literals.
 */
int HudCmdBindButtonBase::LoadFromZrd(
    zReader::Node *zrdSection,
    HudUiBackground *ownerDialog
) {
    HudUiCheckToggleWidget::LoadFromZrd(
        zrdSection,
        ownerDialog
    );

    void *const clipSource = ownerDialog->capturedCompositeImage;

    zReader::Node *const selectedFontNode = zReader_GetNamedNode(
        zrdSection,
        "SELECTED_FONT"
    );
    if (selectedFontNode != 0) {
        selectedFontStyleRef = selectedFontNode->value.i32;
        ApplyHudFontStyleTextOnly(
            (HudUiPanel *)(&bindPanel),
            HudUiZrdOwnerFontStyle(owner, selectedFontStyleRef)
        );
    }

    zReader::Node *const listFontNode = zReader_GetNamedNode(
        zrdSection,
        "LIST_FONT"
    );
    if (listFontNode != 0) {
        listFontStyleRef = listFontNode->value.i32;
    }

    zReader::Node *const spacingNode = zReader_GetNamedNode(
        zrdSection,
        "SPACING"
    );
    if (spacingNode != 0) {
        bindingSlotSpacing = spacingNode->value.i32;
    }

    zReader::Node *const listOffsetNode = zReader_GetNamedNode(
        zrdSection,
        "LIST_OFFSET"
    );
    zReader::Node *const listOffsetBase = ZrdArrayBase(listOffsetNode);
    zReader::Node *const visibleOffsetBase = ZrdArrayBase(ZrdArrayItem(
        listOffsetBase,
        1
    ));
    zReader::Node *const overflowOffsetBase = ZrdArrayBase(ZrdArrayItem(
        listOffsetBase,
        2
    ));
    if (visibleOffsetBase != 0 && overflowOffsetBase != 0) {
        visibleListOffsetX = (float)(ZrdArrayInt(
            visibleOffsetBase,
            1,
            0
        ));
        visibleListOffsetY = (float)(ZrdArrayInt(
            visibleOffsetBase,
            2,
            0
        ));
        overflowListOffsetX = (float)(ZrdArrayInt(
            overflowOffsetBase,
            1,
            0
        ));
        overflowListOffsetY = (float)(ZrdArrayInt(
            overflowOffsetBase,
            2,
            0
        ));
    }

    zReader::Node *const listSizeNode = zReader_GetNamedNode(
        zrdSection,
        "LISTSIZE"
    );
    zReader::Node *const listSizeBase = ZrdArrayBase(listSizeNode);
    if (listSizeBase != 0) {
        const int visibleCount =
            ZrdArrayCount(listSizeBase) > 2 ? ZrdArrayInt(
                listSizeBase,
                2,
                0
            ) : 0;
        RebuildBindingSlotWidgets(
            ZrdArrayInt(
                listSizeBase,
                1,
                0
            ),
            visibleCount
        );

        HudUiRect clipRect = {0};
        const HudFontStyle *const listStyle =
            HudUiZrdOwnerFontStyle(
                owner,
                listFontStyleRef
            );
        {
            for (int index = 0; index < bindingSlotTotalCount; ++index) {
                HudUiListSelectorItem *const item = &bindingSlotPanels[index];
                ((HudUiContainer *)(ownerDialog))->AddChild((HudUiElement *)(item));
                item->SetVisible(1);
                item->owner = this;
                if (clipSource != 0) {
                    HudUiSetPanelClipWithSource(
                        item,
                        clipSource,
                        &clipRect
                    );
                }

                ApplyHudFontStyleTextOnly(
                    (HudUiPanel *)(item),
                    listStyle
                );
            }
        }

        ((HudUiContainer *)(ownerDialog))->AddChild((HudUiElement *)(&bindPanel));
        bindPanel.SetVisible(1);
        bindPanel.owner = this;
        if (clipSource != 0) {
            HudUiSetPanelClipWithSource(
                &bindPanel,
                clipSource,
                &clipRect
            );
        }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-hudcmdbindbuttonbase-rebuildbindingslotwidgets
 * @recoil-artifact defines .text recoil:function:0x4b90e0: HudCmdBindButtonBase::RebuildBindingSlotWidgets.
 * Purpose: recreate the binding-slot panel array and lay out visible and
 * overflow slots around the selected binding panel.
 */
void HudCmdBindButtonBase::RebuildBindingSlotWidgets(
    int totalCount,
    int visibleCount
) {
    DeleteHudUiListSelectorItemArray(bindingSlotPanels);
    bindingSlotPanels = 0;

    const unsigned int allocationSize =
        sizeof(int) + (unsigned int)(totalCount) * sizeof(HudUiListSelectorItem);
    HudUiListSelectorItemArrayHeader *const header =
        (HudUiListSelectorItemArrayHeader *)(::operator new(allocationSize));
    header->count = totalCount;
    HudUiListSelectorItem *const items = (HudUiListSelectorItem *)(header + 1);
    {
        for (int index = 0; index < totalCount; ++index) {
            new (&items[index]) HudUiListSelectorItem;
        }
    }

    bindingSlotPanels = items;
    bindingSlotTotalCount = totalCount;
    visibleBindingSlotCount = visibleCount;

    {
        for (int index = 0; index < visibleBindingSlotCount; ++index) {
            const int x = (int)((float)(originX) + visibleListOffsetX);
            const int y = (int)((float)(originY +
                                        (index - visibleBindingSlotCount) * bindingSlotSpacing) +
                                visibleListOffsetY);
            bindingSlotPanels[index].SetPos(
                x,
                y
            );
        }
    }

    bindPanel.SetPos(
        originX,
        originY
    );

    {
        for (int index = visibleBindingSlotCount; index < bindingSlotTotalCount; ++index) {
            const int x = (int)((float)(originX) + overflowListOffsetX);
            const int y = (int)((float)(originY + (index - visibleBindingSlotCount + 1) *
                                                                bindingSlotSpacing) +
                                overflowListOffsetY);
            bindingSlotPanels[index].SetPos(
                x,
                y
            );
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-hudcmdbindbuttonbase-onselectedindexchanged
 * @recoil-artifact defines .text recoil:function:0x4b9320: HudCmdBindButtonBase::OnSelectedIndexChanged.
 * Purpose: handle the recovered HUD event path for HudCmdBindButtonBase::OnSelectedIndexChanged.
 */
void HudCmdBindButtonBase::OnSelectedIndexChanged(
    int selectedIndex
) {
    SetSelectedEntry(selectedIndex);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-hudcmdbindbuttonbase-setselectedentry
 * @recoil-artifact defines .text recoil:function:0x4b9330: HudCmdBindButtonBase::SetSelectedEntry.
 * Purpose: apply the recovered HUD state change handled by HudCmdBindButtonBase::SetSelectedEntry.
 */
void HudCmdBindButtonBase::SetSelectedEntry(
    int selectedIndex
) {
    int slotIndex;
    for (slotIndex = 0; slotIndex < visibleBindingSlotCount; ++slotIndex) {
        const int entryIndex = selectedIndex + slotIndex - visibleBindingSlotCount;
        if (entryIndex >= 0 && entryIndex < (int)bindingVec.size()) {
            HudCmdBindingEntry **const entries = bindingVec.begin();
            bindingSlotPanels[slotIndex].entryIndex = entryIndex;
            bindingSlotPanels[slotIndex].SetTextFmt(
                "%s",
                entries[entryIndex]->displayText
            );
            bindingSlotPanels[slotIndex].SetVisible(1);
        } else {
            bindingSlotPanels[slotIndex].SetVisible(0);
            bindingSlotPanels[slotIndex].DrawBase();
        }

        bindingSlotPanels[slotIndex].Invalidate();
    }

    if (selectedIndex >= 0 && selectedIndex < (int)bindingVec.size()) {
        HudCmdBindingEntry **const entries = bindingVec.begin();
        bindPanel.entryIndex = selectedIndex;
        bindPanel.SetTextFmt(
            "%s",
            entries[selectedIndex]->displayText
        );
    }

    for (slotIndex = visibleBindingSlotCount; slotIndex < bindingSlotTotalCount; ++slotIndex) {
        const int entryIndex = selectedIndex + slotIndex - visibleBindingSlotCount + 1;
        if (entryIndex >= 0 && entryIndex < (int)bindingVec.size()) {
            HudCmdBindingEntry **const entries = bindingVec.begin();
            bindingSlotPanels[slotIndex].entryIndex = entryIndex;
            bindingSlotPanels[slotIndex].SetTextFmt(
                "%s",
                entries[entryIndex]->displayText
            );
            bindingSlotPanels[slotIndex].SetVisible(1);
        } else {
            bindingSlotPanels[slotIndex].SetVisible(0);
            bindingSlotPanels[slotIndex].DrawBase();
        }

        bindingSlotPanels[slotIndex].Invalidate();
    }

    selectedBindingIndex = selectedIndex;
}

/**
 * @recoil-anchor recoil:anchor:src-gamezrecoil-zui-zui_widgets-function-huduilistselectoritem-onactivate
 * @recoil-artifact defines .text recoil:function:0x4b9520: HudUiListSelectorItem::OnActivate.
 * Source model note: the constructor relationship is unresolved; the source
 * model lives in the inline class-body constructor in zhud_ui.h, and no source
 * edge is claimed here.
 * Purpose: handle the recovered HUD event path for HudUiListSelectorItem::OnActivate.
 */
void HudUiListSelectorItem::OnActivate() {
    typedef void( * OnSelectedIndexChangedFn)(
        void *self,
        int selectedIndex
    );

    void *const selectionOwner = owner;
    if (selectionOwner != 0) {
        const unsigned int *const ownerSlots =
            *(const unsigned int *const *)selectionOwner;
        ((OnSelectedIndexChangedFn)(ownerSlots[33]))(
            selectionOwner,
            entryIndex
        );
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduibackground-huduibackground-0x4b9540
 * @recoil-artifact defines .text recoil:function:0x4b9540: HudUiBackground::HudUiBackground.
 * Purpose: preserve the recovered HUD behavior for HudUiBackground::HudUiBackground.
 */
HudUiBackground::HudUiBackground()
    : HudUiBackgroundContainer(1),
      cursorWidget(0, 1) {
    primaryClipImage = 0;
    capturedCompositeImage = 0;

    {
        for (int index = 0; index < 10; ++index) {
            backgroundSounds[index].sample = 0;
            backgroundSounds[index].volume = 1.0f;
            backgroundSounds[index].playHandle = 0;
        }
    }

    int defaultVMode = 5;
    zOptionEntryPartial *vmodeOption = zGame::Options_FindOption("VMode");
    if (vmodeOption == 0) {
        vmodeOption = (zOptionEntryPartial *)(&defaultVMode);
    }

    switch (vmodeOption->payloadOrBuffer) {
    case 2:
    case 4:
        uiOriginX = 0;
        uiOriginY = -40;
        break;
    case 3:
    case 5:
        uiOriginX = 0;
        uiOriginY = 0;
        break;
    case 6:
        uiOriginX = 0;
        uiOriginY = 60;
        break;
    case 7:
        uiOriginX = 0;
        uiOriginY = 144;
        break;
    }

}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduibackground-huduibackground-0x4b9760
 * @recoil-artifact defines .text recoil:function:0x4b9760: HudUiBackground::~HudUiBackground.
 * Purpose: Releases owned background clip images before compiler-generated member and base cleanup.
 */
HudUiBackground::~HudUiBackground() {
    if (primaryClipImage != 0) {
        primaryClipImage =
            (zVidImagePartial *)(unsigned int)zVid_Image::ReleaseIfNotDefault(
                primaryClipImage
            );
    }

    if (capturedCompositeImage != 0) {
        capturedCompositeImage =
            (zVidImagePartial *)(unsigned int)zVid_Image::ReleaseIfNotDefault(
                capturedCompositeImage
            );
    }

}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduibackground-setenabled
 * @recoil-artifact defines .text recoil:function:0x4b9850: HudUiBackground::SetEnabled.
 * Purpose: start or stop configured background sounds and update background visibility state.
 */
void HudUiBackground::SetEnabled(
    int enabled
) {
    if (enabled != 0) {
        int entryIndex14;
        for (entryIndex14 = 0;
            entryIndex14 < (int)(sizeof(backgroundSounds) / sizeof(backgroundSounds[0]));
            ++entryIndex14) {
            HudUiBackgroundSoundEntry &entry = backgroundSounds[entryIndex14];
            if (entry.sample != 0) {
                entry.playHandle = entry.sample->PlayA3DSimple(entry.volume);
            }
        }

        InvalidateChildren();
    } else {
        int entryIndex15;
        for (entryIndex15 = 0;
            entryIndex15 < (int)(sizeof(backgroundSounds) / sizeof(backgroundSounds[0]));
            ++entryIndex15) {
            HudUiBackgroundSoundEntry &entry = backgroundSounds[entryIndex15];
            if (entry.playHandle != 0) {
                entry.playHandle->StopIfActive();
            }

            entry.playHandle = 0;
        }
    }

    HudUiBackgroundContainer::SetEnabled(enabled);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduibackground-loadfromzrd
 * @recoil-artifact defines .text recoil:function:0x4b98d0: HudUiBackground::LoadFromZrd.
 * Purpose: load the recovered HUD data handled by HudUiBackground::LoadFromZrd.
 */
zReader::Node * HudUiBackground::LoadFromZrd(
    const char *zrdPath,
    const char *sectionName,
    int capturePrimary
) {
    zReader::Node *const root = zReader::LoadNodeFromPath(
        zrdPath,
        0,
        0
    );
    loadedRoot = root;
    return LoadZrdAndSection(
        root,
        sectionName,
        capturePrimary
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduibackground-loadzrdandsection
 * @recoil-artifact defines .text recoil:function:0x4b9900: HudUiBackground::LoadZrdAndSection.
 * Purpose: load the recovered HUD data handled by HudUiBackground::LoadZrdAndSection.
 */
zReader::Node * HudUiBackground::LoadZrdAndSection(
    zReader::Node *loadedRootNode,
    const char *sectionName,
    int capturePrimary
) {
    zReader::Node *result = 0;
    zVideo::RunPostprocessOnPrimaryBuffer();

    if (capturePrimary == 0) {
        primaryClipImage = zVideo_buff_CaptureSurfaceToImage(1);
    }

    if (loadedRootNode != 0) {
        result = loadedRootNode;

        zReader::Node *const sharedImagePath =
            zReader_GetNamedNode(
                loadedRootNode,
                zHudCfgKey_SHARED_IMAGE_PATH
            );
        if (sharedImagePath != 0) {
            zImage_InitMissionResources(sharedImagePath->value.nodes[1].value.str);
        }

        zReader::Node *const sectionRoot = zReader_GetNamedNode(
            loadedRootNode,
            sectionName
        );
        cfgRoot = sectionRoot;

        if (sectionRoot != 0) {
        zReader::Node *const imagePath = zReader_GetNamedNode(
            sectionRoot,
            "IMAGE_PATH"
        );
        if (imagePath != 0) {
            zImage_InitMissionResources(imagePath->value.nodes[1].value.str);
        }

        zReader::Node *const fontListNode = zReader_GetNamedNode(
            cfgRoot,
            g_HudCfgKey_Fonts
        );
        if (fontListNode != 0) {
            int fontCount = fontListNode->value.nodes[0].value.i32;
            if (fontCount > 20) {
                fontCount = 20;
            }

            for (int index = 1; index < fontCount; ++index) {
                zReader::Node *const fontSpec =
                    fontListNode->value.nodes[index].value.nodes;

                const int styleIndex = fontSpec[1].value.i32;

                fontStyles[styleIndex].validMarker = 1;
                fontStyles[styleIndex].bkColor = 0;
                fontStyles[styleIndex].bkMode = 1;
                fontStyles[styleIndex].fontName = fontSpec[2].value.str;
                fontStyles[styleIndex].fontSize = fontSpec[3].value.i32;

                if (fontSpec[4].value.nodes[1].type == zReader::ZRDR_NODE_ARRAY) {
                    fontStyles[styleIndex].textColor =
                        (unsigned char)(fontSpec[4].value.nodes[1].value.nodes[1].value.i32) |
                        ((unsigned int)(unsigned char)(fontSpec[4].value.nodes[1].value.nodes[2].value.i32) << 8) |
                        ((unsigned int)(unsigned char)(fontSpec[4].value.nodes[1].value.nodes[3].value.i32) << 16);

                    fontStyles[styleIndex].bkColor =
                        (unsigned char)(fontSpec[4].value.nodes[2].value.nodes[1].value.i32) |
                        ((unsigned int)(unsigned char)(fontSpec[4].value.nodes[2].value.nodes[2].value.i32) << 8) |
                        ((unsigned int)(unsigned char)(fontSpec[4].value.nodes[2].value.nodes[3].value.i32) << 16);
                    fontStyles[styleIndex].bkMode = 2;
                } else {
                    fontStyles[styleIndex].textColor =
                        (unsigned char)(fontSpec[4].value.nodes[1].value.i32) |
                        ((unsigned int)(unsigned char)(fontSpec[4].value.nodes[2].value.i32) << 8) |
                        ((unsigned int)(unsigned char)(fontSpec[4].value.nodes[3].value.i32) << 16);
                }

                if (fontSpec[0].value.i32 >= 6) {
                    fontStyles[styleIndex].shadowEnabled = fontSpec[5].value.i32;
                }
                if (fontSpec[0].value.i32 >= 7) {
                    fontStyles[styleIndex].fontWeight = fontSpec[6].value.i32;
                }
                if (fontSpec[0].value.i32 >= 8) {
                    const char *const align = fontSpec[7].value.str;
                    if (align == 0 || strcmp(
                        align,
                        "LEFT"
                    ) == 0) {
                        fontStyles[styleIndex].alignMode = 0;
                    } else if (strcmp(
                        align,
                        "RIGHT"
                    ) == 0) {
                        fontStyles[styleIndex].alignMode = 2;
                    } else if (strcmp(
                        align,
                        "CENTER"
                    ) == 0) {
                        fontStyles[styleIndex].alignMode = 1;
                    }
                }
            }
        }

        zReader::Node *const imageListNode = zReader_GetNamedNode(
            cfgRoot,
            zHudCfgKey_BACKGROUND_IMAGES
        );
        if (imageListNode != 0) {
            int imageCount = imageListNode->value.nodes[0].value.i32;
            if (imageCount > 20) {
                imageCount = 20;
            }

            for (int index = 1; index < imageCount; ++index) {
                zReader::Node *const imageSpec =
                    imageListNode->value.nodes[index].value.nodes;

                HudUiWidget &child = backgroundImageWidgets[index - 1];
                child.SetImageByPathOwned(imageSpec[1].value.str);
                if (imageSpec[0].value.i32 >= 4) {
                    const int originX = uiOriginX;
                    const int originY = uiOriginY;
                    ((HudUiElement *)(&child))
                        ->SetPos(
                            imageSpec[2].value.i32 + originX,
                            imageSpec[3].value.i32 + originY
                        );
                }

                child.flags =
                    (unsigned int)((unsigned char)(child.flags) & 0x10u) |
                    0x02u;
                ((HudUiElement *)(&child))->SetVisible(1);
                ((HudUiElement *)(&child))->Invalidate();
                AddChild((HudUiElement *)(&child));
            }
        }

        zReader::Node *const videoListNode = zReader_GetNamedNode(
            cfgRoot,
            zHudCfgKey_BACKGROUND_VIDEOS
        );
        if (videoListNode != 0) {
            int videoCount = videoListNode->value.nodes[0].value.i32;
            if (videoCount > 10) {
                videoCount = 10;
            }

            for (int index = 1; index < videoCount; ++index) {
                zReader::Node *const videoSpec =
                    videoListNode->value.nodes[index].value.nodes;

                HudUiBackgroundVideoWidget &child = backgroundVideoWidgets[index - 1];
                child.SetMediaPathOwnedAndRefresh(videoSpec[1].value.str);
                if (videoSpec[0].value.i32 >= 4) {
                    const int originX = uiOriginX;
                    const int originY = uiOriginY;
                    child.SetPos(
                        videoSpec[2].value.i32 + originX,
                        videoSpec[3].value.i32 + originY
                    );
                }
                if (videoSpec[0].value.i32 >= 5) {
                    zReader::Node *const color = videoSpec[4].value.nodes;
                    child.SetColorKey565((unsigned short)(zVid_PackColorRGB(
                        color[1].value.i32,
                        color[2].value.i32,
                        color[3].value.i32
                    )));
                }

                child.SetVisible(1);
                child.Invalidate();
                child.SetBltSourceAndClipRect(
                    primaryClipImage,
                    0
                );
                child.RebuildBltRect();
                AddChild(&child);
            }
        }

        zReader::Node *const textListNode = zReader_GetNamedNode(
            cfgRoot,
            zHudCfgKey_BACKGROUND_TEXT
        );
        if (textListNode != 0) {
            int textCount = textListNode->value.nodes[0].value.i32;
            if (textCount > 50) {
                textCount = 50;
            }

            for (int index = 1; index < textCount; ++index) {
                zReader::Node *const textSpec =
                    textListNode->value.nodes[index].value.nodes;

                HudUiPanel *const child = (HudUiPanel *)(&backgroundTextPanels[index - 1]);
                child->SetTextFmt(
                    zLoc::ResolveMessageKeyOrFallback(textSpec[1].value.str)
                );
                const int originX = uiOriginX;
                const int originY = uiOriginY;
                child->SetPos(
                    textSpec[2].value.i32 + originX,
                    textSpec[3].value.i32 + originY
                );
                const HudFontStyle *style = &fontStyles[textSpec[4].value.i32];
                style = style->validMarker != 0 ? style : 0;
                if (style != 0) {
                    child->alignMode = style->alignMode;
                    child->SetFont(
                        style->fontName,
                        style->fontSize,
                        style->fontWeight,
                        0,
                        0,
                        0,
                        2
                    );
                    const unsigned int textColor = style->textColor;
                    child->textColor0 = textColor;
                    child->textColor1 = textColor;
                    child->textDirty = 1;
                    child->shadowEnabled = style->shadowEnabled;
                    child->shadowOffsetX = 1;
                    child->shadowOffsetY = 1;
                    child->bkColor = style->bkColor;
                    child->bkMode = style->bkMode;
                }
                child->SetVisible(1);
                AddChild((HudUiElement *)(child));
            }
        }

        if (capturePrimary == 0) {
            HudUiBackgroundContainer::SetEnabled(1);
            UpdateAll(0.0f);
            capturedCompositeImage = zVideo_buff_CaptureSurfaceToImage(1);
            HudUiBackgroundContainer::SetEnabled(0);
            ((HudUiDialogController *)(this))->BlitOwnedSurfaceToPrimary();
        }

        zReader::Node *const cursorNode = zReader_GetNamedNode(
            cfgRoot,
            zHudCfgKey_CURSOR
        );
        if (cursorNode != 0) {
            zReader::Node *const bitmapNode =
                zReader_GetNamedNode(
                    cursorNode,
                    g_HudUiCycleSelectorWidget_ZrdKey_Bitmap
                );
            if (bitmapNode != 0) {
                cursorWidget.SetImageByPathOwnedAndRefresh(
                    bitmapNode->value.nodes[1].value.str
                );
            }

            SetInputFocus((HudUiElement *)(&cursorWidget));

            zReader::Node *const centerNode =
                zReader_GetNamedNode(
                    cursorNode,
                    "CENTER"
                );
            if (centerNode != 0) {
                // Original 0x4b9f69 stores CENTER's string pointer into HudUiWidget::alignFlags;
                // GetCenterX/Y only test the slot for nonzero on this cursor path.
                cursorWidget.alignFlags =
                    (unsigned int)(centerNode->value.nodes[1].value.str);
            }

            int cursorCapture = 1;
            zReader::ReadNamedInt(
                cursorNode,
                zHudCfgKey_CAPTURE,
                &cursorCapture
            );
            cursorWidget.SetImageOwnedAndRefresh(cursorCapture);
        }

        zReader::Node *const soundListNode = zReader_GetNamedNode(
            cfgRoot,
            zHudCfgKey_BACKGROUND_SOUNDS
        );
        if (soundListNode != 0) {
            int soundCount = soundListNode->value.nodes[0].value.i32;
            if (soundCount > 10) {
                soundCount = 10;
            }

            for (int index = 1; index < soundCount; ++index) {
                zReader::Node *const soundSpec =
                    soundListNode->value.nodes[index].value.nodes;

                HudUiBackgroundSoundEntry &entry = backgroundSounds[index - 1];
                float volume = 1.0f;
                if (soundSpec[0].value.i32 >= 3) {
                    volume = soundSpec[2].value.f32;
                }
                entry.sample = zSnd::FindSampleByName(soundSpec[1].value.str);
                entry.volume = volume;
            }
        }
        }
    }

    zVideo::Dispatch_UnlockPrimarySurfaceState();
    return result;
}

#include "GameZRecoil/zHud/zhud_ui_defs.h"

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduibackground-bindbuttonsnodetowidgetbyname
 * @recoil-artifact defines .text recoil:function:0x4ba070: HudUiBackground::BindButtonsNodeToWidgetByName.
 * Purpose: preserve the recovered HUD behavior for HudUiBackground::BindButtonsNodeToWidgetByName.
 */
unsigned char __fastcall HudUiBackground::BindButtonsNodeToWidgetByName(
    zReader::Node *parentNode,
    HudUiWidget *widget,
    const char *name
) {
    if (parentNode != 0) {
        zReader::Node *const buttonsNode = zReader_GetNamedNode(
            parentNode,
            g_HudUiZrdToken_Buttons
        );
        zReader::Node *const widgetNode = zReader_GetNamedNode(
            buttonsNode,
            name
        );
        if (widgetNode != 0) {
            ((HudUiZrdWidget *)widget)->LoadFromZrd(
                widgetNode,
                this
            );
            ((HudUiZrdWidget *)widget)->PostLoadFromZrd();
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduibackground-bindwidgetbyname
 * @recoil-artifact defines .text recoil:function:0x4ba0c0: HudUiBackground::BindWidgetByName.
 * Purpose: preserve the recovered HUD behavior for HudUiBackground::BindWidgetByName.
 */
int HudUiBackground::BindWidgetByName(
    zReader::Node *,
    HudUiWidget *widget,
    const char *name
) {
    return BindButtonsNodeToWidgetByName(
        cfgRoot,
        widget,
        name
    ) & 0xff;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduibackground-bindprimitivenodetoelement
 * @recoil-artifact defines .text recoil:function:0x4ba0e0: HudUiBackground::BindPrimitiveNodeToElement.
 * Purpose: bind a named ZRD primitive node to an existing HUD element.
 * Binary Ninja: 0x4ba0e0 performs direct zReader::Node child/value reads for
 * optional BITMAP, POSITION, WORDWRAP, FONT, COLOR, ENDP_REL, and ENDP_ABS
 * records before assigning the final blit source, clip rect, and dirty state.
 */
int HudUiBackground::BindPrimitiveNodeToElement(
    zReader::Node *,
    HudUiElement *element,
    const char *name
) {
    zReader::Node *const cfgRoot = this->cfgRoot;
    if (cfgRoot == 0) {
        return 0;
    }

    zReader::Node *primitiveNode;
    primitiveNode = zReader_GetNamedNode(
        cfgRoot,
        g_HudUiBackground_ZrdKey_Primitives
    );
    if (primitiveNode != 0) {
        primitiveNode = zReader_GetNamedNode(
            primitiveNode,
            name
        );
        if (primitiveNode != 0) {
            ((HudUiContainer *)(this))->AddChild(element);

            zReader::Node *bitmapNode = zReader_GetNamedNode(
                primitiveNode,
                g_HudUiCycleSelectorWidget_ZrdKey_Bitmap
            );
            if (bitmapNode != 0) {
                ((HudUiWidget *)(element))->SetImageByPathOwned(
                    bitmapNode->value.nodes[1].value.str
                );
            }

            zReader::Node *positionNode = zReader_GetNamedNode(
                primitiveNode,
                g_HudZrd_Key_Position
            );
            if (positionNode != 0) {
                zReader::Node *const positionBase = positionNode->value.nodes;
                element->SetPos(
                    uiOriginX + positionBase[1].value.i32,
                    uiOriginY + positionBase[2].value.i32
                );
            }

            zReader::Node *wordWrapNode = zReader_GetNamedNode(
                primitiveNode,
                g_HudUiZrdToken_WordWrap
            );
            if (wordWrapNode != 0) {
                zReader::Node *const wordWrapBase = wordWrapNode->value.nodes;
                HudUiRect wordWrapRect;
                wordWrapRect.left = 0;
                wordWrapRect.top = 0;
                wordWrapRect.right = wordWrapBase[1].value.i32;
                wordWrapRect.bottom = wordWrapBase[2].value.i32;
                element->EnableWordWrapWithRect(&wordWrapRect);
            }

            zReader::Node *fontNode = zReader_GetNamedNode(
                primitiveNode,
                g_HudUiCycleSelectorWidget_ZrdKey_Font
            );
            if (fontNode != 0) {
                const int fontIndex = fontNode->value.i32;
                const HudFontStyle *style =
                    fontStyles[fontIndex].validMarker != 0 ?
                    &fontStyles[fontIndex] :
                    0;
                if (style != 0) {
                    HudUiPanel *const panel = (HudUiPanel *)(element);
                    panel->alignMode = style->alignMode;
                    panel->SetFont(
                        style->fontName,
                        style->fontSize,
                        style->fontWeight,
                        0,
                        0,
                        0,
                        2
                    );
                    const unsigned int textColor = style->textColor;
                    panel->textColor0 = textColor;
                    panel->textColor1 = textColor;
                    panel->textDirty = 1;
                    panel->shadowEnabled = style->shadowEnabled;
                    panel->shadowOffsetX = 1;
                    panel->shadowOffsetY = 1;
                    panel->bkColor = style->bkColor;
                    panel->bkMode = style->bkMode;
                }
            }

            zReader::Node *colorNode = zReader_GetNamedNode(
                primitiveNode,
                g_HudZrd_Key_Color
            );
            if (colorNode != 0) {
                zReader::Node *const colorBase = colorNode->value.nodes;
                ((HudUiPrimitiveBindTarget *)(element))->color565 =
                    zVid_PackColorRGB(
                        (unsigned char)(colorBase[1].value.i32),
                        (unsigned char)(colorBase[2].value.i32),
                        (unsigned char)(colorBase[3].value.i32)
                    ) & 0xffffu;
            }

            zReader::Node *relativeEndNode = zReader_GetNamedNode(
                primitiveNode,
                g_HudUiZrdToken_EndPointRelative
            );
            if (relativeEndNode != 0) {
                zReader::Node *const relativeEndBase = relativeEndNode->value.nodes;
                ((HudUiPrimitiveBindTarget *)(element))
                    ->SetSegmentEndpoints(
                        element->GetCenterX(),
                        element->GetCenterY(),
                        element->GetCenterX() + relativeEndBase[1].value.i32,
                        element->GetCenterY() + relativeEndBase[2].value.i32
                    );
            }

            zReader::Node *absoluteEndNode = zReader_GetNamedNode(
                primitiveNode,
                g_HudUiZrdToken_EndPointAbsolute
            );
            if (absoluteEndNode != 0) {
                zReader::Node *const absoluteEndBase = absoluteEndNode->value.nodes;
                ((HudUiPrimitiveBindTarget *)(element))
                    ->SetSegmentEndpoints(
                        element->GetCenterX(),
                        element->GetCenterY(),
                        absoluteEndBase[1].value.i32,
                        absoluteEndBase[2].value.i32
                    );
            }

            HudUiRect clipRect;
            clipRect.left = element->GetCenterX();
            clipRect.top = element->GetCenterY();
            clipRect.right = element->GetCenterX();
            clipRect.bottom = element->GetCenterY();
            element->SetBltSourceAndClipRect(
                capturedCompositeImage,
                &clipRect
            );

            element->flags =
                (unsigned int)((unsigned char)(element->flags) & 0x10u) |
                0x02u;
        }
    }
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduibackground-freeloadedtreeroots
 * @recoil-artifact defines .text recoil:function:0x4ba350: HudUiBackground::FreeLoadedTreeRoots.
 * Purpose: preserve the recovered HUD behavior for HudUiBackground::FreeLoadedTreeRoots.
 */
void HudUiBackground::FreeLoadedTreeRoots(
    int
) {
    zReader::Node *const root = loadedRoot;
    if (root != 0) {
        zReader::FreeLoadedTree(root);
    }

    loadedRoot = 0;
    cfgRoot = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduidialogcontroller-blitownedsurfacetoprimary
 * @recoil-artifact defines .text recoil:function:0x4ba380: HudUiDialogController::BlitOwnedSurfaceToPrimary.
 * Purpose: blit the captured dialog image back to the active primary target.
 */
void HudUiDialogController::BlitOwnedSurfaceToPrimary() {
    if (capturedImage != 0) {
        zVid_Image::BlitToActiveTarget(
            capturedImage,
            0,
            0,
            0,
            0
        );
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicontainer-invalidatechildren
 * @recoil-artifact defines .text recoil:function:0x4ba3a0: HudUiContainer::InvalidateChildren.
 * Purpose: preserve the recovered HUD behavior for HudUiContainer::InvalidateChildren.
 */
void HudUiContainer::InvalidateChildren() {
    for (HudUiElement *child = childHead; child != 0; child = child->next) {
        child->Invalidate();
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduifillbitmap-setnormalizedvalue
 * @recoil-artifact defines .text recoil:function:0x4ba3c0: HudUiFillBitmap::SetNormalizedValue.
 * Purpose: apply the recovered HUD state change handled by HudUiFillBitmap::SetNormalizedValue.
 */
void HudUiFillBitmap::SetNormalizedValue(
    float value
) {
    unsigned int valueBits = 0;
    memcpy(
        &valueBits,
        &value,
        sizeof(valueBits)
    );
    memcpy(
        &normalizedValue,
        &valueBits,
        sizeof(normalizedValue)
    );
    Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduiownedtextinput-onaccept
 * @recoil-artifact defines .text recoil:function:0x4ba3e0: HudUiOwnedTextInput::OnAccept.
 * Purpose: handle the recovered HUD event path for HudUiOwnedTextInput::OnAccept.
 */
void HudUiOwnedTextInput::OnAccept() {
    zGame::ReturnOnlyStub();
    owner->OnAcceptForwardToCommit();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-getwraprect
 * @recoil-artifact defines .text recoil:function:0x4ba400: HudUiPanel::GetWrapRect.
 * Purpose: Returns the panel word-wrap rectangle storage.
 */
HudUiRect * HudUiPanel::GetWrapRect() {
    return &wrapRect;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduilistselectoritem-draw
 * @recoil-artifact defines .text recoil:function:0x4ba410: HudUiListSelectorItem::Draw.
 * Purpose: preserve the recovered HUD behavior for HudUiListSelectorItem::Draw.
 */
void HudUiListSelectorItem::Draw() {
    HudUiPanel::Draw();

    clipRect.left = GetCenterX();
    if (textDirty != 0) {
        RebuildTextRect();
    }

    clipRect.right = GetCenterX() + textWidthPx;
    clipRect.top = GetCenterY();
    const int textHeight = QueryTextHeight();
    clipRect.bottom = textHeight + GetCenterY();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-hudfontstyle-hudfontstyle
 * @recoil-artifact defines .text recoil:function:0x4ba4a0: HudFontStyle::HudFontStyle.
 * Purpose: preserve the recovered HUD behavior for HudFontStyle::HudFontStyle.
 */
HudFontStyle::HudFontStyle() {
    validMarker = 0;
    fontName = 0;
    fontSize = 0;
    textColor = 0;
    shadowEnabled = 0;
    alignMode = 0;
    fontWeight = 0x1f4;
}

/**
 * Source model note: Source-faithful helper recovered from address-backed callers in this
 * source file.
 * Purpose: run the recovered HudFontStyle::Destructor teardown path.
 */
HudFontStyle::~HudFontStyle() {
    Destructor();
}

/**
 * Purpose: run the recovered HudFontStyle::Destructor teardown path.
 */
void HudFontStyle::Destructor() {
    validMarker = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-huduipanel-0x4ba740
 * @recoil-artifact defines .text recoil:function:0x4ba740: HudUiPanel::HudUiPanel.
 * Purpose: Construct a text panel with default font, color, wrapping, and bounds state.
 */
HudUiPanel::HudUiPanel(
    const char *text,
    int initX,
    int initY
) : HudUiTextLabel(
        text,
        initX,
        initY,
        0
) {
    textPick = 0;
    textColor0 = 0x00ffffff;
    textColor1 = 0x00ffffff;
    textDirty = 1;
    hFont = GetStockObject(OEM_FIXED_FONT);
    cachedText[0] = '\0';
    shadowEnabled = 0;
    textDirty = 1;
    alignMode = 0;
    bkMode = TRANSPARENT;
    wrapRect.right = 0;
    wrapRect.left = 0;
    wrapRect.bottom = 0;
    wrapRect.top = 0;
    textRect = wrapRect;
    wordWrapEnabled = 0;
    unknown274 = 0;
    textHeightPx = 0;
    textWidthPx = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-huduipanel-0x4ba850
 * @recoil-artifact defines .text recoil:function:0x4ba850: HudUiPanel::HudUiPanel(const HudUiPanel &).
 * Purpose: Copy-construct panel-owned text and font state from another panel.
 */
HudUiPanel::HudUiPanel(
    const HudUiPanel &source
) : HudUiTextLabel(source) {
    textPick = 0;
    textColor0 = source.textColor0;
    textColor1 = source.textColor1;

    LOGFONTA logFont = {0};
    if (GetObjectA(
        source.hFont,
        sizeof(logFont),
        &logFont
    ) != 0) {
        hFont = CreateFontIndirectA(&logFont);
    }

    cachedTextLength = source.cachedTextLength;
    strncpy(
        cachedText,
        source.cachedText,
        0x100
    );

    textWidthPx = source.textWidthPx;
    textHeightPx = source.textHeightPx;
    shadowEnabled = source.shadowEnabled;
    bkMode = source.bkMode;
    bkColor = source.bkColor;
    textDirty = source.textDirty;
    unknown274 = source.unknown274;
    wordWrapEnabled = source.wordWrapEnabled;
    wrapRect = source.wrapRect;
    textRect = source.textRect;
    alignMode = source.alignMode;
    shadowOffsetX = source.shadowOffsetX;
    shadowOffsetY = source.shadowOffsetY;
}

/**
 * Purpose: Initialize this panel by copying text, font, and layout state from another panel.
 */
HudUiPanel & HudUiPanel::operator=(
    const HudUiPanel &source
) {
    HudUiTextLabel::operator=(source);

    textPick = 0;
    textColor0 = source.textColor0;
    textColor1 = source.textColor1;

    LOGFONTA logFont = {0};
    if (GetObjectA(
        source.hFont,
        sizeof(logFont),
        &logFont
    ) != 0) {
        hFont = CreateFontIndirectA(&logFont);
    }

    cachedTextLength = source.cachedTextLength;
    strncpy(
        cachedText,
        source.cachedText,
        0x100
    );

    textWidthPx = source.textWidthPx;
    textHeightPx = source.textHeightPx;
    shadowEnabled = source.shadowEnabled;
    bkMode = source.bkMode;
    bkColor = source.bkColor;
    textDirty = 1;
    unknown274 = source.unknown274;
    wordWrapEnabled = source.wordWrapEnabled;
    wrapRect = source.wrapRect;
    textRect = source.textRect;
    alignMode = source.alignMode;
    shadowOffsetX = source.shadowOffsetX;
    shadowOffsetY = source.shadowOffsetY;
    return *this;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-huduipanel-0x4bab40
 * @recoil-artifact defines .text recoil:function:0x4bab40: HudUiPanel::~HudUiPanel.
 * Purpose: release the panel-owned text image and font resources during C++
 * object teardown.
 */
HudUiPanel::~HudUiPanel() {
    if (textPick != 0) {
        zVid_Image::Destroy(textPick);
        textPick = 0;
    }

    DeleteObject(hFont);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-setfont
 * @recoil-artifact defines .text recoil:function:0x4babb0: HudUiPanel::SetFont.
 * Purpose: Replace the panel font handle and mark text layout dirty.
 */
void HudUiPanel::SetFont(
    const char *faceName,
    int height,
    int weight,
    int width,
    int italic,
    int charSet,
    int pitchAndFamily
) {
    DeleteObject(hFont);
    hFont = CreateFontA(
        -height,
        width,
        0,
        0,
        weight,
        italic,
        0,
        0,
        charSet,
        OUT_TT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DRAFT_QUALITY,
        pitchAndFamily,
        faceName
    );
    textDirty = 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-rebuildtextrect
 * @recoil-artifact defines .text recoil:function:0x4bac10: HudUiPanel::RebuildTextRect.
 * Purpose: preserve the recovered HUD behavior for HudUiPanel::RebuildTextRect.
 */
void HudUiPanel::RebuildTextRect() {
    if (strlen(textBuffer) == 0) {
        memset(
            &textRect,
            0,
            sizeof(HudUiRect)
        );
        textHeightPx = 0;
        textWidthPx = 0;
        textDirty = 0;
        return;
    }

    HDC measureDc = CreateCompatibleDC(0);
    if (measureDc == 0) {
        textDirty = 0;
        return;
    }

    SelectObject(
        measureDc,
        hFont
    );

    RECT &textRectRef = *(RECT *)(&textRect);
    UINT drawFormat = DT_LEFT;
    BOOL measured = FALSE;
    if (wordWrapEnabled != 0) {
        textRectRef = *(RECT *)(&wrapRect);
        drawFormat = DT_WORDBREAK;
        measured = TRUE;
    } else {
        measured = DrawTextA(
            measureDc,
            textBuffer,
            -1,
            &textRectRef,
            DT_CALCRECT
        );
    }

    if (measured == 0) {
        GetLastError();
    } else {
        if (shadowEnabled != 0) {
            textRectRef.bottom += abs(shadowOffsetY);
            textRectRef.right += abs(shadowOffsetX);
        }

        const int textWidth = textRectRef.right - textRectRef.left;
        const int textHeight = textRectRef.bottom - textRectRef.top;
        textWidthPx = textWidth;
        textHeightPx = textHeight;

        if (textPick != 0 && (textWidth > textPick->width || textHeight > textPick->height)) {
            zVid_Image::Destroy(textPick);
            textPick = 0;
        }

        if (textPick == 0) {
            textPick = zVid_Image::Create();
            zVid_Image::SetFormatCode(
                textPick,
                3
            );
            zVid_Image::SetSize(
                textPick,
                (short)(textWidth),
                (short)(textHeight)
            );
            void *const pixels =
                malloc(zVid_Image::QueryBytesPerPixel(textPick) * textWidth * textHeight);
            zVid_Image_SetPixels(
                textPick,
                pixels,
                0
            );
            textPick->formatFlagsPacked |= 0x20;
        }

        if (textPick != 0) {
            const int clearBytes = zVid_Image::QueryBytesPerPixel(textPick) * textPick->pixelCount;
            memset(
                textPick->pixels,
                0,
                clearBytes
            );

            HDC drawDc = 0;
            zVideo_ImageUploadPixelsProc uploadPixels = g_zVideo_pfnImageUploadPixelsToSurface;
            if (uploadPixels != 0 && uploadPixels(
                textPick,
                &drawDc
            ) != 0) {
                RECT shadowRect = textRectRef;
                RECT mainRect = textRectRef;
                SelectObject(
                    drawDc,
                    hFont
                );

                if (shadowEnabled != 0) {
                    if (shadowOffsetX > 0) {
                        shadowRect.left += shadowOffsetX;
                    } else {
                        mainRect.left -= shadowOffsetX;
                    }

                    if (shadowOffsetY > 0) {
                        shadowRect.top += shadowOffsetY;
                    } else {
                        mainRect.top -= shadowOffsetY;
                    }

                    ::SetTextColor(
                        drawDc,
                        0x00141414
                    );
                    if (bkMode == OPAQUE) {
                        SetBkColor(
                            drawDc,
                            0x20
                        );
                    }

                    SetBkMode(
                        drawDc,
                        bkMode
                    );
                    DrawTextA(
                        drawDc,
                        textBuffer,
                        -1,
                        &shadowRect,
                        drawFormat
                    );
                }

                ::SetTextColor(
                    drawDc,
                    textColor0 == textColor1 ? textColor0 : 0x00ffffff
                );
                if (bkMode == OPAQUE) {
                    SetBkColor(
                        drawDc,
                        bkColor
                    );
                }

                SetBkMode(
                    drawDc,
                    bkMode
                );
                DrawTextA(
                    drawDc,
                    textBuffer,
                    -1,
                    &mainRect,
                    drawFormat
                );

                zVideo_ImageReleaseSurfaceProc releaseSurface = g_zVideo_pfnImageReleaseSurface;
                if (releaseSurface != 0) {
                    releaseSurface(
                        textPick,
                        drawDc
                    );
                }
            }

            TEXTMETRICA metrics = {0};
            if (GetTextMetricsA(
                measureDc,
                &metrics
            ) != 0) {
                if (textColor0 != textColor1) {
                    const unsigned short sourceWhite =
                        (unsigned short)(zVid_PackColorRGB(
                            0xff,
                            0xff,
                            0xff
                        ));
                    unsigned short *pixel = (unsigned short *)(textPick->pixels);
                    {
                        for (int row = 0; row < textPick->height; ++row) {
                            const int lineSpan = metrics.tmHeight + metrics.tmExternalLeading;
                            const int rowPhase = (shadowEnabled != 0
                                                         ? row + shadowOffsetY
                                                         : row) %
                                                 lineSpan;
                            const float blend =
                                (float)(rowPhase - metrics.tmInternalLeading) /
                                (float)(metrics.tmAscent - metrics.tmInternalLeading);
                            const unsigned int blendedColor =
                                HudUiFlashPanel::ComputeFlashBlendColor(
                                    textColor0,
                                    textColor1,
                                    blend
                                );
                            const unsigned short packedColor = (unsigned short)(zVid_PackColorRGB(
                                (unsigned char)(blendedColor & 0xffu),
                                (unsigned char)((blendedColor >> 8) & 0xffu),
                                (unsigned char)((blendedColor >> 16) & 0xffu)
                            ));

                            {
                                for (int col = 0; col < textPick->width; ++col, ++pixel) {
                                    if (*pixel == sourceWhite) {
                                        *pixel = packedColor;
                                    }
                                }
                            }
                        }
                    }
                }

                unknown274 = (int)(metrics.tmExternalLeading);
            }
        }
    }

    DeleteDC(measureDc);
    textDirty = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduiflashpanel-computeflashblendcolor
 * @recoil-artifact defines .text recoil:function:0x4bb0c0: HudUiFlashPanel::ComputeFlashBlendColor.
 * Purpose: clamp endpoint flash colors and blend RGB channels for intermediate flash values.
 */
unsigned int __fastcall HudUiFlashPanel::ComputeFlashBlendColor(
    unsigned int color0,
    unsigned int color1,
    float blend
) {
    const double blendValue = (double)(blend);
    if (!(blendValue >= 0.001)) {
        return color0;
    }
    if (blendValue > 0.999) {
        return color1;
    }

    const double inverseBlend = 1.0 - blendValue;
    const unsigned int blue = (unsigned int)((int)((double)(color0 & 0xffu) * inverseBlend +
                                                   (double)(color1 & 0xffu) * blendValue)) &
                              0xffu;
    const unsigned int green = (unsigned int)((int)((double)((color0 >> 8) & 0xffu) * inverseBlend +
                                                    (double)((color1 >> 8) & 0xffu) * blendValue)) &
                               0xffu;
    const unsigned int red = (unsigned int)((int)((double)((color0 >> 16) & 0xffu) * inverseBlend +
                                                  (double)((color1 >> 16) & 0xffu) * blendValue)) &
                             0xffu;
    return (red << 16) | (green << 8) | blue;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-measuretextprefixrect
 * @recoil-artifact defines .text recoil:function:0x4bb1c0: HudUiPanel::MeasureTextPrefixRect.
 * Purpose: preserve the recovered HUD behavior for HudUiPanel::MeasureTextPrefixRect.
 */
int HudUiPanel::MeasureTextPrefixRect(
    int maxChars,
    RECT *outRect
) {
    int result = 0;
    HDC hdc = CreateCompatibleDC(0);
    if (hdc == 0) {
        return 0;
    }

    SelectObject(
        hdc,
        hFont
    );
    if (maxChars > 0) {
        char *const textCopy = _strdup(textBuffer);
        if (maxChars <= (int)(strlen(textCopy))) {
            textCopy[maxChars] = '\0';
            if (DrawTextA(
                hdc,
                textCopy,
                -1,
                outRect,
                DT_CALCRECT
            ) != 0) {
                result = 1;
            }
        }

        free(textCopy);
        DeleteDC(hdc);
        return result;
    }

    if (DrawTextA(
        hdc,
        "W",
        -1,
        outRect,
        DT_CALCRECT
    ) != 0) {
        result = 1;
        outRect->right = outRect->left;
    }

    DeleteDC(hdc);
    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-updatetextboundsfromcontent
 * @recoil-artifact defines .text recoil:function:0x4bb2a0: HudUiPanel::UpdateTextBoundsFromContent.
 * Purpose: update the panel clip rectangle from current text contents, alignment, wrapping, and shadow state.
 */
void HudUiPanel::UpdateTextBoundsFromContent() {
    char *const panelText = textBuffer;
    const int textLength = (int)(strlen(panelText));

    if (wordWrapEnabled != 0) {
        clipRect.left = x;
        clipRect.top = y;
        clipRect.right = x + wrapRect.right;
        clipRect.bottom = y + wrapRect.bottom;
        return;
    }

    HDC hdc = CreateCompatibleDC(0);
    if (hdc == 0) {
        return;
    }

    SelectObject(
        hdc,
        hFont
    );
    SIZE textSize = {0};
    if (GetTextExtentPoint32A(
        hdc,
        panelText,
        textLength,
        &textSize
    ) != 0) {
        int left;
        if (alignMode == 1) {
            if (textDirty != 0) {
                RebuildTextRect();
            }

            left = clipRect.left + ((clipRect.right - clipRect.left) / 2) - (textWidthPx / 2);
        } else if (alignMode == 0) {
            left = clipRect.left;
        } else {
            if (textDirty != 0) {
                RebuildTextRect();
            }

            left = clipRect.right - textWidthPx;
        }

        clipRect.left = left;
        clipRect.right = left + textSize.cx;
        clipRect.bottom = clipRect.top + textSize.cy;

        if (shadowEnabled != 0) {
            clipRect.bottom += abs(shadowOffsetY);
            clipRect.right += abs(shadowOffsetX);
        }
    }

    DeleteDC(hdc);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-hittest
 * @recoil-artifact defines .text recoil:function:0x4bb3d0: HudUiPanel::HitTest.
 * Purpose: test a point against the current visible text bounds, rebuilding dirty text metrics first.
 */
int HudUiPanel::HitTest(
    int px,
    int py
) {
    if ((flags & 0x10u) != 0 || x > px || y > py) {
        return 0;
    }

    if (textDirty != 0) {
        RebuildTextRect();
    }

    if (px >= x + textWidthPx) {
        return 0;
    }

    return py < y + QueryTextHeight() ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-getlasttextptr
 * @recoil-artifact defines .text recoil:function:0x4bb440: HudUiPanel::GetLastTextPtr.
 * Purpose: return the cached panel text after ensuring dirty text rendering state is rebuilt.
 */
char * HudUiPanel::GetLastTextPtr() {
    if (textDirty != 0) {
        RebuildTextRect();
    }

    return cachedText;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-draw
 * @recoil-artifact defines .text recoil:function:0x4bb460: HudUiPanel::Draw.
 * Purpose: rebuild dirty panel text, draw the panel base, and blit the rendered text image with recovered alignment behavior.
 */
void HudUiPanel::Draw() {
    if (textDirty != 0) {
        RebuildTextRect();
    }

    if (textPick == 0) {
        return;
    }

    if (textBuffer[0] == '\0') {
        DrawBase();
        return;
    }

    if (alignMode == 0) {
        DrawBase();
        zVid_Image::BlitToActiveTarget(
            textPick,
            x,
            y,
            0,
            (zVidRect32 *)(&textRect)
        );
        return;
    }

    if (textDirty != 0) {
        RebuildTextRect();
    }

    int frameWidth = clipRect.right - clipRect.left;
    int textWidth = textWidthPx;
    if (alignMode == 1) {
        frameWidth >>= 1;
        textWidth >>= 1;
    }

    x -= frameWidth;
    DrawBase();

    const int dstX = x + frameWidth - textWidth;
    x = dstX;
    zVid_Image::BlitToActiveTarget(
        textPick,
        dstX,
        y,
        0,
        (zVidRect32 *)(&textRect)
    );
    x += textWidth;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-settextfmt
 * @recoil-artifact defines .text recoil:function:0x4bb540: HudUiPanel::SetTextFmt.
 * Purpose: format stack varargs into the panel text buffer and refresh cached
 * panel text state when the content changes.
 */
void HudUiPanel::SetTextFmt(
    const char *format,
    ...
) {
    if (format == 0) {
        memset(
            textBuffer,
            0,
            sizeof(textBuffer)
        );
        textDirty = 1;
        return;
    }

    va_list args;
    va_start(args, format);
    _vsnprintf(
        textBuffer,
        0x100,
        format,
        args
    );
    va_end(args);

    if (strncmp(
        cachedText,
        textBuffer,
        0x100
    ) == 0) {
        return;
    }

    if (centerText != 0) {
        HudUiTextLabel::UpdateTextExtents();
    }

    Invalidate();
    textDirty = 1;
    strncpy(
        cachedText,
        textBuffer,
        0x100
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-settextfmtv
 * @recoil-artifact defines .text recoil:function:0x4bb5e0: HudUiPanel::SetTextFmtV.
 * Purpose: format a va_list into the panel text buffer and refresh cached
 * panel text state when the content changes.
 */
void __cdecl HudUiPanel::SetTextFmtV(
    const char *format,
    va_list args
) {
    if (format == 0) {
        memset(
            textBuffer,
            0,
            sizeof(textBuffer)
        );
        textDirty = 1;
        return;
    }

    _vsnprintf(
        textBuffer,
        0x100,
        format,
        args
    );
    textBuffer[0xff] = '\0';

    if (strncmp(
        cachedText,
        textBuffer,
        0x100
    ) == 0) {
        return;
    }

    if (centerText != 0) {
        HudUiTextLabel::UpdateTextExtents();
    }

    Invalidate();
    textDirty = 1;
    strncpy(
        cachedText,
        textBuffer,
        0x100
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-settext
 * @recoil-artifact defines .text recoil:function:0x4bb680: HudUiPanel::SetText.
 * Purpose: copy literal panel text and refresh cached panel text state when
 * the content changes.
 */
void HudUiPanel::SetText(
    const char *text
) {
    if (text == 0) {
        memset(
            textBuffer,
            0,
            sizeof(textBuffer)
        );
        textDirty = 1;
        return;
    }

    strncpy(
        textBuffer,
        text,
        0x100
    );

    if (strncmp(
        cachedText,
        textBuffer,
        0x100
    ) == 0) {
        return;
    }

    if (centerText != 0) {
        HudUiTextLabel::UpdateTextExtents();
    }

    Invalidate();
    textDirty = 1;
    strncpy(
        cachedText,
        textBuffer,
        0x100
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-querytextheight
 * @recoil-artifact defines .text recoil:function:0x4bb710: HudUiPanel::QueryTextHeight.
 * Purpose: return the panel text height without external leading after rebuilding dirty text metrics.
 */
int HudUiPanel::QueryTextHeight() {
    if (textDirty != 0) {
        RebuildTextRect();
    }

    return textHeightPx - unknown274;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-gettextrect
 * @recoil-artifact defines .text recoil:function:0x4bb740: HudUiPanel::GetTextRect.
 * Purpose: report the inherited element rectangle extended to the current rendered panel text dimensions.
 */
void HudUiPanel::GetTextRect(
    HudUiRect *outRect
) {
    HudUiElement::GetTextRect(outRect);

    if (textDirty != 0) {
        RebuildTextRect();
    }

    outRect->right = outRect->left + textWidthPx;
    outRect->bottom = outRect->top + QueryTextHeight();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-huduicompositepanel
 * @recoil-artifact defines .text recoil:function:0x4bb790: HudUiCompositePanel::HudUiCompositePanel.
 * Purpose: initialize a composite panel and allocate its entry history vector.
 *
 * Evidence: BN retail 0x4bb790 constructs the HudUiPanel base, initializes the
 * vector member, installs g_HudUiCompositePanel_FTable, builds a stack
 * HudUiCompositePanelEntry template entry, resizes the entry vector, applies
 * text "W", relayouts, and sets the panel visible. BN caller 0x403930 invokes
 * this entry-count construction for HudUiBriefingRuntime::messagesPanel before
 * constructing locatorPanels, matching a C++ member-initializer constructor.
 * Source model note: the recovered constructor symbol is
 * ??0HudUiCompositePanel@@QAE@H@Z.
 */
HudUiCompositePanel::HudUiCompositePanel(
    int entryCount
)
    : HudUiPanel(
        0,
        0,
        0
    ) {
    activeEntryCount = 0;
    HudUiCompositePanelEntry templateEntry;
    entryVector.resize(
        (unsigned int)(entryCount),
        templateEntry
    );

    HudUiPanel::SetTextFmt("W");
    LayoutEntries(
        0,
        0
    );
    ResizeEntryVectorAndRelayout(entryCount);
    SetVisible(1);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-update
 * @recoil-artifact defines .text recoil:function:0x4bb980: HudUiCompositePanel::Update.
 * Purpose: tick flash state for each visible composite-panel entry.
 */
void HudUiCompositePanel::Update(
    float deltaSeconds
) {
    if ((flags & 0x10u) != 0) {
        return;
    }

    for (unsigned int index = 0; index < entryVector.size(); ++index) {
        HudUiTransitionTextPanel *const entry = &entryVector[index];
        entry->Update(deltaSeconds);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-setpos
 * @recoil-artifact defines .text recoil:function:0x4bb9f0: HudUiCompositePanel::SetPos.
 * Purpose: position the composite panel and lay out each text entry below the
 * panel origin.
 *
 * Evidence: BN table g_HudUiCompositePanel_FTable stores 0x4bb9f0 in slot 3,
 * the inherited HudUiElement::SetPos slot; the body writes x/y, invalidates,
 * measures one entry height, and dispatches SetPos on each transition entry.
 */
void HudUiCompositePanel::SetPos(
    int x,
    int y
) {
    this->x = x;
    this->y = y;
    Invalidate();

    const int entryHeight = QueryTextHeight();
    int yOffset = 0;
    for (HudUiCompositePanelVector::iterator entry = entryVector.begin();
        entry != entryVector.end();
        ++entry) {
        entry->SetPos(
            GetCenterX(),
            GetCenterY() + yOffset
        );
        yOffset += entryHeight;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-settextfmt
 * @recoil-artifact defines .text recoil:function:0x4bbaa0: HudUiCompositePanel::SetTextFmt.
 * Purpose: format text into the next composite-panel history entry.
 */
void HudUiCompositePanel::SetTextFmt(
    const char *format,
    ...
) {
    va_list args;
    va_start(
        args,
        format
    );
    SetTextFmtV(
        format,
        args
    );
    va_end(args);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-settextfmtv
 * @recoil-artifact defines .text recoil:function:0x4bbac0: HudUiCompositePanel::SetTextFmtV.
 * Purpose: write formatted text into the active composite entry and scroll
 * history as needed.
 */
void __cdecl HudUiCompositePanel::SetTextFmtV(
    const char *format,
    va_list args
) {
    HudUiTransitionTextPanel *const entry =
        &entryVector[activeEntryCount];
    entry->SetTextFmtV(
        format,
        args
    );
    entry->SetVisible(1);
    ScrollHistory();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-scrollhistory
 * @recoil-artifact defines .text recoil:function:0x4bbb20: HudUiCompositePanel::ScrollHistory.
 * Purpose: shift composite text history entries and keep the newest entry
 * active.
 */
void HudUiCompositePanel::ScrollHistory() {
    ++activeEntryCount;

    if ((unsigned int)(activeEntryCount) >= (unsigned int)(entryVector.size())) {
        {
            for (unsigned int index = 0;
                index < (unsigned int)(entryVector.size()) - 1;
                ++index) {
                HudUiTransitionTextPanel *const current =
                    &entryVector[index];
                HudUiTransitionTextPanel *const next =
                    &entryVector[index + 1];
                current->SetText(next->GetLastTextPtr());
            }
        }
        --activeEntryCount;
    }

    Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-setfont
 * @recoil-artifact defines .text recoil:function:0x4bbbe0: HudUiCompositePanel::SetFont.
 * Purpose: apply font parameters to all composite-panel entries and relayout
 * the panel.
 */
void HudUiCompositePanel::SetFont(
    const char *faceName,
    int height,
    int weight,
    int width,
    int italic,
    int charSet,
    int pitchAndFamily
) {
    for (unsigned int index = 0; index < entryVector.size(); ++index) {
        HudUiTransitionTextPanel *const entry = &entryVector[index];
        entry->SetFont(
            faceName,
            height,
            weight,
            width,
            italic,
            charSet,
            pitchAndFamily
        );
    }

    HudUiPanel::SetFont(
        faceName,
        height,
        weight,
        width,
        italic,
        charSet,
        pitchAndFamily
    );

    SetPos(
        GetCenterX(),
        GetCenterY()
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-resizeentryvectorandrelayout
 * @recoil-artifact defines .text recoil:function:0x4bbca0: HudUiCompositePanel::ResizeEntryVectorAndRelayout.
 * Purpose: resize the composite-entry vector, update active entries, and
 * relayout the panel.
 */
void HudUiCompositePanel::ResizeEntryVectorAndRelayout(
    int entryCount
) {
    const int oldCount = (int)(entryVector.size());

    if (entryCount != oldCount) {
        HudUiCompositePanelEntry templateEntry;

        if (entryCount > oldCount) {
            entryVector.insert(
                entryVector.end(),
                (unsigned int)(entryCount - oldCount),
                templateEntry
            );
        } else {
            entryVector.erase(
                entryVector.begin() + entryCount,
                entryVector.end()
            );
        }

        ResizeEntryCount(
            oldCount,
            entryCount
        );
    } else {
        ReapplyEntryCount();
    }

    SetPos(
        GetCenterX(),
        GetCenterY()
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-reapplyentrycount
 * @recoil-artifact defines .text recoil:function:0x4bbe90: HudUiCompositePanel::ReapplyEntryCount.
 * Purpose: reapply the current composite-entry count after vector changes.
 */
void HudUiCompositePanel::ReapplyEntryCount() {
    ResizeEntryCount(
        0,
        (int)(entryVector.size())
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-resizeentrycount
 * @recoil-artifact defines .text recoil:function:0x4bbed0: HudUiCompositePanel::ResizeEntryCount.
 * Purpose: update composite-entry visibility for the requested active count.
 */
void HudUiCompositePanel::ResizeEntryCount(
    int oldCount,
    int entryCount
) {
    if (oldCount > entryCount) {
        oldCount = entryCount;
    }
    if (oldCount < 0) {
        oldCount = 0;
    }

    const int vectorCount = (int)(entryVector.size());
    if (entryCount > vectorCount) {
        entryCount = vectorCount;
    }

    {
        for (int index = oldCount; index < entryCount; ++index) {
            HudUiTransitionTextPanel *const entry = &entryVector[index];
            entry->SetTextFmt("");
            entry->SetVisible(0);
        }
    }

    activeEntryCount = oldCount;
}
