#include "recoil/Mfc42Abi.h"

#include "GameZRecoil/zRender/zrndr.h"

#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zclass.h"

#include <malloc.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-zrndr-activepaletteremapkey
 * @recoil-artifact defines .data recoil:data:0x4e21fc: g_zRndr_ActivePaletteRemapKey.
 * BN xrefs: zRndr palette setter and remap selection paths read/write this
 * packed remap key; retail initializes it to the disabled value -1.
 * Purpose: active palette-remap key selected for software renderer spans.
 */
int g_zRndr_ActivePaletteRemapKey = -1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-zrndr-activepaletteshaderecipeindex
 * @recoil-artifact defines .data recoil:data:0x4e2200: g_zRndr_ActivePaletteShadeRecipeIndex.
 * BN xrefs: zRndr palette setter and remap selection paths read/write this
 * shade recipe index; retail initializes it to the disabled value -1.
 * Purpose: active palette shade recipe selected for remapped spans.
 */
int g_zRndr_ActivePaletteShadeRecipeIndex = -1;

namespace zRndr
{
    /**
     * BN keeps queued texture alpha setup in this initialized slot at 0x4e21ec as
     * pointer value 0x00000007; the queued and fan-triangle paths overwrite it from
     * zVidImagePartial::queuedAlphaMap before queued-alpha use.
     */
    enum { kQueuedTexAlphaMapStartupSentinel = 7 };
    char* g_spanQueuedTexAlphaMap = (char*)(kQueuedTexAlphaMapStartupSentinel);
    int g_spanActiveTexShift = 7;
    int g_spanActiveTexVMask = 0x07f00000;
    int g_spanActiveTexUMask = 0x7f;
    // BN names this BSS slot gRndr_ActiveTexPixels. Word span loops load it as
    // 16-bit texture pixels, while palettized span loops use the same buffer as
    // 8-bit texture indices.
    unsigned char* g_spanActiveTexPixels = 0;
    unsigned short* g_spanActiveTexPalette = 0;
    int g_spanActiveTexUStepFixed20 = 0;
    int g_spanActiveTexVStepFixed20 = 0;
    // BN names this BSS pointer gRndr_CurrentSpanBaseAddr. Span leaves use it as an
    // ordinary unsigned-short destination cursor; the switch-vshift leaves that
    // also use gRndr_SavedEspSlot are separate ESP-pivot source-shape debt.
    unsigned short* g_spanCurrentSpanBaseAddr = 0;
    int g_spanActiveShadeFixed16 = 0;
    int g_spanActiveShadeStepFixed16 = 0;
    // BN names 0x56b27c gRndr_ActiveTexAlphaMap. Alpha-map span leaves including
    // 0x49c360, 0x49c560, 0x49d1a0, and 0x49d3b0 sample it in lockstep with
    // gRndr_ActiveTexPixels using the same U/V masks and fixed-point steps.
    char* g_spanActiveTexAlphaMap = 0;
    // Queued polygon banks from zrndr_draw.c. BN identifies the transparent count
    // at 0x57de7c, the transparent records at 0x57de80, the sort index bank at
    // 0x5cacf8, the overwrite count at 0x5cb270, and overwrite records at
    // 0x5cb274.
    TransparentQueuedPolyDrawCmd g_transparentQueue[0x15e] = { 0 };
    OverwriteQueuedPolyDrawCmd g_overwriteQueue[0x15e] = { 0 };
    int g_transparentQueueSortIndices[0x15e] = { 0 };
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-transparentqueuecount
     * @recoil-artifact defines .data recoil:data:0x57de7c: g_transparentQueueCount.
     * Purpose: Track the number of queued transparent software polygon draw commands.
     */
    int g_transparentQueueCount = 0;
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-overwritequeuecount
     * @recoil-artifact defines .data recoil:data:0x5cb270: g_overwriteQueueCount.
     * Purpose: Track the number of queued overwrite software polygon draw commands.
     */
    int g_overwriteQueueCount = 0;
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-lensflaresamplequeuecount
     * @recoil-artifact defines .data recoil:data:0x62ea00: g_lensFlareSampleQueueCount.
     * zRndr lens-flare frame-state bank. BN identifies the zero-initialized queue count at
     * 0x62ea00, the 0x28a-entry sample queue at 0x62ea04, the visible count at 0x631ccc, the
     * 64-entry visible pointer list at 0x631cd0, the visibility-active flag at 0x56b248, and four
     * stage texture pointers at 0x56b250.
     * Purpose: Track the number of queued projected lens-flare samples for the frame.
     */
    int g_lensFlareSampleQueueCount = 0;
    LensFlareSamplePartial g_lensFlareSampleQueue[0x28a] = { 0 };
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-lensflarevisiblesamplecount
     * @recoil-artifact defines .data recoil:data:0x631ccc: g_lensFlareVisibleSampleCount.
     * Purpose: Track the number of lens-flare samples accepted into the visible-sample list.
     */
    int g_lensFlareVisibleSampleCount = 0;
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-g-lensflarevisibilityactive
     * @recoil-artifact defines .data recoil:data:0x56b248: g_lensFlareVisibilityActive.
     * Purpose: Record whether all lens-flare visibility stage textures are ready for drawing.
     */
    int g_lensFlareVisibilityActive = 0;
    zImage_TexDirEntryPartial* g_lensFlareVisibleSampleStages[4] = { 0 };
    zRndr_LensFlareVisibleSampleDef* g_lensFlareVisibleSampleDefs[0x40] = { 0 };

    RECOIL_STATIC_ASSERT(sizeof(FogParamsPartial) == 0xa0);
    RECOIL_STATIC_ASSERT(offsetof(FogParamsPartial, packedColorRed) == 0x0c);
    RECOIL_STATIC_ASSERT(offsetof(FogParamsPartial, packedColorGreen) == 0x10);
    RECOIL_STATIC_ASSERT(offsetof(FogParamsPartial, packedColorBlue) == 0x14);
    RECOIL_STATIC_ASSERT(offsetof(FogParamsPartial, packedColor16) == 0x18);
    RECOIL_STATIC_ASSERT(offsetof(FogParamsPartial, packedColor16Padding) == 0x1a);
    RECOIL_STATIC_ASSERT(offsetof(FogParamsPartial, packedColor16Dup) == 0x1c);
    RECOIL_STATIC_ASSERT(offsetof(FogParamsPartial, packedColorRamp) == 0x20);
    RECOIL_STATIC_ASSERT(sizeof(SpanOccluderPolyPartial) == 0x64);
    RECOIL_STATIC_ASSERT(sizeof(SpanNodePartial) == 0x18);
    RECOIL_STATIC_ASSERT(sizeof(LensFlareSamplePartial) == 0x14);
    RECOIL_STATIC_ASSERT(offsetof(LensFlareSamplePartial, x) == 0x00);
    RECOIL_STATIC_ASSERT(offsetof(LensFlareSamplePartial, y) == 0x04);
    RECOIL_STATIC_ASSERT(offsetof(LensFlareSamplePartial, reciprocalZ) == 0x08);
    RECOIL_STATIC_ASSERT(offsetof(LensFlareSamplePartial, packedColor16) == 0x0c);
    RECOIL_STATIC_ASSERT(offsetof(LensFlareSamplePartial, lensFlareSource) == 0x10);
    RECOIL_STATIC_ASSERT(offsetof(zRndr_LensFlareSource, lensFlareEnabled) == 0x0c);
    RECOIL_STATIC_ASSERT(offsetof(zRndr_LensFlareSource, fadeNear) == 0x14);
    RECOIL_STATIC_ASSERT(offsetof(zRndr_LensFlareSource, fadeFar) == 0x18);
    RECOIL_STATIC_ASSERT(offsetof(zRndr_LensFlareVisibleSampleDef, depthDivisor) == 0x08);
    RECOIL_STATIC_ASSERT(offsetof(zRndr_LensFlareVisibleSampleDef, lensFlareSource) == 0x10);
    RECOIL_STATIC_ASSERT(sizeof(QueuedVec3) == 0x0c);
    RECOIL_STATIC_ASSERT(sizeof(QueuedPolyClipOverlay) == 0x324);
    RECOIL_STATIC_ASSERT(offsetof(QueuedPolyClipOverlay, clippedTriVerts) == 0x300);
    RECOIL_STATIC_ASSERT(sizeof(TransparentQueuedPolyDrawCmd) == 0x384);
    RECOIL_STATIC_ASSERT(offsetof(TransparentQueuedPolyDrawCmd, materialRef) == 0x00);
    RECOIL_STATIC_ASSERT(offsetof(TransparentQueuedPolyDrawCmd, vertexCount) == 0x04);
    RECOIL_STATIC_ASSERT(offsetof(TransparentQueuedPolyDrawCmd, polyVerts) == 0x08);
    RECOIL_STATIC_ASSERT(offsetof(TransparentQueuedPolyDrawCmd, triVerts) == 0x32c);
    RECOIL_STATIC_ASSERT(offsetof(TransparentQueuedPolyDrawCmd, clippedTriVertOverlay) == 0x08);
    RECOIL_STATIC_ASSERT(offsetof(TransparentQueuedPolyDrawCmd, triUVs) == 0x350);
    RECOIL_STATIC_ASSERT(offsetof(TransparentQueuedPolyDrawCmd, scanConvertMode) == 0x368);
    RECOIL_STATIC_ASSERT(offsetof(TransparentQueuedPolyDrawCmd, hasClippedTriVerts) == 0x36c);
    RECOIL_STATIC_ASSERT(offsetof(TransparentQueuedPolyDrawCmd, savedInvDepthBias) == 0x370);
    RECOIL_STATIC_ASSERT(offsetof(TransparentQueuedPolyDrawCmd, savedInvDepthScale) == 0x374);
    RECOIL_STATIC_ASSERT(offsetof(TransparentQueuedPolyDrawCmd, alphaOrShadeBits) == 0x378);
    RECOIL_STATIC_ASSERT(offsetof(TransparentQueuedPolyDrawCmd, shadeOrSpanMode) == 0x37c);
    RECOIL_STATIC_ASSERT(offsetof(TransparentQueuedPolyDrawCmd, texKey) == 0x380);
    RECOIL_STATIC_ASSERT(sizeof(OverwriteQueuedPolyDrawCmd) == 0x48c);
    RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, commandTag) == 0x00);
    RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, polyVerts) == 0x04);
    RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, clippedTriVertOverlay) == 0x04);
    RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, triVerts) == 0x328);
    RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, alphaOrShadeF) == 0x34c);
    RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, shadeOrSpanMode) == 0x350);
    RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, vertexCount) == 0x354);
    RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, triUVs) == 0x358);
    RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, materialRef) == 0x370);
    RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, perVertexAlphaOrShadeF) == 0x374);
    RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, scanConvertMode) == 0x478);
    RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, hasClippedTriVerts) == 0x47c);
    RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, savedInvDepthBias) == 0x480);
    RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, savedInvDepthScale) == 0x484);
    RECOIL_STATIC_ASSERT(offsetof(OverwriteQueuedPolyDrawCmd, texKey) == 0x488);
    RECOIL_STATIC_ASSERT(offsetof(LensFlareSamplePartial, packedColor16) == 0x0c);
    RECOIL_STATIC_ASSERT(offsetof(LensFlareSamplePartial, lensFlareSource) == 0x10);
    RECOIL_STATIC_ASSERT(offsetof(SpanNodePartial, next) == 0x00);
    RECOIL_STATIC_ASSERT(offsetof(SpanNodePartial, sampleXMin) == 0x04);
    RECOIL_STATIC_ASSERT(offsetof(SpanNodePartial, sampleXMax) == 0x08);
    RECOIL_STATIC_ASSERT(offsetof(SpanNodePartial, invDepth) == 0x0c);
    RECOIL_STATIC_ASSERT(offsetof(SpanNodePartial, invDepthStep) == 0x10);
    RECOIL_STATIC_ASSERT(offsetof(SpanNodePartial, depthSlope) == 0x14);
} // namespace zRndr

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-setpaletteremapkey
 * @recoil-artifact defines .text recoil:function:0x499930: zRndrSetPaletteRemapKey.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Span.cpp.
 * Source file evidence: Binary Ninja function source comment.
 * Purpose: Select the active palette remap key from a recipe and shade level.
 */
void __fastcall zRndrSetPaletteRemapKey(zVidPaletteRemapRecipe* recipe, float shadeLevel)
{
    if (recipe == 0) {
        g_zRndr_ActivePaletteRemapKey = -1;
        return;
    }

    const int recipeIndex = zVidPaletteRemapBuildPaletteVariant(recipe);
    int shadeBucket = (int)(shadeLevel * 0.125f);
    if (shadeBucket > 31) {
        shadeBucket = 31;
    } else if (shadeBucket < 0) {
        shadeBucket = 0;
    }

    g_zRndr_ActivePaletteRemapKey = (recipeIndex << 5) + shadeBucket;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-setpaletteremapkeyfromrgb01
 * @recoil-artifact defines .text recoil:function:0x499990: zRndrSetPaletteRemapKeyFromRgb01.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Span.cpp.
 * Source file evidence: Binary Ninja function source comment.
 * Purpose: Build a single-color palette remap recipe from RGB values and select its remap key.
 */
void __fastcall zRndrSetPaletteRemapKeyFromRgb01(zColorRgb* rgb01, float shadeLevel)
{
    if (rgb01 == 0) {
        g_zRndr_ActivePaletteRemapKey = -1;
        return;
    }

    zVidPaletteRemapRecipe recipe;
    recipe.color0.red = 0.0f;
    recipe.color0.green = 0.0f;
    recipe.color0.blue = 0.0f;
    recipe.color0Strength = 0.0f;
    recipe.color1.red = rgb01->red;
    recipe.color1.green = rgb01->green;
    recipe.color1.blue = rgb01->blue;
    recipe.color1Strength = 1.0f;
    zRndrSetPaletteRemapKey(&recipe, shadeLevel);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-setpaletteshaderecipeindex
 * @recoil-artifact defines .text recoil:function:0x499a00: zRndrSetPaletteShadeRecipeIndex.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_Span.cpp.
 * Source file evidence: Binary Ninja function source comment.
 * Purpose: Select the active palette shade recipe variant index.
 */
void __fastcall zRndrSetPaletteShadeRecipeIndex(zVidPaletteRemapRecipe* recipe)
{
    if (recipe == 0) {
        g_zRndr_ActivePaletteShadeRecipeIndex = -1;
        return;
    }

    g_zRndr_ActivePaletteShadeRecipeIndex = zVidPaletteRemapBuildPaletteVariant(recipe);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-submitpolywithspanlist
 * @recoil-artifact defines .text recoil:function:0x499a20: zRndrSubmitPolyWithSpanList
 *
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zRender\zrndr_draw.c.
 * Source file evidence: embedded zError file path in this function.
 * Purpose: Submit a flat polygon for immediate drawing or deferred transparent/overwrite queues.
 */
void __fastcall zRndrSubmitPolyWithSpanList(
    zVec3* entryVertices,
    zVec3* entryPlaneVertices,
    int spanOpContext,
    int alpha255,
    int vertCount,
    int queueOverwrite
)
{
    const int kMaxQueuedPolys = 0x15e;
    const char* kSourceFile = "D:\\Proj\\GameZRecoil\\zRender\\zrndr_draw.c";

    if (queueOverwrite != 0) {
        const int queueIndex = zRndr::g_overwriteQueueCount;
        if (queueIndex >= kMaxQueuedPolys) {
            zError::ReportOld(0x400, kSourceFile, 0x9c, " Not enough MAX_OVERWRITE_POLYS: need %d\n", queueIndex);
            return;
        }

        zRndr::OverwriteQueuedPolyDrawCmd& cmd = zRndr::g_overwriteQueue[queueIndex];
        zRndr::g_overwriteQueueCount = queueIndex + 1;
        cmd.commandTag = 0;
        memcpy(cmd.polyVerts, entryVertices, (size_t)(vertCount) * sizeof(zVec3));
        memcpy(cmd.triVerts, entryPlaneVertices, 3 * sizeof(zVec3));
        cmd.alphaOrShadeF = (float)(alpha255);
        cmd.materialRef = 0;
        cmd.vertexCount = vertCount;
        cmd.shadeOrSpanMode = spanOpContext;
        cmd.scanConvertMode = zRndr::g_scanConvertMode;
        cmd.savedInvDepthBias = zRndr::g_inverseDepthBias;
        cmd.savedInvDepthScale = zRndr::g_inverseDepthScale;
        return;
    }

    if (alpha255 >= 0xff) {
        zRndrRasterizePolyWithSpanList(entryVertices, entryPlaneVertices, vertCount, spanOpContext);
        return;
    }

    const int queueIndex = zRndr::g_transparentQueueCount;
    if (queueIndex >= kMaxQueuedPolys) {
        zError::ReportOld(0x400, kSourceFile, 0xb9, " Not enough MAX_TRANSPARENT_POLYS: need %d\n", queueIndex);
        return;
    }

    zRndr::TransparentQueuedPolyDrawCmd& cmd = zRndr::g_transparentQueue[queueIndex];
    memcpy(cmd.polyVerts, entryVertices, (size_t)(vertCount) * sizeof(zVec3));
    memcpy(cmd.triVerts, entryPlaneVertices, 3 * sizeof(zVec3));
    cmd.materialRef = 0;
    cmd.vertexCount = vertCount;
    cmd.shadeOrSpanMode = spanOpContext;
    cmd.alphaOrShadeBits = alpha255;
    cmd.scanConvertMode = zRndr::g_scanConvertMode;
    cmd.savedInvDepthBias = zRndr::g_inverseDepthBias;
    cmd.savedInvDepthScale = zRndr::g_inverseDepthScale;
    ++zRndr::g_transparentQueueCount;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-submittexturedpolyuniformalphaorshade
 * @recoil-artifact defines .text recoil:function:0x499c40: zRndrSubmitTexturedPolyUniformAlphaOrShade
 *
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zRender\zrndr_draw.c.
 * Source file evidence: embedded zError file path in this function.
 * Purpose: Submit a textured polygon with one alpha/shade value to the immediate or queued paths.
 */
void __fastcall zRndrSubmitTexturedPolyUniformAlphaOrShade(
    zVec3* projectedPolyVerts,
    zVec3* clippedTriVerts,
    zVec3* triData9f,
    zVec2* triUVs,
    int vertexCount,
    zImage_TexDirEntryPartial* entry,
    float alphaOrShadeF,
    int queueOverwrite
)
{
    const int kMaxQueuedPolys = 0x15e;
    const char* kSourceFile = "D:\\Proj\\GameZRecoil\\zRender\\zrndr_draw.c";

    if (queueOverwrite != 0) {
        const int queueIndex = zRndr::g_overwriteQueueCount;
        if (queueIndex >= kMaxQueuedPolys) {
            zError::ReportOld(0x400, kSourceFile, 0xfa, " Not enough MAX_OVERWRITE_POLYS: need %d\n", queueIndex);
            return;
        }

        zRndr::OverwriteQueuedPolyDrawCmd& cmd = zRndr::g_overwriteQueue[queueIndex];
        zRndr::g_overwriteQueueCount = queueIndex + 1;
        cmd.commandTag = 1;
        cmd.vertexCount = vertexCount;
        cmd.materialRef = entry;
        cmd.savedInvDepthBias = zRndr::g_inverseDepthBias;
        cmd.savedInvDepthScale = zRndr::g_inverseDepthScale;
        cmd.scanConvertMode = zRndr::g_scanConvertMode;
        cmd.alphaOrShadeF = alphaOrShadeF;
        memcpy(cmd.polyVerts, projectedPolyVerts, (size_t)(vertexCount) * sizeof(zVec3));
        memcpy(cmd.triVerts, triData9f, 3 * sizeof(zVec3));
        memcpy(cmd.triUVs, triUVs, 3 * sizeof(zVec2));
        if (clippedTriVerts != 0) {
            memcpy(cmd.clippedTriVertOverlay.clippedTriVerts, clippedTriVerts, 3 * sizeof(zVec3));
            cmd.hasClippedTriVerts = 1;
        } else {
            cmd.hasClippedTriVerts = 0;
        }
        cmd.texKey = g_zRndr_ActivePaletteRemapKey;
        return;
    }

    zVidImagePartial* image = entry != 0 ? entry->image : 0;
    if ((image->formatFlagsPacked & 2) == 0 && alphaOrShadeF >= 1.0f) {
        zRndrDrawTexturedQueuedAlpha(
            entry,
            projectedPolyVerts,
            clippedTriVerts,
            triData9f,
            triUVs,
            vertexCount,
            g_zRndr_ActivePaletteRemapKey
        );
        return;
    }

    const int queueIndex = zRndr::g_transparentQueueCount;
    if (queueIndex >= kMaxQueuedPolys) {
        zError::ReportOld(0x400, kSourceFile, 0x126, " Not enough MAX_TRANSPARENT_POLYS: need %d\n", queueIndex);
        return;
    }

    zRndr::TransparentQueuedPolyDrawCmd& cmd = zRndr::g_transparentQueue[queueIndex];
    cmd.vertexCount = vertexCount;
    cmd.materialRef = entry;
    cmd.savedInvDepthBias = zRndr::g_inverseDepthBias;
    cmd.savedInvDepthScale = zRndr::g_inverseDepthScale;
    cmd.scanConvertMode = zRndr::g_scanConvertMode;
    if ((image->formatFlagsPacked & 2) != 0) {
        memcpy(&cmd.alphaOrShadeBits, &alphaOrShadeF, sizeof(float));
    } else {
        cmd.alphaOrShadeBits = (int)(alphaOrShadeF * 255.0f);
    }
    memcpy(cmd.polyVerts, projectedPolyVerts, (size_t)(vertexCount) * sizeof(zVec3));
    memcpy(cmd.triVerts, triData9f, 3 * sizeof(zVec3));
    memcpy(cmd.triUVs, triUVs, 3 * sizeof(zVec2));
    if (clippedTriVerts != 0) {
        memcpy(cmd.clippedTriVertOverlay.clippedTriVerts, clippedTriVerts, 3 * sizeof(zVec3));
        cmd.hasClippedTriVerts = 1;
    } else {
        cmd.hasClippedTriVerts = 0;
    }
    cmd.texKey = g_zRndr_ActivePaletteRemapKey;
    ++zRndr::g_transparentQueueCount;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-submittexturedpolypervertexalphaorshade
 * @recoil-artifact defines .text recoil:function:0x499ec0: zRndrSubmitTexturedPolyPerVertexAlphaOrShade
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zRender\zrndr_draw.c.
 * Source file evidence: embedded zError file path in this function.
 * Purpose: Submit a textured polygon with per-vertex alpha/shade values to draw or queue paths.
 */
void __fastcall zRndrSubmitTexturedPolyPerVertexAlphaOrShade(
    zVec3* projectedPolyVerts,
    zVec3* clippedTriVerts,
    zVec3* triData9f,
    zVec2* triUVs,
    float* perVertexAlphaOrShadeF,
    int shadeOrSpanMode,
    int vertexCount,
    zImage_TexDirEntryPartial* entry,
    int preservePaletteRemapKey,
    int queueOverwrite
)
{
    const int kMaxQueuedPolys = 0x15e;
    const char* kSourceFile = "D:\\Proj\\GameZRecoil\\zRender\\zrndr_draw.c";
    zVec3 fanVerts[64];
    float fanShade[64];

    int usingDerivedPaletteKey = 0;
    zVidImagePartial* image = entry != 0 ? entry->image : 0;
    int texKey = g_zRndr_ActivePaletteRemapKey;

    if (texKey == -1 && preservePaletteRemapKey == 0 && entry->image->paletteMetaPacked > 0) {
        texKey = zVidPaletteRemapFindRecipeIndexFromRgb((zColorRgb*)(zRndr::g_fogParamsActive.colorRgb01));
        if (texKey >= 0) {
            int shadeBucket = (int)(perVertexAlphaOrShadeF[0] * 0.125f);
            if (shadeBucket > 0x1f) {
                shadeBucket = 0x1f;
            } else if (shadeBucket < 0) {
                shadeBucket = 0;
            }
            texKey = (texKey << 5) + shadeBucket;

            if ((image->formatFlagsPacked == 0) & 2) {
                if (queueOverwrite != 0) {
                    usingDerivedPaletteKey = 1;
                } else {
                    zRndrDrawTexturedQueuedAlpha(
                        entry,
                        projectedPolyVerts,
                        clippedTriVerts,
                        triData9f,
                        triUVs,
                        vertexCount,
                        texKey
                    );
                    return;
                }
            }
        }
    }

    if (queueOverwrite != 0) {
        const int queueIndex = zRndr::g_overwriteQueueCount;
        if (queueIndex >= kMaxQueuedPolys) {
            zError::ReportOld(0x400, kSourceFile, 0x19e, " Not enough MAX_OVERWRITE_POLYS: need %d\n", queueIndex);
            return;
        }

        zRndr::OverwriteQueuedPolyDrawCmd& cmd = zRndr::g_overwriteQueue[queueIndex];
        zRndr::g_overwriteQueueCount = queueIndex + 1;
        cmd.commandTag = usingDerivedPaletteKey != 0 ? 1 : 2;
        cmd.vertexCount = vertexCount;
        cmd.materialRef = entry;
        cmd.savedInvDepthBias = zRndr::g_inverseDepthBias;
        cmd.savedInvDepthScale = zRndr::g_inverseDepthScale;
        cmd.scanConvertMode = zRndr::g_scanConvertMode;
        cmd.alphaOrShadeF = (float)(shadeOrSpanMode);
        memcpy(cmd.polyVerts, projectedPolyVerts, (size_t)(vertexCount) * sizeof(zVec3));
        memcpy(cmd.triVerts, triData9f, 3 * sizeof(zVec3));
        memcpy(cmd.triUVs, triUVs, 3 * sizeof(zVec2));
        if (usingDerivedPaletteKey == 0) {
            memcpy(cmd.perVertexAlphaOrShadeF, perVertexAlphaOrShadeF, (size_t)(vertexCount) * sizeof(float));
        }
        if (clippedTriVerts != 0) {
            memcpy(cmd.clippedTriVertOverlay.clippedTriVerts, clippedTriVerts, 3 * sizeof(zVec3));
            cmd.hasClippedTriVerts = 1;
        } else {
            cmd.hasClippedTriVerts = 0;
        }
        cmd.texKey = texKey;
        return;
    }

    fanVerts[0] = projectedPolyVerts[0];
    fanShade[0] = perVertexAlphaOrShadeF[0];
    if ((image->formatFlagsPacked & 2) == 0) {
        for (int fanTriIndex = 0; fanTriIndex < vertexCount - 2; ++fanTriIndex) {
            fanVerts[1] = projectedPolyVerts[fanTriIndex + 1];
            fanVerts[2] = projectedPolyVerts[fanTriIndex + 2];
            fanShade[1] = perVertexAlphaOrShadeF[fanTriIndex + 1];
            fanShade[2] = perVertexAlphaOrShadeF[fanTriIndex + 2];
            zRndrDrawTexturedQueued(
                entry,
                fanVerts,
                clippedTriVerts,
                triData9f,
                triUVs,
                (zVec3*)(fanShade),
                3,
                fanTriIndex,
                texKey
            );
        }
        return;
    }

    const int queueIndex = zRndr::g_transparentQueueCount;
    if (queueIndex >= kMaxQueuedPolys) {
        zError::ReportOld(0x400, kSourceFile, 0x1e1, " Not enough MAX_TRANSPARENT_POLYS: need %d\n", queueIndex);
        return;
    }

    zRndr::TransparentQueuedPolyDrawCmd& cmd = zRndr::g_transparentQueue[queueIndex];
    cmd.vertexCount = vertexCount;
    cmd.materialRef = entry;
    cmd.savedInvDepthBias = zRndr::g_inverseDepthBias;
    cmd.savedInvDepthScale = zRndr::g_inverseDepthScale;
    cmd.scanConvertMode = zRndr::g_scanConvertMode;
    cmd.alphaOrShadeBits = 0xff;
    memcpy(cmd.polyVerts, projectedPolyVerts, (size_t)(vertexCount) * sizeof(zVec3));
    memcpy(cmd.triVerts, triData9f, 3 * sizeof(zVec3));
    memcpy(cmd.triUVs, triUVs, 3 * sizeof(zVec2));
    cmd.texKey = texKey;
    ++zRndr::g_transparentQueueCount;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-flushtransparentqueue
 * @recoil-artifact defines .text recoil:function:0x49a2b0: zRndrFlushTransparentQueue
 *
 *
 * Source file evidence: zRndr queued draw cluster in this source file.
 * Purpose: Sort and draw queued transparent polygons, then reset the transparent queue.
 */
void __cdecl zRndrFlushTransparentQueue()
{
    {
        for (int i = 0; i < zRndr::g_transparentQueueCount; ++i) {
            zRndr::g_transparentQueueSortIndices[i] = zRndr::g_transparentQueueCount - i - 1;
        }
    }

    bool swapped = false;
    do {
        {
            for (int i = 0; i < zRndr::g_transparentQueueCount - 1; ++i) {
                const int lhsIndex = zRndr::g_transparentQueueSortIndices[i];
                const int rhsIndex = zRndr::g_transparentQueueSortIndices[i + 1];
                if (zRndr::g_transparentQueue[rhsIndex].triVerts[0].z
                    < zRndr::g_transparentQueue[lhsIndex].triVerts[0].z) {
                    zRndr::g_transparentQueueSortIndices[i] = rhsIndex;
                    zRndr::g_transparentQueueSortIndices[i + 1] = lhsIndex;
                    swapped = true;
                }
            }
        }
    } while (swapped);

    {
        for (int i = 0; i < zRndr::g_transparentQueueCount; ++i) {
            const int queueIndex = zRndr::g_transparentQueueSortIndices[i];
            zRndr::TransparentQueuedPolyDrawCmd& cmd = zRndr::g_transparentQueue[queueIndex];

            zRndr::g_inverseDepthBias = cmd.savedInvDepthBias;
            zRndr::g_inverseDepthScale = cmd.savedInvDepthScale;
            zRndr::g_scanConvertMode = cmd.scanConvertMode;

            if (cmd.materialRef != 0) {
                zVec3* clippedTriVerts
                    = cmd.hasClippedTriVerts != 0 ? (zVec3*)(cmd.clippedTriVertOverlay.clippedTriVerts) : 0;
                zVec3* polyVerts = (zVec3*)(cmd.polyVerts);
                zVec3* triVerts = (zVec3*)(cmd.triVerts);
                zVec2* triUVs = (zVec2*)(cmd.triUVs);

                if ((cmd.materialRef->image->formatFlagsPacked & 2) != 0) {
                    float alpha = 0.0f;
                    memcpy(&alpha, &cmd.alphaOrShadeBits, sizeof(float));
                    if (alpha >= 1.0f) {
                        zRndrDrawFlatQueued(cmd.materialRef, polyVerts, triVerts, triUVs, cmd.vertexCount, cmd.texKey);
                    } else {
                        RendererDrawPolyTLV(
                            cmd.materialRef,
                            polyVerts,
                            triVerts,
                            triUVs,
                            cmd.vertexCount,
                            alpha,
                            cmd.texKey
                        );
                    }
                } else {
                    zRndrDrawTexturedFanTri(
                        cmd.materialRef,
                        polyVerts,
                        clippedTriVerts,
                        triVerts,
                        triUVs,
                        cmd.vertexCount,
                        cmd.alphaOrShadeBits,
                        cmd.texKey
                    );
                }
            } else {
                zRndrDrawFlatImmediate(
                    (zVec3*)(cmd.polyVerts),
                    (zVec3*)(cmd.triVerts),
                    cmd.vertexCount,
                    cmd.alphaOrShadeBits,
                    cmd.shadeOrSpanMode
                );
            }
        }
    }

    zRndr::g_transparentQueueCount = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-flushoverwritequeue
 * @recoil-artifact defines .text recoil:function:0x49a490: zRndrFlushOverwriteQueue
 *
 *
 * Source file evidence: zRndr queued draw cluster in this source file.
 * Purpose: Draw queued overwrite polygons through the appropriate flat or textured paths.
 */
void __cdecl zRndrFlushOverwriteQueue()
{
    zVec3 fanVerts[64];
    float fanShade[64];

    zRndr::g_pfnBuildSpanList = zRndrSpanOcclusionInsertSpanNodeNoDepthTest;
    zRndr::g_pfnBuildSpanListSecondary = zRndrSpanOcclusionBuildSpanListFast;

    for (int queueIndex = 0; queueIndex < zRndr::g_overwriteQueueCount; ++queueIndex) {
        zRndr::OverwriteQueuedPolyDrawCmd& cmd = zRndr::g_overwriteQueue[queueIndex];
        zRndr::g_inverseDepthBias = cmd.savedInvDepthBias;
        zRndr::g_inverseDepthScale = cmd.savedInvDepthScale;
        zRndr::g_scanConvertMode = cmd.scanConvertMode;

        int useFallback = 0;
        zVidImagePartial* image = cmd.materialRef != 0 ? cmd.materialRef->image : 0;
        switch (cmd.commandTag) {
        case 0:
            if (cmd.alphaOrShadeF >= 255.0f) {
                zRndrRasterizePolyWithSpanList(
                    (zVec3*)(cmd.polyVerts),
                    (zVec3*)(cmd.triVerts),
                    cmd.vertexCount,
                    cmd.shadeOrSpanMode
                );
            } else {
                useFallback = 1;
            }
            break;

        case 1:
            if ((image->formatFlagsPacked & 2) == 0 && cmd.alphaOrShadeF >= 1.0f) {
                zRndrDrawTexturedQueuedAlpha(
                    cmd.materialRef,
                    (zVec3*)(cmd.polyVerts),
                    cmd.hasClippedTriVerts != 0 ? (zVec3*)(cmd.clippedTriVertOverlay.clippedTriVerts) : 0,
                    (zVec3*)(cmd.triVerts),
                    (zVec2*)(cmd.triUVs),
                    cmd.vertexCount,
                    cmd.texKey
                );
                break;
            }
            if ((image->formatFlagsPacked & 2) == 0) {
                cmd.alphaOrShadeF *= 255.0f;
            }
            useFallback = 1;
            break;

        case 2:
            fanVerts[0] = ((zVec3*)(cmd.polyVerts))[0];
            fanShade[0] = cmd.perVertexAlphaOrShadeF[0];
            if ((image->formatFlagsPacked & 2) == 0) {
                for (int fanTriIndex = 0; fanTriIndex < cmd.vertexCount - 2; ++fanTriIndex) {
                    fanVerts[1] = ((zVec3*)(cmd.polyVerts))[fanTriIndex + 1];
                    fanVerts[2] = ((zVec3*)(cmd.polyVerts))[fanTriIndex + 2];
                    fanShade[1] = cmd.perVertexAlphaOrShadeF[fanTriIndex + 1];
                    fanShade[2] = cmd.perVertexAlphaOrShadeF[fanTriIndex + 2];
                    zRndrDrawTexturedQueued(
                        cmd.materialRef,
                        fanVerts,
                        cmd.hasClippedTriVerts != 0 ? (zVec3*)(cmd.clippedTriVertOverlay.clippedTriVerts) : 0,
                        (zVec3*)(cmd.triVerts),
                        (zVec2*)(cmd.triUVs),
                        (zVec3*)(fanShade),
                        3,
                        fanTriIndex,
                        cmd.texKey
                    );
                }
            } else {
                cmd.alphaOrShadeF = 255.0f;
                useFallback = 1;
            }
            break;
        }

        if (useFallback == 0) {
            continue;
        }

        if (image != 0) {
            if ((image->formatFlagsPacked & 2) != 0) {
                if (cmd.alphaOrShadeF >= 1.0f) {
                    zRndrDrawFlatQueued(
                        cmd.materialRef,
                        (zVec3*)(cmd.polyVerts),
                        (zVec3*)(cmd.triVerts),
                        (zVec2*)(cmd.triUVs),
                        cmd.vertexCount,
                        cmd.texKey
                    );
                } else {
                    RendererDrawPolyTLV(
                        cmd.materialRef,
                        (zVec3*)(cmd.polyVerts),
                        (zVec3*)(cmd.triVerts),
                        (zVec2*)(cmd.triUVs),
                        cmd.vertexCount,
                        cmd.alphaOrShadeF,
                        cmd.texKey
                    );
                }
            } else {
                zRndrDrawTexturedFanTri(
                    cmd.materialRef,
                    (zVec3*)(cmd.polyVerts),
                    cmd.hasClippedTriVerts != 0 ? (zVec3*)(cmd.clippedTriVertOverlay.clippedTriVerts) : 0,
                    (zVec3*)(cmd.triVerts),
                    (zVec2*)(cmd.triUVs),
                    cmd.vertexCount,
                    (int)(cmd.alphaOrShadeF),
                    cmd.texKey
                );
            }
        } else {
            zRndrDrawFlatImmediate(
                (zVec3*)(cmd.polyVerts),
                (zVec3*)(cmd.triVerts),
                cmd.vertexCount,
                (int)(cmd.alphaOrShadeF),
                cmd.shadeOrSpanMode
            );
        }
    }

    zRndr::g_overwriteQueueCount = 0;
    zRndr::g_pfnBuildSpanList = zRndr_SpanOcclusion_InsertSpanNode_Local;
    zRndr::g_pfnBuildSpanListSecondary = zRndrSpanOcclusionBuildSpanList;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-lensflare-queueprojectedsample
 * @recoil-artifact defines .text recoil:function:0x49a830: zRndrLensFlareQueueProjectedSample
 *
 *
 * Purpose: Queue a projected lens-flare sample after applying the active inverse-depth transform.
 */
void __fastcall
zRndrLensFlareQueueProjectedSample(zProjectedPoint* projectedPoint, int packedColor16, int lensFlareSource)
{
    if (zRndr::g_lensFlareSampleQueueCount >= 0x28a) {
        return;
    }

    projectedPoint->reciprocalZ = zRndr::g_inverseDepthScale * projectedPoint->reciprocalZ;
    projectedPoint->reciprocalZ = zRndr::g_inverseDepthBias + projectedPoint->reciprocalZ;

    zRndr::LensFlareSamplePartial* sample = &zRndr::g_lensFlareSampleQueue[zRndr::g_lensFlareSampleQueueCount];
    sample->x = projectedPoint->x;
    sample->y = projectedPoint->y;
    sample->reciprocalZ = projectedPoint->reciprocalZ;
    zRndr::g_lensFlareSampleQueue[zRndr::g_lensFlareSampleQueueCount].packedColor16 = packedColor16;
    zRndr::g_lensFlareSampleQueue[zRndr::g_lensFlareSampleQueueCount].lensFlareSource = lensFlareSource;
    ++zRndr::g_lensFlareSampleQueueCount;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-lensflare-getqueuedsamplecount
 * @recoil-artifact defines .text recoil:function:0x49a8b0: zRndrLensFlareGetQueuedSampleCount
 * @recoil-match byte
 *
 * Purpose: Return the number of lens-flare samples queued for the frame.
 */
int __cdecl zRndrLensFlareGetQueuedSampleCount()
{
    return zRndr::g_lensFlareSampleQueueCount;
}

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-lensflare-drawqueuedsamplesscaled16-clippedframebuffer
     * @recoil-artifact defines .text recoil:function:0x49a8c0: zRndr::LensFlareDrawQueuedSamplesScaled16ClippedFramebuffer
     * @recoil-match byte
     *
     * Source file evidence: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
     * Purpose: Draw every queued lens-flare sample with a shared screen scale and Y offset.
     */
    void __fastcall LensFlareDrawQueuedSamplesScaled16ClippedFramebuffer(int yOffsetPixels, float screenScale)
    {
        {
            for (int sampleIndex = 0; sampleIndex < g_lensFlareSampleQueueCount; ++sampleIndex) {
                LensFlareDrawQueuedSample16ClippedFramebuffer(
                    &g_lensFlareSampleQueue[sampleIndex],
                    screenScale,
                    yOffsetPixels
                );
            }
        }

        g_lensFlareSampleQueueCount = 0;
        g_overlayBlendEnabled = 0;
    }
} // namespace zRndr

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-lensflare-resetsamplequeue
     * @recoil-artifact defines .text recoil:function:0x49a910: zRndr::LensFlareResetSampleQueue
     * @recoil-match byte
     *
     * Purpose: Reset the queued lens-flare sample count for the frame.
     */
    void __cdecl LensFlareResetSampleQueue()
    {
        g_lensFlareSampleQueueCount = 0;
    }
} // namespace zRndr

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-lensflare-drawqueuedsamples16-andbuildvisiblelist
 * @recoil-artifact defines .text recoil:function:0x49a920: zRndrLensFlareDrawQueuedSamples16AndBuildVisibleList
 *
 *
 * Source file evidence: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: Cull queued lens-flare samples and build the visible-sample list for 16-bit drawing.
 */
void __fastcall zRndrLensFlareDrawQueuedSamples16AndBuildVisibleList(int startIndex)
{
    if (startIndex >= zRndr::g_lensFlareSampleQueueCount) {
        return;
    }

    int sampleIndex = startIndex;
    for (;;) {
        if (zRndrSpanOcclusionTestPointVisibility((zVec3*)(&zRndr::g_lensFlareSampleQueue[sampleIndex])) != 0) {
            zRndr_LensFlareSource* source
                = (zRndr_LensFlareSource*)(zRndr::g_lensFlareSampleQueue[sampleIndex].lensFlareSource);
            if (source != 0 && source->lensFlareEnabled != 0 && sampleIndex < 0x40
                && zRndr::g_lensFlareSampleQueue[sampleIndex].reciprocalZ != 0.0f) {
                zRndr_LensFlareVisibleSampleDef** const visibleSlot
                    = &zRndr::g_lensFlareVisibleSampleDefs[zRndr::g_lensFlareVisibleSampleCount];
                *visibleSlot = (zRndr_LensFlareVisibleSampleDef*)(&zRndr::g_lensFlareSampleQueue[sampleIndex]);
                ++zRndr::g_lensFlareVisibleSampleCount;
            }

            ++sampleIndex;
            if (sampleIndex >= zRndr::g_lensFlareSampleQueueCount) {
                return;
            }
        } else {
            --zRndr::g_lensFlareSampleQueueCount;
            if (sampleIndex >= zRndr::g_lensFlareSampleQueueCount) {
                return;
            }

            zRndr::g_lensFlareSampleQueue[sampleIndex]
                = zRndr::g_lensFlareSampleQueue[zRndr::g_lensFlareSampleQueueCount];
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-lensflare-buildvisiblesamplelistfromqueue
 * @recoil-artifact defines .text recoil:function:0x49a9c0: zRndr_LensFlare::BuildVisibleSampleListFromQueue
 *
 *
 * Source file evidence: D:\Proj\GameZRecoil\zRndr\zRndr_Draw.cpp.
 * Purpose: Build the visible lens-flare sample list from queued samples without visibility testing.
 */
int __fastcall zRndrLensFlareBuildVisibleSampleListFromQueue(int startIndex)
{
    int visibleSampleCount = 0;
    zRndr::g_lensFlareVisibleSampleCount = 0;
    for (int sampleIndex = startIndex; sampleIndex < zRndr::g_lensFlareSampleQueueCount; ++sampleIndex) {
        if (zRndr::g_lensFlareSampleQueue[sampleIndex].lensFlareSource != 0
            && ((zRndr_LensFlareSource*)(zRndr::g_lensFlareSampleQueue[sampleIndex].lensFlareSource))->lensFlareEnabled
                != 0
            && sampleIndex < 0x40 && zRndr::g_lensFlareSampleQueue[sampleIndex].reciprocalZ != 0.0f) {
            zRndr_LensFlareVisibleSampleDef** const visibleSlot
                = &zRndr::g_lensFlareVisibleSampleDefs[visibleSampleCount];
            *visibleSlot = (zRndr_LensFlareVisibleSampleDef*)(&zRndr::g_lensFlareSampleQueue[sampleIndex]);
            visibleSampleCount = zRndr::g_lensFlareVisibleSampleCount + 1;
            zRndr::g_lensFlareVisibleSampleCount = visibleSampleCount;
        }
    }

    return visibleSampleCount;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-spanocclusion-filtersamplelist
 * @recoil-artifact defines .text recoil:function:0x49aa30: zRndrSpanOcclusionFilterSampleList
 * @recoil-match byte
 *
 * Purpose: Unproject one visible lens-flare sample into an occlusion-test point.
 */
void __fastcall zRndrSpanOcclusionFilterSampleList(int visibleSampleIndex, zVec3* outPoint)
{
    zRndr_LensFlareVisibleSampleDef* sample = zRndr::g_lensFlareVisibleSampleDefs[visibleSampleIndex];
    zMathUnprojectPointBatchZBuf((const zProjectedPoint*)(sample), outPoint, 1);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-lensflare-setvisiblesamplestage
 * @recoil-artifact defines .text recoil:function:0x49aa40: zRndrLensFlareSetVisibleSampleStage
 * @recoil-match byte
 *
 * Purpose: Store one lens-flare stage texture and refresh the visibility-active flag.
 */
void __fastcall zRndrLensFlareSetVisibleSampleStage(int stageIndex, zImage_TexDirEntryPartial* stageTexDirEntry)
{
    if (stageIndex >= 0 && stageIndex < 4) {
        zRndr::g_lensFlareVisibleSampleStages[stageIndex] = stageTexDirEntry;
    }

    if (zRndr::g_lensFlareVisibleSampleStages[0] != 0 && zRndr::g_lensFlareVisibleSampleStages[1] != 0
        && zRndr::g_lensFlareVisibleSampleStages[2] != 0 && zRndr::g_lensFlareVisibleSampleStages[3] != 0) {
        zRndr::g_lensFlareVisibilityActive = 1;
    } else {
        zRndr::g_lensFlareVisibilityActive = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-lensflare-drawsamplestageclipped
 * @recoil-artifact defines .text recoil:function:0x49aa90: zRndrLensFlareDrawSampleStageClipped
 *
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_LensFlare.cpp.
 * Source file evidence: Binary Ninja function source comment.
 * Purpose: Draw one clipped lens-flare stage quad through hardware or software rendering.
 */
void __fastcall zRndrLensFlareDrawSampleStageClipped(
    const zVec2* sampleCenter,
    zImage_TexDirEntryPartial* stageTexDirEntry,
    float sampleRadius,
    const zRndr_LineClipRect2I* clipRect
)
{
    if (sampleRadius < 1.0f) {
        return;
    }

    float left = sampleCenter->x - sampleRadius;
    float top = sampleCenter->y - sampleRadius;
    float right = sampleRadius + sampleCenter->x;
    float bottom = sampleRadius + sampleCenter->y;

    float clipLeft;
    float clipTop;
    float clipRight;
    float clipBottom;
    if (clipRect != 0) {
        clipRight = (float)(clipRect->right);
        if (left > clipRight - 2.0f) {
            return;
        }

        clipBottom = (float)(clipRect->bottom);
        if (top > clipBottom - 2.0f) {
            return;
        }

        clipLeft = (float)(clipRect->left);
        if (right < clipLeft + 1.0f) {
            return;
        }

        clipTop = (float)(clipRect->top);
        if (bottom < clipTop + 1.0f) {
            return;
        }
    } else {
        clipLeft = 0.0f;
        clipTop = 0.0f;
        clipRight = (float)((unsigned int)(zRndr::g_activeRegionWidth));
        clipBottom = (float)((unsigned int)(zRndr::g_activeRegionHeight));
        if (left > clipRight - 2.0f) {
            return;
        }

        if (top > clipBottom - 2.0f) {
            return;
        }

        if (right < 1.0f) {
            return;
        }

        if (bottom < 1.0f) {
            return;
        }
    }

    const float uvScale = 0.5f / sampleRadius;
    float uLeft = 0.0f;
    float uRight = 1.0f;
    float vTop = 1.0f;
    float vBottom = 0.0f;

    if (left < clipLeft) {
        uLeft = (clipLeft - left) * uvScale;
        left = clipLeft;
    }

    if (top < clipTop) {
        vTop = 1.0f - (clipTop - top) * uvScale;
        top = clipTop;
    }

    const float rightMax = clipRight - 1.0f;
    if (right > rightMax) {
        uRight = 1.0f - ((right + 1.0f) - clipRight) * uvScale;
        right = rightMax;
    }

    const float bottomMax = clipBottom - 1.0f;
    if (bottom > bottomMax) {
        vBottom = ((bottom + 1.0f) - clipBottom) * uvScale;
        bottom = bottomMax;
    }

    zVec3 projectedVerts[4];
    projectedVerts[0].x = right;
    projectedVerts[0].y = bottom;
    projectedVerts[1].x = right;
    projectedVerts[1].y = top;
    projectedVerts[2].x = left;
    projectedVerts[2].y = top;
    projectedVerts[3].x = left;
    projectedVerts[3].y = bottom;

    zVec2 triUVs[4];
    triUVs[0].x = uRight;
    triUVs[0].y = vBottom;
    triUVs[1].x = uRight;
    triUVs[1].y = vTop;
    triUVs[2].x = uLeft;
    triUVs[2].y = vTop;
    triUVs[3].x = uLeft;
    triUVs[3].y = vBottom;

    if (g_zVideo_ActiveRendererPath != 0) {
        projectedVerts[0].z = 0.5f;
        projectedVerts[1].z = 0.5f;
        projectedVerts[2].z = 0.5f;
        projectedVerts[3].z = 0.5f;

        zVideo_RenderClass* renderClass = stageTexDirEntry != 0 ? (zVideo_RenderClass*)(stageTexDirEntry->texture) : 0;
        g_zVideo_pfnSubmitPolyRenderClass(
            (zVideo_XyzVertex*)(projectedVerts),
            (zVideo_TexCoord*)(triUVs),
            4,
            renderClass,
            0x10,
            1.0f,
            0
        );
        return;
    }

    const float kSoftwareScale = 0.100000001f;
    zVec3 clippedTriVerts[4] = {
        { right * kSoftwareScale, bottom * kSoftwareScale, kSoftwareScale },
        { right * kSoftwareScale, top * kSoftwareScale, kSoftwareScale },
        { left * kSoftwareScale, top * kSoftwareScale, kSoftwareScale },
        { left * kSoftwareScale, bottom * kSoftwareScale, kSoftwareScale },
    };
    {
        int vertexIndex1;
        for (vertexIndex1 = 0; vertexIndex1 < (int)(sizeof(projectedVerts) / sizeof((projectedVerts)[0]));
            ++vertexIndex1) {
            zVec3& vertex = (projectedVerts)[vertexIndex1];
            vertex.z = 10.0f;
        }
    }

    zRndrSubmitTexturedPolyUniformAlphaOrShade(
        projectedVerts,
        clippedTriVerts,
        projectedVerts,
        triUVs,
        4,
        stageTexDirEntry,
        1.0f,
        0
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-lensflare-drawvisiblesample
 * @recoil-artifact defines .text recoil:function:0x49afb0: zRndrLensFlareDrawVisibleSample
 *
 *
 * Purpose: Draw one visible lens-flare sample after applying near/far fade.
 */
void __fastcall zRndrLensFlareDrawVisibleSample(int sampleIndex)
{
    zRndr_LensFlareVisibleSampleDef* visibleSampleDef = zRndr::g_lensFlareVisibleSampleDefs[sampleIndex];
    zRndr_LensFlareSource* lensFlareSource = visibleSampleDef->lensFlareSource;

    if (zRndr::g_lensFlareVisibilityActive == 0) {
        return;
    }

    float visibility = 1.0f / visibleSampleDef->depthDivisor;
    if (!(visibility < lensFlareSource->fadeFar)) {
        return;
    }

    if (visibility < lensFlareSource->fadeNear) {
        visibility = 1.0f;
    } else {
        visibility = (lensFlareSource->fadeFar - visibility) / (lensFlareSource->fadeFar - lensFlareSource->fadeNear);
    }

    zRndrLensFlareDrawVisibleSampleStages(visibleSampleDef, visibility);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-lensflare-drawvisiblesamplestages
 * @recoil-artifact defines .text recoil:function:0x49b020: zRndrLensFlareDrawVisibleSampleStages
 *
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zRndr\zRndr_LensFlare.cpp.
 * Source file evidence: Binary Ninja function source comment.
 * Purpose: Draw the four staged lens-flare quads for one visible sample.
 */
void __fastcall
zRndrLensFlareDrawVisibleSampleStages(zRndr_LensFlareVisibleSampleDef* visibleSampleDef, float visibilityAlpha)
{
    const float activeWidth = (float)((unsigned int)(zRndr::g_activeRegionWidth));
    const float activeHeight = (float)((unsigned int)(zRndr::g_activeRegionHeight));
    const float baseRadius = visibilityAlpha * activeWidth * 0.03125f;
    const float largeRadius = baseRadius + baseRadius;
    const float halfClipWidth = activeWidth * 0.5f;
    const float halfClipHeight = activeHeight * 0.5f;
    const float sampleOffsetX = visibleSampleDef->sampleCenterX - halfClipWidth;
    const float sampleOffsetY = visibleSampleDef->sampleCenterY - halfClipHeight;
    const zRndr_LineClipRect2I* clipRect = (const zRndr_LineClipRect2I*)(&zRndr::g_activeRegionRect);

    zVec2 sampleCenter = { visibleSampleDef->sampleCenterX, visibleSampleDef->sampleCenterY };
    zRndrLensFlareDrawSampleStageClipped(
        &sampleCenter,
        zRndr::g_lensFlareVisibleSampleStages[0],
        largeRadius,
        clipRect
    );

    sampleCenter.x = halfClipWidth + sampleOffsetX * 0.5f;
    sampleCenter.y = halfClipHeight + sampleOffsetY * 0.5f;
    zRndrLensFlareDrawSampleStageClipped(
        &sampleCenter,
        zRndr::g_lensFlareVisibleSampleStages[1],
        largeRadius,
        clipRect
    );

    sampleCenter.x = halfClipWidth + sampleOffsetX * 0.100000001f;
    sampleCenter.y = halfClipHeight + sampleOffsetY * 0.100000001f;
    zRndrLensFlareDrawSampleStageClipped(&sampleCenter, zRndr::g_lensFlareVisibleSampleStages[2], baseRadius, clipRect);

    sampleCenter.x = halfClipWidth - sampleOffsetX;
    sampleCenter.y = halfClipHeight - sampleOffsetY;
    zRndrLensFlareDrawSampleStageClipped(
        &sampleCenter,
        zRndr::g_lensFlareVisibleSampleStages[3],
        baseRadius * 3.0f,
        clipRect
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-lensflare-drawvisiblesamples
 * @recoil-artifact defines .text recoil:function:0x49b1a0: zRndrLensFlareDrawVisibleSamples
 * @recoil-match byte
 *
 * Purpose: Draw all visible lens-flare samples and clear the visible-sample list.
 */
void __cdecl zRndrLensFlareDrawVisibleSamples()
{
    if (zRndr::g_lensFlareVisibilityActive == 0) {
        return;
    }

    for (int sampleIndex = 0; sampleIndex < zRndr::g_lensFlareVisibleSampleCount; ++sampleIndex) {
        zRndrLensFlareDrawVisibleSample(sampleIndex);
    }

    zRndr::g_lensFlareVisibleSampleCount = 0;
}
