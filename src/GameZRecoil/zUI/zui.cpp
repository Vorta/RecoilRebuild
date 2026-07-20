#include "Battlesport/Mfc42Abi.h"

#include "GameZRecoil/zHud/zhud_ui.h"

#include "Battlesport/briefing.h"
#include "Battlesport/cz_recoil_frame.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud_ui_net_game_setup.h"
#include "Battlesport/recoil_state_credits.h"
#include "Battlesport/hud.h"
#include "Battlesport/player.h"
#include "GameZRecoil/RecoilApp/recoil_state_main_menu_transition.h"
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
#include "GameZRecoil/zRndr/zrndr.h"

/*
 * Ordinary virtual-destructor provenance retained for the compiler-generated
 * lifecycle rows after removal of the synthetic named-slot ABI.
 * Reimplements 0x40a920: HudCmdDialog lifecycle contribution.
 * Reimplements 0x40daa0: HudUiMessage lifecycle contribution.
 * Reimplements 0x40dbd0: HudUiSlot lifecycle contribution.
 * Reimplements 0x40f2b0: HudUiTripletPanel lifecycle contribution.
 * Reimplements 0x40fa20: HudUiStatsListElement lifecycle contribution.
 * Reimplements 0x41a570: HudUiCycleSelectorWidget lifecycle contribution.
 * Reimplements 0x41a590: HudUiCheckToggleWidget lifecycle contribution.
 * Reimplements 0x41c480: HudUiZrdWidget lifecycle contribution.
 * Reimplements 0x41c4a0: HudUiNumericTextInput lifecycle contribution.
 * Reimplements 0x41c4c0: HudUiZrdWidgetEx17C lifecycle contribution.
 * Reimplements 0x4b3ce0: HudUiWidget lifecycle contribution.
 * Reimplements 0x4b4a90: HudUiNumericTextInput lifecycle contribution.
 * Reimplements 0x4b50a0: HudUiZrdWidget lifecycle contribution.
 * Reimplements 0x4b7000: HudUiCheckToggleWidget lifecycle contribution.
 * Reimplements 0x4b7dc0: HudUiCycleSelectorWidget lifecycle contribution.
 * Reimplements 0x4b84b0: HudUiFillBitmap lifecycle contribution.
 * Reimplements 0x4b87a0: HudUiZrdWidgetEx17C_Item lifecycle contribution.
 * Reimplements 0x4b8b40: HudUiZrdWidgetEx17C lifecycle contribution.
 * Reimplements 0x4b9740: HudUiBackground lifecycle contribution.
 * Reimplements 0x4bb960: HudUiCompositePanel lifecycle contribution.
 */
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zTurret/zturret.h"
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

struct zTimedTask {
    zTimedTask *next;
    int kind;
    int flags;
    float remainingSeconds;
    int actionArg0;
    int actionArg1;
    int actionArg2;
    int actionArg3;
    int actionArg4;
    unsigned char payload_24[0x94];
    int alphaPointCount;
    int alphaVariantIndex;
    int alpha255;
    unsigned char payload_c4[0x48];
    int rasterVertexCount;
    int rasterDrawParam;

    void RemoveFromActiveList();
    void RunImmediateAction();
    static void TickActiveList();
};

RECOIL_STATIC_ASSERT(offsetof(zTimedTask, next) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, kind) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, flags) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, remainingSeconds) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, actionArg0) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, actionArg4) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, alphaPointCount) == 0xb8);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, alphaVariantIndex) == 0xbc);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, alpha255) == 0xc0);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, rasterVertexCount) == 0x10c);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, rasterDrawParam) == 0x110);

/**
 * Reimplements data 0x56bd30: g_zTimedTask_ActiveCount.
 * Purpose: preserve the recovered HUD global storage for g_zTimedTask_ActiveCount.
 */
int g_zTimedTask_ActiveCount = 0;
/**
 * Reimplements data 0x56bd34: g_zTimedTask_ActiveHead.
 * Purpose: preserve the recovered HUD global storage for g_zTimedTask_ActiveHead.
 */
zTimedTask *g_zTimedTask_ActiveHead = 0;
/**
 * Reimplements data 0x56bd38: g_zTimedTask_ActiveTail.
 * Purpose: preserve the recovered HUD global storage for g_zTimedTask_ActiveTail.
 */
zTimedTask *g_zTimedTask_ActiveTail = 0;

/**
 * Reimplements data 0x4e5e00: g_HudCmdMouseDebounceFrames.
 * Purpose: preserve the recovered HUD global storage for g_HudCmdMouseDebounceFrames.
 */
int g_HudCmdMouseDebounceFrames = 0;
/**
 * Reimplements data 0x56bd1c: g_HudUiWidget_ExclusiveDrawImage.
 * Purpose: preserve the recovered HUD global storage for g_HudUiWidget_ExclusiveDrawImage.
 */
zVidImagePartial *g_HudUiWidget_ExclusiveDrawImage = 0;
/**
 * Reimplements data 0x4dd1d8: g_HudUiMgrSensor_RoundRobinTrackIndex.
 * Owner data: four-byte signed index initialized to -1 (ff ff ff ff).
 * Purpose: seed the sensor-target round-robin candidate selector before
 * HudUiMgrSensor::UpdateMarkersAndProgressFromVariantTag advances it.
 */
int g_HudUiMgrSensor_RoundRobinTrackIndex = -1;
HudUiRect g_HudUiMgrSensor_FxRectScratch = {0};

#undef g_HudUiNetGameSetupOverlayOwner
#undef g_HudUiMgr
#undef g_HudLayoutHW
#undef g_HudLayoutSW
#undef g_HudCmdDialogState

union HudUiSensorWindowStorage {
    unsigned long align;
    unsigned char bytes[sizeof(CWnd)];
};
RECOIL_STATIC_ASSERT(sizeof(HudUiSensorWindowStorage) == sizeof(CWnd));


/**
 * Reimplements data 0x4f32a0: g_HudUiNetGameSetupOverlayOwner.
 * Source model: zero-initialized global object storage for the
 * HudUiNetGameSetupOverlayOwner static lifecycle.
 * Purpose: hold the multiplayer setup overlay owner singleton constructed by
 * the static initializer and destroyed through the at-exit thunk.
 */
HudUiNetGameSetupOverlayOwnerStorage g_HudUiNetGameSetupOverlayOwner = {0};

/*
 * Retail HUD UI storage and CRT initialization are not the same sequence.
 * Keep these definitions in the recovered storage run; VC5's compiler-emitted
 * static-lifetime helpers for HudLayoutSW and HudLayoutHW, together with the
 * HudUiMgr initialization and provider CString/CWnd constructors, form the
 * retail CRT pass:
 * CString, HudLayoutSW, HudLayoutHW, HudUiMgr, then CWnd.
 */
/**
 * Reimplements data 0x4e5e90: g_HudUiSensorWindow.
 * Source model: zero-initialized provider CWnd storage; the explicit HUD CRT
 * row constructs it and registers the provider destructor.
 * Purpose: preserve the recovered HUD global storage for g_HudUiSensorWindow.
 */
HudUiSensorWindowStorage g_HudUiSensorWindow = {0};
/**
 * Reimplements data 0x4e5ed0: g_HudUiMgr.
 * Source model: zero-initialized HudUiMgrData storage; HudUiMgr::StaticInit
 * constructs the typed manager through the explicit CRT row.
 * Purpose: preserve the recovered HUD global storage for g_HudUiMgr.
 */
HudUiMgrDataStorage g_HudUiMgr = {0};
/**
 * Reimplements data 0x4eda68: g_HudLayoutSW.
 * Owner data: typed 236-byte singleton with compiler-owned static lifetime.
 * Retail startup constructs the software layout before the hardware layout.
 * Purpose: own the global software HUD layout instance.
 */
HudLayoutSW g_HudLayoutSW;
/**
 * Reimplements data 0x4ed718: g_HudLayoutHW.
 * Owner data: typed 844-byte singleton with compiler-owned static lifetime.
 * Purpose: own the global hardware HUD layout instance.
 */
HudLayoutHW g_HudLayoutHW;

#define g_HudUiNetGameSetupOverlayOwner \
    (*(HudUiNetGameSetupOverlayOwner *)&g_HudUiNetGameSetupOverlayOwner)
#define g_HudUiSensorWindow \
    (*(CWnd *)&g_HudUiSensorWindow)
#define g_HudUiMgr \
    (*(HudUiMgrData *)&g_HudUiMgr)

HudUiRect g_HudUiMgrSensorFxRect = {0};
int g_HudUiMgrSensorFxViewportWidth = 0;
int g_HudUiMgrSensorFxViewportHeight = 0;

/**
 * Reimplements data 0x4edb70: g_HudUiSensorWindowPlayback.
 * Purpose: preserve the recovered HUD global storage for g_HudUiSensorWindowPlayback.
 */
zFMV_Playback *g_HudUiSensorWindowPlayback = 0;

// Moved HUD runtime bodies live in src/Battlesport/hud_runtime_layer_body.h
// and are included by src/Battlesport/hud.cpp for physical HUD order.

extern "C" {
HudUiMgrSensorTrackList g_HudUiMgrSensor_TrackList = {0};
}
/**
 * Reimplements data 0x56bd20: g_HudUiChatMessageStack.
 * Purpose: preserve the recovered HUD global storage for g_HudUiChatMessageStack.
 */
HudUiTextStack4 *g_HudUiChatMessageStack = 0;
/**
 * Reimplements data 0x56bd24: g_HudUiTopMessageStack.
 * Purpose: preserve the recovered HUD global storage for g_HudUiTopMessageStack.
 */
HudUiTextStack4 *g_HudUiTopMessageStack = 0;
/**
 * Reimplements data 0x4f3aa8: g_HudUi_AuxOverlayEnabled.
 * Purpose: preserve the recovered HUD global storage for g_HudUi_AuxOverlayEnabled.
 */
int g_HudUi_AuxOverlayEnabled = 0;
/**
 * Reimplements data 0x4e5df0: g_HudCmdDialogState.
 * BN identifies 0x4e5df0 as an eight-byte BSS HudCmdDialogState object. VC5 emits the
 * 0x40bc20/0x40bc30/0x40bc40/0x40bc50 static init and at-exit thunks from this global object.
 * Purpose: preserve the recovered HUD global storage for g_HudCmdDialogState.
 */
HudCmdDialogStateStorage g_HudCmdDialogState = {0};

#define g_HudCmdDialogState \
    (*(HudCmdDialogState *)&g_HudCmdDialogState)

/**
 * Reimplements data 0x4dac00: g_HudUiOptionsPanel_ResolutionCycleNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD resolution selector node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_ResolutionCycleNodeName[] = "RESOLUTION_CYCLE";
/**
 * Reimplements data 0x4dac14: g_HudUiOptionsPanel_MusicVolumeWidgetNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD music-volume widget node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_MusicVolumeWidgetNodeName[] = "MUSIC_VOLUME";
/**
 * Reimplements data 0x4dac24: g_HudUiOptionsPanel_MusicEnableToggleNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD music-enable toggle node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_MusicEnableToggleNodeName[] = "MUSIC_ENABLE";
/**
 * Reimplements data 0x4dac34: g_HudUiOptionsPanel_SoundVolumeWidgetNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD sound-volume widget node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_SoundVolumeWidgetNodeName[] = "SOUND_VOLUME";
/**
 * Reimplements data 0x4dac44: g_HudUiOptionsPanel_SoundQualitySelectorNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD sound-quality selector node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_SoundQualitySelectorNodeName[] = "SOUND_QUALITY";
/**
 * Reimplements data 0x4dac54: g_HudUiOptionsPanel_SoundActiveToggleNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD sound-active toggle node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_SoundActiveToggleNodeName[] = "SOUND_ACTIVE";
/**
 * Reimplements data 0x4dac64: g_EffectsZrdNodeName
 * (BN: g_HudUiOptionsPanel_EffectsNodeName).
 * Shared data owner: effects_weapons.shared_effects_zrd_node_name; this is
 * not HudOptionsDialog-owned data.
 * Purpose: name the shared EFFECTS ZRD node consumed by HudOptionsDialog and
 * zEffect::InitFromPath.
 */
char g_EffectsZrdNodeName[8] = "EFFECTS";
/**
 * Reimplements data 0x4dac6c: g_HudUiOptionsPanel_TextureMemorySelectorNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD texture-memory selector node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_TextureMemorySelectorNodeName[] = "TEXTURE_MEMORY";
/**
 * Reimplements data 0x4dac7c: g_HudUiOptionsPanel_ObjectDetailSelectorNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD object-detail selector node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_ObjectDetailSelectorNodeName[] = "OBJECT_DETAIL";
/**
 * Reimplements data 0x4dac8c: g_HudUiOptionsPanel_FullHudToggleNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD full-HUD toggle node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_FullHudToggleNodeName[] = "FULLHUD";
/**
 * Reimplements data 0x4dac94: g_HudUiOptionsPanel_PerspectiveToggleNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD perspective toggle node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_PerspectiveToggleNodeName[] = "PERSPECTIVE";
/**
 * Reimplements data 0x4daca0: g_HudUiOptionsPanel_LightingToggleNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD lighting toggle node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_LightingToggleNodeName[] = "LIGHTING";
/**
 * Reimplements data 0x4dacac: g_HudUiOptionsPanel_SectionName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD options-panel section loaded by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_SectionName[] = "OPTIONSPANEL";
extern char g_HudFontName_Arial[];
/**
 * Reimplements data 0x4dad2c..0x4dadd8:
 * hud_ui.hud_ui_mgr_ensure_hud_loaded_literals.
 * Source model: writable HUD ZRD key/source-path string globals shared by
 * HudUiMgr::EnsureHudLoaded and matching reader paths in zTurret/zImage.
 * Purpose: name the HUD layout sections and diagnostics consumed while the
 * HUD singleton loads its ZRD tree.
 */
/**
 * Reimplements data 0x4dad2c: g_HudCfgKey_Modes.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Modes.
 */
char g_HudCfgKey_Modes[6] = "MODES";
/**
 * Reimplements data 0x4dad34: g_HudCfgKey_Weapon.
 * Purpose: preserve the recovered shared WEAPON reader-key storage.
 */
char g_HudCfgKey_Weapon[7] = "WEAPON";
/**
 * Reimplements data 0x4dad3c: g_HudCfgKey_Target.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Target.
 */
char g_HudCfgKey_Target[7] = "TARGET";
/**
 * Reimplements data 0x4dad44: g_HudCfgKey_Shield.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Shield.
 */
char g_HudCfgKey_Shield[7] = "SHIELD";
/**
 * Reimplements data 0x4dad4c: g_HudUiBlankSpaces8.
 * Purpose: preserve the recovered HUD global storage for g_HudUiBlankSpaces8.
 */
char g_HudUiBlankSpaces8[9] = "        ";
/**
 * Reimplements data 0x4dad58: g_HudCfgKey_Stats.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Stats.
 */
char g_HudCfgKey_Stats[6] = "STATS";
/**
 * Reimplements data 0x4dad60: g_HudCfgKey_Reticule.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Reticule.
 */
char g_HudCfgKey_Reticule[9] = "RETICULE";
/**
 * Reimplements data 0x4dad6c: g_HudCfgKey_Objective.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Objective.
 */
char g_HudCfgKey_Objective[10] = "OBJECTIVE";
/**
 * Reimplements data 0x4dad78: g_HudCfgKey_Sensor.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Sensor.
 */
char g_HudCfgKey_Sensor[7] = "SENSOR";
/**
 * Reimplements data 0x4dad80: g_HudCfgKey_Nanite.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Nanite.
 */
char g_HudCfgKey_Nanite[7] = "NANITE";
/**
 * Reimplements data 0x4dad88: g_HudCfgKey_Ammo.
 * Purpose: preserve the recovered shared AMMO reader-key storage.
 */
char g_HudCfgKey_Ammo[5] = "AMMO";
/**
 * Reimplements data 0x4dad90: g_HudCfgKey_Strings.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Strings.
 */
char g_HudCfgKey_Strings[8] = "STRINGS";
/**
 * Reimplements data 0x4dad98: g_HudCfgKey_ObjectiveDescription.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_ObjectiveDescription.
 */
char g_HudCfgKey_ObjectiveDescription[16] = "OBJ_DESCRIPTION";
/**
 * Reimplements data 0x4dada8: g_HudCfgKey_ObjectiveSummary.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_ObjectiveSummary.
 */
char g_HudCfgKey_ObjectiveSummary[12] = "OBJ_SUMMARY";
/**
 * Reimplements data 0x4dadb4: g_HudCfgKey_Fonts.
 * Purpose: preserve the recovered shared FONTS reader-key storage.
 */
char g_HudCfgKey_Fonts[6] = "FONTS";
/**
 * Reimplements data 0x4dadbc: g_Hud_ImageSearchPath_Hud.
 * Purpose: preserve the recovered HUD global storage for g_Hud_ImageSearchPath_Hud.
 */
char g_Hud_ImageSearchPath_Hud[26] = "..\\data\\common\\images\\hud";
/**
 * Reimplements data 0x4dadd8: g_Hud_SourceFile_HudCpp.
 * Purpose: preserve the recovered HUD global storage for g_Hud_SourceFile_HudCpp.
 */
char g_Hud_SourceFile_HudCpp[28] = "D:\\Proj\\Battlesport\\hud.cpp";
/**
 * Reimplements data 0x4dadf4: g_HudSensorTracker_ReadFileFailedFmt.
 * Shared data owner: hud_ui.shared_zrd_read_failed_format_literal.
 * Purpose: provide the shared failed-read diagnostic format used by HUD,
 * mission, pickup, turret, image, and opt-catalog ZRD load paths.
 */
char g_HudSensorTracker_ReadFileFailedFmt[18] = "Failed to read %s";
/**
 * Reimplements data 0x4db428: g_HudZrd_Key_Color.
 * Shared data owner: hud_ui.background_primitive_zrd_key_literals.
 * Purpose: name shared COLOR ZRD records consumed by HUD primitive binding.
 */
char g_HudZrd_Key_Color[0x6] = "COLOR";
/**
 * Reimplements data 0x4e46d0..0x4e4704:
 * hud_ui.hud_ui_zrd_widget_base_zrd_key_literals.
 * Source model: writable HUD ZRD key string globals consumed by the base
 * HudUiZrdWidget loaders. BN shows the six char[] objects in this order with
 * the address-aligned padding between ACTIVATE/DISABLE, RATE/FLASH,
 * FLASH/LABEL, LABEL/ROLLOVER, and ROLLOVER/BITMAP.
 * Purpose: name the optional activation, disable, rollover, label, and flash
 * records in a recovered HudUiZrdWidget section.
 */
/**
 * Reimplements data 0x4e46d0: g_HudZrd_Key_Activate.
 * Purpose: preserve the recovered HUD global storage for g_HudZrd_Key_Activate.
 */
char g_HudZrd_Key_Activate[0x9] = "ACTIVATE";
/**
 * Reimplements data 0x4e46dc: g_HudZrd_Key_Disable.
 * Purpose: preserve the recovered HUD global storage for g_HudZrd_Key_Disable.
 */
char g_HudZrd_Key_Disable[0x8] = "DISABLE";
/**
 * Reimplements data 0x4e46e4: g_HudZrd_Key_Rate.
 * Purpose: preserve the recovered HUD global storage for g_HudZrd_Key_Rate.
 */
char g_HudZrd_Key_Rate[0x5] = "RATE";
/**
 * Reimplements data 0x4e46ec: g_HudZrd_Key_Flash.
 * Purpose: preserve the recovered HUD global storage for g_HudZrd_Key_Flash.
 */
char g_HudZrd_Key_Flash[0x6] = "FLASH";
/**
 * Reimplements data 0x4e46f4: g_HudZrd_Key_Label.
 * Purpose: preserve the recovered HUD global storage for g_HudZrd_Key_Label.
 */
char g_HudZrd_Key_Label[0x6] = "LABEL";
/**
 * Reimplements data 0x4e46fc: g_HudZrd_Key_Rollover.
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
 * Reimplements data 0x4e4708: g_HudUiCycleSelectorWidget_ZrdKey_Bitmap.
 * Shared data owner: hud_ui.cycle_selector_shared_zrd_key_literals.
 * Purpose: name the shared BITMAP ZRD record consumed by HUD widget loaders.
 */
char g_HudUiCycleSelectorWidget_ZrdKey_Bitmap[] = "BITMAP";
/**
 * Reimplements data 0x4e4710: g_HudZrd_Key_Position.
 * Shared data owner: hud_ui.background_primitive_zrd_key_literals.
 * Purpose: name shared POSITION ZRD records consumed by HUD widget loaders.
 */
char g_HudZrd_Key_Position[0x9] = "POSITION";
/**
 * Reimplements data 0x4e471c..0x4e4744:
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
 * Reimplements data 0x4e4738: g_HudUiCycleSelectorWidget_ZrdKey_Text.
 * Shared data owner: hud_ui.cycle_selector_shared_zrd_key_literals.
 * BN exposes four TEXT bytes followed by aligned zero padding before CHECKED;
 * keep the pool slot contiguous so C-string lookups see the terminator.
 * Purpose: name shared TEXT ZRD records consumed by toggle and cycle widgets.
 */
char g_HudUiCycleSelectorWidget_ZrdKey_Text[8] = {'T', 'E', 'X', 'T'};
char g_HudUiZrdKey_Checked[0x7] = {'C', 'H', 'E', 'C', 'K', 'E', 'D'};
/**
 * Reimplements data 0x4e4748: g_HudUiCycleSelectorWidget_ZrdKey_Cycle.
 * Shared data owner: hud_ui.cycle_selector_shared_zrd_key_literals.
 * Purpose: name the CYCLE ZRD array loaded by HudUiCycleSelectorWidget.
 */
char g_HudUiCycleSelectorWidget_ZrdKey_Cycle[] = "CYCLE";
/**
 * Reimplements data 0x4e4750: g_HudUiCycleSelectorWidget_ZrdKey_TextOffset.
 * Shared data owner: hud_ui.cycle_selector_shared_zrd_key_literals.
 * Purpose: name the TEXTOFFSET ZRD array loaded by HudUiCycleSelectorWidget.
 */
char g_HudUiCycleSelectorWidget_ZrdKey_TextOffset[] = "TEXTOFFSET";
/**
 * Reimplements data 0x4e475c: g_HudUiCycleSelectorWidget_ZrdKey_Font.
 * Shared data owner: hud_ui.cycle_selector_shared_zrd_key_literals.
 * Purpose: name shared FONT ZRD records consumed by HUD widget loaders.
 */
char g_HudUiCycleSelectorWidget_ZrdKey_Font[] = "FONT";
/**
 * Reimplements data 0x4e47c0..0x4e4835:
 * hud_ui.zhud_background_config_zrd_key_literals.
 * Source model: writable HudUiBackground ZRD key string globals consumed by
 * HudUiBackground::LoadZrdAndSection. BN shows the keys in this order, with
 * VC5 char-array alignment padding between adjacent slots.
 * Purpose: name the background resource, cursor, capture, and sound records
 * in a recovered background ZRD section.
 */
/**
 * Reimplements data 0x4e47c0: zHudCfgKey_BACKGROUND_SOUNDS.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_BACKGROUND_SOUNDS.
 */
char zHudCfgKey_BACKGROUND_SOUNDS[0x12] = "BACKGROUND_SOUNDS";
/**
 * Reimplements data 0x4e47d4: zHudCfgKey_CAPTURE.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_CAPTURE.
 */
char zHudCfgKey_CAPTURE[0x8] = "CAPTURE";
/**
 * Reimplements data 0x4e47dc: zHudCfgKey_CURSOR.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_CURSOR.
 */
char zHudCfgKey_CURSOR[0x7] = "CURSOR";
/**
 * Reimplements data 0x4e47e4: zHudCfgKey_BACKGROUND_TEXT.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_BACKGROUND_TEXT.
 */
char zHudCfgKey_BACKGROUND_TEXT[0x10] = "BACKGROUND_TEXT";
/**
 * Reimplements data 0x4e47f4: zHudCfgKey_BACKGROUND_VIDEOS.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_BACKGROUND_VIDEOS.
 */
char zHudCfgKey_BACKGROUND_VIDEOS[0x12] = "BACKGROUND_VIDEOS";
/**
 * Reimplements data 0x4e4808: zHudCfgKey_BACKGROUND_IMAGES.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_BACKGROUND_IMAGES.
 */
char zHudCfgKey_BACKGROUND_IMAGES[0x12] = "BACKGROUND_IMAGES";
/**
 * Reimplements data 0x4e4824: zHudCfgKey_SHARED_IMAGE_PATH.
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
 * Reimplements data 0x4e4838: g_HudUiZrdToken_Buttons.
 * Data owner: hud_ui.hud_ui_background_buttons_zrd_key_literal.
 * Purpose: name the BUTTONS child table consumed by
 * HudUiBackground::BindButtonsNodeToWidgetByName.
 */
char g_HudUiZrdToken_Buttons[0x8] = "BUTTONS";
RECOIL_STATIC_ASSERT(sizeof(g_HudUiZrdToken_Buttons) == 0x8);
/**
 * Reimplements data 0x4e4840: g_HudUiZrdToken_EndPointAbsolute.
 * Data owner: hud_ui.background_primitive_zrd_key_literals.
 * Purpose: name the absolute endpoint ZRD record consumed by HUD primitive binding.
 */
char g_HudUiZrdToken_EndPointAbsolute[0x9] = "ENDP_ABS";
/**
 * Reimplements data 0x4e484c: g_HudUiZrdToken_EndPointRelative.
 * Data owner: hud_ui.background_primitive_zrd_key_literals.
 * Purpose: name the relative endpoint ZRD record consumed by HUD primitive binding.
 */
char g_HudUiZrdToken_EndPointRelative[0x9] = "ENDP_REL";
/**
 * Reimplements data 0x4e4858: g_HudUiZrdToken_WordWrap.
 * Data owner: hud_ui.background_primitive_zrd_key_literals.
 * Purpose: name the WORDWRAP ZRD record consumed by HUD primitive binding.
 */
char g_HudUiZrdToken_WordWrap[0x9] = "WORDWRAP";
/**
 * Reimplements data 0x4e4864: g_HudUiBackground_ZrdKey_Primitives.
 * Data owner: hud_ui.background_primitive_zrd_key_literals.
 * Purpose: name the PRIMITIVES ZRD container consumed by HUD primitive binding.
 */
char g_HudUiBackground_ZrdKey_Primitives[0xb] = "PRIMITIVES";
/**
 * Reimplements data 0x4e4764: g_HudUiFillBitmap_ZrdKey_FillBitmap.
 * Data owner: hud_ui.hud_ui_fill_bitmap_zrd_key_literals.
 * Purpose: name the FILLBITMAP ZRD record consumed by HudUiFillBitmap.
 */
char g_HudUiFillBitmap_ZrdKey_FillBitmap[] = "FILLBITMAP";
/**
 * Reimplements data 0x4e4770: g_HudUiZrdWidgetEx17C_Item_ZrdKey_MouseRect.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the optional mouse-rectangle ZRD child loaded by HudUiZrdWidgetEx17C_Item.
 */
char g_HudUiZrdWidgetEx17C_Item_ZrdKey_MouseRect[] = "MOUSERECT";
/**
 * Reimplements data 0x4e477c: g_HudUiZrdToken_Radio.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the RADIO ZRD child array loaded by HudUiZrdWidgetEx17C.
 */
char g_HudUiZrdToken_Radio[] = "RADIO";
/**
 * Reimplements data 0x4dc17c: g_HudZrd_Key_Sound.
 * Shared data owner: hud_ui.shared_zrd_sound_key_literal.
 * Purpose: name the shared SOUND ZRD record consumed by HUD widget, pickup,
 * and opt-catalog sound loaders.
 */
char g_HudZrd_Key_Sound[6] = "SOUND";
/**
 * Reimplements data 0x4dae08: g_HudUiMessage_ClearSpecialToken165.
 * Data owner: hud_ui.hud_ui_message_clear_special_token_literal.
 * Exact extent is the writable 4-byte .data object a5 00 00 00 referenced by
 * HudUiMessage::SetValueIfOwnerMatches and
 * HudUiMessage::UpdateSelectedWeaponDisplay.
 * Purpose: provide the special one-byte token string used to clear HUD message
 * panel text when the sentinel float value is passed.
 */
char g_HudUiMessage_ClearSpecialToken165[4] = "\xa5";
/**
 * Reimplements data 0x4dae0c: g_HudLayout_TypeISectionName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the TYPEI HUD layout section loaded from the HUD ZRD root.
 */
char g_HudLayout_TypeISectionName[] = "TYPEI";
/**
 * Reimplements data 0x4dae14: g_HudLayout_TypeIISectionName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the TYPEII HUD layout section loaded from the HUD ZRD root.
 */
char g_HudLayout_TypeIISectionName[] = "TYPEII";
/**
 * Reimplements data 0x4dae1c: g_HudUiBlankSpaces3.
 * Data owner: hud_ui.hud_ui_message_layout_literals.
 * Exact extent is the writable 4-byte .data object 20 20 20 00 referenced by
 * HudUiMessage::LoadWeaponLayoutFromNode.
 * Purpose: provide the initial blank weapon-message panel text.
 */
char g_HudUiBlankSpaces3[4] = "   ";
/**
 * Reimplements data 0x4dae20: g_Hud_CheckpointOverflowMsg.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable checkpoint-overflow diagnostic text used by
 * HudUiLoadingCheckpoint::AdvanceAndLog.
 */
char g_Hud_CheckpointOverflowMsg[20] = "Checkpoint overflow";
/**
 * Reimplements data 0x4dae40: g_HudUiMessage_NodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the objective HUD message node used for chat and save/load status prompts.
 */
char g_HudUiMessage_NodeName[8] = "Message";
/**
 * Reimplements data 0x4dae48: g_HudUiMessage_SeparatorColon.
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
 * Reimplements data 0x4e4870: g_HudUi_InvalidateMask.
 * Purpose: Stores g HudUi InvalidateMask data used by hud_ui.invalidate_mask_global.
 */
unsigned int g_HudUi_InvalidateMask = 0x0c;
}

namespace {
const char kNumericTextInputAcceptedRawKeyChars[] = "0123456789.-\x1b\r\x08\x7f\x02\x06";
const char kClampedIntTextInputAcceptedRawKeyChars[] = "0123456789\x1b\r\x08\x7f\x02\x06";

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
void HudUiSetPanelVectorVisible(
    HudUiPanelPtrVector &panels,
    int visible
) {
    for (HudUiPanel **it = panels.begin; it != panels.end; ++it) {
        (*it)->SetVisible(visible);
    }
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x414670 HudUiTripletEntries::GetCount callers.
 * Purpose: preserve the recovered HUD behavior for ZrdArrayBase.
 */
zReader::Node *ZrdArrayBase(
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
int ZrdArrayCount(
    zReader::Node *arrayBase
) {
    return arrayBase != 0 ? arrayBase[0].value.i32 : 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x414670 HudUiTripletEntries::GetCount callers.
 * Purpose: preserve the recovered HUD behavior for ZrdArrayItem.
 */
zReader::Node *ZrdArrayItem(
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
const char *ZrdArrayString(
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
int ZrdArrayInt(
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
float ZrdArrayFloat(
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
const HudFontStyle *HudUiZrdOwnerFontStyle(
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
void ApplyHudFontStyleToPanel(
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
void ApplyHudFontStyleTextOnly(
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
void DeleteHudUiListSelectorItemArray(
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
 * Purpose: preserve the recovered HUD behavior for CreateHudZrdLabelPanel.
 */
HudUiPanel *CreateHudZrdLabelPanel(
    HudUiZrdWidget *widget,
    zReader::Node *labelSpecBase,
    int originX,
    int originY
) {
    HudUiTransitionTextPanel *const transitionPanel =
        (HudUiTransitionTextPanel *)(::operator new(sizeof(HudUiTransitionTextPanel)));
    new (transitionPanel) HudUiTransitionTextPanel;

    HudUiPanel *const panel = (HudUiPanel *)(transitionPanel);
    HudUiElement *const element = (HudUiElement *)(transitionPanel);
    element->flags = (element->flags & 0x10u) | 0x02u;

    const char *const key = ZrdArrayString(
        labelSpecBase,
        1
    );
    const char *const text = key != 0 ? zLoc::ResolveMessageKeyOrFallback(key) : "";
    panel->SetTextFmt(text != 0 ? text : "");

    element->SetPos(
        originX + ZrdArrayInt(
            labelSpecBase,
            2,
            0
        ),
        originY + ZrdArrayInt(labelSpecBase, 3, 0)
    );

    const int styleIndex = ZrdArrayInt(
        labelSpecBase,
        4,
        0
    );
    ApplyHudFontStyleToPanel(
        panel,
        HudUiZrdOwnerFontStyle(widget->owner, styleIndex)
    );

    element->SetVisible(1);
    ((HudUiContainer *)(widget->owner))->AddChild(element);
    return panel;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x413d30 HudUiLayoutNode::ApplyImageWidget callers.
 * Purpose: preserve the recovered HUD behavior for AppendHudZrdLabelPanel.
 */
void AppendHudZrdLabelPanel(
    HudUiZrdWidget *widget,
    HudUiPanelPtrVector &panels,
    zReader::Node *labelSpecBase,
    int originX,
    int originY
) {
    HudUiPanel *panel = CreateHudZrdLabelPanel(
        widget,
        labelSpecBase,
        originX,
        originY
    );
    panels.InsertN(
        panels.end,
        1,
        &panel
    );
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x413d30 HudUiLayoutNode::ApplyImageWidget callers.
 * Purpose: preserve the recovered HUD behavior for CreateHudZrdTextPanel.
 */
HudUiPanel *CreateHudZrdTextPanel(
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
 * Purpose: load the recovered HUD data handled by LoadHudZrdLabelSection.
 */
void LoadHudZrdLabelSection(
    HudUiZrdWidget *widget,
    zReader::Node *parentNode,
    HudUiPanelPtrVector &panels
) {
    zReader::Node *const labelNode = zReader_GetNamedNode(
        parentNode,
        g_HudZrd_Key_Label
    );
    zReader::Node *const labelBase = ZrdArrayBase(labelNode);
    if (labelBase == 0) {
        return;
    }

    const int originX = widget->originX;
    const int originY = widget->originY;
    zReader::Node *const firstItem = ZrdArrayItem(
        labelBase,
        1
    );
    if (firstItem != 0 && firstItem->type == zReader::ZRDR_NODE_ARRAY) {
        const int count = ZrdArrayCount(labelBase);
        {
            for (int index = 1; index <= count - 1; ++index) {
                AppendHudZrdLabelPanel(
                    widget,
                    panels,
                    ZrdArrayBase(ZrdArrayItem(
                        labelBase,
                        index
                    )),
                    originX,
                    originY
                );
            }
        }
        return;
    }

    AppendHudZrdLabelPanel(
        widget,
        panels,
        labelBase,
        originX,
        originY
    );
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x41ebd0 HudUiMgrSensor::TrackList_Reset callers.
 * Purpose: apply the recovered HUD layout or option state handled by ApplyHudZrdFlashSection.
 */
void ApplyHudZrdFlashSection(
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

    for (HudUiPanel **it = panels.begin; it != panels.end; ++it) {
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
void LoadHudZrdBitmap(
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
void LoadHudZrdSound(
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

/**
 * Recovered original inline/static helper with no standalone retail function.
 * Observed in text-stack constructors 0x4bd020 and 0x4bd2d0 after each
 * HudUiPanel row is constructed.
 * Purpose: attach and initialize one message-stack row with the recovered
 * panel font, shadow, alignment, position, and hidden state.
 */
void ConfigureTextStackLine(
    HudUiTextStack4 *stack,
    HudUiPanel *panel,
    int y,
    int fontSize,
    int fontWeight,
    int fontWidth
) {
    HudUiElement *const element = (HudUiElement *)(panel);
    stack->AddChild(element);
    panel->SetFont(
        g_HudFontName_Arial,
        fontSize,
        fontWeight,
        fontWidth,
        0,
        0,
        2
    );
    panel->SetShadow(
        1,
        -1,
        -1
    );
    panel->alignMode = 1;
    element->SetPos(
        0x140,
        y
    );
    element->SetVisible(0);
}

} // namespace

namespace HudUiMgrSensor {
/**
 * Reimplements 0x41ebd0: HudUiMgrSensor::TrackList_Reset.
 * Original source path: D:\Proj\Battlesport\HudUiMgrSensor.cpp.
 * Purpose: clear the recovered sensor track-list global before target
 * tracking records are appended for the current HUD update pass.
 */
void __cdecl TrackList_Reset() {
    memset(
        &g_HudUiMgrSensor_TrackList,
        0,
        sizeof(g_HudUiMgrSensor_TrackList)
    );
}

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *HudUiCrtInitializerFn)();
/* VC5 emits this HUD UI startup callback as a direct .CRT$XCU row. */
#pragma data_seg(".CRT$XCU")
HudUiCrtInitializerFn s_HudUiCrtInit_HudUiMgrSensorTrackListReset =
    TrackList_Reset;
#pragma data_seg()
#endif

/**
 * Reimplements 0x438920: HudUiMgrSensor::TrackList_Add.
 * Original source path: D:\Proj\Battlesport\HudUiMgrSensor.cpp.
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

/**
 * Reimplements 0x439690: HudUiMgrSensor::UpdateMarkersAndProgressFromVariantTag.
 * Original source path: D:\Proj\Battlesport\HudUiMgrSensor.cpp.
 * BN/source evidence ties this to the sensor-target runtime owner: the track
 * list stores discriminated player/turret payloads, candidate filtering uses
 * variant tags and scene-path projection visibility, and marker creation feeds
 * the typed HudUiSlot placement/update path rather than raw HUD offsets.
 * Purpose: refresh candidate sensor targets for the requested variant tag,
 * place visible markers, and update the selected target progress slots.
 */
void __fastcall UpdateMarkersAndProgressFromVariantTag(
    const zTag4Partial *requiredVariantTag
) {
    HudUiMgrSensorTrackNode *trackNode = g_HudUiMgrSensor_TrackList.head;
    zUtil_PlayerStateStorage *const localPlayerState =
        (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);

    HudUiMgrSensorTrackNode *candidateTrackNodes[0x64];
    int candidateCount = 0;
    while (trackNode != 0) {
        if (trackNode->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
            zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(trackNode->payload);
            zUtil_PlayerStateStorage *const playerState = saveState->playerState;

            if (playerState->recentHitFlag != 0 &&
                !(g_Time_AccumulatedTimeSec < playerState->recentHitExpireTime)) {
                playerState->recentHitFlag = 0;
            }

            if (playerState->lifecycleState != 1 && playerState->lifecycleState != 4 &&
                VariantTag::TagsOverlap(
                    &playerState->variantTag,
                    requiredVariantTag
                ) != 0) {
                const float distXZ =
                    fabs(playerState->fxOffsetWorld.x - localPlayerState->worldPos.x) +
                    fabs(playerState->fxOffsetWorld.z - localPlayerState->worldPos.z);
                if (distXZ < 650.0f && candidateCount < 0x63) {
                    candidateTrackNodes[candidateCount++] = trackNode;
                }
            }
        } else {
            trackNode->trackKind = HUD_SENSOR_TRACK_KIND_TURRET;
            zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(trackNode->payload);
            if (turretRuntime->HasActiveNode() != 0 &&
                VariantTag::CurrentAllowsId(turretRuntime->turretNode->nodeType) != 0) {
                const float distXZ = fabs(turretRuntime->firePos.z - localPlayerState->worldPos.z) +
                                     fabs(turretRuntime->firePos.x - localPlayerState->worldPos.x);
                if (distXZ < 650.0f && candidateCount < 0x63) {
                    candidateTrackNodes[candidateCount++] = trackNode;
                }
            }
        }

        trackNode = trackNode->next;
    }

    if (candidateCount != 0) {
        int selectedIndex = g_HudUiMgrSensor_RoundRobinTrackIndex + 1;
        g_HudUiMgrSensor_RoundRobinTrackIndex = selectedIndex;
        if (selectedIndex >= candidateCount) {
            selectedIndex = 0;
            g_HudUiMgrSensor_RoundRobinTrackIndex = 0;
        }

        HudUiMgrSensorTrackNode *const selectedTrackNode = candidateTrackNodes[selectedIndex];
        if (selectedTrackNode->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
            zUtil_SaveGameState *const saveState =
                (zUtil_SaveGameState *)(selectedTrackNode->payload);
            zUtil_PlayerStateStorage *const playerState = saveState->playerState;
            zVec3 point = playerState->fxOffsetWorld;
            point.y += 3.0f;

            const int visible =
                AINet::HasLineOfSightFromCameraTarget(
                    playerState->rootNode,
                    &point,
                    1
                );
            playerState->spawnStateInitialized = visible;
            if (visible != 0 && playerState->recentHitMarkerHandle != 0) {
                playerState->recentHitFlag = 1;
                playerState->recentHitExpireTime = g_Time_AccumulatedTimeSec + 3.0f;
            }
        } else if (selectedTrackNode->trackKind == HUD_SENSOR_TRACK_KIND_TURRET) {
            zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(selectedTrackNode->payload);
            turretRuntime->scenePathVisible = AINet::HasLineOfSightFromCameraTarget(
                turretRuntime->turretNode,
                &turretRuntime->firePos,
                2
            );
        }

        {
            for (int index = 0; index < candidateCount; ++index) {
                HudUiMgrSensorTrackNode *const candidate = candidateTrackNodes[index];
                if (candidate->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
                    zUtil_SaveGameState *const saveState =
                        (zUtil_SaveGameState *)(candidate->payload);
                    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
                    if ((playerState->spawnStateInitialized & 1) != 0) {
                        playerState->recentHitMarkerHandle =
                            HudUiMgrSensor::PlaceTrackCounterWidget(
                                candidate,
                                &playerState->fxOffsetWorld
                            );
                    }
                } else if (candidate->trackKind == HUD_SENSOR_TRACK_KIND_TURRET) {
                    zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(candidate->payload);
                    if ((turretRuntime->scenePathVisible & 1) != 0) {
                        HudUiMgrSensor::PlaceTrackCounterWidget(
                            candidate,
                            &turretRuntime->firePos
                        );
                    }
                }
            }
        }
    }

    PlayerGunFireController *const activeAltGunController =
        localPlayerState->activeAltGunController;
    const unsigned int optEntryFlags = activeAltGunController->optCatalogEntry->flags;
    if (((optEntryFlags >> 20) & 1u) != 0) {
        HudUiMgr::CopyReticleProjection(&localPlayerState->autoTurnTargetWorldPos.x);
        localPlayerState->progressTargetCount = 1;
        localPlayerState->progressTargetSlots[0].targetPos =
            &localPlayerState->autoTurnTargetWorldPos;
        localPlayerState->progressTargetSlots[0].targetVelocity = 0;
        HudUiMgrTarget::UpdateSelectedProgressMeter(0);
        return;
    }

    if (activeAltGunController->ammoOrCharge != 0.0f) {
        int markerMode = 0;
        if (((optEntryFlags >> 16) & 1u) != 0) {
            markerMode = 2;
        } else if ((optEntryFlags & 0x4000u) != 0) {
            markerMode = 1;
        }

        localPlayerState->progressTargetCount =
            HudUiMgrSensor::PlaceTrackMarker(
                markerMode,
                localPlayerState->progressTargetSlots
            );
    }

    HudUiMgrTarget::UpdateSelectedProgressMeter(0);
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
 * Reimplements 0x4b3d50: HudUiWidget::DestructorCore.
 * Purpose: run the recovered HudUiWidget::DestructorCore teardown path.
 */
void HudUiWidget::DestructorCore() {
    this->~HudUiWidget();
}


namespace HudUiMgr {

} // namespace HudUiMgr

/**
 * Reimplements 0x4b4070: HudUiElement::Constructor.
 * Original source path: D:\Proj\Battlesport\hudui_element.cpp.
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
 * Reimplements 0x4b4070: HudUiElement::Constructor.
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
 * Reimplements 0x4b40c0: HudUiElement::CopyConstructor.
 * Purpose: initialize a HUD element from another element while clearing owner links.
 */
HudUiElement * HudUiElement::CopyConstructor(
    const HudUiElement *source
) {
    next = 0;
    parent = 0;
    flags = source->flags;
    state = source->state;
    timer = source->timer;
    x = source->x;
    y = source->y;
    bltSource = source->bltSource;
    clipRect = source->clipRect;
    return this;
}

/**
 * Reimplements 0x4b4120: HudUiElement::CopyFrom.
 * Purpose: copy another HUD element's runtime fields while preserving dispatch identity.
 */
HudUiElement * HudUiElement::CopyFrom(
    const HudUiElement *source
) {
    next = 0;
    parent = 0;
    flags = source->flags;
    state = source->state;
    timer = source->timer;
    x = source->x;
    y = source->y;
    bltSource = source->bltSource;
    clipRect = source->clipRect;
    return this;
}

/**
 * Reimplements 0x4b47a0: HudUiElement::~HudUiElement.
 * Purpose: reset the HudUiElement virtual table during class destruction.
 *
 * Evidence: the definition is kept inline in zhud_ui.h so VC5 can inline the
 * base table reset into derived destructors while still emitting the standalone
 * element destructor COMDAT when the address-backed symbol is required.
 */

/**
 * Reimplements 0x4b4180: HudUiElement::Invalidate.
 * Purpose: mark the element dirty by OR-ing the current HUD invalidation mask into its flags.
 */
void HudUiElement::Invalidate() {
    flags |= g_HudUi_InvalidateMask;
}

/**
 * Reimplements 0x4b4190: HudUiElement::SetBltSourceAndClipRect.
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
 * Reimplements 0x4b41b0: HudUiElement::SetClipRect.
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
 * Reimplements 0x4b41e0: HudUiElement::Update.
 * Original file: D:\Proj\Battlesport\HudUiElement.cpp.
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
 * Reimplements 0x4b4280: HudUiElement::SetTimer.
 * Original file: D:\Proj\Battlesport\HudUiElement.cpp.
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
 * Reimplements 0x4bcd40: HudUiPanel::SetClip.
 * Purpose: apply the recovered HUD state change handled by HudUiPanel::SetClip.
 */
void HudUiPanel::SetClip(
    void *bltSourceOrNull,
    const HudUiRect *rectOrNull
) {
    bltSource = bltSourceOrNull;
    if (rectOrNull != 0) {
        clipRect = *rectOrNull;
    }

    Invalidate();
}

/**
 * Reimplements 0x4b42c0: HudUiElement::GetTextRect.
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

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed
 * HudUiElement::GetCenterY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnHoverRepeat.
 */
void HudUiElement::OnHoverRepeat() {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: return the recovered HUD value exposed by HudUiElement::GetBoundsRectOrNull.
 */
HudUiRect * HudUiElement::GetBoundsRectOrNull() {
    return 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnActivate.
 */
void HudUiElement::OnActivate() {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnClearBinding.
 */
void HudUiElement::OnClearBinding() {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::ShowPreview.
 */
void HudUiElement::ShowPreview() {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::HidePreview.
 */
void HudUiElement::HidePreview() {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnBeginCapture.
 */
void HudUiElement::OnBeginCapture() {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnEndCapture.
 */
void HudUiElement::OnEndCapture() {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnPointerButtonState.
 */
void HudUiElement::OnPointerButtonState(
    int,
    int
) {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnCapturedPrimaryRelease.
 */
void HudUiElement::OnCapturedPrimaryRelease() {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::ShouldHandleInput.
 */
int HudUiElement::ShouldHandleInput(
    HudUiBackground *,
    int
) {
    return 1;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::AfterInputUpdate.
 */
void HudUiElement::AfterInputUpdate(
    HudUiBackground *,
    int
) {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::HitTest.
 */
int HudUiElement::HitTest(
    int px,
    int py
) {
    return HitTestTrue(
        px,
        py
    );
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::EnableWordWrapWithRect.
 */
void HudUiElement::EnableWordWrapWithRect(
    const HudUiRect *
) {}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * No standalone retail function has been identified; restored as the default
 * HudUiZrdWidget post-load virtual observed as the ZRD widget tail slot before
 * numeric input adds raw-key virtuals.
 * Purpose: keep ZRD loading ownership on HudUiZrdWidget.
 */
void HudUiZrdWidget::PostLoadFromZrd() {}

/**
 * Reimplements 0x4bffb0: HudUiPrimitiveBindTarget::SetSegmentEndpoints.
 * Purpose: apply the recovered HUD state change handled by HudUiPrimitiveBindTarget::SetSegmentEndpoints.
 */
void HudUiPrimitiveBindTarget::SetSegmentEndpoints(
    int startX,
    int startY,
    int newEndX,
    int newEndY
) {
    SetPos(
        startX,
        startY
    );
    endX = newEndX;
    endY = newEndY;
}

/**
 * Reimplements 0x4bc480: HudUiCircle::HudUiCircle.
 * Original file: D:\Proj\Battlesport\hud.cpp.
 * Purpose: initialize a circle element's position, radius, and color.
 *
 * Evidence: BN assembly calls the HudUiElement base constructor at object
 * offset zero, installs the derived circle C++ dispatch identity, stores
 * radius at 0x34, stores
 * radiusSquared as radius * radius at 0x38, stores color565 at 0x3c, and
 * returns this.
 */
HudUiCircle::HudUiCircle(
    int x,
    int y,
    int circleRadius,
    unsigned int circleColor565
)
    : HudUiElement(
        x,
        y
    ) {
    radius = circleRadius;
    const unsigned int radiusBits = (unsigned int)(circleRadius);
    radiusSquared = (int)(radiusBits * radiusBits);
    color565 = circleColor565;
}

/**
 * Reimplements 0x4bc4c0: HudUiCircle::Draw.
 * Purpose: redraw the inherited base and circle outline for a dirty circle element.
 */
void HudUiCircle::Draw() {
    DrawBase();
    zRndr_DrawCircleOutline16_Framebuffer(
        x,
        y,
        radius,
        color565,
        0
    );
}

/**
 * Reimplements 0x4bc4e0: HudUiCircle::HitTestCore.
 * Purpose: compare a point's squared distance against the circle radius.
 */
unsigned char HudUiCircle::HitTestCore(
    int px,
    int py
) {
    const unsigned int dx = (unsigned int)(px) - (unsigned int)(x);
    const unsigned int dy = (unsigned int)(py) - (unsigned int)(y);
    const unsigned int distanceSquared = dx * dx + dy * dy;
    return (int)(distanceSquared) < radiusSquared ? 1 : 0;
}

/**
 * Reimplements 0x4bb790: HudUiCompositePanel::HudUiCompositePanel.
 * Purpose: initialize a composite panel and allocate its entry history vector.
 *
 * Evidence: BN retail 0x4bb790 constructs the HudUiPanel base, initializes the
 * vector member, installs g_HudUiCompositePanel_FTable, builds a stack
 * HudUiTransitionTextPanel template entry, resizes the entry vector, applies
 * text "W", relayouts, and sets the panel visible. BN caller 0x403930 invokes
 * this entry-count construction for HudUiBriefingRuntime::messagesPanel before
 * constructing locatorPanels, matching a C++ member-initializer constructor.
 * Reimplements 0x4bb790 as the recovered constructor symbol
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
 * Reimplements 0x4bb980: HudUiCompositePanel::Update.
 * Purpose: tick flash state for each visible composite-panel entry.
 */
void HudUiCompositePanel::Update(
    float deltaSeconds
) {
    if ((flags & 0x10u) != 0) {
        return;
    }

    for (unsigned int index = 0; index < entryVector.size(); ++index) {
        HudUiCompositePanelEntry *const entry = &entryVector[index];
        entry->panel.Update(deltaSeconds);
    }
}

/**
 * Reimplements 0x4bb9f0: HudUiCompositePanel::SetPos.
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
        entry->panel.SetPos(
            GetCenterX(),
            GetCenterY() + yOffset
        );
        yOffset += entryHeight;
    }
}

/**
 * Reimplements 0x4bbe90: HudUiCompositePanel::ReapplyEntryCount.
 * Purpose: reapply the current composite-entry count after vector changes.
 */
void HudUiCompositePanel::ReapplyEntryCount() {
    ResizeEntryCount(
        0,
        (int)(entryVector.size())
    );
}

/**
 * Reimplements 0x4bbed0: HudUiCompositePanel::ResizeEntryCount.
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
            HudUiCompositePanelEntry *const entry = &entryVector[index];
            entry->panel.SetTextFmt("");
            entry->panel.SetVisible(0);
        }
    }

    activeEntryCount = oldCount;
}

/**
 * Reimplements 0x4bbaa0: HudUiCompositePanel::SetTextFmt.
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
 * Reimplements 0x4bbac0: HudUiCompositePanel::SetTextFmtV.
 * Purpose: write formatted text into the active composite entry and scroll
 * history as needed.
 */
void HudUiCompositePanel::SetTextFmtV(
    const char *format,
    va_list args
) {
    HudUiCompositePanelEntry *const entry = &entryVector[activeEntryCount];
    entry->panel.SetTextFmtV(
        format,
        args
    );
    entry->panel.SetVisible(1);
    ScrollHistory();
}

/**
 * Reimplements 0x4bbb20: HudUiCompositePanel::ScrollHistory.
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
                HudUiCompositePanelEntry *const current = &entryVector[index];
                HudUiCompositePanelEntry *const next = &entryVector[index + 1];
                current->panel.SetText(next->panel.GetLastTextPtr());
            }
        }
        --activeEntryCount;
    }

    Invalidate();
}

/**
 * Reimplements 0x4bbbe0: HudUiCompositePanel::SetFont.
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
        HudUiCompositePanelEntry *const entry = &entryVector[index];
        entry->panel.SetFont(
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
 * Reimplements 0x4bbca0: HudUiCompositePanel::ResizeEntryVectorAndRelayout.
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
 * Reimplements 0x4bc3a0: HudUiCompositePanelEntry::AssignCopy.
 * Purpose: copy one composite-panel entry into existing entry storage.
 */
HudUiCompositePanelEntry * HudUiCompositePanelEntry::AssignCopy(
    const HudUiCompositePanelEntry *source
) {
    panel.ConstructorCopy(&source->panel);
    panel.flashCountdown = source->panel.flashCountdown;
    panel.flashResetValue = source->panel.flashResetValue;
    panel.flashAltColor0 = source->panel.flashAltColor0;
    panel.flashAltColor1 = source->panel.flashAltColor1;
    panel.flashEnabled = source->panel.flashEnabled;
    panel.flashMode = source->panel.flashMode;
    panel.flashDirectionSign = source->panel.flashDirectionSign;
    return this;
}

/**
 * Reimplements 0x4bc410: HudUiCompositePanelEntry::ConstructorCopy.
 * Purpose: copy-construct one composite-panel entry from another entry.
 */
HudUiCompositePanelEntry * HudUiCompositePanelEntry::ConstructorCopy(
    const HudUiCompositePanelEntry *source
) {
#if !defined(_MSC_VER) || _MSC_VER >= 1200
    new (&panel) HudUiTransitionTextPanel;
#endif
    panel.CopyConstructCore(&source->panel);
    panel.flashCountdown = source->panel.flashCountdown;
    panel.flashResetValue = source->panel.flashResetValue;
    panel.flashAltColor0 = source->panel.flashAltColor0;
    panel.flashAltColor1 = source->panel.flashAltColor1;
    panel.flashEnabled = source->panel.flashEnabled;
    panel.flashMode = source->panel.flashMode;
    panel.flashDirectionSign = source->panel.flashDirectionSign;
    return this;
}

/**
 * Reimplements 0x4bc320: HudUiCompositePanelEntry::ConstructorCopyRange.
 * Purpose: copy-construct a range of composite-panel entries into destination
 * storage.
 */
HudUiCompositePanelEntry *__fastcall HudUiCompositePanelEntry::ConstructorCopyRange(
    const HudUiCompositePanelEntry *sourceBegin,
    const HudUiCompositePanelEntry *sourceEnd,
    HudUiCompositePanelEntry *destBegin
) {
    HudUiCompositePanelEntry *dest = destBegin;
    for (const HudUiCompositePanelEntry *source = sourceBegin; source != sourceEnd;
        ++source, ++dest) {
        dest->panel.ConstructorCopy(&source->panel);
        dest->panel.flashCountdown = source->panel.flashCountdown;
        dest->panel.flashResetValue = source->panel.flashResetValue;
        dest->panel.flashAltColor0 = source->panel.flashAltColor0;
        dest->panel.flashAltColor1 = source->panel.flashAltColor1;
        dest->panel.flashEnabled = source->panel.flashEnabled;
        dest->panel.flashMode = source->panel.flashMode;
        dest->panel.flashDirectionSign = source->panel.flashDirectionSign;
    }

    return dest;
}

/**
 * Reimplements 0x4bb0c0: HudUiFlashPanel::ComputeFlashBlendColor.
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
 * Reimplements 0x4bc780: HudUiContainer::HudUiContainer.
 * Purpose: preserve the recovered HUD behavior for HudUiContainer::HudUiContainer.
 */
HudUiContainer::HudUiContainer() {
    HudUiContainer *const container = this;
    container->SetEnabled(0);
    childHead = 0;
    childTail = 0;
}

/**
 * Reimplements 0x4bc7b0: HudUiContainer::~HudUiContainer.
 * Current BN assembly restores the base HudUiContainer vptr and returns.
 * Purpose: tear down the common container base after derived HUD UI cleanup.
 */
HudUiContainer::~HudUiContainer() {
}

/**
 * Reimplements 0x4bc7b0: HudUiContainer::DestructorCore.
 * Purpose: route legacy native smoke call sites through the recovered C++
 * destructor so base vptr restoration remains compiler-owned.
 */
void HudUiContainer::DestructorCore() {
    this->HudUiContainer::~HudUiContainer();
}

/**
 * Reimplements 0x4bc7c0: HudUiContainer::AddChild.
 * Purpose: preserve the recovered HUD behavior for HudUiContainer::AddChild.
 */
int HudUiContainer::AddChild(
    HudUiElement *child
) {
    if (childHead != 0 && childTail != 0) {
        childTail->next = child;
        childTail = child;
    } else {
        childTail = child;
        childHead = child;
    }

    child->next = 0;
    child->parent = this;
    return 1;
}

/**
 * Reimplements 0x4bc810: HudUiContainer::FindChildWithPrev.
 * Original source path: D:\Proj\Battlesport\HudUi.cpp.
 * Purpose: find a child in the container list and optionally report the
 * previous sibling.
 */
int HudUiContainer::FindChildWithPrev(
    HudUiElement *child,
    HudUiElement **previousOut
) {
    HudUiElement *previous = childHead;
    if (previous == 0) {
        return 0;
    }

    if (child == previous) {
        *previousOut = 0;
        return 1;
    }

    while (previous != 0) {
        HudUiElement *const current = previous->next;
        if (current == child) {
            if (previousOut != 0) {
                *previousOut = previous;
            }

            return 1;
        }

        previous = current;
    }

    return 0;
}

/**
 * Reimplements 0x4bc860: HudUiContainer::RemoveChild.
 * Original source path: D:\Proj\Battlesport\HudUi.cpp.
 * Purpose: unlink a child from this container and clear the child's owner
 * links.
 */
int HudUiContainer::RemoveChild(
    HudUiElement *child
) {
    HudUiElement *previous = child;
    if (FindChildWithPrev(
        child,
        &previous
    ) == 0) {
        return 0;
    }

    if (previous != 0) {
        previous->next = child->next;
        if (child == childTail) {
            childTail = previous;
        }
    } else {
        childHead = child->next;
        if (child == childTail) {
            childTail = child->next;
        }
    }

    child->next = 0;
    child->parent = 0;
    return 1;
}

/**
 * Reimplements 0x4bc8d0: HudUiContainer::SetChildFlags.
 *
 * Purpose: apply a shared child flag mask to every child while preserving each
 * child's hidden/disabled bit 0x10.
 *
 * Evidence: BN assembly at 0x4bc8d0 walks HudUiContainer::childHead through
 * HudUiElement::next, writes childFlags directly when bit 0x10 is clear, and
 * writes childFlags|0x10 when the existing child flags preserve that bit.
 */
void HudUiContainer::SetChildFlags(
    unsigned int childFlags
) {
    for (HudUiElement *child = childHead; child != 0; child = child->next) {
        const unsigned int invertedFlags = ~child->flags;
        if ((invertedFlags & 0x10u) != 0) {
            child->flags = childFlags;
        } else {
            child->flags = childFlags | 0x10u;
        }
    }
}

/**
 * Reimplements 0x4bc900: HudUiContainer::UpdateAll.
 * Purpose: Dispatch per-frame updates to every child in an enabled container.
 */
void HudUiContainer::UpdateAll(
    float deltaSeconds
) {
    if (enabled == 0) {
        return;
    }

    for (HudUiElement *child = childHead; child != 0; child = child->next) {
        child->Update(deltaSeconds);
    }
}

/**
 * Reimplements 0x4ba3a0: HudUiContainer::InvalidateChildren.
 * Purpose: preserve the recovered HUD behavior for HudUiContainer::InvalidateChildren.
 */
void HudUiContainer::InvalidateChildren() {
    for (HudUiElement *child = childHead; child != 0; child = child->next) {
        child->Invalidate();
    }
}

/**
 * Reimplements 0x42ee40: HudUiBackgroundContainer::SetEnabled.
 * Purpose: apply the recovered HUD state change handled by HudUiBackgroundContainer::SetEnabled.
 */
void HudUiBackgroundContainer::SetEnabled(
    int enabled
) {
    HudUiContainer::SetEnabled(enabled);
}

/**
 * Reimplements 0x409570: HudUiZrdScrollingText::LoadFromZrd.
 * Purpose: load the recovered HUD data handled by HudUiZrdScrollingText::LoadFromZrd.
 */
void HudUiZrdScrollingText::OnActivate() {
    OnActivateResetOwnerFade();
}

/**
 * Reimplements 0x409ef0: HudUiPanel::DestructorCallback
 * Source: D:\Proj\Battlesport\HudUiCreditsPanel.cpp
 * Purpose: adapt HudUiPanel array cleanup callbacks to the recovered panel destructor thunk.
 */
void __stdcall HudUiPanel::DestructorCallback(
    HudUiPanel *panel
) {
    panel->DestructorThunk();
}

/**
 * Reimplements 0x4bc510: HudUiBackgroundContainer::HudUiBackgroundContainer.
 * Purpose: preserve the recovered HUD behavior for HudUiBackgroundContainer::HudUiBackgroundContainer.
 */
HudUiBackgroundContainer::HudUiBackgroundContainer(
    int initFlag
) : HudUiContainer() {
    captureTransitionMask = initFlag;
    inputFocusElement = 0;
}

/**
 * Reimplements 0x4bc540: HudUiBackgroundContainer::~HudUiBackgroundContainer.
 * Purpose: Restores the background-container base state and tears down the inherited container.
 */
HudUiBackgroundContainer::~HudUiBackgroundContainer() {
    HudUiContainer::DestructorCore();
}

/**
 * Reimplements 0x4bc550: HudUiBackgroundContainer::SetInputFocus.
 * Purpose: Stores the child element that currently owns background input focus.
 */
void HudUiBackgroundContainer::SetInputFocus(
    HudUiElement *element
) {
    inputFocusElement = element;
}

/**
 * Reimplements 0x4bc560: HudUiBackgroundContainer::GetInputFocus.
 * Purpose: Returns the child element that currently owns background input focus.
 */
HudUiElement * HudUiBackgroundContainer::GetInputFocus() {
    return inputFocusElement;
}

/**
 * Reimplements 0x4b9540: HudUiBackground::HudUiBackground.
 * Original source path: D:\Proj\Battlesport\hudui_background.cpp.
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
 * Reimplements 0x4b9760: HudUiBackground::~HudUiBackground.
 * Original source path: D:\Proj\Battlesport\hudui_background.cpp.
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
 * Reimplements 0x4ba380: HudUiDialogController::BlitOwnedSurfaceToPrimary.
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
 * Reimplements 0x4b9850: HudUiBackground::SetEnabled.
 * Original source path: D:\Proj\GameZRecoil\zHud\HudUiBackground.cpp.
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
 * Reimplements 0x4b98d0: HudUiBackground::LoadFromZrd.
 * Original source path: D:\Proj\Battlesport\hudui_background.cpp.
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
 * Reimplements 0x4b9900: HudUiBackground::LoadZrdAndSection.
 * Original source path: D:\Proj\Battlesport\hudui_background.cpp.
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

/**
 * Reimplements 0x4ba020: HudUiTransitionTextPanel::HudUiTransitionTextPanel.
 * Original source path: D:\Proj\Battlesport\hudui_background.cpp.
 * Purpose: construct the transition text panel and initialize its flash state.
 *
 * Evidence: BN assembly calls HudUiPanel::ConstructorDefault, clears flash
 * fields, writes flashResetValue = 0.35f and flashDirectionSign = 1, and
 * installs the transition text panel table at 0x4cd388. Retail callers such as
 * 0x4bb790 inline this construction into stack template entries.
 */
inline HudUiTransitionTextPanel::HudUiTransitionTextPanel()
    : HudUiPanel(
        0,
        0,
        0
    ) {
    flashResetValue = 0.349999994f;
    flashCountdown = 0;
    flashAltColor0 = 0;
    flashEnabled = 0;
    flashMode = 0;
    flashDirectionSign = 1;
}

/**
 * Reimplements 0x4ba070: HudUiBackground::BindButtonsNodeToWidgetByName.
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
 * Reimplements 0x4ba0c0: HudUiBackground::BindWidgetByName.
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
 * Reimplements 0x4ba0e0: HudUiBackground::BindPrimitiveNodeToElement.
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
 * Reimplements 0x4ba350: HudUiBackground::FreeLoadedTreeRoots.
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
 * Reimplements 0x4bc570: HudUiBackgroundContainer::UpdateAll.
 * Purpose: Dispatch background mouse input, update child widgets, and move the focus cursor.
 */
void HudUiBackgroundContainer::UpdateAll(
    float deltaSeconds
) {
    if (enabled == 0) {
        return;
    }

    HudUiBackground *const background = (HudUiBackground *)this;

    memcpy(
        &mouseState,
        zInput::Mouse_GetStateSnapshotPtr(),
        sizeof(mouseState)
    );

    for (HudUiElement *widget = childHead; widget != 0; widget = widget->next) {
        const int hit = widget->HitTest(
            mouseState.cursorClientX,
            mouseState.cursorClientY
        );
        const int hovered = hit == 1 ? 1 : 0;

        if (widget->ShouldHandleInput(
            background,
            hovered
        ) != 0) {
            if ((mouseState.button2Transition & 4) != 0 && (widget->state & 2) == 2) {
                widget->state = (unsigned short)(widget->state & 0xfffd);
                widget->OnEndCapture();
            }

            if (hovered != 0) {
                if ((widget->state & 1) == 0) {
                    widget->state = (unsigned short)(widget->state | 1);
                    widget->ShowPreview();
                } else {
                    widget->OnHoverRepeat();
                }

                if ((mouseState.button1Transition & captureTransitionMask) != 0 &&
                    (widget->state & 2) == 0) {
                    widget->state = (unsigned short)(widget->state | 2);
                    widget->OnBeginCapture();
                }

                if ((mouseState.button1Transition & 4) != 0) {
                    widget->OnActivate();
                }

                if ((mouseState.button2Transition & 4) != 0) {
                    widget->OnClearBinding();
                }

                if ((mouseState.button1Transition & 3) != 0) {
                    widget->OnPointerButtonState(
                        mouseState.cursorClientX,
                        mouseState.cursorClientY
                    );
                }

                if ((mouseState.button1Transition & 4) != 0 && (widget->state & 2) == 2) {
                    widget->OnCapturedPrimaryRelease();
                }
            } else {
                if ((mouseState.button1Transition & captureTransitionMask) != 0 &&
                    (widget->state & 2) == 2) {
                    widget->state = (unsigned short)(widget->state & 0xfffd);
                    widget->OnEndCapture();
                }

                if ((mouseState.button1Transition & 3) != 0 && (widget->state & 2) == 2) {
                    widget->OnPointerButtonState(
                        mouseState.cursorClientX,
                        mouseState.cursorClientY
                    );
                }

                if ((widget->state & 1) == 1) {
                    widget->state = (unsigned short)(widget->state & 0xfffe);
                    widget->HidePreview();
                }
            }
        }

        widget->AfterInputUpdate(
            background,
            hovered
        );
    }

    HudUiElement *const focusBeforeUpdate = inputFocusElement;
    if (focusBeforeUpdate != 0) {
        focusBeforeUpdate->DrawBase();
    }

    HudUiContainer::UpdateAll(deltaSeconds);

    HudUiElement *const focusAfterUpdate = inputFocusElement;
    if (focusAfterUpdate != 0) {
        focusAfterUpdate->SetPos(
            mouseState.cursorClientX,
            mouseState.cursorClientY
        );
        focusAfterUpdate->Update(deltaSeconds);
    }
}

/**
 * Reimplements 0x4ba4a0: HudFontStyle::HudFontStyle.
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
 * Reimplements 0x4ba4c0: HudFontStyle::Destructor.
 * Source model note: Source-faithful helper recovered from address-backed callers in this
 * source file.
 * Purpose: run the recovered HudFontStyle::Destructor teardown path.
 */
HudFontStyle::~HudFontStyle() {
    Destructor();
}

/**
 * Reimplements 0x4ba4c0: HudFontStyle::Destructor.
 * Purpose: run the recovered HudFontStyle::Destructor teardown path.
 */
void HudFontStyle::Destructor() {
    validMarker = 0;
}

/**
 * Reimplements 0x4b3d00: HudUiWidget::Constructor.
 * Purpose: initialize the recovered HudUiWidget::Constructor state.
 */
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

/**
 * Reimplements 0x4b3d00: HudUiWidget::Constructor.
 * Purpose: initialize the recovered HudUiWidget::Constructor state.
 */
HudUiWidget * HudUiWidget::Constructor(
    unsigned int initAlignFlags
) {
    new (this) HudUiWidget(initAlignFlags);
    return this;
}

/**
 * Reimplements 0x4b3e90: HudUiWidget::InvalidateRect
 * Original source path: D:\Proj\Battlesport\hudui.cpp.
 * Purpose: queue and clip one widget dirty rectangle before invalidating the widget.
 */
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

/**
 * Reimplements 0x4bf980: HudUiBackgroundCursorWidget::HudUiBackgroundCursorWidget.
 * Original source path: D:\Proj\Battlesport\hudui_background.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiBackgroundCursorWidget::HudUiBackgroundCursorWidget.
 */
HudUiBackgroundCursorWidget::HudUiBackgroundCursorWidget(
    const char *imagePath,
    int initCaptureEnabled
) : HudUiWidget(0) {
    captureEnabled = initCaptureEnabled;
    capturedImage = 0;
    if (imagePath != 0) {
        SetImageByPathOwnedAndRefresh(imagePath);
    }

    reservedC8 = 0;
    reservedCC = 0;
    captureSourceSelector = 1;
}

/**
 * Reimplements 0x4bfa20: HudUiBackgroundCursorWidget::~HudUiBackgroundCursorWidget.
 * Original source path: D:\Proj\Battlesport\hudui_background.cpp.
 * Purpose: restore the cursor widget dispatch state, release a captured image, and tear down the widget base.
 */
HudUiBackgroundCursorWidget::~HudUiBackgroundCursorWidget() {
    if (capturedImage != 0) {
        zVid_Image::Destroy(capturedImage);
    }
}

/**
 * Reimplements 0x4bfa50: HudUiBackgroundCursorWidget::SetImageByPathOwnedAndRefresh.
 * Original source path: D:\Proj\Battlesport\hudui_background.cpp.
 * Purpose: apply the recovered HUD state change handled by HudUiBackgroundCursorWidget::SetImageByPathOwnedAndRefresh.
 */
void HudUiBackgroundCursorWidget::SetImageByPathOwnedAndRefresh(
    const char *imagePath
) {
    if (HudUiWidget::SetImageByPathOwned(imagePath) != 0) {
        SetImageBorrowedAndRefresh();
    }
}

/**
 * Reimplements 0x4bfa70: HudUiBackgroundCursorWidget::SetImageBorrowedAndRefreshIfChanged.
 * Original source path: D:\Proj\Battlesport\hudui_background.cpp.
 * Purpose: apply the recovered HUD state change handled by HudUiBackgroundCursorWidget::SetImageBorrowedAndRefreshIfChanged.
 */
void HudUiBackgroundCursorWidget::SetImageBorrowedAndRefreshIfChanged(
    zVidImagePartial *image
) {
    if (HudUiWidget::SetImageBorrowedAndInvalidate(image) != 0) {
        SetImageBorrowedAndRefresh();
    }
}

/**
 * Reimplements 0x4bfa90: HudUiBackgroundCursorWidget::SetImageOwnedAndRefresh.
 * Original source path: D:\Proj\Battlesport\hudui_background.cpp.
 * Purpose: apply the recovered HUD state change handled by HudUiBackgroundCursorWidget::SetImageOwnedAndRefresh.
 */
void HudUiBackgroundCursorWidget::SetImageOwnedAndRefresh(
    int newCaptureEnabled
) {
    captureEnabled = newCaptureEnabled;
    if (newCaptureEnabled == 0 && capturedImage != 0) {
        zVid_Image::Destroy(capturedImage);
        capturedImage = 0;
        HudUiElement::SetBltSourceAndClipRect(
            0,
            0
        );
        return;
    }

    if (capturedImage == 0) {
        SetImageBorrowedAndRefresh();
    }
}

/**
 * Reimplements 0x4bfae0: HudUiBackgroundCursorWidget::SetImageBorrowedAndRefresh.
 * Original source path: D:\Proj\Battlesport\hudui_background.cpp.
 * Purpose: apply the recovered HUD state change handled by HudUiBackgroundCursorWidget::SetImageBorrowedAndRefresh.
 */
void HudUiBackgroundCursorWidget::SetImageBorrowedAndRefresh() {
    if (captureEnabled == 0 || image == 0) {
        return;
    }

    if (capturedImage != 0) {
        zVid_Image::Destroy(capturedImage);
    }

    capturedImage = zVid_Image::Create();
    if (capturedImage == 0) {
        return;
    }

    zVid_Image::SetSize(
        capturedImage,
        image->width,
        image->height
    );
    void *const pixels = malloc((size_t)(capturedImage->pixelCount) * sizeof(unsigned short));
    zVid_Image_SetPixels(
        capturedImage,
        pixels,
        0
    );
    capturedImage->formatFlagsPacked = (unsigned char)(capturedImage->formatFlagsPacked | 0x20u);

    const int y = HudUiElement::GetCenterY();
    const int x = HudUiElement::GetCenterX();
    RebuildCapturedImage(
        x,
        y
    );
}

/**
 * Reimplements 0x4bfb70: HudUiBackgroundCursorWidget::SetPos.
 * Original source path: D:\Proj\Battlesport\hudui_background.cpp.
 * Purpose: apply the recovered HUD state change handled by HudUiBackgroundCursorWidget::SetPos.
 */
void HudUiBackgroundCursorWidget::SetPos(
    int newX,
    int newY
) {
    HudUiWidget::SetPos(
        newX,
        newY
    );
    RebuildCapturedImage(
        x,
        y
    );
}

/**
 * Reimplements 0x4bfba0: HudUiBackgroundCursorWidget::RebuildCapturedImage.
 * Original source path: D:\Proj\Battlesport\hudui_background.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiBackgroundCursorWidget::RebuildCapturedImage.
 */
void HudUiBackgroundCursorWidget::RebuildCapturedImage(
    int originX,
    int originY
) {
    if (capturedImage == 0) {
        return;
    }

    zVidRect32 sourceRect;
    sourceRect.left = originX;
    sourceRect.top = originY;
    sourceRect.right = originX + image->width;
    sourceRect.bottom = originY + image->height;

    if (zVideo_buff::CopySurfaceRectToImage(captureSourceSelector, &sourceRect, capturedImage) !=
        0) {
        const HudUiRect clipRect = {sourceRect.left - originX,
            sourceRect.top - originY,
            sourceRect.right - originX,
            sourceRect.bottom - originY};
        SetBltSourceAndClipRect(
            capturedImage,
            &clipRect
        );
        return;
    }

    SetBltSourceAndClipRect(
        0,
        0
    );
}

/**
 * Reimplements 0x4bfc50: HudUiBackgroundCursorWidget::Draw.
 * Original source path: D:\Proj\Battlesport\hudui_background.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiBackgroundCursorWidget::Draw.
 */
void HudUiBackgroundCursorWidget::Draw() {
    HudUiWidget::Draw();
}

/**
 * Reimplements 0x4bfc60: HudUiBackgroundCursorWidget::DrawBase.
 * Original source path: D:\Proj\Battlesport\hudui_background.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiBackgroundCursorWidget::DrawBase.
 */
inline void HudUiBackgroundCursorWidget::DrawBase() {
    if (bltSource != 0) {
        zVid_Image::BlitToActiveTarget(
            (zVidImagePartial *)(bltSource),
            x,
            y,
            0,
            (zVidRect32 *)(&clipRect)
        );
    }
}

/**
 * Reimplements 0x4bfc80: HudUiBackgroundVideoWidget::HudUiBackgroundVideoWidget.
 * Purpose: Initializes the background video element state before a stream is assigned.
 */
HudUiBackgroundVideoWidget::HudUiBackgroundVideoWidget()
    : HudUiElement(0, 0) {
    mediaPath[0] = '\0';
    stream = 0;
    elapsedTimeSec = 0.0f;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4bfc80 HudUiBackgroundVideoWidget::HudUiBackgroundVideoWidget callers.
 * Purpose: run the recovered HudUiBackgroundVideoWidget::~HudUiBackgroundVideoWidget teardown path.
 */
HudUiBackgroundVideoWidget::~HudUiBackgroundVideoWidget() {
    zFMV_Stream *const oldStream = stream;
    if (oldStream != 0) {
        oldStream->Destructor();
        ::operator delete(oldStream);
        stream = 0;
    }
}

/**
 * Reimplements 0x4bfcd0: HudUiBackgroundVideoWidget::Destructor.
 * Purpose: Runs the authored video-widget destructor entry used by the HUD UI owner.
 */
void HudUiBackgroundVideoWidget::Destructor() {
    this->~HudUiBackgroundVideoWidget();
}

/**
 * Reimplements 0x4bfd40: HudUiBackgroundVideoWidget::SetMediaPathOwnedAndRefresh.
 * Purpose: Stores the movie path, resolves missing media, opens the stream, and refreshes clipping.
 */
void HudUiBackgroundVideoWidget::SetMediaPathOwnedAndRefresh(
    const char *path
) {
    strncpy(
        mediaPath,
        path,
        0x104
    );

    struct _stat statBuffer;
    if (_stat(
        mediaPath,
        &statBuffer
    ) == -1) {
        char *const resolvedPath = zSys::FindFileOnDriveType(
            5,
            mediaPath,
            0
        );
        if (resolvedPath != 0) {
            strncpy(
                mediaPath,
                resolvedPath,
                0x104
            );
        }
    }

    if (_stat(
        mediaPath,
        &statBuffer
    ) == -1) {
        stream = 0;
        return;
    }

    zFMV_Stream *const newStream = (zFMV_Stream *)(::operator new(sizeof(zFMV_Stream)));
    stream = newStream != 0 ? newStream->Init(
        mediaPath,
        0
    ) : 0;

    RebuildBltRect();
}

/**
 * Reimplements 0x4bfe20: HudUiBackgroundVideoWidget::SetColorKey565.
 * Purpose: Marks the active video stream format dirty and stores the 565 color key.
 */
void HudUiBackgroundVideoWidget::SetColorKey565(
    unsigned short colorKey
) {
    if (stream != 0) {
        stream->formatFlagsPacked |= 0x02;
    }

    colorKey565 = colorKey;
}

/**
 * Reimplements 0x4bfe40: HudUiBackgroundVideoWidget::Update.
 * Purpose: Advances decoded video frames while preserving the base element update behavior.
 */
void HudUiBackgroundVideoWidget::Update(
    float deltaSeconds
) {
    if ((flags & 0x10u) != 0) {
        return;
    }

    if (stream != 0) {
        const int frameTick = (int)((float)(stream->videoFramesPerSecond) * elapsedTimeSec);
        stream->ReadAndDecodeFrame((unsigned int)(frameTick % stream->videoFrameCount));
    }

    HudUiElement::Update(deltaSeconds);
    elapsedTimeSec += deltaSeconds;
}

/**
 * Reimplements 0x4bfe90: HudUiBackgroundVideoWidget::Draw.
 * Purpose: Draws the background layer and blits the active stream with the stored color key.
 */
void HudUiBackgroundVideoWidget::Draw() {
    DrawBase();

    if (stream != 0) {
        zVid_Image::BlitToActiveTarget(
            (zVidImagePartial *)(stream),
            x,
            y,
            colorKey565,
            0
        );
    }
}

/**
 * Reimplements 0x4bfec0: HudUiBackgroundVideoWidget::DrawBase.
 * Purpose: Blits the configured background source into the current clipped video area.
 */
void HudUiBackgroundVideoWidget::DrawBase() {
    zVidImagePartial *const bltSource = (zVidImagePartial *)(this->bltSource);
    if (bltSource != 0) {
        const int dstX = x > 0 ? x : 0;
        const int dstY = y > 0 ? y : 0;
        zVid_Image::BlitToActiveTarget(
            bltSource,
            dstX,
            dstY,
            0,
            (zVidRect32 *)(&clipRect)
        );
    }
}

/**
 * Reimplements 0x4bff00: HudUiBackgroundVideoWidget::RebuildBltRect.
 * Purpose: Recomputes the stream clip rectangle against the background blit source.
 */
void HudUiBackgroundVideoWidget::RebuildBltRect() {
    HudUiRect rect;
    rect.left = GetCenterX() > 0 ? GetCenterX() : 0;
    rect.top = GetCenterY() > 0 ? GetCenterY() : 0;

    if (stream == 0) {
        return;
    }

    const int streamRight = rect.left + stream->width;
    const int streamBottom = rect.top + stream->height;

    zVidImagePartial *const bltSource = (zVidImagePartial *)(this->bltSource);
    if (bltSource != 0) {
        rect.right = streamRight < bltSource->width ? streamRight : bltSource->width;
        rect.bottom = streamBottom < bltSource->height ? streamBottom : bltSource->height;
    } else {
        rect.right = streamRight;
        rect.bottom = streamBottom;
    }

    SetClipRect(&rect);
}

/**
 * Reimplements 0x4b4ee0: HudUiZrdWidget::HudUiZrdWidget.
 * Purpose: initialize the ZRD widget's base widget state, image/sound slots,
 * panel vectors, enabled mode, and initial invalidation state.
 */
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

    labelPanels.EraseRangeNoDestroyInline(
        labelPanels.begin
    );
    rolloverLabelPanels.EraseRangeNoDestroyInline(
        rolloverLabelPanels.begin
    );
    activateLabelPanels.EraseRangeNoDestroyInline(
        activateLabelPanels.begin
    );

    *((unsigned short *)(&imageStateWord)) = 1;
    HudUiElement *element = this;
    element->Invalidate();
    unsigned int visibleFlag = (unsigned char)(flags);
    flags = (visibleFlag & 0x10u) | 0x02u;
}

/**
 * Reimplements 0x4b4ee0: HudUiZrdWidget::Constructor.
 * Purpose: initialize the recovered HudUiZrdWidget::Constructor state.
 */
HudUiZrdWidget * HudUiZrdWidget::Constructor() {
    new (this) HudUiZrdWidget;
    return this;
}

/**
 * Reimplements 0x4b59f0: HudUiZrdWidget::LoadFromZrd.
 * Source: D:\Proj\Battlesport\hud.cpp.
 * Purpose: bind a ZRD widget to its owner background, load images, sounds,
 * labels, flash settings, and initial clipping from the recovered ZRD section.
 */
int HudUiZrdWidget::LoadFromZrd(
    zReader::Node *zrdSection,
    HudUiBackground *ownerDialog
) {
    owner = ownerDialog;
    HudUiElement::SetVisible(1);
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
    zReader::Node *const positionBase = ZrdArrayBase(positionNode);
    if (positionBase != 0) {
        originX += ZrdArrayInt(
            positionBase,
            1,
            0
        );
        originY += ZrdArrayInt(
            positionBase,
            2,
            0
        );
    }

    int widgetX = originX;
    int widgetY = originY;
    zReader::Node *const bitmapNode = zReader_GetNamedNode(
        zrdSection,
        g_HudUiCycleSelectorWidget_ZrdKey_Bitmap
    );
    zReader::Node *const bitmapBase = ZrdArrayBase(bitmapNode);
    const char *const bitmapPath = ZrdArrayString(
        bitmapBase,
        1
    );
    if (bitmapPath != 0) {
        defaultImage = SetImageByPathOwned(bitmapPath);
        if (ZrdArrayCount(bitmapBase) >= 4) {
            widgetX += ZrdArrayInt(
                bitmapBase,
                2,
                0
            );
            widgetY += ZrdArrayInt(
                bitmapBase,
                3,
                0
            );
        }
    }

    HudUiElement::SetPos(
        widgetX,
        widgetY
    );

    void *const clipSource = ownerDialog->capturedCompositeImage;
    if (clipSource != 0) {
        HudUiRect *const bounds = GetBoundsRectOrNull();
        if (bounds != 0) {
            HudUiElement::SetBltSourceAndClipRect(
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
        LoadHudZrdBitmap(
            rolloverNode,
            g_HudUiCycleSelectorWidget_ZrdKey_Bitmap,
            &rolloverImage
        );
        LoadHudZrdSound(
            rolloverNode,
            &rolloverSound,
            &rolloverSoundScale
        );
        LoadHudZrdLabelSection(
            this,
            rolloverNode,
            rolloverLabelPanels
        );
        ApplyHudZrdFlashSection(
            rolloverNode,
            rolloverLabelPanels
        );
    }

    zReader::Node *const disableNode = zReader_GetNamedNode(
        zrdSection,
        g_HudZrd_Key_Disable
    );
    if (disableNode != 0) {
        LoadHudZrdBitmap(
            disableNode,
            g_HudUiCycleSelectorWidget_ZrdKey_Bitmap,
            &disabledImage
        );
        LoadHudZrdSound(
            disableNode,
            &disabledSound,
            &disabledSoundScale
        );
        LoadHudZrdLabelSection(
            this,
            disableNode,
            disabledLabelPanels
        );
    }

    zReader::Node *const activateNode = zReader_GetNamedNode(
        zrdSection,
        g_HudZrd_Key_Activate
    );
    if (activateNode != 0) {
        LoadHudZrdBitmap(
            activateNode,
            g_HudUiCycleSelectorWidget_ZrdKey_Bitmap,
            &activateImage
        );
        LoadHudZrdSound(
            activateNode,
            &activateSound,
            &activateSoundScale
        );
        LoadHudZrdLabelSection(
            this,
            activateNode,
            activateLabelPanels
        );
        ApplyHudZrdFlashSection(
            activateNode,
            activateLabelPanels
        );
    }

    LoadHudZrdLabelSection(
        this,
        zrdSection,
        labelPanels
    );
    ApplyHudZrdFlashSection(
        zrdSection,
        labelPanels
    );
    return 1;
}

/**
 * Reimplements 0x4ba4d0: HudUiPanelPtrVector::EraseRange.
 * Binary Ninja identifies the body as a VC5 std::vector-style erase helper
 * over the recovered HudUiPanelPtrVector storage: shift [last, end) over
 * first, retain the pointer-specialized _Destroy cursor walk, update end, and
 * return the original first iterator.
 * Purpose: erase a contiguous range from the recovered panel pointer vector.
 */
HudUiPanel ** HudUiPanelPtrVector::EraseRange(
    HudUiPanel **first,
    HudUiPanel **last
) {
    HudUiPanel **write = first;
    HudUiPanel **read = last;
    HudUiPanel **const oldEnd = end;
    while (read != oldEnd) {
        *write = *read;
        ++write;
        ++read;
    }

    HudUiPanel **destroyIt = write;
    HudUiPanel **const destroyEnd = end;
    while (destroyIt != destroyEnd) {
        ++destroyIt;
    }

    end = write;
    return first;
}

/**
 * Reimplements 0x4ba510: HudUiPanelPtrVector::InsertN.
 * Binary Ninja shows the matching VC5 std::vector-style insert helper for
 * HudUiPanel pointers, including in-place tail movement and reallocation when
 * the current capacity cannot hold the requested insertion count. The retail
 * template body keeps allocator-style guarded construction checks before each
 * pointer copy.
 * Purpose: insert repeated panel pointers into the recovered panel pointer vector.
 */
void HudUiPanelPtrVector::InsertN(
    HudUiPanel **position,
    unsigned int count,
    HudUiPanel **valueSource
) {
    HudUiPanel **oldEnd = end;
    const int spareCount = (int)(capacityEnd - oldEnd);
    if ((unsigned int)spareCount >= count) {
        int tailCount = (int)(oldEnd - position);
        if ((unsigned int)tailCount < count) {
            HudUiPanel **dest = position + count;
            HudUiPanel **source = position;
            while (source != oldEnd) {
                if (dest != 0) {
                    *dest = *source;
                }
                ++source;
                ++dest;
            }

            unsigned int fillCount = count - (unsigned int)tailCount;
            dest = oldEnd;
            while (fillCount != 0) {
                if (dest != 0) {
                    *dest = *valueSource;
                }
                ++dest;
                --fillCount;
            }

            dest = position;
            while (dest != oldEnd) {
                *dest = *valueSource;
                ++dest;
            }
            end += count;
            return;
        }

        if (count != 0) {
            HudUiPanel **source = oldEnd - count;
            HudUiPanel **dest = oldEnd;
            while (source != oldEnd) {
                if (dest != 0) {
                    *dest = *source;
                }
                ++source;
                ++dest;
            }

            source = oldEnd - count;
            dest = oldEnd;
            while (source != position) {
                --source;
                --dest;
                *dest = *source;
            }

            dest = position;
            HudUiPanel **const fillEnd = position + count;
            while (dest != fillEnd) {
                *dest = *valueSource;
                ++dest;
            }

            end += count;
        }
        return;
    }

    HudUiPanel **oldBegin = begin;
    const int oldSize = oldBegin != 0 ? (int)(oldEnd - oldBegin) : 0;
    const int growBy = count < (unsigned int)oldSize ? oldSize : (int)count;
    int newCapacity = oldSize + growBy;
    if (newCapacity < 0) {
        newCapacity = 0;
    }

    HudUiPanel **const newBegin =
        (HudUiPanel **)(::operator new(newCapacity * sizeof(HudUiPanel *)));
    HudUiPanel **write = newBegin;

    HudUiPanel **read = oldBegin;
    while (read != position) {
        if (write != 0) {
            *write = *read;
        }
        ++read;
        ++write;
    }

    unsigned int insertCount = count;
    while (insertCount != 0) {
        if (write != 0) {
            *write = *valueSource;
        }
        ++write;
        --insertCount;
    }

    read = position;
    while (read != oldEnd) {
        if (write != 0) {
            *write = *read;
        }
        ++read;
        ++write;
    }

    ::operator delete(oldBegin);
    begin = newBegin;
    end = newBegin + oldSize + count;
    capacityEnd = newBegin + newCapacity;
}

/**
 * Reimplements 0x4b52f0: HudUiZrdWidget::DeleteChildIfPresent
 * Source: src/Battlesport/hud.cpp
 * Purpose: scalar-delete an optional child widget through the recovered HudUiElement slot.
 */
void *__stdcall HudUiZrdWidget::DeleteChildIfPresent(
    void *childWidgetOrNull
) {
    if (childWidgetOrNull != 0) {
        delete ((HudUiElement *)(childWidgetOrNull));
    }

    return 0;
}

/**
 * Reimplements 0x4b50c0: HudUiZrdWidget::~HudUiZrdWidget
 * Source: D:\Proj\Battlesport\hud.cpp
 * Purpose: release owned ZRD widget panels and alternate images before compiler-generated member cleanup.
 */
HudUiZrdWidget::~HudUiZrdWidget() {
    {
        class DeleteChildIfPresentFunctor {
        public:
            char value;

            /**
             * Recovered from 0x4b50c0: VC5 keeps a one-byte local unary functor
             * around the first label-panel cleanup loop and delegates the child
             * deletion to the address-backed helper at 0x4b52f0.
             * Purpose: preserve the original first label-panel cleanup source shape.
             */
            void * operator()(void *childWidgetOrNull) {
                return HudUiZrdWidget::DeleteChildIfPresent(childWidgetOrNull);
            }
        };

        DeleteChildIfPresentFunctor deleteChildIfPresent;
        DeleteChildIfPresentFunctor deleteChildIfPresentCopy(deleteChildIfPresent);
        HudUiPanel **labelIt = labelPanels.begin;
        HudUiPanel **labelOut = labelPanels.begin;
        HudUiPanel **labelEnd = labelPanels.end;
        while (labelIt != labelEnd) {
            *labelOut = (HudUiPanel *)(deleteChildIfPresentCopy(*labelIt));
            ++labelIt;
            ++labelOut;
        }
    }

    {
        HudUiPanel **rolloverIt = rolloverLabelPanels.begin;
        HudUiPanel **rolloverOut = rolloverLabelPanels.begin;
        HudUiPanel **rolloverEnd = rolloverLabelPanels.end;
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
        HudUiPanel **activateIt = activateLabelPanels.begin;
        HudUiPanel **activateOut = activateLabelPanels.begin;
        HudUiPanel **activateEnd = activateLabelPanels.end;
        while (activateIt != activateEnd) {
            if (*activateIt != 0) {
                delete (*activateIt);
            }

            *activateOut = 0;
            ++activateIt;
            ++activateOut;
        }
    }

    labelPanels.EraseRange(
        labelPanels.begin,
        labelPanels.end
    );
    rolloverLabelPanels.EraseRange(
        rolloverLabelPanels.begin,
        rolloverLabelPanels.end
    );
    activateLabelPanels.EraseRange(
        activateLabelPanels.begin,
        activateLabelPanels.end
    );

    if (defaultImage != 0 && defaultImage != image) {
        defaultImage =
            (zVidImagePartial *)(unsigned int)zVid_Image::ReleaseIfNotDefault(
                defaultImage
            );
    }

    if (activateImage != 0 && activateImage != image) {
        activateImage =
            (zVidImagePartial *)(unsigned int)zVid_Image::ReleaseIfNotDefault(
                activateImage
            );
    }

    if (rolloverImage != 0 && rolloverImage != image) {
        rolloverImage =
            (zVidImagePartial *)(unsigned int)zVid_Image::ReleaseIfNotDefault(
                rolloverImage
            );
    }

    if (disabledImage != 0 && disabledImage != image) {
        disabledImage =
            (zVidImagePartial *)(unsigned int)zVid_Image::ReleaseIfNotDefault(
                disabledImage
            );
    }

    if (image != 0 && ownsImage == 0) {
        zVid_Image::ReleaseIfNotDefault(image);
    }
}

/**
 * Reimplements 0x4b50a0: HudUiZrdWidget::compiler deleting destructor.
 * No standalone retail function; source compatibility wrapper for recovered
 * HudUiZrdWidget cleanup callers that historically named the destructor body
 * DestructorCore in this reconstruction.
 * Source: D:\Proj\Battlesport\hud.cpp
 * Purpose: release owned ZRD widget panels, alternate images, panel vectors, and the base widget.
 */
void HudUiZrdWidget::DestructorCore() {
    this->~HudUiZrdWidget();
}

/**
 * Reimplements 0x4b5310: HudUiZrdWidget::Invalidate.
 * Purpose: invalidate the widget and every base label panel owned by the ZRD widget.
 */
void HudUiZrdWidget::Invalidate() {
    HudUiElement::Invalidate();

    HudUiPanel **panel = labelPanels.begin;
    if (panel == 0) {
        return;
    }

    while (panel != labelPanels.end) {
        HudUiPanel *const label = *panel;
        label->Invalidate();
        ++panel;
    }
}

/**
 * Reimplements 0x4b5350: HudUiZrdWidget::GetBoundsRectOrNull.
 * Purpose: return the recovered HUD value exposed by HudUiZrdWidget::GetBoundsRectOrNull.
 */
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

    HudUiPanel **panelIt = labelPanels.begin;
    if (panelIt == 0) {
        return 0;
    }

    HudUiPanel *const firstPanel = *panelIt;
    boundsRect.top = firstPanel->GetCenterY();
    boundsRect.bottom = boundsRect.top + firstPanel->QueryTextHeight();

    while (panelIt != labelPanels.end) {
        HudUiPanel *const panel = *panelIt;
        boundsRect.bottom += panel->QueryTextHeight();

        const int alignMode = panel->alignMode;
        const int panelX = panel->GetCenterX();
        if (panel->textDirty != 0) {
            panel->RebuildTextRect();
        }

        const int width = panel->textWidthPx;
        if (alignMode == 0) {
            boundsRect.left = firstPanel->GetCenterX();
            const int right = panelX + width;
            if (right > boundsRect.right) {
                boundsRect.right = right;
            }
        } else if (alignMode == 1) {
            const int halfWidth = width / 2;
            const int left = panelX - halfWidth;
            if (left < boundsRect.left) {
                boundsRect.left = left;
            }

            const int right = panelX + halfWidth;
            if (right > boundsRect.right) {
                boundsRect.right = right;
            }
        } else if (alignMode == 2) {
            boundsRect.right = firstPanel->GetCenterX();
            const int left = panelX - width;
            if (left > boundsRect.left) {
                boundsRect.left = boundsRect.left;
            } else {
                boundsRect.left = left;
            }
        }

        ++panelIt;
    }

    boundsRect.bottom -= firstPanel->QueryTextHeight();
    return &boundsRect;
}

/**
 * Reimplements 0x4b5740: HudUiZrdWidget::RefreshState.
 * Purpose: switch the widget between normal and disabled image/label state.
 */
void HudUiZrdWidget::RefreshState() {
    for (HudUiPanel **rolloverIt = rolloverLabelPanels.begin; rolloverIt != rolloverLabelPanels.end;
        ++rolloverIt) {
        HudUiPanel *const panel = *rolloverIt;
        panel->SetVisible(0);
    }

    for (HudUiPanel **activateIt = activateLabelPanels.begin; activateIt != activateLabelPanels.end;
        ++activateIt) {
        HudUiPanel *const panel = *activateIt;
        panel->SetVisible(0);
    }

    if (modeOrEnabled != 0) {
        for (HudUiPanel **labelIt = labelPanels.begin; labelIt != labelPanels.end; ++labelIt) {
            HudUiPanel *const panel = *labelIt;
            panel->SetVisible(1);
        }

        for (HudUiPanel **disabledIt = disabledLabelPanels.begin;
            disabledIt != disabledLabelPanels.end;
            ++disabledIt) {
            HudUiPanel *const panel = *disabledIt;
            panel->SetVisible(0);
        }

        SetImageBorrowedAndInvalidate(defaultImage);
        Invalidate();
        return;
    }

    for (HudUiPanel **labelIt2 = labelPanels.begin; labelIt2 != labelPanels.end; ++labelIt2) {
        HudUiPanel *const panel = *labelIt2;
        panel->SetVisible(0);
    }

    for (HudUiPanel **disabledIt2 = disabledLabelPanels.begin;
        disabledIt2 != disabledLabelPanels.end;
        ++disabledIt2) {
        HudUiPanel *const panel = *disabledIt2;
        panel->SetVisible(1);
    }

    SetImageBorrowedAndInvalidate(disabledImage);
    Invalidate();
}

/**
 * Reimplements 0x4b5630: HudUiZrdWidget::ShowPreview.
 * Purpose: preserve the recovered HUD behavior for HudUiZrdWidget::ShowPreview.
 */
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

    if (rolloverLabelPanels.begin != 0) {
        HudUiSetPanelVectorVisible(
            labelPanels,
            0
        );
        HudUiSetPanelVectorVisible(
            activateLabelPanels,
            0
        );
        HudUiSetPanelVectorVisible(
            rolloverLabelPanels,
            1
        );
        return;
    }

    HudUiSetPanelVectorVisible(
        labelPanels,
        1
    );
    HudUiSetPanelVectorVisible(
        activateLabelPanels,
        0
    );
}

/**
 * Reimplements 0x4b5900: HudUiZrdWidget::OnActivate.
 * Purpose: reset transition input and switch the widget from rollover to
 * activation visuals, labels, and sound.
 */
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

    HudUiSetPanelVectorVisible(
        rolloverLabelPanels,
        0
    );

    if (activateLabelPanels.begin != 0) {
        HudUiSetPanelVectorVisible(
            activateLabelPanels,
            1
        );
        HudUiSetPanelVectorVisible(
            labelPanels,
            0
        );
        return;
    }

    HudUiSetPanelVectorVisible(
        labelPanels,
        1
    );
}

/**
 * Reimplements 0x4b5860: HudUiZrdWidget::HidePreview.
 * Purpose: restore the widget's default image and normal label visibility after rollover preview.
 */
void HudUiZrdWidget::HidePreview() {
    if (defaultImage != 0) {
        SetImageBorrowedAndInvalidate(defaultImage);
    }

    if (rolloverPlayHandle != 0) {
        rolloverPlayHandle = 0;
    }

    for (HudUiPanel **rolloverIt = rolloverLabelPanels.begin; rolloverIt != rolloverLabelPanels.end;
        ++rolloverIt) {
        HudUiPanel *const panel = *rolloverIt;
        panel->SetVisible(0);
    }

    for (HudUiPanel **activateIt = activateLabelPanels.begin; activateIt != activateLabelPanels.end;
        ++activateIt) {
        HudUiPanel *const panel = *activateIt;
        panel->SetVisible(0);
    }

    for (HudUiPanel **labelIt = labelPanels.begin; labelIt != labelPanels.end; ++labelIt) {
        HudUiPanel *const panel = *labelIt;
        panel->SetVisible(1);
    }
}

/**
 * Reimplements 0x4b6fc0: HudUiCheckToggleWidget::HudUiCheckToggleWidget.
 * Purpose: preserve the recovered HUD behavior for HudUiCheckToggleWidget::HudUiCheckToggleWidget.
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
 * Reimplements 0x4b6fc0: HudUiCheckToggleWidget::Constructor.
 * Purpose: initialize the recovered HudUiCheckToggleWidget::Constructor state.
 */
HudUiCheckToggleWidget * HudUiCheckToggleWidget::Constructor() {
    new (this) HudUiCheckToggleWidget;
    return this;
}

/**
 * Reimplements 0x4b7020: HudUiCheckToggleWidget::~HudUiCheckToggleWidget.
 * Original source path: D:\Proj\Battlesport\HudUiZrdWidget.cpp.
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
 * Original source path: D:\Proj\Battlesport\HudUiZrdWidget.cpp.
 * Purpose: Run the check-toggle destructor body.
 */
void HudUiCheckToggleWidget::DestructorCore() {
    this->HudUiCheckToggleWidget::~HudUiCheckToggleWidget();
}

/**
 * Reimplements 0x4b70b0: HudUiCheckToggleWidget::GetBoundsRectOrNull.
 * Purpose: return the recovered HUD value exposed by HudUiCheckToggleWidget::GetBoundsRectOrNull.
 */
HudUiRect * HudUiCheckToggleWidget::GetBoundsRectOrNull() {
    return &boundsRect;
}

/**
 * Reimplements 0x4b70c0: HudUiCheckToggleWidget::RefreshState.
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
            zVidImagePartial *const image = checkedImage != 0 ? checkedImage : uncheckedImage;
            if (image != 0) {
                SetImageBorrowedAndInvalidate(image);
                Invalidate();
                return;
            }
        }

        Invalidate();
        return;
    }

    HudUiSetPanelVectorVisible(
        labelPanels,
        0
    );
    HudUiSetPanelVectorVisible(
        disabledLabelPanels,
        1
    );

    if (checked != 0) {
        zVidImagePartial *const image =
            disabledCheckedImage != 0 ? disabledCheckedImage : disabledCheckedFallbackImage;
        if (image != 0) {
            SetImageBorrowedAndInvalidate(image);
        }
    }

    Invalidate();
}

/**
 * Reimplements 0x4b7210: HudUiCheckToggleWidget::ShowPreview.
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
 * Reimplements 0x4b7250: HudUiCheckToggleWidget::HidePreview.
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
 * Reimplements 0x4b7290: HudUiCheckToggleWidget::OnActivate.
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
 * Reimplements 0x40caa0: HudUiCheckToggleWidget::OnActivateThunk.
 * Purpose: handle the recovered HUD event path for HudUiCheckToggleWidget::OnActivateThunk.
 */
void HudUiCheckToggleWidget::OnActivateThunk() {
    OnActivate();
}

/**
 * Reimplements 0x4b7340: HudUiCheckToggleWidget::LoadFromZrd.
 * Original source path: D:\Proj\Battlesport\HudUiZrdWidget.cpp.
 * Purpose: load check-toggle bitmap/text variants and rebuild bounds from a ZRD node.
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

        LoadHudZrdLabelSection(
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
    } else if (labelPanels.begin != 0) {
        HudUiPanel **panelIt = labelPanels.begin;
        HudUiPanel *const firstPanel = *panelIt;
        boundsRect.top = firstPanel->GetCenterY();
        boundsRect.left = firstPanel->GetCenterX();
        boundsRect.bottom = firstPanel->QueryTextHeight() + boundsRect.top;

        while (panelIt != labelPanels.end) {
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

    return 1;
}

/**
 * Reimplements 0x4b72c0: HudUiCheckToggleWidget::SetChecked.
 * Purpose: apply the recovered HUD state change handled by HudUiCheckToggleWidget::SetChecked.
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
 * Reimplements 0x4b7d60: HudUiCycleSelectorWidget::HudUiCycleSelectorWidget.
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
 * Reimplements 0x4b7d60: HudUiCycleSelectorWidget::Constructor.
 * Purpose: initialize the recovered HudUiCycleSelectorWidget::Constructor state.
 */
HudUiCycleSelectorWidget * HudUiCycleSelectorWidget::Constructor() {
    new (this) HudUiCycleSelectorWidget;
    return this;
}

/**
 * Reimplements 0x4b7de0: HudUiCycleSelectorWidget::~HudUiCycleSelectorWidget.
 * Original source path: D:\Proj\Battlesport\HudUiZrdWidget.cpp.
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
 * Original source path: D:\Proj\Battlesport\HudUiZrdWidget.cpp.
 * Purpose: Run the cycle-selector destructor body.
 */
void HudUiCycleSelectorWidget::DestructorCore() {
    this->HudUiCycleSelectorWidget::~HudUiCycleSelectorWidget();
}

/**
 * Reimplements 0x4b7ee0: HudUiCycleSelectorWidget::AdvanceSelectionAndActivate.
 * Original source file: HudUiCycleSelectorWidget.cpp.
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
 * Reimplements 0x4b7f20: HudUiCycleSelectorWidget::SetIndexClamped.
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
 * Reimplements 0x4b7f80: HudUiCycleSelectorWidget::SetVisibleRange.
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
 * Reimplements 0x4b7e60: HudUiCycleSelectorWidget::Update.
 * Purpose: advance the recovered HUD update path for HudUiCycleSelectorWidget::Update.
 */
void HudUiCycleSelectorWidget::Update(
    float deltaSeconds
) {
    for (int i = 0; i < itemCount; ++i) {
        Invalidate();

        if (entriesA[i] != 0) {
            entriesA[i]->SetVisible(i == selectedIndex ? 1 : 0);
        }

        if (entriesB[i] != 0) {
            entriesB[i]->SetVisible(i == selectedIndex ? 1 : 0);
        }
    }

    HudUiElement::Update(deltaSeconds);
}

/**
 * Reimplements 0x4b7fd0: HudUiCycleSelectorWidget::AddTextEntry.
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
 * Original inline helper; no standalone retail function exists.
 * Observed in transition-panel owners that destroy embedded panel members.
 * Evidence: destructor callers tear down the HudUiPanel base without a
 * separate retail HudUiTransitionTextPanel destructor body.
 * Purpose: restore the source-level destructor for transition text panels.
 */
HudUiTransitionTextPanel::~HudUiTransitionTextPanel() {
    HudUiPanel::~HudUiPanel();
}

/**
 * Reimplements 0x4bc9f0: HudUiTransitionTextPanel::Update.
 * Original source path: D:\Proj\Battlesport\HudUiTransitionTextPanel.cpp.
 * Purpose: update timed visibility and flash-color state before drawing the panel.
 *
 * Evidence: BN assembly subtracts delta time from the base timer and flash
 * countdown, hides timed-out panels through the visibility slot, toggles
 * flashDirectionSign/textDirty, swaps text colors for color-flash modes, and
 * calls HudUiPanel::Draw on visible refresh paths.
 */
void HudUiTransitionTextPanel::Update(
    float deltaSeconds
) {
    const unsigned int elementFlags = flags;
    if (((~elementFlags) & 0x10u) == 0) {
        return;
    }

    if ((elementFlags & 1u) != 0) {
        timer -= deltaSeconds;
        if (timer <= 0.0) {
            SetVisible(0);
        }
    }

    if (flashEnabled == 0 || ((~flags) & 0x10u) == 0) {
        HudUiPanel::Draw();
        return;
    }

    flashCountdown -= deltaSeconds;
    switch (flashMode) {
    case 0:
        HudUiPanel::Draw();
    case 1:
        if (flashCountdown < 0.0) {
            flashCountdown += flashResetValue;
            textDirty = 1;
            flashDirectionSign = -flashDirectionSign;
        }

        if (flashDirectionSign == 1) {
            HudUiPanel::Draw();
        }
        return;

    case 2:
    case 3:
        if (flashCountdown < 0.0) {
            flashCountdown = flashResetValue;
            textDirty = 1;
            flashDirectionSign = -flashDirectionSign;

            const unsigned int oldTextColor0 = textColor0;
            const unsigned int oldTextColor1 = textColor1;
            textColor0 = (unsigned int)(flashAltColor0);
            textColor1 = (unsigned int)(flashAltColor1);
            flashAltColor0 = (int)(oldTextColor0);
            flashAltColor1 = (int)(oldTextColor1);
        }

        HudUiPanel::Draw();
        return;

    default:
        HudUiPanel::Draw();
        return;
    }
}

/**
 * Reimplements 0x4bc930: HudUiTransitionTextPanel::ResetFlashState.
 *
 * Purpose: enable flash state, update a positive flash rate to its half-period,
 * reset the countdown from that period, and restore forward flash direction.
 *
 * Evidence: BN assembly at 0x4bc930 writes flashEnabled, conditionally stores
 * flashRate*0.5 into flashResetValue when flashRate is positive, copies
 * flashResetValue into flashCountdown, and writes flashDirectionSign = 1.
 */
void HudUiTransitionTextPanel::ResetFlashState(
    float flashRate
) {
    flashEnabled = 1;
    if (flashRate > 0.0f) {
        flashResetValue = flashRate * 0.5f;
    }

    flashDirectionSign = 1;
    memcpy(
        &flashCountdown,
        &flashResetValue,
        sizeof(flashCountdown)
    );
}

/**
 * Reimplements 0x4bc980: HudUiTransitionTextPanel::SetFlashRate.
 *
 * Purpose: enter rate-only flashing by resetting flash state unless the panel
 * is already in rate-only flash mode.
 *
 * Evidence: BN assembly at 0x4bc980 returns when flashMode is 1; otherwise it
 * calls ResetFlashState(flashRate) and stores flashMode = 1.
 */
void HudUiTransitionTextPanel::SetFlashRate(
    float flashRate
) {
    if (flashMode == 1) {
        return;
    }

    ResetFlashState(flashRate);
    flashMode = 1;
}

/**
 * Reimplements 0x4bc9b0: HudUiTransitionTextPanel::SetFlashColorAndRate.
 * Original source path: D:\Proj\Battlesport\HudUiPanel.cpp.
 * Purpose: enter color-flash mode and store the alternate flash text colors.
 *
 * Evidence: BN assembly returns when flashMode is already color-flash mode,
 * calls ResetFlashState, writes flashMode = 2, and stores the same alternate
 * color into both flash color fields.
 */
void HudUiTransitionTextPanel::SetFlashColorAndRate(
    unsigned int flashColor,
    float flashRate
) {
    if (flashMode == 2) {
        return;
    }

    ResetFlashState(flashRate);
    flashMode = 2;
    flashAltColor0 = flashColor;
    flashAltColor1 = flashColor;
}

/**
 * Reimplements 0x4b8100: HudUiCycleSelectorWidget::ApplyFontStyleForEntry.
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
 * Reimplements 0x4b8200: HudUiCycleSelectorWidget::AddBitmapEntry.
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
 * Reimplements 0x4b82e0: HudUiCycleSelectorWidget::LoadFromZrd.
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
 * Reimplements 0x4b8450: HudUiFillBitmap::HudUiFillBitmap.
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
 * Reimplements 0x4b84d0: HudUiFillBitmap::~HudUiFillBitmap.
 * Original source path: HudUiFillBitmap.cpp.
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
 * Original source path: HudUiFillBitmap.cpp.
 * Purpose: Run the fill-bitmap destructor body.
 */
void HudUiFillBitmap::DestructorCore() {
    this->HudUiFillBitmap::~HudUiFillBitmap();
}

/**
 * Reimplements 0x4b8520: HudUiFillBitmap::Draw.
 * Purpose: preserve the recovered HUD behavior for HudUiFillBitmap::Draw.
 */
void HudUiFillBitmap::Draw() {
    if (previewImage == 0 || fillImage == 0) {
        return;
    }

    HudUiWidget::Draw();

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
 * Reimplements 0x4b85c0: HudUiFillBitmap::LoadFromZrd.
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
        HudUiElement::Invalidate();

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

        HudUiElement::SetPos(
            posX,
            posY
        );
        previewImage = image;
        HudUiElement::Invalidate();
    }

    SetNormalizedValueAndRebuild(0.0f);
    return 1;
}

/**
 * Reimplements 0x4b8650: HudUiFillBitmap::UpdateNormalizedFromCursor.
 * Purpose: update the normalized fill value from the owner cursor and activate the widget.
 */
void HudUiFillBitmap::UpdateNormalizedFromCursor() {
    const int cursorX = owner->mouseState.cursorClientX;
    const int relativeX = cursorX - GetCenterX();
    const int imageWidth = image != 0 ? image->width : 0;
    SetNormalizedValueAndRebuild((float)(relativeX) / (float)(imageWidth));
    OnActivate();
}

/**
 * Reimplements 0x4ba3c0: HudUiFillBitmap::SetNormalizedValue.
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
 * Reimplements 0x4b86b0: HudUiFillBitmap::SetNormalizedValueAndRebuild.
 * Purpose: apply the recovered HUD state change handled by HudUiFillBitmap::SetNormalizedValueAndRebuild.
 */
void HudUiFillBitmap::SetNormalizedValueAndRebuild(
    float value
) {
    if (fillImage == 0) {
        return;
    }

    normalizedValue = value;
    HudUiElement::Invalidate();

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
 * Reimplements 0x4b8760: HudUiZrdWidgetEx17C_Item::HudUiZrdWidgetEx17C_Item.
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
 * Reimplements 0x4b87c0: HudUiZrdWidgetEx17C_Item::DestructorCore.
 * Purpose: run the recovered HudUiZrdWidgetEx17C_Item::DestructorCore teardown path.
 */
HudUiZrdWidgetEx17C_Item * HudUiZrdWidgetEx17C_Item::Constructor() {
    new (this) HudUiZrdWidgetEx17C_Item;
    return this;
}

/**
 * Reimplements 0x4b87c0: HudUiZrdWidgetEx17C_Item::DestructorCore.
 * Purpose: run the recovered HudUiZrdWidgetEx17C_Item::DestructorCore teardown path.
 */
void HudUiZrdWidgetEx17C_Item::DestructorCore() {
    HudUiZrdWidget::DestructorCore();
}

/**
 * Reimplements 0x4b87d0: HudUiZrdWidgetEx17C_Item::ShowPreviewIfNotSelected.
 * Purpose: preserve the recovered HUD behavior for HudUiZrdWidgetEx17C_Item::ShowPreviewIfNotSelected.
 */
void HudUiZrdWidgetEx17C_Item::ShowPreviewIfNotSelected() {
    if (selected == 0) {
        HudUiZrdWidget::ShowPreview();
    }
}

/**
 * Reimplements 0x4b87e0: HudUiZrdWidgetEx17C_Item::HidePreviewIfNotSelected.
 * Purpose: preserve the recovered HUD behavior for HudUiZrdWidgetEx17C_Item::HidePreviewIfNotSelected.
 */
void HudUiZrdWidgetEx17C_Item::ShowPreview() {
    ShowPreviewIfNotSelected();
}

/**
 * Reimplements 0x4b87e0: HudUiZrdWidgetEx17C_Item::HidePreviewIfNotSelected.
 * Purpose: preserve the recovered HUD behavior for HudUiZrdWidgetEx17C_Item::HidePreviewIfNotSelected.
 */
void HudUiZrdWidgetEx17C_Item::HidePreviewIfNotSelected() {
    if (selected == 0) {
        HudUiZrdWidget::HidePreview();
    }
}

/**
 * Reimplements 0x4b87f0: HudUiZrdWidgetEx17C_Item::OnActivateSelectSelf.
 * Purpose: handle the recovered HUD event path for HudUiZrdWidgetEx17C_Item::OnActivateSelectSelf.
 */
void HudUiZrdWidgetEx17C_Item::HidePreview() {
    HidePreviewIfNotSelected();
}

/**
 * Reimplements 0x4b87f0: HudUiZrdWidgetEx17C_Item::OnActivateSelectSelf.
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
 * Reimplements 0x4b8850: HudUiZrdWidgetEx17C_Item::LoadFromZrd.
 * Purpose: load the recovered HUD data handled by HudUiZrdWidgetEx17C_Item::LoadFromZrd.
 */
void HudUiZrdWidgetEx17C_Item::OnActivate() {
    OnActivateSelectSelf();
}

/**
 * Reimplements 0x4b8850: HudUiZrdWidgetEx17C_Item::LoadFromZrd.
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
    } else if (labelPanels.begin != 0) {
        HudUiPanel **panelIt = labelPanels.begin;
        HudUiPanel *const firstPanel = *panelIt;
        boundsRect.top = firstPanel->GetCenterY();
        boundsRect.left = firstPanel->GetCenterX();
        boundsRect.bottom = firstPanel->QueryTextHeight() + boundsRect.top;

        while (panelIt != labelPanels.end) {
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
 * Reimplements 0x4b8a90: HudUiZrdWidgetEx17C_Item::SetSelected
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
 * Reimplements 0x4b8af0: HudUiZrdWidgetEx17C_Item::GetMouseRectOrBounds.
 * Purpose: return the recovered HUD value exposed by HudUiZrdWidgetEx17C_Item::GetMouseRectOrBounds.
 */
HudUiRect * HudUiZrdWidgetEx17C_Item::GetMouseRectOrBounds() {
    return mouseRectValid != 0 ? &mouseRect : GetBoundsRectOrNull();
}

/**
 * Reimplements 0x4b8b10: HudUiZrdWidgetEx17C::HudUiZrdWidgetEx17C.
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
 * Reimplements 0x4b8b60: HudUiZrdWidgetEx17C::~HudUiZrdWidgetEx17C
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
 * Reimplements 0x4b8b60: HudUiZrdWidgetEx17C::DestructorCore.
 * Purpose: run the recovered HudUiZrdWidgetEx17C destructor through the compatibility name.
 */
HudUiZrdWidgetEx17C * HudUiZrdWidgetEx17C::Constructor() {
    new (this) HudUiZrdWidgetEx17C;
    return this;
}

/**
 * Reimplements 0x4b8b60: HudUiZrdWidgetEx17C::DestructorCore compatibility wrapper.
 * No standalone retail function; source compatibility wrapper for recovered
 * callers that historically named the destructor body DestructorCore in this
 * reconstruction.
 * Purpose: Run the option-selector destructor body.
 */
void HudUiZrdWidgetEx17C::DestructorCore() {
    this->HudUiZrdWidgetEx17C::~HudUiZrdWidgetEx17C();
}

/**
 * Reimplements 0x4b8be0: HudUiZrdWidgetEx17C::LoadFromZrd.
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
                    (HudUiZrdWidgetEx17C_Item *)(::operator new(sizeof(HudUiZrdWidgetEx17C_Item)));
                option->Constructor();
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
 * Reimplements 0x4b8cf0: HudUiZrdWidgetEx17C::SetSelectedIndex.
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
 * Reimplements 0x4b8cf0: HudUiZrdWidgetEx17C::SetSelectedIndex
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
 * Reimplements 0x4b9520: HudUiListSelectorItem::OnActivate.
 * Source model note: Reimplements 0x4b92a0: HudUiListSelectorItem::HudUiListSelectorItem.
 * Source model lives in the inline class-body constructor in zhud_ui.h.
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
 * Reimplements 0x4ba410: HudUiListSelectorItem::Draw.
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

// Physical hud.cpp command-binding layer relocation notes. The following
// address-backed bodies moved to src/Battlesport/hud_command_binding_layer_body.h,
// included by src/Battlesport/hud.cpp, so VC5 can emit the command-binding
// layer through the physical hud.cpp compilation path. The disabled include
// keeps narrow provenance scans able to see the moved docblocks without
// compiling duplicate definitions from zui.cpp.
#if 0
#include "Battlesport/hud_command_binding_layer_body.h"
#endif

 /**
 * Reimplements 0x4b8d30: HudCmdBindButtonBase::HudCmdBindButtonBase.
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
 * Reimplements 0x4ba470: StdPtrVector::FreeBufferAndReset.
 * Purpose: frees the owned pointer buffer and resets the vector iterator triplet.
 */
void StdPtrVector::FreeBufferAndReset() {
    int *const oldBegin = begin;
    ::operator delete(oldBegin);
    begin = 0;
    end = 0;
    capacityEnd = 0;
}

/**
 * Reimplements 0x4b9320: HudCmdBindButtonBase::OnSelectedIndexChanged.
 * Original source path: D:\Proj\Battlesport\HudCmdBindButton.cpp.
 * Purpose: handle the recovered HUD event path for HudCmdBindButtonBase::OnSelectedIndexChanged.
 */
void HudCmdBindButtonBase::OnSelectedIndexChanged(
    int selectedIndex
) {
    SetSelectedEntry(selectedIndex);
}

/**
 * Reimplements 0x4b9330: HudCmdBindButtonBase::SetSelectedEntry.
 * Original source path: D:\Proj\Battlesport\HudCmdBindButton.cpp.
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
 * Reimplements 0x40be00: the command-binding cleanup now instantiates the
 * canonical VC5 std::transform provider from zhud_ui.h.
 * Reimplements 0x40bdc0: vector::erase supplies the canonical clear/move-end
 * contribution formerly represented by a hand-authored vector-reset helper.
 * Reimplements 0x40be60: vector::erase selects the canonical VC5 std::copy
 * provider rather than a hand-authored copy helper.
 * Reimplements 0x40bf00: HudCmdBindingEntry's ordinary destructor owns the
 * display-string cleanup formerly modeled as a utility method.
 * Reimplements 0x40bf20: retained as a legacy verification anchor pending
 * parent classification of the natural compiler-emitted contribution.
 * Reimplements 0x40c1d0: HudCmdBindButtonBase::ClearBindingEntries is now an
 * optimizer-visible member defined with the owner in zhud_ui.h.
 * Purpose: retain precise provenance for the canonical command-binding
 * lifetime model compiled through this consumer translation unit.
 */
#if !defined(_MSC_VER) || _MSC_VER >= 1200
/**
 * Original-source helper; no standalone retail function exists.
 * Restores the VC5 std::vector<HudCmdBindingEntry *>::erase(first,last)
 * dependency used by 0x40b680 after the caller destroys each pointed-to
 * binding entry. The caller-visible retail body invokes the vector erase
 * helper rather than only assigning end = begin.
 * Purpose: keep command-binding vector cleanup source-shaped as typed STL
 * storage while matching the retail caller's erase dependency.
 */
HudCmdBindingEntry **HudCmdBindingVector::erase(
    HudCmdBindingEntry **eraseFirst,
    HudCmdBindingEntry **eraseLast
) {
    HudCmdBindingEntry **write = eraseFirst;
    HudCmdBindingEntry **read = eraseLast;
    HudCmdBindingEntry **const oldEnd = last;
    if (read != oldEnd) {
        do {
            *write++ = *read++;
        } while (read != oldEnd);
    }
    ((StdPtrVector *)(this))->ClearNoOpDestroy(
        (int *)(write),
        (int *)(oldEnd)
    );
    last = write;
    return eraseFirst;
}
#endif

/**
 * Reimplements 0x4b90e0: HudCmdBindButtonBase::RebuildBindingSlotWidgets.
 * Original source path: D:\Proj\Battlesport\HudCmdBindButton.cpp.
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
 * Reimplements 0x4b8de0: HudCmdBindButtonBase::LoadFromZrd.
 * Original source path: D:\Proj\Battlesport\HudCmdBindButton.cpp.
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
 * Reimplements 0x40f2d0: HudUiWidget::HudUiWidget.
 * Purpose: preserve the recovered HUD behavior for HudUiWidget::HudUiWidget.
 */
HudUiWidget::~HudUiWidget() {
    ReleaseImageIfOwned();
}

/**
 * Reimplements 0x4b4030: HudUiWidget::HitTest.
 * Purpose: preserve the recovered HUD behavior for HudUiWidget::HitTest.
 */
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
 * Reimplements 0x4b3fb0: HudUiWidget::Draw.
 * Original source path: D:\Proj\Battlesport\hudui.cpp.
 * Purpose: draw pending widget dirty rectangles or the whole widget image after the base draw pass.
 */
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

/**
 * Reimplements 0x4b3da0: HudUiWidget::ReleaseImageIfOwned.
 * Purpose: release an owned widget image and clear the ownership bit.
 */
void HudUiWidget::ReleaseImageIfOwned() {
    if (image != 0 && ownsImage != 0) {
        zVid_Image::ReleaseIfNotDefault(image);
        image = 0;
    }

    ownsImage = 0;
}

/**
 * Reimplements 0x4b3e70: HudUiWidget::SetImageBorrowedAndInvalidate.
 *
 * Purpose: install a borrowed widget image, clear ownership, invalidate the
 * widget, and return the borrowed image pointer.
 *
 * Evidence: BN assembly at 0x4b3e70 clears ownsImage at offset 0x34, stores
 * the incoming image at offset 0x3c, dispatches Invalidate through the
 * HudUiWidget class slot, and returns the image argument in eax.
 */
zVidImagePartial * HudUiWidget::SetImageBorrowedAndInvalidate(
    zVidImagePartial *newImage
) {
    ownsImage = 0;
    image = newImage;
    Invalidate();
    return newImage;
}

/**
 * Reimplements 0x4b3e30: HudUiWidget::SetImageByPathOwned.
 * Purpose: replace an owned widget image from a texture-directory path and invalidate the widget.
 */
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

/**
 * Reimplements 0x4b3dd0: HudUiWidget::SetPos.
 * Purpose: apply the recovered HUD state change handled by HudUiWidget::SetPos.
 */
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

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x40e910 HudUiTriplet::InterpolateLayout callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnPrintableKey.
 */
void HudUiTextInput::OnPrintableKey(
    int key
) {
    InsertCharAtCursor(key);
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x40e910 HudUiTriplet::InterpolateLayout callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnAccept.
 */
void HudUiTextInput::OnAccept() {
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b4370 HudUiTextInput::~HudUiTextInput callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnCancel.
 */
void HudUiTextInput::OnCancel() {
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b4370 HudUiTextInput::~HudUiTextInput callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnBackspace.
 */
void HudUiTextInput::OnBackspace() {
    BackspaceDeleteChar();
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b4370 HudUiTextInput::~HudUiTextInput callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnDeleteForward.
 */
void HudUiTextInput::OnDeleteForward() {
    DeleteCharForward();
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b4370 HudUiTextInput::~HudUiTextInput callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnMoveCursorLeft.
 */
void HudUiTextInput::OnMoveCursorLeft() {
    MoveCursorLeft();
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b4370 HudUiTextInput::~HudUiTextInput callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnMoveCursorRight.
 */
void HudUiTextInput::OnMoveCursorRight() {
    MoveCursorRight();
}

/**
 * Reimplements 0x4b4370: HudUiTextInput::~HudUiTextInput.
 * Source model note: Source-faithful helper recovered from address-backed callers in this
 * source file.
 * Purpose: run the recovered HudUiTextInput::~HudUiTextInput teardown path.
 */
void HudUiTextInput::OnOverflow() {
}

/**
 * Reimplements 0x4b4370: HudUiTextInput::~HudUiTextInput.
 * Current BN assembly resets the HudUiTextInput vptr, then deletes the owned
 * buffer. Modeling this as the authored C++ destructor preserves that class
 * cleanup shape without a hand-written table reset.
 * Purpose: tear down the base text-input buffer after derived text-input
 * cleanup has restored the base class identity.
 */
HudUiTextInput::~HudUiTextInput() {
    char *const ownedBuffer = buffer;
    ::operator delete(ownedBuffer);
}

/**
 * Reimplements 0x4b4ab0: HudUiTextInput::DestructorCoreThunk.
 * Purpose: tail-call the recovered base text-input destructor from legacy
 * thunk entry points.
 */
void HudUiTextInput::DestructorCore() {
    this->HudUiTextInput::~HudUiTextInput();
}

/**
 * Reimplements 0x4b4390: HudUiTextInput::AllocTextBuffer.
 * Purpose: preserve the recovered HUD behavior for HudUiTextInput::AllocTextBuffer.
 */
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

/**
 * Reimplements 0x4b42f0: HudUiTextInput::HudUiTextInput.
 * Purpose: preserve the recovered HUD behavior for HudUiTextInput::HudUiTextInput.
 */
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

/**
 * Reimplements 0x4b4420: HudUiTextInput::SetCursorPosition.
 * Purpose: apply the recovered HUD state change handled by HudUiTextInput::SetCursorPosition.
 */
HudUiTextInput * HudUiTextInput::Constructor(
    int bufferSize
) {
    new (this) HudUiTextInput(bufferSize);
    return this;
}

/**
 * Reimplements 0x4b4420: HudUiTextInput::SetCursorPosition.
 * Purpose: apply the recovered HUD state change handled by HudUiTextInput::SetCursorPosition.
 */
void HudUiTextInput::SetCursorPosition(
    int position
) {
    cursor =
        (position < (int)(strlen(buffer)))
            ? (unsigned int)(position)
            : (unsigned int)(strlen(buffer));
}

/**
 * Reimplements 0x4b43d0: HudUiTextInput::SetContents.
 * Purpose: apply the recovered HUD state change handled by HudUiTextInput::SetContents.
 */
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

/**
 * Reimplements 0x4b4410: HudUiTextInput::GetBuffer.
 * Purpose: return the recovered HUD value exposed by HudUiTextInput::GetBuffer.
 */
char * HudUiTextInput::GetBuffer() {
    return buffer;
}

/**
 * Reimplements 0x4b4590: HudUiTextInput::ShiftTextRight.
 * Purpose: make room in the edit buffer for inserted characters.
 */
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

/**
 * Reimplements 0x4b45e0: HudUiTextInput::ShiftTextLeft.
 * Purpose: close a deleted text range by shifting the following characters.
 */
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

/**
 * Reimplements 0x4b4550: HudUiTextInput::DeleteCharForward.
 * Purpose: delete the character at the cursor without moving the cursor.
 */
void HudUiTextInput::DeleteCharForward() {
    ShiftTextLeft(
        1,
        (int)(cursor)
    );
}

/**
 * Reimplements 0x4b4560: HudUiTextInput::MoveCursorLeft.
 * Purpose: move the edit cursor one position left when possible.
 */
void HudUiTextInput::MoveCursorLeft() {
    if ((int)(cursor) > 0) {
        --cursor;
    }
}

/**
 * Reimplements 0x4b4570: HudUiTextInput::MoveCursorRight.
 * Purpose: move the edit cursor one position right within the text contents.
 */
void HudUiTextInput::MoveCursorRight() {
    const int textLength = (int)(strlen(buffer));
    if ((int)(cursor) < textLength) {
        ++cursor;
    }
}

/**
 * Reimplements 0x4b4530: HudUiTextInput::BackspaceDeleteChar.
 * Purpose: delete the character before the cursor and move the cursor back.
 */
void HudUiTextInput::BackspaceDeleteChar() {
    if ((int)(cursor) > 0) {
        --cursor;
        ShiftTextLeft(
            1,
            (int)(cursor)
        );
    }
}

/**
 * Reimplements 0x4b44e0: HudUiTextInput::InsertCharAtCursor.
 * Purpose: insert one printable character at the current cursor position.
 */
void HudUiTextInput::InsertCharAtCursor(
    int ch
) {
    const int textLength = (int)(strlen(buffer));
    if (textLength >= (int)(capacity)-1) {
        OnOverflow();
        return;
    }

    ShiftTextRight(
        1,
        (int)(cursor)
    );
    buffer[cursor] = (char)(ch);
    ++cursor;
}

/**
 * Reimplements 0x4b4460: HudUiTextInput::DispatchKeyAction.
 * Binary Ninja shows the key action byte read from HudUiTextInput::keyActionMap
 * and dispatches action values 0 through 7 through the text-input virtual
 * methods; no authored globals are touched by this body.
 * Purpose: translate a raw key into the recovered text-input editing action.
 */
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

/**
 * Reimplements 0x4ba3e0: HudUiOwnedTextInput::OnAccept.
 * Purpose: handle the recovered HUD event path for HudUiOwnedTextInput::OnAccept.
 */
void HudUiOwnedTextInput::OnAccept() {
    zGame::ReturnOnlyStub();
    owner->OnAcceptForwardToCommit();
}

/**
 * Reimplements 0x40d660: HudUiMgrObjectiveBlock::~HudUiMgrObjectiveBlock.
 * Purpose: run the recovered HudUiMgrObjectiveBlock::~HudUiMgrObjectiveBlock teardown path.
 */
void HudUiChatComposeTextInput::OnAccept() {
    GameNet::EndChatComposeAndSendThunk();
}

/**
 * Reimplements 0x40db20: HudUiSlot::HudUiSlot.
 * Purpose: construct the common HUD element base and embedded slot widgets
 * through ordinary C++ member construction.
 */
HudUiSlot::HudUiSlot() : HudUiElement(
        0,
        0
) {
}

/**
 * Reimplements 0x4bcf20: HudUiBar::HudUiBar.
 * Purpose: Constructs the HUD element base, clears bar point storage, and marks the bar dirty.
 */
HudUiBar::HudUiBar() : HudUiElement(
    0,
    0
) {
    drawVertexCount = 0;
    memset(
        points,
        0,
        sizeof(points)
    );
    Invalidate();
}

/**
 * Reimplements 0x4bcff0: HudUiBar::Draw.
 * Binary Ninja evidence: dispatches the base DrawBase method, reads
 * drawVertexCount, and calls zRndr::RasterizePoly with points and drawParam
 * only when at least one vertex is active.
 * Purpose: Draw the bar base and rasterize the populated point list.
 */
void HudUiBar::Draw() {
    DrawBase();
    if (drawVertexCount != 0) {
        zRndr_RasterizePoly(
            (zVec3 *)(points),
            drawVertexCount,
            drawParam
        );
    }
}

/**
 * Reimplements 0x4bcf80: HudUiBar::SetPointXY.
 * Original file: D:\Proj\Battlesport\hud.cpp.
 * Binary Ninja evidence: bounds-checks pointIndex against the 21-element point
 * array, writes the HudUiBarPoint x/y fields, raises drawVertexCount, dispatches
 * SetPos for point zero, and always invalidates the element.
 * Purpose: Update one bar point and keep the element position/count state dirty.
 */
void HudUiBar::SetPointXY(
    int pointIndex,
    float x,
    float y
) {
    if (pointIndex >= 0 && pointIndex < 21) {
        points[pointIndex].x = x;
        points[pointIndex].y = y;

        if (drawVertexCount < pointIndex + 1) {
            drawVertexCount = pointIndex + 1;
        }

        if (pointIndex == 0) {
            SetPos(
                (int)(x),
                (int)(y)
            );
        }
    }

    Invalidate();
}

/**
 * Reimplements 0x4bf840: HudUiPolyline::HudUiPolyline.
 * Purpose: preserve the recovered HUD behavior for HudUiPolyline::HudUiPolyline.
 */
HudUiPolyline::HudUiPolyline()
    : HudUiElement(
          0,
          0
      ) {
    pointCount = 0;
    memset(
        points,
        0,
        sizeof(points)
    );
    Invalidate();
    clipRect = 0;
}

/**
 * Reimplements 0x4bf900: HudUiPolyline::Draw.
 * Purpose: preserve the recovered HUD behavior for HudUiPolyline::Draw.
 */
HudUiPolyline * HudUiPolyline::Constructor() {
    new (this) HudUiPolyline;
    return this;
}

/**
 * Reimplements 0x4bf900: HudUiPolyline::Draw.
 * Purpose: preserve the recovered HUD behavior for HudUiPolyline::Draw.
 */
void HudUiPolyline::Draw() {
    DrawBase();

    const int currentPointCount = pointCount;
    if (currentPointCount == 0) {
        return;
    }

    if (clipRect != 0) {
        zRndr_DrawClippedImmediateLineStrip(
            (const zRndr_LinePoint2I *)(points),
            currentPointCount - 1,
            clipRect,
            color565
        );
        return;
    }

    {
        for (int index = 0; index < currentPointCount - 1; ++index) {
            const HudUiPolylinePoint &point = points[index];
            const HudUiPolylinePoint &nextPoint = points[index + 1];
            zRndr_DrawImmediateLine(
                point.x,
                point.y,
                nextPoint.x,
                nextPoint.y,
                color565
            );
        }
    }
}

/**
 * Reimplements 0x4bf8b0: HudUiPolyline::SetPoint.
 * Purpose: apply the recovered HUD state change handled by HudUiPolyline::SetPoint.
 */
void HudUiPolyline::SetPoint(
    int index,
    int pointX,
    int pointY
) {
    points[index].x = pointX;
    points[index].y = pointY;

    if (pointCount <= index) {
        pointCount = index + 1;
    }

    if (index == 0) {
        SetPos(
            pointX,
            pointY
        );
    }

    Invalidate();
}

/**
 * Reimplements 0x4b4620: HudUiSliderBorder::HudUiSliderBorder.
 * Purpose: preserve the recovered HUD behavior for HudUiSliderBorder::HudUiSliderBorder.
 */
HudUiSliderBorder::HudUiSliderBorder() {
    originX = 0;
    originY = 0;
    halfWidth = 1;
    height = 10;
    blinkEnabled = 0;
    blinkPeriodSec = 0.35f;
    blinkDirSign = 1;
    blinkTimeRemainingSec = 0.0f;

    SetPoint(
        0,
        -1,
        0
    );
    SetPoint(
        1,
        halfWidth,
        0
    );
    SetPoint(
        2,
        halfWidth,
        1
    );
    SetPoint(
        3,
        0,
        1
    );
    SetPoint(
        4,
        0,
        height - 1
    );
    SetPoint(
        5,
        halfWidth,
        height - 1
    );
    SetPoint(
        6,
        halfWidth,
        height
    );
    SetPoint(
        7,
        -halfWidth,
        height
    );
    SetPoint(
        8,
        -halfWidth,
        height - 1
    );
    SetPoint(
        9,
        0,
        height - 1
    );
    SetPoint(
        10,
        0,
        1
    );
    SetPoint(
        11,
        -halfWidth,
        1
    );
    SetPoint(
        12,
        -halfWidth,
        0
    );
}

/**
 * Reimplements 0x4b47b0: HudUiSliderBorder::Update.
 * Purpose: advance the recovered HUD update path for HudUiSliderBorder::Update.
 */
HudUiSliderBorder * HudUiSliderBorder::Constructor() {
    new (this) HudUiSliderBorder;
    return this;
}

/**
 * Reimplements 0x4b47b0: HudUiSliderBorder::Update.
 * Purpose: advance the recovered HUD update path for HudUiSliderBorder::Update.
 */
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

/**
 * Reimplements 0x4b4810: HudUiSliderBorder::SetBounds.
 * Purpose: Stores slider border bounds and rebuilds the polyline outline points.
 */
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

    SetPoint(
        0,
        originX - halfWidth,
        originY
    );
    SetPoint(
        1,
        originX + halfWidth,
        originY
    );
    SetPoint(
        2,
        originX + halfWidth,
        originY + 1
    );
    SetPoint(
        3,
        originX,
        originY + 1
    );
    SetPoint(
        4,
        originX,
        originY + height - 1
    );
    SetPoint(
        5,
        originX + halfWidth,
        originY + height - 1
    );
    SetPoint(
        6,
        originX + halfWidth,
        originY + height
    );
    SetPoint(
        7,
        originX - halfWidth,
        originY + height
    );
    SetPoint(
        8,
        originX - halfWidth,
        originY + height - 1
    );
    SetPoint(
        9,
        originX,
        originY + height - 1
    );
    SetPoint(
        10,
        originX,
        originY + 1
    );
    SetPoint(
        11,
        originX - halfWidth,
        originY + 1
    );
    SetPoint(
        12,
        originX - halfWidth,
        originY
    );
}

/**
 * Reimplements 0x4b49e0: HudUiNumericTextInput::HudUiNumericTextInput.
 * Purpose: Construct the ZRD widget base and owned numeric text-entry controls.
 */
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
 * Reimplements 0x41a190: HudUiNumericTextInput::Constructor.
 * Purpose: initialize the recovered HudUiNumericTextInput::Constructor state.
 */
HudUiNumericTextInput * HudUiNumericTextInput::BaseConstructor() {
    new (this) HudUiNumericTextInput;
    return this;
}

/**
 * Reimplements 0x41a190: HudUiNumericTextInput::Constructor.
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
 * Reimplements 0x41a200: HudUiClampedIntTextInput::HudUiClampedIntTextInput.
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
 * Reimplements 0x4b4e40: HudUiNumericTextInput::AllocTextBuffer.
 * Purpose: preserve the recovered HUD behavior for HudUiNumericTextInput::AllocTextBuffer.
 */
void HudUiNumericTextInput::AllocTextBuffer(
    unsigned int bufferSize
) {
    textInput.AllocTextBuffer(bufferSize);
}

/**
 * Reimplements 0x4b4ed0: HudUiNumericTextInput::GetBuffer.
 * Purpose: return the recovered HUD value exposed by HudUiNumericTextInput::GetBuffer.
 */
char * HudUiNumericTextInput::GetBuffer() {
    return textInput.GetBuffer();
}

/**
 * Reimplements 0x4b4e60: HudUiNumericTextInput::Update.
 * Purpose: Update the text-input buffer, mirror the visible label text, and invalidate the owning widget.
 */
void HudUiNumericTextInput::Update(
    const char *text
) {
    textInput.SetContents(text);
    textInput.SetCursorPosition((int)(strlen(text)));
    char *const buffer = textInput.GetBuffer();

    if (labelPanels.Count() != 0) {
        HudUiPanel *const firstPanel = labelPanels.At(0);
        firstPanel->SetText(buffer);
    }

    Invalidate();
}

/**
 * Reimplements 0x4b4ca0: HudUiNumericTextInput::UpdateCaptureUiAndClip.
 * Purpose: advance the recovered HUD update path for HudUiNumericTextInput::UpdateCaptureUiAndClip.
 */
RECOIL_NO_GS void HudUiNumericTextInput::UpdateCaptureUiAndClip(
    float deltaSeconds
) {
    HudUiPanel *const firstPanel = labelPanels.begin[0];
    HudUiElement *const baseElement = (HudUiElement *)(this);

    if ((flags & 0x10) != 0) {
        firstPanel->SetVisible(0);
        firstPanel->Invalidate();
        sliderBorder.SetVisible(0);
        sliderBorder.Invalidate();
        return;
    }

    if (sliderBorder.sliderVisibleWhenInputActive != 0) {
        firstPanel->SetVisible(1);
        char *const buffer = textInput.GetBuffer();

        if (labelPanels.begin != 0) {
            const ptrdiff_t panelCount = labelPanels.end - labelPanels.begin;
            if (panelCount != 0) {
                firstPanel->SetText(buffer);
            }
        }

        RECT textRect = {0};
        textRect.left = firstPanel->GetCenterX();
        textRect.top = firstPanel->GetCenterY();
        textRect.right = firstPanel->GetCenterX();
        textRect.bottom = firstPanel->GetCenterY();

        if (firstPanel->MeasureTextPrefixRect(
            (int)(textInput.cursor),
            &textRect
        ) != 0) {
            const unsigned int textColor = firstPanel->textColor0;
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

    baseElement->Update(deltaSeconds);
    sliderBorder.Update(deltaSeconds);
}

/**
 * Reimplements 0x4b4c50: HudUiNumericTextInput::SetRawKeyboardCapture.
 * Purpose: apply the recovered HUD state change handled by HudUiNumericTextInput::SetRawKeyboardCapture.
 */
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

/**
 * Reimplements 0x4b4c90: HudUiNumericTextInput::OnActivate.
 * Purpose: handle the recovered HUD event path for HudUiNumericTextInput::OnActivate.
 */
void HudUiNumericTextInput::OnActivate() {
    sliderBorder.inputActive = 1;
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x4b4ac0: HudUiNumericTextInput::~HudUiNumericTextInput.
 * Binary Ninja shows VC5 destructor codegen: derived vtable restore, raw
 * keyboard capture release, embedded HudUiOwnedTextInput teardown, then
 * HudUiZrdWidget cleanup with EH state transitions.
 * Purpose: Disable raw keyboard capture before C++ member/base destruction.
 */
HudUiNumericTextInput::~HudUiNumericTextInput() {
    SetRawKeyboardCapture(0);
}

/**
 * Reimplements 0x41a3f0: HudUiNumericTextInput::DestructorThunk.
 * Source-faithful helper wrapper for legacy native smoke call sites; the
 * address-backed retail body is the C++ destructor above.
 * Purpose: route compatibility calls through the recovered C++ destructor.
 */
void HudUiNumericTextInput::Destructor() {
    this->HudUiNumericTextInput::~HudUiNumericTextInput();
}

/**
 * Reimplements 0x41a3f0: HudUiNumericTextInput::DestructorThunk.
 * Purpose: Route the ftable destructor closure through the recovered typed
 * numeric-input destructor wrapper.
 */
void HudUiNumericTextInput::DestructorThunk() {
    Destructor();
}

/**
 * Reimplements 0x4b4b30: HudUiNumericTextInput::RawKeyboardCallback.
 * Purpose: preserve the recovered HUD behavior for HudUiNumericTextInput::RawKeyboardCallback.
 */
int __fastcall HudUiNumericTextInput::RawKeyboardCallback(
    int key,
    HudUiNumericTextInput *callbackCtx
) {
    if (callbackCtx != 0) {
        return callbackCtx->OnRawKeyboardChar(key);
    }

    return 0;
}

/**
 * Reimplements 0x4b4ba0: HudUiNumericTextInput::SetInputActive.
 * Purpose: Show or hide the numeric text input, slider border, and first
 * label panel while returning the previous active state.
 */
int HudUiNumericTextInput::SetInputActive(
    int active
) {
    HudUiPanel *firstLabelPanel = 0;
    const int previousActive = sliderBorder.inputActive;
    sliderBorder.inputActive = active;

    const int labelPanelCount = labelPanels.Count();
    unsigned char labelPanelsEmpty = labelPanelCount == 0;
    if (labelPanelsEmpty == 0) {
        firstLabelPanel = labelPanels.At(0);
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

/**
 * Reimplements 0x4b4b50: HudUiNumericTextInput::OnRawKeyboardChar.
 * Purpose: handle the recovered HUD event path for HudUiNumericTextInput::OnRawKeyboardChar.
 */
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

/**
 * Reimplements 0x41a290: HudUiNumericTextInput::OnAcceptForwardToCommit.
 * Purpose: handle the recovered HUD event path for HudUiNumericTextInput::OnAcceptForwardToCommit.
 */
int HudUiNumericTextInput::OnAcceptForwardToCommit() {
    return CommitAndGetValue();
}

/**
 * Reimplements 0x41a2a0: HudUiClampedIntTextInput::OnRawKeyboardChar.
 * No standalone retail function has been identified for the base numeric
 * text-input commit slot; clamped/save-game owners override the slot when they
 * need committed values.
 * Purpose: provide the base numeric input commit default.
 */
int HudUiNumericTextInput::CommitAndGetValue() {
    return 0;
}

/**
 * Reimplements 0x41a2a0: HudUiClampedIntTextInput::OnRawKeyboardChar.
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
 * Reimplements 0x41a2d0: HudUiClampedIntTextInput::CommitAndGetValue.
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
 * Reimplements 0x41a350: HudUiClampedIntStepButton::OnActivate.
 * Binary Ninja source file D:\Proj\Battlesport\hud.cpp shows the target-input
 * guard, virtual commit slot, signed step/clamp, numeric text update, target
 * invalidate slot, then HudUiZrdWidget activation.
 * Purpose: commit the linked clamped integer input, apply this button's step,
 * clamp/display the result, invalidate the input, and run base activation.
 */
void HudUiClampedIntStepButton::OnActivate() {
    if (targetInput != 0) {
        HudUiClampedIntTextInput *input = targetInput;
        int value = input->CommitAndGetValue() + stepDelta;

        if (value < input->minValue) {
            value = input->minValue;
        }

        if (value > input->maxValue) {
            value = input->maxValue;
        }

        char valueText[20];
        sprintf(
            valueText,
            "%d",
            value
        );
        input->Update(valueText);

        input = targetInput;
        input->Invalidate();
    }

    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x41a7b0: HudUiNetGameSetupTextInput::OnActivateFocusAndCursor.
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
 * Reimplements 0x41ab60: HudUiNetGameSetupOverlayOwner::StaticInitAndRegisterAtExit.
 * Purpose: preserve the recovered HUD behavior for HudUiNetGameSetupOverlayOwner::StaticInitAndRegisterAtExit.
 */
void HudUiNetGameSetupTextInput::OnActivate() {
    OnActivateFocusAndCursor();
}

/**
 * Reimplements 0x41ab60: HudUiNetGameSetupOverlayOwner::StaticInitAndRegisterAtExit.
 * BN source path: D:\Proj\Battlesport\HudUi.cpp.
 * Purpose: construct the static multiplayer setup overlay owner and register
 * its at-exit destructor during HUD static initialization.
 */
void __cdecl HudUiNetGameSetupOverlayOwner::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

/**
 * Reimplements 0x41ab70: HudUiNetGameSetupOverlayOwner::StaticInit.
 * BN source path: D:\Proj\Battlesport\HudUi.cpp.
 * Purpose: placement-construct the global multiplayer setup overlay owner
 * singleton in its zero-initialized storage.
 */
HudUiNetGameSetupOverlayOwner *HudUiNetGameSetupOverlayOwner::StaticInit() {
    return new (&g_HudUiNetGameSetupOverlayOwner) HudUiNetGameSetupOverlayOwner;
}

/**
 * Reimplements 0x41ab80: HudUiNetGameSetupOverlayOwner::RegisterAtExit.
 * BN source path: D:\Proj\Battlesport\HudUi.cpp.
 * Purpose: register the static overlay owner destructor with the CRT atexit
 * list after the singleton is constructed.
 */
void HudUiNetGameSetupOverlayOwner::RegisterAtExit() {
    atexit(AtExitDestructor);
}

/**
 * Reimplements 0x41ab90: HudUiNetGameSetupOverlayOwner::AtExitDestructor.
 * BN source path: D:\Proj\Battlesport\HudUi.cpp.
 * Purpose: destroy the global multiplayer setup overlay owner from the CRT
 * at-exit callback.
 */
void __cdecl HudUiNetGameSetupOverlayOwner::AtExitDestructor() {
    g_HudUiNetGameSetupOverlayOwner.~HudUiNetGameSetupOverlayOwner();
}

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *HudUiNetGameSetupOverlayOwnerCrtInitializerFn)();
/* VC5 emits this setup-overlay-owner startup callback as a direct .CRT$XCU row. */
#pragma data_seg(".CRT$XCU")
HudUiNetGameSetupOverlayOwnerCrtInitializerFn s_HudUiNetGameSetupOverlayOwnerCrtInit =
    HudUiNetGameSetupOverlayOwner::StaticInitAndRegisterAtExit;
#pragma data_seg()
#endif

/**
 * Reimplements 0x41aba0: HudUiNetGameSetupOverlayOwner::HudUiNetGameSetupOverlayOwner.
 * BN source path: D:\Proj\Battlesport\HudUi.cpp.
 * Purpose: initialize the overlay owner state with no active setup panel and
 * no pending reconfigure request.
 */
HudUiNetGameSetupOverlayOwner::HudUiNetGameSetupOverlayOwner()
    : m_reconfigureExistingSession(0) {
    m_dialog = 0;
}

/**
 * Reimplements 0x41abe0: HudUiNetGameSetupOverlayOwner::~HudUiNetGameSetupOverlayOwner.
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
 * Reimplements 0x41ac50: HudUiNetGameSetupOverlayOwner::OnTryBecomeCurrent.
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
 * Reimplements 0x41ad20: HudUiNetGameSetupOverlayOwner::OnDeactivate.
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
 * Reimplements 0x41ad80: HudUiNetGameSetupOverlayOwner::QueueEnterWithReconfigureFlag.
 * BN source path: D:\Proj\GameZRecoil\zHud\HudUiNetGameSetup.cpp.
 * Purpose: store the requested reconfigure mode on the static overlay owner
 * and queue that owner as the next application state.
 */
void HudUiNetGameSetupOverlayOwner::QueueEnterWithReconfigureFlag(
    int reconfigureExistingSession
) {
    g_HudUiNetGameSetupOverlayOwner.m_reconfigureExistingSession = reconfigureExistingSession;
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_HudUiNetGameSetupOverlayOwner,
        0
    );
}

/**
 * Reimplements 0x4bcb50: HudUiTextLabel::HudUiTextLabel.
 * Purpose: initialize label text, position, font handle, and alignment state.
 */
HudUiTextLabel::HudUiTextLabel(
    const char *text,
    int initX,
    int initY,
    int flags
) : HudUiElement(
        0,
        0
    ) {
    centerText = 0;
    SetTextFmt(text);
    x = initX;
    y = initY;
    ((HudUiElement *)(this))->Invalidate();
    fontHandle = flags;
    ((HudUiElement *)(this))->Invalidate();
    alignMode = 0;
}

/**
 * Original helper; no standalone retail function exists. Observed in the
 * HudUiTextLabel method cluster as the caller-owned storage wrapper around
 * the 0x4bcb50 address-backed constructor.
 * Purpose: construct a text label in caller-provided storage and return it.
 */
HudUiTextLabel * HudUiTextLabel::ConstructorWithPosAndFlags(
    const char *text,
    int initX,
    int initY,
    int flags
) {
    new (this) HudUiTextLabel(
        text,
        initX,
        initY,
        flags
    );
    return this;
}

/**
 * Reimplements 0x4bcbe0: HudUiTextLabel::CopyConstructor.
 * Original source path: D:\Proj\Battlesport\HudUiTextLabel.cpp.
 * Purpose: Copy-construct a text label from an existing label, including its text buffer.
 */
HudUiTextLabel * HudUiTextLabel::CopyConstructor(
    const HudUiTextLabel *source
) {
    HudUiElement::CopyConstructor(source);
    strncpy(
        textBuffer,
        source->textBuffer,
        sizeof(textBuffer)
    );
    fontHandle = source->fontHandle;
    centerText = source->centerText;
    centerBoundsLeft = source->centerBoundsLeft;
    centerBoundsRight = source->centerBoundsRight;
    alignMode = source->alignMode;
    return this;
}

/**
 * Reimplements 0x4bcc80: HudUiTextLabel::Constructor.
 * Original source path: D:\Proj\Battlesport\HudUiTextLabel.cpp.
 * Purpose: Initialize this text label by copying the source label state.
 */
HudUiTextLabel * HudUiTextLabel::Constructor(
    const HudUiTextLabel *source
) {
    HudUiElement::CopyFrom(source);
    strncpy(
        textBuffer,
        source->textBuffer,
        sizeof(textBuffer)
    );
    fontHandle = source->fontHandle;
    centerText = source->centerText;
    centerBoundsLeft = source->centerBoundsLeft;
    centerBoundsRight = source->centerBoundsRight;
    alignMode = source->alignMode;
    return this;
}

/**
 * Reimplements 0x4bccf0: HudUiTextLabel::SetTextFmt.
 * Purpose: format label text, refresh centered extents when needed, and
 * invalidate the element.
 */
void HudUiTextLabel::SetTextFmt(
    const char *format,
    ...
) {
    if (format == 0) {
        memset(
            textBuffer,
            0,
            sizeof(textBuffer)
        );
        return;
    }

    va_list args;
    va_start(
        args,
        format
    );
    vsprintf(
        textBuffer,
        format,
        args
    );
    va_end(args);

    if (centerText != 0) {
        UpdateTextExtents();
    }

    Invalidate();
}

/**
 * Reimplements 0x4bcd80: HudUiTextLabel::RebuildTextBounds.
 * Purpose: rebuild the clip rectangle from the current formatted text size.
 */
void HudUiTextLabel::RebuildTextBounds() {
    int widthPx;
    int lineAdvance;
    zImage_Font::MeasureString(
        textBuffer,
        fontHandle,
        &widthPx,
        &lineAdvance
    );
    clipRect.right = clipRect.left + widthPx;
    clipRect.bottom = clipRect.top + lineAdvance;
}

/**
 * Reimplements 0x4bcdc0: HudUiTextLabel::MeasureTextWidth.
 * Purpose: return the measured pixel width of the current label text.
 */
int HudUiTextLabel::MeasureTextWidth() {
    int widthPx;
    int lineAdvance;
    zImage_Font::MeasureString(
        textBuffer,
        fontHandle,
        &widthPx,
        &lineAdvance
    );
    return widthPx;
}

/**
 * Reimplements 0x4bce30: HudUiTextLabel::OnDraw.
 * Purpose: draw non-empty label text with the recovered alignment handling.
 */
void HudUiTextLabel::OnDraw() {
    DrawBase();

    if (textBuffer[0] == '\0') {
        return;
    }

    if (alignMode != 0) {
        int xOffset = MeasureTextWidth();
        if (alignMode == 1) {
            xOffset >>= 1;
        }

        x -= xOffset;
        zImage_Font::BlitStringToActiveTarget(
            textBuffer,
            x,
            y,
            fontHandle
        );
        x += xOffset;
        return;
    }

    zImage_Font::BlitStringToActiveTarget(
        textBuffer,
        x,
        y,
        fontHandle
    );
}

/**
 * Reimplements 0x4bcea0: HudUiTextLabel::HitTest.
 * Purpose: test coordinates against the visible text bounds unless input is
 * disabled.
 */
int HudUiTextLabel::HitTest(
    int px,
    int py
) {
    if ((flags & 0x10u) != 0 || x > px || y > py) {
        return 0;
    }

    int textWidth = 0;
    int lineAdvance = 0;
    zImage_Font::MeasureString(
        textBuffer,
        fontHandle,
        &textWidth,
        &lineAdvance
    );

    if (px > x + textWidth) {
        return 0;
    }

    return py <= y + lineAdvance ? 1 : 0;
}

/**
 * Reimplements 0x4bcdf0: HudUiTextLabel::UpdateTextExtents.
 * Purpose: recenter the label inside its stored bounds and refresh clip
 * extents when a blit source is active.
 */
void HudUiTextLabel::UpdateTextExtents() {
    const int widthPx = MeasureTextWidth();
    x = centerBoundsLeft + (centerBoundsRight - widthPx - centerBoundsLeft) / 2;

    if (bltSource != 0) {
        clipRect.top = y;
        clipRect.left = x;
        RebuildTextBounds();
    }
}

/**
 * Reimplements 0x4ba740: HudUiPanel::HudUiPanel.
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
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4bd100 HudUiPanel::ConstructorDefaultThunk callers.
 * Purpose: preserve the recovered HUD behavior for HudUiPanel::ConstructorDefault.
 */
HudUiPanel * HudUiPanel::ConstructorDefault(
    const char *text,
    int initX,
    int initY
) {
    new (this) HudUiPanel(
        text,
        initX,
        initY
    );
    return this;
}

/**
 * Reimplements 0x4bd100: HudUiPanel::ConstructorDefaultThunk.
 * Purpose: preserve the recovered HUD behavior for HudUiPanel::ConstructorDefaultThunk.
 */
HudUiPanel * HudUiPanel::ConstructorDefaultThunk() {
    return ConstructorDefault(
        0,
        0,
        0
    );
}

/**
 * Reimplements 0x4ba850: HudUiPanel::CopyConstructCore.
 * Original source path: D:\Proj\Battlesport\HudUiPanel.cpp.
 * Purpose: Copy-construct panel-owned text and font state from another panel.
 */
HudUiPanel * HudUiPanel::CopyConstructCore(
    const HudUiPanel *source
) {
    // BN 0x4ba87c installs HudUiPanel dispatch identity during copy
    // construction into vector storage.
    new (this) HudUiPanel;
    HudUiTextLabel::CopyConstructor(source);

    textPick = 0;
    textColor0 = source->textColor0;
    textColor1 = source->textColor1;

    LOGFONTA logFont = {0};
    if (GetObjectA(
        source->hFont,
        sizeof(logFont),
        &logFont
    ) != 0) {
        hFont = CreateFontIndirectA(&logFont);
    }

    cachedTextLength = source->cachedTextLength;
    strncpy(
        cachedText,
        source->cachedText,
        0x100
    );

    textWidthPx = source->textWidthPx;
    textHeightPx = source->textHeightPx;
    shadowEnabled = source->shadowEnabled;
    bkMode = source->bkMode;
    bkColor = source->bkColor;
    textDirty = source->textDirty;
    unknown274 = source->unknown274;
    wordWrapEnabled = source->wordWrapEnabled;
    wrapRect = source->wrapRect;
    textRect = source->textRect;
    alignMode = source->alignMode;
    shadowOffsetX = source->shadowOffsetX;
    shadowOffsetY = source->shadowOffsetY;
    return this;
}

/**
 * Reimplements 0x4ba9e0: HudUiPanel::ConstructorCopy.
 * Original source path: D:\Proj\Battlesport\HudUiPanel.cpp.
 * Purpose: Initialize this panel by copying text, font, and layout state from another panel.
 */
HudUiPanel * HudUiPanel::ConstructorCopy(
    const HudUiPanel *source
) {
    HudUiTextLabel::Constructor(source);

    textPick = 0;
    textColor0 = source->textColor0;
    textColor1 = source->textColor1;

    LOGFONTA logFont = {0};
    if (GetObjectA(
        source->hFont,
        sizeof(logFont),
        &logFont
    ) != 0) {
        hFont = CreateFontIndirectA(&logFont);
    }

    cachedTextLength = source->cachedTextLength;
    strncpy(
        cachedText,
        source->cachedText,
        0x100
    );

    textWidthPx = source->textWidthPx;
    textHeightPx = source->textHeightPx;
    shadowEnabled = source->shadowEnabled;
    bkMode = source->bkMode;
    bkColor = source->bkColor;
    textDirty = 1;
    unknown274 = source->unknown274;
    wordWrapEnabled = source->wordWrapEnabled;
    wrapRect = source->wrapRect;
    textRect = source->textRect;
    alignMode = source->alignMode;
    shadowOffsetX = source->shadowOffsetX;
    shadowOffsetY = source->shadowOffsetY;
    return this;
}

/**
 * Reimplements 0x4bab40: HudUiPanel::~HudUiPanel.
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
 * Reimplements 0x4bb460: HudUiPanel::Draw.
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
 * Reimplements 0x4ba400: HudUiPanel::GetWrapRect.
 * Purpose: Returns the panel word-wrap rectangle storage.
 */
HudUiRect * HudUiPanel::GetWrapRect() {
    return &wrapRect;
}

/**
 * Reimplements 0x4bb3d0: HudUiPanel::HitTest.
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
 * Reimplements 0x4bb440: HudUiPanel::GetLastTextPtr.
 * Original file: D:\Proj\Battlesport\HudUiPanel.cpp.
 * Purpose: return the cached panel text after ensuring dirty text rendering state is rebuilt.
 */
char * HudUiPanel::GetLastTextPtr() {
    if (textDirty != 0) {
        RebuildTextRect();
    }

    return cachedText;
}

/**
 * Reimplements 0x4bb740: HudUiPanel::GetTextRect.
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
 * Reimplements 0x4babb0: HudUiPanel::SetFont.
 * Original source path: D:\Proj\Battlesport\HudUiPanel.cpp.
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
 * Reimplements 0x4bb540: HudUiPanel::SetTextFmt.
 * Purpose: format stack varargs into the panel text buffer and refresh cached
 * panel text state when the content changes.
 */
void HudUiPanel::SetTextFmt(
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
 * Reimplements 0x4bb5e0: HudUiPanel::SetTextFmtV.
 * Purpose: format a va_list into the panel text buffer and refresh cached
 * panel text state when the content changes.
 */
void HudUiPanel::SetTextFmtV(
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
 * Reimplements 0x4bb680: HudUiPanel::SetText.
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
 * Reimplements 0x4bac10: HudUiPanel::RebuildTextRect.
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
 * Reimplements 0x4bb2a0: HudUiPanel::UpdateTextBoundsFromContent.
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
 * Reimplements 0x4bb1c0: HudUiPanel::MeasureTextPrefixRect.
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
 * Reimplements 0x4bb710: HudUiPanel::QueryTextHeight.
 * Purpose: return the panel text height without external leading after rebuilding dirty text metrics.
 */
int HudUiPanel::QueryTextHeight() {
    if (textDirty != 0) {
        RebuildTextRect();
    }

    return textHeightPx - unknown274;
}

namespace HudScoreboard {

} // namespace HudScoreboard

/**
 * Reimplements 0x4bd160: HudUiTextStack4::PushLine.
 * Original source path: D:\Proj\Battlesport\HudUiTextStack4.cpp.
 * Purpose: push a visible timed message into the four-row text stack.
 */
HudUiPanel * HudUiTextStack4::PushLine(
    const char *message,
    float duration
) {
    SetEnabled(1);

    if (((~((HudUiElement *)(&lines[0]))->flags) & 0x10u) != 0 &&
        strcmp(
            message,
            lines[0].GetLastTextPtr()
        ) != 0) {
        for (HudUiPanel *source = &lines[2]; source >= &lines[0]; --source) {
            HudUiPanel *const dest = source + 1;
            HudUiElement *const sourceElement = (HudUiElement *)(source);

            if (((~sourceElement->flags) & 0x10u) != 0) {
                source->SetVisible(0);
                ((HudUiElement *)(dest))->SetTimer(
                    ((HudUiElement *)(source))->timer
                );
                dest->SetTextFmt(source->GetLastTextPtr());
                dest->textColor0 = source->textColor0;
                dest->textColor1 = source->textColor1;
                dest->textDirty = 1;
                ((HudUiElement *)(dest))->SetVisible(1);
            }
        }
    }

    ((HudUiElement *)(&lines[0]))->SetTimer(duration);
    lines[0].SetTextFmt(
        "%s",
        message
    );
    ((HudUiElement *)(&lines[0]))->SetVisible(1);
    return &lines[0];
}

/**
 * Reimplements 0x4bd470: zTimedTask::RemoveFromActiveList.
 * Purpose: preserve the recovered HUD behavior for zTimedTask::RemoveFromActiveList.
 */
void zTimedTask::RemoveFromActiveList() {
    zTimedTask *node = g_zTimedTask_ActiveHead;
    zTimedTask *previous = 0;
    if (node == 0) {
        return;
    }

    while (node != this) {
        previous = node;
        node = node->next;
        if (node == 0) {
            return;
        }
    }

    if (previous == 0) {
        g_zTimedTask_ActiveHead = g_zTimedTask_ActiveHead->next;
        --g_zTimedTask_ActiveCount;
        return;
    }

    if (node == g_zTimedTask_ActiveTail) {
        g_zTimedTask_ActiveTail = previous;
    }

    previous->next = node->next;
    --g_zTimedTask_ActiveCount;
}

/**
 * Reimplements 0x4bd4d0: zTimedTask::RunImmediateAction.
 * Purpose: preserve the recovered HUD behavior for zTimedTask::RunImmediateAction.
 */
void zTimedTask::RunImmediateAction() {
    switch (kind) {
    case 1:
        if (actionArg2 != 0) {
            zVid_Image::BlitToActiveTarget(
                (zVidImagePartial *)(actionArg2),
                actionArg0,
                actionArg1,
                (unsigned short)(actionArg3),
                (zVidRect32 *)(actionArg4)
            );
        }
        break;

    case 2:
        zRndr_DrawImmediateLine(
            actionArg0,
            actionArg1,
            actionArg2,
            actionArg3,
            actionArg4
        );
        break;

    case 3:
        zRndr_RasterizePoly(
            (zVec3 *)(&actionArg0),
            rasterVertexCount,
            rasterDrawParam
        );
        break;

    case 4: {
        const char *text = (const char *)(&actionArg2) + 2;
        if (*text != '\0') {
            zImage_Font::BlitStringToActiveTarget(
                text,
                (short)(actionArg0),
                (short)(actionArg1),
                (short)(actionArg2)
            );
        }
        break;
    }

    case 5: {
        const char *text = (const char *)(actionArg3);
        if (text != 0 && *text != '\0') {
            zImage_Font::BlitStringToActiveTarget(
                text,
                (short)(actionArg0),
                (short)(actionArg1),
                (short)(actionArg2)
            );
        }
        break;
    }

    case 6:
        zRndr_SpanOcclusion_TestSample(
            actionArg0,
            actionArg1,
            actionArg2
        );
        break;

    case 7: {
        zVec3 point0;
        zVec3 point1;
        int point0Clipped;
        int point1Clipped;
        point0.x = (float)(actionArg0);
        point0.y = (float)(actionArg1);
        point1.x = (float)(actionArg2);
        point1.y = (float)(actionArg3);

        if (HudLineClip::ClipSegmentToCurrentBounds(
                &point0,
                &point1,
                &point0Clipped,
                &point1Clipped
            ) != 0) {
            zRndr_DrawImmediateLine(
                (int)(point0.x),
                (int)(point0.y),
                (int)(point1.x),
                (int)(point1.y),
                actionArg4
            );
        }
        break;
    }

    case 8:
        zRndr_DrawClippedImmediateLineStrip(
            (const zRndr_LinePoint2I *)(&actionArg0),
            alphaPointCount - 1,
            (void *)(alpha255),
            alphaVariantIndex
        );
        break;

    default:
        break;
    }
}

/**
 * Reimplements 0x4bd660: zTimedTask::TickActiveList.
 * Purpose: preserve the recovered HUD behavior for zTimedTask::TickActiveList.
 */
void zTimedTask::TickActiveList() {
    zTimedTask *task = g_zTimedTask_ActiveHead;
    while (task != 0) {
        if ((task->flags & 0x02) == 0) {
            task->RunImmediateAction();
        } else if ((task->flags & 0x04) != 0) {
            task->RunImmediateAction();
            task->flags &= ~0x04;
        } else if ((task->flags & 0x08) != 0) {
            task->RunImmediateAction();
            task->flags &= ~0x08;
        }

        if ((task->flags & 0x01) != 0) {
            task->remainingSeconds -= g_FrameDeltaTimeSec;
            if (task->remainingSeconds <= 0.0f) {
                task->kind = 9;
                task->RemoveFromActiveList();
            }
        }

        task = task->next;
    }
}

namespace HudUi {
/**
 * Reimplements 0x4bc760: HudUi::SetInvalidateMode.
 * Purpose: apply the recovered HUD state change handled by HudUi::SetInvalidateMode.
 */
void __fastcall SetInvalidateMode(
    int mode
) {
    g_HudUi_InvalidateMask = mode != 0 ? 0x0c : 0x04;
}

/**
 * Reimplements 0x426150: HudUi::HandleHotkeyCommand.
 * Original source path: D:\Proj\Battlesport\hudui.cpp.
 * Purpose: Dispatch gameplay hotkeys to camera, HUD, cheat, chat, aux overlay, throttle, and save/load commands.
 */
void __fastcall HandleHotkeyCommand(
    int commandId
) {
    switch (commandId) {
    case 9:
        Player::ToggleSteeringModeAndResetMouseLook();
        return;
    case 30:
        Player::ApplyCameraState(0);
        return;
    case 31:
        Player::ApplyCameraState(2);
        return;
    case 32:
        HudUiMgr::ToggleHud();
        return;
    case 33:
        zOpt::ToggleHudTypeForCurrentHwMode();
        return;
    case 35:
        if (zOpt::GetNetworkEnabled() == 0) {
            HudUiCallback::QueueCheatCodeState();
        }
        zInput::Keyboard_ResetTransitionState();
        return;
    case 36:
        if (g_HudUi_AuxOverlayEnabled == 0) {
            g_HudUi_AuxOverlayEnabled = 1;
            HudUiMgr::SetFloatTimerVisible(1);
            HudUiMgr::SetAuxOverlayVisible(1);
        } else {
            g_HudUi_AuxOverlayEnabled = 0;
            HudUiMgr::SetFloatTimerVisible(0);
            HudUiMgr::SetAuxOverlayVisible(0);
        }
        return;
    case 42:
        GameNet::BeginChatCompose();
        return;
    case 43:
        if (zOpt::GetThrottleMode() == 0) {
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x24c),
                5.0f
            );
            zOpt::SetThrottleMode(1);
        } else {
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x24d),
                5.0f
            );
            zOpt::SetThrottleMode(0);
        }
        return;
    case 44:
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiMgrObjective::Show(
                0,
                g_HudUiMessage_NodeName,
                zLoc::GetMessageString(0x86),
                2.0f
            );
        } else if (HudUiMainMenuDialog::CanLoadGame() != 0) {
            RecoilStateSaveLoadTransition::QueueOpenLoadDialog(RECOIL_SAVELOAD_MODE_QUICKLOAD);
        } else {
            HudUiMgrObjective::Show(
                0,
                g_HudUiMessage_NodeName,
                zLoc::GetMessageString(0x87),
                2.0f
            );
        }
        return;
    case 45:
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiMgrObjective::Show(
                0,
                g_HudUiMessage_NodeName,
                zLoc::GetMessageString(0x85),
                2.0f
            );
        } else if (HudUiMainMenuDialog::CanSaveGame() != 0) {
            RecoilStateSaveLoadTransition::QueueOpenSaveDialog(
                RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED
            );
        } else {
            HudUiMgrObjective::Show(
                0,
                g_HudUiMessage_NodeName,
                zLoc::GetMessageString(0x82),
                2.0f
            );
        }
        return;
    default:
        return;
    }
}

/**
 * Reimplements 0x42bf40: HudUi::PlayPowerupSfx.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: lazily resolve the powerup sound sample and play or stop its active voices.
 */
void __fastcall PlayPowerupSfx(
    int shouldPlay
) {
    static zSndSample *powerupSample = zSnd::FindSampleByName("snd_powerup");

    if (shouldPlay != 0) {
        powerupSample->PlayA3DSimple(1.0f);
        return;
    }

    powerupSample->StopActiveVoicesIfPlaying();
}

/**
 * Reimplements 0x4bd280: HudUi::PushTopMessageLine.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: push a message directly into the global top-message stack.
 */
void __fastcall PushTopMessageLine(
    const char *message,
    float duration
) {
    g_HudUiTopMessageStack->PushLine(
        message,
        duration
    );
}
} // namespace HudUi

/**
 * Reimplements 0x4bd3d0: HudUiTextStack4::SetTextColors.
 * Purpose: assign both text colors to every row in the four-line stack.
 */
void HudUiTextStack4::SetTextColors(
    unsigned int color0,
    unsigned int color1
) {
    for (int index = 3; index >= 0; --index) {
        HudUiPanel *const panel = &lines[index];
        panel->textColor0 = color0;
        panel->textColor1 = color1;
        panel->textDirty = 1;
    }
}

/**
 * Reimplements 0x4bd2a0: HudUiTextStack4::Clear.
 * Purpose: clear text and hide every row in the four-line stack.
 */
void HudUiTextStack4::Clear() {
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = &lines[index];
        panel->SetTextFmt("");
        panel->SetVisible(0);
    }
}

/**
 * Reimplements 0x4bd110: HudUiTextStack4::SetFontAll.
 * Original source path: D:\Proj\Battlesport\HudUiTextStack4.cpp.
 * Purpose: apply one font definition to every row in the four-line stack.
 */
void HudUiTextStack4::SetFontAll(
    const char *faceName,
    int height,
    int weight,
    int width
) {
    for (int index = 3; index >= 0; --index) {
        HudUiPanel *const panel = &lines[index];
        panel->SetFont(
            faceName,
            height,
            weight,
            width,
            0,
            0,
            2
        );
    }
}

/**
 * Reimplements 0x4bd410: HudUiTextStack4::SetXAll.
 * Purpose: move every row in the four-line stack to a shared x position.
 */
void HudUiTextStack4::SetXAll(
    int newX
) {
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = &lines[index];
        panel->SetX(newX);
    }
}

/**
 * Reimplements 0x4bd440: HudUiTextStack4::SetYDescending.
 * Purpose: place every row in the four-line stack at descending y positions.
 */
void HudUiTextStack4::SetYDescending(
    int yStart
) {
    int y = yStart;
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = &lines[index];
        panel->SetY(y);
        y -= 0x12;
    }
}

/**
 * Reimplements 0x4bd020: HudUiTopMessageStack::Constructor.
 * Purpose: construct the top-message four-line stack and configure ascending rows.
 */
HudUiTopMessageStack * HudUiTopMessageStack::Constructor() {
    new ((HudUiContainer *)this) HudUiContainer;

    {
        for (int index = 0; index < 4; ++index) {
            lines[index].ConstructorDefault(
                0,
                0,
                0
            );
        }
    }

    int y = 0x1e;
    {
        for (int index = 0; index < 4; ++index) {
            ConfigureTextStackLine(
                this,
                &lines[index],
                y,
                0x0d,
                0x258,
                7
            );
            y += 0x12;
        }
    }

    return this;
}

/**
 * Reimplements 0x4bd2d0: HudUiChatMessageStack::Constructor.
 * Purpose: construct the chat-message four-line stack and configure descending rows.
 */
HudUiChatMessageStack * HudUiChatMessageStack::Constructor() {
    new ((HudUiContainer *)this) HudUiContainer;

    {
        for (int index = 0; index < 4; ++index) {
            lines[index].ConstructorDefault(
                0,
                0,
                0
            );
        }
    }

    int y = 0x159;
    {
        for (int index = 0; index < 4; ++index) {
            HudUiPanel *const panel = &lines[index];
            panel->textColor0 = 0x00996a00;
            panel->textColor1 = 0x0095c7ff;
            panel->textDirty = 1;
            ConfigureTextStackLine(
                this,
                panel,
                y,
                0x0a,
                0x1f4,
                6
            );
            y -= 0x12;
        }
    }

    return this;
}

/*
 * Single physical compile host for the authored HudWeatherFx class-family bodies.
 * Reimplements 0x4bdc70: HudWeatherFx::HudWeatherFx(int).
 * Reimplements 0x4bde40: HudWeatherFx::~HudWeatherFx.
 * Reimplements 0x4bdee0: HudWeatherFx::ResetParticleSlot.
 * Reimplements 0x4bdfd0: HudWeatherFx::ApplyPass3.
 * Reimplements 0x4be210: HudWeatherFxPointBatch::ArePointBatchInsideRect.
 * Reimplements 0x4be280: HudWeatherFxSnow::HudWeatherFxSnow(int).
 * Reimplements 0x4be2e0: HudWeatherFxSnow::~HudWeatherFxSnow.
 * Reimplements 0x4be2f0: HudWeatherFxSnow::Update.
 * Reimplements 0x4be810: HudWeatherFxRain::HudWeatherFxRain(int).
 * Reimplements 0x4be870: HudWeatherFxRain::~HudWeatherFxRain.
 * Reimplements 0x4be880: HudWeatherFxRain::Update.
 * Purpose: Compile the recovered weather implementation bodies once from the
 * later zUI physical host while keeping them out of the early hud.cpp object;
 * the VC5 order manifest inventories compiler-generated lifecycle contributions separately.
 */
#include "GameZRecoil/zUI/hud_weather_fx_body.h"
