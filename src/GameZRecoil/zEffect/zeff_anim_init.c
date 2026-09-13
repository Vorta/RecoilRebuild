#include "GameZRecoil/zEffect/zeff.h"

#include "GameZRecoil/zTime/time.h"
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

const char kAnimationNodeNotFoundMessage[] =
    "Animation node not found.\n  Animation: %s; Node: %s\n";



struct zEffectAnimZbdFilePrefix {
    int signature;
    int formatMarker;
    int sourceFileStampCount;
};

RECOIL_STATIC_ASSERT(sizeof(zEffectAnimZbdFilePrefix) == 0x0c);
} // namespace

namespace zEffect_Anim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.init
 * @recoil-artifact defines .text recoil:function:0x45e100: zEffect_Anim::Init.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_init.c.
 * Purpose: reset animation globals, seed runtime random values, and register
 * animation save/load ZAR section handlers.
 */
int __cdecl Init() {
    if (g_zEffectAnim_State.entriesInstantiated != 0) {
        Shutdown();
    }

    g_zEffectAnim_ZbdFilename[0] = '\0';
    g_zEffectAnim_State.entriesInstantiated = 0;
    g_zEffectAnim_State.heapPtr = 0;
    g_zEffectAnim_State.countsPackedLoWord = 0;
    g_zEffectAnim_State.entryCount = 0;
    g_zEffectAnim_State.entryList = 0;
    g_zEffectAnim_State.textIdEntryCount = 0;
    g_zEffectAnim_State.textIdEntryList = 0;
    g_zEffectAnim_State.worldNode = 0;
    g_zEffectAnim_State.conditionalRefPosEnabled = 0;
    g_zEffectAnim_State.variantOverrideEnabled = 0;
    g_zEffectAnim_State.defaultGravity = -9.8f;

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
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff.c.
 * Purpose: store the world node used by zEffect runtime handlers.
 */
void __fastcall SetWorldNode(
    zClass_NodePartial *worldNode
) {
    g_zEffectAnim_State.worldNode = worldNode;
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
            "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_init.c",
            0xd1,
            "Animation ZBD filename too long: %s\n",
            filename
        );
        return;
    }

    strcpy(g_zEffectAnim_ZbdFilename, filename);
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
        if (node != 0 && strcmp(node->name, name) == 0) {
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
        if (node != 0 && strcmp(node->name, name) == 0) {
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
    const int existingIndex = FindSoundRefIndexByName(self, name);
    if (existingIndex > 0) {
        return existingIndex;
    }

    zClass_NodePartial *const node = zClass_Sound::gwSoundNew();
    if (node == 0) {
        return -1;
    }

    zClass_Class::gwNodeSetName(node, name);
    zClass_Sound::SetSampleSetByName(node, name);

    if (self->soundRefList == 0) {
        const int initialCount = (int)(self->soundRefCount) + 1;
        self->soundRefList = (zEffectAnimRuntimeNodeRef *)(realloc(
            0,
            initialCount * sizeof(zEffectAnimRuntimeNodeRef)
        ));
        memset(self->soundRefList, 0, sizeof(zEffectAnimRuntimeNodeRef));
        ++self->soundRefCount;
    }

    if (self->soundRefCount == 0xff) {
        zError::ReportOld(
            0x400,
            "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_init.c",
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
    memcpy(newRef->name.text, node->name, sizeof(newRef->name.text));
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
    const int existingIndex = FindLightRefIndexByName(self, name);
    if (existingIndex > 0) {
        return existingIndex;
    }

    zClass_NodePartial *const node = zClass_Light::gwLightNew();
    if (node == 0) {
        return -1;
    }

    zClass_Class::gwNodeSetName(node, name);

    if (self->lightRefList == 0) {
        const int initialCount = (int)(self->lightRefCount) + 1;
        self->lightRefList = (zEffectAnimRuntimeNodeRef *)(realloc(
            0,
            initialCount * sizeof(zEffectAnimRuntimeNodeRef)
        ));
        memset(self->lightRefList, 0, sizeof(zEffectAnimRuntimeNodeRef));
        ++self->lightRefCount;
    }

    if (self->lightRefCount == 0xff) {
        zError::ReportOld(
            0x400,
            "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_init.c",
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
    memcpy(newRef->name.text, node->name, sizeof(newRef->name.text));
    newRef->runtimeNode = node;
    newRef->isAttached = 0;
    ++self->lightRefCount;
    return (int)(self->lightRefCount) - 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.resolvenodebyname
 * @recoil-artifact defines .text recoil:function:0x45e5c0: zEffectAnim::ResolveNodeByName (zeff_anim.c)
 * @recoil-match byte
 *
 * Purpose: Resolve an animation node name through callback, bound, runtime-ref, then zClass lookup paths.
 */
zClass_NodePartial *__fastcall ResolveNodeByName(
    zEffectAnimEntry *self,
    const char *name
) {
    zClass_NodePartial *resolvedNode = FindNodeRecursiveByName(self->callbackNode, name);
    if (resolvedNode != 0) {
        return resolvedNode;
    }

    resolvedNode = FindNodeRecursiveByName(self->boundNode, name);
    if (resolvedNode != 0) {
        return resolvedNode;
    }

    const int lightRefIndex = FindLightRefIndexByName(self, name);
    if (lightRefIndex > 0) {
        resolvedNode = self->lightRefList[lightRefIndex].runtimeNode;
    }
    if (resolvedNode != 0) {
        return resolvedNode;
    }

    const int soundRefIndex = FindSoundRefIndexByName(self, name);
    if (soundRefIndex > 0) {
        resolvedNode = self->soundRefList[soundRefIndex].runtimeNode;
    }
    if (resolvedNode != 0) {
        return resolvedNode;
    }

    return zClass::FindByTypeAndName(6, name);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.findnoderecursivebyname
 * @recoil-artifact defines .text recoil:function:0x45e650: zEffectAnim::FindNodeRecursiveByName (zeff_anim.c)
 * @recoil-match byte
 *
 * Purpose: Return the first node in the root-first child traversal whose name matches.
 */
zClass_NodePartial *__fastcall FindNodeRecursiveByName(
    zClass_NodePartial *rootNode,
    const char *name
) {
    if (rootNode == 0) {
        return 0;
    }

    if (strcmp(rootNode->name, name) == 0) {
        return rootNode;
    }

    for (int i = 0; i < rootNode->listCountB; ++i) {
        zClass_NodePartial *const childMatch = FindNodeRecursiveByName(rootNode->listB[i], name);
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

        RebindEntryToNode(self, copiedRoot);
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

    zEffectAnimEntry *const clonedEntry = (zEffectAnimEntry *)(calloc(1, sizeof(zEffectAnimEntry)));
    memcpy(clonedEntry, self, sizeof(zEffectAnimEntry));

    clonedEntry->runtimeNode = zClass_Object3D::gwObject3DInit();

    char runtimeNodeName[0x24];
    sprintf(runtimeNodeName, "_%s", self->name);
    zClass_Class::gwNodeSetName(clonedEntry->runtimeNode, runtimeNodeName);
    zClass_Class::gwNodeSetPriority(clonedEntry->runtimeNode, clonedEntry->priority);

    clonedEntry->runtimeSibling = 0;
    clonedEntry->flags &= ~0x00000100u;

    if (node != 0) {
        clonedEntry->boundNode = node;
        strcpy(clonedEntry->rootNodeName, node->name);
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
                "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_init.c",
                0x26d1,
                "ERROR:\n  Copying Animation Node Tree: %s\n",
                clonedEntry
            );
            return 0;
        }
    }

    if (self->callbackNode == self->boundNode) {
        clonedEntry->callbackNode = clonedEntry->boundNode;
        strcpy(clonedEntry->attachNodeName, clonedEntry->boundNode->name);
    } else {
        clonedEntry->callbackNode =
            FindNodeRecursiveByName(clonedEntry->boundNode, self->attachNodeName);
    }

    if (clonedEntry->callbackNode == 0) {
        zError::ReportOld(
            0x400,
            "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_init.c",
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
            (zEffectAnimRuntimeNodeRef *)(calloc(count, sizeof(zEffectAnimRuntimeNodeRef)));
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
                zClass_Class::gwNodeSetName(lightNode, lightRef->name.text);
            }
            lightRef->isAttached = 0;
        }
    }

    if (clonedEntry->soundRefCount > 0) {
        const int count = clonedEntry->soundRefCount;
        clonedEntry->soundRefList =
            (zEffectAnimRuntimeNodeRef *)(calloc(count, sizeof(zEffectAnimRuntimeNodeRef)));
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
                zClass_Class::gwNodeSetName(soundNode, soundRef->name.text);
                zClass_Sound::SetSampleSetByName(soundNode, soundRef->name.text);
            }
            soundRef->isAttached = 0;
        }
    }

    if (clonedEntry->trackedNodeCount > 0) {
        const int count = clonedEntry->trackedNodeCount;
        clonedEntry->trackedNodeList =
            (zEffectAnimTrackedNode *)(calloc(count, sizeof(zEffectAnimTrackedNode)));
        memcpy(
            clonedEntry->trackedNodeList,
            self->trackedNodeList,
            sizeof(zEffectAnimTrackedNode) * count
        );

        for (int i = 0; i < count; ++i) {
            if (self->trackedNodeList[i].trackedNode != 0) {
                clonedEntry->trackedNodeList[i].trackedNode =
                    ResolveNodeByName(clonedEntry, clonedEntry->trackedNodeList[i].trackedNodeName);
            }
        }
    }

    if (clonedEntry->nodeRefCount > 0) {
        const int count = clonedEntry->nodeRefCount;
        clonedEntry->nodeRefList =
            (zEffectAnimNodeRef28 *)(calloc(count, sizeof(zEffectAnimNodeRef28)));
        memcpy(clonedEntry->nodeRefList, self->nodeRefList, sizeof(zEffectAnimNodeRef28) * count);

        for (int i = 0; i < count; ++i) {
            if (self->nodeRefList[i].node != 0) {
                clonedEntry->nodeRefList[i].node =
                    ResolveNodeByName(clonedEntry, clonedEntry->nodeRefList[i].name.text);
            }
        }
    }

    if (clonedEntry->runtimeSequenceCount > 0) {
        const int count = clonedEntry->runtimeSequenceCount;
        clonedEntry->runtimeList =
            (zEffectAnimSurfaceRuntime *)(calloc(count, sizeof(zEffectAnimSurfaceRuntime)));

        for (int i = 0; i < count; ++i) {
            memcpy(
                &clonedEntry->runtimeList[i],
                &self->runtimeList[i],
                sizeof(zEffectAnimSurfaceRuntime)
            );

            const int eventStreamSize = self->runtimeList[i].eventStreamSize;
            if (eventStreamSize > 0) {
                clonedEntry->runtimeList[i].eventStream = calloc(1, eventStreamSize);
                memcpy(
                    clonedEntry->runtimeList[i].eventStream,
                    self->runtimeList[i].eventStream,
                    eventStreamSize
                );
            }
        }
    }

    if (self->surfacePrimary.eventStreamSize > 0) {
        clonedEntry->surfacePrimary.eventStream = calloc(1, self->surfacePrimary.eventStreamSize);
        memcpy(
            clonedEntry->surfacePrimary.eventStream,
            self->surfacePrimary.eventStream,
            self->surfacePrimary.eventStreamSize
        );
    }

    if (clonedEntry->sampleRefCount > 0) {
        const int count = clonedEntry->sampleRefCount;
        clonedEntry->sampleRefList =
            (zEffectAnimSampleRef *)(calloc(count, sizeof(zEffectAnimSampleRef)));
        memcpy(
            clonedEntry->sampleRefList,
            self->sampleRefList,
            sizeof(zEffectAnimSampleRef) * count
        );
    }

    if (clonedEntry->effectTemplateRefCount > 0) {
        const int count = clonedEntry->effectTemplateRefCount;
        clonedEntry->effectTemplateRefList =
            (zEffectAnimTemplateIndexRef *)(calloc(count, sizeof(zEffectAnimTemplateIndexRef)));
        memcpy(
            clonedEntry->effectTemplateRefList,
            self->effectTemplateRefList,
            sizeof(zEffectAnimTemplateIndexRef) * count
        );
    }

    if (clonedEntry->activationPrereqCount > 0) {
        const int count = clonedEntry->activationPrereqCount;
        clonedEntry->activationPrereqList =
            (zEffectAnimActivationPrereq *)(calloc(count, sizeof(zEffectAnimActivationPrereq)));
        memcpy(
            clonedEntry->activationPrereqList,
            self->activationPrereqList,
            sizeof(zEffectAnimActivationPrereq) * count
        );
    }

    if (clonedEntry->runtimeRefCount > 0) {
        const int count = clonedEntry->runtimeRefCount;
        clonedEntry->runtimeRefList =
            (zEffectAnimRuntimeRef *)(calloc(count, sizeof(zEffectAnimRuntimeRef)));
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

    const int callbackNeedsLookup = self->callbackNode != self->boundNode;
    self->boundNode = node;

    if (callbackNeedsLookup) {
        strcpy(self->rootNodeName, node->name);
        self->callbackNode = FindNodeRecursiveByName(self->boundNode, self->attachNodeName);
        if (self->callbackNode == 0) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_init.c",
                0x27f9,
                kAnimationNodeNotFoundMessage,
                self,
                self->attachNodeName
            );
            self->activationState = 5;
            return 0;
        }
    } else {
        strcpy(self->rootNodeName, node->name);
        self->callbackNode = node;
        strcpy(self->attachNodeName, node->name);
    }

    for (int i = 0; i < self->trackedNodeCount; ++i) {
        zEffectAnimTrackedNode *const tracked = &self->trackedNodeList[i];
        if (tracked->trackedNode != 0) {
            tracked->trackedNode = ResolveNodeByName(self, tracked->trackedNodeName);
            if (tracked->trackedNode == 0) {
                zError::ReportOld(
                    0x400,
                    "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_init.c",
                    0x2811,
                    kAnimationNodeNotFoundMessage,
                    self,
                    tracked->trackedNodeName
                );
                self->activationState = 5;
                return 0;
            }
        }
    }

    for (int i_1669 = 0; i_1669 < self->nodeRefCount; ++i_1669) {
        zEffectAnimNodeRef28 *const nodeRef = &self->nodeRefList[i_1669];
        if (nodeRef->node != 0) {
            nodeRef->node = ResolveNodeByName(self, nodeRef->name.text);
            if (nodeRef->node == 0) {
                zError::ReportOld(
                    0x400,
                    "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_init.c",
                    0x282c,
                    kAnimationNodeNotFoundMessage,
                    self,
                    nodeRef->name.text
                );
                self->activationState = 5;
                return 0;
            }
        }
    }

    zClass_NodePartial *prereqSearchRoot = 0;
    for (int i_1682 = 0; i_1682 < self->activationPrereqCount; ++i_1682) {
        zEffectAnimActivationPrereq *const prereq = &self->activationPrereqList[i_1682];
        if (prereq->mode == 2 || prereq->mode == 3) {
            const char *const nodeName = &prereq->targetName[4];
            prereqSearchRoot = prereqSearchRoot == 0
                ? ResolveNodeByName(self, nodeName)
                : zClass_Class::FindSubNodeByName(prereqSearchRoot, nodeName);

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
int __cdecl LoadZbd() {
    int i;
    if (g_zEffect_ResourceNode == 0) {
        return -1;
    }
    if (strlen(g_zEffectAnim_ZbdFilename) == 0) {
        return -1;
    }

    FILE *stream = fopen(g_zEffectAnim_ZbdFilename, "rb");
    if (stream == 0) {
        return -1;
    }

    zEffectAnimZbdFilePrefix filePrefix;
    if (fread(&filePrefix, sizeof(filePrefix), 1, stream) != 1) {
        fclose(stream);
        return -1;
    }
    if (filePrefix.signature != 0x08170616) {
        fclose(stream);
        return -1;
    }
    if (filePrefix.formatMarker != 0x1c) {
        fclose(stream);
        return -1;
    }

    g_zEffectAnim_SourceFileStampCount = filePrefix.sourceFileStampCount;
    g_zEffectAnim_SourceFileStampList = (zEffectAnimSourceFileStamp *)malloc(
        sizeof(zEffectAnimSourceFileStamp) * g_zEffectAnim_SourceFileStampCount);
    bool stampMismatch = false;
    if (fread(g_zEffectAnim_SourceFileStampList, sizeof(zEffectAnimSourceFileStamp),
            g_zEffectAnim_SourceFileStampCount, stream) !=
            (unsigned int)g_zEffectAnim_SourceFileStampCount) {
        stampMismatch = true;
    }
    if (!stampMismatch) {
        for (i = 0; i < g_zEffectAnim_SourceFileStampCount; ++i) {
            struct _stat statBuffer;
            if (_stat(g_zEffectAnim_SourceFileStampList[i].sourcePath, &statBuffer) == 0 &&
                g_zEffectAnim_SourceFileStampList[i].fileMtime != statBuffer.st_mtime) {
                stampMismatch = true;
                break;
            }
        }
    }

    if (g_zEffectAnim_SourceFileStampList != 0) {
        free(g_zEffectAnim_SourceFileStampList);
    }
    g_zEffectAnim_SourceFileStampList = 0;
    g_zEffectAnim_SourceFileStampCount = 0;
    if (stampMismatch) {
        fclose(stream);
        return -1;
    }

    zClass_NodePartial *const savedWorld = g_zEffectAnim_State.worldNode;
    if (fread(&g_zEffectAnim_State, sizeof(g_zEffectAnim_State), 1, stream) != 1) {
        fclose(stream);
        return -1;
    }
    g_zEffectAnim_State.worldNode = savedWorld;
    g_zEffectAnim_State.textIdEntryList = 0;

    g_zEffectAnim_State.entryList = (zEffectAnimEntry *)malloc(
        sizeof(zEffectAnimEntry) * g_zEffectAnim_State.entryCount);
    memset(g_zEffectAnim_State.entryList, 0,
        sizeof(zEffectAnimEntry) * g_zEffectAnim_State.entryCount);

    for (i = 0; i < g_zEffectAnim_State.entryCount; ++i) {
        zEffectAnimEntry *const entry = &g_zEffectAnim_State.entryList[i];
        if (fread(entry, sizeof(zEffectAnimEntry), 1, stream) != 1) {
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
            if (fread(
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
            if (fread(
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
            if (fread(
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
            if (fread(
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
            if (fread(
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
            if (fread(
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
            if (fread(
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
            if (fread(
                    entry->runtimeRefList,
                    sizeof(zEffectAnimRuntimeRef),
                    entry->runtimeRefCount,
                    stream
                ) != entry->runtimeRefCount) {
                fclose(stream);
                return -1;
            }

            for (int j0 = 0; j0 < entry->runtimeRefCount; ++j0) {
                entry->runtimeRefList[j0].cachedChildEntry = 0;
            }
        }

        if (entry->runtimeSequenceCount > 0) {
            entry->runtimeList = (zEffectAnimSurfaceRuntime *)(malloc(
                sizeof(zEffectAnimSurfaceRuntime) * entry->runtimeSequenceCount
            ));
        }
        zEffectAnimSurfaceRuntime *runtime = &entry->surfacePrimary;
        if (fread(runtime, sizeof(zEffectAnimSurfaceRuntime), 1, stream) != 1) {
            fclose(stream);
            return -1;
        }
        if (runtime->eventStreamSize > 0) {
            runtime->eventStream =
                malloc(runtime->eventStreamSize);
            if (fread(
                    runtime->eventStream,
                    runtime->eventStreamSize,
                    1,
                    stream
                ) != 1) {
                fclose(stream);
                return -1;
            }
        }

        for (int j1 = 0; j1 < entry->runtimeSequenceCount; ++j1) {
            runtime = &entry->runtimeList[j1];
            if (fread(runtime, sizeof(zEffectAnimSurfaceRuntime), 1, stream) != 1) {
                fclose(stream);
                return -1;
            }
            if (runtime->eventStreamSize > 0) {
                runtime->eventStream = malloc(runtime->eventStreamSize);
                if (fread(runtime->eventStream, runtime->eventStreamSize, 1, stream) != 1) {
                    fclose(stream);
                    return -1;
                }
            }
        }
    }

    const char *previousRootNodeName = 0;
    for (i = 0; i < g_zEffectAnim_State.entryCount; ++i) {
        zEffectAnimEntry *const entry = &g_zEffectAnim_State.entryList[i];
        const unsigned char savedSoundRefCount = entry->soundRefCount;
        const unsigned char savedLightRefCount = entry->lightRefCount;
        entry->lightRefCount = 0;
        entry->soundRefCount = 0;

        if (i == 0 || entry->activationState == 5) {
            continue;
        }
        {
            entry->boundNode = 0;
            if (previousRootNodeName != 0 &&
                strcmp(previousRootNodeName, entry->rootNodeName) == 0) {
                entry->boundNode = zClass_Class::gwNodeFindNextByName(0, 0);
            }

            if (entry->boundNode == 0) {
                zClass_Class::gwNodeFindNextByName(entry->rootNodeName, 6);
                entry->boundNode = zClass_Class::gwNodeFindNextByName(0, 0);
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

            entry->callbackNode = 0;
            entry->callbackNode = zEffectAnim::ResolveNodeByName(entry, entry->attachNodeName);
            if (entry->callbackNode == 0) {
                fclose(stream);
                return -1;
            }
        }

        entry->lightRefCount = savedLightRefCount;
        for (int j2 = 1; j2 < entry->lightRefCount; ++j2) {
            entry->lightRefList[j2].runtimeNode = zClass_Light::gwLightNew();
            if (entry->lightRefList[j2].runtimeNode == 0) {
                fclose(stream);
                return -1;
            }
            zClass_Class::gwNodeSetName(entry->lightRefList[j2].runtimeNode, entry->lightRefList[j2].name.text);
        }

        entry->soundRefCount = savedSoundRefCount;
        for (int j3 = 1; j3 < entry->soundRefCount; ++j3) {
            entry->soundRefList[j3].runtimeNode = zClass_Sound::gwSoundNew();
            if (entry->soundRefList[j3].runtimeNode == 0) {
                fclose(stream);
                return -1;
            }
            zClass_Class::gwNodeSetName(entry->soundRefList[j3].runtimeNode, entry->soundRefList[j3].name.text);
            zClass_Sound::SetSampleSetByName(entry->soundRefList[j3].runtimeNode, entry->soundRefList[j3].name.text);
        }

        for (int j4 = 1; j4 < entry->trackedNodeCount; ++j4) {
            entry->trackedNodeList[j4].trackedNode = zEffectAnim::ResolveNodeByName(entry, entry->trackedNodeList[j4].trackedNodeName);
            if (entry->trackedNodeList[j4].trackedNode == 0) {
                fclose(stream);
                return -1;
            }
        }

        for (int j5 = 1; j5 < entry->nodeRefCount; ++j5) {
            entry->nodeRefList[j5].node = zEffectAnim::ResolveNodeByName(entry, entry->nodeRefList[j5].name.text);
            if (entry->nodeRefList[j5].node == 0) {
                fclose(stream);
                return -1;
            }
        }

        for (int j6 = 1; j6 < entry->sampleRefCount; ++j6) {
            entry->sampleRefList[j6].sample =
                zSnd::FindSampleByName(entry->sampleRefList[j6].name);
        }

        for (int j7 = 1; j7 < entry->effectTemplateRefCount; ++j7) {
            entry->effectTemplateRefList[j7].templateIndex = zEffect::FindTemplateIndexByName(entry->effectTemplateRefList[j7].name);
            if (entry->effectTemplateRefList[j7].templateIndex == -1) {
                fclose(stream);
                return -1;
            }
        }

        if (entry->activationPrereqCount > 0) {
            zClass_NodePartial *prereqSearchRoot = 0;
            for (int j8 = 0; j8 < entry->activationPrereqCount; ++j8) {
                zEffectAnimActivationPrereq *const prereq = &entry->activationPrereqList[j8];
                if (prereq->mode == 1) {
                    prereq->targetEntry = 0;
                    zEffectAnimEntry *targetEntry = g_zEffectAnim_State.entryList;
                    for (int k = 1; k < g_zEffectAnim_State.entryCount; ++k, ++targetEntry) {
                        if (strcmp(targetEntry->name, prereq->targetName) == 0) {
                            prereq->targetEntry = targetEntry;
                            break;
                        }
                    }
                } else if (prereq->mode == 3 || prereq->mode == 2) {
                    const char *const nodeName = &prereq->targetName[4];
                    if (prereqSearchRoot == 0) {
                        prereqSearchRoot = zClass::FindByTypeAndName(6, nodeName);
                    } else {
                        prereqSearchRoot = zClass_Class::FindSubNodeByName(prereqSearchRoot, nodeName);
                    }

                    if (prereqSearchRoot == 0) {
                        zError::ReportOld(
                            0x400,
                            "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_init.c",
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
                    }
                }
            }
        }
    }

    if (g_zEffectAnim_State.textIdEntryCount > 0) {
        g_zEffectAnim_State.textIdEntryList = (zEffectAnimTextIdEntry *)(malloc(
            sizeof(zEffectAnimTextIdEntry) *
            (unsigned int)(g_zEffectAnim_State.textIdEntryCount)
        ));
        if (fread(
                g_zEffectAnim_State.textIdEntryList,
                sizeof(zEffectAnimTextIdEntry),
                (unsigned int)(g_zEffectAnim_State.textIdEntryCount),
                stream
            ) != (unsigned int)(g_zEffectAnim_State.textIdEntryCount)) {
            fclose(stream);
            return -1;
        }

        for (i = 0; i < g_zEffectAnim_State.textIdEntryCount; ++i) {
            g_zEffectAnim_State.textIdEntryList[i].messageId =
                zLoc::GetMessageId(g_zEffectAnim_State.textIdEntryList[i].messageKey);
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
int __cdecl LoadAndInstantiate() {
    char runtimeNodeName[0x24];
    if (g_zEffectAnim_State.entriesInstantiated != 0) {
        return 0;
    }

    LoadZbd();

    zEffectAnimEntry *entry = &g_zEffectAnim_State.entryList[1];
    for (int i = 1; i < g_zEffectAnim_State.entryCount; ++i, ++entry) {
        if (entry->activationState != 5 && entry->activationState != 4 &&
            zClass::AnyNodeMatchesPredicateRecursive(
                entry->boundNode,
                zClass_Node::HasRenderableDiPredicate
            ) != 0) {
            entry->flags |= 0x200;
        }
    }

    entry = &g_zEffectAnim_State.entryList[1];
    for (int i_2286 = 1; i_2286 < g_zEffectAnim_State.entryCount; ++i_2286, ++entry) {
        if (entry->activationState == 5) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_init.c",
                0x2d55,
                g_zEffectAnim_CorruptAnimationLoadedFmt,
                entry
            );
        } else {
            if (entry->boundNode == 0) {
                zClass_NodePartial *const objectNode = zClass_Object3D::gwObject3DInit();
                entry->boundNode = objectNode;
                entry->callbackNode = objectNode;
            }

            if ((entry->flags & kEffectAnimNeedsCopiedRootFlag) != 0) {
                zEffectAnim::EnsureCopiedRootTree(entry, entry->boundNode);
            }

            entry->runtimeNode = zClass_Object3D::gwObject3DInit();
            sprintf(runtimeNodeName, "_%s", entry->name);
            zClass_Class::gwNodeSetName(entry->runtimeNode, runtimeNodeName);
            zClass_Class::gwNodeSetPriority(entry->runtimeNode, entry->priority);

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
                    zEffectAnim::StopAndCleanup(entry, 0, 0);
                }
                if (entry->activationMode == 4) {
                    zEffectAnim::SetVelocity(entry, 0, 0.0f, 0.0f, 0.0f);
                }
            }
        }
    }

    g_zEffectAnim_State.entriesInstantiated = 1;
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
int __cdecl Shutdown() {
    if (g_zEffectAnim_State.heapPtr != 0) {
        free(g_zEffectAnim_State.heapPtr);
        g_zEffectAnim_State.heapPtr = 0;
    }

    g_zEffectAnim_State.countsPackedLoWord = 0;
    for (int i = 0; i < g_zEffectAnim_State.entryCount; ++i) {
        zEffectAnim::ShutdownEntry(&g_zEffectAnim_State.entryList[i]);
    }

    if (g_zEffectAnim_State.entryList != 0) {
        free(g_zEffectAnim_State.entryList);
        g_zEffectAnim_State.entryList = 0;
    }

    g_zEffectAnim_State.entryCount = 0;
    if (g_zEffectAnim_State.textIdEntryList != 0) {
        free(g_zEffectAnim_State.textIdEntryList);
        g_zEffectAnim_State.textIdEntryList = 0;
    }

    g_zEffectAnim_State.textIdEntryCount = 0;
    ClearActivationRecords();
    g_zEffectAnim_State.entriesInstantiated = 0;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.shutdownifloaded
 * @recoil-artifact defines .text recoil:function:0x45fef0: zEffect_Anim::ShutdownIfLoaded.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_init.c.
 * Purpose: run animation shutdown only when entries are currently
 * instantiated.
 */
int __cdecl ShutdownIfLoaded() {
    if (g_zEffectAnim_State.entriesInstantiated != 0) {
        Shutdown();
    }

    return 0;
}

} // namespace zEffect_Anim

namespace zEffectAnim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.findentrybyname
 * @recoil-artifact defines .text recoil:function:0x45ff10: zEffectAnim::FindEntryByName (zeff_anim.c)
 * @recoil-match byte
 *
 * Purpose: Return the first zEffect animation entry whose table name matches the requested name.
 */
zEffectAnimEntry *__fastcall FindEntryByName(
    const char *name
) {
    if (name == 0) {
        return 0;
    }

    const int count = g_zEffectAnim_State.entryCount;
    for (int i = 0; i < count; ++i) {
        if (strcmp(g_zEffectAnim_State.entryList[i].name, name) == 0) {
            return &g_zEffectAnim_State.entryList[i];
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
        index = (int)(currentEntry - g_zEffectAnim_State.entryList) + 1;
    }

    for (; index < g_zEffectAnim_State.entryCount; ++index) {
        if ((g_zEffectAnim_State.entryList[index].flags & 0x10) != 0) {
            return &g_zEffectAnim_State.entryList[index];
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-init.getrootnodeornull
 * @recoil-artifact defines .text recoil:function:0x460010: zEffectAnim::GetRootNodeOrNull (zeff_anim.c)
 * @recoil-match byte
 *
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
