#include "GameZRecoil/include/zClass.h"

#include "GameZRecoil/zVideo/zVideo.h"

#include <stdio.h>

namespace zClass_Camera {
RECOIL_NOINLINE int RECOIL_FASTCALL RenderScene(zClass_NodePartial *camera,
                                                         int updateFxPass3Local);
}

RECOIL_NOINLINE int RECOIL_FASTCALL
zVideo_sw_RenderFrame(zClass_NodePartial *camera, int updateFxPass3Local);

namespace zClass_List {

// Reimplements 0x44f630: zClass_List::RenderActiveCameras (GameZRecoil/zClass/List.c)
RECOIL_NOINLINE int RECOIL_CDECL RenderActiveCameras()
{
    zClass_TypeListLink *link = zClass_TypeList::GetBucketHead(8);
    if (link == 0) {
        fprintf(stderr, "ERROR: No camera on camera list.\n");
        return 1;
    }

    do {
        zClass_NodePartial *const camera = link->node;
        zClass_TypeListLink *const next = link->next;

        if ((camera->flags & 4) != 0) {
            if (g_zVideo_ActiveRendererPath != 0) {
                zVideo_sw_RenderFrame(camera, 0);
            } else {
                zClass_Camera::RenderScene(camera, 0);
            }
        }

        link = next;
    } while (link != 0);

    return 0;
}

} // namespace zClass_List
