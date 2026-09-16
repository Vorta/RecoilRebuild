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
 * @recoil-artifact defines .data recoil:data:0x539c90: g_CZClass_NodeArraySize.
 * Purpose: track the configured zClass node free-list capacity.
 */
int g_CZClass_NodeArraySize = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-isinitialized
 * @recoil-artifact defines .data recoil:data:0x539ca4: g_CZClass_IsInitialized.
 * Purpose: track whether the core zClass utility subsystem is initialized.
 */
int g_CZClass_IsInitialized = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeclonedimode
 * @recoil-artifact defines .data recoil:data:0x4de4cc: g_CZClass_CopyNodeCloneDiMode.
 * Purpose: hold the active display-instance clone mode during node-copy recursion.
 */
int g_CZClass_CopyNodeCloneDiMode = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-sourcefile-clsutilc
 * @recoil-artifact defines .data recoil:data:0x4de4d0: g_CZClass_SourceFile_ClsUtilC.
 * Purpose: store the recovered cls_util.c source path used by zError reports.
 */
char g_CZClass_SourceFile_ClsUtilC[0x26] =
    "D:\\Proj\\GameZRecoil\\zClass\\cls_util.c";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-nodearraysizealreadysetfmt
 * @recoil-artifact defines .data recoil:data:0x4de4f8: g_CZClass_NodeArraySizeAlreadySetFmt.
 * Purpose: report attempts to resize zClass node storage after configuration.
 */
char g_CZClass_NodeArraySizeAlreadySetFmt[0x37] =
    "Error setting node array size; size already set to %d.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodediarg0
 * @recoil-artifact defines .data recoil:data:0x539c9c: g_CZClass_CopyNodeDiArg0.
 * Purpose: hold the first display-instance clone argument during node-copy recursion.
 */
int g_CZClass_CopyNodeDiArg0 = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodediarg1
 * @recoil-artifact defines .data recoil:data:0x539ca0: g_CZClass_CopyNodeDiArg1.
 * Purpose: hold the second display-instance clone argument during node-copy recursion.
 */
int g_CZClass_CopyNodeDiArg1 = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-rebuildgwworldbltrectonshutdown
 * @recoil-artifact defines .data recoil:data:0x4de4c4: g_CZClass_RebuildGwWorldBltRectOnShutdown.
 * Purpose: gate registration of the GWWorld ZBD settings handler at zClass init.
 */
int g_CZClass_RebuildGwWorldBltRectOnShutdown = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-gwworldnodename
 * @recoil-artifact defines .data recoil:data:0x4de530: g_CZClass_GWWorldNodeName.
 * Purpose: store the GWWorld ZBD section name used by the class utility handler.
 */
char g_CZClass_GWWorldNodeName[8] = "GWWorld";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodezoneiderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de538: g_CZClass_CopyNodeZoneIdErrorFmt.
 * Purpose: report failed zone id copies while cloning common node data.
 */
char g_CZClass_CopyNodeZoneIdErrorFmt[0x52] =
    "ERROR copying node while setting zone ID.  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeactioncallbackfielderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de58c: g_CZClass_CopyNodeActionCallbackFieldErrorFmt.
 * Purpose: report failed action callback copies while cloning common node data.
 */
char g_CZClass_CopyNodeActionCallbackFieldErrorFmt[0x5f] =
    "ERROR copying node while setting action callback field  Source "
    "Node: (address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeactioncallbackpriorityfielderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de5ec: g_CZClass_CopyNodeActionCallbackPriorityFieldErrorFmt.
 * Purpose: report failed action callback priority copies.
 */
char g_CZClass_CopyNodeActionCallbackPriorityFieldErrorFmt[0x69] =
    "ERROR copying node while setting action callback priority field.  "
    "Source Node: (address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeenvironmentdataignoredfmt
 * @recoil-artifact defines .data recoil:data:0x4de658: g_CZClass_CopyNodeEnvironmentDataIgnoredFmt.
 * Purpose: warn when source node environment data is intentionally skipped.
 */
char g_CZClass_CopyNodeEnvironmentDataIgnoredFmt[0x32] =
    "Source node (%s) has environment data.  Ignoring.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodegraphicsdataerrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de68c: g_CZClass_CopyNodeGraphicsDataErrorFmt.
 * Purpose: report failed display-instance copies.
 */
char g_CZClass_CopyNodeGraphicsDataErrorFmt[0x4a] =
    "ERROR copying node graphics data.  Source Node: (address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeoverwriteflagerrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de6d8: g_CZClass_CopyNodeOverwriteFlagErrorFmt.
 * Purpose: report failed overwrite flag copies.
 */
char g_CZClass_CopyNodeOverwriteFlagErrorFmt[0x58] =
    "ERROR copying node while setting overwrite flag  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodedizonecheckflagerrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de730: g_CZClass_CopyNodeDiZoneCheckFlagErrorFmt.
 * Purpose: report failed DI zone check flag copies.
 */
char g_CZClass_CopyNodeDiZoneCheckFlagErrorFmt[0x5c] =
    "ERROR copying node while setting DI zone check flag  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodecliptoflagerrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de78c: g_CZClass_CopyNodeClipToFlagErrorFmt.
 * Purpose: report failed clip_to flag copies.
 */
char g_CZClass_CopyNodeClipToFlagErrorFmt[0x56] =
    "ERROR copying node while setting clip_to flag  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodecanmodifyflagerrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de7e4: g_CZClass_CopyNodeCanModifyFlagErrorFmt.
 * Purpose: report failed can_modify flag copies.
 */
char g_CZClass_CopyNodeCanModifyFlagErrorFmt[0x59] =
    "ERROR copying node while setting can_modify flag  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodelandmarkflagerrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de840: g_CZClass_CopyNodeLandmarkFlagErrorFmt.
 * Purpose: report failed landmark flag copies.
 */
char g_CZClass_CopyNodeLandmarkFlagErrorFmt[0x57] =
    "ERROR copying node while setting landmark flag  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeproximityflagerrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de898: g_CZClass_CopyNodeProximityFlagErrorFmt.
 * Purpose: report failed proximity flag copies.
 */
char g_CZClass_CopyNodeProximityFlagErrorFmt[0x58] =
    "ERROR copying node while setting proximity flag  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeintersectbboxfielderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de8f0: g_CZClass_CopyNodeIntersectBboxFieldErrorFmt.
 * Purpose: report failed intersect bbox field copies.
 */
char g_CZClass_CopyNodeIntersectBboxFieldErrorFmt[0x5f] =
    "ERROR copying node while setting intersect bbox field.  Source "
    "Node: (address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeintersectionfielderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de950: g_CZClass_CopyNodeIntersectionFieldErrorFmt.
 * Purpose: report failed intersection field copies.
 */
char g_CZClass_CopyNodeIntersectionFieldErrorFmt[0x5d] =
    "ERROR copying node while setting intersection field.  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodealtitudesurfacefielderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4de9b0: g_CZClass_CopyNodeAltitudeSurfaceFieldErrorFmt.
 * Purpose: report failed altitude surface field copies.
 */
char g_CZClass_CopyNodeAltitudeSurfaceFieldErrorFmt[0x61] =
    "ERROR copying node while setting altitude surface field.  Source "
    "Node: (address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeactivefielderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4dea14: g_CZClass_CopyNodeActiveFieldErrorFmt.
 * Purpose: report failed active field copies.
 */
char g_CZClass_CopyNodeActiveFieldErrorFmt[0x57] =
    "ERROR copying node while setting active field.  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodedescriptionfielderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4dea6c: g_CZClass_CopyNodeDescriptionFieldErrorFmt.
 * Purpose: report failed description field copies.
 */
char g_CZClass_CopyNodeDescriptionFieldErrorFmt[0x5c] =
    "ERROR copying node while setting description field.  Source Node: "
    "(address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodebasedataerrormsg
 * @recoil-artifact defines .data recoil:data:0x4deac8: g_CZClass_CopyNodeBaseDataErrorMsg.
 * Purpose: report generic common-node-data copy failures.
 */
char g_CZClass_CopyNodeBaseDataErrorMsg[0x1e] =
    "ERROR copying node base data.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodesourcenodefmt
 * @recoil-artifact defines .data recoil:data:0x4deae8: g_CZClass_CopyNodeSourceNodeFmt.
 * Purpose: append source node address/description context to copy errors.
 */
char g_CZClass_CopyNodeSourceNodeFmt[0x29] =
    "  Source Node: (address =%x) (desc = %s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copylightnodeunimplementedmsg
 * @recoil-artifact defines .data recoil:data:0x4deb14: g_CZClass_CopyLightNodeUnimplementedMsg.
 * Purpose: report the retail unimplemented light-node copy path.
 */
char g_CZClass_CopyLightNodeUnimplementedMsg[0x34] =
    "Can't copy light node; Function not yet implemented";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copysoundnodeunimplementedmsg
 * @recoil-artifact defines .data recoil:data:0x4deb48: g_CZClass_CopySoundNodeUnimplementedMsg.
 * Purpose: report the retail unimplemented sound-node copy path.
 */
char g_CZClass_CopySoundNodeUnimplementedMsg[0x34] =
    "Can't copy sound node; Function not yet implemented";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copyanimatenodeunimplementedmsg
 * @recoil-artifact defines .data recoil:data:0x4deb7c: g_CZClass_CopyAnimateNodeUnimplementedMsg.
 * Purpose: report the retail unimplemented animate-node copy path.
 */
char g_CZClass_CopyAnimateNodeUnimplementedMsg[0x3a] =
    "ERROR copying animate node; Function not implemented yet.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copysequencenodeunimplementedmsg
 * @recoil-artifact defines .data recoil:data:0x4debb8: g_CZClass_CopySequenceNodeUnimplementedMsg.
 * Purpose: report the retail unimplemented sequence-node copy path.
 */
char g_CZClass_CopySequenceNodeUnimplementedMsg[0x3b] =
    "ERROR copying sequence node; Function not implemented yet.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copyswitchnodeunimplementedmsg
 * @recoil-artifact defines .data recoil:data:0x4debf4: g_CZClass_CopySwitchNodeUnimplementedMsg.
 * Purpose: report the retail switch-node copy stub path.
 */
char g_CZClass_CopySwitchNodeUnimplementedMsg[0x39] =
    "ERROR copying switch node; Function not implemented yet.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copynodeunrecognizednodefmt
 * @recoil-artifact defines .data recoil:data:0x4dec30: g_CZClass_CopyNodeUnrecognizedNodeFmt.
 * Purpose: report unrecognized node classes during clone dispatch.
 */
char g_CZClass_CopyNodeUnrecognizedNodeFmt[0x2f] =
    "ERROR Unrecognized node in copying process: %s";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.g-zclass-copyworldclassnodeserrormsg
 * @recoil-artifact defines .data recoil:data:0x4dec60: g_CZClass_CopyWorldClassNodesErrorMsg.
 * Purpose: report the retail rejection of world-class node copies.
 */
char g_CZClass_CopyWorldClassNodesErrorMsg[0x25] =
    "ERROR cannot copy world class nodes.";
}

namespace {
    const int kDefaultNodeArraySize = 8250;
    const unsigned int kNodeFreeTagIndexMask = 0x00ffffff;
}

namespace CZClass {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.setnodearraysize
     * @recoil-artifact defines .text recoil:function:0x4518b0: CZClass::SetNodeArraySize.
     * Source owner: engine.zclass.lifecycle_node_array_control.
     * Purpose: set the core zClass node-array capacity before initialization.
     */
    void __fastcall SetNodeArraySize(int size) {
        if (g_CZClass_NodeArraySize != 0) {
            zError::ReportOld(
                0x200,
                g_CZClass_SourceFile_ClsUtilC,
                0x210,
                g_CZClass_NodeArraySizeAlreadySetFmt,
                g_CZClass_NodeArraySize
            );
            return;
        }

        g_CZClass_NodeArraySize = size;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.shutdown
     * @recoil-artifact defines .text recoil:function:0x4518e0: CZClass::Shutdown
     * @recoil-match byte
     *
     * Source owner: engine.zclass.lifecycle_node_array_control.
     * Purpose: run the core zClass shutdown sequence.
     */
    int __cdecl Shutdown() {
        ShutdownCore();
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.isinitialized
     * @recoil-artifact defines .text recoil:function:0x4518f0: CZClass::IsInitialized.
     * @recoil-match byte
     *
     * Source owner: engine.zclass.lifecycle_node_array_control.
     * Purpose: return the current zClass initialization flag.
     */
    int __cdecl IsInitialized() {
        return g_CZClass_IsInitialized;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.init
     * @recoil-artifact defines .text recoil:function:0x451900: CZClass::Init
     * Source owner: engine.zclass.lifecycle_node_array_control.
     * Purpose: initialize zClass node storage and register the GWWorld ZBD handler.
     */
    int __cdecl Init() {
        if (g_CZClass_NodeArraySize == 0) {
            g_CZClass_NodeArraySize = kDefaultNodeArraySize;
        }

        g_CZClass_NodeArray = (CZNodeFreeListSlot *)(malloc(
            (size_t)g_CZClass_NodeArraySize * sizeof(CZNodeFreeListSlot)));
        memset(g_CZClass_NodeArray, 0,
            (size_t)g_CZClass_NodeArraySize * sizeof(CZNodeFreeListSlot));

        g_CZClass_ActiveNodeCount = 0;
        g_CZClass_NodeFreeHeadIndex = 0;
        if (g_CZClass_NodeArraySize > 0) {
            for (int i = 0; i < g_CZClass_NodeArraySize - 1; ++i) {
                unsigned int freeTag = g_CZClass_NodeArray[i].freeTag;
                freeTag = (freeTag & ~kNodeFreeTagIndexMask) |
                          ((unsigned int)(i + 1) & kNodeFreeTagIndexMask);
                g_CZClass_NodeArray[i].freeTag = freeTag;
            }
            g_CZClass_NodeArray[g_CZClass_NodeArraySize - 1].freeTag |= kNodeFreeTagIndexMask;
        }

        if (g_CZClass_RebuildGwWorldBltRectOnShutdown != 0) {
            zUtil_ZAR::RegisterSectionHandler(
                g_CZClass_GWWorldNodeName,
                (zZbdSectionCallback)(&CZWorld::WriteSettingsSection),
                (zZbdSectionCallback)(&CZWorld::ReadSettingsSection),
                1000,
                0
            );
        }

        g_CZClass_IsInitialized = 1;
        return 0;
    }

    /*
     * Source-placement marker: the complete emitted definition belongs to
     * cls_zbd.c; this non-emitting marker preserves the legacy cls_util.c
     * provenance check while ShutdownCore continues to use the declaration.
     */

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.shutdowncore
     * @recoil-artifact defines .text recoil:function:0x451a00: CZClass::ShutdownCore.
     * @recoil-match byte
     *
     * Source owner: engine.zclass.lifecycle_node_array_control.
     * Purpose: tear down zClass-owned nodes, type lists, node storage, and path state.
     */
    int __cdecl ShutdownCore() {
        CZList::DeleteAllOfType(6);
        CZTypeList::FreeAll();

        if (g_CZClass_NodeArraySize > 0) {
            if (g_CZClass_NodeArray != 0) {
                free(g_CZClass_NodeArray);
                g_CZClass_NodeArray = 0;
            }

            g_CZClass_NodeArraySize = 0;
            g_CZClass_ActiveNodeCount = 0;
            g_CZClass_NodeFreeHeadIndex = -1;
        }

        ResetCurrentZbdPath();
        g_CZClass_IsInitialized = 0;
        return 0;
    }

}

namespace CZUtil {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.destroynoderecursive
     * @recoil-artifact defines .text recoil:function:0x451a60: CZUtil::DestroyNodeRecursive.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_util.c.
     * Purpose: recursively remove children, release display/class data, and
     * return nodes to the zClass free list.
     */
    int __fastcall DestroyNodeRecursive(CZNodePartial * node) {
        if (node == 0) {
            zError::ReportOld(0x400, g_CZClass_SourceFile_ClsUtilC, 0x2b6, "Null node pointer.");
            return 1;
        }

        if (node->listCountA > 0) {
            return 1;
        }

        if (node->listCountB > 0) {
            for (;;) {
                CZNodePartial *child = node->listB[0];
                const int removeResult = CZClass::RemoveChild(node, child);
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
            const int setResult = CZClass::gwNodeSetDisplayInstance(node, 0);
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

        return CZClass::DeleteNodeByType(node);
    }
}

namespace CZUtil {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copynodedisplayinstance
     * @recoil-artifact defines .text recoil:function:0x451b20: CZUtil::CopyNodeDisplayInstance
     * Purpose: copy or clone a source node's display instance into the destination.
     */
    int __fastcall CopyNodeDisplayInstance(
        CZNodePartial * source,
        CZNodePartial * dest
    ) {
        int result = 0;
        unsigned int displayInstanceValue;

        if (source->userDataOrDiRef != 0) {
            if (g_CZClass_CopyNodeCloneDiMode != 0) {
                unsigned int sourceInstanceValue;
                result = CZClass::gwNodeGetUserData(source, &sourceInstanceValue);
                if (result != 0) {
                    return result;
                }

                displayInstanceValue = sourceInstanceValue;
                int cloneInstance = 1;
                if (g_CZClass_CopyNodeDiArg1 != 0 &&
                    zDi::HasSpecialFlagsOrAuxMaterialData((zDiPartial *)sourceInstanceValue) == 0) {
                    cloneInstance = 0;
                }
                if (cloneInstance) {
                    const unsigned int clonedInstanceValue = (unsigned int)zDi::CloneToInstance(
                        (zDiPartial *)sourceInstanceValue,
                        g_CZClass_CopyNodeDiArg0,
                        g_CZClass_CopyNodeDiArg1);
                    if (clonedInstanceValue == 0) {
                        return 1;
                    }
                    displayInstanceValue = clonedInstanceValue;
                    return CZClass::gwNodeSetDisplayInstance(dest, (zDiPartial *)displayInstanceValue);
                }
            } else {
                result = CZClass::gwNodeGetUserData(source, &displayInstanceValue);
                if (result != 0) {
                    return result;
                }
            }
            result = CZClass::gwNodeSetDisplayInstance(dest, (zDiPartial *)displayInstanceValue);
        }

        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copynodebasedata
     * @recoil-artifact defines .text recoil:function:0x451bd0: CZUtil::CopyNodeBaseData
     * Purpose: copy common node flags, callbacks, type, and graphics state.
     */
    int __fastcall CopyNodeBaseData(
        CZNodePartial * source,
        CZNodePartial * dest
    ) {
        int result = CZClass::gwNodeSetName(dest, source->name);
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_CZClass_SourceFile_ClsUtilC,
                0x34e,
                g_CZClass_CopyNodeDescriptionFieldErrorFmt,
                source,
                source
            );
            return result;
        }

        result = CZClass::gwNodeSetActive(dest, ((unsigned int)source->flags >> 2) & 1);
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_CZClass_SourceFile_ClsUtilC,
                0x359,
                g_CZClass_CopyNodeActiveFieldErrorFmt,
                source,
                source
            );
            return result;
        }

        result = CZClass::gwNodeSetCellPickable(dest, ((unsigned int)source->flags >> 3) & 1);
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_CZClass_SourceFile_ClsUtilC,
                0x364,
                g_CZClass_CopyNodeAltitudeSurfaceFieldErrorFmt,
                source,
                source
            );
            return result;
        }

        result = CZClass::gwNodeSetRaycastable(dest, ((unsigned int)source->flags >> 4) & 1);
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_CZClass_SourceFile_ClsUtilC,
                0x36f,
                g_CZClass_CopyNodeIntersectionFieldErrorFmt,
                source,
                source
            );
            return result;
        }

        result = CZClass::gwNodeSetPickable(dest, ((unsigned int)source->flags >> 5) & 1);
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_CZClass_SourceFile_ClsUtilC,
                0x37a,
                g_CZClass_CopyNodeIntersectBboxFieldErrorFmt,
                source,
                source
            );
            return result;
        }

        result = CZClass::gwNodeSetHasHitCallback(dest, ((unsigned int)source->flags >> 6) & 1);
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_CZClass_SourceFile_ClsUtilC,
                0x385,
                g_CZClass_CopyNodeProximityFlagErrorFmt,
                source,
                source
            );
            return result;
        }

        result = CZClass::gwNodeSetBypassFarClip(dest, ((unsigned int)source->flags >> 7) & 1);
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_CZClass_SourceFile_ClsUtilC,
                0x390,
                g_CZClass_CopyNodeLandmarkFlagErrorFmt,
                source,
                source
            );
            return result;
        }

        result = CZClass::gwNodeSetFlag16(dest, ((unsigned int)source->flags >> 16) & 1);
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_CZClass_SourceFile_ClsUtilC,
                0x39b,
                g_CZClass_CopyNodeCanModifyFlagErrorFmt,
                source,
                source
            );
            return result;
        }

        result = CZClass::gwNodeSetFlag17(dest, ((unsigned int)source->flags >> 17) & 1);
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_CZClass_SourceFile_ClsUtilC,
                0x3a6,
                g_CZClass_CopyNodeClipToFlagErrorFmt,
                source,
                source
            );
            return result;
        }

        result = CZClass::gwNodeClearVariantGate(dest, ((unsigned int)source->flags >> 24) & 1);
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_CZClass_SourceFile_ClsUtilC,
                0x3b1,
                g_CZClass_CopyNodeDiZoneCheckFlagErrorFmt,
                source,
                source
            );
            return result;
        }

        result = CZClass::gwNodeSetVertexAlphaOverride(dest, ((unsigned int)source->flags >> 23) & 1);
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_CZClass_SourceFile_ClsUtilC,
                0x3bc,
                g_CZClass_CopyNodeOverwriteFlagErrorFmt,
                source,
                source
            );
            return result;
        }

        dest->flags |= source->flags & 0x70000000;
        dest->auxFlags = source->auxFlags;

        result = CopyNodeDisplayInstance(source, dest);
        if (result != 0) {
            zError::ReportOld(
                0x400,
                g_CZClass_SourceFile_ClsUtilC,
                0x3ce,
                g_CZClass_CopyNodeGraphicsDataErrorFmt,
                source,
                source
            );
            return result;
        }

        if (source->callbackContext != 0) {
            zError::ReportOld(
                0x100,
                g_CZClass_SourceFile_ClsUtilC,
                0x3d8,
                g_CZClass_CopyNodeEnvironmentDataIgnoredFmt,
                source
            );
        }
        dest->callbackContext = 0;

        result = CZClass::gwNodeSetPriority(dest, source->callbackPriority);
        if (result != 0) {
            zError::ReportOld(
                0x100,
                g_CZClass_SourceFile_ClsUtilC,
                0x3e2,
                g_CZClass_CopyNodeActionCallbackPriorityFieldErrorFmt,
                source,
                source
            );
            return result;
        }

        result = CZClass::gwNodeSetActionCallback(dest, source->actionCallback);
        if (result != 0) {
            zError::ReportOld(
                0x100,
                g_CZClass_SourceFile_ClsUtilC,
                0x3ed,
                g_CZClass_CopyNodeActionCallbackFieldErrorFmt,
                source,
                source
            );
            return result;
        }

        result = CZClass::gwNodeSetNodeType(dest, source->nodeType);
        if (result != 0) {
            zError::ReportOld(
                0x100,
                g_CZClass_SourceFile_ClsUtilC,
                0x3f8,
                g_CZClass_CopyNodeZoneIdErrorFmt,
                source,
                source
            );
            return result;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copycameranode
     * @recoil-artifact defines .text recoil:function:0x451f70: CZUtil::CopyCameraNode
     * Purpose: allocate and populate a copied camera node and its copied children.
     */
    CZNodePartial *__fastcall CopyCameraNode(
        CZNodePartial * source
    ) {
        int result; // Status captures are unused afterward but are proven by byte matching.
        CZNodePartial *const camera = CZCamera::gwCameraNew();
        if (camera == 0) {
            return camera;
        }

        if ((result = CopyNodeBaseData(source, camera)) != 0) {
            zError::ReportOld(
                0x100,
                g_CZClass_SourceFile_ClsUtilC,
                0x422,
                g_CZClass_CopyNodeBaseDataErrorMsg,
                g_CZClass_CopyNodeSourceNodeFmt,
                source,
                source
            );
            return 0;
        }

        CZCameraDataPartial *const data = (CZCameraDataPartial *)(source->classData);
        if ((result = CZCamera::gwCameraSetWorld(camera, data->worldNode)) != 0) {
            return 0;
        }
        if ((result = CZCamera::gwCameraSetWindow(camera, data->windowNode)) != 0) {
            return 0;
        }
        if ((result = CZCamera::gwCameraSetTarget(
                camera,
                data->targetOrEuler.x,
                data->targetOrEuler.y,
                data->targetOrEuler.z
            )) != 0) {
            return 0;
        }
        if ((result = CZCamera::gwCameraSetPosition(
                camera,
                data->posOffset.x,
                data->posOffset.y,
                data->posOffset.z
            )) != 0) {
            return 0;
        }
        if ((result = CZCamera::gwCameraSetNearFarClip(camera, data->nearClip, data->farClip)) != 0) {
            return 0;
        }
        if ((result = CZCamera::gwCameraSetClipDistance(camera, data->clipDistance)) != 0) {
            return 0;
        }
        if ((result = CZCamera::gwCameraSetFOV(camera, data->fovX, data->fovY)) != 0) {
            return 0;
        }

        for (int i = 0; i < source->listCountB; ++i) {
            CZNodePartial *const child = CopyNodeDispatch(source->listB[i]);
            if (child == 0 || CZCamera::gwCameraAddChild(camera, child) != 0) {
                return 0;
            }
        }
        return camera;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copylightnode-unimplemented
     * @recoil-artifact defines .text recoil:function:0x4520c0: CZUtil::CopyLightNode
     * Purpose: preserve the retail unimplemented light-node copy path.
     */
    CZNodePartial *__fastcall CopyLightNode(
        CZNodePartial *
    ) {
        zError::ReportOld(
            0x800,
            g_CZClass_SourceFile_ClsUtilC,
            0x47d,
            g_CZClass_CopyLightNodeUnimplementedMsg
        );
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copysoundnode-unimplemented
     * @recoil-artifact defines .text recoil:function:0x4520e0: CZUtil::CopySoundNode
     * Purpose: preserve the retail unimplemented sound-node copy path.
     */
    CZNodePartial *__fastcall CopySoundNode(
        CZNodePartial *
    ) {
        zError::ReportOld(
            0x800,
            g_CZClass_SourceFile_ClsUtilC,
            0x493,
            g_CZClass_CopySoundNodeUnimplementedMsg
        );
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copyobject3dnode
     * @recoil-artifact defines .text recoil:function:0x452100: CZUtil::CopyObject3DNode
     * Purpose: allocate and populate a copied Object3D node and its copied children.
     */
    CZNodePartial *__fastcall CopyObject3DNode(
        CZNodePartial * source
    ) {
        int result; // Status captures are unused afterward but are proven by byte matching.
        CZNodePartial *const parent = CZObject3D::gwObject3DInit();
        if (parent == 0) {
            return parent;
        }

        if ((result = CopyNodeBaseData(source, parent)) != 0) {
            zError::ReportOld(
                0x100,
                g_CZClass_SourceFile_ClsUtilC,
                0x4b9,
                g_CZClass_CopyNodeBaseDataErrorMsg,
                g_CZClass_CopyNodeSourceNodeFmt,
                source,
                source
            );
            return 0;
        }

        CZObject3DDataPartial *const data = (CZObject3DDataPartial *)(source->classData);
        if ((result = CZObject3D::gwObject3DSetAlphaScale(parent, data->alphaScale)) != 0) {
            return 0;
        }
        if ((result = CZObject3D::gwObject3DSetLitFlag(parent, ((unsigned int)data->flags >> 1) & 1)) != 0) {
            return 0;
        }

        if ((data->flags & 0x08) == 0) {
            if ((data->flags & 0x10) != 0) {
                if ((result = CZObject3D::gwObject3DSetMatrix(parent, data->localMatrix)) != 0) {
                    return 0;
                }
            } else {
                if ((result = CZObject3D::gwObject3DSetPosition(
                        parent,
                        data->localMatrix[9],
                        data->localMatrix[10],
                        data->localMatrix[11]
                    )) != 0) {
                    return 0;
                }
                if ((result = CZObject3D::gwObject3DSetRotation(
                        parent,
                        data->rotation.x,
                        data->rotation.y,
                        data->rotation.z
                    )) != 0) {
                    return 0;
                }
                if ((result = CZObject3D::gwObject3DSetScale(
                        parent,
                        data->scale.x,
                        data->scale.y,
                        data->scale.z
                    )) != 0) {
                    return 0;
                }
            }
        }

        for (int i = 0; i < source->listCountB; ++i) {
            CZNodePartial *const child = CopyNodeDispatch(source->listB[i]);
            if (child != 0 && CZObject3D::gwObject3DAddChild(parent, child) != 0) {
                return 0;
            }
        }
        return parent;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copyanimatenode-unimplemented
     * @recoil-artifact defines .text recoil:function:0x452230: CZUtil::CopyAnimateNode
     * Purpose: preserve the retail unimplemented animate-node copy path.
     */
    CZNodePartial *__fastcall CopyAnimateNode(
        CZNodePartial *
    ) {
        zError::ReportOld(
            0x100,
            g_CZClass_SourceFile_ClsUtilC,
            0x518,
            g_CZClass_CopyAnimateNodeUnimplementedMsg
        );
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copylodnode
     * @recoil-artifact defines .text recoil:function:0x452250: CZUtil::CopyLodNode
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zclass.copy-lod-range recoil:function:0x452250
     * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zclass.copy-lod-range
     *
     * Purpose: allocate and populate a copied LOD node and its copied children.
     * Raw assembly: Pro-reviewed after native VC5 C++ range-estimate variants failed.
     */
    CZNodePartial *__fastcall CopyLodNode(CZNodePartial * source) {
        float savedRange; // Unused later; this argument capture is proven by byte matching.
        CZNodePartial *const parent = CZLod::gwLodNew();
        if (parent == 0) {
            return parent;
        }

        if (CopyNodeBaseData(source, parent) != 0) {
            zError::ReportOld(
                0x100,
                g_CZClass_SourceFile_ClsUtilC,
                0x53e,
                g_CZClass_CopyNodeBaseDataErrorMsg,
                g_CZClass_CopyNodeSourceNodeFmt,
                source,
                source
            );
            return 0;
        }

        CZLodDataPartial *const sourceData = (CZLodDataPartial *)(source->classData);
        if (CZLod::SetComputeOwnDistance(parent, sourceData->computeOwnDistance) != 0) {
            return 0;
        }

        CZLodDataPartial *const destData = (CZLodDataPartial *)(parent->classData);
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

        float range = 0.0f;
        {
            int inputValue;
            memcpy(&inputValue, &sourceData->rangeSq, sizeof(inputValue));
            float rangeValue;
            /**
             * Purpose: Compute the retail range estimate from the named input bits.
             * Address-specific Pro review permits only this four-op conversion;
             * native VC5 C++ variants did not reproduce its storage and schedule.
             */
            __asm {
                mov eax, inputValue
                sar eax, 1
                add eax, 01fc00000h
                mov rangeValue, eax
            }
            range = rangeValue;
        }
        if (CZLod::SetTargetNodeAndRange(parent, sourceData->rangeNode, (savedRange = range)) != 0) {
            return 0;
        }

        for (int i = 0; i < source->listCountB; ++i) {
            CZNodePartial *const child = CopyNodeDispatch(source->listB[i]);
            if (child == 0 || CZLod::gwLodAddChild(parent, child) != 0) {
                return 0;
            }
        }

        return parent;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copysequencenode-unimplemented
     * @recoil-artifact defines .text recoil:function:0x4523c0: CZUtil::CopySequenceNode
     * Purpose: preserve the retail unimplemented sequence-node copy path.
     */
    CZNodePartial *__fastcall CopySequenceNode(
        CZNodePartial *
    ) {
        zError::ReportOld(
            0x100,
            g_CZClass_SourceFile_ClsUtilC,
            0x585,
            g_CZClass_CopySequenceNodeUnimplementedMsg
        );
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copyswitchnode-stub
     * @recoil-artifact defines .text recoil:function:0x4523e0: CZUtil::CopySwitchNode
     * Purpose: preserve the retail switch-node copy stub behavior.
     */
    CZNodePartial *__fastcall CopySwitchNode(CZNodePartial *) {
        zError::ReportOld(
            0x100,
            g_CZClass_SourceFile_ClsUtilC,
            0x59c,
            g_CZClass_CopySwitchNodeUnimplementedMsg
        );
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copynodedispatch
     * @recoil-artifact defines .text recoil:function:0x452400: CZUtil::CopyNodeDispatch
     * Purpose: dispatch node-copy work by class id.
     */
    CZNodePartial *__fastcall CopyNodeDispatch(
        CZNodePartial * source
    ) {
        CZNodePartial *result = 0;
        if (source == 0) {
            zError::ReportOld(0x400, g_CZClass_SourceFile_ClsUtilC, 0x5b8, "Null node pointer.");
            return 0;
        }
        if ((source->flags & 0x04000000) != 0) {
            return source;
        }

        switch (source->classId) {
        case 5:
            result = CopyObject3DNode(source);
            break;
        case 6:
            result = CopyLodNode(source);
            break;
        case 1:
            result = CopyCameraNode(source);
            break;
        case 9:
            result = CopyLightNode(source);
            break;
        case 10:
            result = CopySoundNode(source);
            break;
        case 8:
            result = CopyAnimateNode(source);
            break;
        case 7:
            result = CopySequenceNode(source);
            break;
        case 11:
            result = CopySwitchNode(source);
            break;
        case 2:
            zError::ReportOld(
                0x100,
                g_CZClass_SourceFile_ClsUtilC,
                0x5e1,
                g_CZClass_CopyWorldClassNodesErrorMsg
            );
            return 0;
        default:
            zError::ReportOld(
                0x100,
                g_CZClass_SourceFile_ClsUtilC,
                0x5e8,
                g_CZClass_CopyNodeUnrecognizedNodeFmt,
                source
            );
            break;
        }
        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copynodewithcloneoptions
     * @recoil-artifact defines .text recoil:function:0x452500: CZUtil::CopyNodeWithCloneOptions
     * Purpose: copy a node while temporarily overriding clone-mode globals.
     */
    CZNodePartial *__fastcall CopyNodeWithCloneOptions(
        CZNodePartial * source,
        int cloneDiMode,
        int diArg0
    ) {
        if (source == 0) {
            zError::ReportOld(0x400, g_CZClass_SourceFile_ClsUtilC, 0x60f, "Null node pointer.");
            return 0;
        }

        const int savedCloneDiMode = g_CZClass_CopyNodeCloneDiMode;
        const int savedDiArg0 = g_CZClass_CopyNodeDiArg0;
        g_CZClass_CopyNodeCloneDiMode = cloneDiMode;
        g_CZClass_CopyNodeDiArg0 = diArg0;

        CZNodePartial *const result = CopyNodeDispatch(source);
        g_CZClass_CopyNodeCloneDiMode = savedCloneDiMode;
        g_CZClass_CopyNodeDiArg0 = savedDiArg0;
        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.copynode
     * @recoil-artifact defines .text recoil:function:0x452560: CZUtil::CopyNode
     * Purpose: copy a node while temporarily overriding all display-instance clone options.
     */
    CZNodePartial *__fastcall CopyNode(
        CZNodePartial * source,
        int cloneDiMode,
        int diArg0,
        int diArg1
    ) {
        if (source == 0) {
            zError::ReportOld(0x400, g_CZClass_SourceFile_ClsUtilC, 0x648, "Null node pointer.");
            return 0;
        }

        const int savedCloneDiMode = g_CZClass_CopyNodeCloneDiMode;
        const int savedDiArg0 = g_CZClass_CopyNodeDiArg0;
        const int savedDiArg1 = g_CZClass_CopyNodeDiArg1;
        g_CZClass_CopyNodeCloneDiMode = cloneDiMode;
        g_CZClass_CopyNodeDiArg0 = diArg0;
        g_CZClass_CopyNodeDiArg1 = diArg1;

        CZNodePartial *const result = CopyNodeDispatch(source);
        g_CZClass_CopyNodeDiArg1 = savedDiArg1;
        g_CZClass_CopyNodeCloneDiMode = savedCloneDiMode;
        g_CZClass_CopyNodeDiArg0 = savedDiArg0;
        return result;
    }
}

namespace CZBBox {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.minmaxtoboundingsphere
     * @recoil-artifact defines .text recoil:function:0x4525d0: CZBBox::MinMaxToBoundingSphere.
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zclass.minmax-radius recoil:function:0x4525d0
     * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zclass.minmax-radius
     *
     * Purpose: write the min/max bbox center and retail approximate
     * bounding-sphere radius.
     * Raw assembly: Pro-reviewed after native VC5 C++ radius-estimate variants failed.
     */
    float *__fastcall MinMaxToBoundingSphere(
        const zBBox3f *bbox,
        zVec3 *outCenter,
        float *outRadius
    ) {
        float savedHalf; // Unused later; these input captures are proven by byte matching.
        const float halfX = (bbox->max.x - bbox->min.x) * 0.5f;
        const float halfY = (bbox->max.y - bbox->min.y) * 0.5f;
        const float halfZ = (bbox->max.z - bbox->min.z) * 0.5f;
        outCenter->x = (savedHalf = halfX) + bbox->min.x;
        outCenter->y = (savedHalf = halfY) + bbox->min.y;
        outCenter->z = (savedHalf = halfZ) + bbox->min.z;

        {
            float rangeSquaredValue = halfX * halfX + halfY * halfY + halfZ * halfZ;
            float rangeValue;
            /**
             * Purpose: Compute the retail radius estimate from the squared half-extents.
             * Address-specific Pro review permits only this four-op conversion;
             * native VC5 C++ variants did not reproduce its storage and schedule.
             */
            __asm {
                mov eax, rangeSquaredValue
                sar eax, 1
                add eax, 01fc00000h
                mov rangeValue, eax
            }
            *outRadius = rangeValue;
        }
        return outRadius;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-util.cornerstoboundingsphere
     * @recoil-artifact defines .text recoil:function:0x452650: CZBBox::CornersToBoundingSphere.
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zclass.corners-radius recoil:function:0x452650
     * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zclass.corners-radius
     *
     * Purpose: scan eight bbox corners, write the center, and write the
     * retail approximate bounding-sphere radius.
     * Raw assembly: Pro-reviewed after native VC5 C++ radius-estimate variants failed.
     */
    void __fastcall CornersToBoundingSphere(
        zBBoxCorners * corners,
        zVec3 * outCenter,
        float *outRadius
    ) {
        const zVec3 *corner = corners->corners;
        zBBox3f bounds;
        bounds.min.x = corner->x;
        bounds.max.x = corner->x;
        bounds.min.y = corner->y;
        bounds.max.y = corner->y;
        bounds.min.z = corner->z;
        bounds.max.z = corner->z;
        ++corner;

        for (int i = 7; i > 0; --i, ++corner) {
            if (corner->x < bounds.min.x) {
                bounds.min.x = corner->x;
            } else if (corner->x > bounds.max.x) {
                bounds.max.x = corner->x;
            }
            if (corner->y < bounds.min.y) {
                bounds.min.y = corner->y;
            } else if (corner->y > bounds.max.y) {
                bounds.max.y = corner->y;
            }
            if (corner->z < bounds.min.z) {
                bounds.min.z = corner->z;
            } else if (corner->z > bounds.max.z) {
                bounds.max.z = corner->z;
            }
        }

        const float halfX = (bounds.max.x - bounds.min.x) * 0.5f;
        const float halfY = (bounds.max.y - bounds.min.y) * 0.5f;
        const float halfZ = (bounds.max.z - bounds.min.z) * 0.5f;
        outCenter->x = bounds.min.x + halfX;
        outCenter->y = bounds.min.y + halfY;
        outCenter->z = bounds.min.z + halfZ;
        {
            float rangeSquaredValue = halfX * halfX + halfY * halfY + halfZ * halfZ;
            float rangeValue;
            /**
             * Purpose: Compute the retail radius estimate after scanning the corner bounds.
             * Address-specific Pro review permits only this four-op conversion;
             * native VC5 C++ variants did not reproduce its storage and schedule.
             */
            __asm {
                mov eax, rangeSquaredValue
                sar eax, 1
                add eax, 01fc00000h
                mov rangeValue, eax
            }
            *outRadius = rangeValue;
        }
    }
}

namespace CZClass {
    /**
     * Purpose: recursively search a node subtree by name, checking the root
     * first and then visiting child-list entries from tail to head.
     */
    CZNodePartial *__fastcall FindSubNodeByName(
        CZNodePartial * root,
        const char *name
    ) {
        if (root == 0) {
            return 0;
        }
        if (strcmp(name, root->name) == 0) {
            return root;
        }

        for (int i = root->listCountB; i--; ) {
            CZNodePartial *found = FindSubNodeByName(root->listB[i], name);
            if (found != 0) {
                return found;
            }
        }

        return 0;
    }
}

namespace CZNode {
    /**
     * Purpose: test whether a node's DI reference points to a renderable display
     * instance mode without the hidden flag.
     */
    int __fastcall HasRenderableDiPredicate(CZNodePartial * node) {
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

namespace CZClass {
    /**
     * Purpose: recursively test a node and its secondary children with a
     * caller-supplied predicate.
     */
    int __fastcall AnyNodeMatchesPredicateRecursive(
        CZNodePartial * root,
        CZNodePredicate predicate
    ) {
        if (predicate(root) == 1) {
            return 1;
        }

        for (int i = root->listCountB; i-- > 0; ) {
            if (AnyNodeMatchesPredicateRecursive(root->listB[i], predicate) == 1) {
                return 1;
            }
        }

        return 0;
    }
}

namespace CZNode {
    /**
     * Purpose: recurse a child-list subtree and propagate material flag bit 9
     * updates through each node display instance.
     */
    void __fastcall SetMaterialFlagBit9ForFlagBit0EntriesRecursive(
        CZNodePartial * node,
        int enabled
    ) {
        zDiPartial *di = (zDiPartial *)((unsigned int)(node->userDataOrDiRef));
        if (di != 0) {
            zDi::SetMaterialFlagBit9ForFlagBit0Entries(di, enabled);
        }

        for (int i = 0; i < node->listCountB; ++i) {
            SetMaterialFlagBit9ForFlagBit0EntriesRecursive(node->listB[i], enabled);
        }
    }

    /**
     * Purpose: invalidate flagged material images under a node subtree and
     * then load pending texture-directory entries.
     */
    void __fastcall LoadFlagBit8MaterialImagesAndTexturePack(
        CZNodePartial * node
    ) {
        if (node == 0) {
            return;
        }

        InvalidateFlagBit8MaterialImagesRecursive(node);
        zImage::TexDirLoadPendingEntries();
    }

    /**
     * Purpose: recurse a child-list subtree and invalidate loaded material
     * image variants for each display instance with material flag bit 8 set.
     */
    void __fastcall InvalidateFlagBit8MaterialImagesRecursive(
        CZNodePartial * node
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
        CZNodePartial * node,
        int value
    ) {
        zDiPartial *di = (zDiPartial *)((unsigned int)(node->userDataOrDiRef));
        if (di != 0) {
            zDi::SetFlagBit0(di, value);
        }

        for (int i = 0; i < node->listCountB; ++i) {
            AssignInt32ToDiRecursive(node->listB[i], value);
        }
    }
}
