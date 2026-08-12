#include "zclass.h"

#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <stdlib.h>

namespace {
    const int kZClassNodeDisplay = 4;

    /*
     * BN diagnostic string data used by Display.c validation paths at
     * 0x44fdd0, 0x44fe50, 0x44fe90, 0x44ff10, and 0x44ff90.
     */
    const char kDisplaySourceFile[] = "D:\\Proj\\GameZRecoil\\zClass\\Display.c";

}

namespace zClass_Display {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.display.deletenode
     * @recoil-artifact defines .text recoil:logical-function:0x44db00:zclass-display-delete-node: zClass_Display::DeleteNode
     * Purpose: route display deletion through the generic node free path.
     */
    int __fastcall DeleteNode(zClass_NodePartial * node) {
        return zClass_Class::TryFreeNode(node);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.display.zclass-display-gwdisplayinit
     * @recoil-artifact defines .text recoil:function:0x44fdd0: zClass_Display::gwDisplayInit
     *
     * Purpose: allocate a display node, initialize its class data defaults, and
     * insert it into the display type list.
     */
    zClass_NodePartial *__cdecl gwDisplayInit() {
        zClass_NodePartial *node = zClass_Class::AllocNodeFromFreeList();
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kDisplaySourceFile,
                0x41,
                "Null node pointer."
            );
            return 0;
        }

        node->classId = kZClassNodeDisplay;
        zClass_DisplayDataPartial *data =
            (zClass_DisplayDataPartial *)(calloc(
                1,
                sizeof(zClass_DisplayDataPartial)
            ));
        node->classData = data;
        data->width = 1;
        data->height = 1;
        data->backgroundR = 0.392f;
        data->backgroundG = 0.392f;
        data->backgroundB = 1.0f;

        if (zClass_TypeList::Insert(
            15,
            node
        ) != 0) {
            zClass_Class::DeleteNodeByType(node);
            return 0;
        }

        return node;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.display.zclass-display-removechild
     * @recoil-artifact defines .text recoil:function:0x44fe50: zClass_Display::RemoveChild
     *
     * Purpose: validate the parent and child pointers, then remove the child
     * through the generic zClass child-list helper.
     */
    int __fastcall RemoveChild(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ) {
        if (parent == 0) {
            zError::ReportOld(
                0x400,
                kDisplaySourceFile,
                0x8f,
                "Null node pointer."
            );
            return 5;
        }

        if (child == 0) {
            zError::ReportOld(
                0x400,
                kDisplaySourceFile,
                0x90,
                "Null node pointer."
            );
            return 5;
        }

        zClass_Class::RemoveChildGeneric(
            parent,
            child
        );
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.display.zclass-display-gwdisplaysetsize
     * @recoil-artifact defines .text recoil:function:0x44fe90: zClass_Display::gwDisplaySetSize
     *
     * Purpose: validate a display node and update its stored width and height.
     */
    int __fastcall gwDisplaySetSize(
        zClass_NodePartial * node,
        int width,
        int height
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kDisplaySourceFile,
                0xb0,
                "node != NULL"
            );
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(
                0x400,
                kDisplaySourceFile,
                0xb1,
                "node->classData != NULL"
            );
            return 5;
        }
        if (node->classId != kZClassNodeDisplay) {
            zError::ReportOld(
                0x400,
                kDisplaySourceFile,
                0xb2,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                node->classId,
                kZClassNodeDisplay
            );
            return 3;
        }

        zClass_DisplayDataPartial *data =
            (zClass_DisplayDataPartial *)(node->classData);
        data->width = width;
        data->height = height;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.display.zclass-display-gwdisplaysetposition
     * @recoil-artifact defines .text recoil:function:0x44ff10: zClass_Display::gwDisplaySetPosition
     *
     * Purpose: validate a display node and update its stored screen position.
     */
    int __fastcall gwDisplaySetPosition(
        zClass_NodePartial * node,
        int x,
        int y
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kDisplaySourceFile,
                0xee,
                "node != NULL"
            );
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(
                0x400,
                kDisplaySourceFile,
                0xef,
                "node->classData != NULL"
            );
            return 5;
        }
        if (node->classId != kZClassNodeDisplay) {
            zError::ReportOld(
                0x400,
                kDisplaySourceFile,
                0xf0,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                node->classId,
                kZClassNodeDisplay
            );
            return 3;
        }

        zClass_DisplayDataPartial *data =
            (zClass_DisplayDataPartial *)(node->classData);
        data->x = x;
        data->y = y;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.display.zclass-display-gwdisplaysetbackgroundcolor
     * @recoil-artifact defines .text recoil:function:0x44ff90: zClass_Display::gwDisplaySetBackgroundColor
     *
     * Purpose: update the display background color, pack it to the video clear
     * color format, and set the renderer clear color.
     */
    int __fastcall gwDisplaySetBackgroundColor(
        zClass_NodePartial * node,
        float red,
        float green,
        float blue
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kDisplaySourceFile,
                0x133,
                "Null node pointer."
            );
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(
                0x400,
                kDisplaySourceFile,
                0x134,
                "Null class data pointer"
            );
            return 5;
        }
        if (node->classId != kZClassNodeDisplay) {
            zError::ReportOld(
                0x400,
                kDisplaySourceFile,
                0x135,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                node->classId,
                kZClassNodeDisplay
            );
            return 3;
        }

        zClass_DisplayDataPartial *data =
            (zClass_DisplayDataPartial *)(node->classData);
        data->backgroundR = red;
        data->backgroundG = green;
        data->backgroundB = blue;
        const unsigned short packedColor =
            zVid_PackColorRgbFloats((zVideo_ColorRgbFloat *)(&data->backgroundR));
        zVideo_SetClearColorPacked16(packedColor);
        return 0;
    }
}
