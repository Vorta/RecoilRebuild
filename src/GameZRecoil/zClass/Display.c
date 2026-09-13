#include "zclass.h"

#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <stdlib.h>

namespace {
    const int kZClassNodeDisplay = 4;

}

namespace CZDisplay {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.display.deletenode
     * @recoil-artifact defines .text recoil:logical-function:0x44db00:zclass-display-delete-node: CZDisplay::DeleteNode
     *
     * Purpose: route display deletion through the generic node free path.
     */
    int __fastcall DeleteNode(CZNodePartial * node) {
        return CZClass::TryFreeNode(node);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.display.zclass-display-gwdisplayinit
     * @recoil-artifact defines .text recoil:function:0x44fdd0: CZDisplay::gwDisplayInit
     *
     *
     * Purpose: allocate a display node, initialize its class data defaults, and
     * insert it into the display type list.
     */
    CZNodePartial *__cdecl gwDisplayInit() {
        CZNodePartial *node = CZClass::gwNodeNew();
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Display.c", 0x41, "Null node pointer.");
            return 0;
        }

        node->classId = kZClassNodeDisplay;
        CZDisplayDataPartial *data =
            (CZDisplayDataPartial *)(calloc(1, sizeof(CZDisplayDataPartial)));
        node->classData = data;
        data->width = 1;
        data->height = 1;
        data->backgroundR = 0.392f;
        data->backgroundG = 0.392f;
        data->backgroundB = 1.0f;

        if (CZTypeList::Insert(15, node) != 0) {
            CZClass::DeleteNodeByType(node);
            return 0;
        }

        return node;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.display.zclass-display-removechild
     * @recoil-artifact defines .text recoil:function:0x44fe50: CZDisplay::RemoveChild
     * @recoil-match byte
     *
     * Purpose: validate the parent and child pointers, then remove the child
     * through the generic zClass child-list helper.
     */
    int __fastcall RemoveChild(
        CZNodePartial * parent,
        CZNodePartial * child
    ) {
        if (parent == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Display.c", 0x8f, "Null node pointer.");
            return 5;
        }

        if (child == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Display.c", 0x90, "Null node pointer.");
            return 5;
        }

        CZClass::RemoveChildGeneric(parent, child);
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.display.zclass-display-gwdisplaysetsize
     * @recoil-artifact defines .text recoil:function:0x44fe90: CZDisplay::gwDisplaySetSize
     * @recoil-match byte
     *
     * Purpose: validate a display node and update its stored width and height.
     */
    int __fastcall gwDisplaySetSize(
        CZNodePartial * node,
        int width,
        int height
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Display.c", 0xb0, "Null node pointer.");
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Display.c", 0xb1, "Null class data pointer");
            return 5;
        }
        if (node->classId != kZClassNodeDisplay) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Display.c",
                0xb2,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                node->classId,
                kZClassNodeDisplay
            );
            return 3;
        }

        CZDisplayDataPartial *data =
            (CZDisplayDataPartial *)(node->classData);
        data->width = width;
        data->height = height;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.display.zclass-display-gwdisplaysetposition
     * @recoil-artifact defines .text recoil:function:0x44ff10: CZDisplay::gwDisplaySetPosition
     * @recoil-match byte
     *
     * Purpose: validate a display node and update its stored screen position.
     */
    int __fastcall gwDisplaySetPosition(
        CZNodePartial * node,
        int x,
        int y
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Display.c", 0xee, "Null node pointer.");
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Display.c", 0xef, "Null class data pointer");
            return 5;
        }
        if (node->classId != kZClassNodeDisplay) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Display.c",
                0xf0,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                node->classId,
                kZClassNodeDisplay
            );
            return 3;
        }

        CZDisplayDataPartial *data =
            (CZDisplayDataPartial *)(node->classData);
        data->x = x;
        data->y = y;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.display.zclass-display-gwdisplaysetbackgroundcolor
     * @recoil-artifact defines .text recoil:function:0x44ff90: CZDisplay::gwDisplaySetBackgroundColor
     *
     *
     * Purpose: update the display background color, pack it to the video clear
     * color format, and set the renderer clear color.
     */
    int __fastcall gwDisplaySetBackgroundColor(
        CZNodePartial * node,
        float red,
        float green,
        float blue
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Display.c", 0x133, "Null node pointer.");
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Display.c", 0x134, "Null class data pointer");
            return 5;
        }
        if (node->classId != kZClassNodeDisplay) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Display.c",
                0x135,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                node->classId,
                kZClassNodeDisplay
            );
            return 3;
        }

        CZDisplayDataPartial *data =
            (CZDisplayDataPartial *)(node->classData);
        data->backgroundR = red;
        data->backgroundG = green;
        data->backgroundB = blue;
        const unsigned short packedColor =
            zVidPackColorRgbFloats((zVideo_ColorRgbFloat *)(&data->backgroundR));
        zVideoSetClearColorPacked16(packedColor);
        return 0;
    }
}
