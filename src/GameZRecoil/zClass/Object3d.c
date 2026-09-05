#include "zclass.h"

#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/include/zclip_alt.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <stdlib.h>
#include <string.h>

namespace {
    const int kZClassNodeObject3D = 5;
    const int kObject3DLitFlag = 0x02;
    const int kObject3DVisibleFlag = 0x04;
    const int kObject3DTransformDirtyFlag = 0x20;
    const int kNodeBoundsDirtyFlag = 0x04;
    const int kSingleParentFlag = 0x00080000;
    const int kNodeTransformDirtyPropagatedFlag = 0x02000000;

    const char kObject3DSourceFile[] = "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c";
}

namespace zClass_Node {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.propagatetransformdirtyrecursive
     * @recoil-artifact defines .text recoil:function:0x44d990: zClass_Node::PropagateTransformDirtyRecursive
     * Purpose: mark Object3D transform data, node bounds, and descendants dirty
     * for transform-dependent world/render updates.
     */
    void __fastcall PropagateTransformDirtyRecursive(
        zClass_NodePartial * self
    ) {
        if (self->classId == kZClassNodeObject3D) {
            *(int *)(self->classData) |= kObject3DTransformDirtyFlag;
        }

        self->boundsFlags |= kNodeBoundsDirtyFlag;
        self->flags |= kNodeTransformDirtyPropagatedFlag;

        for (int i = 0; i < self->listCountB; ++i) {
            zClass_NodePartial *child = self->listB[i];
            if ((child->flags & kNodeTransformDirtyPropagatedFlag) == 0) {
                PropagateTransformDirtyRecursive(child);
            }
        }
    }
}

namespace zClass_Object3D {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.propagatetransformdirty
     * @recoil-artifact defines .text recoil:function:0x44d9e0: zClass_Object3D::PropagateTransformDirty
     * Purpose: reset local Object3D transform fields to identity defaults and
     * queue a transform/bounds dirty update for the node subtree.
     */
    int __fastcall PropagateTransformDirty(zClass_NodePartial * node) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c",
                0xe8,
                "Null node pointer."
            );
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c",
                0xe9,
                "Null class data pointer"
            );
            return 5;
        }

        zClass_Object3DDataPartial *data = (zClass_Object3DDataPartial *)(node->classData);
        /* Retail keeps the rotation-zero and scale-one dword stores paired. */
        volatile unsigned int *scaleBits = (volatile unsigned int *)(&data->scale.x);
        int count = 3;
        do {
            scaleBits[-3] = 0;
            *scaleBits = 0x3f800000;
            ++scaleBits;
            --count;
        } while (count != 0);

        memset(
            data->localMatrix,
            0,
            sizeof(data->localMatrix)
        );
        data->localMatrix[0] = 1.0f;
        data->localMatrix[4] = 1.0f;
        data->localMatrix[8] = 1.0f;
        data->flags = (data->flags & ~0x10) | 0x09;

        zClass_Node::PropagateTransformDirtyRecursive(node);
        if ((node->flags & 0x01) == 0) {
            zClass_TypeList::Insert(
                7,
                node
            );
            node->flags |= 0x01;
        }
        node->flags |= 0x02;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dinit
     * @recoil-artifact defines .text recoil:function:0x44daa0: zClass_Object3D::gwObject3DInit
     * Purpose: allocate an Object3D node, attach zeroed Object3D data, and
     * initialize/queue its default transform state.
     */
    zClass_NodePartial *__cdecl gwObject3DInit() {
        zClass_NodePartial *node = zClass_Class::AllocNodeFromFreeList();
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x12f,
                "Null node pointer."
            );
            return 0;
        }

        node->classId = kZClassNodeObject3D;
        node->classData = calloc(
            1,
            sizeof(zClass_Object3DDataPartial)
        );
        return PropagateTransformDirty(node) == 0 ? node : 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.deletenode
     * @recoil-artifact defines .text recoil:logical-function:0x44db00:zclass-object3d-delete-node: zClass_Object3D::DeleteNode
     * Purpose: route Object3D deletion through the generic node free path.
     */
    int __fastcall DeleteNode(zClass_NodePartial * node) {
        return zClass_Class::TryFreeNode(node);
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3daddchild
     * @recoil-artifact defines .text recoil:function:0x44db10: zClass_Object3D::gwObject3DAddChild
     * Purpose: validate parent, child, and Object3D class data before delegating
     * to the generic child-add helper.
     */
    gwObject3DAddChild(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ) {
        if (parent == 0) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x178,
                "Null node pointer."
            );
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x179,
                "Null node pointer."
            );
            return 5;
        }
        if (parent->classData == 0) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x17a,
                "Null class data pointer"
            );
            return 5;
        }

        return zClass_Class::AddChildGeneric(
            parent,
            child
        );
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.removechild
     * @recoil-artifact defines .text recoil:function:0x44db60: zClass_Object3D::RemoveChild
     * Purpose: validate parent, child, and Object3D class data before delegating
     * to the generic child-removal helper.
     */
    RemoveChild(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ) {
        if (parent == 0) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x194,
                "Null node pointer."
            );
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x195,
                "Null node pointer."
            );
            return 5;
        }
        if (parent->classData == 0) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x196,
                "Null class data pointer"
            );
            return 5;
        }

        return zClass_Class::RemoveChildGeneric(
            parent,
            child
        );
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dsetvisibleflag
     * @recoil-artifact defines .text recoil:function:0x44dbb0: zClass_Object3D::gwObject3DSetVisibleFlag
     * Purpose: validate Object3D data and set or clear the visible render flag.
     */
    gwObject3DSetVisibleFlag(
        zClass_NodePartial * node,
        int visible
    ) {
        zClass_Object3DDataPartial *data;

        if (node == 0) {
            zError::ReportOld(0x400, kObject3DSourceFile, 0x1b1, "Null node pointer.");
            return 5;
        }
        if (node->classId != kZClassNodeObject3D) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x1b3,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                kZClassNodeObject3D,
                node->classId
            );
            return 3;
        }

        data = (zClass_Object3DDataPartial *)(node->classData);

        if (visible != 0) {
            data->flags |= kObject3DVisibleFlag;
        } else {
            data->flags &= ~kObject3DVisibleFlag;
        }
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dsetcoloralpha
     * @recoil-artifact defines .text recoil:function:0x44dc30: zClass_Object3D::gwObject3DSetColorAlpha
     * Purpose: validate Object3D data, clamp alpha/color inputs, and store the
     * software color override state.
     */
    gwObject3DSetColorAlpha(
        zClass_NodePartial * node,
        zColorRgb * color,
        float alpha
    ) {
        zClass_Object3DDataPartial *data;

        if (node == 0) {
            zError::ReportOld(0x400, kObject3DSourceFile, 0x1d9, "Null node pointer.");
            return 5;
        }
        if (node->classId != kZClassNodeObject3D) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x1db,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                kZClassNodeObject3D,
                node->classId
            );
            return 3;
        }

        data = (zClass_Object3DDataPartial *)(node->classData);

        data->colorAlpha = alpha > 1.0f ? 1.0f : (alpha < 0.0f ? 0.0f : alpha);
        if (color != 0) {
            data->color.red = color->red > 1.0f ? 1.0f :
                (color->red < 0.0f ? 0.0f : color->red);
            data->color.green = color->green > 1.0f ? 1.0f :
                (color->green < 0.0f ? 0.0f : color->green);
            data->color.blue = color->blue > 1.0f ? 1.0f :
                (color->blue < 0.0f ? 0.0f : color->blue);
        }

        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dsetalphascale
     * @recoil-artifact defines .text recoil:function:0x44dd90: zClass_Object3D::gwObject3DSetAlphaScale
     * Purpose: validate Object3D data and store the alpha-scale render value.
     */
    gwObject3DSetAlphaScale(
        zClass_NodePartial * node,
        float alphaScale
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c",
                0x21f,
                "Null node pointer."
            );
            return 5;
        }

        zClass_Object3DDataPartial *data =
            (zClass_Object3DDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c",
                0x220,
                "Null class data pointer"
            );
            return 5;
        }

        if (node->classId != kZClassNodeObject3D) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c",
                0x221,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                node->classId,
                kZClassNodeObject3D
            );
            return 3;
        }

        data->alphaScale = alphaScale;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dgetalphascale
     * @recoil-artifact defines .text recoil:function:0x44de10: zClass_Object3D::gwObject3DGetAlphaScale
     * Purpose: validate Object3D data and return the stored alpha-scale value.
     */
    gwObject3DGetAlphaScale(
        zClass_NodePartial * node,
        float *outAlphaScale
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c",
                0x238,
                "Null node pointer."
            );
            return 5;
        }

        zClass_Object3DDataPartial *data =
            (zClass_Object3DDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c",
                0x239,
                "Null class data pointer"
            );
            return 5;
        }

        if (node->classId != kZClassNodeObject3D) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c",
                0x23a,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                node->classId,
                kZClassNodeObject3D
            );
            return 3;
        }

        *outAlphaScale = data->alphaScale;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dsetlitflag
     * @recoil-artifact defines .text recoil:function:0x44de80: zClass_Object3D::gwObject3DSetLitFlag
     * Purpose: validate Object3D data and set or clear the lit/model-reference
     * render flag.
     */
    int __fastcall gwObject3DSetLitFlag(
        zClass_NodePartial * node,
        int lit
    ) {
        if (node == 0) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c",
                0x254,
                "Null node pointer."
            );
            return 5;
        }

        zClass_Object3DDataPartial *data =
            (zClass_Object3DDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c",
                0x255,
                "Null class data pointer"
            );
            return 5;
        }

        if (node->classId != kZClassNodeObject3D) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c",
                0x256,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                node->classId,
                kZClassNodeObject3D
            );
            return 3;
        }

        if (lit != 0) {
            data->flags |= kObject3DLitFlag;
        } else {
            data->flags &= ~kObject3DLitFlag;
        }
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dsetscale
     * @recoil-artifact defines .text recoil:function:0x44df00: zClass_Object3D::gwObject3DSetScale
     * Purpose: validate Object3D data, store local scale, update identity state,
     * and queue transform/bounds propagation.
     */
    gwObject3DSetScale(
        zClass_NodePartial * node,
        float x,
        float y,
        float z
    ) {
        zClass_Object3DDataPartial *data;

        if (node == 0) {
            zError::ReportOld(0x400, kObject3DSourceFile, 0x294, "Null node pointer.");
            return 5;
        }
        data = (zClass_Object3DDataPartial *)(node->classData);

        if ((data->flags & 0x10) != 0) {
            data->flags &= ~0x10;
        }
        data->scale.x = x;
        data->scale.y = y;
        data->scale.z = z;
        if ((data->flags & 0x08) != 0 && (x != 1.0 || y != 1.0 || z != 1.0)) {
            data->flags &= ~0x08;
        }

        data->flags |= 0x01;
        zClass_Node::PropagateTransformDirtyRecursive(node);
        if ((node->flags & 0x01) == 0) {
            zClass_TypeList::Insert(7, node);
            node->flags |= 0x01;
        }
        node->flags |= 0x02;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dgetscale
     * @recoil-artifact defines .text recoil:function:0x44dfd0: zClass_Object3D::gwObject3DGetScale
     * Purpose: validate Object3D data and return the local scale vector.
     */
    gwObject3DGetScale(
        zClass_NodePartial * node,
        float *outX,
        float *outY,
        float *outZ
    ) {
        zClass_Object3DDataPartial *data;

        if (node == 0) {
            zError::ReportOld(0x400, kObject3DSourceFile, 0x331, "Null node pointer.");
            return 5;
        }
        data = (zClass_Object3DDataPartial *)(node->classData);

        *outX = data->scale.x;
        *outY = data->scale.y;
        *outZ = data->scale.z;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dsetrotation
     * @recoil-artifact defines .text recoil:function:0x44e030: zClass_Object3D::gwObject3DSetRotation
     * Purpose: validate Object3D data, store local rotation, update identity
     * state, and queue transform/bounds propagation.
     */
    gwObject3DSetRotation(
        zClass_NodePartial * node,
        float x,
        float y,
        float z
    ) {
        zClass_Object3DDataPartial *data;

        if (node == 0) {
            zError::ReportOld(0x400, kObject3DSourceFile, 0x357, "Null node pointer.");
            return 5;
        }
        data = (zClass_Object3DDataPartial *)(node->classData);

        if ((data->flags & 0x10) != 0) {
            data->flags &= ~0x10;
        }
        data->rotation.x = x;
        data->rotation.y = y;
        data->rotation.z = z;
        if ((data->flags & 0x08) != 0 && (x != 0.0f || y != 0.0f || z != 0.0f)) {
            data->flags &= ~0x08;
        }

        data->flags |= 0x01;
        zClass_Node::PropagateTransformDirtyRecursive(node);
        if ((node->flags & 0x01) == 0) {
            zClass_TypeList::Insert(7, node);
            node->flags |= 0x01;
        }
        node->flags |= 0x02;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dgetrotation
     * @recoil-artifact defines .text recoil:function:0x44e110: zClass_Object3D::gwObject3DGetRotation
     * Purpose: validate Object3D data and return the local rotation vector.
     */
    gwObject3DGetRotation(
        zClass_NodePartial * node,
        float *outX,
        float *outY,
        float *outZ
    ) {
        zClass_Object3DDataPartial *data;

        if (node == 0) {
            zError::ReportOld(0x400, kObject3DSourceFile, 0x3a9, "Null node pointer.");
            return 5;
        }
        data = (zClass_Object3DDataPartial *)(node->classData);

        *outX = data->rotation.x;
        *outY = data->rotation.y;
        *outZ = data->rotation.z;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dtranslaterotation
     * @recoil-artifact defines .text recoil:function:0x44e170: zClass_Object3D::gwObject3DTranslateRotation
     * Purpose: validate Object3D data, add local rotation deltas, update
     * identity state, and queue transform/bounds propagation.
     */
    gwObject3DTranslateRotation(
        zClass_NodePartial * node,
        float dx,
        float dy,
        float dz
    ) {
        zClass_Object3DDataPartial *data;

        if (node == 0) {
            zError::ReportOld(0x400, kObject3DSourceFile, 0x3cf, "Null node pointer.");
            return 5;
        }
        data = (zClass_Object3DDataPartial *)(node->classData);

        if ((data->flags & 0x10) != 0) {
            data->flags &= ~0x10;
        }
        data->rotation.x += dx;
        data->rotation.y += dy;
        data->rotation.z += dz;
        if ((data->flags & 0x08) != 0 &&
            (data->rotation.x != 0.0f || data->rotation.y != 0.0f ||
             data->rotation.z != 0.0f)) {
            data->flags &= ~0x08;
        }

        data->flags |= 0x01;
        zClass_Node::PropagateTransformDirtyRecursive(node);
        if ((node->flags & 0x01) == 0) {
            zClass_TypeList::Insert(7, node);
            node->flags |= 0x01;
        }
        node->flags |= 0x02;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dgetposition
     * @recoil-artifact defines .text recoil:function:0x44e270: zClass_Object3D::gwObject3DGetPosition
     * Purpose: validate Object3D data and return translation components from
     * the local matrix.
     */
    gwObject3DGetPosition(
        zClass_NodePartial * node,
        float *outX,
        float *outY,
        float *outZ
    ) {
        zClass_Object3DDataPartial *data;

        if (node == 0) {
            zError::ReportOld(0x400, kObject3DSourceFile, 0x41a, "Null node pointer.");
            return 5;
        }
        if (node->classId != kZClassNodeObject3D) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x41c,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                kZClassNodeObject3D,
                node->classId
            );
            return 3;
        }
        data = (zClass_Object3DDataPartial *)(node->classData);

        *outX = data->localMatrix[9];
        *outY = data->localMatrix[10];
        *outZ = data->localMatrix[11];
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dsetposition
     * @recoil-artifact defines .text recoil:function:0x44e300: zClass_Object3D::gwObject3DSetPosition
     * Purpose: validate Object3D data, store local matrix translation, update
     * identity state, and queue transform/bounds propagation.
     */
    gwObject3DSetPosition(
        zClass_NodePartial * node,
        float x,
        float y,
        float z
    ) {
        zClass_Object3DDataPartial *data;

        if (node == 0) {
            zError::ReportOld(0x400, kObject3DSourceFile, 0x441, "Null node pointer.");
            return 5;
        }
        data = (zClass_Object3DDataPartial *)(node->classData);

        data->localMatrix[9] = x;
        data->localMatrix[10] = y;
        data->localMatrix[11] = z;
        if ((data->flags & 0x08) != 0 && (x != 0.0f || y != 0.0f || z != 0.0f)) {
            data->flags &= ~0x08;
        }

        data->flags |= 0x01;
        zClass_Node::PropagateTransformDirtyRecursive(node);
        if ((node->flags & 0x01) == 0) {
            zClass_TypeList::Insert(7, node);
            node->flags |= 0x01;
        }
        node->flags |= 0x02;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dtranslateposition
     * @recoil-artifact defines .text recoil:function:0x44e3d0: zClass_Object3D::gwObject3DTranslatePosition
     * Purpose: validate Object3D data, add local translation deltas, update
     * identity state, and queue transform/bounds propagation.
     */
    gwObject3DTranslatePosition(
        zClass_NodePartial * node,
        float dx,
        float dy,
        float dz
    ) {
        zClass_Object3DDataPartial *data;

        if (node == 0) {
            zError::ReportOld(0x400, kObject3DSourceFile, 0x47e, "Null node pointer.");
            return 5;
        }
        if (node->classId != kZClassNodeObject3D) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x480,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                kZClassNodeObject3D,
                node->classId
            );
            return 3;
        }
        data = (zClass_Object3DDataPartial *)(node->classData);

        data->localMatrix[9] += dx;
        data->localMatrix[10] += dy;
        data->localMatrix[11] += dz;
        if ((data->flags & 0x08) != 0 &&
            (data->localMatrix[9] != 0.0f || data->localMatrix[10] != 0.0f ||
             data->localMatrix[11] != 0.0f)) {
            data->flags &= ~0x08;
        }

        data->flags |= 0x01;
        zClass_Node::PropagateTransformDirtyRecursive(node);
        if ((node->flags & 0x01) == 0) {
            zClass_TypeList::Insert(7, node);
            node->flags |= 0x01;
        }
        node->flags |= 0x02;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dsetmatrix
     * @recoil-artifact defines .text recoil:function:0x44e4f0: zClass_Object3D::gwObject3DSetMatrix
     * Purpose: validate Object3D data, copy local matrix storage when needed,
     * mark matrix-authored transform state, and enqueue transform propagation.
     */
    gwObject3DSetMatrix(
        zClass_NodePartial * node,
        float *matrix
    ) {
        zClass_Object3DDataPartial *data;

        if (node == 0) {
            zError::ReportOld(0x400, kObject3DSourceFile, 0x4bb, "Null node pointer.");
            return 5;
        }
        if (node->classId != kZClassNodeObject3D) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x4bd,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                kZClassNodeObject3D,
                node->classId
            );
            return 3;
        }
        data = (zClass_Object3DDataPartial *)(node->classData);

        if (matrix != data->localMatrix) {
            memcpy(
                data->localMatrix,
                matrix,
                sizeof(data->localMatrix)
            );
        }

        data->flags = (data->flags & ~0x08) | 0x11;
        zClass_Node::PropagateTransformDirtyRecursive(node);
        if ((node->flags & 0x01) == 0) {
            zClass_TypeList::Insert(
                7,
                node
            );
            node->flags |= 0x01;
        }
        node->flags |= 0x02;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dgetmatrixptr
     * @recoil-artifact defines .text recoil:function:0x44e5b0: zClass_Object3D::gwObject3DGetMatrixPtr
     * Purpose: validate Object3D data and return a pointer to the local matrix
     * storage.
     */
    float *__fastcall gwObject3DGetMatrixPtr(zClass_NodePartial * node) {
        zClass_Object3DDataPartial *data;

        if (node == 0) {
            zError::ReportOld(0x400, kObject3DSourceFile, 0x4fe, "Null node pointer.");
            return 0;
        }
        if (node->classData == 0) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x4ff,
                "Null class data pointer"
            );
            return 0;
        }
        if (node->classId != kZClassNodeObject3D) {
            zError::ReportOld(
                0x400,
                kObject3DSourceFile,
                0x500,
                "Bad Class Found.\n Wanted (%d)\n Found (%d)",
                kZClassNodeObject3D,
                node->classId
            );
            return 0;
        }
        data = (zClass_Object3DDataPartial *)(node->classData);

        return data->localMatrix;
    }
}

/**
 * Purpose: initialize an empty model-reference lerp queue. Retail startup
 * 0x437ff0 reaches the constructor inlined into global initialization.
 */
inline zClass_Object3D_ModelRefLerpQueueState::zClass_Object3D_ModelRefLerpQueueState() {
    listAux = 0;
    tail = 0;
    head = 0;
    count = 0;
}

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.clearglobalstate
 * @recoil-artifact emits .text recoil:function:0x438000: Global queue initialization.
 * Purpose: own the queue whose native construction is registered in CRT startup.
 */
zClass_Object3D_ModelRefLerpQueueState g_ModelRefLerpQueueState;
}

namespace zClass_Object3D_ModelRefLerpQueue {

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.add
     * @recoil-artifact defines .text recoil:function:0x438020: zClass_Object3D_ModelRefLerpQueue::Add
     * Purpose: allocate and append a model-reference lerp task, normalize fade
     * direction/rate, and enable the node's lit/model-reference flag.
     */
    void __fastcall Add(
        zClass_NodePartial * node,
        void *callbackCtx,
        void *onComplete,
        float startModelRef,
        float targetModelRef,
        float durationSec
    ) {
        zClass_Object3D_ModelRefLerpTask *task = new zClass_Object3D_ModelRefLerpTask;
        memset(
            task,
            0,
            sizeof(*task)
        );

        if (task != 0) {
            task->next = 0;
            if (g_ModelRefLerpQueueState.count == 0) {
                g_ModelRefLerpQueueState.head = task;
            } else {
                g_ModelRefLerpQueueState.tail->next = task;
            }

            g_ModelRefLerpQueueState.tail = task;
            task->next = 0;
            ++g_ModelRefLerpQueueState.count;
        }

        task->node = node;
        task->onComplete = onComplete;
        task->callbackCtx = callbackCtx;

        targetModelRef = targetModelRef > 1.0f ? 1.0f :
            (targetModelRef < 0.0f ? 0.0f : targetModelRef);
        task->targetModelRef = targetModelRef;
        startModelRef = startModelRef > 1.0f ? 1.0f :
            (startModelRef < 0.0f ? 0.0f : startModelRef);
        const float delta = targetModelRef - startModelRef;
        task->currentModelRef = startModelRef;
        if (durationSec == 0.0f) {
            task->modelRefDeltaPerSec = 99999997952.0f;
        } else {
            task->modelRefDeltaPerSec = delta / durationSec;
        }
        if (delta < 0.0f) {
            task->targetModelRef = 1.0f - targetModelRef;
            task->invertModelRef = 1;
            task->currentModelRef = 1.0f - startModelRef;
            task->modelRefDeltaPerSec = -task->modelRefDeltaPerSec;
        } else {
            task->invertModelRef = 0;
        }

        zClass_Object3D::gwObject3DSetLitFlag(
            node,
            1
        );
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.reset
     * @recoil-artifact defines .text recoil:function:0x438180: zClass_Object3D_ModelRefLerpQueue::Reset
     * Purpose: delete all queued model-reference lerp tasks and zero the global
     * queue state.
     */
    void __cdecl Reset() {
        zClass_Object3D_ModelRefLerpTask *task = g_ModelRefLerpQueueState.head;
        while (task != 0) {
            zClass_Object3D_ModelRefLerpTask *const next = task != 0
                ? task->next
                : 0;
            ::operator delete(task);
            task = next;
        }

        g_ModelRefLerpQueueState.listAux = 0;
        g_ModelRefLerpQueueState.tail = 0;
        g_ModelRefLerpQueueState.head = 0;
        g_ModelRefLerpQueueState.count = 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.update
     * @recoil-artifact defines .text recoil:function:0x4381d0: zClass_Object3D_ModelRefLerpQueue::Update
     * Purpose: advance queued model-reference fades by frame time, apply alpha
     * scale, invoke completion callbacks, and unlink finished tasks.
     */
    void __cdecl Update() {
        if (g_ModelRefLerpQueueState.count == 0) {
            return;
        }

        zClass_Object3D_ModelRefLerpTask *task = g_ModelRefLerpQueueState.head;
        if (task == 0) {
            return;
        }

        while (task != 0) {
            task->currentModelRef += task->modelRefDeltaPerSec * g_FrameDeltaTimeSec;
            if (task->currentModelRef > 1.0f) {
                task->currentModelRef = 1.0f;
            } else if (task->currentModelRef < 0.0f) {
                task->currentModelRef = 0.0f;
            }

            float alphaScale = task->currentModelRef;
            if (task->invertModelRef == 1) {
                alphaScale = 1.0f - alphaScale;
            }

            zClass_Object3D::gwObject3DSetAlphaScale(
                task->node,
                alphaScale
            );

            if (task->currentModelRef >= task->targetModelRef) {
                union {
                    void *raw;
                    zClass_Object3D_ModelRefLerpCallback callback;
                } onComplete = {0};
                onComplete.raw = task->onComplete;
                if (onComplete.callback != 0) {
                    onComplete.callback(task->callbackCtx);
                }

                if (alphaScale == 1.0f) {
                    zClass_Object3D::gwObject3DSetLitFlag(
                        task->node,
                        0
                    );
                }

                zClass_Object3D_ModelRefLerpTask *const nextTask = task != 0
                    ? task->next
                    : 0;
                if (task != 0) {
                    if (g_ModelRefLerpQueueState.count != 0) {
                        zClass_Object3D_ModelRefLerpTask *prevTask = g_ModelRefLerpQueueState.head;
                        if (task == prevTask) {
                            --g_ModelRefLerpQueueState.count;
                            g_ModelRefLerpQueueState.head = task->next;
                            if (g_ModelRefLerpQueueState.head == 0) {
                                g_ModelRefLerpQueueState.listAux = 0;
                                g_ModelRefLerpQueueState.tail = 0;
                            }
                            ::operator delete(task);
                        } else {
                            while (prevTask != 0) {
                                if (prevTask->next == task) {
                                    --g_ModelRefLerpQueueState.count;
                                    prevTask->next = task->next;
                                    if (g_ModelRefLerpQueueState.tail == task) {
                                        g_ModelRefLerpQueueState.tail = prevTask;
                                    }
                                    ::operator delete(task);
                                    break;
                                }
                                prevTask = prevTask->next;
                            }
                        }
                    }
                }
                task = nextTask;
            } else {
                task = task != 0
                    ? task->next
                    : 0;
            }
        }
    }
}

namespace zClass_Node {
    /**
     * Source-shape note: the definition is emitted by cls_util.c; Object3d.c
     * retains related callers and the public declaration.
     */
}
