#include "zclass.h"

#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zRender/zrndr.h"

#include <stdio.h>
#include <stdlib.h>

namespace
{
    const int kZClassNodeWindow = 3;

    /**
     * Window class data as Window.c maintains it.
     * Evidence: 0x44fcf0 advances the clear-polygon count with VC5's bitfield
     * merge ('inc; or 0x80000000'); every reader masks 0x7fffffff / tests bit 31.
     * Purpose: the clear-polygon count word is a 31-bit count plus an enable bit.
     */
    struct CZWindowClearPolygon {
        zVec3 vertices[4];
        unsigned int vertCount : 31;
        unsigned int hasVertices : 1;
    };

    struct CZWindowData {
        int viewportWidth;
        int viewportHeight;
        int resolutionWidth;
        int resolutionHeight;
        CZWindowClearPolygon clearPolys[4];
        unsigned int clearPolyCount : 31;
        unsigned int clearPolysEnabled : 1;
        int bufferIndex;
        void* buffer;
        int fbWidth;
        int fbHeight;
        int fbBpp;
    };
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
     *
     *
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
        CZWindowData* data = (CZWindowData*)(calloc(1, sizeof(CZWindowData)));
        node->classData = data;
        data->resolutionWidth = 1;
        data->resolutionHeight = 1;
        data->bufferIndex = -1;

        int pitchBytes;
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
     * @recoil-anchor recoil:anchor:zclass.window.czclass-remove-child-checked
     * @recoil-artifact defines .text recoil:function:0x44f870: CZClass::RemoveChildChecked.
     * @recoil-match byte
     *
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
     * @recoil-match byte
     *
     * Purpose: validate a window node and store the requested render
     * resolution in its window data record.
     */
    int __fastcall gwWindowSetResolution(CZNodePartial * node, int width, int height)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0xcd, "Null node pointer.");
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0xce, "Null class data pointer");
            return 5;
        }
        if (node->classId != kZClassNodeWindow) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Window.c",
                0xcf,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                node->classId,
                kZClassNodeWindow
            );
            return 3;
        }

        ((CZWindowData*)(node->classData))->resolutionWidth = width;
        ((CZWindowData*)(node->classData))->resolutionHeight = height;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.window.zclass-window-gwwindowgetresolution
     * @recoil-artifact defines .text recoil:function:0x44f930: CZWindow::gwWindowGetResolution.
     * @recoil-match byte
     *
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

        *outWidth = ((CZWindowData*)(node->classData))->resolutionWidth;
        *outHeight = ((CZWindowData*)(node->classData))->resolutionHeight;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.window.zclass-window-gwwindowsetsize
     * @recoil-artifact defines .text recoil:function:0x44f9c0: CZWindow::gwWindowSetSize.
     * @recoil-match byte
     *
     * Purpose: validate a window node and store the requested viewport size in
     * its window data record.
     */
    int __fastcall gwWindowSetSize(CZNodePartial * node, int width, int height)
    {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0x102, "Null node pointer.");
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0x103, "Null class data pointer");
            return 5;
        }
        if (node->classId != kZClassNodeWindow) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Window.c",
                0x104,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                node->classId,
                kZClassNodeWindow
            );
            return 3;
        }

        ((CZWindowData*)(node->classData))->viewportWidth = width;
        ((CZWindowData*)(node->classData))->viewportHeight = height;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.window.zclass-window-gwwindowgetsize
     * @recoil-artifact defines .text recoil:function:0x44fa40: CZWindow::gwWindowGetSize.
     * @recoil-match byte
     *
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

        *outWidth = ((CZWindowData*)(node->classData))->viewportWidth;
        *outHeight = ((CZWindowData*)(node->classData))->viewportHeight;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.window.zclass-window-gwwindowsetbuffer
     * @recoil-artifact defines .text recoil:function:0x44fad0: CZWindow::gwWindowSetBuffer.
     * @recoil-match byte
     *
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

        CZWindowData* data = (CZWindowData*)(node->classData);
        data->bufferIndex = bufferIndex;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.window.zclass-window-gwwindowsetclearpolygon
     * @recoil-artifact defines .text recoil:function:0x44fb40: CZWindow::gwWindowSetClearPolygon.
     * @recoil-match byte
     *
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

        CZWindowData* data = (CZWindowData*)(node->classData);
        if (enabled == 1) {
            data->clearPolysEnabled = 1;
        } else {
            data->clearPolysEnabled = 0;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.window.zclass-window-gwwindowaddclearpolygonvertex
     * @recoil-artifact defines .text recoil:function:0x44fbd0: CZWindow::gwWindowAddClearPolygonVertex.
     * @recoil-match byte
     *
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

        CZWindowData* data = (CZWindowData*)(node->classData);
        const int polyIndex = data->clearPolyCount;
        if (polyIndex == 4) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Window.c",
                0x178,
                "ERROR adding window clear polygon vertex.  Clear polygon buffer is full."
            );
            return 1;
        }

        const int vertIndex = data->clearPolys[polyIndex].vertCount;
        if (vertIndex == 4) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Window.c",
                0x182,
                "ERROR adding window clear polygon vertex.  Clear polygon vertex buffer is full."
            );
            return 1;
        }

        data->clearPolys[polyIndex].hasVertices = 1;
        data->clearPolys[polyIndex].vertices[vertIndex] = *point;
        data->clearPolys[polyIndex].vertices[vertIndex].z = 100.0f;
        data->clearPolys[polyIndex].vertCount++;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.window.zclass-window-gwwindowcloseclearpolygon
     * @recoil-artifact defines .text recoil:function:0x44fcf0: CZWindow::gwWindowCloseClearPolygon.
     * @recoil-match byte
     *
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

        CZWindowData* data = (CZWindowData*)(node->classData);
        const int polyIndex = data->clearPolyCount;
        if (polyIndex == 4) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Window.c",
                0x1af,
                "ERROR closing window clear polygon.  Clear polygon buffer is full."
            );
            return 1;
        }

        zRndr::SpanOcclusionAddPolygon(data->clearPolys[polyIndex].vertices, data->clearPolys[polyIndex].vertCount);
        data->clearPolyCount++;
        data->clearPolysEnabled = 1;
        return polyIndex;
    }
}
