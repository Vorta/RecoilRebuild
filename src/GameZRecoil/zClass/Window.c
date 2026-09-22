#include "zclass.h"

#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zRender/zrndr.h"

#include <stdio.h>
#include <stdlib.h>

namespace
{
    const int kZClassNodeWindow = 3;
}

namespace CZWindow
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.window.deletenode
     * @recoil-artifact defines .text recoil:logical-function:0x44db00:zclass-window-delete-node: CZWindow::DeleteNode
     * Purpose: route window deletion through the generic node free path.
     */
    int __fastcall DeleteNode(CZNodePartial * node)
    {
        return CZClass::TryFreeNode(node);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.window.zclass-window-gwwindownew
     * @recoil-artifact defines .text recoil:function:0x44f7a0: CZWindow::gwWindowNew.
     * Purpose: allocate a window node, initialize its window data record from
     * the active render region, and insert it into the window type bucket.
     */
    CZNodePartial* __cdecl gwWindowNew()
    {
        CZNodePartial* node = CZClass::gwNodeNew();
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0x61, "Null node pointer.");
            return 0;
        }

        node->classId = kZClassNodeWindow;
        CZWindowDataPartial* data = (CZWindowDataPartial*)(calloc(1, sizeof(CZWindowDataPartial)));
        node->classData = data;
        data->resolutionWidth = 1;
        data->resolutionHeight = 1;
        data->bufferIndex = -1;

        int pitchBytes = 0;
        void* buffer = zRndr::GetActiveRegionState(&data->fbWidth, &data->fbHeight, &data->fbBpp, &pitchBytes);
        data->buffer = buffer;
        printf(
            "Window (new %x) buffer: %x (%d x %d x %d)\n",
            (unsigned int)((unsigned int)(data)),
            (unsigned int)((unsigned int)(buffer)),
            data->fbWidth,
            data->fbHeight,
            data->fbBpp
        );

        if (CZTypeList::Insert(14, node) != 0) {
            CZClass::DeleteNodeByType(node);
            return 0;
        }

        return node;
    }
}

namespace CZClass
{
    /**
     * Purpose: validate parent and child pointers before removing the child
     * through the generic class helper.
     */
    int __fastcall RemoveChildChecked(CZNodePartial * parent, CZNodePartial * child)
    {
        if (parent == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0xb6, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0xb7, "Null node pointer.");
            return 5;
        }

        return CZClass::RemoveChildGeneric(parent, child);
    }
}

namespace CZWindow
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.window.zclass-window-gwwindowsetresolution
     * @recoil-artifact defines .text recoil:function:0x44f8b0: CZWindow::gwWindowSetResolution.
     * Purpose: validate a window node and store the requested render
     * resolution in its window data record.
     */
    int __fastcall gwWindowSetResolution(CZNodePartial * node, int width, int height)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "GameZRecoil/zClass/Window.c", 0xcd, "node != NULL");
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(0x400, "GameZRecoil/zClass/Window.c", 0xce, "node->classData != NULL");
            return 5;
        }
        if (node->classId != kZClassNodeWindow) {
            zError::ReportOld(
                0x400,
                "GameZRecoil/zClass/Window.c",
                0xcf,
                "Unexpected class id",
                node->classId,
                kZClassNodeWindow
            );
            return 3;
        }

        CZWindowDataPartial* data = (CZWindowDataPartial*)(node->classData);
        data->resolutionWidth = width;
        data->resolutionHeight = height;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.window.zclass-window-gwwindowgetresolution
     * @recoil-artifact defines .text recoil:function:0x44f930: CZWindow::gwWindowGetResolution.
     * Purpose: validate a window node and return the stored render resolution.
     */
    int __fastcall gwWindowGetResolution(CZNodePartial * node, int* outWidth, int* outHeight)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0xe7, "Null node pointer.");
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0xe8, "Null class data pointer");
            return 5;
        }
        if (node->classId != kZClassNodeWindow) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Window.c",
                0xe9,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                node->classId,
                kZClassNodeWindow
            );
            return 3;
        }

        CZWindowDataPartial* data = (CZWindowDataPartial*)(node->classData);
        *outWidth = data->resolutionWidth;
        *outHeight = data->resolutionHeight;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.window.zclass-window-gwwindowsetsize
     * @recoil-artifact defines .text recoil:function:0x44f9c0: CZWindow::gwWindowSetSize.
     * Purpose: validate a window node and store the requested viewport size in
     * its window data record.
     */
    int __fastcall gwWindowSetSize(CZNodePartial * node, int width, int height)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "GameZRecoil/zClass/Window.c", 0x102, "node != NULL");
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(0x400, "GameZRecoil/zClass/Window.c", 0x103, "node->classData != NULL");
            return 5;
        }
        if (node->classId != kZClassNodeWindow) {
            zError::ReportOld(
                0x400,
                "GameZRecoil/zClass/Window.c",
                0x104,
                "Unexpected class id",
                node->classId,
                kZClassNodeWindow
            );
            return 3;
        }

        CZWindowDataPartial* data = (CZWindowDataPartial*)(node->classData);
        data->viewportWidth = width;
        data->viewportHeight = height;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.window.zclass-window-gwwindowgetsize
     * @recoil-artifact defines .text recoil:function:0x44fa40: CZWindow::gwWindowGetSize.
     * Purpose: validate a window node and return the stored viewport size.
     */
    int __fastcall gwWindowGetSize(CZNodePartial * node, int* outWidth, int* outHeight)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0x11d, "Null node pointer.");
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0x11e, "Null class data pointer");
            return 5;
        }
        if (node->classId != kZClassNodeWindow) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Window.c",
                0x11f,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                node->classId,
                kZClassNodeWindow
            );
            return 3;
        }

        CZWindowDataPartial* data = (CZWindowDataPartial*)(node->classData);
        *outWidth = data->viewportWidth;
        *outHeight = data->viewportHeight;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.window.zclass-window-gwwindowsetbuffer
     * @recoil-artifact defines .text recoil:function:0x44fad0: CZWindow::gwWindowSetBuffer.
     * Purpose: validate a window node and store the selected render-buffer
     * index.
     */
    int __fastcall gwWindowSetBuffer(CZNodePartial * node, int bufferIndex)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0x137, "Null node pointer.");
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0x138, "Null class data pointer");
            return 5;
        }
        if (node->classId != kZClassNodeWindow) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Window.c",
                0x139,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                node->classId,
                kZClassNodeWindow
            );
            return 3;
        }

        CZWindowDataPartial* data = (CZWindowDataPartial*)(node->classData);
        data->bufferIndex = bufferIndex;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.window.zclass-window-gwwindowsetclearpolygon
     * @recoil-artifact defines .text recoil:function:0x44fb40: CZWindow::gwWindowSetClearPolygon.
     * Purpose: validate a window node and toggle the high-bit enabled flag on
     * the clear-polygon index field.
     */
    int __fastcall gwWindowSetClearPolygon(CZNodePartial * node, int enabled)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0x150, "Null node pointer.");
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0x151, "Null class data pointer");
            return 5;
        }
        if (node->classId != kZClassNodeWindow) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Window.c",
                0x152,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                node->classId,
                kZClassNodeWindow
            );
            return 3;
        }

        CZWindowDataPartial* data = (CZWindowDataPartial*)(node->classData);
        if (enabled == 1) {
            data->clearPolyIndexFlags |= (int)(0x80000000u);
        } else {
            data->clearPolyIndexFlags &= 0x7fffffff;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.window.zclass-window-gwwindowaddclearpolygonvertex
     * @recoil-artifact defines .text recoil:function:0x44fbd0: CZWindow::gwWindowAddClearPolygonVertex.
     * Purpose: validate a window node and append one vertex to the active
     * clear polygon, preserving the vertex-count flag bits.
     */
    int __fastcall gwWindowAddClearPolygonVertex(CZNodePartial * node, const zVec3* point)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0x170, "Null node pointer.");
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0x171, "Null class data pointer");
            return 5;
        }
        if (node->classId != kZClassNodeWindow) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Window.c",
                0x172,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                node->classId,
                kZClassNodeWindow
            );
            return 3;
        }

        CZWindowDataPartial* data = (CZWindowDataPartial*)(node->classData);
        const int polyIndex = data->clearPolyIndexFlags & 0x7fffffff;
        if (polyIndex == 4) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Window.c",
                0x178,
                "ERROR adding window clear polygon vertex.  Clear polygon buffer is full."
            );
            return 1;
        }

        CZWindowClearPoly* poly = &data->clearPolys[polyIndex];
        const int vertIndex = poly->vertCount & 0x7fffffff;
        if (vertIndex == 4) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Window.c",
                0x182,
                "ERROR adding window clear polygon vertex.  Clear polygon vertex buffer is full."
            );
            return 1;
        }

        poly->vertCount |= (int)(0x80000000u);
        poly->vertices[vertIndex].x = point->x;
        poly->vertices[vertIndex].y = point->y;
        poly->vertices[vertIndex].z = point->z;
        poly->vertices[vertIndex].z = 100.0f;

        const int countFlags = poly->vertCount;
        poly->vertCount = (((countFlags + 1) ^ countFlags) & 0x7fffffff) ^ countFlags;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.window.zclass-window-gwwindowcloseclearpolygon
     * @recoil-artifact defines .text recoil:function:0x44fcf0: CZWindow::gwWindowCloseClearPolygon.
     * Purpose: submit the active clear polygon to the renderer and advance the
     * stored clear-polygon index.
     */
    int __fastcall gwWindowCloseClearPolygon(CZNodePartial * node)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0x1a7, "Null node pointer.");
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0x1a8, "Null class data pointer");
            return 5;
        }
        if (node->classId != kZClassNodeWindow) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Window.c",
                0x1a9,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                node->classId,
                kZClassNodeWindow
            );
            return 3;
        }

        CZWindowDataPartial* data = (CZWindowDataPartial*)(node->classData);
        const int polyIndex = data->clearPolyIndexFlags & 0x7fffffff;
        if (polyIndex == 4) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Window.c",
                0x1af,
                "ERROR closing window clear polygon.  Clear polygon buffer is full."
            );
            return 1;
        }

        CZWindowClearPoly* poly = &data->clearPolys[polyIndex];
        zRndr::SpanOcclusionAddPolygon(poly->vertices, poly->vertCount & 0x7fffffff);
        data->clearPolyIndexFlags = (data->clearPolyIndexFlags + 1) | (int)(0x80000000u);
        return polyIndex;
    }
}
