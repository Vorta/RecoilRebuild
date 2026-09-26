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
 * @recoil-artifact defines .data recoil:data:0x4f4a90: g_CZClass_NodeCount.
 * Unresolved candidate: no retail references prove a node/core shadow block
 * or the individual objects below. Their BN identities and positive tracker
 * gates were withdrawn by the full .data audit. Keep the current source
 * contributions pending an evidence-backed storage/placement correction;
 * zero bytes alone authorize neither these types nor deletion of storage.
 * Purpose: retain this candidate storage without asserting a node-count role.
 */
int g_CZClass_NodeCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-nodetablebase
 * @recoil-artifact defines .data recoil:data:0x4f4a94: g_CZClass_NodeTableBase.
 * Purpose: retain candidate storage; original pointer identity is unresolved.
 */
CZNodePartial* g_CZClass_NodeTableBase = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-nodeactivecount
 * @recoil-artifact defines .data recoil:data:0x4f4a98: g_CZClass_NodeActiveCount.
 * Purpose: retain candidate storage; original count identity is unresolved.
 */
int g_CZClass_NodeActiveCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-copynodeclonematerialrefs
 * @recoil-artifact defines .data recoil:data:0x4f4a9c: g_CZClass_CopyNodeCloneMaterialRefs.
 * Purpose: retain candidate storage; original policy-flag identity is unresolved.
 */
int g_CZClass_CopyNodeCloneMaterialRefs = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-copynodecloneallmaterialsifrelevant
 * @recoil-artifact defines .data recoil:data:0x4f4aa0: g_CZClass_CopyNodeCloneAllMaterialsIfRelevant.
 * Purpose: retain candidate storage; original policy-flag identity is unresolved.
 */
int g_CZClass_CopyNodeCloneAllMaterialsIfRelevant = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-coreinitialized
 * @recoil-artifact defines .data recoil:data:0x4f4aa4: g_CZClass_CoreInitialized.
 * Purpose: retain candidate storage; original initialization-flag identity is unresolved.
 */
int g_CZClass_CoreInitialized = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-lastzbdpath
 * @recoil-artifact defines .data recoil:data:0x4f4aa8: g_CZClass_LastZbdPath.
 * Purpose: retain candidate storage; original buffer identity and extent are unresolved.
 */
char g_CZClass_LastZbdPath[0x30] = { 0 };
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-nodearray
 * @recoil-artifact defines .data recoil:data:0x539c94: g_CZClass_NodeArray.
 * BN evidence: CZClass::Init/ShutdownCore and ZBD node-table helpers reference
 * this global node pool pointer, and Class.c alloc/free paths index through it.
 * Purpose: store the active zClass node-slot array backing runtime scene nodes.
 */
CZNodeFreeListSlot* g_CZClass_NodeArray = 0;
/**
 * BN evidence: Class.c alloc/free paths update this count, while CZClass::Init,
 * ShutdownCore, and ZBD reads reset or recompute it from the node pool.
 * Purpose: count currently allocated nodes in the global zClass node array.
 */
int g_CZClass_ActiveNodeCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-nodefreeheadindex
 * @recoil-artifact defines .data recoil:data:0x4de4c8: g_CZClass_NodeFreeHeadIndex.
 * BN evidence: Class.c alloc/free paths load and store this head index, with
 * CZClass::Init/ShutdownCore and ZBD serialization preserving the free list.
 * Purpose: identify the first free zClass node-slot index or -1 when empty.
 */
int g_CZClass_NodeFreeHeadIndex = -1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-currentzbdpath
 * @recoil-artifact defines .data recoil:data:0x539ca8: g_CZClass_CurrentZbdPath.
 * BN data inventory declares char[0x30] at 0x539ca8.
 * Purpose: store the current ZBD path prefix used by zClass loading.
 */
char g_CZClass_CurrentZbdPath[0x30] = { 0 };
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-maincamera
 * @recoil-artifact defines .data recoil:data:0x4f36bc: g_MainCamera.
 * BN evidence: player, HUD, and play-state camera callers reference this
 * global before CZCamera operations and world-node attachment calls.
 * Purpose: store the current main camera node used by gameplay and rendering.
 */
CZNodePartial* g_MainCamera = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-player-runtimediscene
 * @recoil-artifact defines .data recoil:data:0x4f36b8: g_Player_RuntimeDiScene.
 * Purpose: Stores g Player RuntimeDiScene data used by engine.zclass.player_runtime_di_scene_global.
 */
CZNodePartial* g_Player_RuntimeDiScene = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-renderboundscontextactive
 * @recoil-artifact defines .data recoil:data:0x4ddd28: g_CZClass_RenderBoundsContextActive.
 * BN evidence: camera, sound, light, object, animate, LOD, sequence, and switch
 * render traversals test and bracket this flag while updating bounds contexts.
 * Purpose: mark that rendering is inside a bounds-update traversal context.
 */
int g_CZClass_RenderBoundsContextActive = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-renderfrustumgridtileindex
 * @recoil-artifact defines .data recoil:data:0x4ddd2c: g_CZClass_RenderFrustumGridTileIndex.
 * BN evidence: camera frustum-grid rendering writes this index and object
 * traversal reads it when selecting grid-tile render behavior.
 * Purpose: identify the active frustum-grid tile during camera render passes.
 */
int g_CZClass_RenderFrustumGridTileIndex = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-renderrangefadeactive
 * @recoil-artifact defines .data recoil:data:0x539980: g_CZClass_RenderRangeFadeActive.
 * BN evidence: LOD traversal brackets this flag, and render traversals test it
 * before applying range-fade blend scale to display instances.
 * Purpose: mark that range-fade alpha scaling is active for child renders.
 */
int g_CZClass_RenderRangeFadeActive = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-renderrangefadescale
 * @recoil-artifact defines .data recoil:data:0x539828: g_CZClass_RenderRangeFadeScale.
 * BN evidence: LOD traversal computes this float and camera, sound, light,
 * object, and animate traversals copy it into display-instance blend scale.
 * Purpose: store the current range-fade blend scale for render traversal.
 */
float g_CZClass_RenderRangeFadeScale = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-rendervertexalphaoverrideactive
 * @recoil-artifact defines .data recoil:data:0x539b94: g_CZClass_RenderVertexAlphaOverrideActive.
 * BN evidence: object and LOD render traversals set, test, and clear this flag
 * around vertex-alpha override rendering.
 * Purpose: prevent nested vertex-alpha override state from being applied twice.
 */
int g_CZClass_RenderVertexAlphaOverrideActive = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-renderalphascalestacktop
 * @recoil-artifact defines .data recoil:data:0x4ddd3c: g_CZClass_RenderAlphaScaleStackTop.
 * BN evidence: object and LOD render traversals push and pop this index, then
 * restore zModel render alpha scale from the stack top.
 * Purpose: track the current render alpha-scale stack entry.
 */
int g_CZClass_RenderAlphaScaleStackTop = -1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-renderalphascalestack
 * @recoil-artifact defines .data recoil:data:0x539830: g_CZClass_RenderAlphaScaleStack.
 * BN data inventory declares float[0x10] at 0x539830.
 * Purpose: store nested render alpha scale values for traversal restore.
 */
float g_CZClass_RenderAlphaScaleStack[0x10] = { 0 };
extern char g_CZClass_SourceFile_SwitchC[0x24];
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-softwarepathstatestacktop
 * @recoil-artifact defines .data recoil:data:0x4ddd40: g_CZClass_SoftwarePathStateStackTop.
 * BN evidence: object render traversal pushes and pops this index while
 * restoring software-path color and alpha render state.
 * Purpose: track the current software renderer color/alpha state stack entry.
 */
int g_CZClass_SoftwarePathStateStackTop = -1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-softwarepathrenderstatestack
 * @recoil-artifact defines .data recoil:data:0x539988: g_CZClass_SoftwarePathRenderStateStack.
 * BN data inventory declares a 64-byte stack, matching four color/alpha states.
 * Purpose: store nested software render color and alpha state.
 */
CZRenderColorAlphaState g_CZClass_SoftwarePathRenderStateStack[4] = { 0 };
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-loddistancestatestacktop
 * @recoil-artifact defines .data recoil:data:0x4ddd30: g_CZClass_LodDistanceStateStackTop.
 * BN evidence: LOD traversal and camera/video render setup read, reset, push,
 * and pop this index while computing active LOD distance state.
 * Purpose: track the current LOD distance-state stack entry during rendering.
 */
int g_CZClass_LodDistanceStateStackTop = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.g-zclass-loddistancestatestack
 * @recoil-artifact defines .data recoil:data:0x539900: g_CZClass_LodDistanceStateStack.
 * BN data inventory declares a 64-byte stack, matching four LOD states.
 * Purpose: store nested LOD distance state for render traversal.
 */
CZLodDistanceState g_CZClass_LodDistanceStateStack[4] = { 0 };
}

namespace
{

    const int kQueuedTreeBucket = 7;
    const int kTypeListInsertedFlag = 0x01;
    const int kTransformQueuedFlag = 0x02;
    const int kBoundsDirtyFlag = 0x02;
    const int kSingleParentFlag = 0x00080000;
    const int kNodeVariantGateFlag = 0x01000000;
    const int kNodeTransformDirtyPropagatedFlag = 0x02000000;

    /*
     * BN type evidence: CZCameraData stores a union at 0x80 whose
     * cachedViewMatrix arm is used by node bbox query helpers 0x4487c0 and
     * 0x448920.
     */
    struct CZCameraViewTargetStatePartial {
        unsigned char viewBasis[0x24];
        zVec3 worldTarget;
    };

    union CZCameraViewOverlayPartial {
        CZCameraViewTargetStatePartial targetState;
        zMat4x3 cachedViewMatrix;
    };

    struct CZCameraBBoxQueryDataPartial {
        CZNodePartial* worldNode;
        CZNodePartial* windowNode;
        CZNodePartial* horizonNode;
        CZNodePartial* horizonXZNode;
        int cameraFlags;
        zVec3 targetOrEuler;
        zVec3 posOffset;
        zVec3 worldPos;
        zVec3 eulerAngles;
        zMat4x3 worldTransform;
        zVec3 forwardDir;
        CZCameraViewOverlayPartial viewOverlay;
    };

    RECOIL_STATIC_ASSERT(sizeof(CZCameraViewTargetStatePartial) == 0x30);
    RECOIL_STATIC_ASSERT(sizeof(CZCameraViewOverlayPartial) == 0x30);
    RECOIL_STATIC_ASSERT(offsetof(CZCameraBBoxQueryDataPartial, viewOverlay) == 0x80);

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
    zBBox3f MergeBBoxes(const zBBox3f* a, const zBBox3f* b)
    {
        zBBox3f merged = { 0 };
        merged.min.x = a->min.x < b->min.x ? a->min.x : b->min.x;
        merged.min.y = a->min.y < b->min.y ? a->min.y : b->min.y;
        merged.min.z = a->min.z < b->min.z ? a->min.z : b->min.z;
        merged.max.x = a->max.x > b->max.x ? a->max.x : b->max.x;
        merged.max.y = a->max.y > b->max.y ? a->max.y : b->max.y;
        merged.max.z = a->max.z > b->max.z ? a->max.z : b->max.z;
        return merged;
    }

    /**
     * Original-source helper evidence: no standalone retail function is
     * present; observed in 0x448e90 cached-bounds update logic.
     * Purpose: copy a typed bounding box into the node cached-bounds storage.
     */
    void CopyBBoxToCachedBounds(CZNodePartial * node, const zBBox3f* bbox)
    {
        memcpy(node->cachedBounds, bbox, sizeof(*bbox));
    }
}

namespace CZClass
{
    int __fastcall TryFreeNode(CZNodePartial * node);

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.allocnodefromfreelist
     * @recoil-artifact defines .text recoil:function:0x4478c0: CZClass::gwNodeNew.
     *
     *
     * Purpose: pop a node from the global free list, clear it, and install
     * default active-node state.
     */
    CZNodePartial* __cdecl gwNodeNew()
    {
        const int index = g_CZClass_NodeFreeHeadIndex;
        if (index != -1) {
            CZNodeFreeListSlot* slot = &g_CZClass_NodeArray[index];
            CZNodePartial* node = &slot->node;
            g_CZClass_NodeFreeHeadIndex = (int)(slot->freeTag << 8) >> 8;

            memset(node, 0, offsetof(CZNodeFreeListSlot, freeTag));
            /**
             * BN evidence: gwNodeNew increments this global after
             * clearing a popped node slot from g_CZClass_NodeArray.
             * Purpose: account for the newly active node before type-list use.
             */
            ++g_CZClass_ActiveNodeCount;
            CZTypeList::Insert(6, node);

            node->flags = 0x0108001c;
            node->callbackPriority = 1;
            node->gridCol = -1;
            node->gridRow = -1;
            node->nodeType = 0xff;
            sprintf(node->name, "%s", "Default_node_name");
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
     * @recoil-artifact defines .text recoil:function:0x447980: CZClass::DeleteNodeByType.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Class.c.
     * Purpose: validate node ownership and dispatch deletion by classId.
     */
    int __fastcall DeleteNodeByType(CZNodePartial * node)
    {
        int result; // Case 0 leaves this uninitialized, as reproduced by VC5 byte comparison.
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x231, "Null node pointer.");
            return 5;
        }

        if (node->listCountA > 0) {
            return 1;
        }

        switch (node->classId) {
        case 5:
            result = CZObject3D::DeleteNode(node);
            break;
        case 1:
            result = CZCamera::DeleteNode(node);
            break;
        case 2:
            result = CZWorld::DeleteNode(node);
            break;
        case 3:
            result = CZWindow::DeleteNode(node);
            break;
        case 4:
            result = CZDisplay::DeleteNode(node);
            break;
        case 6:
            result = CZLod::DeleteNode(node);
            break;
        case 7:
            result = CZSequence::DeleteNode(node);
            break;
        case 8:
            result = CZAnimate::DeleteNode(node);
            break;
        case 9:
            result = CZLight::DeleteNode(node);
            break;
        case 10:
            result = CZSound::DeleteNode(node);
            break;
        case 11:
            result = CZSwitch::DeleteNode(node);
            break;
        case 0:
            TryFreeNode(node);
            break;
        default:
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Class.c",
                0x272,
                "ERROR: Unrecognized node class type for node: %s\n",
                node->name
            );
            return 1;
        }
        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.freenodetofreelist
     * @recoil-artifact defines .text recoil:function:0x447a70: CZClass::FreeNodeToFreeList.
     * @recoil-match byte
     *
     * Purpose: release owned node lists/data and return the node slot to the
     * global zClass free-list while preserving the slot free-tag flags.
     */
    int __fastcall FreeNodeToFreeList(CZNodePartial * node)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x28e, "Null node pointer.");
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

        const ptrdiff_t index = (CZNodeFreeListSlot*)(node)-g_CZClass_NodeArray;
        unsigned int* freeTag = &g_CZClass_NodeArray[index].freeTag;
        *freeTag = (*freeTag & 0xff000000) | ((unsigned int)(g_CZClass_NodeFreeHeadIndex) & 0x00ffffff);
        --g_CZClass_ActiveNodeCount;
        g_CZClass_NodeFreeHeadIndex = (int)(index);

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.tryfreenode
     * @recoil-artifact defines .text recoil:function:0x447b60: CZClass::TryFreeNode.
     * @recoil-match byte
     *
     * Purpose: remove a node from active lists, then either free it
     * immediately or enqueue it for deferred freeing.
     */
    int __fastcall TryFreeNode(CZNodePartial * node)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x2f0, "Null node pointer.");
            return 5;
        }

        node->flags &= ~kTransformQueuedFlag;
        CZList::DeleteNodeFromLists(node);
        if (CZClass::ProcessDeferredWork() == 0) {
            FreeNodeToFreeList(node);
        } else {
            CZNodeList::Insert(node);
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.findnoderecursivebyname
     * @recoil-artifact defines .text recoil:function:0x447bc0: CZClass::FindNodeRecursiveByName
     * @recoil-match byte
     *
     * BN caveat: the inlined strcmp-style comparison has a known sbb
     * flag-generation limitation; assembly still proves the typed node-name
     * comparison and forward child recursion.
     * Purpose: search a zClass node subtree by exact node name, returning the
     * first matching node in forward child-list order.
     */
    CZNodePartial* __fastcall FindNodeRecursiveByName(CZNodePartial * root, const char* name)
    {
        if (root == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x33a, "Null node pointer.");
            return 0;
        }

        if (strcmp(root->name, name) == 0) {
            return root;
        }

        for (int i = 0; i < root->listCountB; ++i) {
            CZNodePartial* const childMatch = FindNodeRecursiveByName(root->listB[i], name);
            if (childMatch != 0) {
                return childMatch;
            }
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetactive
     * @recoil-artifact defines .text recoil:function:0x447c60: CZClass::gwNodeSetActive.
     * @recoil-match byte
     *
     * Purpose: toggle the active flag for supported node classes and delegate
     * sound-node activity changes to the sound owner.
     */
    int __fastcall gwNodeSetActive(CZNodePartial * node, int active)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x38d, "Null node pointer.");
            return 5;
        }

        switch (node->classId) {
        case 10:
            CZSound::gwSoundSetActive(node, active);
            return 0;
        case 1:
        case 2:
        case 5:
        case 6:
        case 9:
            if (active == 1) {
                node->flags |= 0x04;
            } else if (active == 0) {
                node->flags &= ~0x04;
            }
            return 0;
        }

        zError::ReportOld(
            0x400,
            "D:\\Proj\\GameZRecoil\\zClass\\Class.c",
            0x3a4,
            "gwNodeSetActive(): Unrecognized node class type:\n  node = %s class_type = %d\n",
            node,
            node->classId
        );
        return 3;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetflag16
     * @recoil-artifact defines .text recoil:function:0x447d20: CZClass::gwNodeSetFlag16
     * @recoil-match byte
     *
     * Purpose: set or clear node flag bit 16.
     */
    int __fastcall gwNodeSetFlag16(CZNodePartial * node, int value)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x3b7, "Null node pointer.");
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
     * @recoil-artifact defines .text recoil:function:0x447d70: CZClass::gwNodeSetFlag17
     * @recoil-match byte
     *
     * Purpose: set or clear node flag bit 17.
     */
    int __fastcall gwNodeSetFlag17(CZNodePartial * node, int value)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x3c6, "Null node pointer.");
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
     * @recoil-artifact defines .text recoil:function:0x447dc0: CZClass::gwNodeSetName
     * @recoil-match byte
     *
     * Purpose: copy or truncate a caller-supplied name into a zClass node's
     * fixed-size name buffer.
     */
    int __fastcall gwNodeSetName(CZNodePartial * node, const char* name)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x3df, "Null node pointer.");
            return 5;
        }

        if (strlen(name) >= sizeof(node->name)) {
            strncpy(node->name, name, 0x22);
            node->name[0x23] = '\0';
        } else {
            sprintf(node->name, "%s", name);
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodegetname
     * @recoil-artifact defines .text recoil:function:0x447e30: CZClass::gwNodeGetName
     * @recoil-match byte
     *
     * Purpose: return the fixed-size name buffer for a zClass node.
     */
    char* __fastcall gwNodeGetName(CZNodePartial * node)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x40d, "Null node pointer.");
            return 0;
        }

        return node->name;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetdisplayinstance
     * @recoil-artifact defines .text recoil:function:0x447e60: CZClass::gwNodeSetDisplayInstance
     * @recoil-match byte
     *
     * Purpose: replace a node's display-instance reference, maintain zDi
     * reference counts, rebuild its bounds, and queue transform updates.
     */
    int __fastcall gwNodeSetDisplayInstance(CZNodePartial * node, zDiPartial * displayInstance)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x424, "Null node pointer.");
            return 5;
        }

        zDiPartial* oldDisplayInstance = (zDiPartial*)((unsigned int)(node->userDataOrDiRef));
        if (oldDisplayInstance != 0) {
            zDi::Release(oldDisplayInstance);
        }

        node->userDataOrDiRef = (unsigned int)((unsigned int)(displayInstance));
        if (displayInstance != 0) {
            zDi::AddRef(displayInstance);
            zDi::RebuildBounds(
                (zDiPartial*)((unsigned int)(node->userDataOrDiRef)),
                (zBoundsMinMaxPartial*)(&((CZNodeFreeListSlot*)node)->primaryBounds)
            );
            node->flags |= 0x200;
        } else {
            node->flags &= ~0x200;
        }

        node->boundsFlags |= 0x01;
        if ((node->flags & kTypeListInsertedFlag) == 0) {
            CZTypeList::Insert(kQueuedTreeBucket, node);
            node->flags |= kTypeListInsertedFlag;
        }
        node->flags |= kTransformQueuedFlag;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodegetuserdata
     * @recoil-artifact defines .text recoil:function:0x447f00: CZClass::gwNodeGetUserData
     * @recoil-match byte
     *
     * Purpose: read the user-data or display-instance reference stored on a
     * zClass node.
     */
    int __fastcall gwNodeGetUserData(CZNodePartial * node, unsigned int* outData)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x464, "Null node pointer.");
            return 5;
        }

        *outData = node->userDataOrDiRef;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetactioncallback
     * @recoil-artifact defines .text recoil:function:0x447f30: CZClass::gwNodeSetActionCallback
     * @recoil-match byte
     *
     * Purpose: install or clear the node action callback in its priority
     * bucket using head insertion for newly active callback nodes.
     */
    int __fastcall gwNodeSetActionCallback(CZNodePartial * node, void* actionCallback)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x47e, "Null node pointer.");
            return 5;
        }

        int callbackPriority = node->callbackPriority;
        if (callbackPriority >= 0 && callbackPriority < 6) {
            if (node->actionCallback == 0 && actionCallback != 0) {
                if (CZTypeList::Insert(callbackPriority, node) != 0) {
                    if ((node->flags & 0x800) == 0) {
                        free(node);
                    }
                    return 5;
                }
            } else if (node->actionCallback != 0 && actionCallback == 0) {
                CZTypeList::MarkPendingRemoval(callbackPriority, node);
            }

            node->actionCallback = actionCallback;
            return 0;
        }

        zError::ReportOld(
            0x400,
            "D:\\Proj\\GameZRecoil\\zClass\\Class.c",
            0x483,
            "ERROR setting action callback; priority = %d",
            callbackPriority
        );
        return 1;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetactioncallbacktail
     * @recoil-artifact defines .text recoil:function:0x447fe0: CZClass::gwNodeSetActionCallbackTail.
     * @recoil-match byte
     *
     * Purpose: install or clear a node action callback using tail insertion
     * for newly active callback buckets.
     */
    int __fastcall gwNodeSetActionCallbackTail(CZNodePartial * node, void* actionCallback)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x4c3, "Null node pointer.");
            return 5;
        }

        int callbackPriority = node->callbackPriority;
        if (callbackPriority >= 0 && callbackPriority < 6) {
            if (node->actionCallback == 0 && actionCallback != 0) {
                if (CZTypeList::InsertChildNodes(callbackPriority, node) != 0) {
                    if ((node->flags & 0x800) == 0) {
                        free(node);
                    }
                    return 5;
                }
            } else if (node->actionCallback != 0 && actionCallback == 0) {
                CZTypeList::MarkPendingRemoval(callbackPriority, node);
            }

            node->actionCallback = actionCallback;
            return 0;
        }

        zError::ReportOld(
            0x400,
            "D:\\Proj\\GameZRecoil\\zClass\\Class.c",
            0x4c8,
            "ERROR setting action callback; priority = %d",
            callbackPriority
        );
        return 1;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetpriority
     * @recoil-artifact defines .text recoil:function:0x448090: CZClass::gwNodeSetPriority
     * @recoil-match byte
     *
     * Purpose: move an active callback node between priority buckets and store
     * the caller-supplied priority value.
     */
    int __fastcall gwNodeSetPriority(CZNodePartial * node, int priority)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x4fc, "Null node pointer.");
            return 5;
        }

        if (node->actionCallback != 0) {
            if (node->callbackPriority >= 0 && node->callbackPriority < 6) {
                CZTypeList::MarkPendingRemoval(node->callbackPriority, node);
            }
            if (priority >= 0 && priority < 6) {
                CZTypeList::Insert(priority, node);
            }
        }

        node->callbackPriority = priority;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetcellpickable
     * @recoil-artifact defines .text recoil:function:0x448100: CZClass::gwNodeSetCellPickable
     * @recoil-match byte
     *
     * Purpose: set or clear the cell-pickable flag on a node.
     */
    int __fastcall gwNodeSetCellPickable(CZNodePartial * node, int value)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x529, "Null node pointer.");
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
     * @recoil-artifact defines .text recoil:function:0x448140: CZClass::gwNodeGetCellPickable
     * @recoil-match byte
     *
     * Purpose: read the cell-pickable flag from a node.
     */
    int __fastcall gwNodeGetCellPickable(CZNodePartial * node, int* outValue)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x542, "Null node pointer.");
            return 5;
        }

        *outValue = (node->flags & 0x08) != 0;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodegetnodetype
     * @recoil-artifact defines .text recoil:function:0x448180: CZClass::gwNodeGetNodeType
     * @recoil-match byte
     *
     * Purpose: read the byte-sized node type metadata value.
     */
    int __fastcall gwNodeGetNodeType(CZNodePartial * node, int* outValue)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x556, "Null node pointer.");
            return 5;
        }

        *outValue = node->nodeType;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetraycastable
     * @recoil-artifact defines .text recoil:function:0x4481b0: CZClass::gwNodeSetRaycastable
     * @recoil-match byte
     *
     * Purpose: set or clear the raycastable flag on a node.
     */
    int __fastcall gwNodeSetRaycastable(CZNodePartial * node, int value)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x56c, "Null node pointer.");
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
     * @recoil-artifact defines .text recoil:function:0x4481f0: CZClass::gwNodeGetRaycastable
     * @recoil-match byte
     *
     * Purpose: read the raycastable flag from a node.
     */
    int __fastcall gwNodeGetRaycastable(CZNodePartial * node, int* outValue)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x584, "Null node pointer.");
            return 5;
        }

        *outValue = (node->flags & 0x10) != 0;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetpickable
     * @recoil-artifact defines .text recoil:function:0x448230: CZClass::gwNodeSetPickable
     * @recoil-match byte
     *
     * Purpose: set or clear the pickable flag on a node.
     */
    int __fastcall gwNodeSetPickable(CZNodePartial * node, int value)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x59a, "Null node pointer.");
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
     * @recoil-artifact defines .text recoil:function:0x448270: CZClass::gwNodeGetPickable
     * @recoil-match byte
     *
     * Purpose: read the pickable flag from a node.
     */
    int __fastcall gwNodeGetPickable(CZNodePartial * node, int* outValue)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x5b2, "Null node pointer.");
            return 5;
        }

        *outValue = (node->flags & 0x20) != 0;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesethashitcallback
     * @recoil-artifact defines .text recoil:function:0x4482b0: CZClass::gwNodeSetHasHitCallback
     * @recoil-match byte
     *
     * Purpose: set or clear the node flag that marks an installed hit
     * callback handler.
     */
    int __fastcall gwNodeSetHasHitCallback(CZNodePartial * node, int value)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x5c7, "Null node pointer.");
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
     * @recoil-artifact defines .text recoil:function:0x4482f0: CZClass::gwNodeSetBypassFarClip
     * @recoil-match byte
     *
     * Purpose: set or clear the node flag that bypasses far-clip culling.
     */
    int __fastcall gwNodeSetBypassFarClip(CZNodePartial * node, int value)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x5e1, "Null node pointer.");
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
     * @recoil-artifact defines .text recoil:function:0x448330: CZClass::gwNodeSetNodeType
     * @recoil-match byte
     *
     * Purpose: store the low byte of the caller-supplied node type metadata
     * value.
     */
    int __fastcall gwNodeSetNodeType(CZNodePartial * node, int nodeType)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x5f9, "Null node pointer.");
            return 5;
        }

        node->nodeType = (unsigned char)(nodeType);
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodeclearvariantgate
     * @recoil-artifact defines .text recoil:function:0x448360: CZClass::gwNodeClearVariantGate
     * @recoil-match byte
     *
     * Purpose: clear the node variant-gate flag when the caller supplies a
     * zero gate value.
     */
    int __fastcall gwNodeClearVariantGate(CZNodePartial * node, int value)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x60f, "Null node pointer.");
            return 5;
        }

        if ((node->flags & kNodeVariantGateFlag) != 0 && value == 0) {
            node->flags &= ~kNodeVariantGateFlag;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodesetvertexalphaoverride
     * @recoil-artifact defines .text recoil:function:0x4483a0: CZClass::gwNodeSetVertexAlphaOverride.
     * @recoil-match byte
     *
     * Purpose: set or clear the caller-owned node vertex-alpha override flag.
     */
    int __fastcall gwNodeSetVertexAlphaOverride(CZNodePartial * node, int value)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x62d, "Null node pointer.");
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
     * @recoil-artifact defines .text recoil:function:0x4483f0: CZClass::AddChild.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Class.c.
     * Purpose: dispatch child attachment by parent classId across the
     * data-driven zClass node subsystem.
     */
    int __fastcall AddChild(CZNodePartial * parent, CZNodePartial * child)
    {
        if (child == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x666, "Null node pointer.");
            return 5;
        }
        if (parent == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x667, "Null node pointer.");
            return 5;
        }

        int result;
        switch (parent->classId) {
        case 2:
            result = CZWorld::AddChildAtGrid(parent, child);
            break;
        case 5:
            result = CZObject3D::gwObject3DAddChild(parent, child);
            break;
        case 1:
            result = CZCamera::gwCameraAddChild(parent, child);
            break;
        case 6:
            result = CZLod::gwLodAddChild(parent, child);
            break;
        case 7:
            sprintf(
                g_zError_DebugMsgBuffer,
                "%s: Line %d: ERROR: Please use dedicated function "
                "gwSequenceAddChild() for node: %s\n",
                "D:\\Proj\\GameZRecoil\\zClass\\Class.c",
                0x687,
                parent->name
            );
            zError::EmitDebugBuffer(1);
            result = 1;
            break;
        case 8:
            result = CZAnimate::AddChild(parent, child);
            break;
        case 3:
        case 4:
        case 9:
        case 10:
            result = CZClass::AddChildGeneric(parent, child);
            break;
        case 11:
            result = CZClass::AddChildValidated(parent, child);
            break;
        default:
            sprintf(
                g_zError_DebugMsgBuffer,
                "%s: Line %d: ERROR: Unrecognized node class type for node: %s\n",
                "D:\\Proj\\GameZRecoil\\zClass\\Class.c",
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
     * @recoil-artifact defines .text recoil:function:0x4484d0: CZClass::AddChildGeneric.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Class.c.
     * Purpose: append child and parent references to the generic listB/listA
     * node-link arrays and queue parent transform/bounds updates.
     */
    int __fastcall AddChildGeneric(CZNodePartial * parent, CZNodePartial * child)
    {
        const int newChildCount = parent->listCountB + 1;
        parent->listB = (CZNodePartial**)(realloc(parent->listB, (size_t)(newChildCount) * sizeof(parent->listB[0])));
        parent->listB[newChildCount - 1] = child;
        ++parent->listCountB;

        const int newParentCount = child->listCountA + 1;
        child->listA = (CZNodePartial**)(realloc(child->listA, (size_t)(newParentCount) * sizeof(child->listA[0])));
        child->listA[newParentCount - 1] = parent;
        ++child->listCountA;
        if (child->listCountA > 1) {
            SetSingleParentFlagRecursive(child, 0);
        }

        parent->boundsFlags |= kBoundsDirtyFlag;
        if ((parent->flags & kTypeListInsertedFlag) == 0) {
            CZTypeList::Insert(kQueuedTreeBucket, parent);
            parent->flags |= kTypeListInsertedFlag;
        }
        parent->flags |= kTransformQueuedFlag;

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.removechild
     * @recoil-artifact defines .text recoil:function:0x448570: CZClass::RemoveChild.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Class.c.
     * Purpose: dispatch child removal by parent classId across the data-driven
     * zClass node subsystem.
     */
    int __fastcall RemoveChild(CZNodePartial * parent, CZNodePartial * child)
    {
        if (parent == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x713, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x714, "Null node pointer.");
            return 5;
        }

        int result;
        switch (parent->classId) {
        case 2:
            result = CZWorld::RemoveChildAtGrid(parent, child);
            break;
        case 5:
            result = CZObject3D::RemoveChild(parent, child);
            break;
        case 9:
            result = CZLight::RemoveChild(parent, child);
            break;
        case 10:
            result = CZSound::RemoveChild(parent, child);
            break;
        case 1:
            result = CZCamera::gwCameraRemoveChild(parent, child);
            break;
        case 3:
            result = CZClass::RemoveChildChecked(parent, child);
            break;
        case 4:
            result = CZDisplay::RemoveChild(parent, child);
            break;
        case 6:
            result = CZLod::RemoveChild(parent, child);
            break;
        case 7:
            result = CZSequence::RemoveChild(parent, child);
            break;
        case 8:
            result = CZAnimate::RemoveChild(parent, child);
            break;
        case 11:
            result = CZClass::RemoveChildValidated(parent, child);
            break;
        default:
            sprintf(
                g_zError_DebugMsgBuffer,
                "%s: Line %d: ERROR: Unrecognized node class type for node: %s\n",
                "D:\\Proj\\GameZRecoil\\zClass\\Class.c",
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
     * @recoil-artifact defines .text recoil:function:0x448660: CZClass::RemoveChildGeneric.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Class.c.
     * Purpose: remove matching child and parent references from generic
     * listB/listA node-link arrays and queue parent transform/bounds updates.
     */
    int __fastcall RemoveChildGeneric(CZNodePartial * parent, CZNodePartial * child)
    {
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
                "D:\\Proj\\GameZRecoil\\zClass\\Class.c",
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
                SetSingleParentFlagRecursive(child, 1);
            }
        }

        parent->boundsFlags |= kBoundsDirtyFlag;
        if ((parent->flags & kTypeListInsertedFlag) == 0) {
            CZTypeList::Insert(kQueuedTreeBucket, parent);
            parent->flags |= kTypeListInsertedFlag;
        }
        parent->flags |= kTransformQueuedFlag;

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodegetbbox
     * @recoil-artifact defines .text recoil:function:0x448760: CZClass::gwNodeGetBBox.
     * @recoil-match byte
     *
     * Purpose: copy the cached node bounding box when it is currently valid.
     */
    int __fastcall gwNodeGetBBox(CZNodePartial * node, zBBox3f * outBBox)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x7f9, "Null node pointer.");
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x7fa, "Null class data pointer");
            return 5;
        }
        if ((node->flags & 0x100) == 0) {
            return 1;
        }

        memcpy(outBBox, (const zBBox3f*)(node->cachedBounds), sizeof(*outBBox));
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodegetworldbboxcorners
     * @recoil-artifact defines .text recoil:function:0x4487c0: CZClass::gwNodeGetWorldBBoxCorners.
     * @recoil-match byte
     *
     * Purpose: return cached bounds corners in world/node space for object,
     * camera, animate, and untransformed node classes.
     */
    int __fastcall gwNodeGetWorldBBoxCorners(CZNodePartial * node, zBBoxCorners * outCorners)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x81b, "Null node pointer.");
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x81c, "Null class data pointer");
            return 5;
        }
        if ((node->flags & 0x100) == 0) {
            return 1;
        }

        if (node->classId == 5) {
            const CZObject3DDataPartial* objectData = (const CZObject3DDataPartial*)(node->classData);
            if ((objectData->flags & 0x08) == 0) {
                zMathMatTransformBBoxToCorners(
                    (const zMat4x3*)(objectData->localMatrix),
                    (const zBBox3f*)node->cachedBounds,
                    outCorners
                );
                return 0;
            }
        } else if (node->classId == 1) {
            const CZCameraDataPartial* cameraData = (const CZCameraDataPartial*)(node->classData);
            zMathMatTransformBBoxToCorners(
                &((const CZCameraBBoxQueryDataPartial*)(cameraData))->viewOverlay.cachedViewMatrix,
                (const zBBox3f*)node->cachedBounds,
                outCorners
            );
            return 0;
        } else if (node->classId == 8) {
            const CZAnimateDataPartial* animateData = (const CZAnimateDataPartial*)(node->classData);
            if ((node->flags & 0x04) != 0 && (animateData->statusFlags & 0x04) != 0) {
                zMathMatTransformBBoxToCorners(
                    (const zMat4x3*)(animateData->animatedTransform),
                    (const zBBox3f*)node->cachedBounds,
                    outCorners
                );
                return 0;
            }
        }

        zVec3* out = outCorners->corners;
        out[0].x = node->cachedBounds[0];
        out[0].y = node->cachedBounds[1];
        out[0].z = node->cachedBounds[5];
        out[1].x = node->cachedBounds[3];
        out[1].y = node->cachedBounds[1];
        out[1].z = node->cachedBounds[5];
        out[2].x = node->cachedBounds[3];
        out[2].y = node->cachedBounds[1];
        out[2].z = node->cachedBounds[2];
        out[3].x = node->cachedBounds[0];
        out[3].y = node->cachedBounds[1];
        out[3].z = node->cachedBounds[2];
        out[4].x = node->cachedBounds[0];
        out[4].y = node->cachedBounds[4];
        out[4].z = node->cachedBounds[5];
        out[5].x = node->cachedBounds[3];
        out[5].y = node->cachedBounds[4];
        out[5].z = node->cachedBounds[5];
        out[6].x = node->cachedBounds[3];
        out[6].y = node->cachedBounds[4];
        out[6].z = node->cachedBounds[2];
        out[7].x = node->cachedBounds[0];
        out[7].y = node->cachedBounds[4];
        out[7].z = node->cachedBounds[2];
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodegetviewbboxcorners
     * @recoil-artifact defines .text recoil:function:0x448920: CZClass::gwNodeGetViewBBoxCorners.
     * @recoil-match commutative
     *
     * Purpose: return cached bounds corners after combining the view and node transforms.
     */
    int __fastcall gwNodeGetViewBBoxCorners(CZNodePartial * node, zBBoxCorners * outCorners)
    {
        int returnCode = 0;
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x85f, "Null node pointer.");
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x860, "Null class data pointer");
            return 5;
        }
        if ((node->flags & 0x100) == 0) {
            return 1;
        }

        int currentIsIdentity = zMathMatIsCurrentIdentity();
        zMat4x3* currentMatrix = zMathMatGetCurrent();
        int skipTransform = 0;
        const zMat4x3* nodeMatrix = 0;

        switch (node->classId) {
        case 5: {
            const CZObject3DDataPartial* objectData = (const CZObject3DDataPartial*)(node->classData);
            skipTransform = ((unsigned int)objectData->flags >> 3) & 0x01;
            nodeMatrix = (const zMat4x3*)(objectData->localMatrix);
            break;
        }
        case 8: {
            const CZAnimateDataPartial* animateData = (const CZAnimateDataPartial*)(node->classData);
            if ((node->flags & 0x04) == 0 || (animateData->statusFlags & 0x04) == 0) {
                skipTransform = 1;
            }
            nodeMatrix = (const zMat4x3*)(animateData->animatedTransform);
            break;
        }
        case 1: {
            const CZCameraDataPartial* cameraData = (const CZCameraDataPartial*)(node->classData);
            nodeMatrix = &((const CZCameraBBoxQueryDataPartial*)(cameraData))->viewOverlay.cachedViewMatrix;
            break;
        }
        case 2:
        case 6:
        case 7:
            skipTransform = 1;
            nodeMatrix = 0;
            break;
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

        zMat4x3 combinedMatrix;
        const zMat4x3* transformMatrix;
        if (currentIsIdentity != 0) {
            if (skipTransform != 0) {
                zVec3* out = outCorners->corners;
                out[0].x = node->cachedBounds[0];
                out[0].y = node->cachedBounds[1];
                out[0].z = node->cachedBounds[5];
                out[1].x = node->cachedBounds[3];
                out[1].y = node->cachedBounds[1];
                out[1].z = node->cachedBounds[5];
                out[2].x = node->cachedBounds[3];
                out[2].y = node->cachedBounds[1];
                out[2].z = node->cachedBounds[2];
                out[3].x = node->cachedBounds[0];
                out[3].y = node->cachedBounds[1];
                out[3].z = node->cachedBounds[2];
                out[4].x = node->cachedBounds[0];
                out[4].y = node->cachedBounds[4];
                out[4].z = node->cachedBounds[5];
                out[5].x = node->cachedBounds[3];
                out[5].y = node->cachedBounds[4];
                out[5].z = node->cachedBounds[5];
                out[6].x = node->cachedBounds[3];
                out[6].y = node->cachedBounds[4];
                out[6].z = node->cachedBounds[2];
                out[7].x = node->cachedBounds[0];
                out[7].y = node->cachedBounds[4];
                out[7].z = node->cachedBounds[2];
                return returnCode;
            }
            transformMatrix = nodeMatrix;
        } else if (skipTransform != 0) {
            transformMatrix = currentMatrix;
        } else {
            double xxPartial;
            float zxPartial;
            float yyPartial;
            double xzPartial;
            double zzPartial;
            double posYPartial;
            double xxValue, yxValue; // Unused captures preserve the observed VC5 store sequence.
            const zMat4x3* left = currentMatrix;
            const zMat4x3* right = nodeMatrix;
            zMat4x3* product = &combinedMatrix;
            xxPartial = left->zx * right->xz + left->yx * right->xy;
            product->xx = xxValue = xxPartial + left->xx * right->xx;
            product->yx = yxValue = left->xx * right->yx + left->yx * right->yy + left->zx * right->yz;
            zxPartial = left->xx * right->zx + left->zx * right->zz;
            product->zx = zxPartial + left->yx * right->zy;
            product->xy = left->xy * right->xx + left->yy * right->xy + left->zy * right->xz;
            yyPartial = left->xy * right->yx + left->zy * right->yz;
            product->yy = yyPartial + left->yy * right->yy;
            product->zy = left->xy * right->zx + left->yy * right->zy + left->zy * right->zz;
            xzPartial = left->xz * right->xx + left->zz * right->xz;
            product->xz = xzPartial + left->yz * right->xy;
            product->yz = left->xz * right->yx + left->yz * right->yy + left->zz * right->yz;
            zzPartial = left->xz * right->zx + left->zz * right->zz;
            product->zz = zzPartial + left->yz * right->zy;
            product->posX = left->xx * right->posX + left->yx * right->posY + left->zx * right->posZ + left->posX;
            posYPartial = left->yy * right->posY + left->zy * right->posZ;
            product->posY = posYPartial + left->xy * right->posX + left->posY;
            product->posZ
                = (float)(left->xz * right->posX + left->yz * right->posY) + left->zz * right->posZ + left->posZ;
            transformMatrix = product;
        }

        zMathMatTransformBBoxToCorners(transformMatrix, (const zBBox3f*)node->cachedBounds, outCorners);
        return returnCode;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodeupdate
     * @recoil-artifact defines .text recoil:function:0x448cc0: CZClass::gwNodeUpdate.
     * @recoil-match byte
     *
     * Purpose: process pending transform and bounds work for one scene node
     * and run class-specific camera, world, object, and animate updates.
     */
    int __fastcall gwNodeUpdate(CZNodePartial * node)
    {
        int result = 0;
        int needsBBoxRecalc = 0;

        if (node->boundsFlags != 0) {
            if ((node->boundsFlags & 0x01) != 0) {
                gwNodeUpdateDisplayInstance(node);
                needsBBoxRecalc = 1;
            }
            if ((node->boundsFlags & 0x02) != 0) {
                gwNodeComputeChildBBox(node);
                needsBBoxRecalc = 1;
            }
            node->boundsFlags &= 0x04;
        }

        switch (node->classId) {
        case 5: {
            CZObject3DDataPartial* objectData = (CZObject3DDataPartial*)(node->classData);
            if ((objectData->flags & 0x01) != 0) {
                if ((objectData->flags & 0x10) == 0) {
                    // Preserve translation before MatLoadIdentity overwrites its storage.
                    const zVec3 position = *(const zVec3*)&objectData->localMatrix[9];
                    zMath::MatStackPushPtr(objectData->localMatrix);
                    zMath::MatLoadIdentity();
                    zMath::MatApplyLocalTRS(&objectData->rotation, &position, &objectData->scale);
                    zMath::MatStackPopPtr();
                }
                gwNodeRecalcBBox(node);
                needsBBoxRecalc = 0;
                objectData->flags &= ~0x01;
            }
            break;
        }
        case 1: {
            const zVec3 unitScale = { 1.0f, 1.0f, 1.0f };
            CZCameraDataPartial* cameraData = (CZCameraDataPartial*)(node->classData);
            if ((cameraData->cameraFlags & 0x04) != 0) {
                if ((cameraData->cameraFlags & 0x02) == 0) {
                    zMath::MatStackPushPtr(
                        (float*)(&((CZCameraBBoxQueryDataPartial*)(cameraData))->viewOverlay.cachedViewMatrix)
                    );
                    zMath::MatLoadIdentity();
                    zMath::MatApplyLocalTRS(&cameraData->posOffset, &cameraData->targetOrEuler, &unitScale);
                    zMath::MatStackPopPtr();
                }
                gwNodeRecalcBBox(node);
                needsBBoxRecalc = 0;
                cameraData->cameraFlags &= ~0x04;
            }
            break;
        }
        case 6:
        case 7:
            break;
        case 8: {
            CZAnimateDataPartial* animateData = (CZAnimateDataPartial*)(node->classData);
            if ((node->flags & 0x04) != 0 && (animateData->statusFlags & 0x04) != 0 && animateData->flags != 0) {
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
                    needsBBoxRecalc = 0;
                }
                animateData->flags = 0;
            }
            break;
        }
        case 2:
            CZWorld::ApplyPendingFogSettings(node);
            break;
        default:
            zError::ReportOld(
                0x200,
                "D:\\Proj\\GameZRecoil\\zClass\\Class.c",
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
     * @recoil-artifact defines .text recoil:function:0x448e90: CZClass::gwNodeRecalcBBox.
     *
     *
     * Purpose: select or merge primary and child bounds, cache the result, and
     * propagate parent/world-grid bounds updates.
     */
    int __fastcall gwNodeRecalcBBox(CZNodePartial * node)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0x9d0, "Null node pointer.");
            return 5;
        }
        if (node->classId == 2) {
            return 0;
        }

        zBBox3f merged = { 0 };
        const zBBox3f* bboxSource = 0;
        const bool hasPrimaryBBox = (node->flags & 0x200) != 0;
        const bool hasChildBBox = (node->flags & 0x400) != 0;
        CZNodeFreeListSlot* nodeSlot = (CZNodeFreeListSlot*)(node);
        const zBBox3f* primaryBBox = hasPrimaryBBox ? &nodeSlot->primaryBounds : 0;
        const zBBox3f* secondaryBBox = hasChildBBox ? &nodeSlot->secondaryBounds : 0;
        if (hasPrimaryBBox && hasChildBBox) {
            merged.min.x = primaryBBox->min.x < secondaryBBox->min.x ? primaryBBox->min.x : secondaryBBox->min.x;
            merged.min.y = primaryBBox->min.y < secondaryBBox->min.y ? primaryBBox->min.y : secondaryBBox->min.y;
            merged.min.z = primaryBBox->min.z < secondaryBBox->min.z ? primaryBBox->min.z : secondaryBBox->min.z;
            merged.max.x = primaryBBox->max.x > secondaryBBox->max.x ? primaryBBox->max.x : secondaryBBox->max.x;
            merged.max.y = primaryBBox->max.y > secondaryBBox->max.y ? primaryBBox->max.y : secondaryBBox->max.y;
            merged.max.z = primaryBBox->max.z > secondaryBBox->max.z ? primaryBBox->max.z : secondaryBBox->max.z;
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
            CZNodePartial* parent = node->listA[i];
            if (parent->classId == 2) {
                if (!worldRectComputed) {
                    zBBoxCorners corners = { 0 };
                    gwNodeGetWorldBBoxCorners(node, &corners);
                    minX = maxX = corners.corners[0].x;
                    minZ = maxZ = corners.corners[0].z;
                    for (int cornerIndex = 1; cornerIndex < 8; ++cornerIndex) {
                        const zVec3* corner = &corners.corners[cornerIndex];
                        if (corner->x < minX)
                            minX = corner->x;
                        else if (corner->x > maxX)
                            maxX = corner->x;
                        if (corner->z < minZ)
                            minZ = corner->z;
                        else if (corner->z > maxZ)
                            maxZ = corner->z;
                    }
                    worldRectComputed = true;
                }

                int gridCol = -1;
                int gridRow = -1;
                if ((node->flags & 0x80) == 0) {
                    CZWorld::WorldRectToGridIndex(parent, &gridCol, minX, maxX, minZ, maxZ, &gridRow);
                }

                if (gridCol == node->gridCol && gridRow == node->gridRow) {
                    if (node->gridCol >= 0 && node->gridRow >= 0) {
                        CZWorld::EnsureGridCellDisplayPosition(parent, node->gridCol, node->gridRow);
                    }
                } else {
                    CZWorld::RemoveChildAtGrid(parent, node);
                    CZWorld::AddChildToGridCell(parent, node, gridCol, gridRow);
                }
            } else {
                parent->boundsFlags |= 0x02;
                if ((parent->flags & 0x01) == 0) {
                    CZTypeList::InsertChildNodes(kQueuedTreeBucket, parent);
                    parent->flags |= 0x01;
                }
                parent->flags |= 0x02;
            }
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodecomputechildbbox
     * @recoil-artifact defines .text recoil:function:0x4491b0: CZClass::gwNodeComputeChildBBox.
     * @recoil-match byte
     *
     * Purpose: merge valid child world-bounds corners into the node's
     * secondary bounding box.
     */
    int __fastcall gwNodeComputeChildBBox(CZNodePartial * node)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0xaa3, "Null node pointer.");
            return 5;
        }

        node->flags &= ~0x400;
        if (node->listCountB == 0 || node->classId == 2) {
            return 0;
        }

        CZNodeFreeListSlot* nodeSlot = (CZNodeFreeListSlot*)node;
        zBBoxCorners corners;
        int childIndex = 0;
        int nextChildIndex = node->listCountB;

        for (; childIndex < node->listCountB; ++childIndex) {
            CZNodePartial* child = node->listB[childIndex];
            if ((child->flags & 0x100) == 0) {
                continue;
            }

            gwNodeGetWorldBBoxCorners(node->listB[childIndex], &corners);
            nextChildIndex = childIndex + 1;
            nodeSlot->node.flags |= 0x400;

            nodeSlot->secondaryBounds.max.x = nodeSlot->secondaryBounds.min.x = corners.corners[0].x;
            nodeSlot->secondaryBounds.max.y = nodeSlot->secondaryBounds.min.y = corners.corners[0].y;
            nodeSlot->secondaryBounds.max.z = nodeSlot->secondaryBounds.min.z = corners.corners[0].z;

            for (int cornerIndex = 1; cornerIndex < 8; ++cornerIndex) {
                const zVec3* corner = &corners.corners[cornerIndex];
                if (corner->x < nodeSlot->secondaryBounds.min.x) {
                    nodeSlot->secondaryBounds.min.x = corner->x;
                } else if (corner->x > nodeSlot->secondaryBounds.max.x) {
                    nodeSlot->secondaryBounds.max.x = corner->x;
                }
                if (corner->y < nodeSlot->secondaryBounds.min.y) {
                    nodeSlot->secondaryBounds.min.y = corner->y;
                } else if (corner->y > nodeSlot->secondaryBounds.max.y) {
                    nodeSlot->secondaryBounds.max.y = corner->y;
                }
                if (corner->z < nodeSlot->secondaryBounds.min.z) {
                    nodeSlot->secondaryBounds.min.z = corner->z;
                } else if (corner->z > nodeSlot->secondaryBounds.max.z) {
                    nodeSlot->secondaryBounds.max.z = corner->z;
                }
            }
            break;
        }

        if ((node->flags & 0x400) == 0) {
            return 0;
        }

        for (childIndex = nextChildIndex; childIndex < node->listCountB; ++childIndex) {
            CZNodePartial* child = node->listB[childIndex];
            if ((child->flags & 0x100) == 0) {
                continue;
            }

            gwNodeGetWorldBBoxCorners(child, &corners);

            for (int cornerIndex = 0; cornerIndex < 8; ++cornerIndex) {
                const zVec3* corner = &corners.corners[cornerIndex];
                if (corner->x < nodeSlot->secondaryBounds.min.x) {
                    nodeSlot->secondaryBounds.min.x = corner->x;
                } else if (corner->x > nodeSlot->secondaryBounds.max.x) {
                    nodeSlot->secondaryBounds.max.x = corner->x;
                }
                if (corner->y < nodeSlot->secondaryBounds.min.y) {
                    nodeSlot->secondaryBounds.min.y = corner->y;
                } else if (corner->y > nodeSlot->secondaryBounds.max.y) {
                    nodeSlot->secondaryBounds.max.y = corner->y;
                }
                if (corner->z < nodeSlot->secondaryBounds.min.z) {
                    nodeSlot->secondaryBounds.min.z = corner->z;
                } else if (corner->z > nodeSlot->secondaryBounds.max.z) {
                    nodeSlot->secondaryBounds.max.z = corner->z;
                }
            }
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodeupdatedisplayinstance
     * @recoil-artifact defines .text recoil:function:0x449420: CZClass::gwNodeUpdateDisplayInstance.
     * @recoil-match byte
     *
     * Purpose: rebuild display-instance bounds into the node primary box and
     * update the primary-bounds-valid flag.
     */
    int __fastcall gwNodeUpdateDisplayInstance(CZNodePartial * node)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0xb31, "Null node pointer.");
            return 5;
        }

        zDiPartial* di = (zDiPartial*)((unsigned int)(node->userDataOrDiRef));
        if (di != 0) {
            zDi::RebuildBounds(di, (zBoundsMinMaxPartial*)(&((CZNodeFreeListSlot*)(node))->primaryBounds));
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

namespace CZNode
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.buildnodetoancestormatrix
     * @recoil-artifact defines .text recoil:function:0x449480: CZNode::gwNodeBuildNodeToAncestorMatrix
     *
     *
     * Purpose: apply a node's parent-chain transforms into the current matrix.
     */
    int __fastcall gwNodeBuildNodeToAncestorMatrix(CZNodePartial * node, int matMode)
    {
        zVec3 unitScale = { 1.0f, 1.0f, 1.0f };
        zVec3 zeroAngles = { 0 };

        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Class.c", 0xb66, "Null node pointer.");
            return 5;
        }

        if (node->classId == 5 && (node->flags & kSingleParentFlag) != 0) {
            CZObject3DDataPartial* objectData = (CZObject3DDataPartial*)(node->classData);
            if ((objectData->flags & 0x20) == 0) {
                zMath::MatLoadCurrentFrom((const zMat4x3*)(objectData->cachedWorldMatrix));
                return 0;
            }
        }

        CZNodePartial* parentChain[15] = { 0 };
        int chainCount = 1;
        parentChain[0] = node;
        CZNodePartial* current = node;
        while (current != 0) {
            if (current->listCountA > 1) {
                zError::ReportOld(
                    0x800,
                    "D:\\Proj\\GameZRecoil\\zClass\\Class.c",
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
            CZNodePartial* chainNode = parentChain[i];
            if (chainNode->classId != 2 && (chainNode->flags & 0x01) != 0) {
                UpdateTree(chainNode);
                break;
            }
        }

        for (int i_1435 = chainCount - 1; i_1435 >= 0; --i_1435) {
            CZNodePartial* ancestor = parentChain[i_1435];
            const int ancestorFlags = ancestor->flags & ~kNodeTransformDirtyPropagatedFlag;
            ancestor->flags = ancestorFlags;
            switch (ancestor->classId) {
            case 5: {
                CZObject3DDataPartial* objectData = (CZObject3DDataPartial*)(ancestor->classData);
                const int objectFlags = objectData->flags;
                if ((objectFlags & 0x08) == 0) {
                    if ((ancestorFlags & kSingleParentFlag) != 0) {
                        if ((objectFlags & 0x20) != 0) {
                            zMath::MatMultiply((const zMat4x3*)(objectData->localMatrix), matMode);
                            zMat4x3 currentMatrix;
                            zMath::MatCopyCurrentTo(&currentMatrix);
                            memcpy(objectData->cachedWorldMatrix, &currentMatrix, sizeof(currentMatrix));
                            objectData->flags &= ~0x20;
                        } else {
                            zMath::MatLoadCurrentFrom((const zMat4x3*)(objectData->cachedWorldMatrix));
                        }
                    } else {
                        zMath::MatMultiply((const zMat4x3*)(objectData->localMatrix), matMode);
                    }
                } else if ((ancestorFlags & kSingleParentFlag) != 0 && (objectFlags & 0x20) != 0) {
                    zMat4x3 currentMatrix;
                    zMath::MatCopyCurrentTo(&currentMatrix);
                    memcpy(objectData->cachedWorldMatrix, &currentMatrix, sizeof(currentMatrix));
                    objectData->flags &= ~0x20;
                }
                break;
            }
            case 2:
            case 6:
                break;
            case 9: {
                CZLightDataPartial* lightData = (CZLightDataPartial*)(ancestor->classData);
                zMath::MatApplyLocalTRS(&lightData->localRotation, &lightData->localPosition, &unitScale);
                break;
            }
            case 10: {
                CZSoundDataPartial* soundData = (CZSoundDataPartial*)(ancestor->classData);
                zMath::MatApplyLocalTRS(&zeroAngles, &soundData->localPosition, &unitScale);
                break;
            }
            case 1: {
                CZCameraDataPartial* cameraData = (CZCameraDataPartial*)(ancestor->classData);
                if ((cameraData->cameraFlags & 0x02) == 0) {
                    zMath::MatApplyLocalTRS(&cameraData->posOffset, &cameraData->targetOrEuler, &unitScale);
                } else {
                    zMath::MatMultiply(&((CZCameraBBoxQueryDataPartial*)(cameraData))->viewOverlay.cachedViewMatrix, 1);
                }
                break;
            }
            case 8: {
                CZAnimateDataPartial* animateData = (CZAnimateDataPartial*)(ancestor->classData);
                if ((ancestorFlags & 0x04) != 0 && (animateData->statusFlags & 0x04) != 0) {
                    zMath::MatMultiply((const zMat4x3*)(animateData->animatedTransform), matMode);
                }
                break;
            }
            default:
                sprintf(
                    g_zError_DebugMsgBuffer,
                    "%s: Line %d: gwNodeBuildNodeToAncestorMatrix(): Unrecognized node "
                    "class type:\n",
                    "D:\\Proj\\GameZRecoil\\zClass\\Class.c",
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
     * @recoil-artifact defines .text recoil:function:0x4497b0: CZNode::GetWorldPosition
     * @recoil-match byte
     *
     * Purpose: resolve a node's world-space translation into the output vector.
     */
    int __fastcall GetWorldPosition(CZNodePartial * node, zVec3 * outPosition)
    {
        if (node != 0) {
            if (node->classId == 5 && (node->flags & kSingleParentFlag) != 0) {
                CZObject3DDataPartial* objectData = (CZObject3DDataPartial*)(node->classData);
                if ((objectData->flags & 0x20) == 0) {
                    memcpy(outPosition, &objectData->cachedWorldMatrix[9], sizeof(*outPosition));
                    return 0;
                }
            }

            outPosition->x = outPosition->y = outPosition->z = 0.0f;

            float matrix[12];
            zMath::MatStackPushPtr(matrix);
            zMath::MatLoadIdentity();
            gwNodeBuildNodeToAncestorMatrix(node, 1);
            outPosition->x = matrix[9];
            outPosition->y = matrix[10];
            outPosition->z = matrix[11];
            zMath::MatStackPopPtr();
            return 0;
        }

        return 1;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.transformpoint
     * @recoil-artifact defines .text recoil:function:0x449850: CZNode::TransformPoint.
     * @recoil-match byte
     *
     * Purpose: transform a point from node-local space into world space.
     */
    int __fastcall TransformPoint(CZNodePartial * node, zVec3 * point)
    {
        if (node != 0) {
            if (point->x == 0.0f && point->y == 0.0f && point->z == 0.0f) {
                GetWorldPosition(node, point);
                return 0;
            }

            float matrix[12];
            zMath::MatStackPushPtr(matrix);
            zMath::MatLoadIdentity();
            gwNodeBuildNodeToAncestorMatrix(node, 1);
            zMath::MatTransformPointBatchInPlace(point, 1);
            zMath::MatStackPopPtr();
            return 0;
        }

        return 1;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.getworldposandorientation
     * @recoil-artifact defines .text recoil:function:0x4498e0: CZNode::GetWorldPosAndOrientation.
     *
     *
     * Purpose: compute a node world position and derive orientation angles
     * from transformed basis points.
     */
    int __fastcall GetWorldPosAndOrientation(CZNodePartial * node, zVec3 * inOutPosition, zVec3 * outOrientation)
    {
        zVec3 localOrientationBasis[2] = { { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f } };

        if (node == 0) {
            return 1;
        }

        zMat4x3 matrix = { 0 };
        zMath::MatStackPushPtr((float*)(&matrix));
        zMath::MatLoadIdentity();
        gwNodeBuildNodeToAncestorMatrix(node, 1);

        if (inOutPosition->x == 0.0f && inOutPosition->y == 0.0f && inOutPosition->z == 0.0f) {
            inOutPosition->x = matrix.posX;
            inOutPosition->y = matrix.posY;
            inOutPosition->z = matrix.posZ;
        } else {
            zMath::MatTransformPointBatchInPlace(inOutPosition, 1);
        }

        zVec3 worldPosition = { matrix.posX, matrix.posY, matrix.posZ };
        zVec3 worldOrientationBasis[2];
        memcpy(worldOrientationBasis, localOrientationBasis, sizeof(worldOrientationBasis));
        if (*zMath::g_currentMatrixIdentityFlagSlot == 0) {
            const zMat4x3* currentMatrix = (const zMat4x3*)(*zMath::g_currentMatrixPtrSlot);
            for (int i = 0; i < 2; ++i) {
                const zVec3 point = localOrientationBasis[i];
                worldOrientationBasis[i].x = point.x * currentMatrix->xx + point.y * currentMatrix->yx
                    + point.z * currentMatrix->zx + currentMatrix->posX;
                worldOrientationBasis[i].y = point.x * currentMatrix->xy + point.y * currentMatrix->yy
                    + point.z * currentMatrix->zy + currentMatrix->posY;
                worldOrientationBasis[i].z = point.x * currentMatrix->xz + point.y * currentMatrix->yz
                    + point.z * currentMatrix->zz + currentMatrix->posZ;
            }
        }

        zMath::MatLoadIdentity();
        *outOrientation = zMath::Vec3DirectionAnglesBetweenPoints(&worldPosition, &worldOrientationBasis[0]);
        outOrientation->z = zMathVec3ElevationAngleBetweenPoints(&worldPosition, &worldOrientationBasis[1]);

        zMath::MatStackPopPtr();
        return 0;
    }
}

namespace CZClass
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodegetroot
     * @recoil-artifact defines .text recoil:function:0x449ab0: CZClass::gwNodeGetRoot
     * @recoil-match byte
     *
     * Purpose: walk a node's single-parent chain and return the root node.
     */
    CZNodePartial* __fastcall gwNodeGetRoot(CZNodePartial * node)
    {
        CZNodePartial* current = node;
        while (current != 0) {
            switch (current->listCountA) {
            case 0:
                return current;
            case 1:
                current = current->listA[0];
                break;
            default:
                zError::ReportOld(
                    0x200,
                    "D:\\Proj\\GameZRecoil\\zClass\\Class.c",
                    0xd0d,
                    "Error getting root node; Multiple parents found.\n  Node: %s\n",
                    current
                );
                return 0;
            }
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.gwnodegetworldchild
     * @recoil-artifact defines .text recoil:function:0x449af0: CZClass::gwNodeGetWorldChild.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Class.c.
     * Purpose: walk a node's single-parent chain through listA links and
     * return the child directly owned by the world node.
     */
    CZNodePartial* __fastcall gwNodeGetWorldChild(CZNodePartial * node)
    {
        CZNodePartial* current = node;
        while (current != 0) {
            switch (current->listCountA) {
            case 0:
                return 0;
            case 1:
                if (current->listA[0]->classId == 2) {
                    return current;
                }
                current = current->listA[0];
                break;
            default:
                zError::ReportOld(
                    0x200,
                    "D:\\Proj\\GameZRecoil\\zClass\\Class.c",
                    0xd4e,
                    "Error getting root node; Multiple parents found.\n  Node: %s\n",
                    current
                );
                return 0;
            }
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.setsingleparentflagrecursive
     * @recoil-artifact defines .text recoil:function:0x449b40: CZClass::SetSingleParentFlagRecursive.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\Class.c.
     * Purpose: propagate the single-parent flag through a data-driven zClass
     * child subtree when listA ownership count changes.
     */
    int __fastcall SetSingleParentFlagRecursive(CZNodePartial * node, int setFlag)
    {
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
            SetSingleParentFlagRecursive(node->listB[i], setFlag);
        }

        return 0;
    }
}

namespace CZNode
{

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.setcontextrecursive
     * @recoil-artifact defines .text recoil:function:0x437e60: CZNode::SetContextRecursive
     * @recoil-match byte
     *
     * BN evidence: fastcall self/context, stack flagMask, callbackContext at
     * 0x40, flags at 0x24, signed listCountB at 0x5c, listB at 0x60,
     * recursive self-call only, and no global data references.
     * Purpose: assign a callback context and OR flag bits through a node
     * subtree using the zClass child-list links.
     */
    void __fastcall SetContextRecursive(CZNodePartial * self, CZNodePartial * context, int flagMask)
    {
        self->callbackContext = context;
        self->flags |= flagMask;

        for (int i = 0; i < self->listCountB; ++i) {
            SetContextRecursive(self->listB[i], context, flagMask);
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.class.setdiflagbit0recursive
     * @recoil-artifact defines .text recoil:function:0x437ea0: CZNode::SetDiFlagBit0Recursive
     * @recoil-match byte
     *
     * BN evidence: fastcall node/enabled, gwNodeGetUserData for the typed
     * userDataOrDiRef display-instance reference, zDi::SetFlagBit0 when
     * non-null, signed listCountB at 0x5c, listB at 0x60, recursive self-call
     * only, and no global data references.
     * Purpose: set display-instance flag bit 0 for each display instance
     * reachable through a node's child-list subtree.
     */
    void __fastcall SetDiFlagBit0Recursive(CZNodePartial * node, int enabled)
    {
        unsigned int userData;
        CZClass::gwNodeGetUserData(node, &userData);
        zDiPartial* di = (zDiPartial*)(userData);
        if (di != 0) {
            zDi::SetFlagBit0(di, enabled);
        }

        for (int i = 0; i < node->listCountB; ++i) {
            SetDiFlagBit0Recursive(node->listB[i], enabled);
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
