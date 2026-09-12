#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zVideo/zvid.h"

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.switch.g-zclass-sourcefile-switchc
 * @recoil-artifact defines .data recoil:data:0x4dec88: g_zClass_SourceFile_SwitchC.
 * BN data inventory declares writable Switch.c source path char[0x24], and
 * Switch.c parent/child validation callers reference it for zError reports.
 * Purpose: preserve the legacy source-file literal for switch-node diagnostics.
 */
char g_zClass_SourceFile_SwitchC[0x24] =
    "D:\\Proj\\GameZRecoil\\zClass\\Switch.c";
}

namespace zClass_Class {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.switch.addchildvalidated
     * @recoil-artifact defines .text recoil:function:0x452920: zClass_Class::AddChildValidated.
     * @recoil-match byte
     *
     * Purpose: validate both nodes, then link the child through the generic lists.
     */
    int __fastcall AddChildValidated(zClass_NodePartial *parent,
        zClass_NodePartial *child) {
        if (parent == 0) {
            zError::ReportOld(0x400, g_zClass_SourceFile_SwitchC, 0x80, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(0x400, g_zClass_SourceFile_SwitchC, 0x81, "Null node pointer.");
            return 5;
        }
        if (parent->classData == 0) {
            zError::ReportOld(0x400, g_zClass_SourceFile_SwitchC, 0x82, "Null class data pointer");
            return 5;
        }

        return AddChildGeneric(parent, child);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.switch.removechildvalidated
     * @recoil-artifact defines .text recoil:function:0x452970: zClass_Class::RemoveChildValidated.
     * @recoil-match byte
     *
     * Purpose: validate both nodes, then unlink the child through the generic lists.
     */
    int __fastcall RemoveChildValidated(zClass_NodePartial *parent,
        zClass_NodePartial *child) {
        if (parent == 0) {
            zError::ReportOld(0x400, g_zClass_SourceFile_SwitchC, 0x9f, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(0x400, g_zClass_SourceFile_SwitchC, 0xa0, "Null node pointer.");
            return 5;
        }
        if (parent->classData == 0) {
            zError::ReportOld(
                0x400,
                g_zClass_SourceFile_SwitchC,
                0xa1,
                "Null class data pointer"
            );
            return 5;
        }

        return RemoveChildGeneric(parent, child);
    }
}

namespace zClass_Switch {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.switch.deletenode
     * @recoil-artifact defines .text recoil:logical-function:0x44db00:zclass-switch-delete-node: zClass_Switch::DeleteNode
     * Purpose: route switch deletion through the generic node free path.
     */
    int __fastcall DeleteNode(zClass_NodePartial * node) {
        return zClass_Class::TryFreeNode(node);
    }
}
