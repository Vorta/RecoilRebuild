#include "GameZRecoil/zEffect/zeff.h"

#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zUtil/zutil.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zdi.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-runtimemanager
 * @recoil-artifact defines .data recoil:data:0x575a40: g_zEffect_RuntimeManager.
 * Purpose: Owns the loaded runtime-effect template table, free-list, parent,
 * and listener state used by eff_runtime.c.
 */
zEffect_RuntimeManager g_zEffect_RuntimeManager = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-randunittable
 * @recoil-artifact defines .data recoil:data:0x575a80: g_zEffect_RandUnitTable.
 * Purpose: Stores the initialized random-unit table consumed by zEffect
 * animation runtime events.
 */
float g_zEffect_RandUnitTable[200] = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-randunitscale
 * @recoil-artifact defines .data recoil:data:0x53a0b8: g_zEffect_RandUnitScale.
 * Purpose: Stores the rand-to-unit scale used to populate
 * g_zEffect_RandUnitTable during zEffect_Anim::Init.
 */
float g_zEffect_RandUnitScale = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-randtableindex
 * @recoil-artifact defines .data recoil:data:0x53a244: g_zEffect_RandTableIndex.
 * Purpose: Tracks the cursor into g_zEffect_RandUnitTable for repeated
 * animation event sampling.
 */
int g_zEffect_RandTableIndex = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-zbdfilename
 * @recoil-artifact defines .data recoil:data:0x53a1c0: g_zEffectAnim_ZbdFilename.
 * Purpose: Stores the animation ZBD path loaded by zeff_anim_init.c.
 */
char g_zEffectAnim_ZbdFilename[0x80] = {0};
/* Animation load-state globals at 0x575da0..0x575db4 are stored as the
 * original adjacent records read by zEffect_Anim::LoadZbd and cleared by
 * Init/Shutdown. Keep declaration order and zero initialization source-visible.
 */
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-entriesinstantiated
 * @recoil-artifact defines .data recoil:data:0x575da0: g_zEffectAnim_EntriesInstantiated.
 * Purpose: Records whether animation entries have been loaded and
 * instantiated.
 */
int g_zEffectAnim_EntriesInstantiated = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-heapptr
 * @recoil-artifact defines .data recoil:data:0x575da4: g_zEffectAnim_HeapPtr.
 * Purpose: Retains the loaded animation heap block released by shutdown.
 */
void *g_zEffectAnim_HeapPtr = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-countspackedloword
 * @recoil-artifact defines .data recoil:data:0x575da8: g_zEffectAnim_CountsPackedLoWord.
 * Purpose: Stores the packed count field read from the animation ZBD header.
 */
short g_zEffectAnim_CountsPackedLoWord = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-entrycount
 * @recoil-artifact defines .data recoil:data:0x575daa: g_zEffectAnim_EntryCount.
 * Purpose: Tracks the number of entries in g_zEffectAnim_EntryList.
 */
short g_zEffectAnim_EntryCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-entrylist
 * @recoil-artifact defines .data recoil:data:0x575dac: g_zEffectAnim_EntryList.
 * Purpose: Owns the loaded animation entry table used by load, shutdown,
 * save, activation, and runtime dispatch.
 */
zEffectAnimEntry *g_zEffectAnim_EntryList = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-textidentrycount
 * @recoil-artifact defines .data recoil:data:0x575db0: g_zEffectAnim_TextIdEntryCount.
 * Purpose: Tracks localized text-id entries loaded from the animation ZBD.
 */
int g_zEffectAnim_TextIdEntryCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-textidentrylist
 * @recoil-artifact defines .data recoil:data:0x575db4: g_zEffectAnim_TextIdEntryList.
 * Purpose: Owns the localized text-id entry table loaded from the animation
 * ZBD.
 */
zEffectAnimTextIdEntry *g_zEffectAnim_TextIdEntryList = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-sourcefilestampcount
 * @recoil-artifact defines .data recoil:data:0x53a248: g_zEffectAnim_SourceFileStampCount.
 * Purpose: Tracks source-file stamp records used to reject stale animation
 * ZBD data.
 */
int g_zEffectAnim_SourceFileStampCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-sourcefilestamplist
 * @recoil-artifact defines .data recoil:data:0x53a24c: g_zEffectAnim_SourceFileStampList.
 * Purpose: Owns the temporary source-file stamp table read during animation
 * ZBD loading.
 */
zEffectAnimSourceFileStamp *g_zEffectAnim_SourceFileStampList = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-copynodemode
 * @recoil-artifact defines .data recoil:data:0x4df724: g_zEffectAnim_CopyNodeMode.
 * Purpose: Stores g zEffectAnim CopyNodeMode data used by engine.zeffect.anim_init_data.
 */
int g_zEffectAnim_CopyNodeMode = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-copynodearg1
 * @recoil-artifact defines .data recoil:data:0x4df728: g_zEffectAnim_CopyNodeArg1.
 * Purpose: Stores g zEffectAnim CopyNodeArg1 data used by engine.zeffect.anim_init_data.
 */
int g_zEffectAnim_CopyNodeArg1 = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-copynodearg2
 * @recoil-artifact defines .data recoil:data:0x4df72c: g_zEffectAnim_CopyNodeArg2.
 * Purpose: Stores g zEffectAnim CopyNodeArg2 data used by engine.zeffect.anim_init_data.
 */
int g_zEffectAnim_CopyNodeArg2 = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-forceclonenondynamicroot
 * @recoil-artifact defines .data recoil:data:0x53a240: g_zEffectAnim_ForceCloneNonDynamicRoot.
 * Purpose: Forces loaded animation roots outside dynamic/world classes to be
 * copied before runtime binding.
 */
int g_zEffectAnim_ForceCloneNonDynamicRoot = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-clonecopymode
 * @recoil-artifact defines .data recoil:data:0x4dfaec: g_zEffect_CloneCopyMode.
 * Purpose: Supplies the primary copy mode for runtime effect template clones.
 */
int g_zEffect_CloneCopyMode = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-clonecopychildrenmode
 * @recoil-artifact defines .data recoil:data:0x4dfaf0: g_zEffect_CloneCopyChildrenMode.
 * Purpose: Supplies the child-copy mode for runtime effect template clones.
 */
int g_zEffect_CloneCopyChildrenMode = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-world
 * @recoil-artifact defines .data recoil:data:0x575db8: g_zEffect_World.
 * Purpose: Stores g zEffect World data used by engine.zeffect.stop_cleanup_globals.
 */
zClass_NodePartial *g_zEffect_World = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-defaultgravity
 * @recoil-artifact defines .data recoil:data:0x575dbc: g_zEffect_DefaultGravity.
 * Purpose: Stores the default gravity value restored from animation ZBD state.
 */
float g_zEffect_DefaultGravity = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-conditionalrefposenabled
 * @recoil-artifact defines .data recoil:data:0x575dc0: g_zEffect_ConditionalRefPosEnabled.
 * Purpose: Enables conditional reference-position distance checks.
 */
int g_zEffect_ConditionalRefPosEnabled = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-variantoverrideenabled
 * @recoil-artifact defines .data recoil:data:0x575dc4: g_zEffect_VariantOverrideEnabled.
 * Purpose: Enables the packed variant override once every id is complete.
 */
int g_zEffect_VariantOverrideEnabled = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-conditionalrefposx
 * @recoil-artifact defines .data recoil:data:0x575dc8: g_zEffect_ConditionalRefPosX.
 * Purpose: Stores the conditional reference-position X component.
 */
float g_zEffect_ConditionalRefPosX = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-conditionalrefposy
 * @recoil-artifact defines .data recoil:data:0x575dcc: g_zEffect_ConditionalRefPosY.
 * Purpose: Stores the conditional reference-position Y component.
 */
float g_zEffect_ConditionalRefPosY = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-conditionalrefposz
 * @recoil-artifact defines .data recoil:data:0x575dd0: g_zEffect_ConditionalRefPosZ.
 * Purpose: Stores the conditional reference-position Z component.
 */
float g_zEffect_ConditionalRefPosZ = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-variantoverridepackedids
 * @recoil-artifact defines .data recoil:data:0x575dd4: g_zEffect_VariantOverridePackedIds.
 * Purpose: Stores the completed packed variant ids copied from zTag4Partial.
 */
unsigned int g_zEffect_VariantOverridePackedIds = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-framedeltaremainingsec
 * @recoil-artifact defines .data recoil:data:0x575dd8: g_zEffect_FrameDeltaRemainingSec.
 * Purpose: Stores g zEffect FrameDeltaRemainingSec data used by engine.zeffect.stop_cleanup_globals.
 */
float g_zEffect_FrameDeltaRemainingSec = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-conditionaleffectlevel
 * @recoil-artifact defines .data recoil:data:0x539ea0: g_zEffect_ConditionalEffectLevel.
 * Purpose: Stores the active conditional-chain level for zEffect animation
 * events.
 */
int g_zEffect_ConditionalEffectLevel = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-variantcycleid
 * @recoil-artifact defines .data recoil:data:0x539ea8: g_zEffect_VariantCycleId.
 * Purpose: Stores the current variant cycle id used by zEffect material
 * variant events.
 */
int g_zEffect_VariantCycleId = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-skipstopdelay
 * @recoil-artifact defines .data recoil:data:0x539ea4: g_zEffect_SkipStopDelay.
 * Purpose: Stores g zEffect SkipStopDelay data used by engine.zeffect.stop_cleanup_globals.
 */
int g_zEffect_SkipStopDelay = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-anim-debugframetag
 * @recoil-artifact defines .data recoil:data:0x4df670: g_zEffect_Anim_DebugFrameTag.
 * Purpose: Stores the frame tag assigned by zEffect::SetAnimDebugFrameTag.
 */
int g_zEffect_Anim_DebugFrameTag = -1;
/**
 * Purpose: Stores the resource root node required to load animation ZBD data.
 */
zClass_NodePartial *g_zEffect_ResourceNode = (zClass_NodePartial *)(1);
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-activationrecordtable
 * @recoil-artifact defines .data recoil:data:0x53a2d8: g_zEffectAnim_ActivationRecordTable.
 * Purpose: Owns the queued activation-record table used to save, load, and replay deferred animation activations.
 */
zEffectAnimActivationRecord *g_zEffectAnim_ActivationRecordTable = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-activationrecordcapacity
 * @recoil-artifact defines .data recoil:data:0x53a2dc: g_zEffectAnim_ActivationRecordCapacity.
 * Purpose: Tracks the allocated activation-record slots for the queue table.
 */
int g_zEffectAnim_ActivationRecordCapacity = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-activationrecordcount
 * @recoil-artifact defines .data recoil:data:0x53a2e0: g_zEffectAnim_ActivationRecordCount.
 * Purpose: Tracks the number of queued activation records available for serialization or dispatch.
 */
int g_zEffectAnim_ActivationRecordCount = 0;
/**
 * Purpose: Stores the optional activation-record dispatch callback.
 */
void(__fastcall *g_zEffectAnim_ActivationDispatchCallback)(
    zEffectAnimActivationRecord *record
) = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-activationdispatchtaghigh
 * @recoil-artifact defines .data recoil:data:0x53a2e8: g_zEffectAnim_ActivationDispatchTagHigh.
 * Purpose: Stores the high-byte activation-record tag context.
 */
unsigned int g_zEffectAnim_ActivationDispatchTagHigh = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-recordqueueenabled
 * @recoil-artifact defines .data recoil:data:0x4df9b4: g_zEffectAnim_RecordQueueEnabled.
 * Purpose: Enables allocation of activation records when commands should be queued instead of run immediately.
 */
int g_zEffectAnim_RecordQueueEnabled = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-dispatchenabled
 * @recoil-artifact defines .data recoil:data:0x4df9b8: g_zEffectAnim_DispatchEnabled.
 * Purpose: Enables immediate activation dispatch when queueing is bypassed.
 */
int g_zEffectAnim_DispatchEnabled = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-enablezarregistration
 * @recoil-artifact defines .data recoil:data:0x4df734: g_zEffectAnim_EnableZarRegistration.
 * Purpose: Enables registration of animation save/load ZAR section handlers.
 */
int g_zEffectAnim_EnableZarRegistration = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-zarsectionname-anim
 * @recoil-artifact defines .data recoil:data:0x4df738: g_zEffectAnim_ZarSectionName_Anim.
 * Purpose: Stores the ZAR section token for saved animation records.
 */
char g_zEffectAnim_ZarSectionName_Anim[5] = "Anim";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-zarsectionname-runninganim
 * @recoil-artifact defines .data recoil:data:0x4df740: g_zEffectAnim_ZarSectionName_RunningAnim.
 * Purpose: Stores the ZAR section token for saved running animation records.
 */
char g_zEffectAnim_ZarSectionName_RunningAnim[12] = "RunningAnim";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-zarsectionname-animactivation
 * @recoil-artifact defines .data recoil:data:0x4df74c: g_zEffectAnim_ZarSectionName_AnimActivation.
 * Purpose: Stores the ZAR section token for queued activation records.
 */
char g_zEffectAnim_ZarSectionName_AnimActivation[15] = "AnimActivation";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-activationprereqnodenotfoundfmt
 * @recoil-artifact defines .data recoil:data:0x4df820: g_zEffectAnim_ActivationPrereqNodeNotFoundFmt.
 * Purpose: Stores the activation-prerequisite missing-node diagnostic format
 * used while loading animation ZBD records.
 */
char g_zEffectAnim_ActivationPrereqNodeNotFoundFmt[0x4e] =
    "ACTIVATION_PREREQUISITE error; couldn't find node.\n"
    "  Animation: %s; node: %s\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-corruptanimationloadedfmt
 * @recoil-artifact defines .data recoil:data:0x4df8d4: g_zEffectAnim_CorruptAnimationLoadedFmt.
 * Purpose: Reports an invalid animation entry encountered after loading the
 * animation ZBD.
 */
char g_zEffectAnim_CorruptAnimationLoadedFmt[0x2b] =
    "Corrupt animation loaded:\n  Animation: %s\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-tokenlooping
 * @recoil-artifact defines .data recoil:data:0x4df900: g_zEffectAnim_TokenLooping.
 * Purpose: Names the LOOPING parser field in runtime effect material map rows.
 */
char g_zEffectAnim_TokenLooping[0x8] = "LOOPING";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-tokenspeed
 * @recoil-artifact defines .data recoil:data:0x4df908: g_zEffectAnim_TokenSpeed.
 * Purpose: Names the SPEED parser field in runtime effect material map rows.
 */
char g_zEffectAnim_TokenSpeed[0x6] = "SPEED";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-failedtofindgfxdatafmt
 * @recoil-artifact defines .data recoil:data:0x4df910: g_zEffect_FailedToFindGfxDataFmt.
 * Purpose: Reports effect templates whose model node has no graphics data.
 */
char g_zEffect_FailedToFindGfxDataFmt[0x29] =
    "Failed to find gfx data for effect (%s)\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-nodelookupfailedfmt
 * @recoil-artifact defines .data recoil:data:0x4df93c: g_zEffect_NodeLookupFailedFmt.
 * Purpose: Reports missing model nodes while loading runtime effect templates.
 */
char g_zEffect_NodeLookupFailedFmt[0x2b] =
    "%s(%d): Failed to find node (%s) for (%s)\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-tokenmaps
 * @recoil-artifact defines .data recoil:data:0x4df968: g_zEffect_TokenMaps.
 * Purpose: Names the MAPS parser field in runtime effect template rows.
 */
char g_zEffect_TokenMaps[0x5] = "MAPS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-readfieldfailedfmt
 * @recoil-artifact defines .data recoil:data:0x4df970: g_zEffect_ReadFieldFailedFmt.
 * Purpose: Reports failed zReader effect data loads from zeff_init.c.
 */
char g_zEffect_ReadFieldFailedFmt[0x1b] = "%s(%d): Failed to read %s\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-sourcefile-zeffinitc
 * @recoil-artifact defines .data recoil:data:0x4df98c: g_zEffect_SourceFile_ZeffInitC.
 * Purpose: Stores the original zeff_init.c diagnostic source path.
 */
char g_zEffect_SourceFile_ZeffInitC[0x28] =
    "D:\\Proj\\GameZRecoil\\zEffect\\zeff_init.c";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-activationsectionnamefmt
 * @recoil-artifact defines .data recoil:data:0x4df9bc: g_zEffectAnim_ActivationSectionNameFmt.
 * Purpose: Formats queued activation-record ZAR section names.
 */
char g_zEffectAnim_ActivationSectionNameFmt[0xf] = "Activation%04d";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-restorenodefmt
 * @recoil-artifact defines .data recoil:data:0x4df9cc: g_zEffectAnim_RestoreNodeFmt.
 * Purpose: Reports tracked-node restoration during animation save loading.
 */
char g_zEffectAnim_RestoreNodeFmt[0x14] = "Restore node: %s %d";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-stateinvalidmsg
 * @recoil-artifact defines .data recoil:data:0x4df9e0: g_zEffectAnim_StateInvalidMsg.
 * Purpose: Reports restored activation records forced to ANIM_STATE_INVALID.
 */
char g_zEffectAnim_StateInvalidMsg[0x24] =
    "Set anim_state = ANIM_STATE_INVALID";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-resetfunctionname
 * @recoil-artifact defines .data recoil:data:0x4dfa04: g_zEffectAnim_ResetFunctionName.
 * Purpose: Names the animation reset diagnostic path.
 */
char g_zEffectAnim_ResetFunctionName[0xe] = "zEffAnimReset";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-stateexecutedmsg
 * @recoil-artifact defines .data recoil:data:0x4dfa14: g_zEffectAnim_StateExecutedMsg.
 * Purpose: Reports restored activation records forced to ANIM_STATE_EXECUTED.
 */
char g_zEffectAnim_StateExecutedMsg[0x25] =
    "Set anim_state = ANIM_STATE_EXECUTED";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-processactivationrecordname
 * @recoil-artifact defines .data recoil:data:0x4dfa3c: g_zEffectAnim_ProcessActivationRecordName.
 * Purpose: Names the queued activation-record processing diagnostic path.
 */
char g_zEffectAnim_ProcessActivationRecordName[0x1c] =
    "zEffProcessActivationRecord";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-sourcefile-zeffanimsavec
 * @recoil-artifact defines .data recoil:data:0x4dfa58: g_zEffect_SourceFile_ZeffAnimSaveC.
 * Purpose: Stores the original zeff_anim_save.c diagnostic source path.
 */
char g_zEffect_SourceFile_ZeffAnimSaveC[0x2d] =
    "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_save.c";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-resetactivationrecordfmt
 * @recoil-artifact defines .data recoil:data:0x4dfa88: g_zEffectAnim_ResetActivationRecordFmt.
 * Purpose: Reports reset of queued activation records during save loading.
 */
char g_zEffectAnim_ResetActivationRecordFmt[0x1e] =
    "zEffResetActivationRecord: %s";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-activationsectionname0
 * @recoil-artifact defines .data recoil:data:0x4dfaa8: g_zEffectAnim_ActivationSectionName0.
 * Purpose: Names the first activation-record ZAR section.
 */
char g_zEffectAnim_ActivationSectionName0[0xf] = "Activation0000";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-runningsectionnamefmt
 * @recoil-artifact defines .data recoil:data:0x4dfab8: g_zEffectAnim_RunningSectionNameFmt.
 * Purpose: Formats running-animation ZAR section names.
 */
char g_zEffectAnim_RunningSectionNameFmt[0xc] = "Running%03d";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-animsectionnamefmt
 * @recoil-artifact defines .data recoil:data:0x4dfac4: g_zEffectAnim_AnimSectionNameFmt.
 * Purpose: Formats non-running animation ZAR section names.
 */
char g_zEffectAnim_AnimSectionNameFmt[0x9] = "Anim%04d";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffect-stringnone
 * @recoil-artifact defines .data recoil:data:0x4dfad0: g_zEffect_StringNone.
 * Purpose: Stores the empty animation section sentinel name.
 */
char g_zEffect_StringNone[0x5] = "None";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.g-zeffectanim-resettracefmt
 * @recoil-artifact defines .data recoil:data:0x4dfad8: g_zEffectAnim_ResetTraceFmt.
 * Purpose: Reports reset of animation entries during save loading.
 */
char g_zEffectAnim_ResetTraceFmt[0x12] = "zEffAnimReset: %s";
}

extern char g_EffectsZrdNodeName[8];

namespace {
const unsigned int kEffectAnimWorldChildAttachedFlag = 0x00000100u;
const float kEmitterLoopTriggerClampValue = 86400.0f;
const float kEffectAnimStopDelaySkipBias = -0.01f;
const float kEffectAnimActivationSentinel = -99.0f;
const float kEffectAnimActivationSentinelTolerance = 0.1f;
const float kEffectAnimVelocityEpsilon = 0.01f;
const short kEffectAnimResetScratchRefIndex = -200;
const short kEffectAnimBoundNodeRefIndex = -100;
const char *kZeffAnimRunSourceFile = "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_run.c";
} // namespace


namespace zEffect {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.setconditionalrefpos
 * @recoil-artifact defines .text recoil:function:0x458af0: zEffect::SetConditionalRefPos.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeffect.cpp.
 * Purpose: store the conditional reference position used by zEffect
 * conditional event tests.
 */
void __fastcall SetConditionalRefPos(
    const zVec3 *position
) {
    g_zEffect_ConditionalRefPosX = position->x;
    g_zEffect_ConditionalRefPosY = position->y;
    g_zEffect_ConditionalRefPosZ = position->z;
    g_zEffect_ConditionalRefPosEnabled = 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.setvariantoverridepackedidsifcomplete
 * @recoil-artifact defines .text recoil:function:0x458b20: zEffect::SetVariantOverridePackedIdsIfComplete.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeffect.cpp.
 * Purpose: enable the packed variant override only when the active id bytes are
 * populated.
 */
void __fastcall SetVariantOverridePackedIdsIfComplete(
    const zTag4Partial *packedIds
) {
    const unsigned char count = packedIds->count;
    if (count == 0) {
        return;
    }

    for (int i = 0; i < count; ++i) {
        if (packedIds->tags[i] == 0xff) {
            return;
        }
    }

    unsigned int packedValue = 0;
    memcpy(
        &packedValue,
        packedIds,
        sizeof(packedValue)
    );
    g_zEffect_VariantOverrideEnabled = 1;
    g_zEffect_VariantOverridePackedIds = packedValue;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.tickresetdelayontimer
 * @recoil-artifact defines .text recoil:function:0x458b50: zEffect::TickResetDelayOnTimer.
 * Purpose: Advance timer-gated reset delay and clear transform/velocity when it expires.
 */
float __fastcall TickResetDelayOnTimer(
    zEffectAnimEntry *self,
    float deltaSec
) {
    if (self->activationMode == 1 || self->activationMode == 2) {
        self->activationCountdown -= deltaSec;
        if (self->activationCountdown <= 0.0f) {
            zEffectAnim::SetTransformRotAndVelocity(
                self,
                0,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f
            );
        }
    }

    return self->activationCountdown;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.tickresetdelayonhit
 * @recoil-artifact defines .text recoil:function:0x458bb0: zEffect::TickResetDelayOnHit.
 * Purpose: Advance hit-gated reset delay and clear transform/velocity when it expires.
 */
int __fastcall TickResetDelayOnHit(
    zEffectAnimEntry *self,
    zClass_NodePartial *hitNode,
    int,
    float damageAmount
) {
    if ((hitNode->listCountA & 0x200) == 0 &&
        (self->activationMode == 0 || self->activationMode == 2)) {
        self->activationCountdown -= damageAmount;
        if (self->activationCountdown <= 0.0f) {
            zEffectAnim::SetTransformRotAndVelocity(
                self,
                0,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f,
                0.0f
            );
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.updatebeamnodebetweenpoints
 * @recoil-artifact defines .text recoil:function:0x458c10: zEffect::UpdateBeamNodeBetweenPoints.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_detach.c.
 * Purpose: position, orient, scale, and return the length of a beam node
 * spanning two world points.
 */
float __fastcall UpdateBeamNodeBetweenPoints(
    zClass_NodePartial *obj3d,
    const zVec3 *srcPos,
    const zVec3 *destPos
) {
    if (obj3d == 0) {
        return 0.0f;
    }

    zClass_Object3D::gwObject3DSetPosition(
        obj3d,
        srcPos->x,
        srcPos->y,
        srcPos->z
    );

    zVec3 angles = {0};
    zMath::Vec3DirectionAnglesBetweenPoints(
        srcPos,
        destPos,
        &angles
    );
    zClass_Object3D::gwObject3DSetRotation(
        obj3d,
        angles.x,
        angles.y,
        0.0f
    );

    zVec3 scale = {0};
    zClass_Object3D::gwObject3DGetScale(
        obj3d,
        &scale.x,
        &scale.y,
        &scale.z
    );

    const float dx = destPos->x - srcPos->x;
    const float dy = destPos->y - srcPos->y;
    const float dz = destPos->z - srcPos->z;
    const float lengthSq = dx * dx + dy * dy + dz * dz;
    int lengthBits = 0;
    memcpy(
        &lengthBits,
        &lengthSq,
        sizeof(lengthBits)
    );
    lengthBits = (lengthBits >> 1) + 0x1fc00000;
    float length = 0.0f;
    memcpy(
        &length,
        &lengthBits,
        sizeof(length)
    );

    zClass_Object3D::gwObject3DSetScale(
        obj3d,
        scale.x,
        scale.y,
        length
    );
    return length;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.updatebeamnodebetweenfractions
 * @recoil-artifact defines .text recoil:function:0x458ce0: zEffect::UpdateBeamNodeBetweenFractions.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_detach.c.
 * Purpose: position, orient, scale, and return the length of a beam node
 * spanning two fractional points on a world-space segment.
 */
float __fastcall UpdateBeamNodeBetweenFractions(
    zClass_NodePartial *obj3d,
    const zVec3 *srcPos,
    float t0,
    const zVec3 *destPos,
    float t1
) {
    if (obj3d == 0) {
        return 0.0f;
    }

    const zVec3 delta = {destPos->x - srcPos->x, destPos->y - srcPos->y, destPos->z - srcPos->z};
    const zVec3 start = {srcPos->x + delta.x * t0,
        srcPos->y + delta.y * t0,
        srcPos->z + delta.z * t0};
    const zVec3 end = {srcPos->x + delta.x * t1,
        srcPos->y + delta.y * t1,
        srcPos->z + delta.z * t1};

    zClass_Object3D::gwObject3DSetPosition(
        obj3d,
        start.x,
        start.y,
        start.z
    );

    zVec3 angles = {0};
    zMath::Vec3DirectionAnglesBetweenPoints(
        srcPos,
        destPos,
        &angles
    );
    zClass_Object3D::gwObject3DSetRotation(
        obj3d,
        angles.x,
        angles.y,
        0.0f
    );

    zVec3 scale = {0};
    zClass_Object3D::gwObject3DGetScale(
        obj3d,
        &scale.x,
        &scale.y,
        &scale.z
    );

    const float dx = end.x - start.x;
    const float dy = end.y - start.y;
    const float dz = end.z - start.z;
    const float lengthSq = dx * dx + dy * dy + dz * dz;
    int lengthBits = 0;
    memcpy(
        &lengthBits,
        &lengthSq,
        sizeof(lengthBits)
    );
    lengthBits = (lengthBits >> 1) + 0x1fc00000;
    float length = 0.0f;
    memcpy(
        &length,
        &lengthBits,
        sizeof(length)
    );

    zClass_Object3D::gwObject3DSetScale(
        obj3d,
        scale.x,
        scale.y,
        length
    );
    return length;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handlesamplerefoffsetevent
 * @recoil-artifact defines .text recoil:function:0x458e10: zEffect::HandleSampleRefOffsetEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: play a referenced sound sample either directly or at a referenced
 * node world position plus the event offset.
 */
int __fastcall HandleSampleRefOffsetEvent(
    zEffectAnimEntry *self,
    zEffectAnimRefOffsetEvent *event
) {
    zSndSample *const sample = self->sampleRefList[event->refIndex].sample;
    if (event->nodeRefIndex <= 0) {
        sample->PlayA3DSimple(1.0f);
        return 2;
    }

    zVec3 worldPosition = {0};
    gwNode::GetWorldPosition(
        self->nodeRefList[event->nodeRefIndex].node,
        &worldPosition
    );
    worldPosition.x += event->offsetX;
    worldPosition.y += event->offsetY;
    worldPosition.z += event->offsetZ;
    sample->PlayA3D(
        &worldPosition,
        1.0f,
        0
    );
    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handleeffecttemplateoffsetevent
 * @recoil-artifact defines .text recoil:function:0x458eb0: zEffect::HandleEffectTemplateOffsetEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: spawn a referenced effect template at a node or reset-scratch
 * position plus the event offset.
 */
int __fastcall HandleEffectTemplateOffsetEvent(
    zEffectAnimEntry *self,
    zEffectAnimRefOffsetEvent *event
) {
    zVec3 worldPosition = {0};
    zClass_NodePartial *node = 0;

    if (event->nodeRefIndex > 0) {
        node = self->nodeRefList[event->nodeRefIndex].node;
    } else if (event->nodeRefIndex == -200) {
        node = (zClass_NodePartial *)((unsigned int)(self->resetScratch[0]));
        memcpy(
            &worldPosition.x,
            &self->resetScratch[1],
            sizeof(worldPosition.x)
        );
        memcpy(
            &worldPosition.y,
            &self->resetScratch[2],
            sizeof(worldPosition.y)
        );
        memcpy(
            &worldPosition.z,
            &self->resetScratch[3],
            sizeof(worldPosition.z)
        );
    }

    if (node != 0) {
        gwNode::TransformPoint(
            node,
            &worldPosition
        );
    }

    worldPosition.x += event->offsetX;
    worldPosition.y += event->offsetY;
    worldPosition.z += event->offsetZ;
    SpawnRuntimeInstanceAt(
        self->effectTemplateRefList[event->refIndex].templateIndex,
        &worldPosition
    );
    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handlesoundevent
 * @recoil-artifact defines .text recoil:function:0x458f70: zEffect::HandleSoundEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: activate, attach, detach, and position a runtime sound reference.
 */
int __fastcall HandleSoundEvent(
    zEffectAnimEntry *self,
    zEffectAnimSoundEvent *event
) {
    if (event->soundRefIndex <= 0) {
        event->soundRefIndex = zEffectAnim::FindOrCreateSoundRef(
            self,
            event->soundName
        );
        if (event->soundRefIndex <= 0) {
            return 2;
        }
    }

    zEffectAnimRuntimeNodeRef *const soundRef = &self->soundRefList[event->soundRefIndex];
    zClass_Class::gwNodeSetActive(
        soundRef->runtimeNode,
        event->activeState
    );

    if (event->activeState == 1) {
        if (soundRef->isAttached == 0) {
            zClass_World::AddSound(
                g_zEffect_World,
                soundRef->runtimeNode
            );
            soundRef->isAttached = 1;
        }
    } else if (soundRef->isAttached != 0) {
        zClass_World::RemoveSound(
            g_zEffect_World,
            soundRef->runtimeNode
        );
        soundRef->isAttached = 0;
    }

    if ((event->fieldMask & 0x01) != 0) {
        zClass_Sound::gwSoundSetPosition(
            soundRef->runtimeNode,
            event->offsetX,
            event->offsetY,
            event->offsetZ
        );
    }

    if ((event->fieldMask & 0x02) != 0 && event->parentNodeRefIndex > 0) {
        zVec3 worldPosition = {0};
        gwNode::GetWorldPosition(
            self->nodeRefList[event->parentNodeRefIndex].node,
            &worldPosition
        );
        worldPosition.x += event->offsetX;
        worldPosition.y += event->offsetY;
        worldPosition.z += event->offsetZ;
        zClass_Sound::gwSoundSetPosition(
            soundRef->runtimeNode,
            worldPosition.x,
            worldPosition.y,
            worldPosition.z
        );
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handlelightevent
 * @recoil-artifact defines .text recoil:function:0x459080: zEffect::HandleLightEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: activate a runtime light reference and apply serialized light
 * state fields selected by the event mask.
 */
int __fastcall HandleLightEvent(
    zEffectAnimEntry *self,
    zEffectAnimLightEvent *event
) {
    if (event->lightRefIndex <= 0) {
        event->lightRefIndex = zEffectAnim::FindOrCreateLightRef(
            self,
            event->lightName
        );
        if (event->lightRefIndex <= 0) {
            return 2;
        }
    }

    zEffectAnimRuntimeNodeRef *const lightRef = &self->lightRefList[event->lightRefIndex];
    zClass_NodePartial *const lightNode = lightRef->runtimeNode;
    zClass_Class::gwNodeSetActive(
        lightNode,
        event->activeState
    );

    if (event->activeState == 1) {
        if (lightRef->isAttached == 0) {
            zClass_World::AddLight(
                g_zEffect_World,
                lightNode
            );
            lightRef->isAttached = 1;
        }
    } else if (lightRef->isAttached != 0) {
        zClass_World::RemoveLight(
            g_zEffect_World,
            lightNode
        );
        lightRef->isAttached = 0;
    }

    if (event->mode == 0) {
        zClass_Light::gwLightSetPointMode(lightNode);
    } else {
        zClass_Light::gwLightSetDirectionalMode(lightNode);
    }

    if ((event->fieldMask & 0x01) != 0) {
        zClass_Light::gwLightSetPosition(
            lightNode,
            event->basisOrColorX,
            event->basisOrColorY,
            event->basisOrColorZ
        );
    }

    if ((event->fieldMask & 0x02) != 0) {
        zVec3 worldPosition = {0};
        zClass_NodePartial *basisNode = 0;

        if (event->basisNodeRefIndex > 0) {
            basisNode = self->nodeRefList[event->basisNodeRefIndex].node;
        } else if (event->basisNodeRefIndex == -200) {
            basisNode = (zClass_NodePartial *)((unsigned int)(self->resetScratch[0]));
            memcpy(
                &worldPosition.x,
                &self->resetScratch[1],
                sizeof(worldPosition.x)
            );
            memcpy(
                &worldPosition.y,
                &self->resetScratch[2],
                sizeof(worldPosition.y)
            );
            memcpy(
                &worldPosition.z,
                &self->resetScratch[3],
                sizeof(worldPosition.z)
            );
        }

        if (basisNode != 0) {
            gwNode::TransformPoint(
                basisNode,
                &worldPosition
            );
        }

        worldPosition.x += event->basisOrColorX;
        worldPosition.y += event->basisOrColorY;
        worldPosition.z += event->basisOrColorZ;
        zClass_Light::gwLightSetPosition(
            lightNode,
            worldPosition.x,
            worldPosition.y,
            worldPosition.z
        );
    }

    if ((event->fieldMask & 0x04) != 0) {
        zClass_Light::gwLightSetRotation(
            lightNode,
            event->positionX,
            event->positionY,
            event->positionZ
        );
    }

    if ((event->fieldMask & 0x08) != 0) {
        zClass_Light::gwLightSetRange(
            lightNode,
            event->rangeInner,
            event->rangeOuter
        );
    }

    if ((event->fieldMask & 0x10) != 0) {
        zClass_Light::gwLightSetSpecularColor(
            lightNode,
            event->specularR,
            event->specularG,
            event->specularB
        );
    }

    if ((event->fieldMask & 0x20) != 0) {
        zClass_Light::gwLightSetIntensity(
            lightNode,
            event->intensity
        );
    }

    if ((event->fieldMask & 0x40) != 0) {
        zClass_Light::gwLightSetFalloff(
            lightNode,
            event->falloff
        );
    }

    if ((event->fieldMask & 0x80) != 0) {
        zClass_Light::gwLightSetConeAngle(
            lightNode,
            event->coneAngleBits
        );
    }

    if ((event->fieldMask & 0x100) != 0) {
        zClass_Light::gwLightSetParam(
            lightNode,
            event->param
        );
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handlelightanimevent
 * @recoil-artifact defines .text recoil:function:0x459280: zEffect::HandleLightAnimEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: animate a light reference's range and specular color over a timed
 * event slice.
 */
int __fastcall HandleLightAnimEvent(
    zEffectAnimEntry *self,
    zEffectAnimSurfaceRuntime *sequenceRuntime,
    zEffectLightRangeSpecularAnimEvent *animEvent
) {
    if (animEvent->lightRefIndex <= 0) {
        animEvent->lightRefIndex = zEffectAnim::FindOrCreateLightRef(
            self,
            animEvent->lightName
        );
        if (animEvent->lightRefIndex <= 0) {
            return 2;
        }
    }

    zEffectAnimRuntimeNodeRef *const lightRef = &self->lightRefList[animEvent->lightRefIndex];
    if (sequenceRuntime->runState == 0) {
        animEvent->currentRangeInner = animEvent->initialRangeInner;
        animEvent->currentRangeOuter = animEvent->initialRangeOuter;
        animEvent->currentSpecularR = animEvent->initialSpecularR;
        animEvent->currentSpecularG = animEvent->initialSpecularG;
        animEvent->currentSpecularB = animEvent->initialSpecularB;
    }

    float stepSec = g_zEffect_FrameDeltaRemainingSec;
    if (sequenceRuntime->eventElapsedSec > animEvent->durationSec) {
        stepSec = g_zEffect_FrameDeltaRemainingSec -
                  (sequenceRuntime->eventElapsedSec - animEvent->durationSec);
    }

    float lightRangeInner = 0.0f;
    float lightRangeOuter = 0.0f;
    zClass_Light::gwLightGetRange(
        lightRef->runtimeNode,
        &lightRangeInner,
        &lightRangeOuter
    );

    lightRangeInner += stepSec * animEvent->currentRangeInner;
    lightRangeOuter += stepSec * animEvent->currentRangeOuter;
    if (lightRangeInner < 0.0f) {
        lightRangeInner = 1.0f;
    }
    if (lightRangeOuter < lightRangeInner) {
        lightRangeOuter = lightRangeInner + 1.0f;
    }

    animEvent->currentRangeInner += stepSec * animEvent->rangeInnerDelta;
    animEvent->currentRangeOuter += stepSec * animEvent->rangeOuterDelta;
    zClass_Light::gwLightSetRange(
        lightRef->runtimeNode,
        lightRangeInner,
        lightRangeOuter
    );

    float specularR = 0.0f;
    float specularG = 0.0f;
    float specularB = 0.0f;
    zClass_Light::gwLightGetSpecularColor(
        lightRef->runtimeNode,
        &specularR,
        &specularG,
        &specularB
    );

    specularR += stepSec * animEvent->currentSpecularR;
    specularG += stepSec * animEvent->currentSpecularG;
    specularB += stepSec * animEvent->currentSpecularB;

    animEvent->currentSpecularR += stepSec * animEvent->specularRDelta;
    animEvent->currentSpecularG += stepSec * animEvent->specularGDelta;
    animEvent->currentSpecularB += stepSec * animEvent->specularBDelta;

    if (specularR > 1.0f) {
        specularR = 1.0f;
    } else if (specularR < 0.0f) {
        specularR = 0.0f;
    }

    if (specularG > 1.0f) {
        specularG = 1.0f;
    } else if (specularG < 0.0f) {
        specularG = 0.0f;
    }

    if (specularB > 1.0f) {
        specularB = 1.0f;
    } else if (specularB < 0.0f) {
        specularB = 0.0f;
    }

    zClass_Light::gwLightSetSpecularColor(
        lightRef->runtimeNode,
        specularR,
        specularG,
        specularB
    );

    g_zEffect_FrameDeltaRemainingSec -= stepSec;
    return sequenceRuntime->eventElapsedSec > animEvent->durationSec ? 2 : 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handlefogevent
 * @recoil-artifact defines .text recoil:function:0x459510: zEffect::HandleFogEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: copy selected fog state fields into the pending world fog settings.
 */
int __fastcall HandleFogEvent(
    zEffectAnimEntry * /*self*/,
    zEffectFogEvent *event
) {
    if ((event->flags & 0x01) != 0) {
        zClass_World::SetPendingFogState(
            g_zEffect_World,
            event->fogState
        );
    }

    if ((event->flags & 0x02) != 0) {
        zClass_World::SetPendingFogColorRgb01(
            g_zEffect_World,
            event->fogColorR,
            event->fogColorG,
            event->fogColorB
        );
    }

    if ((event->flags & 0x04) != 0) {
        zClass_World::SetPendingFogAltitudeRange(
            g_zEffect_World,
            event->fogAltitudeMin,
            event->fogAltitudeMax
        );
    }

    if ((event->flags & 0x08) != 0) {
        zClass_World::SetPendingFogRange(
            g_zEffect_World,
            event->fogRangeStart,
            event->fogRangeEnd
        );
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handlecameraparamsevent
 * @recoil-artifact defines .text recoil:function:0x459580: zEffect::HandleCameraParamsEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: apply immediate near/far clip, clip distance, FOV, and viewport
 * camera parameters from an event mask.
 */
int __fastcall HandleCameraParamsEvent(
    zEffectAnimEntry *self,
    zEffectAnimSurfaceRuntime *sequenceRuntime,
    zEffectCameraEvent *event
) {
    if (self == 0 || sequenceRuntime == 0 || event == 0 || event->targetNodeRefIndex < 0) {
        return 2;
    }

    zClass_NodePartial *const node = self->nodeRefList[event->targetNodeRefIndex].node;
    float primaryValue = 0.0f;
    float secondaryValue = 0.0f;

    if ((event->flags & 0x01) != 0) {
        zClass_Camera::gwCameraGetNearFarClip(
            node,
            &primaryValue,
            &secondaryValue
        );
        zClass_Camera::gwCameraSetNearFarClip(
            node,
            event->nearClip,
            secondaryValue
        );
    }

    if ((event->flags & 0x02) != 0) {
        zClass_Camera::gwCameraGetNearFarClip(
            node,
            &primaryValue,
            &secondaryValue
        );
        zClass_Camera::gwCameraSetNearFarClip(
            node,
            primaryValue,
            event->farClip
        );
    }

    if ((event->flags & 0x04) != 0) {
        zClass_Camera::gwCameraSetClipDistance(
            node,
            event->clipDistance
        );
    }

    if ((event->flags & 0x08) != 0) {
        zClass_Camera::gwCameraGetFOV(
            node,
            &primaryValue,
            &secondaryValue
        );
        zClass_Camera::gwCameraSetFOV(
            node,
            event->fovPrimary,
            secondaryValue
        );
    }

    if ((event->flags & 0x10) != 0) {
        zClass_Camera::gwCameraGetFOV(
            node,
            &primaryValue,
            &secondaryValue
        );
        zClass_Camera::gwCameraSetFOV(
            node,
            primaryValue,
            event->fovSecondary
        );
    }

    if ((event->flags & 0x20) != 0) {
        zClass_Camera::gwCameraGetViewport(
            node,
            &primaryValue,
            &secondaryValue
        );
        zClass_Camera::gwCameraSetViewport(
            node,
            event->viewportPrimary,
            secondaryValue
        );
    }

    if ((event->flags & 0x40) != 0) {
        zClass_Camera::gwCameraGetViewport(
            node,
            &primaryValue,
            &secondaryValue
        );
        zClass_Camera::gwCameraSetViewport(
            node,
            primaryValue,
            event->viewportSecondary
        );
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.animatecameraparamsovertime
 * @recoil-artifact defines .text recoil:function:0x4596c0: zEffect::AnimateCameraParamsOverTime.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: interpolate selected camera parameters across the current timed
 * event slice and clamp to final values when complete.
 */
int __fastcall AnimateCameraParamsOverTime(
    zEffectAnimEntry *self,
    zEffectAnimSurfaceRuntime *sequenceRuntime,
    zEffectCameraAnimEvent *animEvent
) {
    if (self == 0 || sequenceRuntime == 0 || animEvent == 0 || animEvent->targetNodeRefIndex < 0) {
        return 2;
    }

    zClass_NodePartial *const node = self->nodeRefList[animEvent->targetNodeRefIndex].node;
    float primaryValue = 0.0f;
    float secondaryValue = 0.0f;

    if (sequenceRuntime->runState == 0) {
        if ((animEvent->flags & 0x01) != 0) {
            zClass_Camera::gwCameraGetNearFarClip(
                node,
                &primaryValue,
                &secondaryValue
            );
            zClass_Camera::gwCameraSetNearFarClip(
                node,
                animEvent->nearClipStart,
                secondaryValue
            );
        }

        if ((animEvent->flags & 0x02) != 0) {
            zClass_Camera::gwCameraGetNearFarClip(
                node,
                &primaryValue,
                &secondaryValue
            );
            zClass_Camera::gwCameraSetNearFarClip(
                node,
                primaryValue,
                animEvent->farClipStart
            );
        }

        if ((animEvent->flags & 0x04) != 0) {
            zClass_Camera::gwCameraSetClipDistance(
                node,
                animEvent->clipDistanceStart
            );
        }

        if ((animEvent->flags & 0x08) != 0) {
            zClass_Camera::gwCameraGetFOV(
                node,
                &primaryValue,
                &secondaryValue
            );
            zClass_Camera::gwCameraSetFOV(
                node,
                animEvent->fovPrimaryStart,
                secondaryValue
            );
        }

        if ((animEvent->flags & 0x10) != 0) {
            zClass_Camera::gwCameraGetFOV(
                node,
                &primaryValue,
                &secondaryValue
            );
            zClass_Camera::gwCameraSetFOV(
                node,
                primaryValue,
                animEvent->fovSecondaryStart
            );
        }

        if ((animEvent->flags & 0x20) != 0) {
            zClass_Camera::gwCameraGetViewport(
                node,
                &primaryValue,
                &secondaryValue
            );
            zClass_Camera::gwCameraSetViewport(
                node,
                animEvent->viewportPrimaryStart,
                secondaryValue
            );
        }

        if ((animEvent->flags & 0x40) != 0) {
            zClass_Camera::gwCameraGetViewport(
                node,
                &primaryValue,
                &secondaryValue
            );
            zClass_Camera::gwCameraSetViewport(
                node,
                primaryValue,
                animEvent->viewportSecondaryStart
            );
        }
    }

    float stepSec = g_zEffect_FrameDeltaRemainingSec;
    if (sequenceRuntime->eventElapsedSec > animEvent->endTime) {
        stepSec = g_zEffect_FrameDeltaRemainingSec -
                  (sequenceRuntime->eventElapsedSec - animEvent->endTime);
    }

    if ((animEvent->flags & 0x01) != 0) {
        zClass_Camera::gwCameraGetNearFarClip(
            node,
            &primaryValue,
            &secondaryValue
        );
        zClass_Camera::gwCameraSetNearFarClip(
            node,
            primaryValue + animEvent->nearClipRate * stepSec,
            secondaryValue
        );
    }

    if ((animEvent->flags & 0x02) != 0) {
        zClass_Camera::gwCameraGetNearFarClip(
            node,
            &primaryValue,
            &secondaryValue
        );
        zClass_Camera::gwCameraSetNearFarClip(
            node,
            primaryValue,
            secondaryValue + animEvent->farClipRate * stepSec
        );
    }

    if ((animEvent->flags & 0x04) != 0) {
        zClass_Camera::gwCameraGetClipDistance(
            node,
            &primaryValue
        );
        zClass_Camera::gwCameraSetClipDistance(
            node,
            primaryValue + animEvent->clipDistanceRate * stepSec
        );
    }

    if ((animEvent->flags & 0x08) != 0) {
        zClass_Camera::gwCameraGetFOV(
            node,
            &primaryValue,
            &secondaryValue
        );
        zClass_Camera::gwCameraSetFOV(
            node,
            primaryValue + animEvent->fovPrimaryRate * stepSec,
            secondaryValue
        );
    }

    if ((animEvent->flags & 0x10) != 0) {
        zClass_Camera::gwCameraGetFOV(
            node,
            &primaryValue,
            &secondaryValue
        );
        zClass_Camera::gwCameraSetFOV(
            node,
            primaryValue,
            secondaryValue + animEvent->fovSecondaryRate * stepSec
        );
    }

    if ((animEvent->flags & 0x20) != 0) {
        zClass_Camera::gwCameraGetViewport(
            node,
            &primaryValue,
            &secondaryValue
        );
        zClass_Camera::gwCameraSetViewport(
            node,
            primaryValue + animEvent->viewportPrimaryRate * stepSec,
            secondaryValue
        );
    }

    if ((animEvent->flags & 0x40) != 0) {
        zClass_Camera::gwCameraGetViewport(
            node,
            &primaryValue,
            &secondaryValue
        );
        zClass_Camera::gwCameraSetViewport(
            node,
            primaryValue,
            secondaryValue + animEvent->viewportSecondaryRate * stepSec
        );
    }

    g_zEffect_FrameDeltaRemainingSec -= stepSec;
    if (sequenceRuntime->eventElapsedSec <= animEvent->endTime) {
        return 1;
    }

    if ((animEvent->flags & 0x01) != 0) {
        zClass_Camera::gwCameraGetNearFarClip(
            node,
            &primaryValue,
            &secondaryValue
        );
        zClass_Camera::gwCameraSetNearFarClip(
            node,
            animEvent->nearClipEnd,
            secondaryValue
        );
    }

    if ((animEvent->flags & 0x02) != 0) {
        zClass_Camera::gwCameraGetNearFarClip(
            node,
            &primaryValue,
            &secondaryValue
        );
        zClass_Camera::gwCameraSetNearFarClip(
            node,
            primaryValue,
            animEvent->farClipEnd
        );
    }

    if ((animEvent->flags & 0x04) != 0) {
        zClass_Camera::gwCameraSetClipDistance(
            node,
            animEvent->clipDistanceEnd
        );
    }

    if ((animEvent->flags & 0x08) != 0) {
        zClass_Camera::gwCameraGetFOV(
            node,
            &primaryValue,
            &secondaryValue
        );
        zClass_Camera::gwCameraSetFOV(
            node,
            animEvent->fovPrimaryEnd,
            secondaryValue
        );
    }

    if ((animEvent->flags & 0x10) != 0) {
        zClass_Camera::gwCameraGetFOV(
            node,
            &primaryValue,
            &secondaryValue
        );
        zClass_Camera::gwCameraSetFOV(
            node,
            primaryValue,
            animEvent->fovSecondaryEnd
        );
    }

    if ((animEvent->flags & 0x20) != 0) {
        zClass_Camera::gwCameraGetViewport(
            node,
            &primaryValue,
            &secondaryValue
        );
        zClass_Camera::gwCameraSetViewport(
            node,
            animEvent->viewportPrimaryEnd,
            secondaryValue
        );
    }

    if ((animEvent->flags & 0x40) != 0) {
        zClass_Camera::gwCameraGetViewport(
            node,
            &primaryValue,
            &secondaryValue
        );
        zClass_Camera::gwCameraSetViewport(
            node,
            primaryValue,
            animEvent->viewportSecondaryEnd
        );
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handlerotationevent
 * @recoil-artifact defines .text recoil:function:0x459ae0: zEffect::HandleRotationEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: set or translate target node rotation or camera position with
 * optional basis-node rotation composition.
 */
int __fastcall HandleRotationEvent(
    zEffectAnimEntry *self,
    zEffectTransformEvent *event
) {
    zClass_NodePartial *const targetNode = self->nodeRefList[event->targetNodeRefIndex].node;
    if (targetNode->classId == 1) {
        if ((event->flags & 0x01) != 0) {
            zClass_Camera::gwCameraTranslate(
                targetNode,
                event->vecX,
                event->vecY,
                event->vecZ
            );
        } else {
            zClass_Camera::gwCameraSetPosition(
                targetNode,
                event->vecX,
                event->vecY,
                event->vecZ
            );
        }
    } else if (targetNode->classId == 5) {
        if ((event->flags & 0x02) != 0 || (event->flags & 0x04) != 0) {
            zClass_NodePartial *basisNode = 0;
            zVec3 basisAngles = {0};

            if (event->basisNodeRefIndex > 0) {
                basisNode = self->nodeRefList[event->basisNodeRefIndex].node;
            } else if (event->basisNodeRefIndex == kEffectAnimResetScratchRefIndex) {
                basisNode = (zClass_NodePartial *)((unsigned int)(self->resetScratch[0]));
            }

            if (basisNode != 0) {
                if ((event->flags & 0x02) != 0) {
                    zClass_Object3D::gwObject3DGetRotation(
                        basisNode,
                        &basisAngles.x,
                        &basisAngles.y,
                        &basisAngles.z
                    );
                } else {
                    zMat4x3 matrix;
                    zMath::MatStackPushPtr((float *)(&matrix));
                    zMath::MatLoadIdentity();
                    gwNode::BuildNodeToAncestorMatrix(
                        basisNode,
                        1
                    );
                    zMath_Mat_ExtractEulerAngles(
                        &matrix,
                        &basisAngles
                    );
                    zMath::MatStackPopPtr();
                }
            }

            zClass_Object3D::gwObject3DSetRotation(
                targetNode,
                event->vecX + basisAngles.x,
                event->vecY + basisAngles.y,
                event->vecZ + basisAngles.z
            );
        } else if ((event->flags & 0x01) != 0) {
            zClass_Object3D::gwObject3DTranslateRotation(
                targetNode,
                event->vecX,
                event->vecY,
                event->vecZ
            );
        } else {
            zClass_Object3D::gwObject3DSetRotation(
                targetNode,
                event->vecX,
                event->vecY,
                event->vecZ
            );
        }
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handlenodescaleevent
 * @recoil-artifact defines .text recoil:function:0x459cb0: zEffect::HandleNodeScaleEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: set a referenced node's object scale from serialized event values.
 */
int __fastcall HandleNodeScaleEvent(
    zEffectAnimEntry *self,
    zEffectNodeScaleEvent *event
) {
    zClass_Object3D::gwObject3DSetScale(
        self->nodeRefList[event->targetNodeRefIndex].node,
        event->scaleX,
        event->scaleY,
        event->scaleZ
    );
    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handlepositionevent
 * @recoil-artifact defines .text recoil:function:0x459ce0: zEffect::HandlePositionEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: set or translate a target node position or camera target using an
 * optional basis node and serialized offset.
 */
int __fastcall HandlePositionEvent(
    zEffectAnimEntry *self,
    zEffectTransformEvent *event
) {
    zVec3 point = {0};
    zClass_NodePartial *basisNode = 0;

    if (event->basisNodeRefIndex > 0) {
        basisNode = self->nodeRefList[event->basisNodeRefIndex].node;
    } else if (event->basisNodeRefIndex == kEffectAnimResetScratchRefIndex) {
        basisNode = (zClass_NodePartial *)((unsigned int)(self->resetScratch[0]));
        memcpy(
            &point.x,
            &self->resetScratch[1],
            sizeof(point.x)
        );
        memcpy(
            &point.y,
            &self->resetScratch[2],
            sizeof(point.y)
        );
        memcpy(
            &point.z,
            &self->resetScratch[3],
            sizeof(point.z)
        );
    }

    if (basisNode != 0) {
        gwNode::TransformPoint(
            basisNode,
            &point
        );
    }

    point.x += event->vecX;
    point.y += event->vecY;
    point.z += event->vecZ;

    zClass_NodePartial *const targetNode = self->nodeRefList[event->targetNodeRefIndex].node;
    if (targetNode != 0) {
        if (targetNode->classId == 1) {
            if ((event->flags & 0x01) != 0) {
                zClass_Camera::gwCameraTranslateTarget(
                    targetNode,
                    point.x,
                    point.y,
                    point.z
                );
            } else {
                zClass_Camera::gwCameraSetTarget(
                    targetNode,
                    point.x,
                    point.y,
                    point.z
                );
            }
        } else if (targetNode->classId == 5) {
            if ((event->flags & 0x01) != 0) {
                zClass_Object3D::gwObject3DTranslatePosition(
                    targetNode,
                    point.x,
                    point.y,
                    point.z
                );
            } else {
                zClass_Object3D::gwObject3DSetPosition(
                    targetNode,
                    point.x,
                    point.y,
                    point.z
                );
            }
        }
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handleactivateevent
 * @recoil-artifact defines .text recoil:function:0x459e30: zEffect::HandleActivateEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: set the active state for a referenced node or the bound animation
 * node.
 */
int __fastcall HandleActivateEvent(
    zEffectAnimEntry *self,
    zEffectActivateEvent *event
) {
    const short targetIndex = event->targetNodeRefIndex;
    if (targetIndex >= 0) {
        zClass_Class::gwNodeSetActive(
            self->nodeRefList[targetIndex].node,
            event->activeValue
        );
    } else if (targetIndex == kEffectAnimBoundNodeRefIndex) {
        zClass_Class::gwNodeSetActive(
            self->boundNode,
            event->activeValue
        );
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handlenodeanimevent
 * @recoil-artifact defines .text recoil:function:0x459e70: zEffect::HandleNodeAnimEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: run per-frame node motion, physics-like velocity, rotation, scale,
 * DI blend, and collision gating for a node animation event.
 */
int __fastcall HandleNodeAnimEvent(
    zEffectAnimEntry *self,
    zEffectAnimSurfaceRuntime *sequenceRuntime,
    zEffectNodeAnimEvent *animEvent
) {
    if (self == 0 || sequenceRuntime == 0 || animEvent == 0 || animEvent->targetNodeRefIndex < 0) {
        return 1;
    }

    zClass_NodePartial *node = self->nodeRefList[animEvent->targetNodeRefIndex].node;
    int result = 1;

    if (sequenceRuntime->runState == 0) {
        if ((animEvent->flags & 0x0400) == 0) {
            animEvent->runtimeElapsedSec = 0.0f;
        }
        if ((animEvent->flags & 0x0100) != 0) {
            animEvent->runtimeVecE = animEvent->runtimeVecC;
        }
        if ((animEvent->flags & 0x20) != 0) {
            animEvent->runtimeVecB.x = animEvent->scaleRate.z;
            animEvent->runtimeVecB.y = animEvent->endTimeSec;
            memcpy(
                &animEvent->runtimeVecB.z,
                animEvent->unknown_90,
                sizeof(animEvent->runtimeVecB.z)
            );
        }
        if ((animEvent->flags & 0x04) != 0) {
            animEvent->rotationOrCameraPosEnd.z = animEvent->positionOrTargetRate.z;
            animEvent->rotationOrCameraPosRate = animEvent->rotationOrCameraPosStart;
            animEvent->scaleStart.x = animEvent->rotationOrCameraPosEnd.x;
            animEvent->scaleStart.y = animEvent->rotationOrCameraPosEnd.y;
        }
        if ((animEvent->flags & 0x08) != 0) {
            const float randAngleUnit =
                g_zEffect_RandUnitTable[g_zEffect_RandTableIndex];
            g_zEffect_RandTableIndex =
                (g_zEffect_RandTableIndex + 1) % 200;
            const float randAngle =
                (animEvent->positionOrTargetStart.y - animEvent->positionOrTargetStart.x) *
                    randAngleUnit +
                animEvent->positionOrTargetStart.x;
            const float yawUnit =
                g_zEffect_RandUnitTable[g_zEffect_RandTableIndex];
            g_zEffect_RandTableIndex =
                (g_zEffect_RandTableIndex + 1) % 200;
            const float yaw =
                (animEvent->positionOrTargetEnd.x - animEvent->positionOrTargetStart.z) *
                    yawUnit +
                animEvent->positionOrTargetStart.z;
            const float launchMagnitudeUnit =
                g_zEffect_RandUnitTable[g_zEffect_RandTableIndex];
            g_zEffect_RandTableIndex =
                (g_zEffect_RandTableIndex + 1) % 200;
            const float launchMagnitude =
                (animEvent->positionOrTargetEnd.z - animEvent->positionOrTargetEnd.y) *
                    launchMagnitudeUnit +
                animEvent->positionOrTargetEnd.y;
            const float spinMagnitudeUnit =
                g_zEffect_RandUnitTable[g_zEffect_RandTableIndex];
            g_zEffect_RandTableIndex =
                (g_zEffect_RandTableIndex + 1) % 200;
            const float spinMagnitude =
                (animEvent->positionOrTargetRate.y - animEvent->positionOrTargetRate.x) *
                    spinMagnitudeUnit +
                animEvent->positionOrTargetRate.x;

            zVec3 dir = {0};
            zMath_Vec3_DirFromYaw(
                &dir,
                spinMagnitude * 0.01745329252f
            );
            const float vertical = yaw * 0.0111111114f;
            const float horizontal = vertical < 0.0f ? vertical + 1.0f : 1.0f - vertical;
            const float vx = horizontal * dir.x;
            const float vz = horizontal * dir.z;

            animEvent->scaleEnd.x = vertical;
            animEvent->scaleStart.z = vx;
            animEvent->scaleEnd.y = vz;
            animEvent->positionOrTargetRate.z = launchMagnitude * vx;
            animEvent->rotationOrCameraPosStart.x = launchMagnitude * vertical;
            animEvent->rotationOrCameraPosStart.y = launchMagnitude * vz;
            animEvent->rotationOrCameraPosStart.z = randAngle * vx;
            animEvent->rotationOrCameraPosEnd.x = randAngle * vertical;
            animEvent->rotationOrCameraPosEnd.y = randAngle * vz;
            animEvent->rotationOrCameraPosEnd.z = animEvent->positionOrTargetRate.z;
            animEvent->rotationOrCameraPosRate = animEvent->rotationOrCameraPosStart;
            animEvent->scaleStart.x = animEvent->rotationOrCameraPosEnd.x;
            animEvent->scaleStart.y = animEvent->rotationOrCameraPosEnd.y;
        }
        if ((animEvent->flags & 0xc0) != 0) {
            const float scaleEndZ = animEvent->scaleEnd.z;
            const float scaleRateX = animEvent->scaleRate.x;
            animEvent->scaleRate.y = scaleEndZ;
            animEvent->scaleRate.z = scaleRateX;
            animEvent->endTimeSec = animEvent->scaleRate.y;
        }

        bool transformBasis = false;
        if ((animEvent->flags & 0x02) != 0 && (self->flags & 0x80) != 0) {
            transformBasis = true;
        }
        if ((animEvent->flags & 0x01) != 0) {
            transformBasis = true;
            if ((animEvent->flags & 0x2000) != 0) {
                animEvent->scaleStart.x += animEvent->nodeAlphaEnd;
            }
        }

        if (transformBasis) {
            if ((animEvent->flags & 0x02) != 0 && (self->flags & 0x80) != 0) {
                const zVec3 velocity = {self->velocityX, self->velocityY, self->velocityZ};
                zMat4x3 slotBuffer = {0};
                zMath::MatStackPushPtr((float *)(&slotBuffer));
                zMath::MatLoadIdentity();
                gwNode::BuildNodeToAncestorMatrix(
                    node,
                    2
                );
                zMat4x3 basisMatrix = {0};
                basisMatrix.xx = slotBuffer.xx;
                basisMatrix.xy = slotBuffer.yx;
                basisMatrix.xz = slotBuffer.zx;
                basisMatrix.yx = slotBuffer.xy;
                basisMatrix.yy = slotBuffer.yy;
                basisMatrix.yz = slotBuffer.zy;
                basisMatrix.zx = slotBuffer.xz;
                basisMatrix.zy = slotBuffer.yz;
                basisMatrix.zz = slotBuffer.zz;
                zMath::MatLoadCurrentFrom(&basisMatrix);
                const zMat4x3 *matrix =
                    (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
                const zVec3 out = {
                    velocity.x * matrix->xx + velocity.y * matrix->yx +
                        velocity.z * matrix->zx + matrix->posX,
                    velocity.x * matrix->xy + velocity.y * matrix->yy +
                        velocity.z * matrix->zy + matrix->posY,
                    velocity.x * matrix->xz + velocity.y * matrix->yz +
                        velocity.z * matrix->zz + matrix->posZ
                };
                zMath::MatStackPopPtr();
                animEvent->rotationOrCameraPosEnd.z += out.x;
                animEvent->rotationOrCameraPosRate.x += out.y;
                animEvent->rotationOrCameraPosRate.y += out.z;
            }

            if ((animEvent->flags & 0x01) != 0) {
                const zVec3 offset = {0.0f, animEvent->nodeAlphaEnd, 0.0f};
                zMat4x3 slotBuffer = {0};
                zMath::MatStackPushPtr((float *)(&slotBuffer));
                zMath::MatLoadIdentity();
                gwNode::BuildNodeToAncestorMatrix(
                    node,
                    2
                );
                zMat4x3 basisMatrix = {0};
                basisMatrix.xx = slotBuffer.xx;
                basisMatrix.xy = slotBuffer.yx;
                basisMatrix.xz = slotBuffer.zx;
                basisMatrix.yx = slotBuffer.xy;
                basisMatrix.yy = slotBuffer.yy;
                basisMatrix.yz = slotBuffer.zy;
                basisMatrix.zx = slotBuffer.xz;
                basisMatrix.zy = slotBuffer.yz;
                basisMatrix.zz = slotBuffer.zz;
                zMath::MatLoadCurrentFrom(&basisMatrix);
                const zMat4x3 *matrix =
                    (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
                const zVec3 out = {
                    offset.x * matrix->xx + offset.y * matrix->yx +
                        offset.z * matrix->zx + matrix->posX,
                    offset.x * matrix->xy + offset.y * matrix->yy +
                        offset.z * matrix->zy + matrix->posY,
                    offset.x * matrix->xz + offset.y * matrix->yz +
                        offset.z * matrix->zz + matrix->posZ
                };
                zMath::MatStackPopPtr();
                animEvent->rotationOrCameraPosRate.z += out.x;
                animEvent->scaleStart.x += out.y;
                animEvent->scaleStart.y += out.z;
            }
        }
    }

    float frameStepSec = g_zEffect_FrameDeltaRemainingSec;
    if ((animEvent->flags & 0x0400) != 0 &&
        sequenceRuntime->eventElapsedSec > animEvent->runtimeElapsedSec) {
        frameStepSec -= sequenceRuntime->eventElapsedSec - animEvent->runtimeElapsedSec;
    }

    zVec3 worldPos = {0};
    if ((animEvent->flags & 0x0c) != 0) {
        float dx = animEvent->rotationOrCameraPosEnd.z * frameStepSec;
        float dy = animEvent->rotationOrCameraPosRate.x * frameStepSec;
        float dz = animEvent->rotationOrCameraPosRate.y * frameStepSec;
        int movementClamped = 0;

        if ((animEvent->flags & 0x01) != 0) {
            gwNode::GetWorldPosition(
                node,
                &worldPos
            );
            zClass_Class::gwNodeSetCellPickable(
                self->boundNode,
                0
            );
            zClassDiPickCandidateEntry candidate = {0};
            const int found = FindNearestPickCandidateBelowPoint(
                &worldPos,
                &candidate
            );
            zClass_Class::gwNodeSetCellPickable(
                self->boundNode,
                1
            );

            if (dy < 0.0f && found != 0 && worldPos.y + dy < candidate.hitPos.y) {
                movementClamped = 1;
                if (fabs(animEvent->rotationOrCameraPosEnd.z) < 0.100000001f &&
                    fabs(animEvent->rotationOrCameraPosRate.y) < 0.100000001f) {
                    dy = candidate.hitPos.y - worldPos.y;
                } else {
                    dy = fabs(dy * 0.5f) + candidate.hitPos.y - worldPos.y;
                }
            } else if (found == 0 && (animEvent->flags & 0x0400) == 0) {
                animEvent->runtimeElapsedSec += frameStepSec;
                if (animEvent->runtimeElapsedSec > 15.0f) {
                    result = 2;
                    movementClamped = 1;
                }
            }
        }

        if (node->classId == 1) {
            zClass_Camera::gwCameraTranslateTarget(
                node,
                dx,
                dy,
                dz
            );
        } else if (node->classId == 5) {
            zClass_Object3D::gwObject3DTranslatePosition(
                node,
                dx,
                dy,
                dz
            );
        }

        if (movementClamped != 0) {
            const float velocitySq =
                animEvent->rotationOrCameraPosEnd.z * animEvent->rotationOrCameraPosEnd.z +
                animEvent->rotationOrCameraPosRate.x * animEvent->rotationOrCameraPosRate.x +
                animEvent->rotationOrCameraPosRate.y * animEvent->rotationOrCameraPosRate.y;
            const float accelSq =
                animEvent->rotationOrCameraPosRate.z * animEvent->rotationOrCameraPosRate.z +
                animEvent->scaleStart.x * animEvent->scaleStart.x +
                animEvent->scaleStart.y * animEvent->scaleStart.y;
            if (velocitySq < accelSq) {
                result = 2;
            } else {
                animEvent->rotationOrCameraPosEnd.z *= 0.199999988f;
                animEvent->rotationOrCameraPosRate.y *= 0.199999988f;
                animEvent->rotationOrCameraPosRate.x *= 0.199999988f;
            }
        }

        if (movementClamped != 0 && (animEvent->flags & 0x0800) != 0) {
            if (animEvent->packedRuntimeIndex < 0) {
                for (int i = 0; i < self->runtimeSequenceCount; ++i) {
                    if (strcmp(
                            self->runtimeList[i].sequenceName,
                            animEvent->targetName
                        ) == 0) {
                        animEvent->packedRuntimeIndex = (short)(i);
                        break;
                    }
                }
            }
            const short runtimeIndex = animEvent->packedRuntimeIndex;
            if (runtimeIndex >= 0 && self->runtimeList[runtimeIndex].runState == 3) {
                self->runtimeList[runtimeIndex].runState = 0;
            }
        }

        if ((animEvent->flags & 0x1000) != 0 && animEvent->sampleRefIndex > 0) {
            const float speed = sqrt(
                animEvent->rotationOrCameraPosEnd.z * animEvent->rotationOrCameraPosEnd.z +
                animEvent->rotationOrCameraPosRate.x * animEvent->rotationOrCameraPosRate.x +
                animEvent->rotationOrCameraPosRate.y * animEvent->rotationOrCameraPosRate.y
            );
            const float threshold = animEvent->lookupScale < 0.0f ? animEvent->nodeAlphaEnd * 10.0f
                                                                  : animEvent->lookupScale;
            const float gain = speed >= threshold ? 1.0f : speed / threshold;
            zSndSample *const sample = self->sampleRefList[animEvent->sampleRefIndex].sample;
            sample->PlayA3D(
                &worldPos,
                gain,
                0
            );
        }

        animEvent->rotationOrCameraPosEnd.z += animEvent->rotationOrCameraPosRate.z * frameStepSec;
        animEvent->rotationOrCameraPosRate.x += animEvent->scaleStart.x * frameStepSec;
        animEvent->rotationOrCameraPosRate.y += animEvent->scaleStart.y * frameStepSec;
    }

    if ((animEvent->flags & 0x20) != 0) {
        const float dx = animEvent->runtimeVecB.x * frameStepSec;
        const float dy = animEvent->runtimeVecB.y * frameStepSec;
        const float dz = animEvent->runtimeVecB.z * frameStepSec;
        if (node->classId == 1) {
            zClass_Camera::gwCameraTranslate(
                node,
                dx,
                dy,
                dz
            );
        } else if (node->classId == 5) {
            zClass_Object3D::gwObject3DTranslateRotation(
                node,
                dx,
                dy,
                dz
            );
        }
        animEvent->runtimeVecB.x += animEvent->runtimeVecA.x * frameStepSec;
        animEvent->runtimeVecB.y += animEvent->runtimeVecA.y * frameStepSec;
        animEvent->runtimeVecB.z += animEvent->runtimeVecA.z * frameStepSec;
    }

    if ((animEvent->flags & 0x40) != 0) {
        zClass_Object3D::gwObject3DTranslateRotation(
            node,
            animEvent->rotationOrCameraPosRate.y * frameStepSec * animEvent->scaleRate.y,
            0.0f,
            -animEvent->rotationOrCameraPosEnd.z * frameStepSec * animEvent->scaleRate.y
        );
    }

    if ((animEvent->flags & 0x80) != 0) {
        const float dx = animEvent->scaleEnd.y * frameStepSec * animEvent->scaleRate.y;
        const float dz = -animEvent->scaleStart.z * frameStepSec * animEvent->scaleRate.y;
        if (node->classId == 1) {
            zClass_Camera::gwCameraTranslate(
                node,
                dx,
                0.0f,
                dz
            );
        } else if (node->classId == 5) {
            zClass_Object3D::gwObject3DTranslateRotation(
                node,
                dx,
                0.0f,
                dz
            );
        }
        animEvent->scaleRate.y += animEvent->scaleRate.x * frameStepSec;
    }

    if ((animEvent->flags & 0x0100) != 0) {
        zVec3 scale = {0};
        zClass_Object3D::gwObject3DGetScale(
            node,
            &scale.x,
            &scale.y,
            &scale.z
        );
        scale.x += animEvent->runtimeVecE.x * frameStepSec;
        scale.y += animEvent->runtimeVecE.y * frameStepSec;
        scale.z += animEvent->runtimeVecE.z * frameStepSec;
        if (scale.x < 0.001f) {
            scale.x = 0.001f;
        }
        if (scale.y < 0.001f) {
            scale.y = 0.001f;
        }
        if (scale.z < 0.001f) {
            scale.z = 0.001f;
        }
        animEvent->runtimeVecD.x += animEvent->runtimeVecD.x * frameStepSec;
        animEvent->runtimeVecD.y += animEvent->runtimeVecD.y * frameStepSec;
        animEvent->runtimeVecD.z += animEvent->runtimeVecD.z * frameStepSec;
        zClass_Object3D::gwObject3DSetScale(
            node,
            scale.x,
            scale.y,
            scale.z
        );
    }

    if ((animEvent->flags & 0x0200) != 0) {
        if (node != 0 && node->userDataOrDiRef != 0) {
            zDiPartial *const di =
                (zDiPartial *)(node->userDataOrDiRef);
            di->flags |= 0x08;
            di->blendScale += animEvent->nodeAlphaRate * frameStepSec;
            if (di->blendScale > 1.0f) {
                di->blendScale = 1.0f;
            } else if (di->blendScale < 0.00001f) {
                di->flags &= ~0x08;
            }
        }
    }

    g_zEffect_FrameDeltaRemainingSec -= frameStepSec;
    if ((animEvent->flags & 0x0400) != 0 &&
        sequenceRuntime->eventElapsedSec > animEvent->runtimeElapsedSec) {
        return 2;
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.findnearestpickcandidatebelowpoint
 * @recoil-artifact defines .text recoil:function:0x45a920: zEffect::FindNearestPickCandidateBelowPoint.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: choose the nearest DI pick candidate below a world-space point.
 */
int __fastcall FindNearestPickCandidateBelowPoint(
    const zVec3 *point,
    zClassDiPickCandidateEntry *outCandidate
) {
    PlayerProbeSampleCandidateBuffer outResults = {0};
    zClass_cls_di::BuildPickCandidateListBelowPoint(
        g_zEffect_World,
        &outResults,
        point->x,
        point->y,
        point->z
    );

    int bestIndex = -1;
    float closestDistance = 0.0f;
    for (int i = 0; i < outResults.candidateCount; ++i) {
        zClassDiPickCandidateEntry *candidate = &outResults.entries[i];
        const float distance = fabs(point->y - candidate->hitPos.y);
        if (bestIndex < 0) {
            closestDistance = distance;
            bestIndex = i;
        } else if (distance < closestDistance && candidate->hitPos.y - point->y < 10.0f) {
            closestDistance = distance;
            bestIndex = i;
        }
    }

    if (bestIndex < 0) {
        return 0;
    }

    *outCandidate = outResults.entries[bestIndex];
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.animatenodeovertime
 * @recoil-artifact defines .text recoil:function:0x45a9d0: zEffect::AnimateNodeOverTime.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: interpolate a node's position, rotation, scale, and DI blend state
 * over a timed event.
 */
int __fastcall AnimateNodeOverTime(
    zEffectAnimEntry *self,
    zEffectAnimSurfaceRuntime *sequenceRuntime,
    zEffectNodeAnimEvent *nodeAnimEvent
) {
    if (self == 0 || sequenceRuntime == 0 || nodeAnimEvent == 0 ||
        nodeAnimEvent->targetNodeRefIndex < 0) {
        return 2;
    }

    zClass_NodePartial *const node = self->nodeRefList[nodeAnimEvent->targetNodeRefIndex].node;
    if (sequenceRuntime->runState == 0) {
        if ((nodeAnimEvent->flags & 0x04) != 0) {
            zClass_Object3D::gwObject3DSetScale(
                node,
                nodeAnimEvent->scaleStart.x,
                nodeAnimEvent->scaleStart.y,
                nodeAnimEvent->scaleStart.z
            );
        }
        if ((nodeAnimEvent->flags & 0x02) != 0) {
            if (node->classId == 1) {
                zClass_Camera::gwCameraSetPosition(
                    node,
                    nodeAnimEvent->rotationOrCameraPosStart.x,
                    nodeAnimEvent->rotationOrCameraPosStart.y,
                    nodeAnimEvent->rotationOrCameraPosStart.z
                );
            } else if (node->classId == 5) {
                zClass_Object3D::gwObject3DSetRotation(
                    node,
                    nodeAnimEvent->rotationOrCameraPosStart.x,
                    nodeAnimEvent->rotationOrCameraPosStart.y,
                    nodeAnimEvent->rotationOrCameraPosStart.z
                );
            }
        }
        if ((nodeAnimEvent->flags & 0x01) != 0) {
            if (node->classId == 1) {
                zClass_Camera::gwCameraSetTarget(
                    node,
                    nodeAnimEvent->positionOrTargetStart.x,
                    nodeAnimEvent->positionOrTargetStart.y,
                    nodeAnimEvent->positionOrTargetStart.z
                );
            } else if (node->classId == 5) {
                zClass_Object3D::gwObject3DSetPosition(
                    node,
                    nodeAnimEvent->positionOrTargetStart.x,
                    nodeAnimEvent->positionOrTargetStart.y,
                    nodeAnimEvent->positionOrTargetStart.z
                );
            }
        }
        if ((nodeAnimEvent->flags & 0x08) != 0) {
            if (node != 0 && node->userDataOrDiRef != 0) {
                zDiPartial *const di =
                    (zDiPartial *)(node->userDataOrDiRef);
                di->flags |= 0x08;
                di->blendScale = nodeAnimEvent->nodeAlphaStart;
                if (di->blendScale > 1.0f) {
                    di->blendScale = 1.0f;
                } else if (di->blendScale < 0.00001f) {
                    di->flags &= ~0x08;
                }
            }
        }
    }

    const float deltaTimeSec =
        sequenceRuntime->eventElapsedSec <= nodeAnimEvent->endTimeSec
            ? g_zEffect_FrameDeltaRemainingSec
            : g_zEffect_FrameDeltaRemainingSec -
                  (sequenceRuntime->eventElapsedSec - nodeAnimEvent->endTimeSec);

    if ((nodeAnimEvent->flags & 0x01) != 0) {
        if (node->classId == 1) {
            zClass_Camera::gwCameraTranslateTarget(
                node,
                nodeAnimEvent->positionOrTargetRate.x * deltaTimeSec,
                nodeAnimEvent->positionOrTargetRate.y * deltaTimeSec,
                nodeAnimEvent->positionOrTargetRate.z * deltaTimeSec
            );
        } else if (node->classId == 5) {
            zClass_Object3D::gwObject3DTranslatePosition(
                node,
                nodeAnimEvent->positionOrTargetRate.x * deltaTimeSec,
                nodeAnimEvent->positionOrTargetRate.y * deltaTimeSec,
                nodeAnimEvent->positionOrTargetRate.z * deltaTimeSec
            );
        }
    }

    if ((nodeAnimEvent->flags & 0x02) != 0) {
        if (node->classId == 1) {
            zClass_Camera::gwCameraTranslate(
                node,
                nodeAnimEvent->rotationOrCameraPosRate.x * deltaTimeSec,
                nodeAnimEvent->rotationOrCameraPosRate.y * deltaTimeSec,
                nodeAnimEvent->rotationOrCameraPosRate.z * deltaTimeSec
            );
        } else if (node->classId == 5) {
            zClass_Object3D::gwObject3DTranslateRotation(
                node,
                nodeAnimEvent->rotationOrCameraPosRate.x * deltaTimeSec,
                nodeAnimEvent->rotationOrCameraPosRate.y * deltaTimeSec,
                nodeAnimEvent->rotationOrCameraPosRate.z * deltaTimeSec
            );
        }
    }

    if ((nodeAnimEvent->flags & 0x04) != 0) {
        zVec3 scale = {0};
        zClass_Object3D::gwObject3DGetScale(
            node,
            &scale.x,
            &scale.y,
            &scale.z
        );
        scale.x += nodeAnimEvent->scaleRate.x * deltaTimeSec;
        scale.y += nodeAnimEvent->scaleRate.y * deltaTimeSec;
        scale.z += nodeAnimEvent->scaleRate.z * deltaTimeSec;
        if (scale.x < 0.001f) {
            scale.x = 0.001f;
        }
        if (scale.y < 0.001f) {
            scale.y = 0.001f;
        }
        if (scale.z < 0.001f) {
            scale.z = 0.001f;
        }
        zClass_Object3D::gwObject3DSetScale(
            node,
            scale.x,
            scale.y,
            scale.z
        );
    }

    if ((nodeAnimEvent->flags & 0x08) != 0) {
        if (node != 0 && node->userDataOrDiRef != 0) {
            zDiPartial *const di =
                (zDiPartial *)(node->userDataOrDiRef);
            di->flags |= 0x08;
            di->blendScale += nodeAnimEvent->nodeAlphaRate * deltaTimeSec;
            if (di->blendScale > 1.0f) {
                di->blendScale = 1.0f;
            } else if (di->blendScale < 0.00001f) {
                di->flags &= ~0x08;
            }
        }
    }

    g_zEffect_FrameDeltaRemainingSec -= deltaTimeSec;
    if (sequenceRuntime->eventElapsedSec <= nodeAnimEvent->endTimeSec) {
        return 1;
    }

    if ((nodeAnimEvent->flags & 0x04) != 0) {
        zClass_Object3D::gwObject3DSetScale(
            node,
            nodeAnimEvent->scaleEnd.x,
            nodeAnimEvent->scaleEnd.y,
            nodeAnimEvent->scaleEnd.z
        );
    }
    if ((nodeAnimEvent->flags & 0x02) != 0) {
        zClass_Object3D::gwObject3DSetRotation(
            node,
            nodeAnimEvent->rotationOrCameraPosEnd.x,
            nodeAnimEvent->rotationOrCameraPosEnd.y,
            nodeAnimEvent->rotationOrCameraPosEnd.z
        );
    }
    if ((nodeAnimEvent->flags & 0x01) != 0) {
        if (node->classId == 1) {
            zClass_Camera::gwCameraSetTarget(
                node,
                nodeAnimEvent->positionOrTargetEnd.x,
                nodeAnimEvent->positionOrTargetEnd.y,
                nodeAnimEvent->positionOrTargetEnd.z
            );
        } else if (node->classId == 5) {
            zClass_Object3D::gwObject3DSetPosition(
                node,
                nodeAnimEvent->positionOrTargetEnd.x,
                nodeAnimEvent->positionOrTargetEnd.y,
                nodeAnimEvent->positionOrTargetEnd.z
            );
        }
    }
    if ((nodeAnimEvent->flags & 0x08) != 0) {
        if (node != 0 && node->userDataOrDiRef != 0) {
            zDiPartial *const di =
                (zDiPartial *)(node->userDataOrDiRef);
            di->flags |= 0x08;
            di->blendScale = nodeAnimEvent->nodeAlphaEnd;
            if (di->blendScale > 1.0f) {
                di->blendScale = 1.0f;
            } else if (di->blendScale < 0.00001f) {
                di->flags &= ~0x08;
            }
        }
    }

    return 2;
}

} // namespace zEffect

namespace zEffect_Anim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.advancekeyframesample
 * @recoil-artifact defines .text recoil:function:0x45ae30: zEffect_Anim::AdvanceKeyframeSample.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: advance a keyframe event cursor past the current sample and report
 * whether another sample remains in the event record.
 */
int __fastcall AdvanceKeyframeSample(
    zEffectAnimSurfaceRuntime *sequenceRuntime,
    zEffectKeyframeEvent *keyframeEvent,
    zEffectKeyframeSampleHeader *sampleHeader
) {
    const int nextSampleOffset =
        keyframeEvent->currentKeyframeOffset + (int)(sizeof(zEffectKeyframeSampleHeader));

    ++keyframeEvent->lookaheadAdvanceCount;
    keyframeEvent->currentKeyframeOffset = nextSampleOffset;

    const int channelFlags = sampleHeader->channelFlags;
    if ((channelFlags & 0x01) != 0) {
        keyframeEvent->currentKeyframeOffset =
            nextSampleOffset + (int)(sizeof(zEffectKeyframeSampleChannel));
    }
    if ((channelFlags & 0x02) != 0) {
        keyframeEvent->currentKeyframeOffset += (int)(sizeof(zEffectKeyframeSampleChannel));
    }
    if ((channelFlags & 0x04) != 0) {
        keyframeEvent->currentKeyframeOffset += (int)(sizeof(zEffectKeyframeSampleChannel));
    }

    keyframeEvent->keyframeLocalTime = 0.0f;

    zEffectAnimEventHeader *const currentEvent =
        (zEffectAnimEventHeader *)(sequenceRuntime->currentEvent);
    return keyframeEvent->currentKeyframeOffset < currentEvent->recordSize ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.animatekeyframesample
 * @recoil-artifact defines .text recoil:function:0x45ae90: zEffect_Anim::AnimateKeyframeSample.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: apply position, rotation, and scale channels for one keyframe
 * sample over the current frame slice.
 */
float __fastcall AnimateKeyframeSample(
    zEffectAnimSurfaceRuntime *sequenceRuntime,
    zEffectKeyframeEvent *keyframeEvent,
    zClass_NodePartial *targetNode,
    zEffectKeyframeSampleHeader *sampleHeader,
    float *deltaTime
) {
    float preStartDelaySec = 0.0f;
    if (sequenceRuntime->eventElapsedSec < sampleHeader->startTimeSec) {
        const float savedDeltaTimeSec = *deltaTime;
        *deltaTime = sequenceRuntime->eventElapsedSec;
        return savedDeltaTimeSec;
    }

    if (*deltaTime < sampleHeader->startTimeSec) {
        preStartDelaySec = sampleHeader->startTimeSec - *deltaTime;
        *deltaTime = sampleHeader->startTimeSec;
    }

    float sampleEndTimeSec = sequenceRuntime->eventElapsedSec;
    if (sequenceRuntime->eventElapsedSec > sampleHeader->endTimeSec) {
        const float savedLocalTime = keyframeEvent->keyframeLocalTime;
        const int savedOffset = keyframeEvent->currentKeyframeOffset;
        if (AdvanceKeyframeSample(
            sequenceRuntime,
            keyframeEvent,
            sampleHeader
        ) != 0) {
            zEffectKeyframeSampleHeader *const nextSample =
                (zEffectKeyframeSampleHeader *)((unsigned char *)(keyframeEvent) +
                                                keyframeEvent->currentKeyframeOffset);
            if (nextSample->channelFlags == sampleHeader->channelFlags) {
                const float consumed = sampleHeader->endTimeSec - *deltaTime + preStartDelaySec;
                *deltaTime = sampleHeader->endTimeSec;
                keyframeEvent->keyframeLocalTime = sampleHeader->endTimeSec;
                keyframeEvent->currentKeyframeOffset = savedOffset;
                return consumed;
            }
        }

        keyframeEvent->currentKeyframeOffset = savedOffset;
        keyframeEvent->keyframeLocalTime = savedLocalTime;
        --keyframeEvent->lookaheadAdvanceCount;
        sampleEndTimeSec = sampleHeader->endTimeSec;
    }

    zEffectKeyframeSampleChannel *sampleChannel =
        (zEffectKeyframeSampleChannel *)(sampleHeader + 1);
    const float sampleDurationSec = sampleEndTimeSec - *deltaTime;
    keyframeEvent->keyframeLocalTime += sampleDurationSec;
    const float sampleLocalTime = keyframeEvent->keyframeLocalTime;

    zVec3 outEuler = {0};
    if ((sampleHeader->channelFlags & 0x01) != 0) {
        outEuler.x = sampleChannel->rate.x * sampleLocalTime + sampleChannel->baseQuat.w;
        outEuler.y = sampleChannel->rate.y * sampleLocalTime + sampleChannel->baseQuat.x;
        outEuler.z = sampleChannel->rate.z * sampleLocalTime + sampleChannel->baseQuat.y;

        if (targetNode->classId == 1) {
            zClass_Camera::gwCameraSetTarget(
                targetNode,
                outEuler.x,
                outEuler.y,
                outEuler.z
            );
        } else if (targetNode->classId == 5) {
            zClass_Object3D::gwObject3DSetPosition(
                targetNode,
                outEuler.x,
                outEuler.y,
                outEuler.z
            );
        }

        ++sampleChannel;
    }

    if ((sampleHeader->channelFlags & 0x02) != 0) {
        const zVec3 rotationVector = {sampleChannel->rate.x * sampleLocalTime,
            sampleChannel->rate.y * sampleLocalTime,
            sampleChannel->rate.z * sampleLocalTime};

        zQuat deltaQuat = {0};
        zMath_Quat_FromRotationVector(
            &rotationVector,
            &deltaQuat
        );

        zQuat blendedQuat = {0};
        zMath_Quat_Multiply(
            &deltaQuat,
            &sampleChannel->baseQuat,
            &blendedQuat
        );

        zMat4x3 rotationMatrix = {0};
        zMath_Quat_ToMatrix(
            &blendedQuat,
            &rotationMatrix
        );
        zMath_Mat_ExtractEulerAngles(
            &rotationMatrix,
            &outEuler
        );

        if (targetNode->classId == 1) {
            zClass_Camera::gwCameraSetPosition(
                targetNode,
                outEuler.x,
                outEuler.y,
                outEuler.z
            );
        } else if (targetNode->classId == 5) {
            zClass_Object3D::gwObject3DSetRotation(
                targetNode,
                outEuler.x,
                outEuler.y,
                outEuler.z
            );
        }

        ++sampleChannel;
    }

    if ((sampleHeader->channelFlags & 0x04) != 0) {
        outEuler.x =
            sampleChannel->rate.x * keyframeEvent->keyframeLocalTime + sampleChannel->baseQuat.w;
        outEuler.y =
            sampleChannel->rate.y * keyframeEvent->keyframeLocalTime + sampleChannel->baseQuat.x;
        outEuler.z =
            sampleChannel->rate.z * keyframeEvent->keyframeLocalTime + sampleChannel->baseQuat.y;

        if (targetNode->classId == 5) {
            zClass_Object3D::gwObject3DSetScale(
                targetNode,
                outEuler.x,
                outEuler.y,
                outEuler.z
            );
        }
    }

    *deltaTime += sampleDurationSec;
    return preStartDelaySec + sampleDurationSec;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.advancekeyframe
 * @recoil-artifact defines .text recoil:function:0x45b120: zEffect_Anim::AdvanceKeyframe.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: advance a serialized keyframe event for a runtime sequence.
 */
int __fastcall AdvanceKeyframe(
    zEffectAnimEntry *self,
    zEffectAnimSurfaceRuntime *sequenceRuntime,
    zEffectKeyframeEvent *keyframeEvent
) {
    int runState = 1;
    if (self == 0 || sequenceRuntime == 0 || keyframeEvent == 0 ||
        keyframeEvent->targetNodeRefIndex < 0) {
        return 2;
    }

    zClass_NodePartial *const targetNode =
        self->nodeRefList[keyframeEvent->targetNodeRefIndex].node;
    if (sequenceRuntime->runState == 0) {
        keyframeEvent->currentKeyframeOffset = (int)(sizeof(zEffectKeyframeEvent));
        keyframeEvent->keyframeLocalTime = 0.0f;
        keyframeEvent->lookaheadAdvanceCount = 0;
    }

    float sampleTimeSec = sequenceRuntime->eventElapsedSec - g_zEffect_FrameDeltaRemainingSec;

    while (keyframeEvent->currentKeyframeOffset < keyframeEvent->header.recordSize) {
        zEffectKeyframeSampleHeader *const sampleHeader =
            (zEffectKeyframeSampleHeader *)((unsigned char *)(keyframeEvent) +
                                            keyframeEvent->currentKeyframeOffset);
        const float consumedSec = AnimateKeyframeSample(
            sequenceRuntime,
            keyframeEvent,
            targetNode,
            sampleHeader,
            &sampleTimeSec
        );
        g_zEffect_FrameDeltaRemainingSec -= consumedSec;

        if (sampleTimeSec < sequenceRuntime->eventElapsedSec &&
            AdvanceKeyframeSample(
                sequenceRuntime,
                keyframeEvent,
                sampleHeader
            ) == 0) {
            runState = 2;
        }

        if (sampleTimeSec >= sequenceRuntime->eventElapsedSec || runState == 2) {
            break;
        }
    }

    return keyframeEvent->currentKeyframeOffset < keyframeEvent->header.recordSize ? runState : 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.evaluatekeyframe
 * @recoil-artifact defines .text recoil:function:0x45b210: zEffect_Anim::EvaluateKeyframe.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: apply immediate lit and alpha-scale values to an animation target
 * node.
 */
int __fastcall EvaluateKeyframe(
    zEffectAnimEntry *self,
    zEffectEvaluateKeyframeEvent *keyframeEvent
) {
    zClass_NodePartial *targetNode = 0;
    if (keyframeEvent->targetNodeRefIndex >= 0) {
        targetNode = self->nodeRefList[keyframeEvent->targetNodeRefIndex].node;
    } else if (keyframeEvent->targetNodeRefIndex == -100) {
        targetNode = self->boundNode;
    }

    if (targetNode != 0) {
        zClass_Object3D::gwObject3DSetLitFlag(
            targetNode,
            keyframeEvent->litFlag == 1 ? 1 : 0
        );

        if (keyframeEvent->hasAlphaScale == 1) {
            zClass_Object3D::gwObject3DSetAlphaScale(
                targetNode,
                keyframeEvent->alphaScale
            );
        }
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.runkeyframes
 * @recoil-artifact defines .text recoil:function:0x45b280: zEffect_Anim::RunKeyframes.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: animate a target node's lit state and alpha scale across a timed
 * keyframe record.
 */
int __fastcall RunKeyframes(
    zEffectAnimEntry *self,
    zEffectAnimSurfaceRuntime *sequenceRuntime,
    zEffectRunKeyframeEvent *keyframeEvent
) {
    if (self == 0 || sequenceRuntime == 0 || keyframeEvent == 0 ||
        keyframeEvent->targetNodeRefIndex < 0) {
        return 2;
    }

    zClass_NodePartial *const targetNode =
        self->nodeRefList[keyframeEvent->targetNodeRefIndex].node;
    if (sequenceRuntime->runState == 0) {
        if (keyframeEvent->startLitFlag == 1) {
            zClass_Object3D::gwObject3DSetLitFlag(
                targetNode,
                1
            );
        } else if (keyframeEvent->startLitFlag == 0) {
            zClass_Object3D::gwObject3DSetLitFlag(
                targetNode,
                0
            );
        }

        zClass_Object3D::gwObject3DSetAlphaScale(
            targetNode,
            keyframeEvent->startAlphaScale
        );
    }

    const float frameDeltaUsedSec =
        sequenceRuntime->eventElapsedSec <= keyframeEvent->endTimeSec
            ? g_zEffect_FrameDeltaRemainingSec
            : g_zEffect_FrameDeltaRemainingSec -
                  (sequenceRuntime->eventElapsedSec - keyframeEvent->endTimeSec);

    float alphaScale = 0.0f;
    if (zClass_Object3D::gwObject3DGetAlphaScale(
        targetNode,
        &alphaScale
    ) != 0) {
        return 2;
    }

    zClass_Object3D::gwObject3DSetAlphaScale(
        targetNode,
        keyframeEvent->alphaScaleRate * frameDeltaUsedSec + alphaScale
    );
    g_zEffect_FrameDeltaRemainingSec -= frameDeltaUsedSec;

    if (sequenceRuntime->eventElapsedSec <= keyframeEvent->endTimeSec) {
        return 1;
    }

    if (keyframeEvent->endLitFlag == 1) {
        zClass_Object3D::gwObject3DSetLitFlag(
            targetNode,
            1
        );
    } else if (keyframeEvent->endLitFlag == 0) {
        zClass_Object3D::gwObject3DSetLitFlag(
            targetNode,
            0
        );
    }

    zClass_Object3D::gwObject3DSetAlphaScale(
        targetNode,
        keyframeEvent->endAlphaScale
    );
    return 2;
}

} // namespace zEffect_Anim

namespace zEffect {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handleaddchildevent
 * @recoil-artifact defines .text recoil:function:0x45b3b0: zEffect::HandleAddChildEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: add a child node to a parent node when the relationship is not
 * already present.
 */
int __fastcall HandleAddChildEvent(
    zEffectAnimEntry *self,
    zEffectParentChildEvent *event
) {
    if (event->parentNodeRefIndex > 0 && event->childNodeRefIndex > 0) {
        zClass_NodePartial *const parentNode = self->nodeRefList[event->parentNodeRefIndex].node;
        zClass_NodePartial *const childNode = self->nodeRefList[event->childNodeRefIndex].node;

        for (int i = 0; i < parentNode->listCountB; ++i) {
            if (parentNode->listB[i] == childNode) {
                return 2;
            }
        }

        zClass_Class::AddChild(
            parentNode,
            childNode
        );
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handleremovechildevent
 * @recoil-artifact defines .text recoil:function:0x45b410: zEffect::HandleRemoveChildEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: remove a child node from its serialized parent node reference.
 */
int __fastcall HandleRemoveChildEvent(
    zEffectAnimEntry *self,
    zEffectParentChildEvent *event
) {
    zEffectAnimNodeRef28 *const nodeRefList = self->nodeRefList;
    zClass_Class::RemoveChild(
        nodeRefList[event->parentNodeRefIndex].node,
        nodeRefList[event->childNodeRefIndex].node
    );
    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handleattachevent
 * @recoil-artifact defines .text recoil:function:0x45b440: zEffect::HandleAttachEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: attach the serialized variant state to a referenced target node.
 */
int __fastcall HandleAttachEvent(
    zEffectAnimEntry *self,
    zEffectAnimSurfaceRuntime *sequenceRuntime,
    zEffectAttachEvent *event
) {
    if (self != 0 && sequenceRuntime != 0 && event != 0) {
        if (event->targetNodeRefIndex >= 0) {
            zClass_NodePartial *const targetNode =
                self->nodeRefList[event->targetNodeRefIndex].node;
            if ((event->flags & 0x01) != 0) {
                zDiPartial *const targetDi = (zDiPartial *)(targetNode->userDataOrDiRef);
                if (targetDi != 0) {
                    zDi::ResetCurrentVariant(targetDi);
                }
            }

            zDi::SetCurrentVariant(
                (zDiPartial *)(targetNode->userDataOrDiRef),
                event->variantIndex
            );
        }
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handledetachevent
 * @recoil-artifact defines .text recoil:function:0x45b4a0: zEffect::HandleDetachEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: detach and animate a beam segment between stored or referenced
 * points over a timed event.
 */
int __fastcall HandleDetachEvent(
    zEffectAnimEntry *self,
    zEffectAnimSurfaceRuntime *runtime,
    zEffectBeamDetachEvent *event
) {
    if (self == 0 || runtime == 0 || event == 0 || event->beamNodeRefIndex < 0) {
        return 2;
    }

    zClass_NodePartial *const beamNode = self->nodeRefList[event->beamNodeRefIndex].node;

    if (runtime->runState == 0) {
        unsigned int flags = (unsigned int)(event->flags);
        if ((flags & 0x0800u) != 0) {
            event->segmentStartCurrent = event->segmentStartInitial;
        }
        if ((flags & 0x2000u) != 0) {
            event->segmentEndCurrent = event->segmentEndInitial;
        }

        if ((flags & 0x04u) != 0) {
            flags &= ~0x04u;
            event->flags = (int)(flags);

            zVec3 point = {0};
            if ((flags & 0x08u) != 0) {
                flags &= ~0x08u;
                event->flags = (int)(flags);
                point = event->pointA;
            } else if ((flags & 0x10u) != 0) {
                flags &= ~0x10u;
                event->flags = (int)(flags);
                memcpy(
                    &point.x,
                    &self->resetScratch[1],
                    sizeof(point.x)
                );
                memcpy(
                    &point.y,
                    &self->resetScratch[2],
                    sizeof(point.y)
                );
                memcpy(
                    &point.z,
                    &self->resetScratch[3],
                    sizeof(point.z)
                );
            }

            zClass_NodePartial *const refNode =
                (zClass_NodePartial *)((unsigned int)(self->resetScratch[0]));
            if (refNode != 0) {
                gwNode::TransformPoint(
                    refNode,
                    &point
                );
            }
            event->pointA = point;
            flags = (unsigned int)(event->flags) | 0x08u;
            event->flags = (int)(flags);
        }

        flags = (unsigned int)(event->flags);
        if ((flags & 0x80u) != 0) {
            flags &= ~0x80u;
            event->flags = (int)(flags);

            zVec3 point = {0};
            if ((flags & 0x0100u) != 0) {
                flags &= ~0x0100u;
                event->flags = (int)(flags);
                point = event->pointB;
            } else if ((flags & 0x0200u) != 0) {
                flags &= ~0x0200u;
                event->flags = (int)(flags);
                memcpy(
                    &point.x,
                    &self->resetScratch[5],
                    sizeof(point.x)
                );
                memcpy(
                    &point.y,
                    &self->resetScratch[6],
                    sizeof(point.y)
                );
                memcpy(
                    &point.z,
                    &self->resetScratch[7],
                    sizeof(point.z)
                );
            }

            zClass_NodePartial *const refNode =
                (zClass_NodePartial *)((unsigned int)(self->resetScratch[4]));
            if (refNode != 0) {
                gwNode::TransformPoint(
                    refNode,
                    &point
                );
            }
            event->pointB = point;
            flags = (unsigned int)(event->flags) | 0x0100u;
            event->flags = (int)(flags);
        }
    }

    const float timeSlice =
        runtime->eventElapsedSec <= event->endTimeSec
            ? g_zEffect_FrameDeltaRemainingSec
            : g_zEffect_FrameDeltaRemainingSec - (runtime->eventElapsedSec - event->endTimeSec);

    const unsigned int flags = (unsigned int)(event->flags);

    zClass_NodePartial *pointANode = 0;
    if ((flags & 0x01u) != 0) {
        if (event->pointANodeRefIndex >= 0) {
            pointANode = self->nodeRefList[event->pointANodeRefIndex].node;
        }
    } else if ((flags & 0x02u) != 0) {
        pointANode =
            (zClass_NodePartial *)((unsigned int)(self->resetScratch[0]));
    }

    zVec3 pointA = {0};
    if ((flags & 0x08u) != 0) {
        pointA = event->pointA;
    } else if ((flags & 0x10u) != 0) {
        memcpy(
            &pointA.x,
            &self->resetScratch[1],
            sizeof(pointA.x)
        );
        memcpy(
            &pointA.y,
            &self->resetScratch[2],
            sizeof(pointA.y)
        );
        memcpy(
            &pointA.z,
            &self->resetScratch[3],
            sizeof(pointA.z)
        );
    }
    if (pointANode != 0) {
        gwNode::TransformPoint(
            pointANode,
            &pointA
        );
    }

    zClass_NodePartial *pointBNode = 0;
    if ((flags & 0x20u) != 0) {
        if (event->pointBNodeRefIndex >= 0) {
            pointBNode = self->nodeRefList[event->pointBNodeRefIndex].node;
        }
    } else if ((flags & 0x40u) != 0) {
        pointBNode =
            (zClass_NodePartial *)((unsigned int)(self->resetScratch[4]));
    }

    zVec3 pointB = {0};
    if ((flags & 0x0100u) != 0) {
        pointB = event->pointB;
    } else if ((flags & 0x0200u) != 0) {
        memcpy(
            &pointB.x,
            &self->resetScratch[5],
            sizeof(pointB.x)
        );
        memcpy(
            &pointB.y,
            &self->resetScratch[6],
            sizeof(pointB.y)
        );
        memcpy(
            &pointB.z,
            &self->resetScratch[7],
            sizeof(pointB.z)
        );
    }
    if (pointBNode != 0) {
        gwNode::TransformPoint(
            pointBNode,
            &pointB
        );
    }

    int fractionChanged = 0;
    if ((flags & 0x0400u) != 0) {
        fractionChanged = 1;
        event->segmentStartCurrent = event->segmentStartInitial;
    } else if ((flags & 0x0800u) != 0) {
        fractionChanged = 1;
        event->segmentStartCurrent += event->segmentStartRate * timeSlice;
    } else {
        event->segmentStartCurrent = 0.0f;
    }

    if ((flags & 0x1000u) != 0) {
        fractionChanged = 1;
        event->segmentEndCurrent = event->segmentEndInitial;
    } else if ((flags & 0x2000u) != 0) {
        fractionChanged = 1;
        event->segmentEndCurrent += event->segmentEndRate * timeSlice;
    } else {
        event->segmentEndCurrent = 1.0f;
    }

    float beamLength = 0.0f;
    if (fractionChanged != 0) {
        if (runtime->eventElapsedSec > event->endTimeSec) {
            event->segmentStartCurrent = event->segmentStartFinal;
            event->segmentEndCurrent = event->segmentEndFinal;
        }

        beamLength = UpdateBeamNodeBetweenFractions(
            beamNode,
            &pointA,
            event->segmentStartCurrent,
            &pointB,
            event->segmentEndCurrent
        );
    } else {
        beamLength = UpdateBeamNodeBetweenPoints(
            beamNode,
            &pointA,
            &pointB
        );
    }

    int result = 2;
    if ((flags & 0x8000u) != 0 && beamLength <= event->lengthThreshold) {
        result = 1;
    }
    if (runtime->eventElapsedSec < event->endTimeSec) {
        result = 1;
    }

    g_zEffect_FrameDeltaRemainingSec -= timeSlice;
    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handletransformrefsevent
 * @recoil-artifact defines .text recoil:function:0x45b8b0: zEffect::HandleTransformRefsEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: launch a child animation using two stored or referenced transform
 * points.
 */
int __fastcall HandleTransformRefsEvent(
    zEffectAnimEntry *self,
    zEffectTransformRefsEvent *event
) {
    if (event->animEntryIndex <= 0) {
        zEffectAnimEntry *entry = g_zEffectAnim_EntryList;
        for (int i = 0; i < g_zEffectAnim_EntryCount; ++i, ++entry) {
            if (strcmp(
                    entry->name,
                    event->animName
                ) == 0) {
                event->animEntryIndex = (short)(i);
                break;
            }
        }
    }
    if (event->animEntryIndex > 0) {
        const unsigned int flags = (unsigned int)(event->flags);

        zVec3 refPointA = {0};
        if ((flags & 0x10u) != 0) {
            refPointA = event->refPointA;
        } else if ((flags & 0x20u) != 0) {
            memcpy(
                &refPointA.x,
                &self->resetScratch[1],
                sizeof(refPointA.x)
            );
            memcpy(
                &refPointA.y,
                &self->resetScratch[2],
                sizeof(refPointA.y)
            );
            memcpy(
                &refPointA.z,
                &self->resetScratch[3],
                sizeof(refPointA.z)
            );
        }

        zVec3 refPointB = {0};
        if ((flags & 0x0400u) != 0) {
            refPointB = event->refPointB;
        } else if ((flags & 0x0800u) != 0) {
            memcpy(
                &refPointB.x,
                &self->resetScratch[5],
                sizeof(refPointB.x)
            );
            memcpy(
                &refPointB.y,
                &self->resetScratch[6],
                sizeof(refPointB.y)
            );
            memcpy(
                &refPointB.z,
                &self->resetScratch[7],
                sizeof(refPointB.z)
            );
        }

        zClass_NodePartial *refNodeA = 0;
        if ((flags & 0x01u) != 0) {
            refNodeA = self->nodeRefList[event->refNodeAIndex].node;
        } else if ((flags & 0x02u) != 0) {
            gwNode::TransformPoint(
                self->nodeRefList[event->refNodeAIndex].node,
                &refPointA
            );
        } else if ((flags & 0x04u) != 0) {
            refNodeA =
                (zClass_NodePartial *)((unsigned int)(self->resetScratch[0]));
        } else if ((flags & 0x08u) != 0) {
            gwNode::TransformPoint(
                (zClass_NodePartial *)((unsigned int)(self->resetScratch[0])),
                &refPointA
            );
        } else {
            refNodeA = (zClass_NodePartial *)(self);
        }

        zClass_NodePartial *refNodeB = 0;
        if ((flags & 0x40u) != 0) {
            refNodeB = self->nodeRefList[event->refNodeBIndex].node;
        } else if ((flags & 0x80u) != 0) {
            gwNode::TransformPoint(
                self->nodeRefList[event->refNodeBIndex].node,
                &refPointB
            );
        } else if ((flags & 0x0100u) != 0) {
            refNodeB =
                (zClass_NodePartial *)((unsigned int)(self->resetScratch[4]));
        } else if ((flags & 0x0200u) != 0) {
            gwNode::TransformPoint(
                (zClass_NodePartial *)((unsigned int)(self->resetScratch[4])),
                &refPointB
            );
        } else {
            refNodeB = (zClass_NodePartial *)(self);
        }

        zEffectAnimEntry *const childEntry = zEffectAnim::SetTransformRefs(
            &g_zEffectAnim_EntryList[event->animEntryIndex],
            0,
            refNodeA,
            &refPointA,
            refNodeB,
            &refPointB
        );

        if (event->runtimeRefIndex >= 0) {
            self->runtimeRefList[event->runtimeRefIndex].cachedChildEntry = childEntry;
        }
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handlesurfacestopevent
 * @recoil-artifact defines .text recoil:function:0x45bb00: zEffect::HandleSurfaceStopEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: stop a named runtime surface sequence when it is currently playing.
 */
int __fastcall HandleSurfaceStopEvent(
    zEffectAnimEntry *self,
    zEffectSurfaceControlEvent *event
) {
    if (event->surfaceSlotIndex < 0) {
        for (int i = 0; i < self->runtimeSequenceCount; ++i) {
            if (strcmp(
                    self->runtimeList[i].sequenceName,
                    event->sequenceName
                ) == 0) {
                event->surfaceSlotIndex = i;
                break;
            }
        }
    }
    const int surfaceIndex = event->surfaceSlotIndex;
    if (surfaceIndex >= 0 && self->runtimeList[surfaceIndex].runState == 3) {
        self->runtimeList[surfaceIndex].runState = 0;
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handlesurfaceplayevent
 * @recoil-artifact defines .text recoil:function:0x45bbb0: zEffect::HandleSurfacePlayEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: set a named runtime surface sequence into play state.
 */
int __fastcall HandleSurfacePlayEvent(
    zEffectAnimEntry *self,
    zEffectSurfaceControlEvent *event
) {
    if (event->surfaceSlotIndex < 0) {
        for (int i = 0; i < self->runtimeSequenceCount; ++i) {
            if (strcmp(
                    self->runtimeList[i].sequenceName,
                    event->sequenceName
                ) == 0) {
                event->surfaceSlotIndex = i;
                break;
            }
        }
    }
    const int surfaceIndex = event->surfaceSlotIndex;
    if (surfaceIndex >= 0) {
        self->runtimeList[surfaceIndex].runState = 2;
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handlesurfacerefevent
 * @recoil-artifact defines .text recoil:function:0x45bc60: zEffect::HandleSurfaceRefEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: start a referenced child animation from surface event data and
 * optionally wait for its activation to finish.
 */
int __fastcall HandleSurfaceRefEvent(
    zEffectAnimEntry *self,
    zEffectAnimSurfaceRuntime *runtime,
    zEffectSurfaceRefEvent *event
) {
    if (runtime->runState == 0) {
        zEffectAnimEntry *childEntry = 0;
        if (event->runtimeRefIndex >= 0) {
            self->runtimeRefList[event->runtimeRefIndex].cachedChildEntry = 0;
        }

        if (event->animEntryIndex <= 0) {
            zEffectAnimEntry *entry = g_zEffectAnim_EntryList;
            for (int i = 0; i < g_zEffectAnim_EntryCount; ++i, ++entry) {
                if (strcmp(
                        entry->name,
                        event->sequenceName
                    ) == 0) {
                    event->animEntryIndex = (short)(i);
                    break;
                }
            }
        }
        if (event->animEntryIndex > 0) {
            zClass_NodePartial *boundNode = 0;
            if (event->boundNodeRefIndex > 0) {
                boundNode = self->nodeRefList[event->boundNodeRefIndex].node;
            }

            const unsigned short flags = (unsigned short)(event->flags);
            zEffectAnimEntry *const targetEntry = &g_zEffectAnim_EntryList[event->animEntryIndex];

            if ((flags & 0x01u) != 0) {
                zClass_NodePartial *const refNode =
                    event->refNodeIndex > 0
                        ? self->nodeRefList[event->refNodeIndex].node
                        : event->refNodeIndex == -200
                              ? (zClass_NodePartial *)((unsigned int)(
                                    self->resetScratch[0]
                                ))
                              : 0;
                if (refNode != 0) {
                    zVec3 position = event->position;
                    zVec3 orientation = {0};
                    if ((flags & 0x04u) != 0) {
                        gwNode::GetWorldPosAndOrientation(
                            refNode,
                            &position,
                            &orientation
                        );
                        orientation.x += event->orientationOffset.x;
                        orientation.y += event->orientationOffset.y;
                        orientation.z += event->orientationOffset.z;
                    } else {
                        gwNode::TransformPoint(
                            refNode,
                            &position
                        );
                    }

                    childEntry = zEffectAnim::SetTransformRotAndVelocity(
                        targetEntry,
                        boundNode,
                        position.x,
                        position.y,
                        position.z,
                        orientation.x,
                        orientation.y,
                        orientation.z,
                        self->velocityX,
                        self->velocityY,
                        self->velocityZ
                    );
                }
            } else if ((flags & 0x08u) != 0) {
                zVec3 position = event->position;
                childEntry = zEffectAnim::SetPositionRefAndVelocity(
                    targetEntry,
                    boundNode,
                    event->refNodeIndex > 0
                        ? self->nodeRefList[event->refNodeIndex].node
                        : event->refNodeIndex == -200
                              ? (zClass_NodePartial *)((unsigned int)(
                                    self->resetScratch[0]
                                ))
                              : 0,
                    &position,
                    (const zVec3 *)(&self->velocityX)
                );
            } else {
                childEntry = zEffectAnim::SetVelocity(
                    targetEntry,
                    boundNode,
                    self->velocityX,
                    self->velocityY,
                    self->velocityZ
                );
            }

            if (event->runtimeRefIndex >= 0) {
                self->runtimeRefList[event->runtimeRefIndex].cachedChildEntry = childEntry;
            }
        }

        runtime->runState = 1;
    }

    if (((unsigned short)(event->flags) & 0x10u) != 0 && event->runtimeRefIndex >= 0) {
        zEffectAnimEntry *&childEntry =
            self->runtimeRefList[event->runtimeRefIndex].cachedChildEntry;
        if (childEntry != 0 &&
            (childEntry->activationState == 2 || childEntry->activationState == 6)) {
            return 1;
        }
        childEntry = 0;
    }

    runtime->runState = 2;
    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.cleanuplightrefs
 * @recoil-artifact defines .text recoil:function:0x45bf60: zEffect::CleanupLightRefs.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: deactivate and detach runtime light references owned by an animation
 * entry.
 */
int __fastcall CleanupLightRefs(
    zEffectAnimEntry *self
) {
    for (int i = 0; i < self->lightRefCount; ++i) {
        zEffectAnimRuntimeNodeRef *const lightRef =
            self->lightRefList != 0 ? &self->lightRefList[i] : 0;
        if (lightRef == 0) {
            continue;
        }

        zClass_NodePartial *const runtimeNode = lightRef->runtimeNode;
        if (runtimeNode == 0) {
            continue;
        }

        if ((runtimeNode->flags & 0x04) != 0) {
            zClass_Class::gwNodeSetActive(
                runtimeNode,
                0
            );
        }

        if (lightRef->isAttached != 0) {
            zClass_World::RemoveLight(
                g_zEffect_World,
                runtimeNode
            );
            lightRef->isAttached = 0;
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.cleanupsoundrefs
 * @recoil-artifact defines .text recoil:function:0x45bfd0: zEffect::CleanupSoundRefs.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: deactivate and detach runtime sound references owned by an animation
 * entry.
 */
int __fastcall CleanupSoundRefs(
    zEffectAnimEntry *self
) {
    for (int i = 0; i < self->soundRefCount; ++i) {
        zEffectAnimRuntimeNodeRef *const soundRef =
            self->soundRefList != 0 ? &self->soundRefList[i] : 0;
        if (soundRef == 0) {
            continue;
        }

        zClass_NodePartial *const runtimeNode = soundRef->runtimeNode;
        if (runtimeNode == 0) {
            continue;
        }

        if ((runtimeNode->flags & 0x04) != 0) {
            zClass_Class::gwNodeSetActive(
                runtimeNode,
                0
            );
        }

        if (soundRef->isAttached != 0) {
            zClass_World::RemoveSound(
                g_zEffect_World,
                runtimeNode
            );
            soundRef->isAttached = 0;
        }
    }

    return 0;
}

} // namespace zEffect

namespace zEffectAnim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.stop
 * @recoil-artifact defines .text recoil:function:0x45c040: zEffectAnim::Stop.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: initiate stop-delay processing or finalize an active animation entry.
 */
int __fastcall Stop(
    zEffectAnimEntry *self
) {
    if (self == 0) {
        return -1;
    }

    const unsigned char activationState = self->activationState;
    if ((activationState == 2 || activationState == 6) && self->runtimeNode != 0) {
        if (self->triggerBaseValue >= 0.0f) {
            if (g_zEffect_SkipStopDelay != 0) {
                self->triggerCurrentValue = self->triggerBaseValue - kEffectAnimStopDelaySkipBias;
            } else {
                self->triggerCurrentValue = 0.0f;
            }

            if (self->triggerBaseValue > self->triggerCurrentValue) {
                zClass_Class::gwNodeSetActionCallbackTail(
                    self->runtimeNode,
                    (void *)(&RunStopDelayCallback)
                );
                return 0;
            }

            RunStopDelayCallback(self->runtimeNode);
            return 0;
        }

        FinalizeStop(self);
        const unsigned char stateAfterFinalize = self->activationState;
        if (stateAfterFinalize != 5 && stateAfterFinalize != 4) {
            self->activationState = stateAfterFinalize == 6 ? 4 : 3;
        }
    }

    return 0;
}

} // namespace zEffectAnim

namespace zEffect {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handlenamedanimstopevent
 * @recoil-artifact defines .text recoil:function:0x45c100: zEffect::HandleNamedAnimStopEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: stop a named animation entry referenced by an emitter event.
 */
int __fastcall HandleNamedAnimStopEvent(
    zEffectAnimEntry * /*self*/,
    zEffectAnimEmitterEvent *event
) {
    if (event->cachedEntryIndex <= 0) {
        zEffectAnimEntry *entry = g_zEffectAnim_EntryList;
        for (int i = 0; i < g_zEffectAnim_EntryCount; ++i, ++entry) {
            if (strcmp(
                    entry->name,
                    event->animName
                ) == 0) {
                event->cachedEntryIndex = i;
                break;
            }
        }
    }
    const int entryIndex = event->cachedEntryIndex;
    if (entryIndex > 0) {
        zEffectAnim::Stop(&g_zEffectAnim_EntryList[entryIndex]);
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handleemitterplayevent
 * @recoil-artifact defines .text recoil:function:0x45c1a0: zEffect::HandleEmitterPlayEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: trigger node-action activation for a named emitter animation entry.
 */
int __fastcall HandleEmitterPlayEvent(
    zEffectAnimEntry * /*self*/,
    zEffectAnimEmitterEvent *event
) {
    if (event->cachedEntryIndex <= 0) {
        zEffectAnimEntry *entry = g_zEffectAnim_EntryList;
        for (int i = 0; i < g_zEffectAnim_EntryCount; ++i, ++entry) {
            if (strcmp(
                    entry->name,
                    event->animName
                ) == 0) {
                event->cachedEntryIndex = i;
                break;
            }
        }
    }
    const int entryIndex = event->cachedEntryIndex;
    if (entryIndex > 0) {
        zEffect_Anim::NodeActionCallback(
            &g_zEffectAnim_EntryList[entryIndex],
            0
        );
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handleemitterstopevent
 * @recoil-artifact defines .text recoil:function:0x45c240: zEffect::HandleEmitterStopEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: request stop or finish state on a named emitter animation entry.
 */
int __fastcall HandleEmitterStopEvent(
    zEffectAnimEntry * /*self*/,
    zEffectAnimEmitterEvent *event
) {
    if (event->cachedEntryIndex <= 0) {
        zEffectAnimEntry *entry = g_zEffectAnim_EntryList;
        for (int i = 0; i < g_zEffectAnim_EntryCount; ++i, ++entry) {
            if (strcmp(
                    entry->name,
                    event->animName
                ) == 0) {
                event->cachedEntryIndex = i;
                break;
            }
        }
    }
    const int entryIndex = event->cachedEntryIndex;
    if (entryIndex > 0) {
        zEffectAnimEntry *const entry = &g_zEffectAnim_EntryList[entryIndex];
        const unsigned char activationState = entry->activationState;
        if (activationState != 5) {
            entry->activationState = activationState == 2 ? 6 : 4;
        }
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handleemitterresetevent
 * @recoil-artifact defines .text recoil:function:0x45c2f0: zEffect::HandleEmitterResetEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: reset an emitter runtime sequence cursor, elapsed timers, and run
 * state.
 */
int __fastcall HandleEmitterResetEvent(
    zEffectAnimSurfaceRuntime *runtime
) {
    if (runtime == 0) {
        return -1;
    }

    runtime->sequenceElapsedSec = 0.0f;
    runtime->eventElapsedSec = 0.0f;
    runtime->currentEvent = runtime->eventStream;
    runtime->runState = runtime->resetMode;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handleemitterloopevent
 * @recoil-artifact defines .text recoil:function:0x45c310: zEffect::HandleEmitterLoopEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: test loop stop limits, reset the emitter runtime, and continue or
 * stop looping.
 */
int __fastcall HandleEmitterLoopEvent(
    zEffectAnimEntry *self,
    zEffectAnimSurfaceRuntime *runtime,
    zEffectAnimLoopEvent *loopEvent
) {
    const float loopElapsed = runtime->sequenceElapsedSec + runtime->loopElapsedSec;
    ++runtime->loopIterationCount;
    runtime->loopElapsedSec = loopElapsed;

    const unsigned char stopModeFlags = (unsigned char)(loopEvent->stopModeFlags);
    if ((stopModeFlags & 0x01) != 0) {
        const unsigned short loopCountLimit = loopEvent->stopValue.u16;
        if (loopCountLimit != 0xffffu && runtime->loopIterationCount == loopCountLimit) {
            runtime->runState = 2;
            return 2;
        }
    } else if ((stopModeFlags & 0x02) != 0) {
        const float elapsedLimit = loopEvent->stopValue.f32;
        if (elapsedLimit >= 0.0f && loopElapsed >= elapsedLimit) {
            runtime->runState = 2;
            return 2;
        }
    }

    if (self->triggerCurrentValue > kEmitterLoopTriggerClampValue) {
        self->triggerCurrentValue = kEmitterLoopTriggerClampValue;
    }

    HandleEmitterResetEvent(runtime);
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handleconditionalchainevent
 * @recoil-artifact defines .text recoil:function:0x45c3c0: zEffect::HandleConditionalChainEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: evaluate a conditional event chain and skip to the matching branch
 * or chain end.
 */
int __fastcall HandleConditionalChainEvent(
    zEffectAnimEntry *self,
    zEffectAnimSurfaceRuntime *runtime,
    zEffectConditionalEvent *event
) {
    int conditionMask = event->conditionMask;
    zEffectAnimEventValue threshold = event->conditionThreshold;
    int stopAfterGroup = 0;
    int cachedConditionMask = 0;
    float conditionalValue = 0.0f;

    while (stopAfterGroup == 0) {
        int conditionMatched = 0;
        if ((conditionMask & 0x01) != 0) {
            const int randIndex = g_zEffect_RandTableIndex;
            conditionalValue = g_zEffect_RandUnitTable[randIndex];
            g_zEffect_RandTableIndex = (randIndex + 1) % 200;
            if (conditionalValue <= threshold.f32) {
                conditionMatched = 1;
            }
        } else if ((conditionMask & 0x02) != 0) {
            if (g_zEffect_ConditionalRefPosEnabled != 0) {
                if (cachedConditionMask != conditionMask) {
                    conditionalValue = GetConditionalRefPosDistanceSq(self->callbackNode);
                }
                if (conditionalValue <= threshold.f32) {
                    conditionMatched = 1;
                }
            }
        } else if ((conditionMask & 0x08) != 0) {
            if (g_zEffect_ConditionalRefPosEnabled != 0) {
                const zVec3 conditionalRefPos = {g_zEffect_ConditionalRefPosX,
                    g_zEffect_ConditionalRefPosY,
                    g_zEffect_ConditionalRefPosZ};
                int hit = 0;
                if (TraceUpwardHitFromNodeOrPos(0, &conditionalRefPos, &threshold.f32, &hit) == 0 &&
                    hit != 0) {
                    conditionMatched = 1;
                }
            }
        } else if ((conditionMask & 0x10) != 0) {
            int hit = 0;
            if (TraceUpwardHitFromNodeOrPos(
                    self->nodeRefList[event->nodeIndex].node,
                    0,
                    &threshold.f32,
                    &hit
                ) == 0 &&
                hit != 0) {
                conditionMatched = 1;
            }
        } else if ((conditionMask & 0x04) != 0 &&
                   g_zEffect_ConditionalEffectLevel >= threshold.i32) {
            conditionMatched = 1;
        }

        cachedConditionMask = conditionMask;
        if (conditionMatched == 0) {
            unsigned char *nextEvent = (unsigned char *)(runtime->currentEvent);
            unsigned char *const eventStreamEnd =
                (unsigned char *)(runtime->eventStream) + runtime->eventStreamSize;

            do {
                const zEffectAnimEventHeader *const header = (zEffectAnimEventHeader *)(nextEvent);
                nextEvent += header->byteSize;
                runtime->currentEvent = nextEvent;

                const unsigned char eventType = ((zEffectAnimEventHeader *)(nextEvent))->eventType;
                if (eventType == 0x20 || eventType == 0x21 || eventType == 0x22) {
                    break;
                }
            } while (nextEvent < eventStreamEnd);

            zEffectAnimEventHeader *const marker = (zEffectAnimEventHeader *)(nextEvent);
            if (marker->eventType == 0x21) {
                zEffectConditionalEvent *const elseIfEvent = (zEffectConditionalEvent *)(marker);
                threshold = elseIfEvent->conditionThreshold;
                conditionMask = elseIfEvent->conditionMask;
            } else {
                stopAfterGroup = 1;
            }
        } else {
            stopAfterGroup = 1;
        }
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.traceupwardhitfromnodeorpos
 * @recoil-artifact defines .text recoil:function:0x45c530: zEffect::TraceUpwardHitFromNodeOrPos.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: cast upward from a node or explicit position and report whether
 * the trace hit a DI candidate.
 */
int __fastcall TraceUpwardHitFromNodeOrPos(
    zClass_NodePartial *nodeOrNull,
    const zVec3 *positionOrNull,
    const float *rayHeight,
    int *outHit
) {
    zVec3 startPosition = {0};
    if (nodeOrNull != 0) {
        const int result = gwNode::GetWorldPosition(
            nodeOrNull,
            &startPosition
        );
        if (result != 0) {
            return result;
        }
    } else {
        if (positionOrNull == 0) {
            return 1;
        }

        startPosition = *positionOrNull;
    }

    const float height = rayHeight != 0 ? *rayHeight : 50.0f;
    zClass_Class::gwNodeSetRaycastable(
        nodeOrNull,
        0
    );
    zClass_cls_di::SetStopAfterFirstHit(0x40000);
    zClass_cls_di::SetBreakOnFirstCandidate(1);

    PlayerProbeSampleCandidateBuffer rayData = {0};
    const int result = zClass_cls_di::RaycastFindClosest(
        g_zEffect_World,
        &rayData,
        startPosition.x,
        startPosition.y,
        startPosition.z,
        startPosition.x,
        startPosition.y + height,
        startPosition.z
    );

    zClass_cls_di::SetBreakOnFirstCandidate(0);
    zClass_Class::gwNodeSetRaycastable(
        nodeOrNull,
        1
    );

    *outHit = result == 0 && rayData.candidateCount > 0 ? 1 : 0;
    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.getconditionalrefposdistancesq
 * @recoil-artifact defines .text recoil:function:0x45c640: zEffect::GetConditionalRefPosDistanceSq.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: compute squared distance from a node's world position to the
 * current conditional reference position.
 */
float __fastcall GetConditionalRefPosDistanceSq(
    zClass_NodePartial *node
) {
    zVec3 worldPosition = {0};
    if (gwNode::GetWorldPosition(
        node,
        &worldPosition
    ) != 0) {
        return 0.0f;
    }

    const float dx = worldPosition.x - g_zEffect_ConditionalRefPosX;
    const float dy = worldPosition.y - g_zEffect_ConditionalRefPosY;
    const float dz = worldPosition.z - g_zEffect_ConditionalRefPosZ;
    return dx * dx + dy * dy + dz * dz;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.skipconditionalchaintoend
 * @recoil-artifact defines .text recoil:function:0x45c6b0: zEffect::SkipConditionalChainToEnd.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: advance the current event cursor to the end marker of a conditional
 * chain.
 */
int __fastcall SkipConditionalChainToEnd(
    zEffectAnimEntry * /*self*/,
    zEffectAnimSurfaceRuntime *runtime,
    void * /*event*/
) {
    unsigned char *currentEvent = (unsigned char *)(runtime->currentEvent);
    unsigned char *const eventStreamEnd =
        (unsigned char *)(runtime->eventStream) + runtime->eventStreamSize;

    do {
        const zEffectAnimEventHeader *const header = (zEffectAnimEventHeader *)(currentEvent);
        currentEvent += header->byteSize;
        runtime->currentEvent = currentEvent;
    } while (currentEvent[0] != 0x22 && currentEvent < eventStreamEnd);

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handlenoopmarkerevent
 * @recoil-artifact defines .text recoil:function:0x45c6e0: zEffect::HandleNoOpMarkerEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: consume a marker event that has no runtime side effects.
 */
int __fastcall HandleNoOpMarkerEvent(
    zEffectAnimEntry * /*self*/,
    zEffectAnimSurfaceRuntime * /*runtime*/,
    void * /*event*/
) {
    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handlecallbackevent
 * @recoil-artifact defines .text recoil:function:0x45c6f0: zEffect::HandleCallbackEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: dispatch an animation callback event to the entry callback when one
 * is registered.
 */
int __fastcall HandleCallbackEvent(
    zEffectAnimEntry *self,
    zEffectAnimSurfaceRuntime * /*runtime*/,
    zEffectAnimCallbackEvent *event
) {
    if (self->eventCallback != 0) {
        self->eventCallback(
            self,
            self->eventCallbackContext,
            event->value
        );
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handlescreencolorfxevent
 * @recoil-artifact defines .text recoil:function:0x45c710: zEffect::HandleScreenColorFxEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: animate and queue the screen color effect for the current frame.
 */
int __fastcall HandleScreenColorFxEvent(
    zEffectAnimEntry *self,
    zEffectAnimSurfaceRuntime *sequenceRuntime,
    zEffectScreenColorFxEvent *event
) {
    if (self == 0 || sequenceRuntime == 0 || event == 0) {
        return 2;
    }

    int result = 1;
    const float timeSlice = sequenceRuntime->eventElapsedSec <= event->endTimeSec
                                ? g_zEffect_FrameDeltaRemainingSec
                                : g_zEffect_FrameDeltaRemainingSec -
                                      (sequenceRuntime->eventElapsedSec - event->endTimeSec);
    const float colorTime = sequenceRuntime->eventElapsedSec <= event->endTimeSec
                                ? sequenceRuntime->eventElapsedSec
                                : event->endTimeSec;

    float red = event->redSlope * colorTime + event->redBase;
    float green = event->greenSlope * colorTime + event->greenBase;
    float alpha = event->alphaSlope * colorTime + event->alphaBase;
    float blue = event->blueSlope * colorTime + event->blueBase;
    g_zEffect_FrameDeltaRemainingSec -= timeSlice;

    if (sequenceRuntime->eventElapsedSec > event->endTimeSec) {
        red = event->redEnd;
        green = event->greenEnd;
        alpha = event->alphaEnd;
        blue = event->blueEnd;
        result = 2;
    }

    if (red < 0.0f) {
        red = 0.0f;
    } else if (red > 1.0f) {
        red = 1.0f;
    }
    if (green < 0.0f) {
        green = 0.0f;
    } else if (green > 1.0f) {
        green = 1.0f;
    }
    if (alpha < 0.0f) {
        alpha = 0.0f;
    } else if (alpha > 1.0f) {
        alpha = 1.0f;
    }
    if (blue < 0.0f) {
        blue = 0.0f;
    } else if (blue > 1.0f) {
        blue = 1.0f;
    }

    const unsigned int packedColor =
        zVid_PackColorRGB(
            (unsigned char)((int)(red * 255.0f + 0.5f)),
            (unsigned char)((int)(green * 255.0f + 0.5f)),
            (unsigned char)((int)(blue * 255.0f + 0.5f))
        );
    zVideo::FxPass3_SetPrimaryElementParamsLocal(
        packedColor,
        (double)(alpha)
    );
    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handlescreenoverlayfxevent
 * @recoil-artifact defines .text recoil:function:0x45c920: zEffect::HandleScreenOverlayFxEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: animate and queue a screen overlay element anchored by time, screen
 * coordinates, or a projected world node.
 */
int __fastcall HandleScreenOverlayFxEvent(
    zEffectAnimEntry *self,
    zEffectAnimSurfaceRuntime *sequenceRuntime,
    zEffectScreenOverlayFxEvent *event
) {
    if (self == 0 || sequenceRuntime == 0 || event == 0) {
        return 2;
    }

    const unsigned char flags = (unsigned char)(event->flagsAndAnchorNodePacked);
    if (sequenceRuntime->runState == 0 && (flags & 0x08u) != 0) {
        const float referenceDistanceSq =
            GetConditionalRefPosDistanceSq(self->callbackNode);
        int referenceDistanceBits = 0;
        memcpy(
            &referenceDistanceBits,
            &referenceDistanceSq,
            sizeof(referenceDistanceBits)
        );
        referenceDistanceBits =
            (referenceDistanceBits >> 1) + 0x1fc00000;
        float referenceDistance = 0.0f;
        memcpy(
            &referenceDistance,
            &referenceDistanceBits,
            sizeof(referenceDistance)
        );
        const zVec2 screenScale = zMath_Project_GetLastScreenScaleXY();
        event->maxRadiusNearPixels = event->maxRadiusNearWorld / referenceDistance * screenScale.x;
        event->maxRadiusFarPixels = event->maxRadiusFarWorld / referenceDistance * screenScale.x;

        const int surfaceWidth = zVideo::GetSwSurfaceWidth();
        const int surfaceHeight = zVideo::GetSwSurfaceHeight();
        const float radiusCap =
            (float)(surfaceWidth > surfaceHeight ? surfaceWidth : surfaceHeight) * 0.25f;
        if (radiusCap < event->maxRadiusNearPixels) {
            event->maxRadiusNearPixels = radiusCap;
        }
        if (radiusCap < event->maxRadiusFarPixels) {
            event->maxRadiusFarPixels = radiusCap;
        }

        event->maxRadiusPixelsSlope =
            (event->maxRadiusFarPixels - event->maxRadiusNearPixels) / event->endTimeSec;
    }

    const float timeSlice = sequenceRuntime->eventElapsedSec <= event->endTimeSec
                                ? g_zEffect_FrameDeltaRemainingSec
                                : g_zEffect_FrameDeltaRemainingSec -
                                      (sequenceRuntime->eventElapsedSec - event->endTimeSec);
    const float overlayTime = sequenceRuntime->eventElapsedSec <= event->endTimeSec
                                  ? sequenceRuntime->eventElapsedSec
                                  : event->endTimeSec;
    g_zEffect_FrameDeltaRemainingSec -= timeSlice;

    int rectLeftPixels = 0;
    int rectTopPixels = 0;
    if ((flags & 0x02u) != 0) {
        const short anchorNodeRefIndex =
            (short)((unsigned int)(event->flagsAndAnchorNodePacked) >> 16);
        if (anchorNodeRefIndex > 0) {
            zVec3 anchorPoint = event->worldAnchor;
            gwNode::TransformPoint(
                self->nodeRefList[anchorNodeRefIndex].node,
                &anchorPoint
            );

            zVec3 projectedPoint = {0};
            if (zMath::ProjectPointAndClampToScreenClip(
                &anchorPoint,
                &projectedPoint
            ) != 0) {
                return 1;
            }

            rectLeftPixels = (int)(projectedPoint.x + 0.5f);
            rectTopPixels = (int)(projectedPoint.y + 0.5f);
        }
    } else if ((flags & 0x01u) != 0) {
        rectLeftPixels = (int)(event->centerXSlope * overlayTime + event->centerXBase);
        rectTopPixels = (int)(event->centerYSlope * overlayTime + event->centerYBase);
    }

    int maxRadius = (int)(event->maxRadiusPixelsSlope * overlayTime + event->maxRadiusNearPixels);
    int extent = (int)(event->extentSlope * overlayTime + event->extentBase);
    int sinFreqInt = (int)(event->sinFreqSlope * overlayTime + event->sinFreqBase);
    int sinPhaseInt = (int)(event->sinPhaseSlope * overlayTime + event->sinPhaseBase);

    int result = 1;
    if (sequenceRuntime->eventElapsedSec > event->endTimeSec) {
        if ((flags & 0x01u) != 0) {
            rectLeftPixels = (int)(event->centerXEnd);
            rectTopPixels = (int)(event->centerYEnd);
        }

        maxRadius = (int)(event->maxRadiusFarPixels);
        extent = (int)(event->extentEnd);
        sinFreqInt = (int)(event->sinFreqEnd);
        sinPhaseInt = (int)(event->sinPhaseEnd);
        result = 2;
    }

    zVideo::FxPass3_QueueElementLocal(
        rectLeftPixels,
        rectTopPixels,
        0,
        maxRadius,
        extent,
        (float)(sinFreqInt),
        (float)(sinPhaseInt)
    );
    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.handletopmessageevent
 * @recoil-artifact defines .text recoil:function:0x45cbc0: zEffect::HandleTopMessageEvent.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: push a top HUD message using a localized message id or fallback
 * text key from the animation text-id table.
 */
int __fastcall HandleTopMessageEvent(
    zEffectAnimEntry * /*self*/,
    zEffectTopMessageEvent *event
) {
    const int textIdIndex = event->textIdIndex;
    if (textIdIndex >= 0) {
        zEffectAnimTextIdEntry *const textEntry = &g_zEffectAnim_TextIdEntryList[textIdIndex];
        const int messageId = textEntry->messageId;
        const char *message = textEntry->messageKey;
        if (messageId != 0) {
            message = zLoc::GetMessageString((unsigned int)(messageId));
        }

        HudUi::PushTopMessageLine(
            message,
            3.0f
        );
    }

    return 2;
}

} // namespace zEffect

namespace zEffect_Anim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.runsequenceevents
 * @recoil-artifact defines .text recoil:function:0x45cc00: zEffect_Anim::RunSequenceEvents.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: advance a runtime event stream by trigger timing and dispatch
 * eligible sequence records.
 */
int __fastcall RunSequenceEvents(
    zEffectAnimEntry *self,
    zEffectAnimSurfaceRuntime *sequenceRuntime
) {
    if (self == 0 || sequenceRuntime == 0 || sequenceRuntime->currentEvent == 0) {
        if (sequenceRuntime != 0) {
            sequenceRuntime->runState = 2;
        }
        return -1;
    }

    zEffectAnimEventHeader *currentEvent =
        (zEffectAnimEventHeader *)(sequenceRuntime->currentEvent);
    if (sequenceRuntime->runState == 1 ||
        currentEvent > (zEffectAnimEventHeader *)(sequenceRuntime->eventStream)) {
        sequenceRuntime->sequenceElapsedSec += g_zEffect_FrameDeltaRemainingSec;
        sequenceRuntime->eventElapsedSec += g_zEffect_FrameDeltaRemainingSec;
    }

    while (true) {
        currentEvent = (zEffectAnimEventHeader *)(sequenceRuntime->currentEvent);

        int startSatisfied = 0;
        switch (currentEvent->startMode) {
        case 1:
            startSatisfied = self->triggerCurrentValue >= currentEvent->startThreshold;
            break;
        case 2:
            startSatisfied = sequenceRuntime->sequenceElapsedSec >= currentEvent->startThreshold;
            break;
        case 3:
            startSatisfied = sequenceRuntime->eventElapsedSec >= currentEvent->startThreshold;
            break;
        default:
            zError::ReportOld(
                0x400,
                kZeffAnimRunSourceFile,
                0x15fe,
                "Invalid Start Time\n  Animation: %s\n",
                self
            );
            return -1;
        }

        if (startSatisfied == 0) {
            return 0;
        }

        sequenceRuntime->eventElapsedSec = g_zEffect_FrameDeltaRemainingSec;
        if (currentEvent == (zEffectAnimEventHeader *)(sequenceRuntime->eventStream)) {
            sequenceRuntime->sequenceElapsedSec = g_zEffect_FrameDeltaRemainingSec;
        }

        int dispatchResult = 0;
        switch (currentEvent->eventType) {
        case 1:
            dispatchResult = zEffect::HandleSampleRefOffsetEvent(
                self,
                (zEffectAnimRefOffsetEvent *)(currentEvent)
            );
            break;
        case 2:
            dispatchResult = zEffect::HandleSoundEvent(
                self,
                (zEffectAnimSoundEvent *)(currentEvent)
            );
            break;
        case 3:
            dispatchResult = zEffect::HandleEffectTemplateOffsetEvent(
                self,
                (zEffectAnimRefOffsetEvent *)(currentEvent)
            );
            break;
        case 4:
            dispatchResult = zEffect::HandleLightEvent(
                self,
                (zEffectAnimLightEvent *)(currentEvent)
            );
            break;
        case 5:
            dispatchResult = zEffect::HandleLightAnimEvent(
                self,
                sequenceRuntime,
                (zEffectLightRangeSpecularAnimEvent *)(currentEvent)
            );
            break;
        case 6:
            dispatchResult = zEffect::HandleActivateEvent(
                self,
                (zEffectActivateEvent *)(currentEvent)
            );
            break;
        case 7:
            dispatchResult = zEffect::HandlePositionEvent(
                self,
                (zEffectTransformEvent *)(currentEvent)
            );
            break;
        case 8:
            dispatchResult = zEffect::HandleNodeScaleEvent(
                self,
                (zEffectNodeScaleEvent *)(currentEvent)
            );
            break;
        case 9:
            dispatchResult = zEffect::HandleRotationEvent(
                self,
                (zEffectTransformEvent *)(currentEvent)
            );
            break;
        case 0x0a:
            dispatchResult = zEffect::HandleNodeAnimEvent(
                self,
                sequenceRuntime,
                (zEffectNodeAnimEvent *)(currentEvent)
            );
            break;
        case 0x0b:
            dispatchResult = zEffect::AnimateNodeOverTime(
                self,
                sequenceRuntime,
                (zEffectNodeAnimEvent *)(currentEvent)
            );
            break;
        case 0x0c:
            dispatchResult = zEffect_Anim::AdvanceKeyframe(
                self,
                sequenceRuntime,
                (zEffectKeyframeEvent *)(currentEvent)
            );
            break;
        case 0x0d:
            dispatchResult = zEffect_Anim::EvaluateKeyframe(
                self,
                (zEffectEvaluateKeyframeEvent *)(currentEvent)
            );
            break;
        case 0x0e:
            dispatchResult = zEffect_Anim::RunKeyframes(
                self,
                sequenceRuntime,
                (zEffectRunKeyframeEvent *)(currentEvent)
            );
            break;
        case 0x1b:
            dispatchResult = zEffect::HandleEmitterStopEvent(
                self,
                (zEffectAnimEmitterEvent *)(currentEvent)
            );
            break;
        case 0x14:
            dispatchResult = zEffect::HandleCameraParamsEvent(
                self,
                sequenceRuntime,
                (zEffectCameraEvent *)(currentEvent)
            );
            break;
        case 0x15:
            dispatchResult = zEffect::AnimateCameraParamsOverTime(
                self,
                sequenceRuntime,
                (zEffectCameraAnimEvent *)(currentEvent)
            );
            break;
        case 0x0f:
            dispatchResult = zEffect::HandleAddChildEvent(
                self,
                (zEffectParentChildEvent *)(currentEvent)
            );
            break;
        case 0x10:
            dispatchResult = zEffect::HandleRemoveChildEvent(
                self,
                (zEffectParentChildEvent *)(currentEvent)
            );
            break;
        case 0x11:
            dispatchResult = zEffect::HandleAttachEvent(
                self,
                sequenceRuntime,
                (zEffectAttachEvent *)(currentEvent)
            );
            break;
        case 0x12:
            dispatchResult = zEffect::HandleDetachEvent(
                self,
                sequenceRuntime,
                (zEffectBeamDetachEvent *)(currentEvent)
            );
            break;
        case 0x16:
            dispatchResult = zEffect::HandleSurfaceStopEvent(
                self,
                (zEffectSurfaceControlEvent *)(currentEvent)
            );
            break;
        case 0x17:
            dispatchResult = zEffect::HandleSurfacePlayEvent(
                self,
                (zEffectSurfaceControlEvent *)(currentEvent)
            );
            break;
        case 0x18:
            dispatchResult = zEffect::HandleSurfaceRefEvent(
                self,
                sequenceRuntime,
                (zEffectSurfaceRefEvent *)(currentEvent)
            );
            break;
        case 0x24:
            dispatchResult = zEffect::HandleScreenColorFxEvent(
                self,
                sequenceRuntime,
                (zEffectScreenColorFxEvent *)(currentEvent)
            );
            break;
        case 0x25:
            dispatchResult = zEffect::HandleScreenOverlayFxEvent(
                self,
                sequenceRuntime,
                (zEffectScreenOverlayFxEvent *)(currentEvent)
            );
            break;
        case 0x13:
            dispatchResult = zEffect::HandleTransformRefsEvent(
                self,
                (zEffectTransformRefsEvent *)(currentEvent)
            );
            break;
        case 0x19:
            dispatchResult = zEffect::HandleNamedAnimStopEvent(
                self,
                (zEffectAnimEmitterEvent *)(currentEvent)
            );
            break;
        case 0x1a:
            dispatchResult = zEffect::HandleEmitterPlayEvent(
                self,
                (zEffectAnimEmitterEvent *)(currentEvent)
            );
            break;
        case 0x1c:
            dispatchResult = zEffect::HandleFogEvent(
                self,
                (zEffectFogEvent *)(currentEvent)
            );
            break;
        case 0x1e:
            dispatchResult = zEffect::HandleEmitterLoopEvent(
                self,
                sequenceRuntime,
                (zEffectAnimLoopEvent *)(currentEvent)
            );
            break;
        case 0x1f:
            dispatchResult = zEffect::HandleConditionalChainEvent(
                self,
                sequenceRuntime,
                (zEffectConditionalEvent *)(currentEvent)
            );
            break;
        case 0x20:
        case 0x21:
            dispatchResult = zEffect::SkipConditionalChainToEnd(
                self,
                sequenceRuntime,
                currentEvent
            );
            break;
        case 0x22:
        case 0x27:
        case 0x28:
            dispatchResult = zEffect::HandleNoOpMarkerEvent(
                self,
                sequenceRuntime,
                currentEvent
            );
            break;
        case 0x23:
            dispatchResult = zEffect::HandleCallbackEvent(
                self,
                sequenceRuntime,
                (zEffectAnimCallbackEvent *)(currentEvent)
            );
            break;
        case 0x26:
            dispatchResult = zEffect::HandleTopMessageEvent(
                self,
                (zEffectTopMessageEvent *)(currentEvent)
            );
            break;
        default:
            zError::ReportOld(
                0x400,
                kZeffAnimRunSourceFile,
                0x171c,
                "Invalid Sequence Event\n  Animation: %s\n",
                self
            );
            dispatchResult = -1;
            break;
        }
        if (dispatchResult < 0) {
            return dispatchResult;
        }

        sequenceRuntime->runState = (unsigned char)(dispatchResult);
        if (currentEvent->eventType == 0x1e && sequenceRuntime->runState == 0) {
            return 0;
        }

        if (sequenceRuntime->runState == 2) {
            unsigned char *const currentEventBytes =
                (unsigned char *)(sequenceRuntime->currentEvent);
            sequenceRuntime->eventElapsedSec = 0.0f;
            unsigned char *const nextEvent = currentEventBytes + currentEvent->recordSize;
            sequenceRuntime->currentEvent = nextEvent;

            unsigned char *const eventStreamEnd =
                (unsigned char *)(sequenceRuntime->eventStream) + sequenceRuntime->eventStreamSize;
            if (nextEvent < eventStreamEnd) {
                sequenceRuntime->runState = 0;
            } else if (sequenceRuntime->resetMode == 3) {
                zEffect::HandleEmitterResetEvent(sequenceRuntime);
            }
            return 0;
        }

        if (sequenceRuntime->runState != 0) {
            return 0;
        }
    }
}

} // namespace zEffect_Anim

namespace zEffect {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.setanimdebugframetag
 * @recoil-artifact defines .text recoil:function:0x45d000: zEffect::SetAnimDebugFrameTag.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: record the next video frame tick as the current animation debug tag.
 */
int SetAnimDebugFrameTag() {
    const int tag = g_zVideo_FrameTick + 1;
    g_zEffect_Anim_DebugFrameTag = tag;
    return tag;
}

} // namespace zEffect

namespace zEffect_Anim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.runsequence
 * @recoil-artifact defines .text recoil:function:0x45d010: zEffect_Anim::RunSequence.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: run an active animation entry, including conditional/variant gates,
 * sequence stepping, completion callback dispatch, and stop cleanup.
 */
int __fastcall RunSequence(
    zClass_NodePartial *node
) {
    int allSequencesFinished = 1;
    if (node == 0) {
        return 0;
    }

    zEffectAnimEntry *const entry = (zEffectAnimEntry *)(node->callbackContext);
    if (entry == 0) {
        return 0;
    }

    const unsigned int flags = entry->flags;
    if ((flags & 0x0eu) != 0) {
        if (entry->variantCycleDelay > 0) {
            --entry->variantCycleDelay;
            return entry->variantCycleDelay;
        }

        if ((flags & 0x02u) != 0) {
            if (g_zEffect_ConditionalRefPosEnabled == 0) {
                return 0;
            }

            const float distanceSq = zEffect::GetConditionalRefPosDistanceSq(entry->callbackNode);
            if (distanceSq < entry->distRefMinSq || distanceSq >= entry->distRefMaxSq) {
                unsigned char &variantCycleId =
                    *((unsigned char *)(&g_zEffect_VariantCycleId));
                entry->variantCycleDelay = variantCycleId;
                ++variantCycleId;
                const int maxCycleId = ((int)(entry->priority) * 10) >> 2;
                if ((int)(variantCycleId) > maxCycleId) {
                    variantCycleId = 1;
                }
                return 0;
            }
        }

        if ((entry->flags & 0x08u) != 0) {
            if (g_zEffect_VariantOverrideEnabled == 0) {
                return 0;
            }

            memcpy(
                &g_Variant_CurrentTag,
                &g_zEffect_VariantOverridePackedIds,
                sizeof(g_Variant_CurrentTag)
            );
            if (VariantTag::CurrentAllowsId(entry->callbackNode->nodeType) == 0) {
                unsigned char &variantCycleId =
                    *((unsigned char *)(&g_zEffect_VariantCycleId));
                entry->variantCycleDelay = variantCycleId;
                ++variantCycleId;
                if (variantCycleId > 10) {
                    variantCycleId = 1;
                }
                return 0;
            }
        }

        if ((entry->flags & 0x04u) != 0) {
            const unsigned int callbackFlags = entry->callbackNode->flags;
            if ((callbackFlags & 0x80000000u) == 0) {
                unsigned char &variantCycleId =
                    *((unsigned char *)(&g_zEffect_VariantCycleId));
                entry->variantCycleDelay = variantCycleId;
                ++variantCycleId;
                const int maxCycleId = ((int)(entry->priority) * 10) >> 2;
                if ((int)(variantCycleId) > maxCycleId) {
                    variantCycleId = 1;
                }
                return 0;
            }
            entry->callbackNode->flags = callbackFlags & 0x7fffffffu;
        }
    }

    entry->triggerCurrentValue += g_FrameDeltaTimeSec;
    zEffectAnimSurfaceRuntime *sequenceRuntime = entry->runtimeList;
    for (int i = 0; i < entry->runtimeSequenceCount; ++i, ++sequenceRuntime) {
        g_zEffect_FrameDeltaRemainingSec = g_FrameDeltaTimeSec;
        const unsigned char runState = sequenceRuntime->runState;
        if (runState == 0 || runState == 1) {
            if (RunSequenceEvents(
                entry,
                sequenceRuntime
            ) != 0) {
                entry->activationState = 5;
                zError::ReportOld(
                    0x400,
                    kZeffAnimRunSourceFile,
                    0x180f,
                    "Corrupt animation:\n  Animation: %s; Sequence: %s\n",
                    entry,
                    sequenceRuntime
                );
            }
        }

        const unsigned char runStateAfterDispatch = sequenceRuntime->runState;
        if (runStateAfterDispatch == 0 || runStateAfterDispatch == 1) {
            allSequencesFinished = 0;
        }
    }

    if (allSequencesFinished != 0) {
        if (entry->eventCallback != 0) {
            entry->eventCallback(
                entry,
                entry->eventCallbackContext,
                0
            );
        }
        zEffectAnim::Stop(entry);
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.capturenodestates
 * @recoil-artifact defines .text recoil:function:0x45d240: zEffect_Anim::CaptureNodeStates.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: save active and transform state for each tracked node before an
 * animation reset.
 */
int __fastcall CaptureNodeStates(
    zEffectAnimEntry *self
) {
    if (self == 0) {
        return -1;
    }

    for (int i = 0; i < self->trackedNodeCount; ++i) {
        zEffectAnimTrackedNode *const tracked = &self->trackedNodeList[i];
        zClass_NodePartial *const node = tracked->trackedNode;
        if (node == 0) {
            continue;
        }

        zEffectAnimCapturedNodeState *const state = &tracked->capturedState;
        state->activeFlag = (node->flags >> 2) & 1;
        if (node->classId != 5) {
            continue;
        }

        zClass_Object3DDataPartial *const objectData =
            (zClass_Object3DDataPartial *)(node->classData);
        state->usesCachedMatrix = (objectData->flags >> 4) & 1;
        if (state->usesCachedMatrix != 0) {
            memcpy(
                state->transformSnapshot,
                zClass_Object3D::gwObject3DGetMatrixPtr(node),
                sizeof(state->transformSnapshot)
            );
        } else {
            zClass_Object3D::gwObject3DGetPosition(
                node,
                &state->transformSnapshot[0],
                &state->transformSnapshot[1],
                &state->transformSnapshot[2]
            );
            zClass_Object3D::gwObject3DGetRotation(
                node,
                &state->transformSnapshot[3],
                &state->transformSnapshot[4],
                &state->transformSnapshot[5]
            );
            zClass_Object3D::gwObject3DGetScale(
                node,
                &state->transformSnapshot[6],
                &state->transformSnapshot[7],
                &state->transformSnapshot[8]
            );
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.restorenodestates
 * @recoil-artifact defines .text recoil:function:0x45d310: zEffect_Anim::RestoreNodeStates.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: restore captured node active, transform, and zDi blend state for an
 * animation entry.
 */
int __fastcall RestoreNodeStates(
    zEffectAnimEntry *self
) {
    if (self == 0) {
        return -1;
    }

    for (int i = 0; i < self->trackedNodeCount; ++i) {
        zEffectAnimTrackedNode *const tracked = &self->trackedNodeList[i];
        zClass_NodePartial *const node = tracked->trackedNode;
        if (node == 0) {
            continue;
        }

        zEffectAnimCapturedNodeState *const state = &tracked->capturedState;
        zClass_Class::gwNodeSetActive(
            node,
            state->activeFlag
        );
        if (node->classId == 5) {
            if (state->usesCachedMatrix != 0) {
                zClass_Object3D::gwObject3DSetMatrix(
                    node,
                    state->transformSnapshot
                );
            } else {
                zClass_Object3D::gwObject3DSetPosition(
                    node,
                    state->transformSnapshot[0],
                    state->transformSnapshot[1],
                    state->transformSnapshot[2]
                );
                zClass_Object3D::gwObject3DSetRotation(
                    node,
                    state->transformSnapshot[3],
                    state->transformSnapshot[4],
                    state->transformSnapshot[5]
                );
                zClass_Object3D::gwObject3DSetScale(
                    node,
                    state->transformSnapshot[6],
                    state->transformSnapshot[7],
                    state->transformSnapshot[8]
                );
            }
        }

        zDiPartial *const di = (zDiPartial *)(node->userDataOrDiRef);
        if (di != 0) {
            di->flags &= ~0x08;
            di->blendScale = 0.0f;
        }
    }

    return 0;
}

} // namespace zEffect_Anim

namespace zEffectAnim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.finalizestop
 * @recoil-artifact defines .text recoil:function:0x45d3d0: zEffectAnim::FinalizeStop.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: detach active runtime state, clear cleanup references, and settle the
 * stopped activation state.
 */
int __fastcall FinalizeStop(
    zEffectAnimEntry *self
) {
    if (self == 0 || self->activationState == 5) {
        return -1;
    }

    if ((self->flags & kEffectAnimWorldChildAttachedFlag) != 0) {
        if (zClass_World::RemoveChildAtGrid(
            g_zEffect_World,
            self->boundNode
        ) != 0) {
            return -1;
        }

        self->flags &= ~kEffectAnimWorldChildAttachedFlag;
    }

    zEffect::CleanupLightRefs(self);
    zEffect::CleanupSoundRefs(self);

    for (int i = 0; i < self->runtimeRefCount; ++i) {
        zEffectAnimRuntimeRef *const runtimeRef = &self->runtimeRefList[i];
        zEffectAnimEntry *const cachedChildEntry = runtimeRef->cachedChildEntry;
        if (cachedChildEntry != 0 && runtimeRef->stopCachedChildOnCleanup != 0) {
            Stop(cachedChildEntry);
        }
        runtimeRef->cachedChildEntry = 0;
    }

    if (self->runtimeNode != 0) {
        zClass_Class::gwNodeSetActionCallback(
            self->runtimeNode,
            0
        );
    }

    const unsigned char activationState = self->activationState;
    if (activationState != 5 && activationState != 4) {
        self->activationState = activationState == 6 ? 4 : 1;
    }

    memcpy(
        &self->activationCountdown,
        &self->triggerContext,
        sizeof(self->activationCountdown)
    );
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.runstopsequencecallback
 * @recoil-artifact defines .text recoil:function:0x45d4c0: zEffectAnim::RunStopSequenceCallback.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: advance the stop sequence until no runnable events remain, then
 * finalize the stopped entry.
 */
int __fastcall RunStopSequenceCallback(
    zClass_NodePartial *node
) {
    int stopSequenceFinished = 1;
    if (node == 0) {
        return -1;
    }

    zEffectAnimEntry *const entry = (zEffectAnimEntry *)(node->callbackContext);
    if (entry == 0) {
        return -1;
    }

    entry->triggerCurrentValue += g_FrameDeltaTimeSec;
    if (entry->surfacePrimary.eventStream != 0) {
        g_zEffect_FrameDeltaRemainingSec = g_FrameDeltaTimeSec;
        const unsigned char runState = entry->surfacePrimary.runState;
        if (runState == 0 || runState == 1) {
            if (zEffect_Anim::RunSequenceEvents(
                entry,
                &entry->surfacePrimary
            ) != 0) {
                entry->activationState = 5;
                zError::ReportOld(
                    0x400,
                    kZeffAnimRunSourceFile,
                    0x196d,
                    "Corrupt animation:\n  Animation: %s; Sequence: %s\n",
                    entry,
                    &entry->surfacePrimary
                );
            }
        }

        const unsigned char runStateAfterDispatch = entry->surfacePrimary.runState;
        if (runStateAfterDispatch == 0 || runStateAfterDispatch == 1) {
            stopSequenceFinished = 0;
        }
    }

    if (stopSequenceFinished != 0) {
        FinalizeStop(entry);
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.stopandcleanup
 * @recoil-artifact defines .text recoil:function:0x45d570: zEffectAnim::StopAndCleanup.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: stop or rebind an animation entry and run immediate or sequenced
 * cleanup for its runtime state.
 */
int __fastcall StopAndCleanup(
    zEffectAnimEntry *self,
    zClass_NodePartial *targetNode,
    int immediateCleanup
) {
    if (self == 0 || self->activationState == 5) {
        return -1;
    }

    zEffectAnimEntry *entry = self;
    unsigned char activationState = self->activationState;
    if (targetNode != 0 && self->boundNode != targetNode) {
        while (entry->runtimeSibling != 0 && entry->activationState == 2) {
            entry = entry->runtimeSibling;
        }

        if (entry->activationState == 2) {
            zEffectAnimEntry *const clonedEntry = CloneEntryForNode(
                entry,
                targetNode
            );
            entry->runtimeSibling = clonedEntry;
            if (clonedEntry == 0) {
                return -1;
            }
            entry = clonedEntry;
        }

        if (RebindEntryToNode(
            entry,
            targetNode
        ) == 0) {
            return -1;
        }
    }

    if (activationState == 2 || activationState == 6) {
        Stop(self);
    }

    entry->activationState = 0;
    if (immediateCleanup != 0) {
        entry->flags |= 0x40u;
    } else {
        entry->flags &= ~0x40u;
    }

    if (entry->activationState != 1) {
        if ((entry->flags & 0x40u) != 0) {
            zEffect_Anim::RestoreNodeStates(entry);
        }

        if (entry->surfacePrimary.eventStream != 0) {
            zEffect::HandleEmitterResetEvent(&entry->surfacePrimary);
            zEffect_Anim::RunSequenceEvents(
                entry,
                &entry->surfacePrimary
            );
            const unsigned char runState = entry->surfacePrimary.runState;
            if (runState == 0 || runState == 1) {
                zClass_Class::gwNodeSetActionCallbackTail(
                    entry->runtimeNode,
                    (void *)(&RunStopSequenceCallback)
                );
                return 0;
            }
        }

        FinalizeStop(entry);
    }

    return 0;
}

} // namespace zEffectAnim

namespace zEffect_Anim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.nodeactioncallback
 * @recoil-artifact defines .text recoil:function:0x45d6b0: zEffect_Anim::NodeActionCallback.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: handle runtime node action callbacks by stopping and cleaning up the
 * owning animation entry.
 */
int __fastcall NodeActionCallback(
    zEffectAnimEntry *self,
    zClass_NodePartial *rootNode
) {
    return zEffectAnim::StopAndCleanup(
        self,
        rootNode,
        1
    );
}

} // namespace zEffect_Anim

namespace zEffectAnim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.resetfornode
 * @recoil-artifact defines .text recoil:function:0x45d6c0: zEffectAnim::ResetForNode.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim.c.
 * Purpose: restore an animation entry to its bound node and reset its runtime
 * emitter state.
 */
int __fastcall ResetForNode(
    zEffectAnimEntry *self
) {
    if (self == 0) {
        return -1;
    }

    zClass_NodePartial *const rootNode = zClass_Class::gwNodeGetRoot(self->boundNode);
    if (rootNode == 0) {
        return -1;
    }

    if (rootNode->classId == 2 || rootNode->classId == 1) {
        self->flags &= ~0x00000100u;
    } else {
        if (zClass_World::AddChildAtGrid(
            g_zEffect_World,
            self->boundNode
        ) != 0) {
            return -1;
        }
        self->flags |= 0x00000100u;
    }

    self->triggerCurrentValue = 0.0f;
    zEffect_Anim::CaptureNodeStates(self);

    zEffectAnimSurfaceRuntime *runtime = self->runtimeList;
    for (int i = 0; i < self->runtimeSequenceCount; ++i, ++runtime) {
        runtime->loopIterationCount = 0;
        runtime->loopElapsedSec = 0.0f;
        zEffect::HandleEmitterResetEvent(runtime);
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.runstopdelaycallback
 * @recoil-artifact defines .text recoil:function:0x45d770: zEffectAnim::RunStopDelayCallback.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_run.c.
 * Purpose: accumulate stop-delay time and trigger cleanup once the delay expires.
 */
int __fastcall RunStopDelayCallback(
    zClass_NodePartial *node
) {
    zEffectAnimEntry *const entry = node != 0 ? (zEffectAnimEntry *)(node->callbackContext) : 0;
    if (entry == 0) {
        return 0;
    }

    entry->triggerCurrentValue += g_FrameDeltaTimeSec;
    if (entry->triggerCurrentValue >= entry->triggerBaseValue) {
        return zEffect_Anim::NodeActionCallback(
            entry,
            0
        );
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.resetactivationprereqcount
 * @recoil-artifact defines .text recoil:function:0x45d7a0: zEffectAnim::ResetActivationPrereqCount.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim.c.
 * Purpose: clear the activation prerequisite counter for an animation entry.
 */
void __fastcall ResetActivationPrereqCount(
    zEffectAnimEntry *self
) {
    self->activationPrereqCount = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.settransformrotandvelocity
 * @recoil-artifact defines .text recoil:function:0x45d7b0: zEffectAnim::SetTransformRotAndVelocity.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim.c.
 * Purpose: activate an animation entry, apply transform and velocity state,
 * and queue the command type 1 activation record when recording is enabled.
 */
zEffectAnimEntry *__fastcall SetTransformRotAndVelocity(
    zEffectAnimEntry *self,
    zClass_NodePartial *boundNode,
    float posX,
    float posY,
    float posZ,
    float rotX,
    float rotY,
    float rotZ,
    float velocityX,
    float velocityY,
    float velocityZ
) {
    zEffectAnimEntry *const activatedEntry = ActivateRuntime(
        self,
        boundNode
    );
    if (activatedEntry == 0) {
        return 0;
    }

    if ((activatedEntry->flags & kEffectAnimWorldChildAttachedFlag) != 0) {
        zClass_NodePartial *const activatedBoundNode = activatedEntry->boundNode;
        if (activatedBoundNode->classId == 1) {
            zClass_Camera::gwCameraSetTarget(
                activatedBoundNode,
                posX,
                posY,
                posZ
            );
            if ((activatedEntry->flags & 0x00000200u) == 0) {
                zClass_Camera::gwCameraSetPosition(
                    activatedEntry->boundNode,
                    rotX,
                    rotY,
                    rotZ
                );
            }
        } else if (activatedBoundNode->classId == 5) {
            zClass_Object3D::gwObject3DSetPosition(
                activatedBoundNode,
                posX,
                posY,
                posZ
            );
            if ((activatedEntry->flags & 0x00000200u) == 0) {
                zClass_Object3D::gwObject3DSetRotation(
                    activatedEntry->boundNode,
                    rotX,
                    rotY,
                    rotZ
                );
            }
        }
    }

    if (fabs(velocityX) > kEffectAnimVelocityEpsilon ||
        fabs(velocityY) > kEffectAnimVelocityEpsilon ||
        fabs(velocityZ) > kEffectAnimVelocityEpsilon) {
        activatedEntry->flags |= 0x80u;
    } else {
        activatedEntry->flags &= ~0x80u;
    }

    activatedEntry->velocityX = velocityX;
    activatedEntry->velocityZ = velocityZ;
    activatedEntry->velocityY = velocityY;
    memset(
        activatedEntry->resetScratch,
        0,
        sizeof(activatedEntry->resetScratch)
    );

    QueueCmdType1TransformRotVelocity(
        self,
        boundNode,
        posX,
        posY,
        posZ,
        rotX,
        rotY,
        rotZ,
        velocityX,
        velocityY,
        velocityZ
    );
    return activatedEntry;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.activateruntime
 * @recoil-artifact defines .text recoil:function:0x45d930: zEffectAnim::ActivateRuntime.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim.c.
 * Purpose: bind or clone an animation entry, reset its runtime node state, and
 * install the sequence callback for active playback.
 */
zEffectAnimEntry *__fastcall ActivateRuntime(
    zEffectAnimEntry *self,
    zClass_NodePartial *targetNode
) {
    int immediateCleanup = 0;
    if (self == 0) {
        return 0;
    }

    const unsigned char activationState = self->activationState;
    if (activationState == 6 || activationState == 4 || activationState == 5) {
        return 0;
    }

    zEffectAnimEntry *entryToActivate = self;
    if (activationState == 3 || activationState == 2) {
        if (fabs(self->triggerBaseValue - kEffectAnimActivationSentinel) <
            kEffectAnimActivationSentinelTolerance) {
            return 0;
        }

        if (activationState == 2) {
            if ((self->flags & kEffectAnimWorldChildAttachedFlag) != 0) {
                while (
                    entryToActivate->runtimeSibling != 0 && entryToActivate->activationState == 2) {
                    entryToActivate = entryToActivate->runtimeSibling;
                }

                if (entryToActivate->activationState == 2) {
                    zEffectAnimEntry *const clonedEntry =
                        CloneEntryForNode(
                            entryToActivate,
                            targetNode
                        );
                    entryToActivate->runtimeSibling = clonedEntry;
                    if (clonedEntry == 0) {
                        return 0;
                    }
                    entryToActivate = clonedEntry;
                    immediateCleanup = 1;
                }
            } else {
                if (targetNode == 0) {
                    return 0;
                }

                zEffectAnimEntry *siblingTail = self;
                zEffectAnimEntry *matchedEntry = 0;
                int targetNodeMatched = 0;
                for (zEffectAnimEntry *cursor = self; cursor != 0;
                    cursor = cursor->runtimeSibling) {
                    if (cursor->activationState == 2) {
                        if (cursor->boundNode == targetNode) {
                            targetNodeMatched = 1;
                            break;
                        }
                    } else {
                        matchedEntry = cursor;
                    }

                    siblingTail = cursor;
                }

                if (targetNodeMatched != 0) {
                    return 0;
                }

                if (matchedEntry == 0) {
                    matchedEntry = CloneEntryForNode(
                        siblingTail,
                        targetNode
                    );
                    siblingTail->runtimeSibling = matchedEntry;
                    if (matchedEntry == 0) {
                        return 0;
                    }
                }

                entryToActivate = matchedEntry;
            }
        }
    }

    if (targetNode != 0 && RebindEntryToNode(
        entryToActivate,
        targetNode
    ) == 0) {
        return 0;
    }

    if (CheckActivationPrereqs(entryToActivate) == 0) {
        return 0;
    }

    if ((entryToActivate->flags & 0x20u) != 0) {
        StopAndCleanup(
            entryToActivate,
            0,
            immediateCleanup
        );
    }

    if (ResetForNode(entryToActivate) != 0) {
        return 0;
    }

    if (entryToActivate->runtimeNode == 0) {
        entryToActivate->runtimeNode = zClass_Object3D::gwObject3DInit();

        char runtimeNodeName[0x24];
        sprintf(
            runtimeNodeName,
            "_%s",
            entryToActivate->name
        );
        zClass_Class::gwNodeSetName(
            entryToActivate->runtimeNode,
            runtimeNodeName
        );
        if (entryToActivate->runtimeNode == 0) {
            return 0;
        }
        zClass_Class::gwNodeSetPriority(
            entryToActivate->runtimeNode,
            entryToActivate->priority
        );
    }

    entryToActivate->runtimeNode->callbackContext = (zClass_NodePartial *)(entryToActivate);
    zClass_Class::gwNodeSetActionCallbackTail(
        entryToActivate->runtimeNode,
        (void *)(&zEffect_Anim::RunSequence)
    );
    entryToActivate->activationState = 2;
    return entryToActivate;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.checkactivationprereqs
 * @recoil-artifact defines .text recoil:function:0x45db20: zEffectAnim::CheckActivationPrereqs (zeff_anim.c)
 * Purpose: Resolve and evaluate activation prerequisites for an animation entry.
 */
int __fastcall CheckActivationPrereqs(
    zEffectAnimEntry *self
) {
    if (self->activationPrereqCount == 0) {
        return 1;
    }

    int matchedPrereqTotal = 0;
    for (int i = 0; i < self->activationPrereqCount; ++i) {
        zEffectAnimActivationPrereq *const prereq = &self->activationPrereqList[i];

        if (prereq->mode == 1) {
            if (prereq->targetEntry == 0) {
                zEffectAnimEntry *candidate = g_zEffectAnim_EntryList;
                for (int entryIndex = 0; entryIndex < g_zEffectAnim_EntryCount;
                    ++entryIndex, ++candidate) {
                    if (strcmp(
                        candidate->name,
                        prereq->targetName
                    ) == 0) {
                        prereq->targetEntry = candidate;
                        break;
                    }
                }
            }

            zEffectAnimEntry *const targetEntry = prereq->targetEntry;
            if (targetEntry == 0) {
                return 0;
            }

            if (targetEntry->activationState == 1) {
                if (prereq->requireMatch == 0) {
                    return 0;
                }
            } else if (prereq->requireMatch != 0) {
                ++matchedPrereqTotal;
            }
        } else if (prereq->mode == 2) {
            zClass_NodePartial *const targetNode = prereq->targetNode;
            if (targetNode != 0) {
                const int nodeFlagValue = (targetNode->flags >> 2) & 1;
                int expectedValue = 0;
                memcpy(
                    &expectedValue,
                    prereq->targetName,
                    sizeof(expectedValue)
                );
                if (nodeFlagValue == expectedValue) {
                    if (prereq->requireMatch != 0) {
                        ++matchedPrereqTotal;
                    }
                } else if (prereq->requireMatch == 0) {
                    return 0;
                }
            }
        }
    }

    return matchedPrereqTotal >= self->activationPrereqMinimumMatchCount ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.settransformrotandvelocity-thunk
 * @recoil-artifact defines .text recoil:function:0x45dc70: zEffectAnim::SetTransformRotAndVelocity_Thunk.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim.c.
 * Purpose: forward a saved transform, rotation, and velocity activation
 * command to SetTransformRotAndVelocity using the retail thunk ABI.
 */
zEffectAnimEntry *__fastcall SetTransformRotAndVelocity_Thunk(
    zEffectAnimEntry *self,
    zClass_NodePartial *boundNode,
    float posX,
    float posY,
    float posZ,
    float rotX,
    float rotY,
    float rotZ,
    float velocityX,
    float velocityY,
    float velocityZ
) {
    return SetTransformRotAndVelocity(
        self,
        boundNode,
        posX,
        posY,
        posZ,
        rotX,
        rotY,
        rotZ,
        velocityX,
        velocityY,
        velocityZ
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.setvelocity
 * @recoil-artifact defines .text recoil:function:0x45dcb0: zEffectAnim::SetVelocity.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim.c.
 * Purpose: activate an animation entry, reset bound transform state, store
 * velocity, and queue the type-2 activation record.
 */
zEffectAnimEntry *__fastcall SetVelocity(
    zEffectAnimEntry *self,
    zClass_NodePartial *boundNode,
    float velocityX,
    float velocityY,
    float velocityZ
) {
    zEffectAnimEntry *const activatedEntry = ActivateRuntime(
        self,
        boundNode
    );
    if (activatedEntry == 0) {
        return 0;
    }

    if ((activatedEntry->flags & kEffectAnimWorldChildAttachedFlag) != 0) {
        zClass_NodePartial *const activatedBoundNode =
            activatedEntry->boundNode;
        if (activatedBoundNode->classId == 1) {
            zClass_Camera::gwCameraSetTarget(
                activatedBoundNode,
                0.0f,
                0.0f,
                0.0f
            );
            if ((activatedEntry->flags & 0x00000200u) == 0) {
                zClass_Camera::gwCameraSetPosition(
                    activatedBoundNode,
                    0.0f,
                    0.0f,
                    0.0f
                );
            }
        } else if (activatedBoundNode->classId == 5) {
            zClass_Object3D::gwObject3DSetPosition(
                activatedBoundNode,
                0.0f,
                0.0f,
                0.0f
            );
            if ((activatedEntry->flags & 0x00000200u) == 0) {
                zClass_Object3D::gwObject3DSetRotation(
                    activatedBoundNode,
                    0.0f,
                    0.0f,
                    0.0f
                );
            }
        }
    }
    if (fabs(velocityX) > kEffectAnimVelocityEpsilon ||
        fabs(velocityY) > kEffectAnimVelocityEpsilon ||
        fabs(velocityZ) > kEffectAnimVelocityEpsilon) {
        activatedEntry->flags |= 0x80u;
    } else {
        activatedEntry->flags &= ~0x80u;
    }

    activatedEntry->velocityX = velocityX;
    activatedEntry->velocityZ = velocityZ;
    activatedEntry->velocityY = velocityY;
    memset(
        activatedEntry->resetScratch,
        0,
        sizeof(activatedEntry->resetScratch)
    );

    QueueCmdType2Velocity(
        self,
        boundNode,
        velocityX,
        velocityY,
        velocityZ
    );
    return activatedEntry;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.setvelocity-thunk
 * @recoil-artifact defines .text recoil:function:0x45dde0: zEffectAnim::SetVelocity_Thunk.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim.c.
 * Purpose: forward the saved activation command to SetVelocity using the
 * retail stack-cleanup thunk ABI.
 */
zEffectAnimEntry *__fastcall SetVelocity_Thunk(
    zEffectAnimEntry *self,
    zClass_NodePartial *boundNode,
    float velocityX,
    float velocityY,
    float velocityZ
) {
    return SetVelocity(
        self,
        boundNode,
        velocityX,
        velocityY,
        velocityZ
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.setpositionrefandvelocity
 * @recoil-artifact defines .text recoil:function:0x45de00: zEffectAnim::SetPositionRefAndVelocity.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim.c.
 * Purpose: activate an entry with a position reference and optional velocity
 * vector.
 */
zEffectAnimEntry *__fastcall SetPositionRefAndVelocity(
    zEffectAnimEntry *self,
    zClass_NodePartial *boundNode,
    zClass_NodePartial *refNode,
    const zVec3 *refVec,
    const zVec3 *velocityVec
) {
    zEffectAnimEntry *const activatedEntry = ActivateRuntime(
        self,
        boundNode
    );
    if (activatedEntry == 0) {
        return 0;
    }

    if ((activatedEntry->flags & kEffectAnimWorldChildAttachedFlag) != 0) {
        zClass_NodePartial *const activatedBoundNode =
            activatedEntry->boundNode;
        if (activatedBoundNode->classId == 1) {
            zClass_Camera::gwCameraSetTarget(
                activatedBoundNode,
                0.0f,
                0.0f,
                0.0f
            );
            if ((activatedEntry->flags & 0x00000200u) == 0) {
                zClass_Camera::gwCameraSetPosition(
                    activatedBoundNode,
                    0.0f,
                    0.0f,
                    0.0f
                );
            }
        } else if (activatedBoundNode->classId == 5) {
            zClass_Object3D::gwObject3DSetPosition(
                activatedBoundNode,
                0.0f,
                0.0f,
                0.0f
            );
            if ((activatedEntry->flags & 0x00000200u) == 0) {
                zClass_Object3D::gwObject3DSetRotation(
                    activatedBoundNode,
                    0.0f,
                    0.0f,
                    0.0f
                );
            }
        }
    }
    activatedEntry->resetScratch[0] = (unsigned int)((unsigned int)(refNode));
    if (refVec != 0) {
        memcpy(
            &activatedEntry->resetScratch[1],
            &refVec->x,
            sizeof(refVec->x)
        );
        memcpy(
            &activatedEntry->resetScratch[2],
            &refVec->y,
            sizeof(refVec->y)
        );
        memcpy(
            &activatedEntry->resetScratch[3],
            &refVec->z,
            sizeof(refVec->z)
        );
    } else {
        activatedEntry->resetScratch[1] = 0;
        activatedEntry->resetScratch[2] = 0;
        activatedEntry->resetScratch[3] = 0;
    }
    activatedEntry->resetScratch[4] = 0;
    activatedEntry->resetScratch[5] = 0;
    activatedEntry->resetScratch[6] = 0;
    activatedEntry->resetScratch[7] = 0;

    if (velocityVec != 0 &&
        (fabs(velocityVec->x) > kEffectAnimVelocityEpsilon ||
         fabs(velocityVec->y) > kEffectAnimVelocityEpsilon ||
         fabs(velocityVec->z) > kEffectAnimVelocityEpsilon)) {
        activatedEntry->flags |= 0x80u;
        activatedEntry->velocityX = velocityVec->x;
        activatedEntry->velocityY = velocityVec->y;
        activatedEntry->velocityZ = velocityVec->z;
    } else {
        activatedEntry->flags &= ~0x80u;
        activatedEntry->velocityZ = 0.0f;
        activatedEntry->velocityY = 0.0f;
        activatedEntry->velocityX = 0.0f;
    }

    QueueCmdType3PositionRefAndVelocity(
        self,
        boundNode,
        refNode,
        refVec,
        velocityVec
    );
    return activatedEntry;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.setpositionrefandvelocity-thunk
 * @recoil-artifact defines .text recoil:function:0x45df70: zEffectAnim::SetPositionRefAndVelocity_Thunk.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim.c.
 * Purpose: forward the saved activation command to SetPositionRefAndVelocity
 * using the retail stack-cleanup thunk ABI.
 */
zEffectAnimEntry *__fastcall SetPositionRefAndVelocity_Thunk(
    zEffectAnimEntry *self,
    zClass_NodePartial *boundNode,
    zClass_NodePartial *refNode,
    const zVec3 *refVec,
    const zVec3 *velocityVec
) {
    return SetPositionRefAndVelocity(
        self,
        boundNode,
        refNode,
        refVec,
        velocityVec
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.settransformrefs
 * @recoil-artifact defines .text recoil:function:0x45df90: zEffectAnim::SetTransformRefs.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim.c.
 * Purpose: activate an entry with two stored transform references.
 */
zEffectAnimEntry *__fastcall SetTransformRefs(
    zEffectAnimEntry *self,
    zClass_NodePartial *boundNode,
    zClass_NodePartial *refNodeA,
    const zVec3 *refVecA,
    zClass_NodePartial *refNodeB,
    const zVec3 *refVecB
) {
    zEffectAnimEntry *const activatedEntry = ActivateRuntime(
        self,
        boundNode
    );
    if (activatedEntry == 0) {
        return 0;
    }

    if ((activatedEntry->flags & kEffectAnimWorldChildAttachedFlag) != 0) {
        zClass_NodePartial *const activatedBoundNode =
            activatedEntry->boundNode;
        if (activatedBoundNode->classId == 1) {
            zClass_Camera::gwCameraSetTarget(
                activatedBoundNode,
                0.0f,
                0.0f,
                0.0f
            );
            if ((activatedEntry->flags & 0x00000200u) == 0) {
                zClass_Camera::gwCameraSetPosition(
                    activatedBoundNode,
                    0.0f,
                    0.0f,
                    0.0f
                );
            }
        } else if (activatedBoundNode->classId == 5) {
            zClass_Object3D::gwObject3DSetPosition(
                activatedBoundNode,
                0.0f,
                0.0f,
                0.0f
            );
            if ((activatedEntry->flags & 0x00000200u) == 0) {
                zClass_Object3D::gwObject3DSetRotation(
                    activatedBoundNode,
                    0.0f,
                    0.0f,
                    0.0f
                );
            }
        }
    }
    activatedEntry->resetScratch[0] = (unsigned int)((unsigned int)(refNodeA));
    if (refVecA != 0) {
        memcpy(
            &activatedEntry->resetScratch[1],
            &refVecA->x,
            sizeof(refVecA->x)
        );
        memcpy(
            &activatedEntry->resetScratch[2],
            &refVecA->y,
            sizeof(refVecA->y)
        );
        memcpy(
            &activatedEntry->resetScratch[3],
            &refVecA->z,
            sizeof(refVecA->z)
        );
    } else {
        activatedEntry->resetScratch[1] = 0;
        activatedEntry->resetScratch[2] = 0;
        activatedEntry->resetScratch[3] = 0;
    }
    activatedEntry->resetScratch[4] = (unsigned int)((unsigned int)(refNodeB));
    if (refVecB != 0) {
        memcpy(
            &activatedEntry->resetScratch[5],
            &refVecB->x,
            sizeof(refVecB->x)
        );
        memcpy(
            &activatedEntry->resetScratch[6],
            &refVecB->y,
            sizeof(refVecB->y)
        );
        memcpy(
            &activatedEntry->resetScratch[7],
            &refVecB->z,
            sizeof(refVecB->z)
        );
    } else {
        activatedEntry->resetScratch[5] = 0;
        activatedEntry->resetScratch[6] = 0;
        activatedEntry->resetScratch[7] = 0;
    }

    QueueCmdType4TransformRefs(
        self,
        boundNode,
        refNodeA,
        refVecA,
        refNodeB,
        refVecB
    );
    return activatedEntry;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.settransformrefs-thunk
 * @recoil-artifact defines .text recoil:function:0x45e0b0: zEffectAnim::SetTransformRefs_Thunk.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim.c.
 * Purpose: forward the saved activation command to SetTransformRefs using the
 * retail stack-cleanup thunk ABI.
 */
zEffectAnimEntry *__fastcall SetTransformRefs_Thunk(
    zEffectAnimEntry *self,
    zClass_NodePartial *boundNode,
    zClass_NodePartial *refNodeA,
    const zVec3 *refVecA,
    zClass_NodePartial *refNodeB,
    const zVec3 *refVecB
) {
    return SetTransformRefs(
        self,
        boundNode,
        refNodeA,
        refVecA,
        refNodeB,
        refVecB
    );
}

} // namespace zEffectAnim

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.zeffectanimentry-setonstatedonecallback
 * @recoil-artifact defines .text recoil:function:0x45e0d0: zEffectAnimEntry::SetOnStateDoneCallback (zeff_anim.c)
 * Purpose: Store the animation state-done callback and user context when an entry is available.
 */
void __fastcall zEffectAnimEntry::SetOnStateDoneCallback(
    zEffectAnimEntry *self,
    void *callback,
    void *user
) {
    if (self != 0) {
        self->eventCallback = (zEffectAnimEventCallback)(callback);
        self->eventCallbackContext = user;
    }
}

namespace zEffect {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-run.setconditionaleffectlevel
 * @recoil-artifact defines .text recoil:function:0x45e0f0: zEffect::SetConditionalEffectLevel.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeffect.cpp.
 * Purpose: store the active conditional effect level used by conditional chain
 * events.
 */
void __fastcall SetConditionalEffectLevel(
    int level
) {
    g_zEffect_ConditionalEffectLevel = level;
}

} // namespace zEffect
