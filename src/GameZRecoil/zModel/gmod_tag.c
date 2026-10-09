// zModel compilation unit between gmod_init.c and gmod_draw.c, inferred from the
// retail object boundary [0x476320, 0x476460): the 1998 demos link these
// variant-tag functions apart from their retail neighbours, directly before
// gmod_const.c. The object has no data of its own. Original filename
// unresolved; gmod_tag.c is a provisional name (2026-10-02).

#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zclip_alt.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zGeometry/zgeo.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "recoil/recoil_types.h"
#include "zclass.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-ztag4-clear
 * @recoil-artifact defines .text recoil:function:0x476320: zTag4::Clear
 * @recoil-match byte
 *
 * Purpose: reset a variant tag set to the empty sentinel state.
 */
void __fastcall Clear(zTag4Partial* tag)
{
    int i;
    if (tag == 0) {
        return;
    }

    tag->count = 0;
    for (i = 0; i < 3; ++i) {
        tag->tags[i] = 0xff;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zdi-setvarianttagifunset
 * @recoil-artifact defines .text recoil:function:0x476340: zDi::SetVariantTagIfUnset
 * @recoil-match byte
 *
 * Purpose: assign the variant tag to each display-instance entry that has
 * not already initialized its variant-tag state.
 */
void __fastcall SetVariantTagIfUnset(zDiPartial* self, unsigned char variantTag)
{
    if (self != 0) {
        zDiEntryPartial* entry = self->entries;
        int i;
        for (i = 0; i < self->entryCount; ++i, ++entry) {
            if (entry->variantTagInitialized == 0) {
                entry->variantTag = (unsigned char)variantTag;
                entry->variantTagInitialized = 1;
            }
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-varianttag-tagsoverlap
 * @recoil-artifact defines .text recoil:function:0x476370: VariantTag::TagsOverlap
 * @recoil-match byte
 *
 * Purpose: test whether two variant tag sets pass the active filter.
 */
int __fastcall TagsOverlap(const zTag4Partial* tagA, const zTag4Partial* tagB)
{
    int indexA;
    if (g_Variant_FilterEnabled == 0 || tagA->count == 0 || tagB->count == 0) {
        return 1;
    }

    for (indexA = 0; indexA < tagA->count; ++indexA) {
        const unsigned char tagIdA = tagA->tags[indexA];
        int indexB;
        if (tagIdA == 0xff) {
            return 1;
        }

        for (indexB = 0; indexB < tagB->count; ++indexB) {
            if (tagB->tags[indexB] == 0xff || tagIdA == tagB->tags[indexB]) {
                return 1;
            }
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-varianttag-currentallowsid
 * @recoil-artifact defines .text recoil:function:0x476400: VariantTag::CurrentAllowsId
 * @recoil-match byte
 *
 * Purpose: Tests whether one variant ID is accepted by the active tag filter.
 */
int __fastcall CurrentAllowsId(int variantId)
{
    zTag4Partial currentTag;
    unsigned char count;
    unsigned char id;
    int i;
    if (g_Variant_FilterEnabled == 0) {
        return 1;
    }
    if (variantId == 0xff) {
        return 1;
    }

    // The complete snapshot preserves retail's word-sized count load.
    currentTag = g_Variant_CurrentTag;
    count = currentTag.count;
    if (count == 0) {
        return 1;
    }
    id = (unsigned char)(variantId);
    for (i = 0; i < count; ++i) {
        const unsigned char tag = g_Variant_CurrentTag.tags[i];
        if (tag == 0xff || id == tag) {
            return 1;
        }
    }

    return 0;
}
