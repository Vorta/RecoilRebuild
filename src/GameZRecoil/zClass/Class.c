#include "zclass.h"

#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "zdi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-nodecount
 * @recoil-artifact defines .data recoil:data:0x4f4a90: g_zClass_NodeCount.
 * Unresolved candidate: no retail references prove a node/core shadow block
 * or the individual objects below. Their BN identities and positive tracker
 * gates were withdrawn by the full .data audit. Keep the current source
 * contributions pending an evidence-backed storage/placement correction;
 * zero bytes alone authorize neither these types nor deletion of storage.
 * Purpose: retain this candidate storage without asserting a node-count role.
 */
int g_zClass_NodeCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-nodetablebase
 * @recoil-artifact defines .data recoil:data:0x4f4a94: g_zClass_NodeTableBase.
 * Purpose: retain candidate storage; original pointer identity is unresolved.
 */
zClass_NodePartial *g_zClass_NodeTableBase = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-nodeactivecount
 * @recoil-artifact defines .data recoil:data:0x4f4a98: g_zClass_NodeActiveCount.
 * Purpose: retain candidate storage; original count identity is unresolved.
 */
int g_zClass_NodeActiveCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-copynodeclonematerialrefs
 * @recoil-artifact defines .data recoil:data:0x4f4a9c: g_zClass_CopyNodeCloneMaterialRefs.
 * Purpose: retain candidate storage; original policy-flag identity is unresolved.
 */
int g_zClass_CopyNodeCloneMaterialRefs = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-copynodecloneallmaterialsifrelevant
 * @recoil-artifact defines .data recoil:data:0x4f4aa0: g_zClass_CopyNodeCloneAllMaterialsIfRelevant.
 * Purpose: retain candidate storage; original policy-flag identity is unresolved.
 */
int g_zClass_CopyNodeCloneAllMaterialsIfRelevant = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-coreinitialized
 * @recoil-artifact defines .data recoil:data:0x4f4aa4: g_zClass_CoreInitialized.
 * Purpose: retain candidate storage; original initialization-flag identity is unresolved.
 */
int g_zClass_CoreInitialized = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-lastzbdpath
 * @recoil-artifact defines .data recoil:data:0x4f4aa8: g_zClass_LastZbdPath.
 * Purpose: retain candidate storage; original buffer identity and extent are unresolved.
 */
char g_zClass_LastZbdPath[0x30] = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-nodearray
 * @recoil-artifact defines .data recoil:data:0x539c94: g_zClass_NodeArray.
 * BN evidence: zClass::Init/ShutdownCore and ZBD node-table helpers reference
 * this global node pool pointer, and Class.c alloc/free paths index through it.
 * Purpose: store the active zClass node-slot array backing runtime scene nodes.
 */
zClass_NodeFreeListSlot *g_zClass_NodeArray = 0;
/**
 * BN evidence: Class.c alloc/free paths update this count, while zClass::Init,
 * ShutdownCore, and ZBD reads reset or recompute it from the node pool.
 * Purpose: count currently allocated nodes in the global zClass node array.
 */
int g_zClass_ActiveNodeCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-nodefreeheadindex
 * @recoil-artifact defines .data recoil:data:0x4de4c8: g_zClass_NodeFreeHeadIndex.
 * BN evidence: Class.c alloc/free paths load and store this head index, with
 * zClass::Init/ShutdownCore and ZBD serialization preserving the free list.
 * Purpose: identify the first free zClass node-slot index or -1 when empty.
 */
int g_zClass_NodeFreeHeadIndex = -1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-currentzbdpath
 * @recoil-artifact defines .data recoil:data:0x539ca8: g_zClass_CurrentZbdPath.
 * BN data inventory declares char[0x30] at 0x539ca8.
 * Purpose: store the current ZBD path prefix used by zClass loading.
 */
char g_zClass_CurrentZbdPath[0x30] = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-maincamera
 * @recoil-artifact defines .data recoil:data:0x4f36bc: g_MainCamera.
 * BN evidence: player, HUD, and play-state camera callers reference this
 * global before zClass_Camera operations and world-node attachment calls.
 * Purpose: store the current main camera node used by gameplay and rendering.
 */
zClass_NodePartial *g_MainCamera = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-player-runtimediscene
 * @recoil-artifact defines .data recoil:data:0x4f36b8: g_Player_RuntimeDiScene.
 * Purpose: Stores g Player RuntimeDiScene data used by engine.zclass.player_runtime_di_scene_global.
 */
zClass_NodePartial *g_Player_RuntimeDiScene = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-renderboundscontextactive
 * @recoil-artifact defines .data recoil:data:0x4ddd28: g_zClass_RenderBoundsContextActive.
 * BN evidence: camera, sound, light, object, animate, LOD, sequence, and switch
 * render traversals test and bracket this flag while updating bounds contexts.
 * Purpose: mark that rendering is inside a bounds-update traversal context.
 */
int g_zClass_RenderBoundsContextActive = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-renderfrustumgridtileindex
 * @recoil-artifact defines .data recoil:data:0x4ddd2c: g_zClass_RenderFrustumGridTileIndex.
 * BN evidence: camera frustum-grid rendering writes this index and object
 * traversal reads it when selecting grid-tile render behavior.
 * Purpose: identify the active frustum-grid tile during camera render passes.
 */
int g_zClass_RenderFrustumGridTileIndex = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-renderrangefadeactive
 * @recoil-artifact defines .data recoil:data:0x539980: g_zClass_RenderRangeFadeActive.
 * BN evidence: LOD traversal brackets this flag, and render traversals test it
 * before applying range-fade blend scale to display instances.
 * Purpose: mark that range-fade alpha scaling is active for child renders.
 */
int g_zClass_RenderRangeFadeActive = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-renderrangefadescale
 * @recoil-artifact defines .data recoil:data:0x539828: g_zClass_RenderRangeFadeScale.
 * BN evidence: LOD traversal computes this float and camera, sound, light,
 * object, and animate traversals copy it into display-instance blend scale.
 * Purpose: store the current range-fade blend scale for render traversal.
 */
float g_zClass_RenderRangeFadeScale = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-rendervertexalphaoverrideactive
 * @recoil-artifact defines .data recoil:data:0x539b94: g_zClass_RenderVertexAlphaOverrideActive.
 * BN evidence: object and LOD render traversals set, test, and clear this flag
 * around vertex-alpha override rendering.
 * Purpose: prevent nested vertex-alpha override state from being applied twice.
 */
int g_zClass_RenderVertexAlphaOverrideActive = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-renderalphascalestacktop
 * @recoil-artifact defines .data recoil:data:0x4ddd3c: g_zClass_RenderAlphaScaleStackTop.
 * BN evidence: object and LOD render traversals push and pop this index, then
 * restore zModel render alpha scale from the stack top.
 * Purpose: track the current render alpha-scale stack entry.
 */
int g_zClass_RenderAlphaScaleStackTop = -1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-renderalphascalestack
 * @recoil-artifact defines .data recoil:data:0x539830: g_zClass_RenderAlphaScaleStack.
 * BN data inventory declares float[0x10] at 0x539830.
 * Purpose: store nested render alpha scale values for traversal restore.
 */
float g_zClass_RenderAlphaScaleStack[0x10] = {0};
extern char g_zClass_SourceFile_SwitchC[0x24];
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-softwarepathstatestacktop
 * @recoil-artifact defines .data recoil:data:0x4ddd40: g_zClass_SoftwarePathStateStackTop.
 * BN evidence: object render traversal pushes and pops this index while
 * restoring software-path color and alpha render state.
 * Purpose: track the current software renderer color/alpha state stack entry.
 */
int g_zClass_SoftwarePathStateStackTop = -1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-softwarepathrenderstatestack
 * @recoil-artifact defines .data recoil:data:0x539988: g_zClass_SoftwarePathRenderStateStack.
 * BN data inventory declares a 64-byte stack, matching four color/alpha states.
 * Purpose: store nested software render color and alpha state.
 */
zClass_RenderColorAlphaState g_zClass_SoftwarePathRenderStateStack[4] = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-loddistancestatestacktop
 * @recoil-artifact defines .data recoil:data:0x4ddd30: g_zClass_LodDistanceStateStackTop.
 * BN evidence: LOD traversal and camera/video render setup read, reset, push,
 * and pop this index while computing active LOD distance state.
 * Purpose: track the current LOD distance-state stack entry during rendering.
 */
int g_zClass_LodDistanceStateStackTop = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-loddistancestatestack
 * @recoil-artifact defines .data recoil:data:0x539900: g_zClass_LodDistanceStateStack.
 * BN data inventory declares a 64-byte stack, matching four LOD states.
 * Purpose: store nested LOD distance state for render traversal.
 */
zClass_LodDistanceState g_zClass_LodDistanceStateStack[4] = {0};
}

namespace {
    const char kClassSourceFile[] = "D:\\Proj\\GameZRecoil\\zClass\\Class.c";
    const int kQueuedTreeBucket = 7;
    const int kTypeListInsertedFlag = 0x01;
    const int kTransformQueuedFlag = 0x02;
    const int kBoundsDirtyFlag = 0x02;
    const int kSingleParentFlag = 0x00080000;
    const int kNodeVariantGateFlag = 0x01000000;
    const int kNodeTransformDirtyPropagatedFlag = 0x02000000;

    /*
     * BN type evidence: zClass_CameraData stores a union at 0x80 whose
     * cachedViewMatrix arm is used by node bbox query helpers 0x4487c0 and
     * 0x448920.
     */
    struct zClass_CameraViewTargetStatePartial {
        unsigned char viewBasis[0x24];
        zVec3 worldTarget;
    };

    union zClass_CameraViewOverlayPartial {
        zClass_CameraViewTargetStatePartial targetState;
        zMat4x3 cachedViewMatrix;
    };

    struct zClass_CameraBBoxQueryDataPartial {
        zClass_NodePartial *worldNode;
        zClass_NodePartial *windowNode;
        zClass_NodePartial *horizonNode;
        zClass_NodePartial *horizonXZNode;
        int cameraFlags;
        zVec3 targetOrEuler;
        zVec3 posOffset;
        zVec3 worldPos;
        zVec3 eulerAngles;
        zMat4x3 worldTransform;
        zVec3 forwardDir;
        zClass_CameraViewOverlayPartial viewOverlay;
    };

    RECOIL_STATIC_ASSERT(sizeof(zClass_CameraViewTargetStatePartial) == 0x30);
    RECOIL_STATIC_ASSERT(sizeof(zClass_CameraViewOverlayPartial) == 0x30);
    RECOIL_STATIC_ASSERT(
        offsetof(
            zClass_CameraBBoxQueryDataPartial,
            viewOverlay
        ) == 0x80
    );

    /**
     * Original-source helper evidence: no standalone retail function is
     * present; observed in Class.c typed vector-field access patterns.
     * Purpose: address a mutable zVec3 field inside a recovered class record.
     */
    zVec3 *Vec3At(
        void *base,
        size_t offset
    ) {
        return (zVec3 *)((unsigned char *)(base) + offset);
    }

    /**
     * Original-source helper evidence: no standalone retail function is
     * present; observed in Class.c typed vector-field access patterns.
     * Purpose: address a const zVec3 field inside a recovered class record.
     */
    const zVec3 *Vec3At(
        const void *base,
        size_t offset
    ) {
        return (const zVec3 *)((const unsigned char *)(base) + offset);
    }

    /**
     * Original-source helper evidence: no standalone retail function is
     * present; observed in 0x448e90 and 0x4491b0.
     * Purpose: access the child-aggregate bounding box for a node slot.
     */
    /**
     * Original-source helper evidence: no standalone retail function is
     * present; observed in 0x448e90 and 0x449420.
     * Purpose: access the display-instance bounding box for a node slot.
     */
    /**
     * Original-source helper evidence: no standalone retail function is
     * present; observed in 0x448e90 and bbox query callers.
     * Purpose: access the const display-instance bounding box for a node slot.
     */
    /**
     * Original-source helper evidence: no standalone retail function is
     * present; observed in 0x448e90 and bbox query callers.
     * Purpose: access the const child-aggregate bounding box for a node slot.
     */
    /**
     * Original-source helper evidence: no standalone retail function is
     * present; observed in 0x448e90 primary/secondary box merge logic.
     * Purpose: produce the union of two node bounding boxes.
     */
    zBBox3f MergeBBoxes(
        const zBBox3f *a,
        const zBBox3f *b
    ) {
        zBBox3f merged = {0};
        merged.minX = a->minX < b->minX ? a->minX : b->minX;
        merged.minY = a->minY < b->minY ? a->minY : b->minY;
        merged.minZ = a->minZ < b->minZ ? a->minZ : b->minZ;
        merged.maxX = a->maxX > b->maxX ? a->maxX : b->maxX;
        merged.maxY = a->maxY > b->maxY ? a->maxY : b->maxY;
        merged.maxZ = a->maxZ > b->maxZ ? a->maxZ : b->maxZ;
        return merged;
    }

    /**
     * Original-source helper evidence: no standalone retail function is
     * present; observed in 0x448e90 cached-bounds update logic.
     * Purpose: copy a typed bounding box into the node cached-bounds storage.
     */
    void CopyBBoxToCachedBounds(
        zClass_NodePartial * node,
        const zBBox3f *bbox
    ) {
        memcpy(
            node->cachedBounds,
            bbox,
            sizeof(*bbox)
        );
    }

}

namespace zClass_Class {
    int __fastcall TryFreeNode(zClass_NodePartial * node);

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.allocnodefromfreelist
     * @recoil-artifact defines .text recoil:function:0x4478c0: zClass_Class::AllocNodeFromFreeList.
     * Purpose: pop a node from the global free list, clear it, and install
     * default active-node state.
     */
    zClass_NodePartial *__cdecl AllocNodeFromFreeList() {
        const int index = g_zClass_NodeFreeHeadIndex;
        if (index != -1) {
            zClass_NodeFreeListSlot *slot = &g_zClass_NodeArray[index];
            zClass_NodePartial *node = &slot->node;
            g_zClass_NodeFreeHeadIndex = (int)(slot->freeTag << 8) >> 8;

            memset(
                node,
                0,
                offsetof(zClass_NodeFreeListSlot, freeTag)
            );
            /**
             * BN evidence: AllocNodeFromFreeList increments this global after
             * clearing a popped node slot from g_zClass_NodeArray.
             * Purpose: account for the newly active node before type-list use.
             */
            ++g_zClass_ActiveNodeCount;
            zClass_TypeList::Insert(
                6,
                node
            );

            node->flags = 0x0108001c;
            node->callbackPriority = 1;
            node->gridCol = -1;
            node->gridRow = -1;
            node->nodeType = 0xff;
            sprintf(
                node->name,
                "%s",
                "Default_node_name"
            );
            slot->damageHandler = 0;
            return node;
        }

        zError::ReportOld(
            0x400,
            "D:\\Proj\\GameZRecoil\\zClass\\Class.c",
            0x1ed,
            "gwNodeNew(): GameZ node buffer is full:\n"
        );
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.deletenodebytype
     * @recoil-artifact defines .text recoil:function:0x447980: zClass_Class::DeleteNodeByType.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Class.c.
     * Purpose: validate node ownership and dispatch deletion by classId.
    */
    int __fastcall DeleteNodeByType(zClass_NodePartial * node) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x231,
                "Null node pointer."
            );
            return 5;
        }

        if (node->listCountA > 0) {
            return 1;
        }

        switch (node->classId) {
        case 5:
            return zClass_Object3D::DeleteNode(node);
        case 1:
            return zClass_Camera::DeleteNode(node);
        case 2:
            return zClass_World::DeleteNode(node);
        case 3:
            return zClass_Window::DeleteNode(node);
        case 4:
            return zClass_Display::DeleteNode(node);
        case 6:
            return zClass_Lod::DeleteNode(node);
        case 7:
            return zClass_Sequence::DeleteNode(node);
        case 8:
            return zClass_Animate::DeleteNode(node);
        case 9:
            return zClass_Light::DeleteNode(node);
        case 10:
            return zClass_Sound::DeleteNode(node);
        case 11:
            return zClass_Switch::DeleteNode(node);
        case 0:
            TryFreeNode(node);
            return (int)((unsigned int)(node));
        default:
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x272,
                "ERROR: Unrecognized node class type for node: %s\n",
                node->name
            );
            return 1;
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.freenodetofreelist
     * @recoil-artifact defines .text recoil:function:0x447a70: zClass_Class::FreeNodeToFreeList.
     * Purpose: release owned node lists/data and return the node slot to the
     * global zClass free-list while preserving the slot free-tag flags.
     */
    int __fastcall FreeNodeToFreeList(zClass_NodePartial * node) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x28e,
                "Null node pointer."
            );
            return 5;
        }
        if (node->listCountB > 0) {
            return 1;
        }
        if (node->listCountA > 0) {
            return 1;
        }
        if (node->userDataOrDiRef != 0) {
            return 1;
        }

        if (node->listB != 0) {
            free(node->listB);
            node->listB = 0;
        }
        if (node->listA != 0) {
            free(node->listA);
            node->listA = 0;
        }
        if (node->classData != 0) {
            free(node->classData);
            node->classData = 0;
            node->classId = 0;
        }

        const ptrdiff_t index = (zClass_NodeFreeListSlot *)(node)-g_zClass_NodeArray;
        zClass_NodeFreeListSlot &slot = g_zClass_NodeArray[index];
        slot.freeTag =
            (slot.freeTag & 0xff000000) | ((unsigned int)(g_zClass_NodeFreeHeadIndex) & 0x00ffffff);
        --g_zClass_ActiveNodeCount;
        g_zClass_NodeFreeHeadIndex = (int)(index);

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.tryfreenode
     * @recoil-artifact defines .text recoil:function:0x447b60: zClass_Class::TryFreeNode.
     * Purpose: remove a node from active lists, then either free it
     * immediately or enqueue it for deferred freeing.
    */
    int __fastcall TryFreeNode(zClass_NodePartial * node) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x2f0,
                "Null node pointer."
            );
            return 5;
        }

        node->flags &= ~kTransformQueuedFlag;
        zClass_List::DeleteNodeFromLists(node);
        if (zClass::ProcessDeferredWork() == 0) {
            FreeNodeToFreeList(node);
        } else {
            zClass_NodeList::Insert(node);
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.findnoderecursivebyname
     * @recoil-artifact defines .text recoil:function:0x447bc0: zClass_Class::FindNodeRecursiveByName
     * BN caveat: the inlined strcmp-style comparison has a known sbb
     * flag-generation limitation; assembly still proves the typed node-name
     * comparison and forward child recursion.
     * Purpose: search a zClass node subtree by exact node name, returning the
     * first matching node in forward child-list order.
     */
    zClass_NodePartial *__fastcall FindNodeRecursiveByName(
        zClass_NodePartial * root,
        const char *name
    ) {
        if (root == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x33a,
                "Null node pointer."
            );
            return 0;
        }

        if (strcmp(
            root->name,
            name
        ) == 0) {
            return root;
        }

        for (int i = 0; i < root->listCountB; ++i) {
            zClass_NodePartial *const childMatch = FindNodeRecursiveByName(
                root->listB[i],
                name
            );
            if (childMatch != 0) {
                return childMatch;
            }
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetactive
     * @recoil-artifact defines .text recoil:function:0x447c60: zClass_Class::gwNodeSetActive.
     *
     * Purpose: toggle the active flag for supported node classes and delegate
     * sound-node activity changes to the sound owner.
     */
    int __fastcall gwNodeSetActive(
        zClass_NodePartial * node,
        int active
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x38d,
                "Null node pointer."
            );
            return 5;
        }

        if (
            node->classId == 1 ||
            node->classId == 2 ||
            node->classId == 5 ||
            node->classId == 6 ||
            node->classId == 9
        ) {
            if (active == 1) {
                node->flags |= 0x04;
            } else if (active == 0) {
                node->flags &= ~0x04;
            }
            return 0;
        }
        if (node->classId == 10) {
            zClass_Sound::gwSoundSetActive(
                node,
                active
            );
            return 0;
        }

        zError::ReportOld(
            0x400,
            kClassSourceFile,
            0x3a4,
            "gwNodeSetActive(): Unrecognized node class type:\n  node = %s class_type = %d\n",
            node,
            node->classId
        );
        return 3;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetflag16
     * @recoil-artifact defines .text recoil:function:0x447d20: zClass_Class::gwNodeSetFlag16
     * Purpose: set or clear node flag bit 16.
     */
    int __fastcall gwNodeSetFlag16(
        zClass_NodePartial * node,
        int value
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x3b7,
                "Null node pointer."
            );
            return 5;
        }

        if (value != 0) {
            node->flags |= 0x10000;
        } else {
            node->flags &= ~0x10000;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetflag17
     * @recoil-artifact defines .text recoil:function:0x447d70: zClass_Class::gwNodeSetFlag17
     * Purpose: set or clear node flag bit 17.
     */
    int __fastcall gwNodeSetFlag17(
        zClass_NodePartial * node,
        int value
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x3c6,
                "Null node pointer."
            );
            return 5;
        }

        if (value != 0) {
            node->flags |= 0x20000;
        } else {
            node->flags &= ~0x20000;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetname
     * @recoil-artifact defines .text recoil:function:0x447dc0: zClass_Class::gwNodeSetName
     * Purpose: copy or truncate a caller-supplied name into a zClass node's
     * fixed-size name buffer.
     */
    int __fastcall gwNodeSetName(
        zClass_NodePartial * node,
        const char *name
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x3df,
                "Null node pointer."
            );
            return 5;
        }

        if (strlen(name) >= sizeof(node->name)) {
            strncpy(
                node->name,
                name,
                0x22
            );
            node->name[0x23] = '\0';
        } else {
            sprintf(
                node->name,
                "%s",
                name
            );
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodegetname
     * @recoil-artifact defines .text recoil:function:0x447e30: zClass_Class::gwNodeGetName
     * Purpose: return the fixed-size name buffer for a zClass node.
     */
    char *__fastcall gwNodeGetName(zClass_NodePartial * node) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x40d,
                "Null node pointer."
            );
            return 0;
        }

        return node->name;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetdisplayinstance
     * @recoil-artifact defines .text recoil:function:0x447e60: zClass_Class::gwNodeSetDisplayInstance
     * Purpose: replace a node's display-instance reference, maintain zDi
     * reference counts, rebuild its bounds, and queue transform updates.
     */
    int __fastcall gwNodeSetDisplayInstance(
        zClass_NodePartial * node,
        zDiPartial * displayInstance
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Class.c",
                0x424,
                "Null node pointer."
            );
            return 5;
        }

        zDiPartial *oldDisplayInstance = (zDiPartial *)((unsigned int)(node->userDataOrDiRef));
        if (oldDisplayInstance != 0) {
            zDi::Release(oldDisplayInstance);
        }

        node->userDataOrDiRef = (unsigned int)((unsigned int)(displayInstance));
        if (displayInstance != 0) {
            zDi::AddRef(displayInstance);
            zDi::RebuildBounds(
                (zDiPartial *)((unsigned int)(node->userDataOrDiRef)),
                (zBoundsMinMaxPartial *)(&((zClass_NodeFreeListSlot *)node)->primaryBounds)
            );
            node->flags |= 0x200;
        } else {
            node->flags &= ~0x200;
        }

        node->boundsFlags |= 0x01;
        if ((node->flags & kTypeListInsertedFlag) == 0) {
            zClass_TypeList::Insert(
                kQueuedTreeBucket,
                node
            );
            node->flags |= kTypeListInsertedFlag;
        }
        node->flags |= kTransformQueuedFlag;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodegetuserdata
     * @recoil-artifact defines .text recoil:function:0x447f00: zClass_Class::gwNodeGetUserData
     * Purpose: read the user-data or display-instance reference stored on a
     * zClass node.
     */
    int __fastcall gwNodeGetUserData(
        zClass_NodePartial * node,
        unsigned int *outData
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Class.c",
                0x464,
                "Null node pointer."
            );
            return 5;
        }

        *outData = node->userDataOrDiRef;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetactioncallback
     * @recoil-artifact defines .text recoil:function:0x447f30: zClass_Class::gwNodeSetActionCallback
     * Purpose: install or clear the node action callback in its priority
     * bucket using head insertion for newly active callback nodes.
     */
    int __fastcall gwNodeSetActionCallback(
        zClass_NodePartial * node,
        void *actionCallback
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x47e,
                "Null node pointer."
            );
            return 5;
        }

        int callbackPriority = node->callbackPriority;
        if (callbackPriority >= 0 && callbackPriority < 6) {
            if (node->actionCallback == 0) {
                if (actionCallback != 0) {
                    if (zClass_TypeList::Insert(
                        callbackPriority,
                        node
                    ) != 0) {
                        if ((node->flags & 0x800) == 0) {
                            free(node);
                        }
                        return 5;
                    }
                }
            } else if (actionCallback == 0) {
                zClass_TypeList::MarkPendingRemoval(
                    callbackPriority,
                    node
                );
            }

            node->actionCallback = actionCallback;
            return 0;
        }

        zError::ReportOld(
            0x400,
            kClassSourceFile,
            0x483,
            "ERROR setting action callback; priority = %d",
            callbackPriority
        );
        return 1;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetactioncallbacktail
     * @recoil-artifact defines .text recoil:function:0x447fe0: zClass_Class::gwNodeSetActionCallbackTail.
     * Purpose: install or clear a node action callback using tail insertion
     * for newly active callback buckets.
     */
    int __fastcall gwNodeSetActionCallbackTail(
        zClass_NodePartial * node,
        void *actionCallback
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x4c3,
                "Null node pointer."
            );
            return 5;
        }

        int callbackPriority = node->callbackPriority;
        if (callbackPriority >= 0 && callbackPriority < 6) {
            if (node->actionCallback == 0) {
                if (actionCallback != 0) {
                    if (zClass_TypeList::InsertChildNodes(
                        callbackPriority,
                        node
                    ) != 0) {
                        if ((node->flags & 0x800) == 0) {
                            free(node);
                        }
                        return 5;
                    }
                }
            } else if (actionCallback == 0) {
                zClass_TypeList::MarkPendingRemoval(
                    callbackPriority,
                    node
                );
            }

            node->actionCallback = actionCallback;
            return 0;
        }

        zError::ReportOld(
            0x400,
            kClassSourceFile,
            0x4c8,
            "ERROR setting action callback; priority = %d",
            callbackPriority
        );
        return 1;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetpriority
     * @recoil-artifact defines .text recoil:function:0x448090: zClass_Class::gwNodeSetPriority
     * Purpose: move an active callback node between priority buckets and store
     * the caller-supplied priority value.
     */
    int __fastcall gwNodeSetPriority(
        zClass_NodePartial * node,
        int priority
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x4fc,
                "Null node pointer."
            );
            return 5;
        }

        if (node->actionCallback != 0) {
            if (node->callbackPriority >= 0 && node->callbackPriority < 6) {
                zClass_TypeList::MarkPendingRemoval(
                    node->callbackPriority,
                    node
                );
            }
            if (priority >= 0 && priority < 6) {
                zClass_TypeList::Insert(
                    priority,
                    node
                );
            }
        }

        node->callbackPriority = priority;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetcellpickable
     * @recoil-artifact defines .text recoil:function:0x448100: zClass_Class::gwNodeSetCellPickable
     * Purpose: set or clear the cell-pickable flag on a node.
     */
    int __fastcall gwNodeSetCellPickable(
        zClass_NodePartial * node,
        int value
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x529,
                "Null node pointer."
            );
            return 5;
        }

        if (value != 0) {
            node->flags |= 0x08;
        } else {
            node->flags &= ~0x08;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodegetcellpickable
     * @recoil-artifact defines .text recoil:function:0x448140: zClass_Class::gwNodeGetCellPickable
     * Purpose: read the cell-pickable flag from a node.
     */
    int __fastcall gwNodeGetCellPickable(
        zClass_NodePartial * node,
        int *outValue
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x542,
                "Null node pointer."
            );
            return 5;
        }

        *outValue = (node->flags >> 3) & 1;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodegetnodetype
     * @recoil-artifact defines .text recoil:function:0x448180: zClass_Class::gwNodeGetNodeType
     * Purpose: read the byte-sized node type metadata value.
     */
    int __fastcall gwNodeGetNodeType(
        zClass_NodePartial * node,
        int *outValue
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x556,
                "Null node pointer."
            );
            return 5;
        }

        *outValue = node->nodeType;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetraycastable
     * @recoil-artifact defines .text recoil:function:0x4481b0: zClass_Class::gwNodeSetRaycastable
     * Purpose: set or clear the raycastable flag on a node.
     */
    int __fastcall gwNodeSetRaycastable(
        zClass_NodePartial * node,
        int value
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x56c,
                "Null node pointer."
            );
            return 5;
        }

        if (value != 0) {
            node->flags |= 0x10;
        } else {
            node->flags &= ~0x10;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodegetraycastable
     * @recoil-artifact defines .text recoil:function:0x4481f0: zClass_Class::gwNodeGetRaycastable
     * Purpose: read the raycastable flag from a node.
     */
    int __fastcall gwNodeGetRaycastable(
        zClass_NodePartial * node,
        int *outValue
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x584,
                "Null node pointer."
            );
            return 5;
        }

        *outValue = (node->flags >> 4) & 1;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetpickable
     * @recoil-artifact defines .text recoil:function:0x448230: zClass_Class::gwNodeSetPickable
     * Purpose: set or clear the pickable flag on a node.
     */
    int __fastcall gwNodeSetPickable(
        zClass_NodePartial * node,
        int value
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x59a,
                "Null node pointer."
            );
            return 5;
        }

        if (value != 0) {
            node->flags |= 0x20;
        } else {
            node->flags &= ~0x20;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodegetpickable
     * @recoil-artifact defines .text recoil:function:0x448270: zClass_Class::gwNodeGetPickable
     * Purpose: read the pickable flag from a node.
     */
    int __fastcall gwNodeGetPickable(
        zClass_NodePartial * node,
        int *outValue
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x5b2,
                "Null node pointer."
            );
            return 5;
        }

        *outValue = (node->flags >> 5) & 1;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesethashitcallback
     * @recoil-artifact defines .text recoil:function:0x4482b0: zClass_Class::gwNodeSetHasHitCallback
     * Purpose: set or clear the node flag that marks an installed hit
     * callback handler.
     */
    int __fastcall gwNodeSetHasHitCallback(
        zClass_NodePartial * node,
        int value
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x5c7,
                "Null node pointer."
            );
            return 5;
        }

        if (value != 0) {
            node->flags |= 0x40;
        } else {
            node->flags &= ~0x40;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetbypassfarclip
     * @recoil-artifact defines .text recoil:function:0x4482f0: zClass_Class::gwNodeSetBypassFarClip
     * Purpose: set or clear the node flag that bypasses far-clip culling.
     */
    int __fastcall gwNodeSetBypassFarClip(
        zClass_NodePartial * node,
        int value
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x5e1,
                "Null node pointer."
            );
            return 5;
        }

        if (value != 0) {
            node->flags |= 0x80;
        } else {
            node->flags &= ~0x80;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetnodetype
     * @recoil-artifact defines .text recoil:function:0x448330: zClass_Class::gwNodeSetNodeType
     * Purpose: store the low byte of the caller-supplied node type metadata
     * value.
     */
    int __fastcall gwNodeSetNodeType(
        zClass_NodePartial * node,
        int nodeType
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Class.c",
                0x5f9,
                "Null node pointer."
            );
            return 5;
        }

        node->nodeType = (unsigned char)(nodeType);
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodeclearvariantgate
     * @recoil-artifact defines .text recoil:function:0x448360: zClass_Class::gwNodeClearVariantGate
     * Purpose: clear the node variant-gate flag when the caller supplies a
     * zero gate value.
     */
    int __fastcall gwNodeClearVariantGate(
        zClass_NodePartial * node,
        int value
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x60f,
                "Null node pointer."
            );
            return 5;
        }

        if ((node->flags & kNodeVariantGateFlag) != 0 && value == 0) {
            node->flags &= ~kNodeVariantGateFlag;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetvertexalphaoverride
     * @recoil-artifact defines .text recoil:function:0x4483a0: zClass_Class::gwNodeSetVertexAlphaOverride.
     * Purpose: set or clear the caller-owned node vertex-alpha override flag.
     */
    int __fastcall gwNodeSetVertexAlphaOverride(
        zClass_NodePartial * node,
        int value
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x62d,
                "Null node pointer."
            );
            return 5;
        }

        if (value != 0) {
            node->flags |= 0x00800000;
        } else {
            node->flags &= ~0x00800000;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.addchild
     * @recoil-artifact defines .text recoil:function:0x4483f0: zClass_Class::AddChild.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Class.c.
     * Purpose: dispatch child attachment by parent classId across the
     * data-driven zClass node subsystem.
     */
    int __fastcall AddChild(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ) {
        if (child == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x666,
                "Null node pointer."
            );
            return 5;
        }
        if (parent == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x667,
                "Null node pointer."
            );
            return 5;
        }

        int result;
        switch (parent->classId) {
        case 2:
            result = zClass_World::AddChildAtGrid(
                parent,
                child
            );
            break;
        case 5:
            result = zClass_Object3D::gwObject3DAddChild(
                parent,
                child
            );
            break;
        case 1:
            result = zClass_Camera::gwCameraAddChild(
                parent,
                child
            );
            break;
        case 6:
            result = zClass_Lod::gwLodAddChild(
                parent,
                child
            );
            break;
        case 8:
            result = zClass_Animate::AddChild(
                parent,
                child
            );
            break;
        case 3:
        case 4:
        case 9:
        case 10:
            result = zClass_Class::AddChildGeneric(
                parent,
                child
            );
            break;
        case 11:
            result = zClass_Class::AddChildValidated(
                parent,
                child
            );
            break;
        case 7:
            sprintf(
                g_zError_DebugMsgBuffer,
                "%s: Line %d: ERROR: Please use dedicated function "
                "gwSequenceAddChild() for node: %s\n",
                kClassSourceFile,
                0x687,
                parent->name
            );
            zError::EmitDebugBuffer(1);
            result = 1;
            break;
        default:
            sprintf(
                g_zError_DebugMsgBuffer,
                "%s: Line %d: ERROR: Unrecognized node class type for node: %s\n",
                kClassSourceFile,
                0x69f,
                parent->name
            );
            zError::EmitDebugBuffer(1);
            result = 1;
            break;
        }

        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.addchildgeneric
     * @recoil-artifact defines .text recoil:function:0x4484d0: zClass_Class::AddChildGeneric.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Class.c.
     * Purpose: append child and parent references to the generic listB/listA
     * node-link arrays and queue parent transform/bounds updates.
     */
    int __fastcall AddChildGeneric(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ) {
        const int newChildCount = parent->listCountB + 1;
        parent->listB = (zClass_NodePartial **)(realloc(
            parent->listB,
            (size_t)(newChildCount) * sizeof(parent->listB[0])
        ));
        parent->listB[parent->listCountB] = child;
        parent->listCountB = newChildCount;

        const int newParentCount = child->listCountA + 1;
        child->listA = (zClass_NodePartial **)(realloc(
            child->listA,
            (size_t)(newParentCount) * sizeof(child->listA[0])
        ));
        child->listA[child->listCountA] = parent;
        child->listCountA = newParentCount;
        if (newParentCount > 1) {
            SetSingleParentFlagRecursive(
                child,
                0
            );
        }

        parent->boundsFlags |= kBoundsDirtyFlag;
        if ((parent->flags & kTypeListInsertedFlag) == 0) {
            zClass_TypeList::Insert(
                kQueuedTreeBucket,
                parent
            );
            parent->flags |= kTypeListInsertedFlag;
        }
        parent->flags |= kTransformQueuedFlag;

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.removechild
     * @recoil-artifact defines .text recoil:function:0x448570: zClass_Class::RemoveChild.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Class.c.
     * Purpose: dispatch child removal by parent classId across the data-driven
     * zClass node subsystem.
     */
    int __fastcall RemoveChild(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ) {
        if (parent == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x713,
                "Null node pointer."
            );
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x714,
                "Null node pointer."
            );
            return 5;
        }

        int result;
        switch (parent->classId) {
        case 2:
            result = zClass_World::RemoveChildAtGrid(
                parent,
                child
            );
            break;
        case 5:
            result = zClass_Object3D::RemoveChild(
                parent,
                child
            );
            break;
        case 9:
            result = zClass_Light::RemoveChild(
                parent,
                child
            );
            break;
        case 10:
            result = zClass_Sound::RemoveChild(
                parent,
                child
            );
            break;
        case 1:
            result = zClass_Camera::gwCameraRemoveChild(
                parent,
                child
            );
            break;
        case 3:
            result = zClass::RemoveChildChecked(
                parent,
                child
            );
            break;
        case 4:
            result = zClass_Display::RemoveChild(
                parent,
                child
            );
            break;
        case 6:
            result = zClass_Lod::RemoveChild(
                parent,
                child
            );
            break;
        case 7:
            result = zClass_Sequence::RemoveChild(
                parent,
                child
            );
            break;
        case 8:
            result = zClass_Animate::RemoveChild(
                parent,
                child
            );
            break;
        case 11:
            result = zClass_Class::RemoveChildValidated(
                parent,
                child
            );
            break;
        default:
            sprintf(
                g_zError_DebugMsgBuffer,
                "%s: Line %d: ERROR: Unrecognized node class type for node: %s\n",
                kClassSourceFile,
                0x748,
                parent->name
            );
            zError::EmitDebugBuffer(1);
            return 1;
        }

        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.removechildgeneric
     * @recoil-artifact defines .text recoil:function:0x448660: zClass_Class::RemoveChildGeneric.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Class.c.
     * Purpose: remove matching child and parent references from generic
     * listB/listA node-link arrays and queue parent transform/bounds updates.
     */
    int __fastcall RemoveChildGeneric(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ) {
        int childIndex = -1;
        for (int i = 0; i < parent->listCountB; ++i) {
            if (parent->listB[i] == child) {
                childIndex = i;
                break;
            }
        }

        if (childIndex < 0) {
            zError::ReportOld(
                0x200,
                kClassSourceFile,
                0x79c,
                "ERROR deleting child node %s from parent node %s",
                child,
                parent
            );
        }
        if (childIndex >= 0) {
            for (int i = childIndex; i < parent->listCountB - 1; ++i) {
                parent->listB[i] = parent->listB[i + 1];
            }
            --parent->listCountB;
        }

        int parentIndex = -1;
        const int parentCount = child->listCountA;
        for (int i_1261 = 0; i_1261 < parentCount; ++i_1261) {
            if (child->listA[i_1261] == parent) {
                parentIndex = i_1261;
                break;
            }
        }

        if (parentIndex >= 0) {
            for (int i = parentIndex; i < child->listCountA - 1; ++i) {
                child->listA[i] = child->listA[i + 1];
            }
            --child->listCountA;
            if (child->listCountA == 1 && (parent->flags & kSingleParentFlag) != 0) {
                SetSingleParentFlagRecursive(
                    child,
                    1
                );
            }
        }

        parent->boundsFlags |= kBoundsDirtyFlag;
        if ((parent->flags & kTypeListInsertedFlag) == 0) {
            zClass_TypeList::Insert(
                kQueuedTreeBucket,
                parent
            );
            parent->flags |= kTypeListInsertedFlag;
        }
        parent->flags |= kTransformQueuedFlag;

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodegetbbox
     * @recoil-artifact defines .text recoil:function:0x448760: zClass_Class::gwNodeGetBBox.
     * Purpose: copy the cached node bounding box when it is currently valid.
     */
    int __fastcall gwNodeGetBBox(
        zClass_NodePartial * node,
        zBBox3f * outBBox
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x7f9,
                "Null node pointer."
            );
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x7fa,
                "Null class data pointer"
            );
            return 5;
        }
        if ((node->flags & 0x100) == 0) {
            return 1;
        }

        memcpy(
            outBBox,
            (const zBBox3f *)(node->cachedBounds),
            sizeof(*outBBox)
        );
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodegetworldbboxcorners
     * @recoil-artifact defines .text recoil:function:0x4487c0: zClass_Class::gwNodeGetWorldBBoxCorners.
     * Purpose: return cached bounds corners in world/node space for object,
     * camera, animate, and untransformed node classes.
     */
    int __fastcall gwNodeGetWorldBBoxCorners(
        zClass_NodePartial * node,
        zBBoxCorners * outCorners
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x81b,
                "Null node pointer."
            );
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x81c,
                "Null class data pointer"
            );
            return 5;
        }
        if ((node->flags & 0x100) == 0) {
            return 1;
        }

        const zBBox3f *bbox = (const zBBox3f *)(node->cachedBounds);
        if (node->classId == 5) {
            const zClass_Object3DDataPartial *objectData =
                (const zClass_Object3DDataPartial *)(node->classData);
            if ((objectData->flags & 0x08) == 0) {
                zMath_Mat_TransformBBoxToCorners(
                    (const zMat4x3 *)(objectData->localMatrix),
                    bbox,
                    outCorners
                );
                return 0;
            }
        } else if (node->classId == 1) {
            const zClass_CameraDataPartial *cameraData =
                (const zClass_CameraDataPartial *)(node->classData);
            zMath_Mat_TransformBBoxToCorners(
                &((const zClass_CameraBBoxQueryDataPartial *)(cameraData))
                    ->viewOverlay.cachedViewMatrix,
                bbox,
                outCorners
            );
            return 0;
        } else if (node->classId == 8) {
            const zClass_AnimateDataPartial *animateData =
                (const zClass_AnimateDataPartial *)(node->classData);
            if ((node->flags & 0x04) != 0 && (animateData->statusFlags & 0x04) != 0) {
                zMath_Mat_TransformBBoxToCorners(
                    (const zMat4x3 *)(animateData->animatedTransform),
                    bbox,
                    outCorners
                );
                return 0;
            }
        }

        float *out = outCorners->values;
        out[0] = bbox->minX; out[1] = bbox->minY; out[2] = bbox->maxZ;
        out[3] = bbox->maxX; out[4] = bbox->minY; out[5] = bbox->maxZ;
        out[6] = bbox->maxX; out[7] = bbox->minY; out[8] = bbox->minZ;
        out[9] = bbox->minX; out[10] = bbox->minY; out[11] = bbox->minZ;
        out[12] = bbox->minX; out[13] = bbox->maxY; out[14] = bbox->maxZ;
        out[15] = bbox->maxX; out[16] = bbox->maxY; out[17] = bbox->maxZ;
        out[18] = bbox->maxX; out[19] = bbox->maxY; out[20] = bbox->minZ;
        out[21] = bbox->minX; out[22] = bbox->maxY; out[23] = bbox->minZ;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodegetviewbboxcorners
     * @recoil-artifact defines .text recoil:function:0x448920: zClass_Class::gwNodeGetViewBBoxCorners.
     * Purpose: return cached bounds corners after combining the active view
     * transform with any class-specific node transform.
     */
    int __fastcall gwNodeGetViewBBoxCorners(
        zClass_NodePartial * node,
        zBBoxCorners * outCorners
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x85f,
                "Null node pointer."
            );
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x860,
                "Null class data pointer"
            );
            return 5;
        }
        if ((node->flags & 0x100) == 0) {
            return 1;
        }

        int returnCode = 0;
        int currentIsIdentity = zMath_Mat_IsCurrentIdentity();
        zMat4x3 *currentMatrix = zMath_Mat_GetCurrent();
        int skipTransform = 0;
        const zMat4x3 *nodeMatrix = 0;

        switch (node->classId) {
        case 1: {
            const zClass_CameraDataPartial *cameraData =
                (const zClass_CameraDataPartial *)(node->classData);
            nodeMatrix = &((const zClass_CameraBBoxQueryDataPartial *)(cameraData))
                ->viewOverlay.cachedViewMatrix;
            break;
        }
        case 2:
        case 6:
        case 7:
            skipTransform = 1;
            break;
        case 5: {
            const zClass_Object3DDataPartial *objectData =
                (const zClass_Object3DDataPartial *)(node->classData);
            skipTransform = (objectData->flags >> 3) & 0x01;
            nodeMatrix = (const zMat4x3 *)(objectData->localMatrix);
            break;
        }
        case 8: {
            const zClass_AnimateDataPartial *animateData =
                (const zClass_AnimateDataPartial *)(node->classData);
            if ((node->flags & 0x04) == 0 ||
                (animateData->statusFlags & 0x04) == 0) {
                skipTransform = 1;
            }
            nodeMatrix = (const zMat4x3 *)(animateData->animatedTransform);
            break;
        }
        case 9:
        case 10:
            break;
        default:
            returnCode = 3;
            break;
        }

        if (currentMatrix == 0) {
            currentIsIdentity = 1;
        }
        if (nodeMatrix == 0) {
            skipTransform = 1;
        }

        const zBBox3f *bbox = (const zBBox3f *)(node->cachedBounds);
        zMat4x3 combinedMatrix = {0};
        if (currentIsIdentity != 0) {
            if (skipTransform != 0) {
                float *out = outCorners->values;
                out[0] = bbox->minX; out[1] = bbox->minY; out[2] = bbox->maxZ;
                out[3] = bbox->maxX; out[4] = bbox->minY; out[5] = bbox->maxZ;
                out[6] = bbox->maxX; out[7] = bbox->minY; out[8] = bbox->minZ;
                out[9] = bbox->minX; out[10] = bbox->minY; out[11] = bbox->minZ;
                out[12] = bbox->minX; out[13] = bbox->maxY; out[14] = bbox->maxZ;
                out[15] = bbox->maxX; out[16] = bbox->maxY; out[17] = bbox->maxZ;
                out[18] = bbox->maxX; out[19] = bbox->maxY; out[20] = bbox->minZ;
                out[21] = bbox->minX; out[22] = bbox->maxY; out[23] = bbox->minZ;
                return returnCode;
            }
            combinedMatrix = *nodeMatrix;
        } else if (skipTransform != 0) {
            combinedMatrix = *currentMatrix;
        } else {
            combinedMatrix.xx = currentMatrix->xx * nodeMatrix->xx + currentMatrix->yx * nodeMatrix->xy + currentMatrix->zx * nodeMatrix->xz;
            combinedMatrix.yx = currentMatrix->xx * nodeMatrix->yx + currentMatrix->yx * nodeMatrix->yy + currentMatrix->zx * nodeMatrix->yz;
            combinedMatrix.zx = currentMatrix->xx * nodeMatrix->zx + currentMatrix->yx * nodeMatrix->zy + currentMatrix->zx * nodeMatrix->zz;
            combinedMatrix.xy = currentMatrix->xy * nodeMatrix->xx + currentMatrix->yy * nodeMatrix->xy + currentMatrix->zy * nodeMatrix->xz;
            combinedMatrix.yy = currentMatrix->xy * nodeMatrix->yx + currentMatrix->yy * nodeMatrix->yy + currentMatrix->zy * nodeMatrix->yz;
            combinedMatrix.zy = currentMatrix->xy * nodeMatrix->zx + currentMatrix->yy * nodeMatrix->zy + currentMatrix->zy * nodeMatrix->zz;
            combinedMatrix.xz = currentMatrix->xz * nodeMatrix->xx + currentMatrix->yz * nodeMatrix->xy + currentMatrix->zz * nodeMatrix->xz;
            combinedMatrix.yz = currentMatrix->xz * nodeMatrix->yx + currentMatrix->yz * nodeMatrix->yy + currentMatrix->zz * nodeMatrix->yz;
            combinedMatrix.zz = currentMatrix->xz * nodeMatrix->zx + currentMatrix->yz * nodeMatrix->zy + currentMatrix->zz * nodeMatrix->zz;
            combinedMatrix.posX = currentMatrix->xx * nodeMatrix->posX + currentMatrix->yx * nodeMatrix->posY + currentMatrix->zx * nodeMatrix->posZ + currentMatrix->posX;
            combinedMatrix.posY = currentMatrix->xy * nodeMatrix->posX + currentMatrix->yy * nodeMatrix->posY + currentMatrix->zy * nodeMatrix->posZ + currentMatrix->posY;
            combinedMatrix.posZ = currentMatrix->xz * nodeMatrix->posX + currentMatrix->yz * nodeMatrix->posY + currentMatrix->zz * nodeMatrix->posZ + currentMatrix->posZ;
        }

        zMath_Mat_TransformBBoxToCorners(
            &combinedMatrix,
            bbox,
            outCorners
        );
        return returnCode;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodeupdate
     * @recoil-artifact defines .text recoil:function:0x448cc0: zClass_Class::gwNodeUpdate.
     * Purpose: process pending transform and bounds work for one scene node
     * and run class-specific camera, world, object, and animate updates.
     */
    int __fastcall gwNodeUpdate(zClass_NodePartial * node) {
        int result = 0;
        bool needsBBoxRecalc = false;
        const zVec3 unitScale = {1.0f, 1.0f, 1.0f};

        if ((node->boundsFlags & 0x01) != 0) {
            gwNodeUpdateDisplayInstance(node);
            needsBBoxRecalc = true;
        }
        if ((node->boundsFlags & 0x02) != 0) {
            gwNodeComputeChildBBox(node);
            needsBBoxRecalc = true;
        }
        node->boundsFlags &= 0x04;

        zClass_NodePartial *nodeValue = node;

        switch (node->classId) {
        case 1: {
            zClass_CameraDataPartial *cameraData = (zClass_CameraDataPartial *)(node->classData);
            if (cameraData != 0 && (cameraData->cameraFlags & 0x04) != 0) {
                if ((cameraData->cameraFlags & 0x02) == 0) {
                    zMath::MatStackPushPtr(
                        (float *)(&((zClass_CameraBBoxQueryDataPartial *)(cameraData))
                            ->viewOverlay.cachedViewMatrix)
                    );
                    zMath::MatLoadIdentity();
                    zMath::MatApplyLocalTRS(
                        &cameraData->posOffset,
                        &cameraData->targetOrEuler,
                        &unitScale
                    );
                    zMath::MatStackPopPtr();
                }
                gwNodeRecalcBBox(node);
                cameraData->cameraFlags &= ~0x04;
                needsBBoxRecalc = false;
            }
            break;
        }
        case 5: {
            zClass_Object3DDataPartial *objectData =
                (zClass_Object3DDataPartial *)(node->classData);
            if (objectData != 0 && (objectData->flags & 0x01) != 0) {
                if ((objectData->flags & 0x10) == 0) {
                    zMath::MatStackPushPtr(objectData->localMatrix);
                    zMath::MatLoadIdentity();
                    zMath::MatApplyLocalTRS(
                        &objectData->rotation,
                        (zVec3 *)(&objectData->localMatrix[9]),
                        &objectData->scale
                    );
                    zMath::MatStackPopPtr();
                }
                gwNodeRecalcBBox(node);
                objectData->flags &= ~0x01;
                needsBBoxRecalc = false;
            }
            break;
        }
        case 6:
        case 7:
            break;
        case 8: {
            zClass_AnimateDataPartial *animateData = (zClass_AnimateDataPartial *)(node->classData);
            if ((node->flags & 0x04) != 0 && (animateData->statusFlags & 0x04) != 0 &&
                animateData->flags != 0) {
                if ((animateData->flags & 0x01) != 0) {
                    zMath::MatStackPushPtr(animateData->animatedTransform);
                    zMath::MatLoadIdentity();
                    zMath::MatApplyLocalTRS(
                        &animateData->runtime.sampledRotation,
                        &animateData->runtime.sampledPosition,
                        &animateData->runtime.sampledScale
                    );
                    zMath::MatStackPopPtr();
                    gwNodeRecalcBBox(node);
                    needsBBoxRecalc = false;
                }
                animateData->flags = 0;
            }
            break;
        }
        case 2:
            zClass_World::ApplyPendingFogSettings(node);
            break;
        default:
            zError::ReportOld(
                0x200,
                kClassSourceFile,
                0x99e,
                "gwNodeUpdate(): Unrecognized node class type:\n  node = %s class_type = %d\n",
                node,
                node->classId
            );
            result = 3;
            break;
        }

        if (needsBBoxRecalc) {
            gwNodeRecalcBBox(node);
        }
        node->flags &= ~kTransformQueuedFlag;
        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnoderecalcbbox
     * @recoil-artifact defines .text recoil:function:0x448e90: zClass_Class::gwNodeRecalcBBox.
     * Purpose: select or merge primary and child bounds, cache the result, and
     * propagate parent/world-grid bounds updates.
     */
    int __fastcall gwNodeRecalcBBox(zClass_NodePartial * node) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0x9d0,
                "Null node pointer."
            );
            return 5;
        }
        if (node->classId == 2) {
            return 0;
        }

        zBBox3f merged = {0};
        const zBBox3f *bboxSource = 0;
        const bool hasPrimaryBBox = (node->flags & 0x200) != 0;
        const bool hasChildBBox = (node->flags & 0x400) != 0;
        zClass_NodeFreeListSlot *nodeSlot = (zClass_NodeFreeListSlot *)(node);
        const zBBox3f *primaryBBox = hasPrimaryBBox ? &nodeSlot->primaryBounds : 0;
        const zBBox3f *secondaryBBox = hasChildBBox ? &nodeSlot->secondaryBounds : 0;
        if (hasPrimaryBBox && hasChildBBox) {
            merged.minX = primaryBBox->minX < secondaryBBox->minX ? primaryBBox->minX : secondaryBBox->minX;
            merged.minY = primaryBBox->minY < secondaryBBox->minY ? primaryBBox->minY : secondaryBBox->minY;
            merged.minZ = primaryBBox->minZ < secondaryBBox->minZ ? primaryBBox->minZ : secondaryBBox->minZ;
            merged.maxX = primaryBBox->maxX > secondaryBBox->maxX ? primaryBBox->maxX : secondaryBBox->maxX;
            merged.maxY = primaryBBox->maxY > secondaryBBox->maxY ? primaryBBox->maxY : secondaryBBox->maxY;
            merged.maxZ = primaryBBox->maxZ > secondaryBBox->maxZ ? primaryBBox->maxZ : secondaryBBox->maxZ;
            bboxSource = &merged;
        } else if (hasPrimaryBBox) {
            bboxSource = primaryBBox;
        } else if (hasChildBBox) {
            bboxSource = secondaryBBox;
        } else {
            node->flags &= ~0x100;
            return 0;
        }

        node->flags |= 0x100;
        memcpy(node->cachedBounds, bboxSource, sizeof(*bboxSource));
        node->boundsFlags |= 0x04;

        bool worldRectComputed = false;
        float minX = 0.0f;
        float maxX = 0.0f;
        float minZ = 0.0f;
        float maxZ = 0.0f;
        for (int i = 0; i < node->listCountA; ++i) {
            zClass_NodePartial *parent = node->listA[i];
            if (parent->classId == 2) {
                if (!worldRectComputed) {
                    zBBoxCorners corners = {0};
                    gwNodeGetWorldBBoxCorners(
                    node,
                        &corners
                    );
                    minX = maxX = corners.values[0];
                    minZ = maxZ = corners.values[2];
                    for (int cornerIndex = 1; cornerIndex < 8; ++cornerIndex) {
                        const float *corner = &corners.values[cornerIndex * 3];
                        if (corner[0] < minX) minX = corner[0];
                        else if (corner[0] > maxX) maxX = corner[0];
                        if (corner[2] < minZ) minZ = corner[2];
                        else if (corner[2] > maxZ) maxZ = corner[2];
                    }
                    worldRectComputed = true;
                }

                int gridCol = -1;
                int gridRow = -1;
                if ((node->flags & 0x80) == 0) {
                zClass_World::WorldRectToGridIndex(
                    parent,
                    &gridCol,
                    minX,
                    maxX,
                    minZ,
                    maxZ,
                    &gridRow
                );
                }

                if (gridCol == node->gridCol && gridRow == node->gridRow) {
                    if (node->gridCol >= 0 && node->gridRow >= 0) {
                    zClass_World::EnsureGridCellDisplayPosition(
                        parent,
                        node->gridCol,
                        node->gridRow
                    );
                    }
                } else {
                    zClass_World::RemoveChildAtGrid(
                        parent,
                        node
                    );
                    zClass_World::AddChildToGridCell(
                        parent,
                        node,
                        gridCol,
                        gridRow
                    );
                }
            } else {
                parent->boundsFlags |= 0x02;
                if ((parent->flags & 0x01) == 0) {
                    zClass_TypeList::InsertChildNodes(
                        kQueuedTreeBucket,
                        parent
                    );
                    parent->flags |= 0x01;
                }
                parent->flags |= 0x02;
            }
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodecomputechildbbox
     * @recoil-artifact defines .text recoil:function:0x4491b0: zClass_Class::gwNodeComputeChildBBox.
     * Purpose: merge valid child world-bounds corners into the node's
     * secondary bounding box.
     */
    int __fastcall gwNodeComputeChildBBox(zClass_NodePartial * node) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0xaa3,
                "Null node pointer."
            );
            return 5;
        }

        node->flags &= ~0x400;
        if (node->listCountB == 0 || node->classId == 2) {
            return 0;
        }

        zBBoxCorners corners = {0};
        int childIndex = 0;
        for (; childIndex < node->listCountB; ++childIndex) {
            zClass_NodePartial *child = node->listB[childIndex];
            if ((child->flags & 0x100) == 0) {
                continue;
            }

            gwNodeGetWorldBBoxCorners(
                child,
                &corners
            );
            zBBox3f *childBBox = &((zClass_NodeFreeListSlot *)(node))->secondaryBounds;
            childBBox->minX = corners.values[0];
            childBBox->minY = corners.values[1];
            childBBox->minZ = corners.values[2];
            childBBox->maxX = corners.values[0];
            childBBox->maxY = corners.values[1];
            childBBox->maxZ = corners.values[2];
            node->flags |= 0x400;

            for (int cornerIndex = 1; cornerIndex < 8; ++cornerIndex) {
                const float *corner = &corners.values[cornerIndex * 3];
                if (corner[0] < childBBox->minX) {
                    childBBox->minX = corner[0];
                } else if (corner[0] > childBBox->maxX) {
                    childBBox->maxX = corner[0];
                }
                if (corner[1] < childBBox->minY) {
                    childBBox->minY = corner[1];
                } else if (corner[1] > childBBox->maxY) {
                    childBBox->maxY = corner[1];
                }
                if (corner[2] < childBBox->minZ) {
                    childBBox->minZ = corner[2];
                } else if (corner[2] > childBBox->maxZ) {
                    childBBox->maxZ = corner[2];
                }
            }
            ++childIndex;
            break;
        }

        if ((node->flags & 0x400) == 0) {
            return 0;
        }

        for (; childIndex < node->listCountB; ++childIndex) {
            zClass_NodePartial *child = node->listB[childIndex];
            if ((child->flags & 0x100) == 0) {
                continue;
            }

            gwNodeGetWorldBBoxCorners(
                child,
                &corners
            );
            zBBox3f *childBBox = &((zClass_NodeFreeListSlot *)(node))->secondaryBounds;
            for (int cornerIndex = 0; cornerIndex < 8; ++cornerIndex) {
                const float *corner = &corners.values[cornerIndex * 3];
                if (corner[0] < childBBox->minX) {
                    childBBox->minX = corner[0];
                } else if (corner[0] > childBBox->maxX) {
                    childBBox->maxX = corner[0];
                }
                if (corner[1] < childBBox->minY) {
                    childBBox->minY = corner[1];
                } else if (corner[1] > childBBox->maxY) {
                    childBBox->maxY = corner[1];
                }
                if (corner[2] < childBBox->minZ) {
                    childBBox->minZ = corner[2];
                } else if (corner[2] > childBBox->maxZ) {
                    childBBox->maxZ = corner[2];
                }
            }
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodeupdatedisplayinstance
     * @recoil-artifact defines .text recoil:function:0x449420: zClass_Class::gwNodeUpdateDisplayInstance.
     * Purpose: rebuild display-instance bounds into the node primary box and
     * update the primary-bounds-valid flag.
     */
    int __fastcall gwNodeUpdateDisplayInstance(zClass_NodePartial * node) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0xb31,
                "Null node pointer."
            );
            return 5;
        }

        zDiPartial *di = (zDiPartial *)((unsigned int)(node->userDataOrDiRef));
        if (di != 0) {
            zDi::RebuildBounds(
                di,
                (zBoundsMinMaxPartial *)(&((zClass_NodeFreeListSlot *)(node))->primaryBounds)
            );
            node->flags |= 0x200;
        } else {
            node->flags &= ~0x200;
        }

        return 0;
    }

    /**
     * Source-shape note: the definition is emitted by cls_util.c; Class.c
     * retains callers and the public declaration.
     */
    /**
     * Source-shape note: the complete definition is emitted by Switch.c;
     * Class.c retains callers and the public declaration.
     */
    /**
     * Source-shape note: the complete definition is emitted by Switch.c;
     * Class.c retains callers and the public declaration.
     */

}

namespace gwNode {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.buildnodetoancestormatrix
     * @recoil-artifact defines .text recoil:function:0x449480: gwNode::BuildNodeToAncestorMatrix
     * Purpose: apply a node's parent-chain transforms into the current matrix.
     */
    int __fastcall BuildNodeToAncestorMatrix(
        zClass_NodePartial * node,
        int matMode
    ) {
        zVec3 unitScale = {1.0f, 1.0f, 1.0f};
        zVec3 zeroAngles = {0};

        if (node == 0) {
            zError::ReportOld(
                0x400,
                kClassSourceFile,
                0xb66,
                "Null node pointer."
            );
            return 5;
        }

        if (node->classId == 5 && (node->flags & kSingleParentFlag) != 0) {
            zClass_Object3DDataPartial *objectData =
                (zClass_Object3DDataPartial *)(node->classData);
            if ((objectData->flags & 0x20) == 0) {
                zMath::MatLoadCurrentFrom((const zMat4x3 *)(objectData->cachedWorldMatrix));
                return 0;
            }
        }

        zClass_NodePartial *parentChain[15] = {0};
        int chainCount = 1;
        parentChain[0] = node;
        zClass_NodePartial *current = node;
        while (current != 0) {
            if (current->listCountA > 1) {
                zError::ReportOld(
                    0x800,
                    kClassSourceFile,
                    0xb80,
                    "node has multiple parents; count = %d.\n  node = %s class_type = %d\n",
                    current->listCountA,
                    current,
                    current->classId
                );
                return 1;
            }
            if (current->listCountA != 1) {
                break;
            }
            current = current->listA[0];
            if (current == 0) {
                break;
            }
            parentChain[chainCount++] = current;
        }

        for (int i = 0; i < chainCount; ++i) {
            zClass_NodePartial *chainNode = parentChain[i];
            if (chainNode->classId != 2 && (chainNode->flags & 0x01) != 0) {
                UpdateTree(chainNode);
                break;
            }
        }

        for (int i_1435 = chainCount - 1; i_1435 >= 0; --i_1435) {
            zClass_NodePartial *ancestor = parentChain[i_1435];
            const int ancestorFlags = ancestor->flags & ~kNodeTransformDirtyPropagatedFlag;
            ancestor->flags = ancestorFlags;
            switch (ancestor->classId) {
            case 5: {
                zClass_Object3DDataPartial *objectData =
                    (zClass_Object3DDataPartial *)(ancestor->classData);
                const int objectFlags = objectData->flags;
                if ((objectFlags & 0x08) == 0) {
                    if ((ancestorFlags & kSingleParentFlag) != 0) {
                        if ((objectFlags & 0x20) != 0) {
                            zMath::MatMultiply(
                                (const zMat4x3 *)(objectData->localMatrix),
                                matMode
                            );
                            zMat4x3 currentMatrix;
                            zMath::MatCopyCurrentTo(&currentMatrix);
                            memcpy(
                                objectData->cachedWorldMatrix,
                                &currentMatrix,
                                sizeof(currentMatrix)
                            );
                            objectData->flags &= ~0x20;
                        } else {
                            zMath::MatLoadCurrentFrom(
                                (const zMat4x3 *)(objectData->cachedWorldMatrix)
                            );
                        }
                    } else {
                        zMath::MatMultiply(
                            (const zMat4x3 *)(objectData->localMatrix),
                            matMode
                        );
                    }
                } else if ((ancestorFlags & kSingleParentFlag) != 0 && (objectFlags & 0x20) != 0) {
                    zMat4x3 currentMatrix;
                    zMath::MatCopyCurrentTo(&currentMatrix);
                    memcpy(
                        objectData->cachedWorldMatrix,
                        &currentMatrix,
                        sizeof(currentMatrix)
                    );
                    objectData->flags &= ~0x20;
                }
                break;
            }
            case 2:
            case 6:
                break;
            case 9: {
                zClass_LightDataPartial *lightData =
                    (zClass_LightDataPartial *)(ancestor->classData);
                zMath::MatApplyLocalTRS(
                    &lightData->localRotation,
                    &lightData->localPosition,
                    &unitScale
                );
                break;
            }
            case 10: {
                zClass_SoundDataPartial *soundData =
                    (zClass_SoundDataPartial *)(ancestor->classData);
                zMath::MatApplyLocalTRS(
                    &zeroAngles,
                    &soundData->localPosition,
                    &unitScale
                );
                break;
            }
            case 1: {
                zClass_CameraDataPartial *cameraData =
                    (zClass_CameraDataPartial *)(ancestor->classData);
                if ((cameraData->cameraFlags & 0x02) == 0) {
                    zMath::MatApplyLocalTRS(
                        &cameraData->posOffset,
                        &cameraData->targetOrEuler,
                        &unitScale
                    );
                } else {
                    zMath::MatMultiply(
                        &((zClass_CameraBBoxQueryDataPartial *)(cameraData))
                            ->viewOverlay.cachedViewMatrix,
                        1
                    );
                }
                break;
            }
            case 8: {
                zClass_AnimateDataPartial *animateData =
                    (zClass_AnimateDataPartial *)(ancestor->classData);
                if ((ancestorFlags & 0x04) != 0 &&
                    (animateData->statusFlags & 0x04) != 0) {
                    zMath::MatMultiply(
                        (const zMat4x3 *)(animateData->animatedTransform),
                        matMode
                    );
                }
                break;
            }
            default:
                sprintf(
                    g_zError_DebugMsgBuffer,
                    "%s: Line %d: gwNodeBuildNodeToAncestorMatrix(): Unrecognized node "
                    "class type:\n",
                    kClassSourceFile,
                    0xbfa
                );
                sprintf(
                    g_zError_DebugMsgBuffer + strlen(g_zError_DebugMsgBuffer),
                    "  node = %s class_type = %d\n",
                    ancestor->name,
                    ancestor->classId
                );
                zError::EmitDebugBuffer(3);
                return 3;
            }
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.getworldposition
     * @recoil-artifact defines .text recoil:function:0x4497b0: gwNode::GetWorldPosition
     * Purpose: resolve a node's world-space translation into the output vector.
     */
    int __fastcall GetWorldPosition(
        zClass_NodePartial * node,
        zVec3 * outPosition
    ) {
        if (node == 0) {
            return 1;
        }

        if (node->classId == 5 && (node->flags & kSingleParentFlag) != 0) {
            zClass_Object3DDataPartial *objectData =
                (zClass_Object3DDataPartial *)(node->classData);
            if ((objectData->flags & 0x20) == 0) {
                outPosition->x = objectData->cachedWorldMatrix[9];
                outPosition->y = objectData->cachedWorldMatrix[10];
                outPosition->z = objectData->cachedWorldMatrix[11];
                return 0;
            }
        }

        outPosition->x = 0.0f;
        outPosition->y = 0.0f;
        outPosition->z = 0.0f;

        float matrix[12];
        zMath::MatStackPushPtr(matrix);
        zMath::MatLoadIdentity();
        BuildNodeToAncestorMatrix(
            node,
            1
        );
        outPosition->x = matrix[9];
        outPosition->y = matrix[10];
        outPosition->z = matrix[11];
        zMath::MatStackPopPtr();
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.transformpoint
     * @recoil-artifact defines .text recoil:function:0x449850: gwNode::TransformPoint.
     * Purpose: transform a point from node-local space into world space.
     */
    int __fastcall TransformPoint(
        zClass_NodePartial * node,
        zVec3 * point
    ) {
        if (node == 0) {
            return 1;
        }

        if (point->x == 0.0f && point->y == 0.0f && point->z == 0.0f) {
            GetWorldPosition(
                node,
                point
            );
            return 0;
        }

        zMat4x3 matrix = {0};
        zMath::MatStackPushPtr((float *)(&matrix));
        zMath::MatLoadIdentity();
        BuildNodeToAncestorMatrix(
            node,
            1
        );
        zMath::MatTransformPointBatchInPlace(
            point,
            1
        );
        zMath::MatStackPopPtr();
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.getworldposandorientation
     * @recoil-artifact defines .text recoil:function:0x4498e0: gwNode::GetWorldPosAndOrientation.
     * Purpose: compute a node world position and derive orientation angles
     * from transformed basis points.
     */
    int __fastcall GetWorldPosAndOrientation(
        zClass_NodePartial * node,
        zVec3 * inOutPosition,
        zVec3 * outOrientation
    ) {
        zVec3 localOrientationBasis[2] = {{0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}};

        if (node == 0) {
            return 1;
        }

        zMat4x3 matrix = {0};
        zMath::MatStackPushPtr((float *)(&matrix));
        zMath::MatLoadIdentity();
        BuildNodeToAncestorMatrix(
            node,
            1
        );

        if (inOutPosition->x == 0.0f && inOutPosition->y == 0.0f && inOutPosition->z == 0.0f) {
            inOutPosition->x = matrix.posX;
            inOutPosition->y = matrix.posY;
            inOutPosition->z = matrix.posZ;
        } else {
            zMath::MatTransformPointBatchInPlace(
                inOutPosition,
                1
            );
        }

        zVec3 worldPosition = {matrix.posX, matrix.posY, matrix.posZ};
        zVec3 worldOrientationBasis[2];
        memcpy(
            worldOrientationBasis,
            localOrientationBasis,
            sizeof(worldOrientationBasis)
        );
        if (*zMath::g_currentMatrixIdentityFlagSlot == 0) {
            const zMat4x3 *currentMatrix =
                (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
            for (int i = 0; i < 2; ++i) {
                const zVec3 point = localOrientationBasis[i];
                worldOrientationBasis[i].x =
                    point.x * currentMatrix->xx +
                    point.y * currentMatrix->yx +
                    point.z * currentMatrix->zx +
                    currentMatrix->posX;
                worldOrientationBasis[i].y =
                    point.x * currentMatrix->xy +
                    point.y * currentMatrix->yy +
                    point.z * currentMatrix->zy +
                    currentMatrix->posY;
                worldOrientationBasis[i].z =
                    point.x * currentMatrix->xz +
                    point.y * currentMatrix->yz +
                    point.z * currentMatrix->zz +
                    currentMatrix->posZ;
            }
        }

        zMath::MatLoadIdentity();
        zVec3 directionAngles = {0};
        zMath::Vec3DirectionAnglesBetweenPoints(
            &worldPosition,
            &worldOrientationBasis[0],
            &directionAngles
        );
        outOrientation->x = directionAngles.x;
        outOrientation->y = directionAngles.y;
        outOrientation->z = directionAngles.z;
        outOrientation->z =
            zMath_Vec3_ElevationAngleBetweenPoints(
                &worldPosition,
                &worldOrientationBasis[1]
            );

        zMath::MatStackPopPtr();
        return 0;
    }
}

namespace zClass_Class {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodegetroot
     * @recoil-artifact defines .text recoil:function:0x449ab0: zClass_Class::gwNodeGetRoot
     * Purpose: walk a node's single-parent chain and return the root node.
     */
    zClass_NodePartial *__fastcall gwNodeGetRoot(zClass_NodePartial * node) {
        zClass_NodePartial *current = node;
        if (current == 0) {
            return 0;
        }

        while (current->listCountA != 0) {
            if (current->listCountA != 1) {
                zError::ReportOld(
                    0x200,
                    kClassSourceFile,
                    0xd0d,
                    "Error getting root node; Multiple parents found.\n  Node: %s\n",
                    current
                );
                return 0;
            }

            current = current->listA[0];
            if (current == 0) {
                return 0;
            }
        }

        return current;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodegetworldchild
     * @recoil-artifact defines .text recoil:function:0x449af0: zClass_Class::gwNodeGetWorldChild.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Class.c.
     * Purpose: walk a node's single-parent chain through listA links and
     * return the child directly owned by the world node.
     */
    zClass_NodePartial *__fastcall gwNodeGetWorldChild(
        zClass_NodePartial * node
    ) {
        zClass_NodePartial *current = node;
        while (current != 0 && current->listCountA != 0) {
            if (current->listCountA != 1) {
                zError::ReportOld(
                    0x200,
                    kClassSourceFile,
                    0xd4e,
                    "Error getting root node; Multiple parents found.\n  Node: %s\n",
                    current
                );
                return 0;
            }

            zClass_NodePartial *parent = current->listA[0];
            if (parent == 0) {
                return 0;
            }
            if (parent->classId == 2) {
                return current;
            }
            current = parent;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.setsingleparentflagrecursive
     * @recoil-artifact defines .text recoil:function:0x449b40: zClass_Class::SetSingleParentFlagRecursive.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Class.c.
     * Purpose: propagate the single-parent flag through a data-driven zClass
     * child subtree when listA ownership count changes.
     */
    int __fastcall SetSingleParentFlagRecursive(
        zClass_NodePartial * node,
        int setFlag
    ) {
        if (node == 0) {
            return 1;
        }

        if (setFlag != 0) {
            if (node->listCountA > 1) {
                return 0;
            }
            node->flags |= kSingleParentFlag;
        } else {
            node->flags &= ~kSingleParentFlag;
        }

        for (int i = 0; i < node->listCountB; ++i) {
            SetSingleParentFlagRecursive(
                node->listB[i],
                setFlag
            );
        }

        return 0;
    }

}

namespace zClass_Node {



    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.setcontextrecursive
     * @recoil-artifact defines .text recoil:function:0x437e60: zClass_Node::SetContextRecursive
     * BN evidence: fastcall self/context, stack flagMask, callbackContext at
     * 0x40, flags at 0x24, signed listCountB at 0x5c, listB at 0x60,
     * recursive self-call only, and no global data references.
     * Purpose: assign a callback context and OR flag bits through a node
     * subtree using the zClass child-list links.
     */
    void __fastcall SetContextRecursive(
        zClass_NodePartial * self,
        zClass_NodePartial * context,
        int flagMask
    ) {
        self->callbackContext = context;
        self->flags |= flagMask;

        for (int i = 0; i < self->listCountB; ++i) {
            SetContextRecursive(
                self->listB[i],
                context,
                flagMask
            );
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.setdiflagbit0recursive
     * @recoil-artifact defines .text recoil:function:0x437ea0: zClass_Node::SetDiFlagBit0Recursive
     * BN evidence: fastcall node/enabled, gwNodeGetUserData for the typed
     * userDataOrDiRef display-instance reference, zDi::SetFlagBit0 when
     * non-null, signed listCountB at 0x5c, listB at 0x60, recursive self-call
     * only, and no global data references.
     * Purpose: set display-instance flag bit 0 for each display instance
     * reachable through a node's child-list subtree.
     */
    void __fastcall SetDiFlagBit0Recursive(
        zClass_NodePartial * node,
        int enabled
    ) {
        unsigned int userData;
        zClass_Class::gwNodeGetUserData(
            node,
            &userData
        );
        zDiPartial *di = (zDiPartial *)(userData);
        if (di != 0) {
            zDi::SetFlagBit0(
                di,
                enabled
            );
        }

        for (int i = 0; i < node->listCountB; ++i) {
            SetDiFlagBit0Recursive(
                node->listB[i],
                enabled
            );
        }
    }

    /*
     * Source-shape routing markers: these definitions are emitted by
     * cls_util.c while Class.c retains related callers.
     */

}
/*
 * Provenance-only routing markers: these definitions compile through the
 * literal-backed Battlesport/player.cpp contribution.
 */
