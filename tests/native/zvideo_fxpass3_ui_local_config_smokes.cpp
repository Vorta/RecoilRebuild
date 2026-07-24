#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "GameZRecoil/zVideo/zvid_fx_pass3.h"

#include <cstdint>
#include <cstring>

extern "C" unsigned int g_HudUi_InvalidateMask;

namespace {
int g_fxPass3UpdateCount;
float g_fxPass3UpdateDelta[4];
int g_fxPass3DrawBaseCount;
int g_fxPass3ApplyCount;
HudUiRect *g_fxPass3ApplyRects[4];

void ResetFxPass3DrawCapture() {
    g_fxPass3DrawBaseCount = 0;
    g_fxPass3ApplyCount = 0;
    for (int i = 0; i < 4; ++i) {
        g_fxPass3ApplyRects[i] = 0;
    }
}

struct TestFxPass3UpdateElement : HudUiElement {
    void Update(float deltaSeconds) {
        const int index = g_fxPass3UpdateCount;
        if (index < 4) {
            g_fxPass3UpdateDelta[index] = deltaSeconds;
        }
        ++g_fxPass3UpdateCount;
    }
};

struct TestFxPass3Element : zVideoFxPass3Element {
    void DrawBase() {
        ++g_fxPass3DrawBaseCount;
    }

    void ApplyPass3() {
        if (g_fxPass3ApplyCount < 4) {
            g_fxPass3ApplyRects[g_fxPass3ApplyCount] = clipRectOrNull;
        }
        ++g_fxPass3ApplyCount;
    }
};

} // namespace

extern "C" int zvideo_fxpass3_local_queue_smoke(void) {
    const zVideoFxPass3Config savedGlobalConfig =
        g_zVideo_FxPass3ConfigLocal;
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0;

    zVideoFxPass3Config config;
    zVideo::zVideoFxPass3Config_SetPrimaryElementParamsLocal(
        &config,
        0x0000abcdu,
        1.25
    );
    const bool rootConfigOk =
        config.rootElement.packedColor16 == 0xabcdu &&
        config.rootElement.alpha == 1.25 &&
        (config.rootElement.flags & 0x01u) != 0;

    HudUiRect inputRect0 = {1, 2, 3, 4};
    HudUiRect inputRect1 = {5, 6, 7, 8};
    HudUiRect ignoredInputRect = {9, 10, 11, 12};
    config.SetInputRectByIndex(0, &inputRect0);
    config.SetInputRectByIndex(1, &inputRect1);
    config.SetInputRectByIndex(2, &ignoredInputRect);
    const bool inputRectConfigOk =
        config.inputRectsOrNull[0] == &inputRect0 &&
        config.inputRectsOrNull[1] == &inputRect1;

    config.slotWriteIndex = 0;
    zVideo::zVideoFxPass3Config_QueueElementLocal(
        &config,
        11,
        22,
        33,
        44,
        55,
        1.5f,
        2.5f
    );
    const zVideoFxPass3Slot &slot0 = config.slots[0];
    const bool queueConfigOk =
        config.slotWriteIndex == 1 && slot0.x == 11 &&
        slot0.y == 22 && slot0.timer == 0.0f && (slot0.flags & 0x01u) != 0 &&
        slot0.currentRadius == 33 && slot0.maxRadius == 44 &&
        slot0.extent == 55 && slot0.sinFreq == 1.5f &&
        slot0.sinPhase == 2.5f;

    config.slotWriteIndex = 4;
    zVideo::zVideoFxPass3Config_QueueElementLocal(
        &config,
        1,
        2,
        3,
        4,
        5,
        6.0f,
        7.0f
    );
    const zVideoFxPass3Slot &slot4 = config.slots[4];
    const bool queueCapOk =
        config.slotWriteIndex == 4 && slot4.x == 1 &&
        slot4.y == 2 && slot4.sinPhase == 7.0f;

    TestFxPass3UpdateElement updateA;
    TestFxPass3UpdateElement updateB;
    HudUiElement *const configChildHead = config.childHead;
    HudUiElement *const configChildTail = config.childTail;
    config.enabled = 1;
    config.childHead = &updateA;
    config.childTail = &updateB;
    updateA.next = &updateB;
    updateB.next = 0;
    config.slotWriteIndex = 3;
    g_fxPass3UpdateCount = 0;
    zVideo::zVideoFxPass3Config_UpdateLocal(&config, 0.75f);
    const bool updateConfigOk =
        g_fxPass3UpdateCount == 2 && g_fxPass3UpdateDelta[0] == 0.75f &&
        g_fxPass3UpdateDelta[1] == 0.75f && config.slotWriteIndex == 0;
    config.childHead = configChildHead;
    config.childTail = configChildTail;

    zVideo::FxPass3_SetPrimaryElementParamsLocal(0x12345678u, 0.625);
    const bool rootWrapperOk =
        g_zVideo_FxPass3ConfigLocal.rootElement.packedColor16 == 0x5678u &&
        g_zVideo_FxPass3ConfigLocal.rootElement.alpha == 0.625 &&
        (g_zVideo_FxPass3ConfigLocal.rootElement.flags & 0x01u) != 0 &&
        g_zVideo_FxPass3ConfigLocal.rootElement.timer == 0.0f;

    zVideo::FxPass3_SetInputRectByIndex(0, &inputRect0);
    zVideo::FxPass3_SetInputRectByIndex(1, &inputRect1);
    zVideo::FxPass3_SetInputRectByIndex(2, &ignoredInputRect);
    const bool inputRectWrapperOk =
        g_zVideo_FxPass3ConfigLocal.inputRectsOrNull[0] == &inputRect0 &&
        g_zVideo_FxPass3ConfigLocal.inputRectsOrNull[1] == &inputRect1;

    g_zVideo_FxPass3ConfigLocal.slotWriteIndex = 0;
    zVideo::FxPass3_QueueElementLocal(11, 22, 33, 44, 55, 1.5f, 2.5f);
    const bool queueWrapperOk =
        g_zVideo_FxPass3ConfigLocal.slotWriteIndex == 1 &&
        g_zVideo_FxPass3ConfigLocal.slots[0].currentRadius == 33 &&
        g_zVideo_FxPass3ConfigLocal.slots[0].maxRadius == 44 &&
        g_zVideo_FxPass3ConfigLocal.slots[0].extent == 55;

    g_zVideo_FxPass3ConfigLocal.slotWriteIndex = 2;
    zVideo::FxPass3_UpdateLocal(0.5f);
    const bool updateWrapperOk =
        g_zVideo_FxPass3ConfigLocal.slotWriteIndex == 0;

    g_zVideo_FxPass3ConfigLocal = savedGlobalConfig;
    g_HudUi_InvalidateMask = oldInvalidateMask;

    return rootWrapperOk && rootConfigOk && inputRectConfigOk &&
                   inputRectWrapperOk && queueConfigOk && queueWrapperOk &&
                   queueCapOk && updateConfigOk && updateWrapperOk
               ? 0
               : 1;
}

extern "C" int zvideo_fxpass3_element_draw_smoke(void) {
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    const int oldFxWidth = g_zVideo_FxSurfaceWidth;
    const int oldFxHeight = g_zVideo_FxSurfaceHeight;
    const int oldFxPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldFxPitchPixels = g_zVideo_FxSurfacePitchPixels16;

    zVideoFxPass3Config config;
    HudUiRect rect0 = {1, 2, 3, 4};
    HudUiRect rect1 = {5, 6, 7, 8};
    HudUiRect sentinel = {9, 10, 11, 12};
    unsigned short pixels[12] = {};
    config.inputRectsOrNull[0] = &rect0;
    config.inputRectsOrNull[1] = &rect1;
    config.surfacePixels = pixels;
    config.surfaceWidth = 4;
    config.surfaceHeight = 3;
    config.surfacePitchBytes = 8;

    TestFxPass3Element element;
    element.parent = &config;
    element.clipRectOrNull = &sentinel;

    ResetFxPass3DrawCapture();
    element.zVideoFxPass3Element::Draw();
    const bool parentConfigOk =
        g_fxPass3DrawBaseCount == 1 && g_fxPass3ApplyCount == 2 &&
        g_fxPass3ApplyRects[0] == &rect0 && g_fxPass3ApplyRects[1] == &rect1 &&
        element.clipRectOrNull == &rect0 && g_zVideo_FxSurfacePixels16 == pixels &&
        g_zVideo_FxSurfaceWidth == 4 && g_zVideo_FxSurfaceHeight == 3 &&
        g_zVideo_FxSurfacePitchBytes == 8 && g_zVideo_FxSurfacePitchPixels16 == 4;

    config.inputRectsOrNull[0] = 0;
    config.inputRectsOrNull[1] = &rect1;
    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels;
    element.parent = &config;
    element.clipRectOrNull = &sentinel;

    ResetFxPass3DrawCapture();
    element.zVideoFxPass3Element::Draw();
    const bool nullFirstInputOk =
        g_fxPass3DrawBaseCount == 1 && g_fxPass3ApplyCount == 1 &&
        g_fxPass3ApplyRects[0] == &rect1 && element.clipRectOrNull == 0 &&
        g_zVideo_FxSurfacePixels16 == oldFxPixels && g_zVideo_FxSurfaceWidth == oldFxWidth &&
        g_zVideo_FxSurfaceHeight == oldFxHeight &&
        g_zVideo_FxSurfacePitchBytes == oldFxPitchBytes &&
        g_zVideo_FxSurfacePitchPixels16 == oldFxPitchPixels;

    element.parent = 0;
    element.clipRectOrNull = &sentinel;
    ResetFxPass3DrawCapture();
    element.zVideoFxPass3Element::Draw();
    const bool noParentOk =
        g_fxPass3DrawBaseCount == 1 && g_fxPass3ApplyCount == 1 &&
        g_fxPass3ApplyRects[0] == &sentinel && element.clipRectOrNull == &sentinel;

    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels;

    return parentConfigOk && nullFirstInputOk && noParentOk ? 0 : 1;
}

extern "C" int zvideo_fxpass3_config_constructor_destructor_smoke(void) {
    zVideoFxPass3Config *const constructed =
        new zVideoFxPass3Config;
    zVideoFxPass3Config &config = *constructed;
    bool childChainOk =
        config.childHead == &config.rootElement && config.rootElement.parent == &config &&
        config.rootElement.next == &config.slots[0] && config.childTail == &config.slots[4];
    for (int i = 0; i < 5; ++i) {
        childChainOk = childChainOk && config.slots[i].parent == &config;
        if (i < 4) {
            childChainOk = childChainOk && config.slots[i].next == &config.slots[i + 1];
        } else {
            childChainOk = childChainOk && config.slots[i].next == 0;
        }
    }

    const bool constructorOk =
        constructed == &config && config.enabled == 1 && childChainOk &&
        config.inputRectsOrNull[0] == 0 && config.inputRectsOrNull[1] == 0 &&
        config.surfacePixels == 0 && config.surfaceWidth == 0 && config.surfaceHeight == 0 &&
        config.slotWriteIndex == 0 && config.rootElement.clipRectOrNull == 0 &&
        config.slots[0].clipRectOrNull == 0 &&
        (config.rootElement.flags & 0x10u) != 0 && (config.slots[0].flags & 0x10u) != 0;

    delete constructed;
    const bool destructorOk = true;

    const bool globalConstructOk =
        g_zVideo_FxPass3ConfigLocal.enabled == 1 &&
        g_zVideo_FxPass3ConfigLocal.childHead ==
            &g_zVideo_FxPass3ConfigLocal.rootElement &&
        g_zVideo_FxPass3ConfigLocal.childTail ==
            &g_zVideo_FxPass3ConfigLocal.slots[4];

    return constructorOk && destructorOk && globalConstructOk ? 0 : 1;
}

extern "C" int zvideo_fxpass3_copy_surface_pixel_clipped_smoke(void) {
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    unsigned short *const oldScratch = g_zVideo_FxPass3_ScratchPixels16;
    const int oldWidth = g_zVideo_FxSurfaceWidth;
    const int oldHeight = g_zVideo_FxSurfaceHeight;
    const int oldPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldPitchPixels = g_zVideo_FxSurfacePitchPixels16;
    const int oldOffsetX = g_zVideo_FxPass3_ScratchOffsetX;
    const int oldOffsetY = g_zVideo_FxPass3_ScratchOffsetY;
    const int oldClipMinX = g_zVideo_FxPass3_ClipMinX;
    const int oldClipMinY = g_zVideo_FxPass3_ClipMinY;
    const int oldClipMaxX = g_zVideo_FxPass3_ClipMaxX;
    const int oldClipMaxY = g_zVideo_FxPass3_ClipMaxY;

    unsigned short pixels[25] = {};
    unsigned short scratch[25] = {};
    for (int i = 0; i < 25; ++i) {
        pixels[i] = static_cast<unsigned short>(0x1000 + i);
        scratch[i] = 0xffff;
    }

    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxPass3_ScratchPixels16 = scratch;
    g_zVideo_FxSurfaceWidth = 5;
    g_zVideo_FxSurfaceHeight = 5;
    g_zVideo_FxSurfacePitchBytes = 10;
    g_zVideo_FxSurfacePitchPixels16 = 5;
    g_zVideo_FxPass3_ScratchOffsetX = 2;
    g_zVideo_FxPass3_ScratchOffsetY = 2;
    g_zVideo_FxPass3_ClipMinX = 1;
    g_zVideo_FxPass3_ClipMinY = 1;
    g_zVideo_FxPass3_ClipMaxX = 4;
    g_zVideo_FxPass3_ClipMaxY = 4;

    zVideo::FxPass3_CopySurfacePixelToScratchClipped(
        0,
        0,
        1,
        1
    );
    const bool copiedOk = scratch[2 + 2 * 5] == pixels[3 + 3 * 5];

    zVideo::FxPass3_CopySurfacePixelToScratchClipped(
        2,
        0,
        0,
        0
    );
    const bool dstClipOk = scratch[4 + 2 * 5] == 0xffff;

    zVideo::FxPass3_CopySurfacePixelToScratchClipped(
        0,
        0,
        -2,
        0
    );
    const bool srcClipOk = scratch[2 + 2 * 5] == pixels[3 + 3 * 5];

    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxPass3_ScratchPixels16 = oldScratch;
    g_zVideo_FxSurfaceWidth = oldWidth;
    g_zVideo_FxSurfaceHeight = oldHeight;
    g_zVideo_FxSurfacePitchBytes = oldPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldPitchPixels;
    g_zVideo_FxPass3_ScratchOffsetX = oldOffsetX;
    g_zVideo_FxPass3_ScratchOffsetY = oldOffsetY;
    g_zVideo_FxPass3_ClipMinX = oldClipMinX;
    g_zVideo_FxPass3_ClipMinY = oldClipMinY;
    g_zVideo_FxPass3_ClipMaxX = oldClipMaxX;
    g_zVideo_FxPass3_ClipMaxY = oldClipMaxY;

    return copiedOk && dstClipOk && srcClipOk ? 0 : 1;
}

extern "C" int zvideo_fxpass3_apply_to_current_surface_smoke(void) {
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    unsigned short *const oldScratch = g_zVideo_FxPass3_ScratchPixels16;
    const int oldWidth = g_zVideo_FxSurfaceWidth;
    const int oldHeight = g_zVideo_FxSurfaceHeight;
    const int oldPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldPitchPixels = g_zVideo_FxSurfacePitchPixels16;
    const int oldOffsetX = g_zVideo_FxPass3_ScratchOffsetX;
    const int oldOffsetY = g_zVideo_FxPass3_ScratchOffsetY;
    const int oldClipMinX = g_zVideo_FxPass3_ClipMinX;
    const int oldClipMinY = g_zVideo_FxPass3_ClipMinY;
    const int oldClipMaxX = g_zVideo_FxPass3_ClipMaxX;
    const int oldClipMaxY = g_zVideo_FxPass3_ClipMaxY;

    unsigned short pixels[49];
    unsigned short original[49];
    unsigned short scratch[49];
    for (int i = 0; i < 49; ++i) {
        pixels[i] = static_cast<unsigned short>(0x1000 + i);
        original[i] = pixels[i];
        scratch[i] = 0xffff;
    }

    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxPass3_ScratchPixels16 = scratch;
    g_zVideo_FxSurfaceWidth = 7;
    g_zVideo_FxSurfaceHeight = 7;
    g_zVideo_FxSurfacePitchBytes = 14;
    g_zVideo_FxSurfacePitchPixels16 = 7;

    zVideo::FxPass3_ApplyToCurrentSurface(
        3,
        3,
        2,
        2,
        2,
        1.0f,
        0.0f,
        0
    );
    bool cappedNoopOk = true;
    for (int i = 0; i < 49; ++i) {
        cappedNoopOk = cappedNoopOk && pixels[i] == original[i];
    }

    for (int i = 0; i < 49; ++i) {
        pixels[i] = original[i];
        scratch[i] = 0xffff;
    }
    zVideo::FxPass3_ApplyToCurrentSurface(
        3,
        3,
        1,
        2,
        2,
        1.0f,
        0.0f,
        0
    );
    const bool fullWarpOk = pixels[2 + 2 * 7] == original[1 + 1 * 7] &&
                            pixels[3 + 3 * 7] == original[3 + 3 * 7];

    for (int i = 0; i < 49; ++i) {
        pixels[i] = original[i];
        scratch[i] = 0xffff;
    }
    zVidRect32 clip = {1, 1, 5, 5};
    zVideo::FxPass3_ApplyToCurrentSurface(
        3,
        3,
        1,
        2,
        2,
        1.0f,
        0.0f,
        &clip
    );
    const bool clippedWarpOk = pixels[2 + 2 * 7] == original[1 + 1 * 7] &&
                               pixels[0 + 0 * 7] == original[0 + 0 * 7] &&
                               g_zVideo_FxPass3_ScratchOffsetX == 3 &&
                               g_zVideo_FxPass3_ScratchOffsetY == 3;

    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxPass3_ScratchPixels16 = oldScratch;
    g_zVideo_FxSurfaceWidth = oldWidth;
    g_zVideo_FxSurfaceHeight = oldHeight;
    g_zVideo_FxSurfacePitchBytes = oldPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldPitchPixels;
    g_zVideo_FxPass3_ScratchOffsetX = oldOffsetX;
    g_zVideo_FxPass3_ScratchOffsetY = oldOffsetY;
    g_zVideo_FxPass3_ClipMinX = oldClipMinX;
    g_zVideo_FxPass3_ClipMinY = oldClipMinY;
    g_zVideo_FxPass3_ClipMaxX = oldClipMaxX;
    g_zVideo_FxPass3_ClipMaxY = oldClipMaxY;

    return cappedNoopOk && fullWarpOk && clippedWarpOk ? 0 : 1;
}
