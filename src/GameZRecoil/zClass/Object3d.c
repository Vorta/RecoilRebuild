#include "cls_api.h"
#include "zclass.h"

#include "GameZRecoil/include/zclip_alt.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <stdlib.h>
#include <string.h>

enum {
    kZClassNodeObject3D = 5,
    kObject3DLitFlag = 0x02,
    kObject3DVisibleFlag = 0x04,
    kObject3DTransformDirtyFlag = 0x20,
    kNodeBoundsDirtyFlag = 0x04,
    kSingleParentFlag = 0x00080000,
    kNodeTransformDirtyPropagatedFlag = 0x02000000
};

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.propagatetransformdirtyrecursive
 * @recoil-artifact defines .text recoil:function:0x44d990: CZNode::PropagateTransformDirtyRecursive
 * @recoil-match byte
 *
 * Purpose: mark Object3D transform data, node bounds, and descendants dirty
 * for transform-dependent world/render updates.
 */
void __fastcall PropagateTransformDirtyRecursive(CZNodePartial* self)
{
    int i;
    if (self->classId == kZClassNodeObject3D) {
        *(int*)(self->classData) |= kObject3DTransformDirtyFlag;
    }

    self->boundsFlags |= kNodeBoundsDirtyFlag;
    self->flags |= kNodeTransformDirtyPropagatedFlag;

    for (i = 0; i < self->listCountB; ++i) {
        CZNodePartial* child = self->listB[i];
        if ((child->flags & kNodeTransformDirtyPropagatedFlag) == 0) {
            PropagateTransformDirtyRecursive(child);
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.propagatetransformdirty
 * @recoil-artifact defines .text recoil:function:0x44d9e0: CZObject3D::PropagateTransformDirty
 * @recoil-match byte
 *
 * Purpose: reset local Object3D transform fields to identity defaults and
 * queue a transform/bounds dirty update for the node subtree.
 */
int __fastcall PropagateTransformDirty(CZNodePartial* node)
{
    CZObject3DDataPartial* data;
    volatile unsigned int* scaleBits;
    int count;
    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0xe8, "Null node pointer.");
        return 5;
    }
    if (node->classData == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0xe9, "Null class data pointer");
        return 5;
    }

    data = (CZObject3DDataPartial*)(node->classData);
    /* Retail keeps the rotation-zero and scale-one dword stores paired. */
    scaleBits = (volatile unsigned int*)(&data->scale.x);
    count = 3;
    do {
        scaleBits[-3] = 0;
        *scaleBits = 0x3f800000;
        ++scaleBits;
        --count;
    } while (count != 0);

    memset(data->localMatrix, 0, sizeof(data->localMatrix));
    data->localMatrix[0] = 1.0f;
    data->localMatrix[4] = 1.0f;
    data->localMatrix[8] = 1.0f;
    data->flags = (data->flags & ~0x10) | 0x09;

    PropagateTransformDirtyRecursive(node);
    if ((node->flags & 0x01) == 0) {
        CZTypeListInsert(7, node);
        node->flags |= 0x01;
    }
    node->flags |= 0x02;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dinit
 * @recoil-artifact defines .text recoil:function:0x44daa0: CZObject3D::gwObject3DInit
 * @recoil-match byte
 *
 * Purpose: allocate an Object3D node, attach zeroed Object3D data, and
 * initialize/queue its default transform state.
 */
CZNodePartial* __fastcall gwObject3DInit(void)
{
    CZNodePartial* node = gwNodeNew();
    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x12f, "Null node pointer.");
        return 0;
    }

    node->classId = kZClassNodeObject3D;
    node->classData = calloc(1, sizeof(CZObject3DDataPartial));
    return PropagateTransformDirty(node) == 0 ? node : 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.deletenode
 * @recoil-artifact defines .text recoil:logical-function:0x44db00:zclass-object3d-delete-node: CZObject3DDeleteNode
 * Purpose: route Object3D deletion through the generic node free path.
 */
int __fastcall CZObject3DDeleteNode(CZNodePartial* node)
{
    return TryFreeNode(node);
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3daddchild
 * @recoil-artifact defines .text recoil:function:0x44db10: CZObject3D::gwObject3DAddChild
 * @recoil-match byte
 *
 * Purpose: validate parent, child, and Object3D class data before delegating
 * to the generic child-add helper.
 */
gwObject3DAddChild(CZNodePartial * parent, CZNodePartial * child)
{
    if (parent == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x178, "Null node pointer.");
        return 5;
    }
    if (child == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x179, "Null node pointer.");
        return 5;
    }
    if (parent->classData == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x17a, "Null class data pointer");
        return 5;
    }

    return AddChildGeneric(parent, child);
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.removechild
 * @recoil-artifact defines .text recoil:function:0x44db60: CZObject3DRemoveChild
 * @recoil-match byte
 *
 * Purpose: validate parent, child, and Object3D class data before delegating
 * to the generic child-removal helper.
 */
CZObject3DRemoveChild(CZNodePartial * parent, CZNodePartial * child)
{
    if (parent == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x194, "Null node pointer.");
        return 5;
    }
    if (child == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x195, "Null node pointer.");
        return 5;
    }
    if (parent->classData == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x196, "Null class data pointer");
        return 5;
    }

    return RemoveChildGeneric(parent, child);
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dsetvisibleflag
 * @recoil-artifact defines .text recoil:function:0x44dbb0: CZObject3D::gwObject3DSetVisibleFlag
 * @recoil-match byte
 *
 * Purpose: validate Object3D data and set or clear the visible render flag.
 */
gwObject3DSetVisibleFlag(CZNodePartial * node, int visible)
{
    CZObject3DDataPartial* data;

    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x1b1, "Null node pointer.");
        return 5;
    }
    if (node->classData == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x1b2, "Null class data pointer");
        return 5;
    }
    if (node->classId != kZClassNodeObject3D) {
        ReportOld(
            0x400,
            "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c",
            0x1b3,
            "Bad Class Found.\n Wanted (%d)\n Found (%d)",
            node->classId,
            kZClassNodeObject3D
        );
        return 3;
    }

    data = (CZObject3DDataPartial*)(node->classData);

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
 * @recoil-artifact defines .text recoil:function:0x44dc30: CZObject3D::gwObject3DSetColorAlpha
 * @recoil-match byte
 *
 * Purpose: validate Object3D data, clamp alpha/color inputs, and store the
 * software color override state.
 */
gwObject3DSetColorAlpha(CZNodePartial * node, zColorRgb * color, float alpha)
{
    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x1d9, "Null node pointer.");
        return 5;
    }
    if (node->classData == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x1da, "Null class data pointer");
        return 5;
    }
    if (node->classId != kZClassNodeObject3D) {
        ReportOld(
            0x400,
            "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c",
            0x1db,
            "Bad Class Found.\n Wanted (%d)\n Found (%d)",
            node->classId,
            kZClassNodeObject3D
        );
        return 3;
    }

    if (alpha > 1.0f) {
        alpha = 1.0f;
    } else if (alpha < 0.0f) {
        alpha = 0.0f;
    }
    ((CZObject3DDataPartial*)(node->classData))->colorAlpha = alpha;
    if (color != 0) {
        ((CZObject3DDataPartial*)(node->classData))->color = *color;
        if (((CZObject3DDataPartial*)(node->classData))->color.red > 1.0f) {
            ((CZObject3DDataPartial*)(node->classData))->color.red = 1.0f;
        } else if (((CZObject3DDataPartial*)(node->classData))->color.red < 0.0f) {
            ((CZObject3DDataPartial*)(node->classData))->color.red = 0.0f;
        }
        if (((CZObject3DDataPartial*)(node->classData))->color.green > 1.0f) {
            ((CZObject3DDataPartial*)(node->classData))->color.green = 1.0f;
        } else if (((CZObject3DDataPartial*)(node->classData))->color.green < 0.0f) {
            ((CZObject3DDataPartial*)(node->classData))->color.green = 0.0f;
        }
        if (((CZObject3DDataPartial*)(node->classData))->color.blue > 1.0f) {
            ((CZObject3DDataPartial*)(node->classData))->color.blue = 1.0f;
        } else if (((CZObject3DDataPartial*)(node->classData))->color.blue < 0.0f) {
            ((CZObject3DDataPartial*)(node->classData))->color.blue = 0.0f;
        }
    }

    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dsetalphascale
 * @recoil-artifact defines .text recoil:function:0x44dd90: CZObject3D::gwObject3DSetAlphaScale
 * @recoil-match byte
 *
 * Purpose: validate Object3D data and store the alpha-scale render value.
 */
gwObject3DSetAlphaScale(CZNodePartial * node, float alphaScale)
{
    CZObject3DDataPartial* data;
    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x21f, "Null node pointer.");
        return 5;
    }

    data = (CZObject3DDataPartial*)(node->classData);
    if (data == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x220, "Null class data pointer");
        return 5;
    }

    if (node->classId != kZClassNodeObject3D) {
        ReportOld(
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
 * @recoil-artifact defines .text recoil:function:0x44de10: CZObject3D::gwObject3DGetAlphaScale
 * @recoil-match byte
 *
 * Purpose: validate Object3D data and return the stored alpha-scale value.
 */
gwObject3DGetAlphaScale(CZNodePartial * node, float* outAlphaScale)
{
    CZObject3DDataPartial* data;
    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x238, "Null node pointer.");
        return 5;
    }

    data = (CZObject3DDataPartial*)(node->classData);
    if (data == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x239, "Null class data pointer");
        return 5;
    }

    if (node->classId != kZClassNodeObject3D) {
        ReportOld(
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
 * @recoil-artifact defines .text recoil:function:0x44de80: CZObject3D::gwObject3DSetLitFlag
 * @recoil-match byte
 *
 * Purpose: validate Object3D data and set or clear the lit/model-reference
 * render flag.
 */
int __fastcall gwObject3DSetLitFlag(CZNodePartial* node, int lit)
{
    CZObject3DDataPartial* data;
    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x254, "Null node pointer.");
        return 5;
    }

    data = (CZObject3DDataPartial*)(node->classData);
    if (data == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x255, "Null class data pointer");
        return 5;
    }

    if (node->classId != kZClassNodeObject3D) {
        ReportOld(
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
 * @recoil-artifact defines .text recoil:function:0x44df00: CZObject3D::gwObject3DSetScale
 * @recoil-match byte
 *
 * Purpose: validate Object3D data, store local scale, update identity state,
 * and queue transform/bounds propagation.
 */
gwObject3DSetScale(CZNodePartial * node, float x, float y, float z)
{
    CZObject3DDataPartial* data;

    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x294, "Null node pointer.");
        return 5;
    }
    if (node->classData == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x295, "Null class data pointer");
        return 5;
    }
    data = (CZObject3DDataPartial*)(node->classData);

    if ((data->flags & 0x10) != 0) {
        data->flags &= ~0x10;
    }
    data->scale.x = x;
    data->scale.y = y;
    data->scale.z = z;
    if (x != 1.0 || y != 1.0 || z != 1.0) {
        data->flags &= ~0x08;
    }

    data->flags |= 0x01;
    PropagateTransformDirtyRecursive(node);
    if ((node->flags & 0x01) == 0) {
        CZTypeListInsert(7, node);
        node->flags |= 0x01;
    }
    node->flags |= 0x02;
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dgetscale
 * @recoil-artifact defines .text recoil:function:0x44dfd0: CZObject3D::gwObject3DGetScale
 * @recoil-match byte
 *
 * Purpose: validate Object3D data and return the local scale vector.
 */
gwObject3DGetScale(CZNodePartial * node, float* outX, float* outY, float* outZ)
{
    CZObject3DDataPartial* data;

    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x331, "Null node pointer.");
        return 5;
    }
    if (node->classData == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x332, "Null class data pointer");
        return 5;
    }
    data = (CZObject3DDataPartial*)(node->classData);

    *outX = data->scale.x;
    *outY = data->scale.y;
    *outZ = data->scale.z;
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dsetrotation
 * @recoil-artifact defines .text recoil:function:0x44e030: CZObject3D::gwObject3DSetRotation
 * @recoil-match byte
 *
 * Purpose: validate Object3D data, store local rotation, update identity
 * state, and queue transform/bounds propagation.
 */
gwObject3DSetRotation(CZNodePartial * node, float x, float y, float z)
{
    CZObject3DDataPartial* data;

    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x357, "Null node pointer.");
        return 5;
    }
    if (node->classData == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x358, "Null class data pointer");
        return 5;
    }
    data = (CZObject3DDataPartial*)(node->classData);

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
    PropagateTransformDirtyRecursive(node);
    if ((node->flags & 0x01) == 0) {
        CZTypeListInsert(7, node);
        node->flags |= 0x01;
    }
    node->flags |= 0x02;
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dgetrotation
 * @recoil-artifact defines .text recoil:function:0x44e110: CZObject3D::gwObject3DGetRotation
 * @recoil-match byte
 *
 * Purpose: validate Object3D data and return the local rotation vector.
 */
gwObject3DGetRotation(CZNodePartial * node, float* outX, float* outY, float* outZ)
{
    CZObject3DDataPartial* data;

    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x3a9, "Null node pointer.");
        return 5;
    }
    if (node->classData == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x3aa, "Null class data pointer");
        return 5;
    }
    data = (CZObject3DDataPartial*)(node->classData);

    *outX = data->rotation.x;
    *outY = data->rotation.y;
    *outZ = data->rotation.z;
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dtranslaterotation
 * @recoil-artifact defines .text recoil:function:0x44e170: CZObject3D::gwObject3DTranslateRotation
 * @recoil-match byte
 *
 * Purpose: validate Object3D data, add local rotation deltas, update
 * identity state, and queue transform/bounds propagation.
 */
gwObject3DTranslateRotation(CZNodePartial * node, float dx, float dy, float dz)
{
    CZObject3DDataPartial* data;

    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x3cf, "Null node pointer.");
        return 5;
    }
    if (node->classData == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x3d0, "Null class data pointer");
        return 5;
    }
    data = (CZObject3DDataPartial*)(node->classData);

    if ((data->flags & 0x10) != 0) {
        data->flags &= ~0x10;
    }
    data->rotation.x += dx;
    data->rotation.y += dy;
    data->rotation.z += dz;
    if ((data->flags & 0x08) != 0
        && (data->rotation.x != 0.0f || data->rotation.y != 0.0f || data->rotation.z != 0.0f)) {
        data->flags &= ~0x08;
    }

    data->flags |= 0x01;
    PropagateTransformDirtyRecursive(node);
    if ((node->flags & 0x01) == 0) {
        CZTypeListInsert(7, node);
        node->flags |= 0x01;
    }
    node->flags |= 0x02;
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dgetposition
 * @recoil-artifact defines .text recoil:function:0x44e270: CZObject3D::gwObject3DGetPosition
 * @recoil-match byte
 *
 * Purpose: validate Object3D data and return translation components from
 * the local matrix.
 */
gwObject3DGetPosition(CZNodePartial * node, float* outX, float* outY, float* outZ)
{
    CZObject3DDataPartial* data;

    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x41a, "Null node pointer.");
        return 5;
    }
    if (node->classData == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x41b, "Null class data pointer");
        return 5;
    }
    if (node->classId != kZClassNodeObject3D) {
        ReportOld(
            0x400,
            "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c",
            0x41c,
            "Bad Class Found.\n Wanted (%d)\n Found (%d)",
            node->classId,
            kZClassNodeObject3D
        );
        return 3;
    }
    data = (CZObject3DDataPartial*)(node->classData);

    *outX = data->localMatrix[9];
    *outY = data->localMatrix[10];
    *outZ = data->localMatrix[11];
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dsetposition
 * @recoil-artifact defines .text recoil:function:0x44e300: CZObject3D::gwObject3DSetPosition
 * @recoil-match byte
 *
 * Purpose: validate Object3D data, store local matrix translation, update
 * identity state, and queue transform/bounds propagation.
 */
gwObject3DSetPosition(CZNodePartial * node, float x, float y, float z)
{
    CZObject3DDataPartial* data;

    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x441, "Null node pointer.");
        return 5;
    }
    if (node->classData == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x442, "Null class data pointer");
        return 5;
    }
    data = (CZObject3DDataPartial*)(node->classData);

    data->localMatrix[9] = x;
    data->localMatrix[10] = y;
    data->localMatrix[11] = z;
    if ((data->flags & 0x08) != 0 && (x != 0.0f || y != 0.0f || z != 0.0f)) {
        data->flags &= ~0x08;
    }

    data->flags |= 0x01;
    PropagateTransformDirtyRecursive(node);
    if ((node->flags & 0x01) == 0) {
        CZTypeListInsert(7, node);
        node->flags |= 0x01;
    }
    node->flags |= 0x02;
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dtranslateposition
 * @recoil-artifact defines .text recoil:function:0x44e3d0: CZObject3D::gwObject3DTranslatePosition
 * @recoil-match byte
 *
 * Purpose: validate Object3D data, add local translation deltas, update
 * identity state, and queue transform/bounds propagation.
 */
gwObject3DTranslatePosition(CZNodePartial * node, float dx, float dy, float dz)
{
    CZObject3DDataPartial* data;

    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x47e, "Null node pointer.");
        return 5;
    }
    if (node->classData == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x47f, "Null class data pointer");
        return 5;
    }
    if (node->classId != kZClassNodeObject3D) {
        ReportOld(
            0x400,
            "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c",
            0x480,
            "Bad Class Found.\n Wanted (%d)\n Found (%d)",
            node->classId,
            kZClassNodeObject3D
        );
        return 3;
    }
    data = (CZObject3DDataPartial*)(node->classData);

    data->localMatrix[9] += dx;
    data->localMatrix[10] += dy;
    data->localMatrix[11] += dz;
    if ((data->flags & 0x08) != 0
        && (data->localMatrix[9] != 0.0f || data->localMatrix[10] != 0.0f || data->localMatrix[11] != 0.0f)) {
        data->flags &= ~0x08;
    }

    data->flags |= 0x01;
    PropagateTransformDirtyRecursive(node);
    if ((node->flags & 0x01) == 0) {
        CZTypeListInsert(7, node);
        node->flags |= 0x01;
    }
    node->flags |= 0x02;
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dsetmatrix
 * @recoil-artifact defines .text recoil:function:0x44e4f0: CZObject3D::gwObject3DSetMatrix
 * @recoil-match byte
 *
 * Purpose: validate Object3D data, copy local matrix storage when needed,
 * mark matrix-authored transform state, and enqueue transform propagation.
 */
gwObject3DSetMatrix(CZNodePartial * node, float* matrix)
{
    CZObject3DDataPartial* data;

    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x4bb, "Null node pointer.");
        return 5;
    }
    if (node->classData == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x4bc, "Null class data pointer");
        return 5;
    }
    if (node->classId != kZClassNodeObject3D) {
        ReportOld(
            0x400,
            "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c",
            0x4bd,
            "Bad Class Found.\n Wanted (%d)\n Found (%d)",
            node->classId,
            kZClassNodeObject3D
        );
        return 3;
    }
    data = (CZObject3DDataPartial*)(node->classData);

    if (matrix != data->localMatrix) {
        memcpy(data->localMatrix, matrix, sizeof(data->localMatrix));
    }

    data->flags = (data->flags & ~0x08) | 0x11;
    PropagateTransformDirtyRecursive(node);
    if ((node->flags & 0x01) == 0) {
        CZTypeListInsert(7, node);
        node->flags |= 0x01;
    }
    node->flags |= 0x02;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.object3d.gwobject3dgetmatrixptr
 * @recoil-artifact defines .text recoil:function:0x44e5b0: CZObject3D::gwObject3DGetMatrixPtr
 * @recoil-match byte
 *
 * Purpose: validate Object3D data and return a pointer to the local matrix
 * storage.
 */
float* __fastcall gwObject3DGetMatrixPtr(CZNodePartial* node)
{
    CZObject3DDataPartial* data;

    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x4fe, "Null node pointer.");
        return 0;
    }
    if (node->classData == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c", 0x4ff, "Null class data pointer");
        return 0;
    }
    if (node->classId != kZClassNodeObject3D) {
        ReportOld(
            0x400,
            "D:\\Proj\\GameZRecoil\\zClass\\Object3d.c",
            0x500,
            "Bad Class Found.\n Wanted (%d)\n Found (%d)",
            node->classId,
            kZClassNodeObject3D
        );
        return 0;
    }
    data = (CZObject3DDataPartial*)(node->classData);

    return data->localMatrix;
}

/**
 * Source-shape note: the definition is emitted by cls_util.c; Object3d.c
 * retains related callers and the public declaration.
 */
