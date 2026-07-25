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

namespace {
const unsigned int kRandUnitScaleBits = 0x38000100u;
const unsigned int kEffectAnimNeedsCopiedRootFlag = 0x00008000u;
const char *kZeffAnimInitSourceFile = "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_init.c";
const char *kAnimationNodeNotFoundMessage =
    "Animation node not found.\n  Animation: %s; Node: %s\n";

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
} // namespace

namespace zEffect_Anim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.init
 * @recoil-artifact defines .text recoil:function:0x45e100: zEffect_Anim::Init.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_init.c.
 * Purpose: reset animation globals, seed runtime random values, and register
 * animation save/load ZAR section handlers.
 */
int Init() {
    if (g_zEffectAnim_EntriesInstantiated != 0) {
        Shutdown();
    }

    g_zEffectAnim_ZbdFilename[0] = '\0';
    g_zEffectAnim_EntriesInstantiated = 0;
    g_zEffectAnim_HeapPtr = 0;
    g_zEffectAnim_CountsPackedLoWord = 0;
    g_zEffectAnim_EntryCount = 0;
    g_zEffectAnim_EntryList = 0;
    g_zEffectAnim_TextIdEntryCount = 0;
    g_zEffectAnim_TextIdEntryList = 0;
    g_zEffect_World = 0;
    g_zEffect_ConditionalRefPosEnabled = 0;
    g_zEffect_VariantOverrideEnabled = 0;
    g_zEffect_DefaultGravity = -9.8f;

    srand((unsigned int)(time(0)));
    *(unsigned int *)(&g_zEffect_RandUnitScale) = kRandUnitScaleBits;
    {
        int valueIndex1;
        for (valueIndex1 = 0; valueIndex1 < (int)(sizeof(g_zEffect_RandUnitTable) /
                                                  sizeof((g_zEffect_RandUnitTable)[0]));
            ++valueIndex1) {
            float &value = (g_zEffect_RandUnitTable)[valueIndex1];
            value = (float)(rand()) * g_zEffect_RandUnitScale;
        }
    }

    if (g_zEffectAnim_EnableZarRegistration != 0) {
        zUtil_ZAR::RegisterSectionHandler(
            g_zEffectAnim_ZarSectionName_AnimActivation,
            (zZbdSectionCallback)(&SaveActivationRecords),
            (zZbdSectionCallback)(&LoadActivationRecords),
            0x32,
            0
        );
        zUtil_ZAR::RegisterSectionHandler(
            g_zEffectAnim_ZarSectionName_RunningAnim,
            (zZbdSectionCallback)(&SaveRunningAnimRecords),
            (zZbdSectionCallback)(&LoadRunningAnimRecords),
            0x33,
            0
        );
        zUtil_ZAR::RegisterSectionHandler(
            g_zEffectAnim_ZarSectionName_Anim,
            (zZbdSectionCallback)(&SaveAnimRecords),
            (zZbdSectionCallback)(&LoadAnimRecords),
            0x34,
            0
        );
    }

    return 0;
}
} // namespace zEffect_Anim

namespace zEffect {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.setworldnode
 * @recoil-artifact defines .text recoil:function:0x45e200: zEffect::SetWorldNode.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff.c.
 * Purpose: store the world node used by zEffect runtime handlers.
 */
void __fastcall SetWorldNode(
    zClass_NodePartial *worldNode
) {
    g_zEffect_World = worldNode;
}

} // namespace zEffect

namespace zEffect_Anim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.setzbdfilename
 * @recoil-artifact defines .text recoil:function:0x45e210: zEffect_Anim::SetZbdFilename.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_init.c.
 * Purpose: store the animation ZBD filename after enforcing the retail length
 * limit.
 */
void __fastcall SetZbdFilename(
    const char *filename
) {
    if (strlen(filename) > 0x80) {
        zError::ReportOld(
            0x400,
            kZeffAnimInitSourceFile,
            0xd1,
            "Animation ZBD filename too long: %s\n",
            filename
        );
        return;
    }

    strcpy(
        g_zEffectAnim_ZbdFilename,
        filename
    );
}

} // namespace zEffect_Anim

namespace zEffect {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.setresourcenode
 * @recoil-artifact defines .text recoil:function:0x45e270: zEffect::SetResourceNode.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff.c.
 * Purpose: store the resource node used by zEffect initialization and runtime
 * lookup.
 */
void __fastcall SetResourceNode(
    zClass_NodePartial *resourceNode
) {
    g_zEffect_ResourceNode = resourceNode;
}

} // namespace zEffect

namespace zEffectAnim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.findsoundrefindexbyname
 * @recoil-artifact defines .text recoil:function:0x45e280: zEffectAnim::FindSoundRefIndexByName (zeff_anim.c)
 * Purpose: Return the first runtime sound reference index whose node name matches.
 */
int __fastcall FindSoundRefIndexByName(
    zEffectAnimEntry *self,
    const char *name
) {
    for (int i = 0; i < self->soundRefCount; ++i) {
        zClass_NodePartial *const node = self->soundRefList[i].runtimeNode;
        if (node != 0 && strcmp(
            node->name,
            name
        ) == 0) {
            return i;
        }
    }

    return -1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.findlightrefindexbyname
 * @recoil-artifact defines .text recoil:function:0x45e300: zEffectAnim::FindLightRefIndexByName (zeff_anim.c)
 * Purpose: Return the first runtime light reference index whose node name matches.
 */
int __fastcall FindLightRefIndexByName(
    zEffectAnimEntry *self,
    const char *name
) {
    for (int i = 0; i < self->lightRefCount; ++i) {
        zClass_NodePartial *const node = self->lightRefList[i].runtimeNode;
        if (node != 0 && strcmp(
            node->name,
            name
        ) == 0) {
            return i;
        }
    }

    return -1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.findorcreatesoundref
 * @recoil-artifact defines .text recoil:function:0x45e380: zEffectAnim::FindOrCreateSoundRef.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim.c.
 * Purpose: find an existing runtime sound reference or create a named sound
 * node reference for an animation entry.
 */
int __fastcall FindOrCreateSoundRef(
    zEffectAnimEntry *self,
    const char *name
) {
    const int existingIndex = FindSoundRefIndexByName(
        self,
        name
    );
    if (existingIndex > 0) {
        return existingIndex;
    }

    zClass_NodePartial *const node = zClass_Sound::gwSoundNew();
    if (node == 0) {
        return -1;
    }

    zClass_Class::gwNodeSetName(
        node,
        name
    );
    zClass_Sound::SetSampleSetByName(
        node,
        name
    );

    if (self->soundRefList == 0) {
        const int initialCount = (int)(self->soundRefCount) + 1;
        self->soundRefList = (zEffectAnimRuntimeNodeRef *)(realloc(
            0,
            initialCount * sizeof(zEffectAnimRuntimeNodeRef)
        ));
        memset(
            self->soundRefList,
            0,
            sizeof(zEffectAnimRuntimeNodeRef)
        );
        ++self->soundRefCount;
    }

    if (self->soundRefCount == 0xff) {
        zError::ReportOld(
            0x400,
            kZeffAnimInitSourceFile,
            0x1b7,
            "Sound list overflow.\n  Animation: %s\n",
            self
        );
        return -1;
    }

    const int resizedCount = (int)(self->soundRefCount) + 1;
    self->soundRefList = (zEffectAnimRuntimeNodeRef *)(realloc(
        self->soundRefList,
        resizedCount * sizeof(zEffectAnimRuntimeNodeRef)
    ));

    zEffectAnimRuntimeNodeRef *const newRef = &self->soundRefList[self->soundRefCount];
    memcpy(
        newRef->name.text,
        node->name,
        sizeof(newRef->name.text)
    );
    newRef->runtimeNode = node;
    newRef->isAttached = 0;
    ++self->soundRefCount;
    return (int)(self->soundRefCount) - 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.findorcreatelightref
 * @recoil-artifact defines .text recoil:function:0x45e4a0: zEffectAnim::FindOrCreateLightRef.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim.c.
 * Purpose: find an existing runtime light reference or create a named light
 * node reference for an animation entry.
 */
int __fastcall FindOrCreateLightRef(
    zEffectAnimEntry *self,
    const char *name
) {
    const int existingIndex = FindLightRefIndexByName(
        self,
        name
    );
    if (existingIndex > 0) {
        return existingIndex;
    }

    zClass_NodePartial *const node = zClass_Light::gwLightNew();
    if (node == 0) {
        return -1;
    }

    zClass_Class::gwNodeSetName(
        node,
        name
    );

    if (self->lightRefList == 0) {
        const int initialCount = (int)(self->lightRefCount) + 1;
        self->lightRefList = (zEffectAnimRuntimeNodeRef *)(realloc(
            0,
            initialCount * sizeof(zEffectAnimRuntimeNodeRef)
        ));
        memset(
            self->lightRefList,
            0,
            sizeof(zEffectAnimRuntimeNodeRef)
        );
        ++self->lightRefCount;
    }

    if (self->lightRefCount == 0xff) {
        zError::ReportOld(
            0x400,
            kZeffAnimInitSourceFile,
            0x200,
            "Light list overflow.\n  Animation: %s\n",
            self
        );
        return -1;
    }

    const int resizedCount = (int)(self->lightRefCount) + 1;
    self->lightRefList = (zEffectAnimRuntimeNodeRef *)(realloc(
        self->lightRefList,
        resizedCount * sizeof(zEffectAnimRuntimeNodeRef)
    ));

    zEffectAnimRuntimeNodeRef *const newRef = &self->lightRefList[self->lightRefCount];
    memcpy(
        newRef->name.text,
        node->name,
        sizeof(newRef->name.text)
    );
    newRef->runtimeNode = node;
    newRef->isAttached = 0;
    ++self->lightRefCount;
    return (int)(self->lightRefCount) - 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.resolvenodebyname
 * @recoil-artifact defines .text recoil:function:0x45e5c0: zEffectAnim::ResolveNodeByName (zeff_anim.c)
 * Purpose: Resolve an animation node name through callback, bound, runtime-ref, then zClass lookup paths.
 */
zClass_NodePartial *__fastcall ResolveNodeByName(
    zEffectAnimEntry *self,
    const char *name
) {
    zClass_NodePartial *resolvedNode = FindNodeRecursiveByName(
        self->callbackNode,
        name
    );
    if (resolvedNode != 0) {
        return resolvedNode;
    }

    resolvedNode = FindNodeRecursiveByName(
        self->boundNode,
        name
    );
    if (resolvedNode != 0) {
        return resolvedNode;
    }

    const int lightRefIndex = FindLightRefIndexByName(
        self,
        name
    );
    if (lightRefIndex > 0) {
        resolvedNode = self->lightRefList[lightRefIndex].runtimeNode;
    }
    if (resolvedNode != 0) {
        return resolvedNode;
    }

    const int soundRefIndex = FindSoundRefIndexByName(
        self,
        name
    );
    if (soundRefIndex > 0) {
        resolvedNode = self->soundRefList[soundRefIndex].runtimeNode;
    }
    if (resolvedNode != 0) {
        return resolvedNode;
    }

    return zClass::FindByTypeAndName(
        6,
        name
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.findnoderecursivebyname
 * @recoil-artifact defines .text recoil:function:0x45e650: zEffectAnim::FindNodeRecursiveByName (zeff_anim.c)
 * Purpose: Return the first node in the root-first child traversal whose name matches.
 */
zClass_NodePartial *__fastcall FindNodeRecursiveByName(
    zClass_NodePartial *rootNode,
    const char *name
) {
    if (rootNode == 0) {
        return 0;
    }

    if (strcmp(
        rootNode->name,
        name
    ) == 0) {
        return rootNode;
    }

    for (int i = 0; i < rootNode->listCountB; ++i) {
        zClass_NodePartial *const childMatch = FindNodeRecursiveByName(
            rootNode->listB[i],
            name
        );
        if (childMatch != 0) {
            return childMatch;
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.ensurecopiedroottree
 * @recoil-artifact defines .text recoil:function:0x45e6d0: zEffectAnim::EnsureCopiedRootTree.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim.c.
 * Purpose: copy and rebind an animation root when the entry is marked as
 * needing an owned runtime tree.
 */
int __fastcall EnsureCopiedRootTree(
    zEffectAnimEntry *self,
    zClass_NodePartial *sourceRoot
) {
    if (self == 0) {
        return 0;
    }

    if ((self->flags & kEffectAnimNeedsCopiedRootFlag) != 0) {
        zClass_NodePartial *const copiedRoot = zClass_cls_util::CopyNode(
            sourceRoot,
            g_zEffectAnim_CopyNodeMode,
            g_zEffectAnim_CopyNodeArg1,
            g_zEffectAnim_CopyNodeArg2
        );
        self->boundNode = copiedRoot;
        if (copiedRoot == 0) {
            return 0;
        }

        RebindEntryToNode(
            self,
            copiedRoot
        );
        self->flags &= ~kEffectAnimNeedsCopiedRootFlag;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.cloneentryfornode
 * @recoil-artifact defines .text recoil:function:0x45e730: zEffectAnim::CloneEntryForNode
 * Purpose: Clone an animation entry and rebuild its runtime node, refs, and copied lists.
 */
zEffectAnimEntry *__fastcall CloneEntryForNode(
    zEffectAnimEntry *self,
    zClass_NodePartial *node
) {
    if (self == 0) {
        return 0;
    }

    zEffectAnimEntry *const clonedEntry = (zEffectAnimEntry *)(calloc(
        1,
        sizeof(zEffectAnimEntry)
    ));
    memcpy(
        clonedEntry,
        self,
        sizeof(zEffectAnimEntry)
    );

    clonedEntry->runtimeNode = zClass_Object3D::gwObject3DInit();

    char runtimeNodeName[0x24];
    sprintf(
        runtimeNodeName,
        "_%s",
        self->name
    );
    zClass_Class::gwNodeSetName(
        clonedEntry->runtimeNode,
        runtimeNodeName
    );
    zClass_Class::gwNodeSetPriority(
        clonedEntry->runtimeNode,
        clonedEntry->priority
    );

    clonedEntry->runtimeSibling = 0;
    clonedEntry->flags &= ~0x00000100u;

    if (node != 0) {
        clonedEntry->boundNode = node;
        strcpy(
            clonedEntry->rootNodeName,
            node->name
        );
    } else {
        zClass_NodePartial *const copiedRoot = zClass_cls_util::CopyNode(
            self->boundNode,
            g_zEffectAnim_CopyNodeMode,
            g_zEffectAnim_CopyNodeArg1,
            g_zEffectAnim_CopyNodeArg2
        );
        clonedEntry->boundNode = copiedRoot;
        if (copiedRoot == 0) {
            zError::ReportOld(
                0x400,
                kZeffAnimInitSourceFile,
                0x26d1,
                "ERROR:\n  Copying Animation Node Tree: %s\n",
                clonedEntry
            );
            return 0;
        }
    }

    if (self->callbackNode == self->boundNode) {
        clonedEntry->callbackNode = clonedEntry->boundNode;
        strcpy(
            clonedEntry->attachNodeName,
            clonedEntry->boundNode->name
        );
    } else {
        clonedEntry->callbackNode =
            FindNodeRecursiveByName(
                clonedEntry->boundNode,
                self->attachNodeName
            );
    }

    if (clonedEntry->callbackNode == 0) {
        zError::ReportOld(
            0x400,
            kZeffAnimInitSourceFile,
            0x26e7,
            kAnimationNodeNotFoundMessage,
            clonedEntry,
            self->attachNodeName
        );
        clonedEntry->activationState = 5;
        return 0;
    }

    if (clonedEntry->lightRefCount > 0) {
        const int count = clonedEntry->lightRefCount;
        clonedEntry->lightRefList =
            (zEffectAnimRuntimeNodeRef *)(calloc(
                count,
                sizeof(zEffectAnimRuntimeNodeRef)
            ));
        memcpy(
            clonedEntry->lightRefList,
            self->lightRefList,
            sizeof(zEffectAnimRuntimeNodeRef) * count
        );

        for (int i = 0; i < count; ++i) {
            zEffectAnimRuntimeNodeRef *const lightRef = &clonedEntry->lightRefList[i];
            if (self->lightRefList[i].runtimeNode != 0) {
                zClass_NodePartial *const lightNode = zClass_Light::gwLightNew();
                lightRef->runtimeNode = lightNode;
                zClass_Class::gwNodeSetName(
                    lightNode,
                    lightRef->name.text
                );
            }
            lightRef->isAttached = 0;
        }
    }

    if (clonedEntry->soundRefCount > 0) {
        const int count = clonedEntry->soundRefCount;
        clonedEntry->soundRefList =
            (zEffectAnimRuntimeNodeRef *)(calloc(
                count,
                sizeof(zEffectAnimRuntimeNodeRef)
            ));
        memcpy(
            clonedEntry->soundRefList,
            self->soundRefList,
            sizeof(zEffectAnimRuntimeNodeRef) * count
        );

        for (int i = 0; i < count; ++i) {
            zEffectAnimRuntimeNodeRef *const soundRef = &clonedEntry->soundRefList[i];
            if (self->soundRefList[i].runtimeNode != 0) {
                zClass_NodePartial *const soundNode = zClass_Sound::gwSoundNew();
                soundRef->runtimeNode = soundNode;
                zClass_Class::gwNodeSetName(
                    soundNode,
                    soundRef->name.text
                );
                zClass_Sound::SetSampleSetByName(
                    soundNode,
                    soundRef->name.text
                );
            }
            soundRef->isAttached = 0;
        }
    }

    if (clonedEntry->trackedNodeCount > 0) {
        const int count = clonedEntry->trackedNodeCount;
        clonedEntry->trackedNodeList =
            (zEffectAnimTrackedNode *)(calloc(
                count,
                sizeof(zEffectAnimTrackedNode)
            ));
        memcpy(
            clonedEntry->trackedNodeList,
            self->trackedNodeList,
            sizeof(zEffectAnimTrackedNode) * count
        );

        for (int i = 0; i < count; ++i) {
            if (self->trackedNodeList[i].trackedNode != 0) {
                clonedEntry->trackedNodeList[i].trackedNode =
                    ResolveNodeByName(
                        clonedEntry,
                        clonedEntry->trackedNodeList[i].trackedNodeName
                    );
            }
        }
    }

    if (clonedEntry->nodeRefCount > 0) {
        const int count = clonedEntry->nodeRefCount;
        clonedEntry->nodeRefList =
            (zEffectAnimNodeRef28 *)(calloc(
                count,
                sizeof(zEffectAnimNodeRef28)
            ));
        memcpy(
            clonedEntry->nodeRefList,
            self->nodeRefList,
            sizeof(zEffectAnimNodeRef28) * count
        );

        for (int i = 0; i < count; ++i) {
            if (self->nodeRefList[i].node != 0) {
                clonedEntry->nodeRefList[i].node =
                    ResolveNodeByName(
                        clonedEntry,
                        clonedEntry->nodeRefList[i].name.text
                    );
            }
        }
    }

    if (clonedEntry->runtimeSequenceCount > 0) {
        const int count = clonedEntry->runtimeSequenceCount;
        clonedEntry->runtimeList =
            (zEffectAnimSurfaceRuntime *)(calloc(
                count,
                sizeof(zEffectAnimSurfaceRuntime)
            ));

        for (int i = 0; i < count; ++i) {
            memcpy(
                &clonedEntry->runtimeList[i],
                &self->runtimeList[i],
                sizeof(zEffectAnimSurfaceRuntime)
            );

            const int eventStreamSize = self->runtimeList[i].eventStreamSize;
            if (eventStreamSize > 0) {
                clonedEntry->runtimeList[i].eventStream = calloc(
                    1,
                    eventStreamSize
                );
                memcpy(
                    clonedEntry->runtimeList[i].eventStream,
                    self->runtimeList[i].eventStream,
                    eventStreamSize
                );
            }
        }
    }

    if (self->surfacePrimary.eventStreamSize > 0) {
        clonedEntry->surfacePrimary.eventStream = calloc(
            1,
            self->surfacePrimary.eventStreamSize
        );
        memcpy(
            clonedEntry->surfacePrimary.eventStream,
            self->surfacePrimary.eventStream,
            self->surfacePrimary.eventStreamSize
        );
    }

    if (clonedEntry->sampleRefCount > 0) {
        const int count = clonedEntry->sampleRefCount;
        clonedEntry->sampleRefList =
            (zEffectAnimSampleRef *)(calloc(
                count,
                sizeof(zEffectAnimSampleRef)
            ));
        memcpy(
            clonedEntry->sampleRefList,
            self->sampleRefList,
            sizeof(zEffectAnimSampleRef) * count
        );
    }

    if (clonedEntry->effectTemplateRefCount > 0) {
        const int count = clonedEntry->effectTemplateRefCount;
        clonedEntry->effectTemplateRefList =
            (zEffectAnimTemplateIndexRef *)(calloc(
                count,
                sizeof(zEffectAnimTemplateIndexRef)
            ));
        memcpy(
            clonedEntry->effectTemplateRefList,
            self->effectTemplateRefList,
            sizeof(zEffectAnimTemplateIndexRef) * count
        );
    }

    if (clonedEntry->activationPrereqCount > 0) {
        const int count = clonedEntry->activationPrereqCount;
        clonedEntry->activationPrereqList =
            (zEffectAnimActivationPrereq *)(calloc(
                count,
                sizeof(zEffectAnimActivationPrereq)
            ));
        memcpy(
            clonedEntry->activationPrereqList,
            self->activationPrereqList,
            sizeof(zEffectAnimActivationPrereq) * count
        );
    }

    if (clonedEntry->runtimeRefCount > 0) {
        const int count = clonedEntry->runtimeRefCount;
        clonedEntry->runtimeRefList =
            (zEffectAnimRuntimeRef *)(calloc(
                count,
                sizeof(zEffectAnimRuntimeRef)
            ));
        memcpy(
            clonedEntry->runtimeRefList,
            self->runtimeRefList,
            sizeof(zEffectAnimRuntimeRef) * count
        );

        for (int i = 0; i < count; ++i) {
            clonedEntry->runtimeRefList[i].cachedChildEntry = 0;
        }
    }

    return clonedEntry;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.rebindentrytonode
 * @recoil-artifact defines .text recoil:function:0x45ed80: zEffectAnim::RebindEntryToNode
 * Purpose: Rebind an animation entry to a new root and resolve dependent node references.
 */
zEffectAnimEntry *__fastcall RebindEntryToNode(
    zEffectAnimEntry *self,
    zClass_NodePartial *node
) {
    if (self == 0 || node == 0 || self->activationState == 5) {
        return 0;
    }

    const bool callbackWasBound = self->callbackNode == self->boundNode;
    self->boundNode = node;
    strcpy(
        self->rootNodeName,
        node->name
    );

    if (callbackWasBound) {
        self->callbackNode = node;
        strcpy(
            self->attachNodeName,
            node->name
        );
    } else {
        zClass_NodePartial *const callbackNode =
            FindNodeRecursiveByName(
                self->boundNode,
                self->attachNodeName
            );
        if (callbackNode == 0) {
            zError::ReportOld(
                0x400,
                kZeffAnimInitSourceFile,
                0x27f9,
                kAnimationNodeNotFoundMessage,
                self,
                self->attachNodeName
            );
            self->activationState = 5;
            return 0;
        }
        self->callbackNode = callbackNode;
    }

    for (int i = 0; i < self->trackedNodeCount; ++i) {
        zEffectAnimTrackedNode *const tracked = &self->trackedNodeList[i];
        if (tracked->trackedNode != 0) {
            zClass_NodePartial *const trackedNode =
                ResolveNodeByName(
                    self,
                    tracked->trackedNodeName
                );
            if (trackedNode == 0) {
                zError::ReportOld(
                    0x400,
                    kZeffAnimInitSourceFile,
                    0x2811,
                    kAnimationNodeNotFoundMessage,
                    self,
                    tracked->trackedNodeName
                );
                self->activationState = 5;
                return 0;
            }
            tracked->trackedNode = trackedNode;
        }
    }

    for (int i_1669 = 0; i_1669 < self->nodeRefCount; ++i_1669) {
        zEffectAnimNodeRef28 *const nodeRef = &self->nodeRefList[i_1669];
        if (nodeRef->node != 0) {
            zClass_NodePartial *const resolvedNode = ResolveNodeByName(
                self,
                nodeRef->name.text
            );
            if (resolvedNode == 0) {
                zError::ReportOld(
                    0x400,
                    kZeffAnimInitSourceFile,
                    0x282c,
                    kAnimationNodeNotFoundMessage,
                    self,
                    nodeRef->name.text
                );
                self->activationState = 5;
                return 0;
            }
            nodeRef->node = resolvedNode;
        }
    }

    zClass_NodePartial *prereqSearchRoot = 0;
    for (int i_1682 = 0; i_1682 < self->activationPrereqCount; ++i_1682) {
        zEffectAnimActivationPrereq *const prereq = &self->activationPrereqList[i_1682];
        if (prereq->mode == 2 || prereq->mode == 3) {
            const char *const nodeName = &prereq->targetName[4];
            if (prereqSearchRoot != 0) {
                prereqSearchRoot = zClass_Class::FindSubNodeByName(
                    prereqSearchRoot,
                    nodeName
                );
            } else {
                prereqSearchRoot = ResolveNodeByName(
                    self,
                    nodeName
                );
            }

            if (prereq->mode == 2) {
                prereq->targetNode = prereqSearchRoot;
                prereqSearchRoot = 0;
            }
        }
    }

    return self;
}

} // namespace zEffectAnim

namespace zEffect_Anim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.loadzbd
 * @recoil-artifact defines .text recoil:function:0x45efb0: zEffect_Anim::LoadZbd.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_init.c.
 * Purpose: load animation entries, dynamic lists, event streams, refs, and
 * text ids from the configured animation ZBD.
 */
int LoadZbd() {
    if (g_zEffect_ResourceNode == 0 || g_zEffectAnim_ZbdFilename[0] == '\0') {
        return -1;
    }

    FILE *stream = fopen(
        g_zEffectAnim_ZbdFilename,
        "rb"
    );
    if (stream == 0) {
        return -1;
    }

    int zbdSignature = 0;
    int sourceFileStampRecordSize = 0;
    int sourceFileStampCount = 0;
    if (fread(&zbdSignature, sizeof(zbdSignature), 1, stream) != 1 ||
        fread(
            &sourceFileStampRecordSize,
            sizeof(sourceFileStampRecordSize),
            1,
            stream
        ) != 1 ||
        fread(
            &sourceFileStampCount,
            sizeof(sourceFileStampCount),
            1,
            stream
        ) != 1 ||
        zbdSignature != 0x08170616 ||
        sourceFileStampRecordSize != sizeof(zEffectAnimSourceFileStamp)) {
        fclose(stream);
        return -1;
    }

    g_zEffectAnim_SourceFileStampCount = sourceFileStampCount;
    g_zEffectAnim_SourceFileStampList = 0;
    if (sourceFileStampCount > 0) {
        g_zEffectAnim_SourceFileStampList =
            (zEffectAnimSourceFileStamp *)(malloc(
                sizeof(zEffectAnimSourceFileStamp) *
                (unsigned int)(sourceFileStampCount)
            ));
        if (g_zEffectAnim_SourceFileStampList == 0 ||
            fread(
                g_zEffectAnim_SourceFileStampList,
                sizeof(zEffectAnimSourceFileStamp),
                (unsigned int)(sourceFileStampCount),
                stream
            ) != (unsigned int)(sourceFileStampCount)) {
            fclose(stream);
            return -1;
        }
    }

    bool stampMismatch = false;
    for (int i = 0; i < g_zEffectAnim_SourceFileStampCount; ++i) {
        struct _stat statBuffer;
        zEffectAnimSourceFileStamp *const stamp = &g_zEffectAnim_SourceFileStampList[i];
        if (_stat(stamp->sourcePath, &statBuffer) == 0 &&
            stamp->fileMtime != (int)(statBuffer.st_mtime)) {
            stampMismatch = true;
            break;
        }
    }

    if (g_zEffectAnim_SourceFileStampList != 0) {
        free(g_zEffectAnim_SourceFileStampList);
        g_zEffectAnim_SourceFileStampList = 0;
    }
    g_zEffectAnim_SourceFileStampCount = 0;
    if (stampMismatch) {
        fclose(stream);
        return -1;
    }

    zClass_NodePartial *const savedWorld = g_zEffect_World;
    zEffectAnimZbdHeaderBlock headerBlock = {0};
    if (fread(
            &headerBlock,
            sizeof(headerBlock),
            1,
            stream
        ) != 1) {
        fclose(stream);
        return -1;
    }

    g_zEffectAnim_EntriesInstantiated = headerBlock.entriesInstantiated;
    g_zEffectAnim_HeapPtr = headerBlock.heapPtr;
    g_zEffectAnim_CountsPackedLoWord = headerBlock.countsPackedLoWord;
    g_zEffectAnim_EntryCount = headerBlock.entryCount;
    g_zEffectAnim_EntryList = 0;
    g_zEffectAnim_TextIdEntryCount = headerBlock.textIdEntryCount;
    g_zEffectAnim_TextIdEntryList = 0;
    g_zEffect_World = savedWorld;
    g_zEffect_DefaultGravity = headerBlock.defaultGravity;
    g_zEffect_ConditionalRefPosEnabled = headerBlock.conditionalRefPosEnabled;
    g_zEffect_VariantOverrideEnabled = headerBlock.variantOverrideEnabled;
    g_zEffect_ConditionalRefPosX = headerBlock.conditionalRefPosX;
    g_zEffect_ConditionalRefPosY = headerBlock.conditionalRefPosY;
    g_zEffect_ConditionalRefPosZ = headerBlock.conditionalRefPosZ;
    g_zEffect_VariantOverridePackedIds = headerBlock.variantOverridePackedIds;
    g_zEffect_FrameDeltaRemainingSec = headerBlock.frameDeltaRemainingSec;

    const int entryCount = g_zEffectAnim_EntryCount;
    g_zEffectAnim_EntryList = (zEffectAnimEntry *)(malloc(sizeof(zEffectAnimEntry) * entryCount));
    if (entryCount > 0 && g_zEffectAnim_EntryList == 0) {
        fclose(stream);
        return -1;
    }
    if (entryCount > 0) {
        memset(
            g_zEffectAnim_EntryList,
            0,
            sizeof(zEffectAnimEntry) * entryCount
        );
    }

    for (int i_2059 = 0; i_2059 < entryCount; ++i_2059) {
        zEffectAnimEntry *const entry = &g_zEffectAnim_EntryList[i_2059];
        if (fread(
                entry,
                sizeof(zEffectAnimEntry),
                1,
                stream
            ) != 1) {
            fclose(stream);
            return -1;
        }

        entry->trackedNodeList = 0;
        entry->nodeRefList = 0;
        entry->lightRefList = 0;
        entry->soundRefList = 0;
        entry->sampleRefList = 0;
        entry->effectTemplateRefList = 0;
        entry->activationPrereqList = 0;
        entry->runtimeRefList = 0;
        entry->runtimeList = 0;
        entry->surfacePrimary.eventStream = 0;

        if (entry->trackedNodeCount > 0) {
            entry->trackedNodeList = (zEffectAnimTrackedNode *)(malloc(
                sizeof(zEffectAnimTrackedNode) * entry->trackedNodeCount
            ));
            if (entry->trackedNodeList == 0 ||
                fread(
                    entry->trackedNodeList,
                    sizeof(zEffectAnimTrackedNode),
                    entry->trackedNodeCount,
                    stream
                ) != entry->trackedNodeCount) {
                fclose(stream);
                return -1;
            }
        }
        if (entry->nodeRefCount > 0) {
            entry->nodeRefList = (zEffectAnimNodeRef28 *)(malloc(
                sizeof(zEffectAnimNodeRef28) * entry->nodeRefCount
            ));
            if (entry->nodeRefList == 0 ||
                fread(
                    entry->nodeRefList,
                    sizeof(zEffectAnimNodeRef28),
                    entry->nodeRefCount,
                    stream
                ) != entry->nodeRefCount) {
                fclose(stream);
                return -1;
            }
        }
        if (entry->lightRefCount > 0) {
            entry->lightRefList = (zEffectAnimRuntimeNodeRef *)(malloc(
                sizeof(zEffectAnimRuntimeNodeRef) * entry->lightRefCount
            ));
            if (entry->lightRefList == 0 ||
                fread(
                    entry->lightRefList,
                    sizeof(zEffectAnimRuntimeNodeRef),
                    entry->lightRefCount,
                    stream
                ) != entry->lightRefCount) {
                fclose(stream);
                return -1;
            }
        }
        if (entry->soundRefCount > 0) {
            entry->soundRefList = (zEffectAnimRuntimeNodeRef *)(malloc(
                sizeof(zEffectAnimRuntimeNodeRef) * entry->soundRefCount
            ));
            if (entry->soundRefList == 0 ||
                fread(
                    entry->soundRefList,
                    sizeof(zEffectAnimRuntimeNodeRef),
                    entry->soundRefCount,
                    stream
                ) != entry->soundRefCount) {
                fclose(stream);
                return -1;
            }
        }
        if (entry->sampleRefCount > 0) {
            entry->sampleRefList = (zEffectAnimSampleRef *)(malloc(
                sizeof(zEffectAnimSampleRef) * entry->sampleRefCount
            ));
            if (entry->sampleRefList == 0 ||
                fread(
                    entry->sampleRefList,
                    sizeof(zEffectAnimSampleRef),
                    entry->sampleRefCount,
                    stream
                ) != entry->sampleRefCount) {
                fclose(stream);
                return -1;
            }
        }
        if (entry->effectTemplateRefCount > 0) {
            entry->effectTemplateRefList = (zEffectAnimTemplateIndexRef *)(malloc(
                sizeof(zEffectAnimTemplateIndexRef) * entry->effectTemplateRefCount
            ));
            if (entry->effectTemplateRefList == 0 ||
                fread(
                    entry->effectTemplateRefList,
                    sizeof(zEffectAnimTemplateIndexRef),
                    entry->effectTemplateRefCount,
                    stream
                ) != entry->effectTemplateRefCount) {
                fclose(stream);
                return -1;
            }
        }
        if (entry->activationPrereqCount > 0) {
            entry->activationPrereqList = (zEffectAnimActivationPrereq *)(malloc(
                sizeof(zEffectAnimActivationPrereq) * entry->activationPrereqCount
            ));
            if (entry->activationPrereqList == 0 ||
                fread(
                    entry->activationPrereqList,
                    sizeof(zEffectAnimActivationPrereq),
                    entry->activationPrereqCount,
                    stream
                ) != entry->activationPrereqCount) {
                fclose(stream);
                return -1;
            }
        }
        if (entry->runtimeRefCount > 0) {
            entry->runtimeRefList = (zEffectAnimRuntimeRef *)(malloc(
                sizeof(zEffectAnimRuntimeRef) * entry->runtimeRefCount
            ));
            if (entry->runtimeRefList == 0 ||
                fread(
                    entry->runtimeRefList,
                    sizeof(zEffectAnimRuntimeRef),
                    entry->runtimeRefCount,
                    stream
                ) != entry->runtimeRefCount) {
                fclose(stream);
                return -1;
            }
        }

        for (int j = 0; j < entry->runtimeRefCount; ++j) {
            entry->runtimeRefList[j].cachedChildEntry = 0;
        }

        if (entry->runtimeSequenceCount > 0) {
            entry->runtimeList = (zEffectAnimSurfaceRuntime *)(malloc(
                sizeof(zEffectAnimSurfaceRuntime) * entry->runtimeSequenceCount
            ));
            if (entry->runtimeList == 0) {
                fclose(stream);
                return -1;
            }
        }

        if (fread(
                &entry->surfacePrimary,
                sizeof(entry->surfacePrimary),
                1,
                stream
            ) != 1) {
            fclose(stream);
            return -1;
        }
        entry->surfacePrimary.eventStream = 0;
        if (entry->surfacePrimary.eventStreamSize > 0) {
            entry->surfacePrimary.eventStream =
                malloc(entry->surfacePrimary.eventStreamSize);
            if (entry->surfacePrimary.eventStream == 0 ||
                fread(
                    entry->surfacePrimary.eventStream,
                    entry->surfacePrimary.eventStreamSize,
                    1,
                    stream
                ) != 1) {
                fclose(stream);
                return -1;
            }
        }

        for (int j_2108 = 0; j_2108 < entry->runtimeSequenceCount; ++j_2108) {
            zEffectAnimSurfaceRuntime *const runtime = &entry->runtimeList[j_2108];
            if (fread(
                    runtime,
                    sizeof(zEffectAnimSurfaceRuntime),
                    1,
                    stream
                ) != 1) {
                fclose(stream);
                return -1;
            }
            runtime->eventStream = 0;
            if (runtime->eventStreamSize > 0) {
                runtime->eventStream = malloc(runtime->eventStreamSize);
                if (runtime->eventStream == 0 ||
                    fread(
                        runtime->eventStream,
                        runtime->eventStreamSize,
                        1,
                        stream
                    ) != 1) {
                    fclose(stream);
                    return -1;
                }
            }
        }
    }

    const char *previousRootNodeName = 0;
    for (int i_2119 = 0; i_2119 < entryCount; ++i_2119) {
        zEffectAnimEntry *const entry = &g_zEffectAnim_EntryList[i_2119];
        const unsigned char savedLightRefCount = entry->lightRefCount;
        const unsigned char savedSoundRefCount = entry->soundRefCount;
        entry->lightRefCount = 0;
        entry->soundRefCount = 0;

        if (i_2119 != 0 && entry->activationState != 5) {
            entry->boundNode = 0;
            if (previousRootNodeName != 0 &&
                strcmp(
                    previousRootNodeName,
                    entry->rootNodeName
                ) == 0) {
                entry->boundNode = zClass_Class::gwNodeFindNextByName(
                    0,
                    0
                );
            }

            if (entry->boundNode == 0) {
                zClass_Class::gwNodeFindNextByName(
                    entry->rootNodeName,
                    6
                );
                entry->boundNode = zClass_Class::gwNodeFindNextByName(
                    0,
                    0
                );
                previousRootNodeName = entry->rootNodeName;
            }

            if (entry->boundNode == 0) {
                fclose(stream);
                return -1;
            }

            zClass_NodePartial *const rootNode = zClass_Class::gwNodeGetRoot(entry->boundNode);
            if (g_zEffectAnim_ForceCloneNonDynamicRoot != 0 && rootNode != 0 &&
                rootNode->classId != 2 && rootNode->classId != 1) {
                entry->boundNode = zClass_cls_util::CopyNode(
                    entry->boundNode,
                    g_zEffectAnim_CopyNodeMode,
                    g_zEffectAnim_CopyNodeArg1,
                    g_zEffectAnim_CopyNodeArg2
                );
                if (entry->boundNode == 0) {
                    fclose(stream);
                    return -1;
                }
            }

            entry->callbackNode = zEffectAnim::ResolveNodeByName(
                entry,
                entry->attachNodeName
            );
            if (entry->callbackNode == 0) {
                fclose(stream);
                return -1;
            }
        }

        entry->lightRefCount = savedLightRefCount;
        for (int j = 1; j < entry->lightRefCount; ++j) {
            zEffectAnimRuntimeNodeRef *const lightRef = &entry->lightRefList[j];
            lightRef->runtimeNode = zClass_Light::gwLightNew();
            if (lightRef->runtimeNode == 0) {
                fclose(stream);
                return -1;
            }
            zClass_Class::gwNodeSetName(
                lightRef->runtimeNode,
                lightRef->name.text
            );
        }

        entry->soundRefCount = savedSoundRefCount;
        for (int j_2175 = 1; j_2175 < entry->soundRefCount; ++j_2175) {
            zEffectAnimRuntimeNodeRef *const soundRef = &entry->soundRefList[j_2175];
            soundRef->runtimeNode = zClass_Sound::gwSoundNew();
            if (soundRef->runtimeNode == 0) {
                fclose(stream);
                return -1;
            }
            zClass_Class::gwNodeSetName(
                soundRef->runtimeNode,
                soundRef->name.text
            );
            zClass_Sound::SetSampleSetByName(
                soundRef->runtimeNode,
                soundRef->name.text
            );
        }

        for (int j_2186 = 1; j_2186 < entry->trackedNodeCount; ++j_2186) {
            zEffectAnimTrackedNode *const tracked = &entry->trackedNodeList[j_2186];
            tracked->trackedNode = zEffectAnim::ResolveNodeByName(
                entry,
                tracked->trackedNodeName
            );
            if (tracked->trackedNode == 0) {
                fclose(stream);
                return -1;
            }
        }

        for (int j_2195 = 1; j_2195 < entry->nodeRefCount; ++j_2195) {
            zEffectAnimNodeRef28 *const nodeRef = &entry->nodeRefList[j_2195];
            nodeRef->node = zEffectAnim::ResolveNodeByName(
                entry,
                nodeRef->name.text
            );
            if (nodeRef->node == 0) {
                fclose(stream);
                return -1;
            }
        }

        for (int j_2204 = 1; j_2204 < entry->sampleRefCount; ++j_2204) {
            entry->sampleRefList[j_2204].sample =
                zSnd::FindSampleByName(entry->sampleRefList[j_2204].name);
        }

        for (int j_2208 = 1; j_2208 < entry->effectTemplateRefCount; ++j_2208) {
            zEffectAnimTemplateIndexRef *const templateRef = &entry->effectTemplateRefList[j_2208];
            templateRef->templateIndex = zEffect::FindTemplateIndexByName(templateRef->name);
            if (templateRef->templateIndex == -1) {
                fclose(stream);
                return -1;
            }
        }

        zClass_NodePartial *prereqSearchRoot = 0;
        zClass_NodePartial *cachedPrereqSearchRoot = 0;
        for (int j_2219 = 0; j_2219 < entry->activationPrereqCount; ++j_2219) {
            zEffectAnimActivationPrereq *const prereq = &entry->activationPrereqList[j_2219];
            if (prereq->mode == 1) {
                prereq->targetEntry = zEffectAnim::FindEntryByName(prereq->targetName);
                prereqSearchRoot = cachedPrereqSearchRoot;
            } else if (prereq->mode == 2 || prereq->mode == 3) {
                const char *const nodeName = &prereq->targetName[4];
                if (prereqSearchRoot != 0) {
                    prereqSearchRoot = zClass_Class::FindSubNodeByName(
                        prereqSearchRoot,
                        nodeName
                    );
                } else {
                    prereqSearchRoot = zClass::FindByTypeAndName(
                        6,
                        nodeName
                    );
                }

                cachedPrereqSearchRoot = prereqSearchRoot;
                if (prereqSearchRoot == 0) {
                    zError::ReportOld(
                        0x400,
                        kZeffAnimInitSourceFile,
                        0x2c17,
                        g_zEffectAnim_ActivationPrereqNodeNotFoundFmt,
                        entry,
                        nodeName
                    );
                    entry->activationPrereqCount = 0;
                    break;
                }

                prereq->targetNode = prereqSearchRoot;
                if (prereq->mode == 2) {
                    prereqSearchRoot = 0;
                    cachedPrereqSearchRoot = 0;
                }
            }
        }
    }

    if (g_zEffectAnim_TextIdEntryCount > 0) {
        g_zEffectAnim_TextIdEntryList = (zEffectAnimTextIdEntry *)(malloc(
            sizeof(zEffectAnimTextIdEntry) *
            (unsigned int)(g_zEffectAnim_TextIdEntryCount)
        ));
        if (g_zEffectAnim_TextIdEntryList == 0 ||
            fread(
                g_zEffectAnim_TextIdEntryList,
                sizeof(zEffectAnimTextIdEntry),
                (unsigned int)(g_zEffectAnim_TextIdEntryCount),
                stream
            ) != (unsigned int)(g_zEffectAnim_TextIdEntryCount)) {
            fclose(stream);
            return -1;
        }

        for (int i = 0; i < g_zEffectAnim_TextIdEntryCount; ++i) {
            g_zEffectAnim_TextIdEntryList[i].messageId =
                zLoc::GetMessageId(g_zEffectAnim_TextIdEntryList[i].messageKey);
        }
    }

    fclose(stream);
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.loadandinstantiate
 * @recoil-artifact defines .text recoil:function:0x45fb30: zEffect_Anim::LoadAndInstantiate.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_init.c.
 * Purpose: ensure animation entries are loaded, bind runtime roots, install
 * callbacks, capture initial node state, and mark entries instantiated.
 */
int LoadAndInstantiate() {
    if (g_zEffectAnim_EntriesInstantiated != 0) {
        return 0;
    }

    LoadZbd();

    for (int i = 1; i < g_zEffectAnim_EntryCount; ++i) {
        zEffectAnimEntry *const entry = &g_zEffectAnim_EntryList[i];
        if (entry->activationState != 5 && entry->activationState != 4 &&
            zClass::AnyNodeMatchesPredicateRecursive(
                entry->boundNode,
                zClass_Node::HasRenderableDiPredicate
            ) != 0) {
            entry->flags |= 0x200;
        }
    }

    for (int i_2286 = 1; i_2286 < g_zEffectAnim_EntryCount; ++i_2286) {
        zEffectAnimEntry *const entry = &g_zEffectAnim_EntryList[i_2286];
        if (entry->activationState == 5) {
            continue;
        }

        if (entry->boundNode == 0) {
            zClass_NodePartial *const objectNode = zClass_Object3D::gwObject3DInit();
            entry->boundNode = objectNode;
            entry->callbackNode = objectNode;
        }

        if ((entry->flags & kEffectAnimNeedsCopiedRootFlag) != 0) {
            zEffectAnim::EnsureCopiedRootTree(
                entry,
                entry->boundNode
            );
        }

        entry->runtimeNode = zClass_Object3D::gwObject3DInit();
        char runtimeNodeName[0x24];
        sprintf(
            runtimeNodeName,
            "_%s",
            entry->name
        );
        zClass_Class::gwNodeSetName(
            entry->runtimeNode,
            runtimeNodeName
        );
        zClass_Class::gwNodeSetPriority(
            entry->runtimeNode,
            entry->priority
        );

        CaptureNodeStates(entry);
        if (entry->activationMode == 1) {
            zClass_Node::SetDamageTimerCallback(
                entry,
                entry->callbackNode,
                (void *)(zEffect::TickResetDelayOnTimer)
            );
        }
        if (entry->activationMode == 0) {
            zClass_Node::SetDamageHitCallback(
                entry,
                entry->callbackNode,
                (void *)(zEffect::TickResetDelayOnHit)
            );
        }
        if (entry->activationMode == 2) {
            zClass_Node::SetDamageTimerCallback(
                entry,
                entry->callbackNode,
                (void *)(zEffect::TickResetDelayOnTimer)
            );
            zClass_Node::SetDamageHitCallback(
                entry,
                entry->callbackNode,
                (void *)(zEffect::TickResetDelayOnHit)
            );
        }

        if (entry->activationState != 2) {
            if ((entry->flags & 0x20) != 0) {
                zEffectAnim::StopAndCleanup(
                    entry,
                    0,
                    0
                );
            }
            if (entry->activationMode == 4) {
                zEffectAnim::SetVelocity(
                    entry,
                    0,
                    0.0f,
                    0.0f,
                    0.0f
                );
            }
        } else {
            zError::ReportOld(
                0x400,
                kZeffAnimInitSourceFile,
                0x2d55,
                g_zEffectAnim_CorruptAnimationLoadedFmt,
                entry
            );
        }
    }

    g_zEffectAnim_EntriesInstantiated = 1;
    return 0;
}

} // namespace zEffect_Anim

namespace zEffectAnim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.shutdownentry
 * @recoil-artifact defines .text recoil:function:0x45fd10: zEffectAnim::ShutdownEntry.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_init.c.
 * Purpose: release runtime nodes, event streams, dynamic entry lists, and
 * cloned siblings owned by one animation entry.
 */
int __fastcall ShutdownEntry(
    zEffectAnimEntry *self
) {
    if (self->activationState == 5) {
        return 0;
    }

    if (self->runtimeNode != 0) {
        zClass_Object3D::DeleteNode(self->runtimeNode);
    }

    const unsigned char activationMode = self->activationMode;
    if (activationMode == 1 || activationMode == 0 || activationMode == 2) {
        zClass_Node::ClearDamageHandler(self->callbackNode);
    }

    if (self->surfacePrimary.eventStream != 0) {
        free(self->surfacePrimary.eventStream);
    }
    self->surfacePrimary.eventStream = 0;
    self->surfacePrimary.eventStreamSize = 0;

    for (int i = 0; i < self->runtimeSequenceCount; ++i) {
        if (self->runtimeList[i].eventStream != 0) {
            free(self->runtimeList[i].eventStream);
        }
    }

    if (self->runtimeList != 0) {
        free(self->runtimeList);
    }
    if (self->trackedNodeList != 0) {
        free(self->trackedNodeList);
    }
    if (self->nodeRefList != 0) {
        free(self->nodeRefList);
    }
    if (self->lightRefList != 0) {
        free(self->lightRefList);
    }
    if (self->soundRefList != 0) {
        free(self->soundRefList);
    }
    if (self->sampleRefList != 0) {
        free(self->sampleRefList);
    }
    if (self->effectTemplateRefList != 0) {
        free(self->effectTemplateRefList);
    }
    if (self->activationPrereqList != 0) {
        free(self->activationPrereqList);
    }
    if (self->runtimeRefList != 0) {
        free(self->runtimeRefList);
    }

    if (self->runtimeSibling != 0) {
        ShutdownEntry(self->runtimeSibling);
        free(self->runtimeSibling);
    }

    return 0;
}

} // namespace zEffectAnim

namespace zEffect_Anim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.shutdown
 * @recoil-artifact defines .text recoil:function:0x45fe50: zEffect_Anim::Shutdown.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_init.c.
 * Purpose: free loaded animation heap, entries, localized text records, and
 * queued activation records, then clear animation-load state.
 */
int Shutdown() {
    if (g_zEffectAnim_HeapPtr != 0) {
        free(g_zEffectAnim_HeapPtr);
        g_zEffectAnim_HeapPtr = 0;
    }

    g_zEffectAnim_CountsPackedLoWord = 0;
    for (int i = 0; i < g_zEffectAnim_EntryCount; ++i) {
        zEffectAnim::ShutdownEntry(&g_zEffectAnim_EntryList[i]);
    }

    if (g_zEffectAnim_EntryList != 0) {
        free(g_zEffectAnim_EntryList);
        g_zEffectAnim_EntryList = 0;
    }

    g_zEffectAnim_EntryCount = 0;
    if (g_zEffectAnim_TextIdEntryList != 0) {
        free(g_zEffectAnim_TextIdEntryList);
        g_zEffectAnim_TextIdEntryList = 0;
    }

    g_zEffectAnim_TextIdEntryCount = 0;
    ClearActivationRecords();
    g_zEffectAnim_EntriesInstantiated = 0;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.shutdownifloaded
 * @recoil-artifact defines .text recoil:function:0x45fef0: zEffect_Anim::ShutdownIfLoaded.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_init.c.
 * Purpose: run animation shutdown only when entries are currently
 * instantiated.
 */
int ShutdownIfLoaded() {
    if (g_zEffectAnim_EntriesInstantiated != 0) {
        Shutdown();
    }

    return 0;
}

} // namespace zEffect_Anim

namespace zEffectAnim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.findentrybyname
 * @recoil-artifact defines .text recoil:function:0x45ff10: zEffectAnim::FindEntryByName (zeff_anim.c)
 * Purpose: Return the first zEffect animation entry whose table name matches the requested name.
 */
zEffectAnimEntry *__fastcall FindEntryByName(
    const char *name
) {
    if (name == 0) {
        return 0;
    }

    const int count = g_zEffectAnim_EntryCount;
    for (int i = 0; i < count; ++i) {
        if (strcmp(
            g_zEffectAnim_EntryList[i].name,
            name
        ) == 0) {
            return &g_zEffectAnim_EntryList[i];
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.findnextasyncentry
 * @recoil-artifact defines .text recoil:function:0x45ffa0: zEffectAnim::FindNextAsyncEntry.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim.c.
 * Purpose: return the next animation entry with the async flag set after an
 * optional current entry.
 */
zEffectAnimEntry *__fastcall FindNextAsyncEntry(
    zEffectAnimEntry *currentEntry
) {
    int index = 0;
    if (currentEntry != 0) {
        index = (int)(currentEntry - g_zEffectAnim_EntryList) + 1;
    }

    for (; index < g_zEffectAnim_EntryCount; ++index) {
        if ((g_zEffectAnim_EntryList[index].flags & 0x10) != 0) {
            return &g_zEffectAnim_EntryList[index];
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.getrootnodeornull
 * @recoil-artifact defines .text recoil:function:0x460010: zEffectAnim::GetRootNodeOrNull (zeff_anim.c)
 * Purpose: Return an animation entry's bound root node, or null for a missing entry.
 */
zClass_NodePartial *__fastcall GetRootNodeOrNull(
    zEffectAnimEntry *self
) {
    if (self == 0) {
        return 0;
    }

    return self->boundNode;
}
} // namespace zEffectAnim
