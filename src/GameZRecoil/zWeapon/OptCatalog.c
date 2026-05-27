#include "OptCatalog.h"
#include "zWeapon.h"

#include "Battlesport/GameNet.h"
#include "Battlesport/player.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "zDi.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

extern "C" {
int g_OptCatalog_CaptureHitSnapshotEnabled = 0;
int g_OptCatalog_FallbackImpactProbeEnabled = 0;
zVec3 g_OptCatalog_CapturedDamageSourcePos = {0};
zVec3 g_OptCatalog_CapturedDamageHitPos = {0};
int g_OptCatalog_EntryCount = 0;
OptCatalogEntryDef *g_OptCatalog_EntryTable = 0;
int g_OptCatalogRuntimeInstanceCount = 0;
void *g_OptCatalogRuntimeInstancePool = 0;
void *g_OptCatalogFreeRuntimeInstanceList = 0;
zClass_NodePartial *g_OptCatalogRuntimeWorld = 0;
void *g_OptCatalogPendingSpawnTargetCountPtr = 0;
void *g_OptCatalogPendingSpawnTargetListPtr = 0;
float g_OptCatalogMaxCraterRadius = 0.0f;
int g_OptCatalogQueuedImpactCount = 0;
int g_OptCatalog_DamageContextKind = 0;
void *g_OptCatalog_DamageContextHitEvent = 0;
void *g_OptCatalog_CurrentDamageOwnerOrCtx = 0;
void *g_OptCatalogDamageFeedbackCallback = 0;
float g_OptCatalogLockOnWarningGateTimeSec = 0.0f;
int g_OptCatalog_DamageFeedbackHitCount = 0;
zClass_NodePartial *g_OptCatalogDamageFeedbackTrackedNode = 0;
float g_OptCatalogDamageFeedbackIntensityScalar = 0.0f;
float g_OptCatalogNextSpawnScale = 0.0f;
float g_OptCatalogRuntimeDeltaTime = 0.0f;
float g_OptCatalogRuntimeNowSec = 0.0f;
zClass_NodePartial *g_OptCatalogThermalGlowFreeList = 0;
OptCatalogRuntimeInstanceStorage *g_OptCatalog_MineIteratorCursor = 0;
zReader::Node *g_OptCatalogLoadedTreeRoot = 0;
zSndSample *g_OptCatalogSndTriggerInactive = 0;
zSndSample *g_OptCatalogSndWeaponInactive = 0;
zSndSample *g_OptCatalogSndNoAmmoWarning = 0;
zSndSample *g_OptCatalogSndLockOnWarning = 0;
OptCatalogRemoveRuntimeRelayCallback g_OptCatalog_RemoveRuntimeRelayCallback = 0;
int g_OptCatalogNetworkOptionState = 0;
OptCatalogAllocRuntimeGateCallback g_OptCatalog_AllocRuntimeGateCallback = 0;
OptCatalogAllocRuntimeGateCallback g_OptCatalog_AltGunDispatchNoOpCallback = 0;
int g_OptCatalogProcessRuntimeRelayEnabled = 1;
}

namespace {
    const unsigned int kOptCatalogFlagImmediateProbeImpact = 1u << 12;
    const unsigned int kOptCatalogFlagFullProbeDamage = 1u << 13;
    const unsigned int kOptCatalogFlagCraterImpact = 0x08;
    const unsigned int kOptCatalogFlagQuickSandImpact = 0x20000;
    const unsigned int kOptCatalogFlagAlwaysPlayImpactFx = 4194304;
    const unsigned int kOptCatalogFlagTrailRuntime = 2;
    const unsigned int kOptCatalogFlagImpactWhenScaleExpired = 4;
    const unsigned int kOptCatalogFlagSingleTrailSegment = 0x800;
    const unsigned int kOptCatalogFlagAllowOutOfRangeAimPitch = 0x2000;
    const unsigned int kOptCatalogFlagSkipTrailSegmentLighting = 0x10000;
    const unsigned int kOptCatalogFlagSkipDamageMaskStamp = 0x200000;
    const unsigned int kOptCatalogFlagForceSpawnVelocity = 0x400;
    const unsigned int kOptCatalogFlagRelativeSpeed = 0x800000;
    const unsigned int kOptCatalogFlagFlyoutSkipRotation = 0x2000;
    const unsigned int kOptCatalogFlagFlyoutModelRotation = 0x100;
    const unsigned int kOptCatalogFlagUsePendingSpawnTarget = 1u << 22;
    const unsigned int kOptCatalogFlagTrailUsePendingSpawnTargets = 1u << 14;
    const unsigned int kOptCatalogFlagTrailStartMutedAndLight = 1u << 19;
    const unsigned int kOptCatalogFlagExpires = 1u << 6;
    const unsigned int kOptCatalogFlagFixedRotate = 1u << 7;
    const unsigned int kOptCatalogFlagInstant = 1u << 10;
    const unsigned int kOptCatalogFlagLockOn = 1u << 14;
    const unsigned int kOptCatalogFlagLockOnLead = 1u << 15;
    const unsigned int kOptCatalogFlagMultiTarget = 1u << 16;
    const unsigned int kOptCatalogFlagReload = 1u << 18;
    const unsigned int kOptCatalogFlagRemoteDetonate = 1u << 19;
    const unsigned int kOptCatalogFlagTetherGuided = 1u << 20;
    const unsigned int kOptCatalogFlagAppliesTimedHitStatus = 1u << 21;
    const unsigned int kOptCatalogFlagTimedStatusSubtractive = 1u << 9;
    const unsigned int kOptCatalogFlagHeatTimedStatus = 1u << 5;
    const unsigned int kOptCatalogNodeFlagAcceptsTerrainDeformation = 0x10000;
    const unsigned int kOptCatalogFastSqrtBias = 0x1fc00000;
    const int kOptCatalogRequiredVersion = 2;
    const int kMaxQueuedImpacts = 64;
    const float kOptCatalogAimPitchRangeScale = -0.239999995f;
    const float kOptCatalogTrailDamageBlendLimit = 0.25f;
    const double kOptCatalogPi = 3.14159265358979323846;
    const char *kZWeaponInitSourceFile = "D:\\Proj\\GameZRecoil\\zWeapon\\zwep_init.c";

    struct OptCatalogQueuedImpactRecord {
        OptCatalogEntryDef *entry;
        zClass_NodePartial *ownerNode;
        zVec3 sourcePos;
        OptCatalogRaycastHitEntry hit;
        float damageAmount;
        unsigned char unknown_40[4];
    };

    RECOIL_STATIC_ASSERT(sizeof(OptCatalogQueuedImpactRecord) == 68);

    OptCatalogQueuedImpactRecord g_OptCatalogQueuedImpacts[kMaxQueuedImpacts] = {};

    typedef void (RECOIL_THISCALL *OptCatalogRuntimeUpdateCallback)(
        OptCatalogRuntimeInstanceStorage *runtimeInstance);

    struct OptCatalogDamageHealthOverlay {
        unsigned char unknown_00[0x7c];
        float health;
    };

    struct OptCatalogRuntimeInstancePoolSlot {
        OptCatalogRuntimeInstanceStorage runtime;
        unsigned char padding[4];
    };

    RECOIL_STATIC_ASSERT(sizeof(OptCatalogRuntimeInstancePoolSlot) == 0x90);

    int zReaderArrayCount(zReader::Node *node) {
        return node->value.nodes[0].value.i32;
    }

    const char *zReaderArrayString(zReader::Node *node, int index) {
        return node->value.nodes[index].value.str;
    }

    int zReaderArrayInt(zReader::Node *node, int index) {
        return node->value.nodes[index].value.i32;
    }

    float zReaderArrayFloat(zReader::Node *node, int index) {
        zReader::Node *const valueNode = &node->value.nodes[index];
        if (valueNode->type == zReader::ZRDR_NODE_INT) {
            return static_cast<float>(valueNode->value.i32);
        }

        return valueNode->value.f32;
    }

    void SetFlagFromBool(unsigned int &flags, unsigned int flag, int value) {
        if (value != 0) {
            flags |= flag;
        } else {
            flags &= ~flag;
        }
    }

    template <typename T> void *ActionCallbackPtr(T callback) {
        RECOIL_STATIC_ASSERT(sizeof(T) == sizeof(void *));
        union {
            T typed;
            void *raw;
        } ptr = {callback};
        return ptr.raw;
    }

    float FastSqrtApprox(float value) {
        unsigned int bits = 0;
        memcpy(&bits, &value, sizeof(bits));
        bits = (bits >> 1) + kOptCatalogFastSqrtBias;
        memcpy(&value, &bits, sizeof(value));
        return value;
    }

    const char *ReadNamedArrayString(zReader::Node *parentNode, const char *name, int index) {
        zReader::Node *const node = zReader_GetNamedNode(parentNode, name);
        if (node == 0 || node->type != zReader::ZRDR_NODE_ARRAY ||
            zReaderArrayCount(node) <= index) {
            return 0;
        }

        return zReaderArrayString(node, index);
    }

    void LoadNamedSoundSample(zReader::Node *parentNode, const char *name, zSndSample **outSample) {
        const char *const sampleName = ReadNamedArrayString(parentNode, name, 1);
        if (sampleName != 0) {
            *outSample = zSnd::FindSampleByName(sampleName);
        }
    }

    void LoadNamedBoolFlag(zReader::Node *entryNode, const char *name, OptCatalogEntryDef *entry,
                           unsigned int flag) {
        zReader::Node *const node = zReader_GetNamedNode(entryNode, name);
        if (node != 0 && node->type == zReader::ZRDR_NODE_ARRAY && zReaderArrayCount(node) > 1) {
            SetFlagFromBool(entry->flags, flag, zReaderArrayInt(node, 1));
        }
    }

    void LoadRadiusRange(zReader::Node *node, OptCatalogEntryDef *entry) {
        const int count = zReaderArrayCount(node);
        if (count > 1 && zReaderArrayInt(node, 1) != 0) {
            entry->flags |= kOptCatalogFlagCraterImpact;
            if (count > 2) {
                const int minRadius = static_cast<int>(zReaderArrayFloat(node, 1));
                entry->craterRadiusBase = minRadius;
                entry->craterRadiusRandomRange =
                    static_cast<int>(zReaderArrayFloat(node, 2)) - minRadius;
            }
        }
    }

    void LoadTimedStatusBlock(zReader::Node *node, OptCatalogEntryDef *entry) {
        if (zReaderArrayCount(node) <= 6) {
            return;
        }

        entry->timedStatusLightRangeMin = zReaderArrayFloat(node, 1);
        entry->timedStatusLightRangeMax = zReaderArrayFloat(node, 2);
        entry->timedStatusUpdateDelay = zReaderArrayFloat(node, 3);
        entry->timedStatusLightSpecularColor.red = zReaderArrayFloat(node, 4);
        entry->timedStatusLightSpecularColor.green = zReaderArrayFloat(node, 5);
        entry->timedStatusLightSpecularColor.blue = zReaderArrayFloat(node, 6);
        entry->flags |= kOptCatalogFlagAppliesTimedHitStatus;
    }

    void LoadDesignateStatusBlock(zReader::Node *node, OptCatalogEntryDef *entry) {
        if (zReaderArrayCount(node) <= 6) {
            return;
        }

        entry->timedStatusLightRangeMin = zReaderArrayFloat(node, 1);
        entry->timedStatusLightRangeMax = zReaderArrayFloat(node, 2);
        entry->timedStatusUpdateDelay = 0.0f;
        entry->timedStatusLightSpecularColor.red = zReaderArrayFloat(node, 3);
        entry->timedStatusLightSpecularColor.green = zReaderArrayFloat(node, 4);
        entry->timedStatusLightSpecularColor.blue = zReaderArrayFloat(node, 5);
        entry->detonationDistSq = zReaderArrayFloat(node, 6);
        entry->flags &= ~(kOptCatalogFlagAppliesTimedHitStatus | kOptCatalogFlagHeatTimedStatus);
        entry->flags |= kOptCatalogFlagRemoteDetonate;
    }

    void LoadDamageFeedbackOnHealth(zReader::Node *node, OptCatalogEntryDef *entry) {
        const int count = zReaderArrayCount(node) - 1;
        entry->damageFeedbackVariantCount = count > 4 ? 4 : count;
        for (int i = 0; i < entry->damageFeedbackVariantCount; ++i) {
            zReader::Node *const variantNode = &node->value.nodes[i + 1];
            if (variantNode->type == zReader::ZRDR_NODE_ARRAY &&
                zReaderArrayCount(variantNode) > 2) {
                entry->damageFeedbackVariants[i].minFeedbackScale =
                    zReaderArrayFloat(variantNode, 1);
                entry->damageFeedbackVariants[i].effect =
                    zEffectAnim::FindEntryByName(zReaderArrayString(variantNode, 2));
            }
        }
    }

    void LoadImpactFxTable(zReader::Node *impactNode, OptCatalogEntryDef *entry) {
        if (g_zRndr_GlobalStringCount <= 0) {
            return;
        }

        OptCatalog::LoadFxSpecFromReaderNode(impactNode, &entry->impactFxTable[0],
                                             g_zRndr_GlobalStringTable[0]);
        for (int i = 1; i < g_zRndr_GlobalStringCount; ++i) {
            if (zReader_GetNamedNode(impactNode, g_zRndr_GlobalStringTable[i]) != 0) {
                OptCatalog::LoadFxSpecFromReaderNode(impactNode, &entry->impactFxTable[i],
                                                     g_zRndr_GlobalStringTable[i]);
            } else {
                entry->impactFxTable[i] = entry->impactFxTable[0];
            }
        }

        zReader::Node *const animationAlwaysNode =
            zReader_GetNamedNode(impactNode, "ANIMATION_ALWAYS");
        if (animationAlwaysNode != 0) {
            entry->flags |= kOptCatalogFlagAlwaysPlayImpactFx;
        }
    }

    void SetupRuntimeInstancePool() {
        if (g_OptCatalogRuntimeInstanceCount <= 0) {
            return;
        }

        OptCatalogRuntimeInstancePoolSlot *const slots =
            static_cast<OptCatalogRuntimeInstancePoolSlot *>(
                calloc(g_OptCatalogRuntimeInstanceCount, sizeof(OptCatalogRuntimeInstancePoolSlot)));
        g_OptCatalogRuntimeInstancePool = slots;
        if (slots == 0) {
            return;
        }

        for (int i = 0; i < g_OptCatalogRuntimeInstanceCount; ++i) {
            OptCatalogRuntimeInstanceStorage *const runtime = &slots[i].runtime;
            runtime->projectileNode = zClass_Object3D::gwObject3DInit();
            if (runtime->projectileNode != 0) {
                char name[40];
                sprintf(name, "Projectile_%d", i);
                zClass_Class::gwNodeSetName(runtime->projectileNode, name);
                zClass_Class::gwNodeSetRaycastable(runtime->projectileNode, 0);
                zClass_Class::gwNodeSetCellPickable(runtime->projectileNode, 0);
                zClass_Class::gwNodeSetPickable(runtime->projectileNode, 1);
            }

            runtime->flyoutAnimPrimary = 0;
            runtime->flyoutAnimSecondary = 0;
            runtime->asyncFxHandle = 0;
            runtime->next = static_cast<OptCatalogRuntimeInstanceStorage *>(
                g_OptCatalogFreeRuntimeInstanceList);
            g_OptCatalogFreeRuntimeInstanceList = runtime;
        }
    }

    OptCatalogDamageHandlerPartial *DamageHandlerForNode(zClass_NodePartial * node) {
        return static_cast<OptCatalogDamageHandlerPartial *>(
            ((zClass_NodeFreeListSlot *)(node))->damageHandler);
    }

    void ActivateDamageFeedbackEffect(zEffectAnimEntry *effect, OptCatalogHitEventPartial *hitEvent) {
        zEffectAnim::SetTransformRotAndVelocity_Thunk(effect, 0, hitEvent->hitPos.x,
                                                      hitEvent->hitPos.y, hitEvent->hitPos.z,
                                                      0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    }

    zClass_NodePartial *ImpactOwnerNodeFromDamageContext() {
        OptCatalogHitEventPartial *const contextHitEvent =
            static_cast<OptCatalogHitEventPartial *>(g_OptCatalog_DamageContextHitEvent);
        if (contextHitEvent == 0 || contextHitEvent->surfaceRef == 0) {
            return 0;
        }

        return contextHitEvent->surfaceRef->impactOwnerNode;
    }

    unsigned int PackVariantTag(const zTag4Partial *tag) {
        unsigned int packed = 0;
        memcpy(&packed, tag, sizeof(packed));
        return packed;
    }

    void SetCurrentVariantTagFromPacked(unsigned int packedTag) {
        memcpy(&g_Variant_CurrentTag, &packedTag, sizeof(g_Variant_CurrentTag));
    }

    void SetCurrentVariantForRuntime(unsigned int packedRuntimeTag,
                                     unsigned int savedPackedVariantTag) {
        const unsigned char runtimeTagCount =
            static_cast<unsigned char>(packedRuntimeTag & 0xffu);
        if (runtimeTagCount == 4) {
            SetCurrentVariantTagFromPacked(savedPackedVariantTag);
        } else {
            SetCurrentVariantTagFromPacked(packedRuntimeTag);
        }
    }
}

namespace zWeapon_OptCatalog {
    // Reimplements 0x43ca20: zWeapon_OptCatalog::LoadKillVerbString
    // (D:\Proj\GameZRecoil\zWeapon\zwep_init.c)
    RECOIL_NOINLINE void RECOIL_FASTCALL LoadKillVerbString(zReader::Node *entryNode,
                                                            OptCatalogEntryDef *entry) {
        char *const killVerbString = static_cast<char *>(calloc(1, 20));
        entry->killVerbString = killVerbString;

        zReader::Node *const killVerbNode = zReader_GetNamedNode(entryNode, "KILL_VERB");
        const char *sourceText = 0;
        if (killVerbNode != 0) {
            sourceText = zLoc::ResolveMessageKeyOrFallback(zReaderArrayString(killVerbNode, 1));
        } else {
            sourceText = zLoc::GetMessageString(0x250);
        }

        strncpy(killVerbString, sourceText, 19);
    }
}

namespace zWeapon {
    // Reimplements 0x4b1190: zWeapon::LoadOptCatalogFromPath
    // (D:\Proj\GameZRecoil\zWeapon\zwep_init.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL
    LoadOptCatalogFromPath(zClass_NodePartial *worldNode, const char *path, int networkState,
                           zWeaponOptCatalogEntryCallback entryCallback) {
        g_OptCatalogRuntimeWorld = worldNode;
        Light::InitThermalGlowPool();

        zReader::Node *const rootNode = zReader::LoadNodeFromPath(path, 0, 0);
        g_OptCatalogLoadedTreeRoot = rootNode;
        if (rootNode == 0) {
            zError::ReportOld(0x200, kZWeaponInitSourceFile, 0xc6, "Failed to read %s", path);
            return -1;
        }

        int version = 0;
        if (zReader::ReadNamedInt(rootNode, "VERSION", &version) == 0 ||
            version != kOptCatalogRequiredVersion) {
            zError::ReportOld(0x200, kZWeaponInitSourceFile, 0xcb,
                              "Unsupported weapons.zrd version");
            return -1;
        }

        LoadNamedSoundSample(rootNode, "LOCK_ON_WARNING", &g_OptCatalogSndLockOnWarning);
        LoadNamedSoundSample(rootNode, "NO_AMMO_WARNING", &g_OptCatalogSndTriggerInactive);
        LoadNamedSoundSample(rootNode, "TRIGGER_INACTIVE", &g_OptCatalogSndWeaponInactive);
        LoadNamedSoundSample(rootNode, "WEAPON_INACTIVE", &g_OptCatalogSndNoAmmoWarning);
        zReader::ReadNamedFloat(rootNode, "MAX_CRATER_RADIUS", &g_OptCatalogMaxCraterRadius);

        zReader::Node *const ballisticsNode = zReader_GetNamedNode(rootNode, "BALLISTICS");
        if (ballisticsNode != 0 && ballisticsNode->type == zReader::ZRDR_NODE_ARRAY) {
            const int ballisticsCount = zReaderArrayCount(ballisticsNode);
            g_OptCatalog_EntryCount = (ballisticsCount - 1) / 2;
            g_OptCatalog_EntryTable = static_cast<OptCatalogEntryDef *>(
                calloc(g_OptCatalog_EntryCount, sizeof(OptCatalogEntryDef)));

            for (int itemIndex = 1, entryIndex = 0; itemIndex < ballisticsCount;
                 itemIndex += 2, ++entryIndex) {
                OptCatalogEntryDef *const entry = &g_OptCatalog_EntryTable[entryIndex];
                const char *const keyName = zReaderArrayString(ballisticsNode, itemIndex);
                entry->keyName = const_cast<char *>(keyName);
                entry->displayName = entry->keyName;
                entry->description = _strdup(keyName);
                entry->militaryName = _strdup(keyName);
                entry->ordinalIndex = entryIndex;
                entry->ammoOrChargeMax = 50.0f;
                entry->range = 500.0f;
                entry->rangeSq = entry->range * entry->range;
                entry->velocity = 120.0f;
                entry->damage = 0.100000001f;
                entry->timedStatusInterpRate = 1.0f;
                entry->impactFxTable = static_cast<OptCatalogFxSpec *>(
                    calloc(g_zRndr_GlobalStringCount, sizeof(OptCatalogFxSpec)));

                zReader::Node *const entryNode = zReader_GetNamedNode(rootNode, keyName);
                if (entryNode != 0) {
                    const char *stringValue = zReader::ReadNamedString(entryNode, "NAME");
                    if (stringValue != 0) {
                        entry->displayName = const_cast<char *>(stringValue);
                    }

                    stringValue = zReader::ReadNamedString(entryNode, "DESC");
                    if (stringValue != 0) {
                        free(entry->description);
                        entry->description = _strdup(zLoc::ResolveMessageKeyOrFallback(stringValue));
                    }

                    stringValue = zReader::ReadNamedString(entryNode, "MILITARY_NAME");
                    if (stringValue != 0) {
                        free(entry->militaryName);
                        entry->militaryName =
                            _strdup(zLoc::ResolveMessageKeyOrFallback(stringValue));
                    }

                    zReader::ReadNamedFloat(entryNode, "ACCELERATION", &entry->acceleration);

                    int intValue = 0;
                    if (zReader::ReadNamedInt(entryNode, "AMMO_LIMIT", &intValue) != 0) {
                        entry->ammoOrChargeMax = static_cast<float>(intValue);
                    }

                    zReader::Node *fieldNode = zReader_GetNamedNode(entryNode, "BEAM");
                    if (fieldNode != 0 && fieldNode->type == zReader::ZRDR_NODE_ARRAY) {
                        entry->flags |= kOptCatalogFlagTrailRuntime;
                        if (zReaderArrayCount(fieldNode) > 1) {
                            entry->velocity = 1.0f / zReaderArrayFloat(fieldNode, 1);
                        }
                        if (zReaderArrayCount(fieldNode) > 2) {
                            entry->timedStatusInterpRate = zReaderArrayFloat(fieldNode, 2);
                        }
                        if (zReaderArrayCount(fieldNode) > 3) {
                            SetFlagFromBool(entry->flags, 1u, zReaderArrayInt(fieldNode, 3));
                        }
                    }

                    LoadNamedBoolFlag(entryNode, "CATCHES_FIRE", entry,
                                      kOptCatalogFlagImmediateProbeImpact);

                    fieldNode = zReader_GetNamedNode(entryNode, "CRATER");
                    if (fieldNode != 0 && fieldNode->type == zReader::ZRDR_NODE_ARRAY) {
                        LoadRadiusRange(fieldNode, entry);
                    }

                    zReader::ReadNamedFloat(entryNode, "DAMAGE", &entry->damage);
                    float floatValue = 0.0f;
                    if (zReader::ReadNamedFloat(entryNode, "DETONATION_DISTANCE", &floatValue) !=
                        0) {
                        entry->detonationDistSq = floatValue * floatValue;
                    }

                    LoadNamedBoolFlag(entryNode, "EXPIRES", entry, kOptCatalogFlagExpires);
                    if (zReader::ReadNamedFloat(entryNode, "FIRE_RATE", &floatValue) != 0 &&
                        floatValue != 0.0f) {
                        entry->fireRateInterval = 1.0f / floatValue;
                    }
                    LoadNamedBoolFlag(entryNode, "FIXED_ROTATE", entry,
                                      kOptCatalogFlagFixedRotate);

                    if ((entry->flags & kOptCatalogFlagLockOn) == 0) {
                        zReader::ReadNamedFloat(entryNode, "GRAVITY", &entry->gravity);
                    }

                    if (zReader::ReadNamedFloat(entryNode, "IMPACT_PROXIMITY", &floatValue) != 0) {
                        entry->impactProximity = floatValue;
                        entry->damageFalloffRange = floatValue * floatValue;
                    }

                    zReader::ReadNamedInt(entryNode, "IMPACT_TYPE", &entry->damageMaskSlotIndex);
                    LoadNamedBoolFlag(entryNode, "INSTANT", entry, kOptCatalogFlagInstant);

                    if (zReader::ReadNamedFloat(entryNode, "LOCK_ON", &entry->lockOnTime) != 0) {
                        entry->flags |= kOptCatalogFlagLockOn;
                    }
                    LoadNamedBoolFlag(entryNode, "LOCK_ON_LEAD", entry,
                                      kOptCatalogFlagLockOnLead);

                    fieldNode = zReader_GetNamedNode(entryNode, "MINE");
                    if (fieldNode != 0 && fieldNode->type == zReader::ZRDR_NODE_ARRAY &&
                        zReaderArrayCount(fieldNode) > 1) {
                        SetFlagFromBool(entry->flags, kOptCatalogFlagFullProbeDamage,
                                        zReaderArrayInt(fieldNode, 1));
                        if (zReaderArrayInt(fieldNode, 1) != 0) {
                            entry->flags |= 1u;
                        }
                    }

                    LoadNamedBoolFlag(entryNode, "MULTI_TARGET", entry,
                                      kOptCatalogFlagMultiTarget);

                    fieldNode = zReader_GetNamedNode(entryNode, "QUICKSAND");
                    if (fieldNode != 0 && fieldNode->type == zReader::ZRDR_NODE_ARRAY &&
                        zReaderArrayCount(fieldNode) > 1 && zReaderArrayInt(fieldNode, 1) != 0) {
                        entry->flags |= kOptCatalogFlagQuickSandImpact;
                        LoadRadiusRange(fieldNode, entry);
                    }

                    if (zReader::ReadNamedFloat(entryNode, "RANGE", &entry->range) != 0) {
                        entry->rangeSq = entry->range * entry->range;
                    }
                    LoadNamedBoolFlag(entryNode, "RELATIVE_SPEED", entry,
                                      kOptCatalogFlagRelativeSpeed);
                    LoadNamedBoolFlag(entryNode, "REMOTE_DETONATE", entry,
                                      kOptCatalogFlagRemoteDetonate);

                    fieldNode = zReader_GetNamedNode(entryNode, "TETHER_GUIDED");
                    if (fieldNode != 0) {
                        entry->flags |= kOptCatalogFlagTetherGuided;
                    }

                    entry->turnRate = 0.159999996f;
                    zReader::ReadNamedFloat(entryNode, "TURN_RATE", &entry->turnRate);
                    zReader::ReadNamedFloat(entryNode, "TURN_SUSPEND_TIME",
                                            &entry->turnSuspendTime);
                    entry->pitchRate = 0.159999996f;
                    zReader::ReadNamedFloat(entryNode, "PITCH_RATE", &entry->pitchRate);

                    if ((entry->flags & kOptCatalogFlagTrailRuntime) == 0) {
                        zReader::ReadNamedFloat(entryNode, "VELOCITY", &entry->velocity);
                    }
                    LoadNamedBoolFlag(entryNode, "RELOAD", entry, kOptCatalogFlagReload);
                    entry->killVerbString = 0;

                    if (entryCallback != 0) {
                        entryCallback(entryNode, entry);
                    }

                    OptCatalog::LoadFxSpecFromReaderNode(entryNode, &entry->fireFxSpec, "FIRE");
                    OptCatalog::LoadFxSpecFromReaderNode(entryNode, &entry->flyoutFxSpec,
                                                         "FLYOUT");

                    if (zReader::ReadNamedInt(entryNode, "FLYOUT_HEALTH", &intValue) != 0) {
                        entry->flags |= kOptCatalogFlagImpactWhenScaleExpired;
                        entry->flyoutHealth = static_cast<float>(intValue);
                        if (entry->attachCloneTemplateNode != 0) {
                            zClass_Class::gwNodeSetRaycastable(entry->attachCloneTemplateNode, 0);
                        }
                    }

                    fieldNode = zReader_GetNamedNode(entryNode, "IMPACT");
                    if (fieldNode != 0 && entry->impactFxTable != 0) {
                        LoadImpactFxTable(fieldNode, entry);
                    }

                    fieldNode = zReader_GetNamedNode(entryNode, "FREEZE");
                    if (fieldNode != 0 && fieldNode->type == zReader::ZRDR_NODE_ARRAY) {
                        LoadTimedStatusBlock(fieldNode, entry);
                        entry->flags |= kOptCatalogFlagTimedStatusSubtractive;
                    }

                    fieldNode = zReader_GetNamedNode(entryNode, "HEAT");
                    if (fieldNode != 0 && fieldNode->type == zReader::ZRDR_NODE_ARRAY) {
                        LoadTimedStatusBlock(fieldNode, entry);
                        entry->flags |= kOptCatalogFlagHeatTimedStatus;
                    }

                    fieldNode = zReader_GetNamedNode(entryNode, "DESIGNATE");
                    if (fieldNode != 0 && fieldNode->type == zReader::ZRDR_NODE_ARRAY) {
                        LoadDesignateStatusBlock(fieldNode, entry);
                    }

                    stringValue = ReadNamedArrayString(entryNode, "KILL_ANIMATION", 1);
                    if (stringValue != 0) {
                        entry->damageContextEffect = zEffectAnim::FindEntryByName(stringValue);
                    }

                    stringValue = ReadNamedArrayString(entryNode, "DAMAGE_ANIMATION", 1);
                    if (stringValue != 0) {
                        entry->damageFeedbackVariantCount = 1;
                        entry->damageFeedbackVariants[0].minFeedbackScale = 1.0f;
                        entry->damageFeedbackVariants[0].effect =
                            zEffectAnim::FindEntryByName(stringValue);
                    }

                    fieldNode = zReader_GetNamedNode(entryNode, "DAMAGE_ANIM_ON_HEALTH");
                    if (g_zVideo_ActiveRendererPath != 0 && fieldNode != 0 &&
                        fieldNode->type == zReader::ZRDR_NODE_ARRAY) {
                        LoadDamageFeedbackOnHealth(fieldNode, entry);
                    }
                }

                if (entry->gravity != 0.0f) {
                    entry->trailSegmentTimeSec =
                        (entry->velocity * entry->velocity) / (entry->gravity * 2.0f);
                    entry->velocity = FastSqrtApprox(entry->gravity * entry->range * 2.0f);
                }

                if (entry->attachCloneTemplateNode != 0) {
                    zClass_Class::gwNodeSetActive(entry->attachCloneTemplateNode, 1);
                    if (zClass::AnyNodeMatchesPredicateRecursive(
                            entry->attachCloneTemplateNode,
                            zClass_Node::HasRenderableDiPredicate) == 0) {
                        entry->flags |= kOptCatalogFlagSkipTrailSegmentLighting;
                    } else {
                        entry->flags &= ~kOptCatalogFlagSkipTrailSegmentLighting;
                    }
                }

                if ((entry->flags & kOptCatalogFlagTrailRuntime) == 0 &&
                    entry->velocity != 0.0f && entry->fireRateInterval != 0.0f) {
                    g_OptCatalogRuntimeInstanceCount +=
                        static_cast<int>(floor(entry->range / entry->velocity /
                                               entry->fireRateInterval)) +
                        1;
                }
            }
        }

        SetupRuntimeInstancePool();

        zClass_NodePartial *const callbackNode = zClass_Object3D::gwObject3DInit();
        if (callbackNode == 0) {
            zError::ReportOld(0x200, kZWeaponInitSourceFile, 0x2c1,
                              "Failed to allocate weapon tick node");
            g_OptCatalogNetworkOptionState = networkState;
            return 0;
        }

        zClass_Class::gwNodeSetPriority(callbackNode, 3);
        zClass_Class::gwNodeSetActionCallback(
            callbackNode, ActionCallbackPtr(&OptCatalog::ProcessRuntimeInstances));
        g_OptCatalogNetworkOptionState = networkState;
        return 0;
    }
} // namespace zWeapon

namespace OptCatalog {
    // Reimplements 0x4ae380: OptCatalog::BlendDirectionTowardTarget
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL
    BlendDirectionTowardTarget(zVec3 *direction, const zVec3 *targetDirection, float xWeight,
                               float yWeight, float zWeight) {
        direction->x += (targetDirection->x - direction->x) * xWeight;
        direction->y += (targetDirection->y - direction->y) * yWeight;
        direction->z += (targetDirection->z - direction->z) * zWeight;
        zMath::Vec3Normalize(direction);
    }

    // Reimplements 0x4ae3c0: OptCatalog::FindEntryByName
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE OptCatalogEntryDef *RECOIL_FASTCALL FindEntryByName(const char *name) {
        for (int i = 0; i < g_OptCatalog_EntryCount; ++i) {
            OptCatalogEntryDef &entry = g_OptCatalog_EntryTable[i];
            if (entry.keyName != 0 && strcmp(name, entry.keyName) == 0) {
                return &g_OptCatalog_EntryTable[i];
            }
        }

        return 0;
    }

    // Reimplements 0x4ae450: OptCatalog::FindEntryById (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE OptCatalogEntryDef *RECOIL_FASTCALL FindEntryById(int entryId) {
        for (int i = 0; i < g_OptCatalog_EntryCount; ++i) {
            OptCatalogEntryDef &entry = g_OptCatalog_EntryTable[i];
            if (entry.keyName != 0 && entry.ordinalIndex == entryId) {
                return &g_OptCatalog_EntryTable[i];
            }
        }

        return 0;
    }

    // Reimplements 0x4b2130: OptCatalog::CreateTrailSegmentNodeFromTemplate
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL
    CreateTrailSegmentNodeFromTemplate(zClass_NodePartial *templateNode) {
        zClass_NodePartial *const parent = zClass_Object3D::gwObject3DInit();
        zClass_Class::gwNodeSetActive(parent, 1);
        if (templateNode != 0) {
            zClass_Object3D::gwObject3DAddChild(parent, templateNode);
        }

        return parent;
    }

    // Reimplements 0x4b1ec0: OptCatalog::CreateTrailRuntimeState
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE OptCatalogTrailRuntimeState *RECOIL_FASTCALL
    CreateTrailRuntimeState(OptCatalogEntryDef *entry, zClass_NodePartial *projectileNode,
                            zTag4Partial *variantTagPtr, void *reserved, zVec3 *spawnPos,
                            zVec3 *spawnDir, int segmentCount) {
        (void)reserved;

        OptCatalogTrailRuntimeState *const runtime =
            static_cast<OptCatalogTrailRuntimeState *>(
                calloc(1, sizeof(OptCatalogTrailRuntimeState)));
        runtime->ownerEntry = entry;
        runtime->projectileNode = projectileNode;
        runtime->variantTagPtr = variantTagPtr;
        runtime->spawnPos = spawnPos;
        runtime->spawnDir = spawnDir;

        int activeNodeSlotCount = segmentCount;
        if (activeNodeSlotCount > 8) {
            activeNodeSlotCount = 8;
        } else if ((entry->flags & kOptCatalogFlagSingleTrailSegment) != 0) {
            activeNodeSlotCount = 1;
        }

        runtime->activeNodeSlotCount = activeNodeSlotCount;
        for (int i = 0; i < activeNodeSlotCount; ++i) {
            zClass_NodePartial *const node =
                CreateTrailSegmentNodeFromTemplate(entry->attachCloneTemplateNode);
            runtime->activeNodeSlots[i].node = node;

            char nodeName[40];
            sprintf(nodeName, "BeamReflect_%d", i);
            zClass_Class::gwNodeSetName(node, nodeName);
            zClass_Class::gwNodeSetActive(node, 0);
            if ((entry->flags & kOptCatalogFlagSkipTrailSegmentLighting) == 0) {
                zClass_Object3D::gwObject3DSetLitFlag(node, 1);
            }
            zClass_Class::AddChild(g_OptCatalogRuntimeWorld, node);
        }

        return runtime;
    }

    // Reimplements 0x4b2930: OptCatalog_MineIterator::Begin
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE OptCatalogRuntimeInstanceStorage *RECOIL_FASTCALL
    MineIterator_Begin(OptCatalogEntryDef *entry) {
        g_OptCatalog_MineIteratorCursor = entry->activeRuntimeListHead;
        return entry->activeRuntimeListHead;
    }

    // Reimplements 0x4b2940: OptCatalog_MineIterator::Next
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE OptCatalogRuntimeInstanceStorage *RECOIL_CDECL MineIterator_Next() {
        OptCatalogRuntimeInstanceStorage *result = g_OptCatalog_MineIteratorCursor;
        if (result != 0) {
            result = result->next;
            g_OptCatalog_MineIteratorCursor = result;
        }

        return result;
    }

    // Reimplements 0x4ae4a0: OptCatalog::SetPendingSpawnTargetOverrides
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL SetPendingSpawnTargetOverrides(
        void *pendingSpawnTargetCountPtr, void *pendingSpawnTargetListPtr) {
        g_OptCatalogPendingSpawnTargetCountPtr = pendingSpawnTargetCountPtr;
        g_OptCatalogPendingSpawnTargetListPtr = pendingSpawnTargetListPtr;
    }

    // Reimplements 0x4340c0: OptCatalog::AltGunDispatchAllocRuntimeGateCallback
    // (D:\Proj\Battlesport\ai_net.cpp)
    RECOIL_NOINLINE int RECOIL_FASTCALL
    AltGunDispatchAllocRuntimeGateCallback(OptCatalogEntryDef *self, void **saveStateSlot) {
        const int ordinalIndex = self->ordinalIndex;
        if (ordinalIndex == 0 || ordinalIndex == 1) {
            return 1;
        }

        zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(*saveStateSlot);
        if (saveState == 0) {
            return 0;
        }

        if (saveState == (zUtil_SaveGameState *)(g_GameStateOrMapTable)) {
            *saveStateSlot = (void *)(zVideo::ReturnSuccessStub());
            GameNet::SendPkt07_AltGunDispatch(static_cast<short>(ordinalIndex),
                                              (unsigned int)(*saveStateSlot));
            *saveStateSlot = (void *)((unsigned int)(*saveStateSlot) | 0x01000000u);
            return 1;
        }

        const unsigned int dispatchFlags =
            static_cast<unsigned int>(saveState->playerState->altGunDispatchFlags);
        if ((dispatchFlags & 0x02000000u) == 0) {
            return 0;
        }

        *saveStateSlot = (void *)(dispatchFlags);
        return 1;
    }

    // Reimplements 0x434240: OptCatalog::SendPkt0A_RemoveRuntimeRelay
    // (D:\Proj\GameZRecoil\GameNet.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL
    SendPkt0A_RemoveRuntimeRelay(OptCatalogEntryDef *self, zVec3 *pointOrVec3,
                                 zClass_NodePartial *ownerNode) {
        if (g_OptCatalogProcessRuntimeRelayEnabled == 0 || ownerNode == 0) {
            return;
        }

        HudUiMgrSensorTrackNode *const ownerTrackContext =
            (HudUiMgrSensorTrackNode *)(ownerNode->callbackContext);
        if (ownerTrackContext == 0) {
            return;
        }

        zUtil_SaveGameState *const ownerSaveState =
            (zUtil_SaveGameState *)(ownerTrackContext->payload);
        g_NetPkt0A_RemoveRuntimeRelayBuf.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
        g_NetPkt0A_RemoveRuntimeRelayBuf.optCatalogEntryId =
            static_cast<short>(self->ordinalIndex);
        if (pointOrVec3 != 0) {
            g_NetPkt0A_RemoveRuntimeRelayBuf.pointOrVec3 = *pointOrVec3;
        } else {
            g_NetPkt0A_RemoveRuntimeRelayBuf.pointOrVec3.x = 0.0f;
            g_NetPkt0A_RemoveRuntimeRelayBuf.pointOrVec3.y = 0.0f;
            g_NetPkt0A_RemoveRuntimeRelayBuf.pointOrVec3.z = 0.0f;
        }
        g_NetPkt0A_RemoveRuntimeRelayBuf.ownerPlayerKey = ownerSaveState->netPlayerRow->playerKey;
        zNetwork_SendPacketReliable(&g_NetPkt0A_RemoveRuntimeRelayBuf.header);
    }

    // Reimplements 0x4342d0: OptCatalog::HandlePkt0A_RemoveRuntimeRelay
    // (D:\Proj\GameZRecoil\GameNet.cpp)
    RECOIL_NOINLINE int RECOIL_FASTCALL
    HandlePkt0A_RemoveRuntimeRelay(int, NetPkt0A_RemoveRuntimeRelay *packet) {
        OptCatalogEntryDef *const entry =
            OptCatalog::FindEntryById(static_cast<int>(packet->optCatalogEntryId));

        zVec3 relayPointScratch;
        zVec3 *pointOrVec3 = &relayPointScratch;
        if (packet->pointOrVec3.x == 0.0f && packet->pointOrVec3.y == 0.0f &&
            packet->pointOrVec3.z == 0.0f) {
            pointOrVec3 = 0;
        }

        GameNetPlayerRow *const row = GameNet::FindPlayerRowByKey(packet->ownerPlayerKey);
        if (row == 0) {
            return 0;
        }

        zUtil_SaveGameState *const ownerSaveState = (zUtil_SaveGameState *)row->saveState;
        if (entry != 0 && ownerSaveState != 0) {
            g_OptCatalogProcessRuntimeRelayEnabled = 0;
            OptCatalog::RemoveRuntimeInstance(entry, pointOrVec3,
                                              ownerSaveState->playerState->rootNode);
            g_OptCatalogProcessRuntimeRelayEnabled = 1;
        }

        return 1;
    }

    // Reimplements 0x4b1fa0: OptCatalog::LoadFxSpecFromReaderNode
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL
    LoadFxSpecFromReaderNode(zReader::Node *parentNode, OptCatalogFxSpec *spec,
                             const char *childName) {
        zReader::Node *const specNode = zReader_GetNamedNode(parentNode, childName);
        if (specNode == 0) {
            return;
        }

        zReader::Node *fieldNode = zReader_GetNamedNode(specNode, "EFFECT");
        if (fieldNode != 0) {
            if (zReaderArrayCount(fieldNode) > 1) {
                spec->effectTemplateIndex =
                    zEffect::FindTemplateIndexByName(zReaderArrayString(fieldNode, 1));
            }
        } else {
            fieldNode = zReader_GetNamedNode(specNode, "MODEL");
            if (fieldNode != 0 && zReaderArrayCount(fieldNode) > 1) {
                spec->modelNode = zClass::FindByTypeAndName(6, zReaderArrayString(fieldNode, 1));
            }
        }

        fieldNode = zReader_GetNamedNode(specNode, "ANIMATION_ATTACHED");
        if (fieldNode != 0 && zReaderArrayCount(fieldNode) > 1) {
            spec->attachedAnimationEntry =
                zEffectAnim::FindEntryByName(zReaderArrayString(fieldNode, 1));
        }

        fieldNode = zReader_GetNamedNode(specNode, "MODEL_ANIMATION");
        if (fieldNode != 0 && zReaderArrayCount(fieldNode) > 1) {
            spec->modelAnimationEntry =
                zEffectAnim::FindEntryByName(zReaderArrayString(fieldNode, 1));
        }

        fieldNode = zReader_GetNamedNode(specNode, "ANIMATION");
        if (fieldNode != 0 && zReaderArrayCount(fieldNode) > 1) {
            spec->animationEntry =
                zEffectAnim::FindEntryByName(zReaderArrayString(fieldNode, 1));
        }

        fieldNode = zReader_GetNamedNode(specNode, "RANDOM_ROTATE");
        if (fieldNode != 0) {
            spec->flags = ((static_cast<unsigned int>(zReaderArrayInt(fieldNode, 1)) ^
                            spec->flags) &
                           1) ^
                          spec->flags;
        }

        fieldNode = zReader_GetNamedNode(specNode, "SOUND");
        if (fieldNode != 0) {
            const int count = zReaderArrayCount(fieldNode);
            spec->soundCount = count - 1;
            if (count > 1) {
                zSndSample **sample = spec->soundSamples;
                for (int i = 1; i < count; ++i, ++sample) {
                    *sample = zSnd::FindSampleByName(zReaderArrayString(fieldNode, i));
                }
            }
        }

        fieldNode = zReader_GetNamedNode(specNode, "BOUNCE_SOUND");
        if (fieldNode != 0) {
            const int count = zReaderArrayCount(fieldNode);
            spec->bounceSoundCount = count - 1;
            if (count > 1) {
                zSndSample **sample = spec->bounceSoundSamples;
                for (int i = 1; i < count; ++i, ++sample) {
                    *sample = zSnd::FindSampleByName(zReaderArrayString(fieldNode, i));
                }
            }
        }
    }

    // Reimplements 0x4ae4b0: OptCatalog::AllocOrReuseAttachNodeChildClone
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL AllocOrReuseAttachNodeChildClone(
        OptCatalogEntryDef * self) {
        zClass_NodePartial *const clone = self->attachCloneChildFreeList;
        if (clone != 0) {
            self->attachCloneChildFreeList = clone->callbackContext;
            clone->callbackContext = 0;
            return clone;
        }

        return zClass_cls_util::CopyNodeWithCloneOptions(self->attachCloneTemplateNode, 0, 1);
    }

    // Reimplements 0x4ae520: OptCatalog::ClearRuntimeInstanceAsyncFxHandleCallback
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL ClearRuntimeInstanceAsyncFxHandleCallback(
        void *, OptCatalogRuntimeInstanceStorage *runtimeInstance, void *) {
        runtimeInstance->asyncFxHandle = 0;
    }

    // Reimplements 0x4ae4e0: OptCatalog::RecycleAttachNodeClone
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL RecycleAttachNodeClone(
        OptCatalogEntryDef * self, OptCatalogRuntimeInstanceStorage * runtimeInstance) {
        zEffectAnimEntry *const asyncFxHandle = runtimeInstance->asyncFxHandle;
        if (asyncFxHandle != 0) {
            zEffect_Anim::NodeActionCallback(asyncFxHandle, 0);
        }

        zClass_Object3D::RemoveChild(runtimeInstance->projectileNode,
                                     runtimeInstance->attachCloneChild);
        runtimeInstance->attachCloneChild->callbackContext = self->attachCloneChildFreeList;
        self->attachCloneChildFreeList = runtimeInstance->attachCloneChild;
        runtimeInstance->attachCloneChild = 0;
    }

    // Reimplements 0x4ae530: OptCatalog::AllocOrReuseAttachNodeClone
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE OptCatalogRuntimeInstanceStorage *RECOIL_FASTCALL AllocOrReuseAttachNodeClone(
        OptCatalogEntryDef * self) {
        OptCatalogRuntimeInstanceStorage *const runtimeInstance =
            static_cast<OptCatalogRuntimeInstanceStorage *>(g_OptCatalogFreeRuntimeInstanceList);
        if (runtimeInstance == 0) {
            return 0;
        }

        g_OptCatalogFreeRuntimeInstanceList = runtimeInstance->next;

        zClass_NodePartial *attachChildNode = self->attachCloneTemplateNode;
        if (attachChildNode != 0) {
            if (self->flyoutModelAnimationEntry != 0) {
                zClass_NodePartial *const clonedAttachChildNode =
                    AllocOrReuseAttachNodeChildClone(self);
                runtimeInstance->attachCloneChild = clonedAttachChildNode;
                attachChildNode = clonedAttachChildNode;
            }

            zClass_Object3D::gwObject3DAddChild(runtimeInstance->projectileNode, attachChildNode);
        }

        runtimeInstance->lifetime = 0.0f;
        runtimeInstance->updateCallback = 0;
        return runtimeInstance;
    }

    // Reimplements 0x4ae660: OptCatalog::AllocRuntimeInstance
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE OptCatalogRuntimeInstanceStorage *RECOIL_FASTCALL AllocRuntimeInstance(
        OptCatalogEntryDef *self, zClass_NodePartial *ownerNode, zTag4Partial *variantTagOrNull,
        zVec3 *spawnPos, zVec3 *spawnDir, zVec3 *spawnVelocity, void *saveState,
        OptCatalogRuntimeInstanceStorage *runtimeInstanceOrNull) {
        if (g_OptCatalogNetworkOptionState != 0 && g_OptCatalog_AllocRuntimeGateCallback != 0 &&
            g_OptCatalog_AllocRuntimeGateCallback(self, &saveState) == 0) {
            return 0;
        }

        OptCatalogRuntimeInstanceStorage *runtimeInstance = runtimeInstanceOrNull;
        if (runtimeInstance == 0) {
            runtimeInstance = AllocOrReuseAttachNodeClone(self);
            if (runtimeInstance == 0) {
                return 0;
            }
        }

        runtimeInstance->next = self->activeRuntimeListHead;
        self->activeRuntimeListHead = runtimeInstance;
        zClass_Class::AddChild(g_OptCatalogRuntimeWorld, runtimeInstance->projectileNode);

        runtimeInstance->origin = *spawnPos;
        runtimeInstance->pos = *spawnPos;
        runtimeInstance->dir = *spawnDir;
        runtimeInstance->ownerNode = ownerNode;
        runtimeInstance->rangeProgress = 0.0f;
        runtimeInstance->scaleFade = 0.0f;
        runtimeInstance->saveState = saveState;
        runtimeInstance->variantTag = variantTagOrNull != 0 ? PackVariantTag(variantTagOrNull) : 4;
        runtimeInstance->spawnScale = g_OptCatalogNextSpawnScale;
        g_OptCatalogNextSpawnScale = 1.0f;

        runtimeInstance->speed = self->velocity;
        if (self->acceleration == 0.0f &&
            (self->flags & kOptCatalogFlagForceSpawnVelocity) == 0) {
            runtimeInstance->lifetime = self->velocity;
            zMath::Vec3ScaleAdd(spawnVelocity, spawnDir, self->velocity,
                                &runtimeInstance->velocity);
        } else {
            runtimeInstance->lifetime = 0.0000999999975f;
            runtimeInstance->velocity = *spawnVelocity;
            if ((self->flags & kOptCatalogFlagRelativeSpeed) != 0) {
                const float relativeSpeed =
                    sqrtf((spawnVelocity->x * spawnVelocity->x) +
                          (spawnVelocity->y * spawnVelocity->y) +
                          (spawnVelocity->z * spawnVelocity->z));
                runtimeInstance->speed += relativeSpeed;
                runtimeInstance->lifetime += relativeSpeed;
                runtimeInstance->velocity.x -= spawnDir->x * relativeSpeed;
                runtimeInstance->velocity.y -= spawnDir->y * relativeSpeed;
                runtimeInstance->velocity.z -= spawnDir->z * relativeSpeed;
            }
        }

        if (self->fireFxSelectedSoundIndex != -1) {
            self->fireFxSoundSamples[self->fireFxSelectedSoundIndex]->PlayA3D(&runtimeInstance->pos,
                                                                              1.0f, 0);
        }

        if (self->fireFxEffectTemplateIndex != 0) {
            zEffect::SpawnRuntimeInstanceAt(self->fireFxEffectTemplateIndex, &runtimeInstance->pos);
        } else if (self->fireFxSelectedEffectIndex != -1) {
            zEffectAnimEntry *const fireAnim =
                self->fireFxAnimationEntries[self->fireFxSelectedEffectIndex];
            if (fireAnim != 0) {
                float randomRoll = 0.0f;
                if ((self->fireFxFlags & 1u) != 0) {
                    randomRoll =
                        ((static_cast<float>(rand()) * 0.0000305185094f) - 0.5f) *
                        static_cast<float>(kOptCatalogPi);
                }

                zEffectAnim::SetTransformRotAndVelocity_Thunk(
                    fireAnim, 0, runtimeInstance->pos.x, runtimeInstance->pos.y,
                    runtimeInstance->pos.z, asinf(spawnDir->y),
                    static_cast<float>(atan2(-spawnDir->z, -spawnDir->x)), randomRoll, 0.0f,
                    0.0f, 0.0f);
            }
        }

        if ((self->flags & kOptCatalogFlagFlyoutSkipRotation) == 0 &&
            (((self->flags & kOptCatalogFlagFlyoutModelRotation) != 0 &&
              self->attachCloneTemplateNode != 0) ||
             (self->flyoutAnimationEntry != 0 && self->attachCloneTemplateNode == 0))) {
            zClass_Object3D::gwObject3DSetRotation(
                runtimeInstance->projectileNode, asinf(spawnDir->y),
                static_cast<float>(atan2(-spawnDir->z, -spawnDir->x)), 0.0f);
        }

        zClass_Object3D::gwObject3DSetPosition(runtimeInstance->projectileNode,
                                               runtimeInstance->pos.x, runtimeInstance->pos.y,
                                               runtimeInstance->pos.z);

        if (self->flyoutSelectedEffectIndex != -1) {
            if (self->flyoutAnimationEntry != 0) {
                runtimeInstance->flyoutAnimPrimary =
                    zEffectAnim::SetTransformRefs_Thunk(self->flyoutAnimationEntry, 0,
                                                        runtimeInstance->projectileNode, 0,
                                                        runtimeInstance->projectileNode, 0);
            }
            if (self->flyoutAttachedAnimationEntry != 0) {
                runtimeInstance->flyoutAnimSecondary =
                    zEffectAnim::SetPositionRefAndVelocity_Thunk(
                        self->flyoutAttachedAnimationEntry, 0, runtimeInstance->projectileNode, 0,
                        0);
            }
            if (self->flyoutModelAnimationEntry != 0) {
                zEffectAnimEntry *const asyncFxHandle = zEffectAnim::SetVelocity_Thunk(
                    self->flyoutModelAnimationEntry, runtimeInstance->attachCloneChild, 0.0f,
                    0.0f, 0.0f);
                runtimeInstance->asyncFxHandle = asyncFxHandle;
                zEffectAnimEntry::SetOnStateDoneCallback(
                    asyncFxHandle, (void *)(&ClearRuntimeInstanceAsyncFxHandleCallback),
                    runtimeInstance);
            }
        }

        runtimeInstance->aux.x = 0.0f;
        runtimeInstance->aux.y = 0.0f;
        runtimeInstance->aux.z = 0.0f;
        runtimeInstance->spawnGateAccum = 0.0f;
        runtimeInstance->pendingTargetA = 0;
        runtimeInstance->pendingTargetB = 0;
        if ((self->flags & kOptCatalogFlagUsePendingSpawnTarget) != 0 &&
            g_OptCatalogPendingSpawnTargetListPtr != 0) {
            runtimeInstance->aux = *spawnVelocity;
            int *const pendingTargetCount =
                static_cast<int *>(g_OptCatalogPendingSpawnTargetCountPtr);
            if (pendingTargetCount != 0 && *pendingTargetCount > 0) {
                PlayerProgressTargetSlotRuntime *const targetList =
                    static_cast<PlayerProgressTargetSlotRuntime *>(
                        g_OptCatalogPendingSpawnTargetListPtr);
                runtimeInstance->pendingTargetA = targetList[0].targetPos;
                runtimeInstance->pendingTargetB = targetList[0].targetVelocity;
            }
            g_OptCatalogPendingSpawnTargetCountPtr = 0;
        }

        if ((self->flags & kOptCatalogFlagImpactWhenScaleExpired) != 0) {
            zClass_Class::gwNodeSetRaycastable(runtimeInstance->projectileNode, 1);
            runtimeInstance->projectileNode->flags |= 0x08000000;
            ((zClass_NodeFreeListSlot *)(runtimeInstance->projectileNode))->damageHandler =
                (void *)(1);
            runtimeInstance->projectileNode->callbackContext =
                (zClass_NodePartial *)(runtimeInstance);
            runtimeInstance->projectileScale = self->flyoutHealth;
        } else {
            zClass_Class::gwNodeSetRaycastable(runtimeInstance->projectileNode, 0);
            runtimeInstance->projectileNode->flags &= ~0x08000000u;
        }

        self->fireFxSelectedSoundIndex = 0;
        self->fireFxSelectedEffectIndex = 0;
        self->flyoutSelectedEffectIndex = 0;
        return runtimeInstance;
    }

    // Reimplements 0x4aeaa0: OptCatalog::SpawnRuntimeInstanceAt
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE OptCatalogRuntimeInstanceStorage *RECOIL_FASTCALL SpawnRuntimeInstanceAt(
        OptCatalogEntryDef * self, zVec3 * spawnPos, zClass_NodePartial * ownerNode) {
        OptCatalogRuntimeInstanceStorage *const runtimeInstance = AllocOrReuseAttachNodeClone(self);

        runtimeInstance->next = self->activeRuntimeListHead;
        self->activeRuntimeListHead = runtimeInstance;
        runtimeInstance->pos = *spawnPos;
        runtimeInstance->lifetime = 0.0f;
        runtimeInstance->ownerNode = ownerNode;
        runtimeInstance->spawnScale = g_OptCatalogNextSpawnScale;
        g_OptCatalogNextSpawnScale = 1.0f;

        zClass_Class::gwNodeSetRaycastable(runtimeInstance->projectileNode, 1);
        runtimeInstance->projectileNode->flags |= 0x08000000;
        ((zClass_NodeFreeListSlot *)(runtimeInstance->projectileNode))->damageHandler = (void *)(1);
        runtimeInstance->projectileNode->callbackContext =
            (zClass_NodePartial *)(runtimeInstance);
        runtimeInstance->projectileScale = self->flyoutHealth;

        zClass_Object3D::gwObject3DSetPosition(runtimeInstance->projectileNode, spawnPos->x,
                                               spawnPos->y, spawnPos->z);
        zClass_Class::AddChild(g_OptCatalogRuntimeWorld, runtimeInstance->projectileNode);
        return runtimeInstance;
    }

    // Reimplements 0x4aeb50: OptCatalog::RecycleRuntimeInstance
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL RecycleRuntimeInstance(
        OptCatalogEntryDef * self, OptCatalogRuntimeInstanceStorage * runtimeInstance) {
        runtimeInstance->lifetime = 0.0f;

        zEffectAnimEntry *const flyoutAnimPrimary = runtimeInstance->flyoutAnimPrimary;
        if (flyoutAnimPrimary != 0) {
            zEffect_Anim::NodeActionCallback(flyoutAnimPrimary, 0);
            runtimeInstance->flyoutAnimPrimary = 0;
        }

        zEffectAnimEntry *const flyoutAnimSecondary = runtimeInstance->flyoutAnimSecondary;
        if (flyoutAnimSecondary != 0) {
            zEffect_Anim::NodeActionCallback(flyoutAnimSecondary, 0);
            runtimeInstance->flyoutAnimSecondary = 0;
        }

        if (runtimeInstance->attachCloneChild != 0) {
            RecycleAttachNodeClone(self, runtimeInstance);
        }

        zClass_Class::RemoveChild(g_OptCatalogRuntimeWorld, runtimeInstance->projectileNode);
        RecycleRuntimeInstanceStorage(self, runtimeInstance);
    }

    // Reimplements 0x4aebc0: OptCatalog::ClearRuntimeInstances
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL ClearRuntimeInstances(OptCatalogEntryDef * self) {
        OptCatalogRuntimeInstanceStorage *runtimeInstance = self->activeRuntimeListHead;
        self->activeRuntimeListHead = 0;
        while (runtimeInstance != 0) {
            OptCatalogRuntimeInstanceStorage *const next = runtimeInstance->next;
            RecycleRuntimeInstance(self, runtimeInstance);
            runtimeInstance = next;
        }
    }

    // Reimplements 0x4aebf0: OptCatalog::RemoveRuntimeInstance
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE int RECOIL_FASTCALL
    RemoveRuntimeInstance(OptCatalogEntryDef *self, zVec3 *pointOrVec3,
                          zClass_NodePartial *ownerNode) {
        int result = 0;

        if (pointOrVec3 != 0) {
            OptCatalogRuntimeInstanceStorage runtimeInstance = {};
            runtimeInstance.ownerNode = ownerNode;
            runtimeInstance.pos = *pointOrVec3;
            runtimeInstance.spawnScale = 1.0f;
            result = ProcessRuntimeInstance(self, &runtimeInstance);
        } else {
            OptCatalogRuntimeInstanceStorage *runtimeInstance = self->activeRuntimeListHead;
            OptCatalogRuntimeInstanceStorage **link = &self->activeRuntimeListHead;
            while (runtimeInstance != 0) {
                OptCatalogRuntimeInstanceStorage *const next = runtimeInstance->next;
                if ((self->flags & (1u << 20)) != 0 ||
                    (runtimeInstance->lifetime == 0.0f &&
                     (ownerNode == 0 || runtimeInstance->ownerNode == ownerNode))) {
                    *link = next;
                    result += ProcessRuntimeInstance(self, runtimeInstance);
                    RecycleRuntimeInstance(self, runtimeInstance);
                } else {
                    link = &runtimeInstance->next;
                }

                runtimeInstance = next;
            }
        }

        if (result != 0 && g_OptCatalog_RemoveRuntimeRelayCallback != 0) {
            g_OptCatalog_RemoveRuntimeRelayCallback(self, pointOrVec3, ownerNode);
        }

        return result;
    }

    // Reimplements 0x4ae590: OptCatalog::RecycleRuntimeInstanceStorage
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL RecycleRuntimeInstanceStorage(
        OptCatalogEntryDef * self, OptCatalogRuntimeInstanceStorage * runtimeInstance) {
        if (runtimeInstance->lifetime > 0.0f) {
            return;
        }

        zClass_NodePartial *const projectileNode = runtimeInstance->projectileNode;
        while (projectileNode->listCountA != 0) {
            zClass_Class::RemoveChild(projectileNode->listA[0], projectileNode);
        }

        zClass_NodePartial *const attachCloneTemplateNode = self->attachCloneTemplateNode;
        if (attachCloneTemplateNode != 0) {
            if (runtimeInstance->attachCloneChild != 0) {
                RecycleAttachNodeClone(self, runtimeInstance);
            } else {
                zClass_Class::RemoveChild(projectileNode, attachCloneTemplateNode);
            }
        }

        while (projectileNode->listCountB != 0) {
            zClass_Class::RemoveChild(projectileNode, projectileNode->listB[0]);
        }

        runtimeInstance->next =
            static_cast<OptCatalogRuntimeInstanceStorage *>(g_OptCatalogFreeRuntimeInstanceList);
        g_OptCatalogFreeRuntimeInstanceList = runtimeInstance;
        zClass_Object3D::gwObject3DSetScale(projectileNode, 1.0f, 1.0f, 1.0f);
        zClass_Object3D::gwObject3DSetRotation(projectileNode, 0.0f, 0.0f, 0.0f);
        zClass_Object3D::gwObject3DSetPosition(projectileNode, 0.0f, 0.0f, 0.0f);
        ((zClass_NodeFreeListSlot *)(projectileNode))->damageHandler = 0;
    }

    // Reimplements 0x4b1d90: OptCatalog::ShutdownCore
    RECOIL_NOINLINE int RECOIL_CDECL ShutdownCore() {
        for (int i = 0; i < g_OptCatalog_EntryCount; ++i) {
            OptCatalogEntryDef &entry = g_OptCatalog_EntryTable[i];
            if (entry.impactFxTable != 0) {
                free(entry.impactFxTable);
                entry.impactFxTable = 0;
            }
            if (entry.killVerbString != 0) {
                free(entry.killVerbString);
                entry.killVerbString = 0;
            }
            if (entry.description != 0) {
                free(entry.description);
                entry.description = 0;
            }
            if (entry.militaryName != 0) {
                free(entry.militaryName);
                entry.militaryName = 0;
            }

            zClass_NodePartial *impactNode = entry.impactNodeListHead;
            while (impactNode != 0) {
                zClass_NodePartial *const next = impactNode->callbackContext;
                entry.impactNodeListHead = next;
                zClass_Util::DestroyNodeRecursive(impactNode);
                impactNode = entry.impactNodeListHead;
            }
        }

        if (g_OptCatalog_EntryTable != 0) {
            free(g_OptCatalog_EntryTable);
            g_OptCatalog_EntryTable = 0;
        }
        if (g_OptCatalogRuntimeInstancePool != 0) {
            free(g_OptCatalogRuntimeInstancePool);
            g_OptCatalogRuntimeInstancePool = 0;
        }
        Light::DestroyThermalGlowPool();
        g_OptCatalogRuntimeWorld = 0;
        zReader::FreeLoadedTree(g_OptCatalogLoadedTreeRoot);
        g_OptCatalogLoadedTreeRoot = 0;

        g_OptCatalog_EntryCount = 0;
        g_OptCatalog_EntryTable = 0;
        g_OptCatalogRuntimeInstanceCount = 0;
        g_OptCatalogRuntimeInstancePool = 0;
        g_OptCatalogFreeRuntimeInstanceList = 0;
        g_OptCatalogRuntimeWorld = 0;
        g_OptCatalogPendingSpawnTargetCountPtr = 0;
        g_OptCatalogPendingSpawnTargetListPtr = 0;
        g_OptCatalog_FallbackImpactProbeEnabled = 1;
        g_OptCatalog_CaptureHitSnapshotEnabled = 1;
        g_OptCatalogQueuedImpactCount = 0;
        g_OptCatalog_DamageFeedbackHitCount = 0;
        return 0;
    }

    // Reimplements 0x4b1180: OptCatalog::Shutdown (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE int RECOIL_CDECL Shutdown() {
        ShutdownCore();
        return 0;
    }

    // Reimplements 0x4b1f90: OptCatalog::FreeTrailRuntimeStateStorage
    RECOIL_NOINLINE void RECOIL_FASTCALL FreeTrailRuntimeStateStorage(void *trailRuntimeState) {
        free(trailRuntimeState);
    }

    // Reimplements 0x4aefb0: OptCatalog::DeactivateTrailRuntimeState
    // (D:\Proj\Battlesport\OptCatalog.cpp)
    RECOIL_NOINLINE int RECOIL_FASTCALL
    DeactivateTrailRuntimeState(OptCatalogTrailRuntimeState *trailRuntimeState) {
        zSndPlayHandle *const stopSoundHandle = trailRuntimeState->stopSoundHandle;
        OptCatalogEntryDef *const ownerEntry = trailRuntimeState->ownerEntry;

        if (stopSoundHandle != 0) {
            stopSoundHandle->StopIfActive();
        }

        zSndSample *const trailStopSample = ownerEntry->trailStopSample;
        if (trailStopSample != 0) {
            trailStopSample->PlayA3DSimple(1.0f);
        }

        zEffectAnimEntry *const trailEffectAnim = ownerEntry->trailEffectAnim;
        if (trailEffectAnim != 0) {
            zEffectAnim::Stop(trailEffectAnim);
            ownerEntry->trailEffectAnim = 0;
        }

        OptCatalogTrailRuntimeState *const next = trailRuntimeState->next;
        if (next != 0) {
            next->prev = trailRuntimeState->prev;
        }

        OptCatalogTrailRuntimeState *const prev = trailRuntimeState->prev;
        if (prev != 0) {
            prev->next = trailRuntimeState->next;
        }

        if (trailRuntimeState == ownerEntry->activeTrailRuntime) {
            ownerEntry->activeTrailRuntime = trailRuntimeState->next;
        }

        zClass_NodePartial *const lightNode = trailRuntimeState->lightNode;
        trailRuntimeState->prev = 0;
        trailRuntimeState->next = 0;
        if (lightNode != 0) {
            Light::ReturnToFreeList(lightNode);
        }

        for (int i = 0; i < trailRuntimeState->activeNodeSlotCount; ++i) {
            zClass_NodePartial *const node = trailRuntimeState->activeNodeSlots[i].node;
            if (node != 0) {
                zClass_Class::gwNodeSetActive(node, 0);
            }
        }

        trailRuntimeState->activeNodeSlotCursor = 0;
        return 0;
    }

    // Reimplements 0x4aee40: OptCatalog::ActivateTrailRuntimeState
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL
    ActivateTrailRuntimeState(OptCatalogTrailRuntimeState *trailRuntimeState, int playerOrdinal) {
        (void)playerOrdinal;

        OptCatalogEntryDef *const ownerEntry = trailRuntimeState->ownerEntry;
        ownerEntry->trailStopSample->PlayA3DSimple(1.0f);
        zSndPlayHandle *const loopHandle = ownerEntry->trailLoopSample->PlayA3DSimple(1.0f);
        trailRuntimeState->stopSoundHandle = loopHandle;
        if ((ownerEntry->flags & kOptCatalogFlagTrailStartMutedAndLight) != 0) {
            loopHandle->SetFreqScaled(0.0f);
        }

        if (ownerEntry->fireFxEffectTemplateIndex != 0) {
            zEffect::SpawnRuntimeInstanceAt(ownerEntry->fireFxEffectTemplateIndex,
                                            trailRuntimeState->spawnPos);
        } else if (ownerEntry->fireFxAnimationEntries[0] != 0) {
            float randomRoll = 0.0f;
            if ((ownerEntry->fireFxFlags & 1u) != 0) {
                randomRoll = ((static_cast<float>(rand()) * 0.0000305185094f) - 0.5f) *
                             static_cast<float>(kOptCatalogPi);
            }

            ownerEntry->trailEffectAnim = zEffectAnim::SetTransformRotAndVelocity_Thunk(
                ownerEntry->fireFxAnimationEntries[0], 0, trailRuntimeState->spawnPos->x,
                trailRuntimeState->spawnPos->y, trailRuntimeState->spawnPos->z,
                asinf(trailRuntimeState->spawnDir->y),
                static_cast<float>(
                    atan2(-trailRuntimeState->spawnDir->z, -trailRuntimeState->spawnDir->x)),
                randomRoll, 0.0f, 0.0f, 0.0f);
        } else {
            ownerEntry->trailEffectAnim = 0;
        }

        trailRuntimeState->trailDistance = 0.0f;
        trailRuntimeState->volumeFadeTimer = 0.0f;
        trailRuntimeState->alphaPulsePhase = 0.0f;
        trailRuntimeState->spawnScale = g_OptCatalogNextSpawnScale;
        g_OptCatalogNextSpawnScale = 1.0f;

        if ((ownerEntry->flags & kOptCatalogFlagTrailUsePendingSpawnTargets) != 0) {
            trailRuntimeState->pendingSpawnTargetCountPtr =
                static_cast<int *>(g_OptCatalogPendingSpawnTargetCountPtr);
            trailRuntimeState->pendingSpawnTargetListPtr =
                static_cast<PlayerProgressTargetSlotRuntime *>(
                    g_OptCatalogPendingSpawnTargetListPtr);
            g_OptCatalogPendingSpawnTargetCountPtr = 0;
        }

        if ((ownerEntry->flags & kOptCatalogFlagTrailStartMutedAndLight) != 0) {
            zClass_NodePartial *const light =
                Light::AllocFromFreeListAndAttach(&ownerEntry->timedStatusLightSpecularColor);
            trailRuntimeState->lightNode = light;
            zClass_Light::gwLightSetRange(light, ownerEntry->timedStatusLightRangeMin,
                                          ownerEntry->timedStatusLightRangeMax);
            zClass_Class::gwNodeSetActive(light, 0);
        }

        OptCatalogTrailRuntimeState *const activeRuntime = ownerEntry->activeTrailRuntime;
        if (activeRuntime != 0) {
            activeRuntime->prev = trailRuntimeState;
        }
        trailRuntimeState->prev = 0;
        trailRuntimeState->next = activeRuntime;
        ownerEntry->activeTrailRuntime = trailRuntimeState;
    }

    // Reimplements 0x4b0530: OptCatalog::ComputeAimPitchForTarget
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE float RECOIL_FASTCALL
    ComputeAimPitchForTarget(OptCatalogEntryDef *self, const zVec3 *origin,
                             const zVec3 *unusedDirection, const zVec3 *target,
                             float *distanceApproxOut) {
        (void)unusedDirection;

        zVec3 delta;
        delta.x = target->x - origin->x;
        delta.y = target->y - origin->y;
        delta.z = target->z - origin->z;

        const float distanceSq =
            delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
        int distanceBits;
        memcpy(&distanceBits, &distanceSq, sizeof(distanceBits));
        distanceBits = (distanceBits >> 1) + static_cast<int>(kOptCatalogFastSqrtBias);

        float distanceApprox;
        memcpy(&distanceApprox, &distanceBits, sizeof(distanceApprox));
        *distanceApproxOut = distanceApprox;

        if (self->gravity == 0.0f) {
            return -1.0f;
        }

        const float verticalSlope = delta.y / distanceApprox;
        if (distanceApprox < self->range) {
            return verticalSlope -
                   (distanceApprox / self->range) * kOptCatalogAimPitchRangeScale;
        }

        if ((self->flags & kOptCatalogFlagAllowOutOfRangeAimPitch) != 0) {
            return verticalSlope - kOptCatalogAimPitchRangeScale;
        }

        return -1.0f;
    }

    // Reimplements 0x4b0600: OptCatalog::PlayTriggerInactiveWarning
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_CDECL PlayTriggerInactiveWarning() {
        g_OptCatalogSndTriggerInactive->PlayA3DSimple(1.0f);
    }

    // Reimplements 0x4b0620: OptCatalog::PlayWeaponInactiveWarning
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_CDECL PlayWeaponInactiveWarning() {
        g_OptCatalogSndWeaponInactive->PlayA3DSimple(1.0f);
    }

    // Reimplements 0x4b0640: OptCatalog::PlayNoAmmoWarning (D:\Proj\Battlesport\OptCatalog.cpp)
    RECOIL_NOINLINE void RECOIL_CDECL PlayNoAmmoWarning() {
        g_OptCatalogSndNoAmmoWarning->PlayA3DSimple(1.0f);
    }

    // Reimplements 0x4b26f0: OptCatalog::InvokeDamageFeedbackAndHitCallback
    // (D:\Proj\GameZRecoil\zWeapon\OptCatalog.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL InvokeDamageFeedbackAndHitCallback(
        OptCatalogEntryDef *self, zClass_NodePartial *damageOwnerNode, zVec3 *sourcePos,
        OptCatalogHitEventPartial *hitEvent, float damageAmount) {
        int result = 0;
        g_OptCatalog_DamageContextKind = 0;
        g_OptCatalog_DamageContextHitEvent = 0;

        if ((self->flags & kOptCatalogFlagSkipDamageMaskStamp) == 0) {
            ApplyDamageMaskStampOnHit(hitEvent);
        }

        OptCatalogDamageHandlerPartial *const handler = DamageHandlerForNode(hitEvent->hitNode);
        if (handler == 0) {
            return 0;
        }

        if (handler == (OptCatalogDamageHandlerPartial *)(1)) {
            OptCatalogDamageHealthOverlay *const healthOverlay =
                (OptCatalogDamageHealthOverlay *)(hitEvent->hitNode->callbackContext);
            healthOverlay->health -= damageAmount;
        } else if (handler->hitContext != 0) {
            if (g_OptCatalog_CaptureHitSnapshotEnabled == 1) {
                g_OptCatalog_CapturedDamageSourcePos = *sourcePos;
                g_OptCatalog_CapturedDamageHitPos = hitEvent->hitPos;
            }

            g_OptCatalog_CurrentDamageOwnerOrCtx = damageOwnerNode;
            g_OptCatalogDamageFeedbackIntensityScalar = 1.0f;

            OptCatalogDamageFeedbackCallback feedbackCallback =
                (OptCatalogDamageFeedbackCallback)(g_OptCatalogDamageFeedbackCallback);
            if (feedbackCallback != 0) {
                feedbackCallback(handler, hitEvent->hitNode, damageAmount);
            }

            OptCatalogHitCallback hitCallback = (OptCatalogHitCallback)(handler->hitContext);
            result = hitCallback(handler->hitCallback, self, hitEvent, damageAmount);

            if (g_OptCatalog_DamageContextKind != 0) {
                if (self->damageContextEffect != 0) {
                    ActivateDamageFeedbackEffect(self->damageContextEffect, hitEvent);
                }
            } else if (self->damageFeedbackVariantCount != 0) {
                for (int i = 0; i < self->damageFeedbackVariantCount; ++i) {
                    if (g_OptCatalogDamageFeedbackIntensityScalar <=
                        self->damageFeedbackVariants[i].minFeedbackScale) {
                        ActivateDamageFeedbackEffect(self->damageFeedbackVariants[i].effect,
                                                     hitEvent);
                        break;
                    }
                }
            }
        }

        if (g_OptCatalogDamageFeedbackTrackedNode == damageOwnerNode) {
            ++g_OptCatalog_DamageFeedbackHitCount;
        }

        return result;
    }

    // Reimplements 0x4b28e0: OptCatalog::SetDamageContext
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL SetDamageContext(
        int contextKind, OptCatalogHitEventPartial * contextHitEvent) {
        if (contextHitEvent != 0 && contextHitEvent->hitNode != 0) {
            g_OptCatalog_DamageContextHitEvent = contextHitEvent;
        }

        g_OptCatalog_DamageContextKind = contextKind;
    }

    // Reimplements 0x4b2880: OptCatalog::CaptureHitSnapshotAndInvokeDamageTimerCallback
    RECOIL_NOINLINE float RECOIL_FASTCALL CaptureHitSnapshotAndInvokeDamageTimerCallback(
        zVec3 * sourcePos, OptCatalogHitEventPartial * hitEvent, float damageAmount) {
        OptCatalogDamageHandlerPartial *handler = DamageHandlerForNode(hitEvent->hitNode);

        if (g_OptCatalog_CaptureHitSnapshotEnabled == 1) {
            g_OptCatalog_CapturedDamageSourcePos = *sourcePos;
            g_OptCatalog_CapturedDamageHitPos = hitEvent->hitPos;
        }

        OptCatalogDamageTimerCallback callback =
            (OptCatalogDamageTimerCallback)(handler->timerCallback);
        return callback(handler->timerContext, damageAmount);
    }

    // Reimplements 0x4b2910: OptCatalog::GetCapturedHitSourcePtr
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE zVec3 *RECOIL_CDECL GetCapturedHitSourcePtr() {
        return &g_OptCatalog_CapturedDamageSourcePos;
    }

    // Reimplements 0x4b0710: OptCatalog::EmitCraterImpactEvent
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE int RECOIL_FASTCALL
    EmitCraterImpactEvent(OptCatalogEntryDef *self, OptCatalogHitEventPartial *hitEvent,
                          zClass_NodePartial *unusedOwnerNode,
                          zClass_NodePartial *damageOwnerNode) {
        (void)unusedOwnerNode;

        if ((hitEvent->hitNode->flags & kOptCatalogNodeFlagAcceptsTerrainDeformation) == 0) {
            return 0;
        }

        zDEClient_CraterEventTemplate eventTemplate;
        zDEClient_Crater::InitEventTemplateDefaults(&eventTemplate);

        eventTemplate.craterMaterialSlot = (zModel_MaterialSlot *)(hitEvent->surfaceRef);
        eventTemplate.center = hitEvent->hitPos;
        if (self->craterRadiusRandomRange != 0) {
            const unsigned int radius =
                static_cast<unsigned int>(self->craterRadiusBase +
                                          ((rand() * self->craterRadiusRandomRange) >> 15));
            eventTemplate.radius = static_cast<float>(radius);
        } else {
            eventTemplate.radius = self->impactProximity * 0.5f;
            if (g_OptCatalogMaxCraterRadius < eventTemplate.radius) {
                eventTemplate.radius = g_OptCatalogMaxCraterRadius;
            }
        }

        eventTemplate.damageOwnerNode = damageOwnerNode;
        return zDEClient_Crater::InstanceEventMaybeRelay(&eventTemplate) == 0 ? 1 : 0;
    }

    // Reimplements 0x4b0660: OptCatalog::EmitQSandImpactEvent
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL
    EmitQSandImpactEvent(OptCatalogEntryDef *self, OptCatalogHitEventPartial *hitEvent,
                         zClass_NodePartial *unusedOwnerNode,
                         zClass_NodePartial *damageOwnerNode) {
        (void)unusedOwnerNode;

        if ((hitEvent->hitNode->flags & kOptCatalogNodeFlagAcceptsTerrainDeformation) == 0) {
            return;
        }

        zDEClient_QSandEventTemplate eventTemplate;
        zDEClient::CopyQSandEventTemplateDefaults(&eventTemplate);

        eventTemplate.center = hitEvent->hitPos;
        if (self->craterRadiusRandomRange != 0) {
            const unsigned int radius =
                static_cast<unsigned int>(self->craterRadiusBase +
                                          ((rand() * self->craterRadiusRandomRange) >> 15));
            eventTemplate.radius = static_cast<float>(radius);
        } else {
            eventTemplate.radius = self->impactProximity * 0.5f;
            if (g_OptCatalogMaxCraterRadius < eventTemplate.radius) {
                eventTemplate.radius = g_OptCatalogMaxCraterRadius;
            }
        }

        eventTemplate.damageOwnerNode = damageOwnerNode;
        zDEClient_QSand::InstanceEventMaybeRelay(&eventTemplate);
    }

    // Reimplements 0x4b0fd0: OptCatalog::PlayImpactSound
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL
    PlayImpactSound(OptCatalogEntryDef *self, OptCatalogHitEventPartial *hitEvent,
                    int impactSlot, float gainScale) {
        OptCatalogFxSpec *const impactSpec = &self->impactFxTable[impactSlot];
        const int soundCount = impactSpec->soundCount;
        if (soundCount == 0) {
            return;
        }

        const int soundIndex = (rand() * soundCount) >> 15;
        zSndSample *const sample = impactSpec->soundSamples[soundIndex];
        sample->PlayA3D(&hitEvent->hitPos, gainScale, 0);
    }

    // Reimplements 0x4b1030: OptCatalog::PlayBounceSound
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL
    PlayBounceSound(OptCatalogEntryDef *self, OptCatalogRaycastHitEntry *hitEvent,
                    int impactSlot, float gainScale) {
        OptCatalogFxSpec *const impactSpec = &self->impactFxTable[impactSlot];
        const int soundCount = impactSpec->bounceSoundCount;
        if (soundCount == 0) {
            return;
        }

        const int soundIndex = (rand() * soundCount) >> 15;
        zSndSample *const sample = impactSpec->bounceSoundSamples[soundIndex];
        sample->PlayA3D(&hitEvent->pos, gainScale, 0);
    }

    // Reimplements 0x4b07d0: OptCatalog::HandleImpactEvent
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL
    HandleImpactEvent(OptCatalogEntryDef *self, OptCatalogHitEventPartial *hitEvent,
                      OptCatalogRuntimeInstanceStorage *runtimeInstance) {
        int impactSlot = 0;
        if (hitEvent->surfaceRef != 0) {
            impactSlot = hitEvent->surfaceRef->impactSlot;
        }

        if (self->impactCallback != 0) {
            self->impactCallback(self, hitEvent, runtimeInstance);
        }

        const float damageAmount = runtimeInstance->spawnScale * self->damage;
        int damageHandled = InvokeDamageFeedbackAndHitCallback(
            self, runtimeInstance->ownerNode, &runtimeInstance->pos, hitEvent, damageAmount);

        int suppressFallbackFx = 0;
        if ((self->flags & kOptCatalogFlagCraterImpact) != 0) {
            if ((self->flags & kOptCatalogFlagAlwaysPlayImpactFx) == 0) {
                suppressFallbackFx = 1;
            }
            suppressFallbackFx &=
                EmitCraterImpactEvent(self, hitEvent,
                                      hitEvent->surfaceRef != 0
                                          ? hitEvent->surfaceRef->impactOwnerNode
                                          : 0,
                                      runtimeInstance->ownerNode);
        } else if ((self->flags & kOptCatalogFlagQuickSandImpact) != 0) {
            zClass_NodePartial *contextOwnerNode = 0;
            if (g_OptCatalog_DamageContextHitEvent != 0) {
                contextOwnerNode = ImpactOwnerNodeFromDamageContext();
            } else if (hitEvent->surfaceRef != 0) {
                contextOwnerNode = hitEvent->surfaceRef->impactOwnerNode;
            }

            EmitQSandImpactEvent(self, hitEvent, contextOwnerNode, runtimeInstance->ownerNode);
        }

        if (g_OptCatalog_DamageContextKind != 0 &&
            (self->flags & kOptCatalogFlagCraterImpact) != 0) {
            EmitCraterImpactEvent(self, hitEvent, ImpactOwnerNodeFromDamageContext(),
                                  runtimeInstance->ownerNode);
        }

        if ((self->flags & kOptCatalogFlagQuickSandImpact) != 0 &&
            g_OptCatalog_DamageContextHitEvent != 0) {
            EmitQSandImpactEvent(self, hitEvent, ImpactOwnerNodeFromDamageContext(),
                                 runtimeInstance->ownerNode);
            return;
        }

        PlayImpactSound(self, hitEvent, impactSlot, 1.0f);
        if (suppressFallbackFx == 0 && damageHandled == 0) {
            OptCatalogFxSpec *const impactSpec = &self->impactFxTable[impactSlot];
            zEffectAnimEntry *const animationEntry = impactSpec->animationEntry;
            if (animationEntry != 0) {
                zEffectAnim::SetTransformRotAndVelocity_Thunk(
                    animationEntry, 0, hitEvent->hitPos.x, hitEvent->hitPos.y,
                    hitEvent->hitPos.z, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            }

            if (impactSpec->effectTemplateIndex != 0) {
                zEffect::SpawnRuntimeInstanceAt(impactSpec->effectTemplateIndex,
                                                &hitEvent->hitPos);
            }
        }
    }

    // Reimplements 0x4b0980: OptCatalog::HandleImpactEventFromRuntimeState
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL
    HandleImpactEventFromRuntimeState(OptCatalogEntryDef *self,
                                      OptCatalogRuntimeInstanceStorage *runtimeInstance) {
        OptCatalogHitEventPartial hitEvent = {};
        OptCatalogSurfaceMaterialRef surfaceRef = {};

        surfaceRef.flags &= 0xfeff;
        surfaceRef.impactSlot = 0;
        hitEvent.hitPos = runtimeInstance->pos;
        hitEvent.surfaceRef = &surfaceRef;
        hitEvent.hitNode = runtimeInstance->projectileNode;

        HandleImpactEvent(self, &hitEvent, runtimeInstance);
    }

    // Reimplements 0x4b09d0: OptCatalog::BuildImpactHitList
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE int RECOIL_FASTCALL
    BuildImpactHitList(OptCatalogEntryDef *self,
                       OptCatalogRuntimeInstanceStorage *runtimeInstance,
                       int allowOwnerOnlyHit, OptCatalogRaycastHitList *outHitList) {
        zClass_NodePartial *projectileNode = runtimeInstance->projectileNode;
        int restoreRaycastable = 0;
        if (projectileNode != 0 && (projectileNode->flags & 0x10) != 0) {
            restoreRaycastable = 1;
            zClass_Class::gwNodeSetRaycastable(projectileNode, 0);
        }

        int result = zClass_cls_di::FilterRegionsAgainstSphere(
            g_Player_RuntimeDiScene, &runtimeInstance->pos, 0, self->impactProximity, 1, 1,
            outHitList);

        if (restoreRaycastable != 0) {
            zClass_Class::gwNodeSetRaycastable(projectileNode, 1);
        }

        if (allowOwnerOnlyHit == 0 && outHitList->hitCount == 1 &&
            outHitList->hits[0].hitNode == runtimeInstance->ownerNode) {
            result = 1;
        }

        return result == 0 ? 1 : 0;
    }

    // Reimplements 0x4b0a50: OptCatalog::HandleImpactFromRuntimeProbe
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE int RECOIL_FASTCALL
    HandleImpactFromRuntimeProbe(OptCatalogEntryDef *self,
                                 OptCatalogRuntimeInstanceStorage *runtimeInstance,
                                 OptCatalogRaycastHitList *hitList,
                                 void *excludedDamageHandler) {
        int processedAny = 0;
        for (int i = 0; i < hitList->hitCount; ++i) {
            OptCatalogRaycastHitEntry *hit = &hitList->hits[i];
            zClass_NodeFreeListSlot *hitSlot = (zClass_NodeFreeListSlot *)(hit->hitNode);
            if (hitSlot->damageHandler == excludedDamageHandler) {
                continue;
            }

            float damageAmount = self->damage;
            if ((self->flags & kOptCatalogFlagFullProbeDamage) == 0) {
                damageAmount =
                    (1.0f - hit->distance / self->damageFalloffRange) * self->damage;
            }
            damageAmount *= runtimeInstance->spawnScale;

            if ((self->flags & kOptCatalogFlagImmediateProbeImpact) != 0 ||
                g_OptCatalogQueuedImpactCount >= kMaxQueuedImpacts) {
                OptCatalogHitEventPartial *hitEvent = (OptCatalogHitEventPartial *)(void *)(hit);
                InvokeDamageFeedbackAndHitCallback(self, runtimeInstance->ownerNode,
                                                   &runtimeInstance->pos, hitEvent,
                                                   damageAmount);
            } else {
                OptCatalogQueuedImpactRecord *record =
                    &g_OptCatalogQueuedImpacts[g_OptCatalogQueuedImpactCount];
                record->entry = self;
                record->ownerNode = runtimeInstance->ownerNode;
                record->sourcePos = runtimeInstance->pos;
                record->hit = *hit;
                record->damageAmount = damageAmount;
                ++g_OptCatalogQueuedImpactCount;
            }

            processedAny = 1;
        }

        return processedAny;
    }

    // Reimplements 0x4aed00: OptCatalog::ProcessRuntimeInstance
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE int RECOIL_FASTCALL
    ProcessRuntimeInstance(OptCatalogEntryDef *self,
                           OptCatalogRuntimeInstanceStorage *runtimeInstance) {
        zClass_NodePartial *const projectileNode = runtimeInstance->projectileNode;
        zVec3 startPoint = runtimeInstance->pos;
        zVec3 endPoint = runtimeInstance->pos;
        startPoint.y += 1.0f;
        endPoint.y -= self->impactProximity * 0.1f;

        int result = 0;
        int restoreProjectileActive = 0;
        if (projectileNode != 0 && (projectileNode->flags & 0x04) != 0) {
            restoreProjectileActive = 1;
            zClass_Class::gwNodeSetActive(projectileNode, 0);
        }

        PlayerProbeSampleCandidateBuffer rayData = {};
        if (zClass_cls_di::RaycastSelectClosestHitBetweenPoints(
                g_OptCatalogRuntimeWorld, &startPoint, &endPoint, &rayData) == 0) {
            OptCatalogHitEventPartial *const hitEvent =
                (OptCatalogHitEventPartial *)(void *)(&rayData.entries[rayData.candidateCount]);
            HandleImpactEvent(self, hitEvent, runtimeInstance);
            result = 1;
        }

        if (restoreProjectileActive != 0) {
            zClass_Class::gwNodeSetActive(projectileNode, 1);
        }

        if (g_OptCatalog_FallbackImpactProbeEnabled != 0 && self->impactProximity > 0.0f) {
            OptCatalogRaycastHitList fallbackHits = {};
            if (BuildImpactHitList(self, runtimeInstance, 1, &fallbackHits) != 0) {
                runtimeInstance->pos = startPoint;
                HandleImpactFromRuntimeProbe(self, runtimeInstance, &fallbackHits, 0);
            }
        }

        return result;
    }

    // Reimplements 0x4af060: OptCatalog::ProcessRuntimeInstances
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_CDECL ProcessRuntimeInstances() {
        const unsigned int savedPackedVariantTag = PackVariantTag(&g_Variant_CurrentTag);
        float nearestLockOnDistance = static_cast<float>(_HUGE);

        g_OptCatalogRuntimeDeltaTime = g_Time_UnscaledDeltaTimeSec;
        g_OptCatalogRuntimeNowSec = g_Time_UnscaledAccumulatedTimeSec;

        while (g_OptCatalogQueuedImpactCount != 0) {
            --g_OptCatalogQueuedImpactCount;
            OptCatalogQueuedImpactRecord *const record =
                &g_OptCatalogQueuedImpacts[g_OptCatalogQueuedImpactCount];
            InvokeDamageFeedbackAndHitCallback(record->entry, record->ownerNode,
                                               &record->sourcePos,
                                               (OptCatalogHitEventPartial *)(void *)(&record->hit),
                                               record->damageAmount);
        }

        for (int i = 0; i < g_OptCatalog_EntryCount; ++i) {
            OptCatalogEntryDef *const entry = &g_OptCatalog_EntryTable[i];
            if (entry->keyName == 0) {
                continue;
            }

            if ((entry->flags & kOptCatalogFlagTrailRuntime) != 0) {
                OptCatalogTrailRuntimeState *trailRuntime = entry->activeTrailRuntime;
                while (trailRuntime != 0) {
                    OptCatalogTrailRuntimeState *const nextTrailRuntime = trailRuntime->next;
                    if (trailRuntime->variantTagPtr != 0) {
                        g_Variant_CurrentTag = *trailRuntime->variantTagPtr;
                    } else {
                        SetCurrentVariantTagFromPacked(savedPackedVariantTag);
                    }

                    OptCatalogTrailNodeSlot *segment = trailRuntime->activeNodeSlots;
                    if (trailRuntime->spawnPos != 0 && trailRuntime->spawnDir != 0 &&
                        trailRuntime->activeNodeSlotCount > 0) {
                        segment->pos = *trailRuntime->spawnPos;
                        segment->dir = *trailRuntime->spawnDir;
                        trailRuntime->trailDistance +=
                            entry->damageFalloffRange * g_OptCatalogRuntimeDeltaTime;
                        trailRuntime->alphaPulsePhase +=
                            g_OptCatalogRuntimeDeltaTime * 10.0f;

                        int visibleSegmentCount = 1;
                        if (trailRuntime->pendingSpawnTargetCountPtr != 0 &&
                            trailRuntime->pendingSpawnTargetListPtr != 0 &&
                            *trailRuntime->pendingSpawnTargetCountPtr > 1) {
                            float targetProjectionScratch[8] = {};
                            zVec3 sortedDirection = {};
                            ReflectAndSortImpactTraceList(trailRuntime, targetProjectionScratch,
                                                          &sortedDirection);

                            visibleSegmentCount = *trailRuntime->pendingSpawnTargetCountPtr;
                            if (visibleSegmentCount > 4) {
                                visibleSegmentCount = 4;
                            }
                            if (visibleSegmentCount > trailRuntime->activeNodeSlotCount) {
                                visibleSegmentCount = trailRuntime->activeNodeSlotCount;
                            }

                            zVec3 cursor = *trailRuntime->spawnPos;
                            for (int segmentIndex = 0; segmentIndex < visibleSegmentCount;
                                 ++segmentIndex) {
                                OptCatalogTrailNodeSlot *const currentSegment =
                                    &trailRuntime->activeNodeSlots[segmentIndex];
                                currentSegment->pos = cursor;

                                zVec3 *const targetPos =
                                    trailRuntime->pendingSpawnTargetListPtr[segmentIndex].targetPos;
                                zMath::Vec3DirectionTo(&currentSegment->pos, targetPos,
                                                       &currentSegment->dir);
                                currentSegment->scale = zMath::Vec3DeltaLength(&currentSegment->pos,
                                                                               targetPos);
                                ComputeTrailImpactResponse(entry, trailRuntime, currentSegment,
                                                           targetPos);
                                UpdateTrailSegmentVisual(currentSegment);
                                zMath::Vec3ScaleAdd(&currentSegment->pos, &currentSegment->dir,
                                                    currentSegment->scale, &cursor);
                            }
                        } else {
                            segment->scale = entry->range;
                            ComputeTrailImpactResponse(entry, trailRuntime, segment,
                                                       trailRuntime->spawnPos);
                            UpdateTrailSegmentVisual(segment);
                        }

                        for (int segmentIndex = visibleSegmentCount;
                             segmentIndex < trailRuntime->activeNodeSlotCursor; ++segmentIndex) {
                            zClass_NodePartial *const node =
                                trailRuntime->activeNodeSlots[segmentIndex].node;
                            if (node != 0) {
                                zClass_Class::gwNodeSetActive(node, 0);
                            }
                        }
                        trailRuntime->activeNodeSlotCursor = visibleSegmentCount;
                    }

                    trailRuntime = nextTrailRuntime;
                }
                continue;
            }

            OptCatalogRuntimeInstanceStorage **link = &entry->activeRuntimeListHead;
            OptCatalogRuntimeInstanceStorage *runtimeInstance = entry->activeRuntimeListHead;
            while (runtimeInstance != 0) {
                OptCatalogRuntimeInstanceStorage *const nextRuntime = runtimeInstance->next;
                int recycleRuntime = 0;

                if (runtimeInstance->updateCallback != 0) {
                    OptCatalogRuntimeUpdateCallback callback =
                        (OptCatalogRuntimeUpdateCallback)(runtimeInstance->updateCallback);
                    callback(runtimeInstance);
                }

                SetCurrentVariantForRuntime(runtimeInstance->variantTag, savedPackedVariantTag);

                if ((entry->flags & kOptCatalogFlagImpactWhenScaleExpired) != 0 &&
                    runtimeInstance->projectileScale <= 0.0f) {
                    HandleImpactEventFromRuntimeState(entry, runtimeInstance);
                    recycleRuntime = 1;
                } else {
                    if (runtimeInstance->speed != 0.0f) {
                        runtimeInstance->velocity.x =
                            runtimeInstance->dir.x * runtimeInstance->speed;
                        runtimeInstance->velocity.y =
                            runtimeInstance->dir.y * runtimeInstance->speed;
                        runtimeInstance->velocity.z =
                            runtimeInstance->dir.z * runtimeInstance->speed;
                    }

                    runtimeInstance->pos.x +=
                        runtimeInstance->velocity.x * g_OptCatalogRuntimeDeltaTime;
                    runtimeInstance->pos.y +=
                        runtimeInstance->velocity.y * g_OptCatalogRuntimeDeltaTime;
                    runtimeInstance->pos.z +=
                        runtimeInstance->velocity.z * g_OptCatalogRuntimeDeltaTime;
                    runtimeInstance->lifetime += g_OptCatalogRuntimeDeltaTime;

                    if (runtimeInstance->projectileNode != 0) {
                        zClass_Object3D::gwObject3DSetPosition(
                            runtimeInstance->projectileNode, runtimeInstance->pos.x,
                            runtimeInstance->pos.y, runtimeInstance->pos.z);

                        zVec3 direction = runtimeInstance->dir;
                        if (zMath::Vec3Normalize(&direction) != 0.0f) {
                            const float yaw =
                                static_cast<float>(atan2(-direction.x, -direction.z));
                            const float pitch = static_cast<float>(asin(direction.y));
                            zClass_Object3D::gwObject3DSetRotation(runtimeInstance->projectileNode,
                                                                   pitch, yaw, 0.0f);
                        }
                    }

                    if (entry->impactProximity > 0.0f &&
                        ProcessRuntimeInstance(entry, runtimeInstance) != 0) {
                        recycleRuntime = 1;
                    } else if (entry->range > 0.0f &&
                               runtimeInstance->lifetime >= entry->range) {
                        recycleRuntime = 1;
                    }
                }

                if (recycleRuntime != 0) {
                    *link = nextRuntime;
                    RecycleRuntimeInstance(entry, runtimeInstance);
                } else {
                    link = &runtimeInstance->next;

                    if (runtimeInstance->pendingTargetA != 0) {
                        zVec3 *const targetPos =
                            static_cast<zVec3 *>(runtimeInstance->pendingTargetA);
                        const float lockOnDistance =
                            zMath::Vec3DeltaLength(&runtimeInstance->pos, targetPos);
                        if (lockOnDistance < nearestLockOnDistance) {
                            nearestLockOnDistance = lockOnDistance;
                        }
                    }
                }

                runtimeInstance = nextRuntime;
            }
        }

        if (nearestLockOnDistance < static_cast<float>(_HUGE) &&
            g_OptCatalogRuntimeNowSec >= g_OptCatalogLockOnWarningGateTimeSec) {
            if (g_OptCatalogSndLockOnWarning != 0) {
                g_OptCatalogSndLockOnWarning->PlayA3DSimple(1.0f);
            }
            g_OptCatalogLockOnWarningGateTimeSec = g_OptCatalogRuntimeNowSec - 0.5f;
        }

        SetCurrentVariantTagFromPacked(savedPackedVariantTag);
    }

    // Reimplements 0x4b0ba0: OptCatalog::CanSpawnThroughRay
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE int RECOIL_FASTCALL
    CanSpawnThroughRay(OptCatalogEntryDef *self, OptCatalogRaycastHitEntry *hit,
                       const zVec3 *rayStart, const zVec3 *rayEnd, float *rayLengthOut,
                       float *reflectedLengthOut, zVec3 *reflectedDirOut) {
        const float rayLength = zMath::Vec3DeltaLength(&hit->pos, rayStart);
        *rayLengthOut = rayLength;
        if (rayLength == 0.0f) {
            return 2;
        }

        const unsigned int flags = self->flags;
        if ((flags & (1u << 19)) == 0) {
            zClass_NodeFreeListSlot *const hitSlot = (zClass_NodeFreeListSlot *)(hit->hitNode);
            if (hitSlot->damageHandler != 0) {
                if (g_OptCatalog_CaptureHitSnapshotEnabled == 1) {
                    g_OptCatalog_CapturedDamageSourcePos = *rayStart;
                    g_OptCatalog_CapturedDamageHitPos = *rayEnd;
                }

                return 0;
            }
        }

        if ((flags & 1u) == 0) {
            return 2;
        }

        zVec3 incident;
        incident.x = rayEnd->x - rayStart->x;
        incident.y = rayEnd->y - rayStart->y;
        incident.z = rayEnd->z - rayStart->z;
        zMath::Vec3Reflect((zVec3 *)(void *)(hit), &incident, reflectedDirOut);
        *reflectedLengthOut = zMath::Vec3Normalize(reflectedDirOut);
        return 1;
    }

    // Reimplements 0x4b0ca0: OptCatalog::ReflectAndSortImpactTraceList
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL
    ReflectAndSortImpactTraceList(OptCatalogTrailRuntimeState *runtime,
                                  float *targetProjectionScratch, zVec3 *directionOut) {
        zVec3 *farthestTarget = directionOut;
        float farthestDistance = 0.0f;
        for (int i = 0; i < *runtime->pendingSpawnTargetCountPtr; ++i) {
            zVec3 *const targetPos = runtime->pendingSpawnTargetListPtr[i].targetPos;
            const float distance = zMath::Vec3DeltaLength(runtime->spawnPos, targetPos);
            if (distance > farthestDistance) {
                farthestDistance = distance;
                farthestTarget = targetPos;
            }
        }

        zMath::Vec3DirectionTo(runtime->spawnPos, farthestTarget, directionOut);

        for (int i = 0; i < *runtime->pendingSpawnTargetCountPtr; ++i) {
            zVec3 *const targetPos = runtime->pendingSpawnTargetListPtr[i].targetPos;
            zVec3 delta;
            delta.x = targetPos->x - runtime->spawnPos->x;
            delta.y = targetPos->y - runtime->spawnPos->y;
            delta.z = targetPos->z - runtime->spawnPos->z;
            targetProjectionScratch[i] = directionOut->x * delta.x + directionOut->y * delta.y +
                                         directionOut->z * delta.z;
        }

        int swapped;
        do {
            swapped = 0;
            for (int i = 0; i < *runtime->pendingSpawnTargetCountPtr - 1; ++i) {
                if (targetProjectionScratch[i] > targetProjectionScratch[i + 1]) {
                    PlayerProgressTargetSlotRuntime targetSwap = runtime->pendingSpawnTargetListPtr[i];
                    runtime->pendingSpawnTargetListPtr[i] = runtime->pendingSpawnTargetListPtr[i + 1];
                    runtime->pendingSpawnTargetListPtr[i + 1] = targetSwap;

                    const float projectionSwap = targetProjectionScratch[i];
                    targetProjectionScratch[i] = targetProjectionScratch[i + 1];
                    targetProjectionScratch[i + 1] = projectionSwap;
                    swapped = 1;
                }
            }
        } while (swapped != 0);
    }

    // Reimplements 0x4b0e20: OptCatalog::ComputeTrailImpactResponse
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE int RECOIL_FASTCALL
    ComputeTrailImpactResponse(OptCatalogEntryDef *self, OptCatalogTrailRuntimeState *trailRuntime,
                               OptCatalogTrailNodeSlot *segment, const zVec3 *targetPos) {
        SetDamageMaskSlotIndex(self->damageMaskSlotIndex);
        zClass_cls_di::SetStopAfterFirstHit(0x40000);
        zClass_Class::gwNodeSetRaycastable(trailRuntime->projectileNode, 0);

        PlayerProbeSampleCandidateBuffer rayData = {};
        const int raycastResult = zClass_cls_di::RaycastSelectClosestHitBetweenPoints(
            g_OptCatalogRuntimeWorld, &segment->pos, targetPos, &rayData);

        zClass_Class::gwNodeSetRaycastable(trailRuntime->projectileNode, 1);

        if (raycastResult != 0) {
            segment->scale = zMath::Vec3DeltaLength(&segment->pos, targetPos);
            return 0;
        }

        zClassDiPickCandidateEntry *const selectedHit = &rayData.entries[rayData.candidateCount];
        OptCatalogHitEventPartial *const hitEvent =
            (OptCatalogHitEventPartial *)(void *)(selectedHit);
        zClass_NodeFreeListSlot *const hitSlot =
            (zClass_NodeFreeListSlot *)(selectedHit->node);

        if (hitSlot->damageHandler != 0) {
            const double phase = (trailRuntime->trailDistance * kOptCatalogPi) / self->range;
            const float computedBlend = static_cast<float>((cos(phase) + 1.0) * 0.5);
            trailRuntime->trailBlend = computedBlend;
            if (computedBlend > kOptCatalogTrailDamageBlendLimit) {
                trailRuntime->trailBlend = kOptCatalogTrailDamageBlendLimit;
            }

            const float damageAmount = trailRuntime->spawnScale * self->damage *
                                       g_OptCatalogRuntimeDeltaTime * trailRuntime->trailBlend;
            InvokeDamageFeedbackAndHitCallback(self, trailRuntime->projectileNode, &segment->pos,
                                               hitEvent, damageAmount);

            int impactSlot = 0;
            if (hitEvent->surfaceRef != 0) {
                impactSlot = hitEvent->surfaceRef->impactSlot;
            }
            PlayImpactSound(self, hitEvent, impactSlot, 1.0f);
        }

        segment->scale = zMath::Vec3DeltaLength(&segment->pos, &selectedHit->hitPos);
        return 1;
    }

    // Reimplements 0x4b0f70: OptCatalog::UpdateTrailSegmentVisual
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL
    UpdateTrailSegmentVisual(OptCatalogTrailNodeSlot *segment) {
        zClass_Class::gwNodeSetActive(segment->node, 1);
        zClass_Object3D::gwObject3DSetPosition(segment->node, segment->pos.x, segment->pos.y,
                                               segment->pos.z);

        const float yaw = static_cast<float>(atan2(-segment->dir.x, -segment->dir.z));
        const float pitch = static_cast<float>(asin(segment->dir.y));
        zClass_Object3D::gwObject3DSetRotation(segment->node, pitch, yaw, 0.0f);
        zClass_Object3D::gwObject3DSetScale(segment->node, 1.0f, 1.0f, segment->scale);
    }
}

namespace DamageFeedback {
    // Reimplements 0x4b2900: DamageFeedback::SetIntensityScalar
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_STDCALL SetIntensityScalar(float scalar) {
        g_OptCatalogDamageFeedbackIntensityScalar = scalar;
    }
}

namespace HitContext {
    // Reimplements 0x4b2920: HitContext::GetCurrentOwnerOrCtx
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void *RECOIL_CDECL GetCurrentOwnerOrCtx() {
        return g_OptCatalog_CurrentDamageOwnerOrCtx;
    }
}
