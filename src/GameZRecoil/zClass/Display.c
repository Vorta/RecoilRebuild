#include "zClass.h"

#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <stdlib.h>

namespace {
    const int kZClassNodeDisplay = 4;

    int ReportDisplayClassError(int sourceLine, const char *message) {
        zError::ReportOld(0x400, "GameZRecoil/zClass/Display.c", sourceLine, message);
        return 5;
    }

    zClass_DisplayDataPartial *GetDisplayData(zClass_NodePartial * node, int nullLine, int dataLine,
                                              int classLine) {
        if (node == 0) {
            ReportDisplayClassError(nullLine, "node != NULL");
            return 0;
        }

        if (node->classData == 0) {
            ReportDisplayClassError(dataLine, "node->classData != NULL");
            return 0;
        }

        if (node->classId != kZClassNodeDisplay) {
            zError::ReportOld(0x400, "GameZRecoil/zClass/Display.c", classLine,
                              "Unexpected class id");
            return 0;
        }

        return static_cast<zClass_DisplayDataPartial *>(node->classData);
    }

    zClass_DisplayDataPartial *GetDisplayDataOldMessages(zClass_NodePartial * node, int nullLine,
                                                         int dataLine, int classLine, int *result) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Display.c", nullLine,
                              "Null node pointer.");
            *result = 5;
            return 0;
        }

        if (node->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Display.c", dataLine,
                              "Null class data pointer");
            *result = 5;
            return 0;
        }

        if (node->classId != kZClassNodeDisplay) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Display.c", classLine,
                              "Bad Class Found.\n Wanted (%d)\n Found (%d)", node->classId,
                              kZClassNodeDisplay);
            *result = 3;
            return 0;
        }

        *result = 0;
        return static_cast<zClass_DisplayDataPartial *>(node->classData);
    }
}

namespace zClass_Display {
    // Reimplements 0x44fdd0: zClass_Display::gwDisplayInit
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL gwDisplayInit() {
        zClass_NodePartial *node = zClass_Class::AllocNodeFromFreeList();
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Display.c", 0x41,
                              "Null node pointer.");
            return 0;
        }

        node->classId = kZClassNodeDisplay;
        zClass_DisplayDataPartial *data = static_cast<zClass_DisplayDataPartial *>(
            calloc(1, sizeof(zClass_DisplayDataPartial)));
        node->classData = data;
        data->width = 1;
        data->height = 1;
        data->backgroundR = 0.392f;
        data->backgroundG = 0.392f;
        data->backgroundB = 1.0f;

        if (zClass_TypeList::Insert(15, node) != 0) {
            zClass_Object3D::DeleteNode(node);
            return 0;
        }

        return node;
    }

    // Reimplements 0x44fe50: zClass_Display::RemoveChild
    RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChild(zClass_NodePartial * parent,
                                                             zClass_NodePartial * child) {
        if (parent == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Display.c", 0x8f,
                              "Null node pointer.");
            return 5;
        }

        if (child == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Display.c", 0x90,
                              "Null node pointer.");
            return 5;
        }

        zClass_Class::RemoveChildGeneric(parent, child);
        return 0;
    }

    // Reimplements 0x44fe90: zClass_Display::gwDisplaySetSize
    RECOIL_NOINLINE int RECOIL_FASTCALL gwDisplaySetSize(
        zClass_NodePartial * node, int width, int height) {
        zClass_DisplayDataPartial *data = GetDisplayData(node, 0xb0, 0xb1, 0xb2);
        if (data == 0) {
            return node != 0 && node->classData != 0 ? 3 : 5;
        }

        data->width = width;
        data->height = height;
        return 0;
    }

    // Reimplements 0x44ff10: zClass_Display::gwDisplaySetPosition
    RECOIL_NOINLINE int RECOIL_FASTCALL gwDisplaySetPosition(
        zClass_NodePartial * node, int x, int y) {
        zClass_DisplayDataPartial *data = GetDisplayData(node, 0xee, 0xef, 0xf0);
        if (data == 0) {
            return node != 0 && node->classData != 0 ? 3 : 5;
        }

        data->x = x;
        data->y = y;
        return 0;
    }

    // Reimplements 0x44ff90: zClass_Display::gwDisplaySetBackgroundColor
    RECOIL_NOINLINE int RECOIL_FASTCALL gwDisplaySetBackgroundColor(
        zClass_NodePartial * node, float red, float green, float blue) {
        int result = 0;
        zClass_DisplayDataPartial *data =
            GetDisplayDataOldMessages(node, 0x133, 0x134, 0x135, &result);
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
