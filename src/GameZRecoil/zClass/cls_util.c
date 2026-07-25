#include "zclass.h"

#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zUtil/zbd.h"

#include <stdlib.h>
#include <string.h>

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-nodearraysize
 * @recoil-artifact defines .data recoil:data:0x539c90: g_zClass_NodeArraySize.
 * Purpose: track the configured zClass node free-list capacity.
 */
int g_zClass_NodeArraySize = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-isinitialized
 * @recoil-artifact defines .data recoil:data:0x539ca4: g_zClass_IsInitialized.
 * Purpose: track whether the core zClass utility subsystem is initialized.
 */
int g_zClass_IsInitialized = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeclonedimode
 * @recoil-artifact defines .data recoil:data:0x4de4cc: g_zClass_CopyNodeCloneDiMode.
 * Purpose: hold the active display-instance clone mode during node-copy recursion.
 */
int g_zClass_CopyNodeCloneDiMode = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-sourcefile-clsutilc
 * @recoil-artifact defines .data recoil:data:0x4de4d0: g_zClass_SourceFile_ClsUtilC.
 * Purpose: store the recovered cls_util.c source path used by zError reports.
 */
char g_zClass_SourceFile_ClsUtilC[0x26] =
    "D:\\Proj\\GameZRecoil\\zClass\\cls_util.c";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-nodearraysizealreadysetfmt
 * @recoil-artifact defines .data recoil:data:0x4de4f8: g_zClass_NodeArraySizeAlreadySetFmt.
 * Purpose: report attempts to resize zClass node storage after configuration.
 */
char g_zClass_NodeArraySizeAlreadySetFmt[0x37] =
    "Error setting node array size; size already set to %d.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodediarg0
 * @recoil-artifact defines .data recoil:data:0x539c9c: g_zClass_CopyNodeDiArg0.
 * Purpose: hold the first display-instance clone argument during node-copy recursion.
 */
int g_zClass_CopyNodeDiArg0 = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodediarg1
 * @recoil-artifact defines .data recoil:data:0x539ca0: g_zClass_CopyNodeDiArg1.
 * Purpose: hold the second display-instance clone argument during node-copy recursion.
 */
int g_zClass_CopyNodeDiArg1 = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-rebuildgwworldbltrectonshutdown
 * @recoil-artifact defines .data recoil:data:0x4de4c4: g_zClass_RebuildGwWorldBltRectOnShutdown.
 * Purpose: gate registration of the GWWorld ZBD settings handler at zClass init.
 */
int g_zClass_RebuildGwWorldBltRectOnShutdown = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-gwworldnodename
 * @recoil-artifact defines .data recoil:data:0x4de530: g_zClass_GWWorldNodeName.
 * Purpose: store the GWWorld ZBD section name used by the class utility handler.
 */
char g_zClass_GWWorldNodeName[8] = "GWWorld";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodezoneiderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de538: g_zClass_CopyNodeZoneIdErrorFmt.
 * Purpose: report failed zone id copies while cloning common node data.
 */
char g_zClass_CopyNodeZoneIdErrorFmt[0x52] =
    "ERROR copying node while setting zone ID.  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeactioncallbackfielderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de58c: g_zClass_CopyNodeActionCallbackFieldErrorFmt.
 * Purpose: report failed action callback copies while cloning common node data.
 */
char g_zClass_CopyNodeActionCallbackFieldErrorFmt[0x5f] =
    "ERROR copying node while setting action callback field  Source "
    "Node: (address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeactioncallbackpriorityfielderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de5ec: g_zClass_CopyNodeActionCallbackPriorityFieldErrorFmt.
 * Purpose: report failed action callback priority copies.
 */
char g_zClass_CopyNodeActionCallbackPriorityFieldErrorFmt[0x69] =
    "ERROR copying node while setting action callback priority field.  "
    "Source Node: (address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeenvironmentdataignoredfmt
 * @recoil-artifact defines .data recoil:data:0x4de658: g_zClass_CopyNodeEnvironmentDataIgnoredFmt.
 * Purpose: warn when source node environment data is intentionally skipped.
 */
char g_zClass_CopyNodeEnvironmentDataIgnoredFmt[0x32] =
    "Source node (%s) has environment data.  Ignoring.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodegraphicsdataerrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de68c: g_zClass_CopyNodeGraphicsDataErrorFmt.
 * Purpose: report failed display-instance copies.
 */
char g_zClass_CopyNodeGraphicsDataErrorFmt[0x4a] =
    "ERROR copying node graphics data.  Source Node: (address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeoverwriteflagerrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de6d8: g_zClass_CopyNodeOverwriteFlagErrorFmt.
 * Purpose: report failed overwrite flag copies.
 */
char g_zClass_CopyNodeOverwriteFlagErrorFmt[0x58] =
    "ERROR copying node while setting overwrite flag  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodedizonecheckflagerrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de730: g_zClass_CopyNodeDiZoneCheckFlagErrorFmt.
 * Purpose: report failed DI zone check flag copies.
 */
char g_zClass_CopyNodeDiZoneCheckFlagErrorFmt[0x5c] =
    "ERROR copying node while setting DI zone check flag  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodecliptoflagerrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de78c: g_zClass_CopyNodeClipToFlagErrorFmt.
 * Purpose: report failed clip_to flag copies.
 */
char g_zClass_CopyNodeClipToFlagErrorFmt[0x56] =
    "ERROR copying node while setting clip_to flag  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodecanmodifyflagerrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de7e4: g_zClass_CopyNodeCanModifyFlagErrorFmt.
 * Purpose: report failed can_modify flag copies.
 */
char g_zClass_CopyNodeCanModifyFlagErrorFmt[0x59] =
    "ERROR copying node while setting can_modify flag  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodelandmarkflagerrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de840: g_zClass_CopyNodeLandmarkFlagErrorFmt.
 * Purpose: report failed landmark flag copies.
 */
char g_zClass_CopyNodeLandmarkFlagErrorFmt[0x57] =
    "ERROR copying node while setting landmark flag  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeproximityflagerrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de898: g_zClass_CopyNodeProximityFlagErrorFmt.
 * Purpose: report failed proximity flag copies.
 */
char g_zClass_CopyNodeProximityFlagErrorFmt[0x58] =
    "ERROR copying node while setting proximity flag  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeintersectbboxfielderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de8f0: g_zClass_CopyNodeIntersectBboxFieldErrorFmt.
 * Purpose: report failed intersect bbox field copies.
 */
char g_zClass_CopyNodeIntersectBboxFieldErrorFmt[0x5f] =
    "ERROR copying node while setting intersect bbox field.  Source "
    "Node: (address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeintersectionfielderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de950: g_zClass_CopyNodeIntersectionFieldErrorFmt.
 * Purpose: report failed intersection field copies.
 */
char g_zClass_CopyNodeIntersectionFieldErrorFmt[0x5d] =
    "ERROR copying node while setting intersection field.  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodealtitudesurfacefielderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de9b0: g_zClass_CopyNodeAltitudeSurfaceFieldErrorFmt.
 * Purpose: report failed altitude surface field copies.
 */
char g_zClass_CopyNodeAltitudeSurfaceFieldErrorFmt[0x61] =
    "ERROR copying node while setting altitude surface field.  Source "
    "Node: (address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeactivefielderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4dea14: g_zClass_CopyNodeActiveFieldErrorFmt.
 * Purpose: report failed active field copies.
 */
char g_zClass_CopyNodeActiveFieldErrorFmt[0x57] =
    "ERROR copying node while setting active field.  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodedescriptionfielderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4dea6c: g_zClass_CopyNodeDescriptionFieldErrorFmt.
 * Purpose: report failed description field copies.
 */
char g_zClass_CopyNodeDescriptionFieldErrorFmt[0x5c] =
    "ERROR copying node while setting description field.  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodebasedataerrormsg
 * @recoil-artifact defines .data recoil:data:0x4deac8: g_zClass_CopyNodeBaseDataErrorMsg.
 * Purpose: report generic common-node-data copy failures.
 */
char g_zClass_CopyNodeBaseDataErrorMsg[0x1e] =
    "ERROR copying node base data.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodesourcenodefmt
 * @recoil-artifact defines .data recoil:data:0x4deae8: g_zClass_CopyNodeSourceNodeFmt.
 * Purpose: append source node address/description context to copy errors.
 */
char g_zClass_CopyNodeSourceNodeFmt[0x29] =
    "  Source Node: (address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copylightnodeunimplementedmsg
 * @recoil-artifact defines .data recoil:data:0x4deb14: g_zClass_CopyLightNodeUnimplementedMsg.
 * Purpose: report the retail unimplemented light-node copy path.
 */
char g_zClass_CopyLightNodeUnimplementedMsg[0x34] =
    "Can't copy light node; Function not yet implemented";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copysoundnodeunimplementedmsg
 * @recoil-artifact defines .data recoil:data:0x4deb48: g_zClass_CopySoundNodeUnimplementedMsg.
 * Purpose: report the retail unimplemented sound-node copy path.
 */
char g_zClass_CopySoundNodeUnimplementedMsg[0x34] =
    "Can't copy sound node; Function not yet implemented";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copyanimatenodeunimplementedmsg
 * @recoil-artifact defines .data recoil:data:0x4deb7c: g_zClass_CopyAnimateNodeUnimplementedMsg.
 * Purpose: report the retail unimplemented animate-node copy path.
 */
char g_zClass_CopyAnimateNodeUnimplementedMsg[0x3a] =
    "ERROR copying animate node; Function not implemented yet.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copysequencenodeunimplementedmsg
 * @recoil-artifact defines .data recoil:data:0x4debb8: g_zClass_CopySequenceNodeUnimplementedMsg.
 * Purpose: report the retail unimplemented sequence-node copy path.
 */
char g_zClass_CopySequenceNodeUnimplementedMsg[0x3b] =
    "ERROR copying sequence node; Function not implemented yet.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copyswitchnodeunimplementedmsg
 * @recoil-artifact defines .data recoil:data:0x4debf4: g_zClass_CopySwitchNodeUnimplementedMsg.
 * Purpose: report the retail switch-node copy stub path.
 */
char g_zClass_CopySwitchNodeUnimplementedMsg[0x39] =
    "ERROR copying switch node; Function not implemented yet.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeunrecognizednodefmt
 * @recoil-artifact defines .data recoil:data:0x4dec30: g_zClass_CopyNodeUnrecognizedNodeFmt.
 * Purpose: report unrecognized node classes during clone dispatch.
 */
char g_zClass_CopyNodeUnrecognizedNodeFmt[0x2f] =
    "ERROR Unrecognized node in copying process: %s";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copyworldclassnodeserrormsg
 * @recoil-artifact defines .data recoil:data:0x4dec60: g_zClass_CopyWorldClassNodesErrorMsg.
 * Purpose: report the retail rejection of world-class node copies.
 */
char g_zClass_CopyWorldClassNodesErrorMsg[0x25] =
    "ERROR cannot copy world class nodes.";
}

namespace {
    const int kDefaultNodeArraySize = 8250;
    const unsigned int kNodeFreeTagIndexMask = 0x00ffffff;

    /**
     * Original-source helper evidence: no standalone retail function is present;
     * observed BBox callers 0x4525d0 and 0x452650 inline the same operation.
     * Evidence: both callers use the same float-bit radius approximation,
     * `bits >> 1` plus 0x1fc00000, after accumulating squared half-extents.
     * Purpose: return the retail approximate range from a squared range.
     */
    float ApproximateRangeFromRangeSq(float rangeSq) {
        int bits = 0;
        memcpy(
            &bits,
            &rangeSq,
            sizeof(bits)
        );
        bits = (bits >> 1) + 0x1fc00000;
        float range = 0.0f;
        memcpy(
            &range,
            &bits,
            sizeof(range)
        );
        return range;
    }
}

namespace zClass {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.setnodearraysize
     * @recoil-artifact defines .text recoil:function:0x4518b0: zClass::SetNodeArraySize.
     * Source owner: engine.zclass.lifecycle_node_array_control.
     * Purpose: set the core zClass node-array capacity before initialization.
     */
    void __fastcall SetNodeArraySize(int size) {
        if (g_zClass_NodeArraySize != 0) {
            zError::ReportOld(
                0x200,
                g_zClass_SourceFile_ClsUtilC,
                0x210,
                g_zClass_NodeArraySizeAlreadySetFmt,
                g_zClass_NodeArraySize
            );
            return;
        }

        g_zClass_NodeArraySize = size;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.shutdown
     * @recoil-artifact defines .text recoil:function:0x4518e0: zClass::Shutdown
     * Source owner: engine.zclass.lifecycle_node_array_control.
     * Purpose: run the core zClass shutdown sequence.
     */
    int Shutdown() {
        ShutdownCore();
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.isinitialized
     * @recoil-artifact defines .text recoil:function:0x4518f0: zClass::IsInitialized.
     * Source owner: engine.zclass.lifecycle_node_array_control.
     * Purpose: return the current zClass initialization flag.
     */
    int IsInitialized() {
        return g_zClass_IsInitialized;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.init
     * @recoil-artifact defines .text recoil:function:0x451900: zClass::Init
     * Source owner: engine.zclass.lifecycle_node_array_control.
     * Purpose: initialize zClass node storage and register the GWWorld ZBD handler.
     */
    int Init() {
        if (g_zClass_NodeArraySize == 0) {
            g_zClass_NodeArraySize = kDefaultNodeArraySize;
        }

        const size_t nodeArrayBytes =
            (size_t)(g_zClass_NodeArraySize) * sizeof(zClass_NodeFreeListSlot);
        g_zClass_NodeArray = (zClass_NodeFreeListSlot *)(malloc(nodeArrayBytes));
        memset(
            g_zClass_NodeArray,
            0,
            nodeArrayBytes
        );

        g_zClass_ActiveNodeCount = 0;
        g_zClass_NodeFreeHeadIndex = 0;
        if (g_zClass_NodeArraySize > 0) {
            for (int i = 0; i < g_zClass_NodeArraySize - 1; ++i) {
                unsigned int freeTag = g_zClass_NodeArray[i].freeTag;
                freeTag = (freeTag & ~kNodeFreeTagIndexMask) |
                          ((unsigned int)(i + 1) & kNodeFreeTagIndexMask);
                g_zClass_NodeArray[i].freeTag = freeTag;
            }
            g_zClass_NodeArray[g_zClass_NodeArraySize - 1].freeTag |= kNodeFreeTagIndexMask;
        }

        if (g_zClass_RebuildGwWorldBltRectOnShutdown != 0) {
            zUtil_ZAR::RegisterSectionHandler(
                g_zClass_GWWorldNodeName,
                (zZbdSectionCallback)(&zClass_World::WriteSettingsSection),
                (zZbdSectionCallback)(&zClass_World::ReadSettingsSection),
                1000,
                0
            );
        }

        g_zClass_IsInitialized = 1;
        return 0;
    }

    /*
     * Source-placement marker: the complete emitted definition belongs to
     * cls_zbd.c; this non-emitting marker preserves the legacy cls_util.c
     * provenance check while ShutdownCore continues to use the declaration.
     */

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.shutdowncore
     * @recoil-artifact defines .text recoil:function:0x451a00: zClass::ShutdownCore.
     * Source owner: engine.zclass.lifecycle_node_array_control.
     * Purpose: tear down zClass-owned nodes, type lists, node storage, and path state.
     */
    int ShutdownCore() {
        zClass_List::DeleteAllOfType(6);
        zClass_TypeList::FreeAll();

        if (g_zClass_NodeArraySize > 0) {
            if (g_zClass_NodeArray != 0) {
                free(g_zClass_NodeArray);
                g_zClass_NodeArray = 0;
            }

            g_zClass_NodeArraySize = 0;
            g_zClass_ActiveNodeCount = 0;
            g_zClass_NodeFreeHeadIndex = -1;
        }

        ResetCurrentZbdPath();
        g_zClass_IsInitialized = 0;
        return 0;
    }

}

namespace zClass_Util {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.destroynoderecursive
     * @recoil-artifact defines .text recoil:function:0x451a60: zClass_Util::DestroyNodeRecursive.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_util.c.
     * Purpose: recursively remove children, release display/class data, and
     * return nodes to the zClass free list.
     */
    int __fastcall DestroyNodeRecursive(zClass_NodePartial * node) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                g_zClass_SourceFile_ClsUtilC,
                0x2b6,
                "Null node pointer."
            );
            return 1;
        }

        if (node->listCountA > 0) {
            return 1;
        }

        if (node->listCountB > 0) {
            for (;;) {
                zClass_NodePartial *child = node->listB[0];
                const int removeResult = zClass_Class::RemoveChild(
                    node,
                    child
                );
                if (removeResult != 0) {
                    return removeResult;
                }

                if (child->listCountA == 0) {
                    const int destroyResult = DestroyNodeRecursive(child);
                    if (destroyResult != 0) {
                        return destroyResult;
                    }
                }

                if (node->listCountB <= 0) {
                    break;
                }
            }
        }

        zDiPartial *displayInstance = (zDiPartial *)((unsigned int)(node->userDataOrDiRef));
        if (displayInstance != 0) {
            const int setResult = zClass_Class::gwNodeSetDisplayInstance(
                node,
                0
            );
            if (setResult != 0) {
                return setResult;
            }
            if (displayInstance->refCount == 0) {
                const int freeResult = zModel_DiPool::FreeIfUnreferenced(displayInstance);
                if (freeResult != 0) {
                    return freeResult;
                }
            }
        }

        return zClass_Class::DeleteNodeByType(node);
    }
}

namespace zClass_cls_util {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copynodedisplayinstance
     * @recoil-artifact defines .text recoil:function:0x451b20: zClass_cls_util::CopyNodeDisplayInstance
     * Purpose: copy or clone a source node's display instance into the destination.
     */
    int __fastcall CopyNodeDisplayInstance(
        zClass_NodePartial * source,
        zClass_NodePartial * dest
    ) {
        int result = 0;

        if (source->userDataOrDiRef == 0) {
            return result;
        }

        unsigned int displayInstanceValue = 0;
        if (g_zClass_CopyNodeCloneDiMode == 0) {
            result = zClass_Class::gwNodeGetUserData(
                source,
                &displayInstanceValue
            );
            if (result == 0) {
                return zClass_Class::gwNodeSetDisplayInstance(
                    dest,
                    (zDiPartial *)((unsigned int)(displayInstanceValue))
                );
            }
            return result;
        }

        unsigned int sourceDisplayInstanceValue = 0;
        result = zClass_Class::gwNodeGetUserData(
            source,
            &sourceDisplayInstanceValue
        );
        if (result != 0) {
            return result;
        }

        zDiPartial *const sourceDisplayInstance =
            (zDiPartial *)((unsigned int)(sourceDisplayInstanceValue));
        displayInstanceValue = sourceDisplayInstanceValue;
        int cloneInstance = 1;
        if (g_zClass_CopyNodeDiArg1 != 0 &&
            zDi::HasSpecialFlagsOrAuxMaterialData(sourceDisplayInstance) == 0) {
            cloneInstance = 0;
        }

        if (cloneInstance == 0) {
            return zClass_Class::gwNodeSetDisplayInstance(
                dest,
                (zDiPartial *)((unsigned int)(displayInstanceValue))
            );
        }

        zDiPartial *const clonedDisplayInstance = zDi::CloneToInstance(
            sourceDisplayInstance,
            g_zClass_CopyNodeDiArg0,
            g_zClass_CopyNodeDiArg1
        );
        if (clonedDisplayInstance == 0) {
            return 1;
        }

        return zClass_Class::gwNodeSetDisplayInstance(
            dest,
            clonedDisplayInstance
        );
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copynodebasedata
     * @recoil-artifact defines .text recoil:function:0x451bd0: zClass_cls_util::CopyNodeBaseData
     * Purpose: copy common node flags, callbacks, type, and graphics state.
     */
    int __fastcall CopyNodeBaseData(
        zClass_NodePartial * source,
        zClass_NodePartial * dest
    ) {
        int result = zClass_Class::gwNodeSetName(
            dest,
            source->name
        );
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_zClass_SourceFile_ClsUtilC,
                0x34e,
                g_zClass_CopyNodeDescriptionFieldErrorFmt,
                source,
                source
            );
            return result;
        }

        result = zClass_Class::gwNodeSetActive(
            dest,
            (source->flags >> 2) & 1
        );
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_zClass_SourceFile_ClsUtilC,
                0x359,
                g_zClass_CopyNodeActiveFieldErrorFmt,
                source,
                source
            );
            return result;
        }

        result = zClass_Class::gwNodeSetCellPickable(
            dest,
            (source->flags >> 3) & 1
        );
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_zClass_SourceFile_ClsUtilC,
                0x364,
                g_zClass_CopyNodeAltitudeSurfaceFieldErrorFmt,
                source,
                source
            );
            return result;
        }

        result = zClass_Class::gwNodeSetRaycastable(
            dest,
            (source->flags >> 4) & 1
        );
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_zClass_SourceFile_ClsUtilC,
                0x36f,
                g_zClass_CopyNodeIntersectionFieldErrorFmt,
                source,
                source
            );
            return result;
        }

        result = zClass_Class::gwNodeSetPickable(
            dest,
            (source->flags >> 5) & 1
        );
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_zClass_SourceFile_ClsUtilC,
                0x37a,
                g_zClass_CopyNodeIntersectBboxFieldErrorFmt,
                source,
                source
            );
            return result;
        }

        result = zClass_Class::gwNodeSetHasHitCallback(
            dest,
            (source->flags >> 6) & 1
        );
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_zClass_SourceFile_ClsUtilC,
                0x385,
                g_zClass_CopyNodeProximityFlagErrorFmt,
                source,
                source
            );
            return result;
        }

        result = zClass_Class::gwNodeSetBypassFarClip(
            dest,
            (source->flags >> 7) & 1
        );
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_zClass_SourceFile_ClsUtilC,
                0x390,
                g_zClass_CopyNodeLandmarkFlagErrorFmt,
                source,
                source
            );
            return result;
        }

        result = zClass_Class::gwNodeSetFlag16(
            dest,
            (source->flags >> 16) & 1
        );
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_zClass_SourceFile_ClsUtilC,
                0x39b,
                g_zClass_CopyNodeCanModifyFlagErrorFmt,
                source,
                source
            );
            return result;
        }

        result = zClass_Class::gwNodeSetFlag17(
            dest,
            (source->flags >> 17) & 1
        );
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_zClass_SourceFile_ClsUtilC,
                0x3a6,
                g_zClass_CopyNodeClipToFlagErrorFmt,
                source,
                source
            );
            return result;
        }

        result = zClass_Class::gwNodeClearVariantGate(
            dest,
            (source->flags >> 24) & 1
        );
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_zClass_SourceFile_ClsUtilC,
                0x3b1,
                g_zClass_CopyNodeDiZoneCheckFlagErrorFmt,
                source,
                source
            );
            return result;
        }

        result = zClass_Class::gwNodeSetVertexAlphaOverride(
            dest,
            (source->flags >> 23) & 1
        );
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_zClass_SourceFile_ClsUtilC,
                0x3bc,
                g_zClass_CopyNodeOverwriteFlagErrorFmt,
                source,
                source
            );
            return result;
        }

        dest->flags |= source->flags & 0x70000000;
        dest->auxFlags = source->auxFlags;

        result = CopyNodeDisplayInstance(
            source,
            dest
        );
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_zClass_SourceFile_ClsUtilC,
                0x3ce,
                g_zClass_CopyNodeGraphicsDataErrorFmt,
                source,
                source
            );
            return result;
        }

        if (source->callbackContext != 0) {
            zError::ReportOld(
                0x100,
                g_zClass_SourceFile_ClsUtilC,
                0x3d8,
                g_zClass_CopyNodeEnvironmentDataIgnoredFmt,
                source
            );
        }
        dest->callbackContext = 0;

        result = zClass_Class::gwNodeSetPriority(
            dest,
            source->callbackPriority
        );
        if (result != 0) {
            zError::ReportOld(
                0x100,
                g_zClass_SourceFile_ClsUtilC,
                0x3e2,
                g_zClass_CopyNodeActionCallbackPriorityFieldErrorFmt,
                source,
                source
            );
            return result;
        }

        result = zClass_Class::gwNodeSetActionCallback(
            dest,
            source->actionCallback
        );
        if (result != 0) {
            zError::ReportOld(
                0x100,
                g_zClass_SourceFile_ClsUtilC,
                0x3ed,
                g_zClass_CopyNodeActionCallbackFieldErrorFmt,
                source,
                source
            );
            return result;
        }

        result = zClass_Class::gwNodeSetNodeType(
            dest,
            source->nodeType
        );
        if (result != 0) {
            zError::ReportOld(
                0x100,
                g_zClass_SourceFile_ClsUtilC,
                0x3f8,
                g_zClass_CopyNodeZoneIdErrorFmt,
                source,
                source
            );
            return result;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copycameranode
     * @recoil-artifact defines .text recoil:function:0x451f70: zClass_cls_util::CopyCameraNode
     * Purpose: allocate and populate a copied camera node and its copied children.
     */
    zClass_NodePartial *__fastcall CopyCameraNode(
        zClass_NodePartial * source
    ) {
        zClass_NodePartial *const camera = zClass_Camera::gwCameraNew();
        if (camera == 0) {
            return camera;
        }

        if (CopyNodeBaseData(
            source,
            camera
        ) != 0) {
            zError::ReportOld(
                0x100,
                g_zClass_SourceFile_ClsUtilC,
                0x422,
                g_zClass_CopyNodeBaseDataErrorMsg,
                g_zClass_CopyNodeSourceNodeFmt,
                source,
                source
            );
            return 0;
        }

        zClass_CameraDataPartial *const data = (zClass_CameraDataPartial *)(source->classData);
        if (zClass_Camera::gwCameraSetWorld(
            camera,
            data->worldNode
        ) != 0) {
            return 0;
        }
        if (zClass_Camera::gwCameraSetWindow(
            camera,
            data->windowNode
        ) != 0) {
            return 0;
        }
        if (zClass_Camera::gwCameraSetTarget(
                camera,
                data->targetOrEuler.x,
                data->targetOrEuler.y,
                data->targetOrEuler.z
            ) != 0) {
            return 0;
        }
        if (zClass_Camera::gwCameraSetPosition(
                camera,
                data->posOffset.x,
                data->posOffset.y,
                data->posOffset.z
            ) != 0) {
            return 0;
        }
        if (zClass_Camera::gwCameraSetNearFarClip(
            camera,
            data->nearClip,
            data->farClip
        ) != 0) {
            return 0;
        }
        if (zClass_Camera::gwCameraSetClipDistance(
            camera,
            data->clipDistance
        ) != 0) {
            return 0;
        }
        if (zClass_Camera::gwCameraSetFOV(
            camera,
            data->fovX,
            data->fovY
        ) != 0) {
            return 0;
        }

        for (int i = 0; i < source->listCountB; ++i) {
            zClass_NodePartial *const child = CopyNodeDispatch(source->listB[i]);
            if (child == 0 || zClass_Camera::gwCameraAddChild(
                camera,
                child
            ) != 0) {
                return 0;
            }
        }

        return camera;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copylightnode-unimplemented
     * @recoil-artifact defines .text recoil:function:0x4520c0: zClass_cls_util::CopyLightNode_Unimplemented
     * Purpose: preserve the retail unimplemented light-node copy path.
     */
    zClass_NodePartial *__fastcall CopyLightNode_Unimplemented(
        zClass_NodePartial *
    ) {
        zError::ReportOld(
            0x800,
            g_zClass_SourceFile_ClsUtilC,
            0x47d,
            g_zClass_CopyLightNodeUnimplementedMsg
        );
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copysoundnode-unimplemented
     * @recoil-artifact defines .text recoil:function:0x4520e0: zClass_cls_util::CopySoundNode_Unimplemented
     * Purpose: preserve the retail unimplemented sound-node copy path.
     */
    zClass_NodePartial *__fastcall CopySoundNode_Unimplemented(
        zClass_NodePartial *
    ) {
        zError::ReportOld(
            0x800,
            g_zClass_SourceFile_ClsUtilC,
            0x493,
            g_zClass_CopySoundNodeUnimplementedMsg
        );
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copyobject3dnode
     * @recoil-artifact defines .text recoil:function:0x452100: zClass_cls_util::CopyObject3DNode
     * Purpose: allocate and populate a copied Object3D node and its copied children.
     */
    zClass_NodePartial *__fastcall CopyObject3DNode(
        zClass_NodePartial * source
    ) {
        zClass_NodePartial *const parent = zClass_Object3D::gwObject3DInit();
        if (parent == 0) {
            return parent;
        }

        if (CopyNodeBaseData(
            source,
            parent
        ) != 0) {
            zError::ReportOld(
                0x100,
                g_zClass_SourceFile_ClsUtilC,
                0x4b9,
                g_zClass_CopyNodeBaseDataErrorMsg,
                g_zClass_CopyNodeSourceNodeFmt,
                source,
                source
            );
            return 0;
        }

        zClass_Object3DDataPartial *const data = (zClass_Object3DDataPartial *)(source->classData);
        if (zClass_Object3D::gwObject3DSetAlphaScale(
            parent,
            data->alphaScale
        ) != 0) {
            return 0;
        }
        if (zClass_Object3D::gwObject3DSetLitFlag(
            parent,
            (data->flags >> 1) & 1
        ) != 0) {
            return 0;
        }

        if ((data->flags & 0x08) == 0) {
            if ((data->flags & 0x10) != 0) {
                if (zClass_Object3D::gwObject3DSetMatrix(
                    parent,
                    data->localMatrix
                ) != 0) {
                    return 0;
                }
            } else {
                if (zClass_Object3D::gwObject3DSetPosition(
                        parent,
                        data->localMatrix[9],
                        data->localMatrix[10],
                        data->localMatrix[11]
                    ) != 0) {
                    return 0;
                }
                if (zClass_Object3D::gwObject3DSetRotation(
                        parent,
                        data->rotation.x,
                        data->rotation.y,
                        data->rotation.z
                    ) != 0) {
                    return 0;
                }
                if (zClass_Object3D::gwObject3DSetScale(
                        parent,
                        data->scale.x,
                        data->scale.y,
                        data->scale.z
                    ) != 0) {
                    return 0;
                }
            }
        }

        for (int i = 0; i < source->listCountB; ++i) {
            zClass_NodePartial *const child = CopyNodeDispatch(source->listB[i]);
            if (child != 0 && zClass_Object3D::gwObject3DAddChild(
                parent,
                child
            ) != 0) {
                return 0;
            }
        }

        return parent;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copyanimatenode-unimplemented
     * @recoil-artifact defines .text recoil:function:0x452230: zClass_cls_util::CopyAnimateNode_Unimplemented
     * Purpose: preserve the retail unimplemented animate-node copy path.
     */
    zClass_NodePartial *__fastcall CopyAnimateNode_Unimplemented(
        zClass_NodePartial *
    ) {
        zError::ReportOld(
            0x100,
            g_zClass_SourceFile_ClsUtilC,
            0x518,
            g_zClass_CopyAnimateNodeUnimplementedMsg
        );
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copylodnode
     * @recoil-artifact defines .text recoil:function:0x452250: zClass_cls_util::CopyLodNode
     * Purpose: allocate and populate a copied LOD node and its copied children.
     */
    zClass_NodePartial *__fastcall CopyLodNode(zClass_NodePartial * source) {
        zClass_NodePartial *const parent = zClass_Lod::gwLodNew();
        if (parent == 0) {
            return parent;
        }

        if (CopyNodeBaseData(
            source,
            parent
        ) != 0) {
            zError::ReportOld(
                0x100,
                g_zClass_SourceFile_ClsUtilC,
                0x53e,
                g_zClass_CopyNodeBaseDataErrorMsg,
                g_zClass_CopyNodeSourceNodeFmt,
                source,
                source
            );
            return 0;
        }

        zClass_LodDataPartial *const sourceData = (zClass_LodDataPartial *)(source->classData);
        if (zClass_Lod::SetComputeOwnDistance(
            parent,
            sourceData->computeOwnDistance
        ) != 0) {
            return 0;
        }

        zClass_LodDataPartial *const destData = (zClass_LodDataPartial *)(parent->classData);
        destData->nearRangeSq = sourceData->nearRangeSq;
        destData->nearRange = sourceData->nearRange;
        destData->farRangeSq = sourceData->farRangeSq;
        destData->fadeWidth = sourceData->fadeWidth;
        destData->fadeAmount = sourceData->fadeAmount;
        destData->fadeEndScale = sourceData->fadeEndScale;
        destData->fogFadeWidth = sourceData->fogFadeWidth;
        destData->fogFadeAmount = sourceData->fogFadeAmount;
        destData->fogStartDist = sourceData->fogStartDist;
        destData->vertexShadingAmount = sourceData->vertexShadingAmount;
        destData->active = sourceData->active;

        if (zClass_Lod::SetTargetNodeAndRange(
                parent,
                sourceData->rangeNode,
                ApproximateRangeFromRangeSq(sourceData->rangeSq)
            ) != 0) {
            return 0;
        }

        for (int i = 0; i < source->listCountB; ++i) {
            zClass_NodePartial *const child = CopyNodeDispatch(source->listB[i]);
            if (child == 0 || zClass_Lod::gwLodAddChild(
                parent,
                child
            ) != 0) {
                return 0;
            }
        }

        return parent;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copysequencenode-unimplemented
     * @recoil-artifact defines .text recoil:function:0x4523c0: zClass_cls_util::CopySequenceNode_Unimplemented
     * Purpose: preserve the retail unimplemented sequence-node copy path.
     */
    zClass_NodePartial *__fastcall CopySequenceNode_Unimplemented(
        zClass_NodePartial *
    ) {
        zError::ReportOld(
            0x100,
            g_zClass_SourceFile_ClsUtilC,
            0x585,
            g_zClass_CopySequenceNodeUnimplementedMsg
        );
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copyswitchnode-stub
     * @recoil-artifact defines .text recoil:function:0x4523e0: zClass_cls_util::CopySwitchNode_Stub
     * Purpose: preserve the retail switch-node copy stub behavior.
     */
    zClass_NodePartial *__fastcall CopySwitchNode_Stub(zClass_NodePartial *) {
        zError::ReportOld(
            0x100,
            g_zClass_SourceFile_ClsUtilC,
            0x59c,
            g_zClass_CopySwitchNodeUnimplementedMsg
        );
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copynodedispatch
     * @recoil-artifact defines .text recoil:function:0x452400: zClass_cls_util::CopyNodeDispatch
     * Purpose: dispatch node-copy work by class id.
     */
    zClass_NodePartial *__fastcall CopyNodeDispatch(
        zClass_NodePartial * source
    ) {
        if (source == 0) {
            zError::ReportOld(
                0x400,
                g_zClass_SourceFile_ClsUtilC,
                0x5b8,
                "Null node pointer."
            );
            return 0;
        }

        if ((source->flags & 0x04000000) != 0) {
            return source;
        }

        switch (source->classId) {
        case 1:
            return CopyCameraNode(source);
        case 2:
            zError::ReportOld(
                0x100,
                g_zClass_SourceFile_ClsUtilC,
                0x5e1,
                g_zClass_CopyWorldClassNodesErrorMsg
            );
            return 0;
        case 5:
            return CopyObject3DNode(source);
        case 6:
            return CopyLodNode(source);
        case 7:
            CopySequenceNode_Unimplemented(source);
            return 0;
        case 8:
            CopyAnimateNode_Unimplemented(source);
            return 0;
        case 9:
            CopyLightNode_Unimplemented(source);
            return 0;
        case 10:
            CopySoundNode_Unimplemented(source);
            return 0;
        case 11:
            CopySwitchNode_Stub(source);
            return 0;
        default:
            zError::ReportOld(
                0x100,
                g_zClass_SourceFile_ClsUtilC,
                0x5e8,
                g_zClass_CopyNodeUnrecognizedNodeFmt,
                source
            );
            return 0;
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copynodewithcloneoptions
     * @recoil-artifact defines .text recoil:function:0x452500: zClass_cls_util::CopyNodeWithCloneOptions
     * Purpose: copy a node while temporarily overriding clone-mode globals.
     */
    zClass_NodePartial *__fastcall CopyNodeWithCloneOptions(
        zClass_NodePartial * source,
        int cloneDiMode,
        int diArg0
    ) {
        if (source == 0) {
            zError::ReportOld(
                0x400,
                g_zClass_SourceFile_ClsUtilC,
                0x60f,
                "Null node pointer."
            );
            return 0;
        }

        const int savedDiArg0 = g_zClass_CopyNodeDiArg0;
        const int savedCloneDiMode = g_zClass_CopyNodeCloneDiMode;
        g_zClass_CopyNodeCloneDiMode = cloneDiMode;
        g_zClass_CopyNodeDiArg0 = diArg0;

        zClass_NodePartial *const result = CopyNodeDispatch(source);
        g_zClass_CopyNodeCloneDiMode = savedCloneDiMode;
        g_zClass_CopyNodeDiArg0 = savedDiArg0;
        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copynode
     * @recoil-artifact defines .text recoil:function:0x452560: zClass_cls_util::CopyNode
     * Purpose: copy a node while temporarily overriding all display-instance clone options.
     */
    zClass_NodePartial *__fastcall CopyNode(
        zClass_NodePartial * source,
        int cloneDiMode,
        int diArg0,
        int diArg1
    ) {
        if (source == 0) {
            zError::ReportOld(
                0x400,
                g_zClass_SourceFile_ClsUtilC,
                0x648,
                "Null node pointer."
            );
            return 0;
        }

        const int savedDiArg0 = g_zClass_CopyNodeDiArg0;
        const int savedCloneDiMode = g_zClass_CopyNodeCloneDiMode;
        const int savedDiArg1 = g_zClass_CopyNodeDiArg1;
        g_zClass_CopyNodeCloneDiMode = cloneDiMode;
        g_zClass_CopyNodeDiArg0 = diArg0;
        g_zClass_CopyNodeDiArg1 = diArg1;

        zClass_NodePartial *const result = CopyNodeDispatch(source);
        g_zClass_CopyNodeDiArg1 = savedDiArg1;
        g_zClass_CopyNodeCloneDiMode = savedCloneDiMode;
        g_zClass_CopyNodeDiArg0 = savedDiArg0;
        return result;
    }
}

namespace BBox {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.minmaxtoboundingsphere
     * @recoil-artifact defines .text recoil:function:0x4525d0: BBox::MinMaxToBoundingSphere.
     * Purpose: write the min/max bbox center and retail approximate
     * bounding-sphere radius.
     */
    float *__fastcall MinMaxToBoundingSphere(
        const zBBox3f *bbox,
        zVec3 *outCenter,
        float *outRadius
    ) {
        const float halfX = (bbox->maxX - bbox->minX) * 0.5f;
        const float halfY = (bbox->maxY - bbox->minY) * 0.5f;
        const float halfZ = (bbox->maxZ - bbox->minZ) * 0.5f;
        outCenter->x = bbox->minX + halfX;
        outCenter->y = bbox->minY + halfY;
        outCenter->z = bbox->minZ + halfZ;

        *outRadius = ApproximateRangeFromRangeSq(halfX * halfX + halfY * halfY + halfZ * halfZ);
        return outRadius;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.cornerstoboundingsphere
     * @recoil-artifact defines .text recoil:function:0x452650: BBox::CornersToBoundingSphere.
     * Purpose: scan eight bbox corners, write the center, and write the
     * retail approximate bounding-sphere radius.
     */
    void __fastcall CornersToBoundingSphere(
        zBBoxCorners * corners,
        zVec3 * outCenter,
        float *outRadius
    ) {
        const zVec3 *corner = (const zVec3 *)corners->values;
        float minX = corner[0].x;
        float maxX = corner[0].x;
        float minY = corner[0].y;
        float maxY = corner[0].y;
        float minZ = corner[0].z;
        float maxZ = corner[0].z;

        for (int i = 1; i < 8; ++i) {
            if (corner[i].x < minX) {
                minX = corner[i].x;
            } else if (corner[i].x > maxX) {
                maxX = corner[i].x;
            }
            if (corner[i].y < minY) {
                minY = corner[i].y;
            } else if (corner[i].y > maxY) {
                maxY = corner[i].y;
            }
            if (corner[i].z < minZ) {
                minZ = corner[i].z;
            } else if (corner[i].z > maxZ) {
                maxZ = corner[i].z;
            }
        }

        const float halfX = (maxX - minX) * 0.5f;
        const float halfY = (maxY - minY) * 0.5f;
        const float halfZ = (maxZ - minZ) * 0.5f;
        outCenter->x = minX + halfX;
        outCenter->y = minY + halfY;
        outCenter->z = minZ + halfZ;
        *outRadius = ApproximateRangeFromRangeSq(halfX * halfX + halfY * halfY + halfZ * halfZ);
    }
}

namespace zClass_Class {
    /**
     * Purpose: recursively search a node subtree by name, checking the root
     * first and then visiting child-list entries from tail to head.
     */
    zClass_NodePartial *__fastcall FindSubNodeByName(
        zClass_NodePartial * root,
        const char *name
    ) {
        if (root == 0) {
            return 0;
        }
        if (strcmp(
            name,
            root->name
        ) == 0) {
            return root;
        }

        for (int i = root->listCountB - 1; i >= 0; --i) {
            zClass_NodePartial *found = FindSubNodeByName(
                root->listB[i],
                name
            );
            if (found != 0) {
                return found;
            }
        }

        return 0;
    }
}

namespace zClass_Node {
    /**
     * Purpose: test whether a node's DI reference points to a renderable display
     * instance mode without the hidden flag.
     */
    int __fastcall HasRenderableDiPredicate(zClass_NodePartial * node) {
        ::zDiPartial *di = (::zDiPartial *)((unsigned int)(node->userDataOrDiRef));
        if (di == 0) {
            return 0;
        }

        if (di->mode == 1 && (di->flags & 0x10) == 0) {
            return 1;
        }

        return 0;
    }
}

namespace zClass {
    /**
     * Purpose: recursively test a node and its secondary children with a
     * caller-supplied predicate.
     */
    int __fastcall AnyNodeMatchesPredicateRecursive(
        zClass_NodePartial * root,
        zClass_NodePredicate predicate
    ) {
        if (predicate(root) == 1) {
            return 1;
        }

        for (int i = root->listCountB - 1; i >= 0; --i) {
            if (AnyNodeMatchesPredicateRecursive(
                root->listB[i],
                predicate
            ) == 1) {
                return 1;
            }
        }

        return 0;
    }
}

namespace zClass_Node {
    /**
     * Purpose: recurse a child-list subtree and propagate material flag bit 9
     * updates through each node display instance.
     */
    void __fastcall SetMaterialFlagBit9ForFlagBit0EntriesRecursive(
        zClass_NodePartial * node,
        int enabled
    ) {
        zDiPartial *di = (zDiPartial *)((unsigned int)(node->userDataOrDiRef));
        if (di != 0) {
            zDi::SetMaterialFlagBit9ForFlagBit0Entries(
                di,
                enabled
            );
        }

        for (int i = 0; i < node->listCountB; ++i) {
            SetMaterialFlagBit9ForFlagBit0EntriesRecursive(
                node->listB[i],
                enabled
            );
        }
    }

    /**
     * Purpose: invalidate flagged material images under a node subtree and
     * then load pending texture-directory entries.
     */
    void __fastcall LoadFlagBit8MaterialImagesAndTexturePack(
        zClass_NodePartial * node
    ) {
        if (node == 0) {
            return;
        }

        InvalidateFlagBit8MaterialImagesRecursive(node);
        zImage::TexDir_LoadPendingEntries();
    }

    /**
     * Purpose: recurse a child-list subtree and invalidate loaded material
     * image variants for each display instance with material flag bit 8 set.
     */
    void __fastcall InvalidateFlagBit8MaterialImagesRecursive(
        zClass_NodePartial * node
    ) {
        zDiPartial *di = (zDiPartial *)((unsigned int)(node->userDataOrDiRef));
        if (di != 0) {
            zDi::InvalidateImagesForFlagBit8Materials(di);
        }

        for (int i = 0; i < node->listCountB; ++i) {
            InvalidateFlagBit8MaterialImagesRecursive(node->listB[i]);
        }
    }

    /**
     * Purpose: assign display-instance flag bit 0 for each display instance
     * reachable through a node's child-list subtree.
     */
    void __fastcall AssignInt32ToDiRecursive(
        zClass_NodePartial * node,
        int value
    ) {
        zDiPartial *di = (zDiPartial *)((unsigned int)(node->userDataOrDiRef));
        if (di != 0) {
            zDi::SetFlagBit0(
                di,
                value
            );
        }

        for (int i = 0; i < node->listCountB; ++i) {
            AssignInt32ToDiRecursive(
                node->listB[i],
                value
            );
        }
    }
}
