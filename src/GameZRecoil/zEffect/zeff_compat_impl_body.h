#include "zeff.h"

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
 * Reimplements data 0x575a40: g_zEffect_RuntimeManager.
 * Purpose: Owns the loaded runtime-effect template table, free-list, parent,
 * and listener state used by eff_runtime.c.
 */
zEffect_RuntimeManager g_zEffect_RuntimeManager = {0};
/**
 * Reimplements data 0x575a80: g_zEffect_RandUnitTable.
 * Purpose: Stores the initialized random-unit table consumed by zEffect
 * animation runtime events.
 */
float g_zEffect_RandUnitTable[200] = {0};
/**
 * Reimplements data 0x53a0b8: g_zEffect_RandUnitScale.
 * Purpose: Stores the rand-to-unit scale used to populate
 * g_zEffect_RandUnitTable during zEffect_Anim::Init.
 */
float g_zEffect_RandUnitScale = 0.0f;
/**
 * Reimplements data 0x53a244: g_zEffect_RandTableIndex.
 * Purpose: Tracks the cursor into g_zEffect_RandUnitTable for repeated
 * animation event sampling.
 */
int g_zEffect_RandTableIndex = 0;
/**
 * Reimplements data 0x53a1c0: g_zEffectAnim_ZbdFilename.
 * Purpose: Stores the animation ZBD path loaded by zeff_anim_init.c.
 */
char g_zEffectAnim_ZbdFilename[0x80] = {0};
/* Animation load-state globals at 0x575da0..0x575db4 are stored as the
 * original adjacent records read by zEffect_Anim::LoadZbd and cleared by
 * Init/Shutdown. Keep declaration order and zero initialization source-visible.
 */
/**
 * Reimplements data 0x575da0: g_zEffectAnim_EntriesInstantiated.
 * Purpose: Records whether animation entries have been loaded and
 * instantiated.
 */
int g_zEffectAnim_EntriesInstantiated = 0;
/**
 * Reimplements data 0x575da4: g_zEffectAnim_HeapPtr.
 * Purpose: Retains the loaded animation heap block released by shutdown.
 */
void *g_zEffectAnim_HeapPtr = 0;
/**
 * Reimplements data 0x575da8: g_zEffectAnim_CountsPackedLoWord.
 * Purpose: Stores the packed count field read from the animation ZBD header.
 */
short g_zEffectAnim_CountsPackedLoWord = 0;
/**
 * Reimplements data 0x575daa: g_zEffectAnim_EntryCount.
 * Purpose: Tracks the number of entries in g_zEffectAnim_EntryList.
 */
short g_zEffectAnim_EntryCount = 0;
/**
 * Reimplements data 0x575dac: g_zEffectAnim_EntryList.
 * Purpose: Owns the loaded animation entry table used by load, shutdown,
 * save, activation, and runtime dispatch.
 */
zEffectAnimEntry *g_zEffectAnim_EntryList = 0;
/**
 * Reimplements data 0x575db0: g_zEffectAnim_TextIdEntryCount.
 * Purpose: Tracks localized text-id entries loaded from the animation ZBD.
 */
int g_zEffectAnim_TextIdEntryCount = 0;
/**
 * Reimplements data 0x575db4: g_zEffectAnim_TextIdEntryList.
 * Purpose: Owns the localized text-id entry table loaded from the animation
 * ZBD.
 */
zEffectAnimTextIdEntry *g_zEffectAnim_TextIdEntryList = 0;
/**
 * Reimplements data 0x53a248: g_zEffectAnim_SourceFileStampCount.
 * Purpose: Tracks source-file stamp records used to reject stale animation
 * ZBD data.
 */
int g_zEffectAnim_SourceFileStampCount = 0;
/**
 * Reimplements data 0x53a24c: g_zEffectAnim_SourceFileStampList.
 * Purpose: Owns the temporary source-file stamp table read during animation
 * ZBD loading.
 */
zEffectAnimSourceFileStamp *g_zEffectAnim_SourceFileStampList = 0;
/**
 * Reimplements data 0x4df724: g_zEffectAnim_CopyNodeMode.
 * Purpose: Stores g zEffectAnim CopyNodeMode data used by engine.zeffect.anim_init_data.
 */
int g_zEffectAnim_CopyNodeMode = 1;
/**
 * Reimplements data 0x4df728: g_zEffectAnim_CopyNodeArg1.
 * Purpose: Stores g zEffectAnim CopyNodeArg1 data used by engine.zeffect.anim_init_data.
 */
int g_zEffectAnim_CopyNodeArg1 = 1;
/**
 * Reimplements data 0x4df72c: g_zEffectAnim_CopyNodeArg2.
 * Purpose: Stores g zEffectAnim CopyNodeArg2 data used by engine.zeffect.anim_init_data.
 */
int g_zEffectAnim_CopyNodeArg2 = 1;
/**
 * Reimplements data 0x53a240: g_zEffectAnim_ForceCloneNonDynamicRoot.
 * Purpose: Forces loaded animation roots outside dynamic/world classes to be
 * copied before runtime binding.
 */
int g_zEffectAnim_ForceCloneNonDynamicRoot = 0;
/**
 * Reimplements data 0x4dfaec: g_zEffect_CloneCopyMode.
 * Purpose: Supplies the primary copy mode for runtime effect template clones.
 */
int g_zEffect_CloneCopyMode = 1;
/**
 * Reimplements data 0x4dfaf0: g_zEffect_CloneCopyChildrenMode.
 * Purpose: Supplies the child-copy mode for runtime effect template clones.
 */
int g_zEffect_CloneCopyChildrenMode = 1;
/**
 * Reimplements data 0x575db8: g_zEffect_World.
 * Purpose: Stores g zEffect World data used by engine.zeffect.stop_cleanup_globals.
 */
zClass_NodePartial *g_zEffect_World = 0;
/**
 * Reimplements data 0x575dbc: g_zEffect_DefaultGravity.
 * Purpose: Stores the default gravity value restored from animation ZBD state.
 */
float g_zEffect_DefaultGravity = 0.0f;
/**
 * Reimplements data 0x575dc0: g_zEffect_ConditionalRefPosEnabled.
 * Purpose: Enables conditional reference-position distance checks.
 */
int g_zEffect_ConditionalRefPosEnabled = 0;
/**
 * Reimplements data 0x575dc4: g_zEffect_VariantOverrideEnabled.
 * Purpose: Enables the packed variant override once every id is complete.
 */
int g_zEffect_VariantOverrideEnabled = 0;
/**
 * Reimplements data 0x575dc8: g_zEffect_ConditionalRefPosX.
 * Purpose: Stores the conditional reference-position X component.
 */
float g_zEffect_ConditionalRefPosX = 0.0f;
/**
 * Reimplements data 0x575dcc: g_zEffect_ConditionalRefPosY.
 * Purpose: Stores the conditional reference-position Y component.
 */
float g_zEffect_ConditionalRefPosY = 0.0f;
/**
 * Reimplements data 0x575dd0: g_zEffect_ConditionalRefPosZ.
 * Purpose: Stores the conditional reference-position Z component.
 */
float g_zEffect_ConditionalRefPosZ = 0.0f;
/**
 * Reimplements data 0x575dd4: g_zEffect_VariantOverridePackedIds.
 * Purpose: Stores the completed packed variant ids copied from zTag4Partial.
 */
unsigned int g_zEffect_VariantOverridePackedIds = 0;
/**
 * Reimplements data 0x575dd8: g_zEffect_FrameDeltaRemainingSec.
 * Purpose: Stores g zEffect FrameDeltaRemainingSec data used by engine.zeffect.stop_cleanup_globals.
 */
float g_zEffect_FrameDeltaRemainingSec = 0.0f;
/**
 * Reimplements data 0x539ea0: g_zEffect_ConditionalEffectLevel.
 * Purpose: Stores the active conditional-chain level for zEffect animation
 * events.
 */
int g_zEffect_ConditionalEffectLevel = 0;
/**
 * Reimplements data 0x539ea8: g_zEffect_VariantCycleId.
 * Purpose: Stores the current variant cycle id used by zEffect material
 * variant events.
 */
int g_zEffect_VariantCycleId = 0;
/**
 * Reimplements data 0x539ea4: g_zEffect_SkipStopDelay.
 * Purpose: Stores g zEffect SkipStopDelay data used by engine.zeffect.stop_cleanup_globals.
 */
int g_zEffect_SkipStopDelay = 0;
/**
 * Reimplements data 0x4df670: g_zEffect_Anim_DebugFrameTag.
 * Purpose: Stores the frame tag assigned by zEffect::SetAnimDebugFrameTag.
 */
int g_zEffect_Anim_DebugFrameTag = -1;
/**
 * Reimplements data 0x4df730: g_zEffect_ResourceNode.
 * Purpose: Stores the resource root node required to load animation ZBD data.
 */
zClass_NodePartial *g_zEffect_ResourceNode = (zClass_NodePartial *)(1);
/**
 * Reimplements data 0x53a2d8: g_zEffectAnim_ActivationRecordTable.
 * Purpose: Owns the queued activation-record table used to save, load, and replay deferred animation activations.
 */
zEffectAnimActivationRecord *g_zEffectAnim_ActivationRecordTable = 0;
/**
 * Reimplements data 0x53a2dc: g_zEffectAnim_ActivationRecordCapacity.
 * Purpose: Tracks the allocated activation-record slots for the queue table.
 */
int g_zEffectAnim_ActivationRecordCapacity = 0;
/**
 * Reimplements data 0x53a2e0: g_zEffectAnim_ActivationRecordCount.
 * Purpose: Tracks the number of queued activation records available for serialization or dispatch.
 */
int g_zEffectAnim_ActivationRecordCount = 0;
/**
 * Reimplements data 0x53a2e4: g_zEffectAnim_ActivationDispatchCallback.
 * Purpose: Stores the optional activation-record dispatch callback.
 */
void(__fastcall *g_zEffectAnim_ActivationDispatchCallback)(
    zEffectAnimActivationRecord *record
) = 0;
/**
 * Reimplements data 0x53a2e8: g_zEffectAnim_ActivationDispatchTagHigh.
 * Purpose: Stores the high-byte activation-record tag context.
 */
unsigned int g_zEffectAnim_ActivationDispatchTagHigh = 0;
/**
 * Reimplements data 0x4df9b4: g_zEffectAnim_RecordQueueEnabled.
 * Purpose: Enables allocation of activation records when commands should be queued instead of run immediately.
 */
int g_zEffectAnim_RecordQueueEnabled = 1;
/**
 * Reimplements data 0x4df9b8: g_zEffectAnim_DispatchEnabled.
 * Purpose: Enables immediate activation dispatch when queueing is bypassed.
 */
int g_zEffectAnim_DispatchEnabled = 1;
/**
 * Reimplements data 0x4df734: g_zEffectAnim_EnableZarRegistration.
 * Purpose: Enables registration of animation save/load ZAR section handlers.
 */
int g_zEffectAnim_EnableZarRegistration = 1;
/**
 * Reimplements data 0x4df738: g_zEffectAnim_ZarSectionName_Anim.
 * Purpose: Stores the ZAR section token for saved animation records.
 */
char g_zEffectAnim_ZarSectionName_Anim[5] = "Anim";
/**
 * Reimplements data 0x4df740: g_zEffectAnim_ZarSectionName_RunningAnim.
 * Purpose: Stores the ZAR section token for saved running animation records.
 */
char g_zEffectAnim_ZarSectionName_RunningAnim[12] = "RunningAnim";
/**
 * Reimplements data 0x4df74c: g_zEffectAnim_ZarSectionName_AnimActivation.
 * Purpose: Stores the ZAR section token for queued activation records.
 */
char g_zEffectAnim_ZarSectionName_AnimActivation[15] = "AnimActivation";
/**
 * Reimplements data 0x4df820: g_zEffectAnim_ActivationPrereqNodeNotFoundFmt.
 * Purpose: Stores the activation-prerequisite missing-node diagnostic format
 * used while loading animation ZBD records.
 */
char g_zEffectAnim_ActivationPrereqNodeNotFoundFmt[0x4e] =
    "ACTIVATION_PREREQUISITE error; couldn't find node.\n"
    "  Animation: %s; node: %s\n";
/**
 * Reimplements data 0x4df8d4: g_zEffectAnim_CorruptAnimationLoadedFmt.
 * Purpose: Reports an invalid animation entry encountered after loading the
 * animation ZBD.
 */
char g_zEffectAnim_CorruptAnimationLoadedFmt[0x2b] =
    "Corrupt animation loaded:\n  Animation: %s\n";
/**
 * Reimplements data 0x4df900: g_zEffectAnim_TokenLooping.
 * Purpose: Names the LOOPING parser field in runtime effect material map rows.
 */
char g_zEffectAnim_TokenLooping[0x8] = "LOOPING";
/**
 * Reimplements data 0x4df908: g_zEffectAnim_TokenSpeed.
 * Purpose: Names the SPEED parser field in runtime effect material map rows.
 */
char g_zEffectAnim_TokenSpeed[0x6] = "SPEED";
/**
 * Reimplements data 0x4df910: g_zEffect_FailedToFindGfxDataFmt.
 * Purpose: Reports effect templates whose model node has no graphics data.
 */
char g_zEffect_FailedToFindGfxDataFmt[0x29] =
    "Failed to find gfx data for effect (%s)\n";
/**
 * Reimplements data 0x4df93c: g_zEffect_NodeLookupFailedFmt.
 * Purpose: Reports missing model nodes while loading runtime effect templates.
 */
char g_zEffect_NodeLookupFailedFmt[0x2b] =
    "%s(%d): Failed to find node (%s) for (%s)\n";
/**
 * Reimplements data 0x4df968: g_zEffect_TokenMaps.
 * Purpose: Names the MAPS parser field in runtime effect template rows.
 */
char g_zEffect_TokenMaps[0x5] = "MAPS";
/**
 * Reimplements data 0x4df970: g_zEffect_ReadFieldFailedFmt.
 * Purpose: Reports failed zReader effect data loads from zeff_init.c.
 */
char g_zEffect_ReadFieldFailedFmt[0x1b] = "%s(%d): Failed to read %s\n";
/**
 * Reimplements data 0x4df98c: g_zEffect_SourceFile_ZeffInitC.
 * Purpose: Stores the original zeff_init.c diagnostic source path.
 */
char g_zEffect_SourceFile_ZeffInitC[0x28] =
    "D:\\Proj\\GameZRecoil\\zEffect\\zeff_init.c";
/**
 * Reimplements data 0x4df9bc: g_zEffectAnim_ActivationSectionNameFmt.
 * Purpose: Formats queued activation-record ZAR section names.
 */
char g_zEffectAnim_ActivationSectionNameFmt[0xf] = "Activation%04d";
/**
 * Reimplements data 0x4df9cc: g_zEffectAnim_RestoreNodeFmt.
 * Purpose: Reports tracked-node restoration during animation save loading.
 */
char g_zEffectAnim_RestoreNodeFmt[0x14] = "Restore node: %s %d";
/**
 * Reimplements data 0x4df9e0: g_zEffectAnim_StateInvalidMsg.
 * Purpose: Reports restored activation records forced to ANIM_STATE_INVALID.
 */
char g_zEffectAnim_StateInvalidMsg[0x24] =
    "Set anim_state = ANIM_STATE_INVALID";
/**
 * Reimplements data 0x4dfa04: g_zEffectAnim_ResetFunctionName.
 * Purpose: Names the animation reset diagnostic path.
 */
char g_zEffectAnim_ResetFunctionName[0xe] = "zEffAnimReset";
/**
 * Reimplements data 0x4dfa14: g_zEffectAnim_StateExecutedMsg.
 * Purpose: Reports restored activation records forced to ANIM_STATE_EXECUTED.
 */
char g_zEffectAnim_StateExecutedMsg[0x25] =
    "Set anim_state = ANIM_STATE_EXECUTED";
/**
 * Reimplements data 0x4dfa3c: g_zEffectAnim_ProcessActivationRecordName.
 * Purpose: Names the queued activation-record processing diagnostic path.
 */
char g_zEffectAnim_ProcessActivationRecordName[0x1c] =
    "zEffProcessActivationRecord";
/**
 * Reimplements data 0x4dfa58: g_zEffect_SourceFile_ZeffAnimSaveC.
 * Purpose: Stores the original zeff_anim_save.c diagnostic source path.
 */
char g_zEffect_SourceFile_ZeffAnimSaveC[0x2d] =
    "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_save.c";
/**
 * Reimplements data 0x4dfa88: g_zEffectAnim_ResetActivationRecordFmt.
 * Purpose: Reports reset of queued activation records during save loading.
 */
char g_zEffectAnim_ResetActivationRecordFmt[0x1e] =
    "zEffResetActivationRecord: %s";
/**
 * Reimplements data 0x4dfaa8: g_zEffectAnim_ActivationSectionName0.
 * Purpose: Names the first activation-record ZAR section.
 */
char g_zEffectAnim_ActivationSectionName0[0xf] = "Activation0000";
/**
 * Reimplements data 0x4dfab8: g_zEffectAnim_RunningSectionNameFmt.
 * Purpose: Formats running-animation ZAR section names.
 */
char g_zEffectAnim_RunningSectionNameFmt[0xc] = "Running%03d";
/**
 * Reimplements data 0x4dfac4: g_zEffectAnim_AnimSectionNameFmt.
 * Purpose: Formats non-running animation ZAR section names.
 */
char g_zEffectAnim_AnimSectionNameFmt[0x9] = "Anim%04d";
/**
 * Reimplements data 0x4dfad0: g_zEffect_StringNone.
 * Purpose: Stores the empty animation section sentinel name.
 */
char g_zEffect_StringNone[0x5] = "None";
/**
 * Reimplements data 0x4dfad8: g_zEffectAnim_ResetTraceFmt.
 * Purpose: Reports reset of animation entries during save loading.
 */
char g_zEffectAnim_ResetTraceFmt[0x12] = "zEffAnimReset: %s";
}

extern char g_EffectsZrdNodeName[8];

namespace {
const unsigned int kRandUnitScaleBits = 0x38000100u;
const int kInitialActivationRecordCapacity = 1000;
const unsigned int kActivationRecordNoQueueDispatchFlag = 0x00000100u;
const unsigned int kActivationRecordQueueOverrideFlag = 0x00001000u;
const unsigned int kActivationRecordQueueOverrideValue = 0x00002000u;
const unsigned int kActivationRecordDispatchOverrideFlag = 0x00000400u;
const unsigned int kActivationRecordDispatchOverrideValue = 0x00000800u;
const unsigned int kEffectAnimWorldChildAttachedFlag = 0x00000100u;
const unsigned int kEffectAnimNeedsCopiedRootFlag = 0x00008000u;
const float kEmitterLoopTriggerClampValue = 86400.0f;
const float kEffectAnimStopDelaySkipBias = -0.01f;
const float kEffectAnimActivationSentinel = -99.0f;
const float kEffectAnimActivationSentinelTolerance = 0.1f;
const float kEffectAnimVelocityEpsilon = 0.01f;
const short kEffectAnimResetScratchRefIndex = -200;
const short kEffectAnimBoundNodeRefIndex = -100;
const char *kZeffAnimInitSourceFile = "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_init.c";
const char *kZeffAnimRunSourceFile = "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_run.c";
const char *kZeffInitSourceFile = g_zEffect_SourceFile_ZeffInitC;
const char *kAnimationNodeNotFoundMessage =
    "Animation node not found.\n  Animation: %s; Node: %s\n";

/**
 * Recovered original static helper for zEffect cleanup routines.
 * No standalone retail function is present; observed callers include
 * 0x45fd10 zEffectAnim::ShutdownEntry and nearby zEffect cleanup paths that
 * perform the same null-checked free.
 * Purpose: free an owned allocation only when its pointer is non-null.
 */
void FreeIfSet(
    void *ptr
) {
    if (ptr != 0) {
        free(ptr);
    }
}

struct zEffectAnimZbdHeaderBlock {
    int entriesInstantiated;
    void *heapPtr;
    short countsPackedLoWord;
    short entryCount;
    zEffectAnimEntry *entryList;
    int textIdEntryCount;
    zEffectAnimTextIdEntry *textIdEntryList;
    zClass_NodePartial *worldNode;
    float defaultGravity;
    int conditionalRefPosEnabled;
    int variantOverrideEnabled;
    float conditionalRefPosX;
    float conditionalRefPosY;
    float conditionalRefPosZ;
    unsigned int variantOverridePackedIds;
    float frameDeltaRemainingSec;
};

RECOIL_STATIC_ASSERT(sizeof(zEffectAnimZbdHeaderBlock) == 0x3c);

struct zEffectAnimTrackedNodeSaveRecord {
    int nodeIndex;
    int activeFlag;
    int usesCachedMatrix;
    float transform[12];
    int diFlagBits;
    int diUserValue;
};

struct zEffectAnimActivationSaveRecord {
    zEffectAnimActivationRecord base;
    unsigned char unknown_50[4];
    unsigned char savedActivationState;
    unsigned char trackedNodeCount;
    unsigned char unknown_56[2];
    zEffectAnimTrackedNodeSaveRecord trackedNodes[1];
};

struct zEffectAnimSaveHeader {
    zEffectAnimActivationRecord base;
    int entryTableIndex;
    unsigned char savedActivationState;
    unsigned char trackedNodeCount;
    unsigned char unknown_56[2];
};

const int kMaxEffectAnimTrackedNodeSaveCount = 256;

struct zEffectAnimSaveRecord {
    zEffectAnimSaveHeader header;
    zEffectAnimTrackedNodeSaveRecord trackedNodes[kMaxEffectAnimTrackedNodeSaveCount];
};

RECOIL_STATIC_ASSERT(sizeof(zEffectAnimTrackedNodeSaveRecord) == 0x44);
RECOIL_STATIC_ASSERT(
    offsetof(
        zEffectAnimActivationSaveRecord,
        savedActivationState
    ) == 0x54
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zEffectAnimActivationSaveRecord,
        trackedNodeCount
    ) == 0x55
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zEffectAnimActivationSaveRecord,
        trackedNodes
    ) == 0x58
);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimSaveHeader) == 0x58);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimSaveRecord, trackedNodes) == 0x58);
const unsigned int kMaxActivationSaveRecordSize =
    offsetof(zEffectAnimActivationSaveRecord, trackedNodes) +
    sizeof(zEffectAnimTrackedNodeSaveRecord) * kMaxEffectAnimTrackedNodeSaveCount;

struct zEffectAnimRunningSaveHeader {
    int entryTableIndex;
    int matchSavedRootNode;
    char entryName[0x20];
    int rootNodeIndex;
    int nodeRefAIndex;
    zVec3 refVecA;
    int nodeRefBIndex;
    zVec3 refVecB;
    unsigned char activationState;
    unsigned char unknown_4d[3];
    float triggerCurrentValue;
    float activationCountdown;
    float velocityX;
    float velocityY;
    float velocityZ;
    unsigned char runtimeSurfaceCount;
    unsigned char lightRefCount;
    unsigned char soundRefCount;
    unsigned char reserved;
};

struct zEffectAnimRuntimeNodeSaveRecord {
    char name[0x24];
    int isAttached;
    float posX;
    float posY;
    float posZ;
    int parentNodeIndex;
};

struct zEffectAnimSoundNodeSaveRecord {
    char name[0x24];
    int isAttached;
    int hasPosition;
    float posX;
    float posY;
    float posZ;
    int parentNodeIndex;
};

RECOIL_STATIC_ASSERT(sizeof(zEffectAnimRunningSaveHeader) == 0x68);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimRuntimeNodeSaveRecord) == 0x38);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimSoundNodeSaveRecord) == 0x3c);

/**
 * Recovered original static helper for zeff_anim_init.c ZBD loading.
 * No standalone retail function is present; observed callers are the
 * 0x45efb0 zEffect_Anim::LoadZbd read sequence and the save/load persistence
 * helpers in this source file that compare fread counts against one record.
 * Purpose: read one fixed-size record from a stream.
 */
bool ReadOne(
    FILE *stream,
    void *data,
    size_t size
) {
    return fread(
        data,
        size,
        1,
        stream
    ) == 1;
}

/**
 * Recovered original static helper for zeff_anim_init.c ZBD loading.
 * No standalone retail function is present; observed caller 0x45efb0 allocates
 * typed arrays for animation entries, refs, stamps, and text records with this
 * malloc-plus-fread pattern.
 * Purpose: allocate and read a raw fixed-stride array.
 */
bool ReadArrayRaw(
    FILE *stream,
    void **outData,
    size_t stride,
    unsigned int count
) {
    *outData = 0;
    if (count == 0) {
        return true;
    }

    void *data = malloc(stride * count);
    if (data == 0) {
        return false;
    }

    *outData = data;
    return fread(
        data,
        stride,
        count,
        stream
    ) == count;
}

template <typename T>
/**
 * Recovered original static helper for zeff_anim_init.c ZBD loading.
 * No standalone retail function is present; observed caller 0x45efb0 repeats
 * typed array reads with element counts taken from serialized animation data.
 * Purpose: allocate and read a typed array from an animation ZBD stream.
 */
bool ReadArray(
    FILE *stream,
    T **outData,
    unsigned int count
) {
    void *data = 0;
    if (!ReadArrayRaw(
        stream,
        &data,
        sizeof(T),
        count
    )) {
        return false;
    }

    *outData = (T *)(data);
    return true;
}

/**
 * Recovered original static helper for zEffect animation stream loading.
 * No standalone retail function is present; observed callers are the 0x45efb0
 * load of primary and per-sequence event streams from the animation ZBD.
 * Purpose: allocate and read a serialized event stream when one is present.
 */
bool ReadEventStream(
    FILE *stream,
    zEffectAnimSurfaceRuntime *runtime
) {
    runtime->eventStream = 0;
    if (runtime->eventStreamSize <= 0) {
        return true;
    }

    runtime->eventStream = malloc(runtime->eventStreamSize);
    if (runtime->eventStream == 0) {
        return false;
    }

    return ReadOne(
        stream,
        runtime->eventStream,
        runtime->eventStreamSize
    );
}

/**
 * Original helper evidence: no standalone retail function.
 * Observed callers: 0x460070.
 * Evidence basis: InitFromPath indexes the same zReader node-array payload for
 * effect-template rows and material texture-map rows.
 * Purpose: return the first payload node for a zReader array node.
 */
zReader::Node *zReaderArrayBase(
    zReader::Node *node
) {
    return node->value.nodes;
}

/**
 * Original helper evidence: no standalone retail function.
 * Observed callers: 0x460020.
 * Evidence basis: Init uses the same zReader node-array count load for effect
 * template and material texture-map lists.
 * Purpose: return the element count stored at the head of a zReader array node.
 */
int zReaderArrayCount(
    zReader::Node *node
) {
    return zReaderArrayBase(node)->value.i32;
}

/**
 * Original helper evidence: no standalone retail function.
 * Observed callers: 0x460070.
 * Evidence basis: InitFromPath repeatedly reads zReader array payload strings
 * for model names, looping text, and material texture-map paths.
 * Purpose: return the string stored at a zReader array payload index.
 */
char *zReaderArrayStringAt(
    zReader::Node *node,
    int index
) {
    return zReaderArrayBase(node)[index].value.str;
}

/**
 * Recovered original inlined helper for zeff_anim_init.c.
 * No standalone retail function is present; the observed caller is
 * 0x45ed80 zEffectAnim::RebindEntryToNode, where the callback-node,
 * tracked-node, and node-ref failure branches repeat the same
 * zError::ReportOld(kAnimationNodeNotFoundMessage, self, nodeName)
 * sequence followed by activationState = 5.
 * Purpose: report a missing animation node and mark the entry failed.
 */
static inline void ReportAnimationNodeNotFound(
    zEffectAnimEntry *self,
    int sourceLine,
    const char *nodeName
) {
    zError::ReportOld(
        0x400,
        kZeffAnimInitSourceFile,
        sourceLine,
        kAnimationNodeNotFoundMessage,
        self,
        nodeName
    );
    self->activationState = 5;
}

/**
 * Recovered original static helper for zeff_anim_run.c emitter events.
 * No standalone retail function is present; observed callers are 0x45c240,
 * 0x45c100, and 0x45c1a0, which share the cached-name entry lookup.
 * Purpose: resolve and cache an animation entry index for emitter control events.
 */
int ResolveEmitterEventEntryIndex(
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

    return event->cachedEntryIndex;
}

/**
 * Recovered original static helper for zeff_anim_run.c runtime-ref events.
 * No standalone retail function is present; observed callers are 0x45bc60 and
 * 0x45b8b0, which reuse the same cached animation-name lookup.
 * Purpose: resolve and cache an animation entry index by serialized name.
 */
int ResolveAnimEntryIndexByName(
    short *cachedIndex,
    const char *animName
) {
    if (*cachedIndex <= 0) {
        zEffectAnimEntry *entry = g_zEffectAnim_EntryList;
        for (int i = 0; i < g_zEffectAnim_EntryCount; ++i, ++entry) {
            if (strcmp(
                entry->name,
                animName
            ) == 0) {
                *cachedIndex = (short)(i);
                break;
            }
        }
    }

    return *cachedIndex;
}

/**
 * Recovered original static helper for zeff_anim_run.c surface events.
 * No standalone retail function is present; observed callers are 0x45bb00 and
 * 0x45bbb0, which share the cached surface sequence lookup.
 * Purpose: resolve and cache a runtime surface sequence index by name.
 */
int ResolveSurfaceRuntimeIndex(
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

    return event->surfaceSlotIndex;
}

/**
 * Recovered original static helper for zEffect variant gating.
 * No standalone retail function is present; observed caller 0x45d010 copies
 * the packed override ids into the current VariantTag record before testing it.
 * Purpose: make the active zEffect variant override the current variant tag.
 */
void CopyPackedVariantOverrideToCurrentTag() {
    memcpy(
        &g_Variant_CurrentTag,
        &g_zEffect_VariantOverridePackedIds,
        sizeof(g_Variant_CurrentTag)
    );
}

/**
 * Original helper evidence: no standalone retail function.
 * Observed callers: 0x45d010.
 * Evidence basis: RunSequence uses byte loads/stores against the four-byte
 * zero-initialized g_zEffect_VariantCycleId slot at 0x539ea8.
 * Purpose: store the current low-byte variant cycle delay and advance it with
 * the caller-specific priority cap.
 */
void AdvanceVariantCycleDelay(
    zEffectAnimEntry *self
) {
    unsigned char &variantCycleId = *((unsigned char *)(&g_zEffect_VariantCycleId));
    self->variantCycleDelay = variantCycleId;
    ++variantCycleId;

    const int maxCycleId = ((int)(self->priority) * 10) >> 2;
    if ((int)(variantCycleId) > maxCycleId) {
        variantCycleId = 1;
    }
}

/**
 * Original helper evidence: no standalone retail function.
 * Observed callers: 0x45dcb0, 0x45de00, 0x45df90.
 * Evidence basis: repeated activation setup in the velocity and transform-ref
 * entry points resets the bound camera/object before storing runtime motion data.
 * Purpose: clear the activated entry's bound-node transform before velocity or
 * reference reset state is applied.
 */
void ResetActivatedBoundTransform(
    zEffectAnimEntry *activatedEntry
) {
    if ((activatedEntry->flags & kEffectAnimWorldChildAttachedFlag) == 0) {
        return;
    }

    zClass_NodePartial *const boundNode = activatedEntry->boundNode;
    if (boundNode->classId == 1) {
        zClass_Camera::gwCameraSetTarget(
            boundNode,
            0.0f,
            0.0f,
            0.0f
        );
        if ((activatedEntry->flags & 0x00000200u) == 0) {
            zClass_Camera::gwCameraSetPosition(
                activatedEntry->boundNode,
                0.0f,
                0.0f,
                0.0f
            );
        }
    } else if (boundNode->classId == 5) {
        zClass_Object3D::gwObject3DSetPosition(
            boundNode,
            0.0f,
            0.0f,
            0.0f
        );
        if ((activatedEntry->flags & 0x00000200u) == 0) {
            zClass_Object3D::gwObject3DSetRotation(
                activatedEntry->boundNode,
                0.0f,
                0.0f,
                0.0f
            );
        }
    }
}

/**
 * Original helper evidence: no standalone retail function.
 * Observed callers: 0x45dcb0, 0x45de00.
 * Evidence basis: repeated velocity-threshold test in activation entry points
 * with the shared kEffectAnimVelocityEpsilon constant.
 * Purpose: determine whether a velocity vector should mark the runtime entry as
 * moving.
 */
bool VelocityIsActive(
    float x,
    float y,
    float z
) {
    return fabs(x) > kEffectAnimVelocityEpsilon || fabs(y) > kEffectAnimVelocityEpsilon ||
           fabs(z) > kEffectAnimVelocityEpsilon;
}

/**
 * Recovered original static helper for zEffect beam and screen-fx math.
 * No standalone retail function is present; observed callers are 0x458c10,
 * 0x458ce0, and 0x45c920, which use the same fast square-root estimate.
 * Purpose: approximate a beam or reference length from a squared distance.
 */
float BeamLengthFromLengthSq(
    float lengthSq
) {
    int bits = 0;
    memcpy(
        &bits,
        &lengthSq,
        sizeof(bits)
    );
    bits = (bits >> 1) + 0x1fc00000;

    float length = 0.0f;
    memcpy(
        &length,
        &bits,
        sizeof(length)
    );
    return length;
}

/**
 * Recovered original static helper for zeff_anim_run.c screen color events.
 * No standalone retail function is present; observed caller 0x45c710 clamps
 * color channels before packing them for zVideo.
 * Purpose: clamp a floating-point color channel into the unit interval.
 */
float ClampUnitFloat(
    float value
) {
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

/**
 * Recovered original static helper for zeff_anim_run.c screen color events.
 * No standalone retail function is present; observed caller 0x45c710 converts
 * clamped float color channels to packed byte channels.
 * Purpose: convert a unit floating-point channel to a rounded byte value.
 */
unsigned char UnitFloatToByte(
    float value
) {
    return (unsigned char)((int)(value * 255.0f + 0.5f));
}

/**
 * Recovered original static helper for zeff_anim_run.c node animation.
 * No standalone retail function is present; observed callers 0x459e70 and
 * 0x45a9d0 set the same DI blend-scale flag and clamp behavior.
 * Purpose: apply an absolute DI blend scale to a node.
 */
void SetNodeDiBlendScale(
    zClass_NodePartial *node,
    float blendScale
) {
    if (node == 0 || node->userDataOrDiRef == 0) {
        return;
    }

    zDiPartial *const di = (zDiPartial *)(node->userDataOrDiRef);
    di->flags |= 0x08;
    di->blendScale = blendScale;
    if (di->blendScale > 1.0f) {
        di->blendScale = 1.0f;
    } else if (di->blendScale < 0.00001f) {
        di->flags &= ~0x08;
    }
}

/**
 * Recovered original static helper for zeff_anim_run.c node animation.
 * No standalone retail function is present; observed callers 0x459e70 and
 * 0x45a9d0 add frame deltas to the same DI blend-scale field.
 * Purpose: add a delta to a node's DI blend scale.
 */
void AddNodeDiBlendScale(
    zClass_NodePartial *node,
    float blendDelta
) {
    if (node == 0 || node->userDataOrDiRef == 0) {
        return;
    }

    zDiPartial *const di = (zDiPartial *)(node->userDataOrDiRef);
    SetNodeDiBlendScale(
        node,
        di->blendScale + blendDelta
    );
}

/**
 * Recovered original static helper for zEffect animation random sampling.
 * No standalone retail function is present; observed callers 0x459e70 and
 * 0x45c3c0 consume the shared 200-entry random unit table and wrapping index.
 * Purpose: return the next animation random unit value and advance the stream.
 */
float NextEffectRandUnit() {
    const float value = g_zEffect_RandUnitTable[g_zEffect_RandTableIndex];
    g_zEffect_RandTableIndex = (g_zEffect_RandTableIndex + 1) % 200;
    return value;
}

/**
 * Recovered original static helper for zeff_anim_run.c matrix transforms.
 * No standalone retail function is present; observed caller 0x459e70 uses the
 * current zMath matrix slot to transform local vectors.
 * Purpose: transform a vector by the current zMath matrix.
 */
zVec3 TransformByCurrentMatrix(
    const zVec3 *vec
) {
    const zMat4x3 *matrix = (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
    zVec3 result = {vec->x * matrix->xx + vec->y * matrix->yx + vec->z * matrix->zx + matrix->posX,
        vec->x * matrix->xy + vec->y * matrix->yy + vec->z * matrix->zy + matrix->posY,
        vec->x * matrix->xz + vec->y * matrix->yz + vec->z * matrix->zz + matrix->posZ};
    return result;
}

/**
 * Recovered original static helper for zeff_anim_run.c node-basis transforms.
 * No standalone retail function is present; observed caller 0x459e70 builds a
 * node-to-ancestor basis matrix before applying local velocity or offset data.
 * Purpose: transform a vector by a node's current basis.
 */
zVec3 TransformNodeBasisVector(
    zClass_NodePartial *node,
    const zVec3 *vec
) {
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

    const zVec3 out = TransformByCurrentMatrix(vec);
    zMath::MatStackPopPtr();
    return out;
}

/**
 * Recovered original static helper for zeff_anim_run.c node animation.
 * No standalone retail function is present; observed caller 0x459e70 resolves
 * a cached runtime sequence index for chained node-animation restarts.
 * Purpose: resolve and cache the runtime sequence index for a node anim event.
 */
short ResolveNodeAnimRuntimeIndex(
    zEffectAnimEntry *self,
    zEffectNodeAnimEvent *event
) {
    if (event->packedRuntimeIndex < 0) {
        for (int i = 0; i < self->runtimeSequenceCount; ++i) {
            if (strcmp(
                self->runtimeList[i].sequenceName,
                event->targetName
            ) == 0) {
                event->packedRuntimeIndex = (short)(i);
                break;
            }
        }
    }

    return event->packedRuntimeIndex;
}

/**
 * Original helper evidence: no standalone retail function.
 * Observed callers: 0x45de00, 0x45df90, 0x461040.
 * Evidence basis: repeated resetScratch three-word vector stores used by
 * position-ref, transform-ref, and running-animation load paths.
 * Purpose: store an optional vector into the reset scratch record.
 */
void StoreResetScratchVec(
    zEffectAnimEntry *entry,
    size_t firstIndex,
    const zVec3 *vec
) {
    if (vec != 0) {
        memcpy(
            &entry->resetScratch[firstIndex],
            &vec->x,
            sizeof(vec->x)
        );
        memcpy(
            &entry->resetScratch[firstIndex + 1],
            &vec->y,
            sizeof(vec->y)
        );
        memcpy(
            &entry->resetScratch[firstIndex + 2],
            &vec->z,
            sizeof(vec->z)
        );
    } else {
        entry->resetScratch[firstIndex] = 0;
        entry->resetScratch[firstIndex + 1] = 0;
        entry->resetScratch[firstIndex + 2] = 0;
    }
}

/**
 * Original helper evidence: no standalone retail function.
 * Observed callers: 0x45b4a0, 0x45b8b0, 0x460bc0.
 * Evidence basis: repeated resetScratch three-word vector loads in detach,
 * transform-ref, and running-animation save paths.
 * Purpose: load a vector previously preserved in the reset scratch record.
 */
zVec3 LoadResetScratchVec(
    zEffectAnimEntry *entry,
    size_t firstIndex
) {
    zVec3 value = {0};
    memcpy(
        &value.x,
        &entry->resetScratch[firstIndex],
        sizeof(value.x)
    );
    memcpy(
        &value.y,
        &entry->resetScratch[firstIndex + 1],
        sizeof(value.y)
    );
    memcpy(
        &value.z,
        &entry->resetScratch[firstIndex + 2],
        sizeof(value.z)
    );
    return value;
}

/**
 * Original helper evidence: no standalone retail function.
 * Observed callers: 0x45b4a0, 0x45b8b0, 0x460bc0.
 * Evidence basis: repeated resetScratch node-pointer loads paired with the
 * vector scratch slots in detach, transform-ref, and running-animation save
 * paths.
 * Purpose: recover a node pointer stored in the reset scratch record.
 */
zClass_NodePartial *LoadResetScratchNode(
    zEffectAnimEntry *entry,
    size_t index
) {
    return (zClass_NodePartial *)((unsigned int)(entry->resetScratch[index]));
}

/**
 * Original helper evidence: no standalone retail function.
 * Observed callers: 0x45bc60.
 * Evidence basis: HandleSurfaceRefEvent repeats the positive node-ref lookup
 * and the -200 reset-scratch node fallback for child-animation positioning.
 * Purpose: resolve a serialized node reference or recover the reset-scratch node.
 */
zClass_NodePartial *ResolveNodeRefOrResetScratch(
    zEffectAnimEntry *self,
    short nodeRefIndex
) {
    if (nodeRefIndex > 0) {
        return self->nodeRefList[nodeRefIndex].node;
    }
    if (nodeRefIndex == -200) {
        return LoadResetScratchNode(
            self,
            0
        );
    }
    return 0;
}

/**
 * Original helper evidence: no standalone retail function.
 * Observed callers: 0x45cc00.
 * Evidence basis: the sequence event-type switch is owned by RunSequenceEvents
 * in the zeff_anim_run.c source-file subsystem and has no separate retail body.
 * Purpose: dispatch the current sequence event record to the matching handler.
 */
int DispatchSequenceEvent(
    zEffectAnimEntry *self,
    zEffectAnimSurfaceRuntime *sequenceRuntime,
    zEffectAnimEventHeader *currentEvent
) {
    switch (currentEvent->eventType) {
    case 1:
        return zEffect::HandleSampleRefOffsetEvent(
            self,
            (zEffectAnimRefOffsetEvent *)(currentEvent)
        );
    case 2:
        return zEffect::HandleSoundEvent(
            self,
            (zEffectAnimSoundEvent *)(currentEvent)
        );
    case 3:
        return zEffect::HandleEffectTemplateOffsetEvent(
            self,
            (zEffectAnimRefOffsetEvent *)(currentEvent)
        );
    case 4:
        return zEffect::HandleLightEvent(
            self,
            (zEffectAnimLightEvent *)(currentEvent)
        );
    case 5:
        return zEffect::HandleLightAnimEvent(
            self,
            sequenceRuntime,
            (zEffectLightRangeSpecularAnimEvent *)(currentEvent)
        );
    case 6:
        return zEffect::HandleActivateEvent(
            self,
            (zEffectActivateEvent *)(currentEvent)
        );
    case 7:
        return zEffect::HandlePositionEvent(
            self,
            (zEffectTransformEvent *)(currentEvent)
        );
    case 8:
        return zEffect::HandleNodeScaleEvent(
            self,
            (zEffectNodeScaleEvent *)(currentEvent)
        );
    case 9:
        return zEffect::HandleRotationEvent(
            self,
            (zEffectTransformEvent *)(currentEvent)
        );
    case 0x0a:
        return zEffect::HandleNodeAnimEvent(
            self,
            sequenceRuntime,
            (zEffectNodeAnimEvent *)(currentEvent)
        );
    case 0x0b:
        return zEffect::AnimateNodeOverTime(
            self,
            sequenceRuntime,
            (zEffectNodeAnimEvent *)(currentEvent)
        );
    case 0x0c:
        return zEffect_Anim::AdvanceKeyframe(
            self,
            sequenceRuntime,
            (zEffectKeyframeEvent *)(currentEvent)
        );
    case 0x0d:
        return zEffect_Anim::EvaluateKeyframe(
            self,
            (zEffectEvaluateKeyframeEvent *)(currentEvent)
        );
    case 0x0e:
        return zEffect_Anim::RunKeyframes(
            self,
            sequenceRuntime,
            (zEffectRunKeyframeEvent *)(currentEvent)
        );
    case 0x1b:
        return zEffect::HandleEmitterStopEvent(
            self,
            (zEffectAnimEmitterEvent *)(currentEvent)
        );
    case 0x14:
        return zEffect::HandleCameraParamsEvent(
            self,
            sequenceRuntime,
            (zEffectCameraEvent *)(currentEvent)
        );
    case 0x15:
        return zEffect::AnimateCameraParamsOverTime(
            self,
            sequenceRuntime,
            (zEffectCameraAnimEvent *)(currentEvent)
        );
    case 0x0f:
        return zEffect::HandleAddChildEvent(
            self,
            (zEffectParentChildEvent *)(currentEvent)
        );
    case 0x10:
        return zEffect::HandleRemoveChildEvent(
            self,
            (zEffectParentChildEvent *)(currentEvent)
        );
    case 0x11:
        return zEffect::HandleAttachEvent(
            self,
            sequenceRuntime,
            (zEffectAttachEvent *)(currentEvent)
        );
    case 0x12:
        return zEffect::HandleDetachEvent(
            self,
            sequenceRuntime,
            (zEffectBeamDetachEvent *)(currentEvent)
        );
    case 0x16:
        return zEffect::HandleSurfaceStopEvent(
            self,
            (zEffectSurfaceControlEvent *)(currentEvent)
        );
    case 0x17:
        return zEffect::HandleSurfacePlayEvent(
            self,
            (zEffectSurfaceControlEvent *)(currentEvent)
        );
    case 0x18:
        return zEffect::HandleSurfaceRefEvent(
            self,
            sequenceRuntime,
            (zEffectSurfaceRefEvent *)(currentEvent)
        );
    case 0x24:
        return zEffect::HandleScreenColorFxEvent(
            self,
            sequenceRuntime,
            (zEffectScreenColorFxEvent *)(currentEvent)
        );
    case 0x25:
        return zEffect::HandleScreenOverlayFxEvent(
            self,
            sequenceRuntime,
            (zEffectScreenOverlayFxEvent *)(currentEvent)
        );
    case 0x13:
        return zEffect::HandleTransformRefsEvent(
            self,
            (zEffectTransformRefsEvent *)(currentEvent)
        );
    case 0x19:
        return zEffect::HandleNamedAnimStopEvent(
            self,
            (zEffectAnimEmitterEvent *)(currentEvent)
        );
    case 0x1a:
        return zEffect::HandleEmitterPlayEvent(
            self,
            (zEffectAnimEmitterEvent *)(currentEvent)
        );
    case 0x1c:
        return zEffect::HandleFogEvent(
            self,
            (zEffectFogEvent *)(currentEvent)
        );
    case 0x1e:
        return zEffect::HandleEmitterLoopEvent(
            self,
            sequenceRuntime,
            (zEffectAnimLoopEvent *)(currentEvent)
        );
    case 0x1f:
        return zEffect::HandleConditionalChainEvent(
            self,
            sequenceRuntime,
            (zEffectConditionalEvent *)(currentEvent)
        );
    case 0x20:
    case 0x21:
        return zEffect::SkipConditionalChainToEnd(
            self,
            sequenceRuntime,
            currentEvent
        );
    case 0x22:
    case 0x27:
    case 0x28:
        return zEffect::HandleNoOpMarkerEvent(
            self,
            sequenceRuntime,
            currentEvent
        );
    case 0x23:
        return zEffect::HandleCallbackEvent(
            self,
            sequenceRuntime,
            (zEffectAnimCallbackEvent *)(currentEvent)
        );
    case 0x26:
        return zEffect::HandleTopMessageEvent(
            self,
            (zEffectTopMessageEvent *)(currentEvent)
        );
    default:
        zError::ReportOld(
            0x400,
            kZeffAnimRunSourceFile,
            0x171c,
            "Invalid Sequence Event\n  Animation: %s\n",
            self
        );
        return -1;
    }
}
} // namespace

