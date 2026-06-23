#include "zClass.h"

#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <stdlib.h>

namespace {
    const int kZClassNodeDisplay = 4;

    /*
     * BN diagnostic string data used by Display.c validation paths at
     * 0x44fdd0, 0x44fe50, 0x44fe90, 0x44ff10, and 0x44ff90.
     */
    const char *kDisplaySourceFile = "D:\\Proj\\GameZRecoil\\zClass\\Display.c";

    /**
     * Original static helper observed in callers 0x44fe90 and 0x44ff10
     * through display-node validation failures.
     *
     * Purpose: report a Display.c class validation failure and return the
     * generic zClass error code.
     */
    int ReportDisplayClassError(
        int sourceLine,
        const char *message
    ) {
        zError::ReportOld(
            0x400,
            kDisplaySourceFile,
            sourceLine,
            message
        );
        return 5;
    }

    /**
     * Original static helper observed in callers 0x44fe90 and 0x44ff10 as
     * shared display-node validation.
     *
     * Purpose: validate a display node and return its class data for size and
     * position updates.
     */
    zClass_DisplayDataPartial *GetDisplayData(
        zClass_NodePartial * node,
        int nullLine,
        int dataLine,
        int classLine
    ) {
        if (node == 0) {
            ReportDisplayClassError(
                nullLine,
                "node != NULL"
            );
            return 0;
        }

        if (node->classData == 0) {
            ReportDisplayClassError(
                dataLine,
                "node->classData != NULL"
            );
            return 0;
        }

        if (node->classId != kZClassNodeDisplay) {
            zError::ReportOld(
                0x400,
                kDisplaySourceFile,
                classLine,
                "Unexpected class id"
            );
            return 0;
        }

        return (zClass_DisplayDataPartial *)(node->classData);
    }

    /**
     * Original static helper observed in caller
     * 0x44ff90 with the original Display.c diagnostic strings.
     *
     * Purpose: validate a display node and preserve the old Display.c error
     * messages for background-color updates.
     */
    zClass_DisplayDataPartial *GetDisplayDataOldMessages(
        zClass_NodePartial * node,
        int nullLine,
        int dataLine,
        int classLine,
        int *result
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kDisplaySourceFile,
                nullLine,
                "Null node pointer."
            );
            *result = 5;
            return 0;
        }

        if (node->classData == 0) {
            zError::ReportOld(
                0x400,
                kDisplaySourceFile,
                dataLine,
                "Null class data pointer"
            );
            *result = 5;
            return 0;
        }

        if (node->classId != kZClassNodeDisplay) {
            zError::ReportOld(
                0x400,
                kDisplaySourceFile,
                classLine,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                node->classId,
                kZClassNodeDisplay
            );
            *result = 3;
            return 0;
        }

        *result = 0;
        return (zClass_DisplayDataPartial *)(node->classData);
    }
}

namespace zClass_Display {
    /**
     * Reimplements 0x44fdd0: zClass_Display::gwDisplayInit
     * (D:\Proj\GameZRecoil\zClass\Display.c).
     *
     * Purpose: allocate a display node, initialize its class data defaults, and
     * insert it into the display type list.
     */
    zClass_NodePartial *gwDisplayInit() {
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
            zClass_Object3D::DeleteNode(node);
            return 0;
        }

        return node;
    }

    /**
     * Reimplements 0x44fe50: zClass_Display::RemoveChild
     * (D:\Proj\GameZRecoil\zClass\Display.c).
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
     * Reimplements 0x44fe90: zClass_Display::gwDisplaySetSize
     * (D:\Proj\GameZRecoil\zClass\Display.c).
     *
     * Purpose: validate a display node and update its stored width and height.
     */
    int __fastcall gwDisplaySetSize(
        zClass_NodePartial * node,
        int width,
        int height
    ) {
        zClass_DisplayDataPartial *data = GetDisplayData(
            node,
            0xb0,
            0xb1,
            0xb2
        );
        if (data == 0) {
            return node != 0 && node->classData != 0 ? 3 : 5;
        }

        data->width = width;
        data->height = height;
        return 0;
    }

    /**
     * Reimplements 0x44ff10: zClass_Display::gwDisplaySetPosition
     * (D:\Proj\GameZRecoil\zClass\Display.c).
     *
     * Purpose: validate a display node and update its stored screen position.
     */
    int __fastcall gwDisplaySetPosition(
        zClass_NodePartial * node,
        int x,
        int y
    ) {
        zClass_DisplayDataPartial *data = GetDisplayData(
            node,
            0xee,
            0xef,
            0xf0
        );
        if (data == 0) {
            return node != 0 && node->classData != 0 ? 3 : 5;
        }

        data->x = x;
        data->y = y;
        return 0;
    }

    /**
     * Reimplements 0x44ff90: zClass_Display::gwDisplaySetBackgroundColor
     * (D:\Proj\GameZRecoil\zClass\Display.c).
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
        int result = 0;
        zClass_DisplayDataPartial *data =
            GetDisplayDataOldMessages(
                node,
                0x133,
                0x134,
                0x135,
                &result
            );
        if (data == 0) {
            return result;
        }

        data->backgroundR = red;
        data->backgroundG = green;
        data->backgroundB = blue;
        const unsigned short packedColor =
            zVid_PackColorRgbFloats((zVideo_ColorRgbFloat *)(&data->backgroundR));
        zVideo_SetClearColorPacked16(packedColor);
        return 0;
    }
}
