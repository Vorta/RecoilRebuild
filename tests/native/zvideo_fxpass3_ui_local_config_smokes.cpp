#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "GameZRecoil/zVideo/zVideoFxPass3.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>

extern "C" unsigned int g_HudUi_InvalidateMask;

struct zVideoFxPass3RootElement : zVideoFxPass3Element {
    unsigned short packedColor16;
    unsigned char unknown_3a[0x06];
    double alpha;

    void ApplyPass3();
};

struct zVideoFxPass3Slot : zVideoFxPass3Element {
    int currentRadius;
    int maxRadius;
    int extent;
    float sinFreq;
    float sinPhase;

    zVideoFxPass3Slot();
    void SetRectAndPayload(
        int rectLeftPixels,
        int rectTopPixels,
        int currentRadiusPixels,
        int maxRadiusPixels,
        int extentPixels,
        float sinFreqValue,
        float sinPhaseValue
    );
    void ApplyPass3();
};

struct zVideoFxPass3Config : HudUiContainer {
    HudUiRect *inputRectsOrNull[2];
    unsigned short *surfacePixels;
    int surfaceWidth;
    int surfaceHeight;
    int surfacePitchBytes;
    zVideoFxPass3RootElement rootElement;
    zVideoFxPass3Slot slots[5];
    int slotWriteIndex;

    zVideoFxPass3Config();
    ~zVideoFxPass3Config();
};

namespace {
void *ReadVtable(
    const void *object
) {
    return *reinterpret_cast<void *const *>(object);
}

void WriteVtable(
    void *object,
    void *vtable
) {
    *reinterpret_cast<void **>(object) = vtable;
}

unsigned char *FxPass3ConfigBytes() {
    return reinterpret_cast<unsigned char *>(&g_zVideo_FxPass3ConfigLocal);
}

template <typename T>
T &FxPass3FieldAt(
    std::size_t offset
) {
    return *reinterpret_cast<T *>(FxPass3ConfigBytes() + offset);
}

enum {
    kFxPass3ConfigSize = 0x1f0,
    kFxPass3InputRect0Offset = 0x10,
    kFxPass3InputRect1Offset = 0x14,
    kFxPass3RootElementOffset = 0x28,
    kFxPass3RootPackedColorOffset = 0x38,
    kFxPass3RootAlphaOffset = 0x40,
    kFxPass3SlotsOffset = 0x70,
    kFxPass3SlotSize = 0x4c,
    kFxPass3SlotCurrentRadiusOffset = 0x38,
    kFxPass3SlotMaxRadiusOffset = 0x3c,
    kFxPass3SlotExtentOffset = 0x40,
    kFxPass3SlotSinFreqOffset = 0x44,
    kFxPass3SlotSinPhaseOffset = 0x48,
    kFxPass3SlotWriteIndexOffset = 0x1ec
};

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

void __fastcall CaptureFxPass3DrawBase(
    zVideoFxPass3Element *
) {
    ++g_fxPass3DrawBaseCount;
}

void __fastcall CaptureFxPass3ApplyCurrentInput(
    zVideoFxPass3Element *element
) {
    if (g_fxPass3ApplyCount < 4) {
        g_fxPass3ApplyRects[g_fxPass3ApplyCount] = element->clipRectOrNull;
    }
    ++g_fxPass3ApplyCount;
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

template <typename Method>
std::uintptr_t MethodAddress(
    Method method
) {
    std::uintptr_t address = 0;
    std::memcpy(&address, &method, sizeof(method));
    return address;
}

void *HudUiElementBaseVtable() {
    HudUiElement base;
    base.Constructor(0, 0);
    return ReadVtable(&base);
}

void ConstructGlobalFxPass3Config() {
    std::memset(&g_zVideo_FxPass3ConfigLocal, 0, sizeof(g_zVideo_FxPass3ConfigLocal));
    new (&g_zVideo_FxPass3ConfigLocal) zVideoFxPass3Config;
}
} // namespace

extern "C" int zvideo_fxpass3_local_queue_smoke(void) {
    unsigned char savedConfig[kFxPass3ConfigSize];
    std::memcpy(savedConfig, FxPass3ConfigBytes(), sizeof(savedConfig));
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0;

    ConstructGlobalFxPass3Config();
    zVideo::FxPass3_SetPrimaryElementParamsLocal(0x12345678u, 0.625);
    const bool rootWrapperOk =
        FxPass3FieldAt<unsigned short>(kFxPass3RootElementOffset + kFxPass3RootPackedColorOffset) ==
            0x5678u &&
        FxPass3FieldAt<double>(kFxPass3RootElementOffset + kFxPass3RootAlphaOffset) == 0.625 &&
        (g_zVideo_FxPass3ConfigLocal.rootElement.flags & 0x11u) == 0x01u &&
        g_zVideo_FxPass3ConfigLocal.rootElement.timer == 0.0f;

    ConstructGlobalFxPass3Config();
    zVideo::zVideoFxPass3Config_SetPrimaryElementParamsLocal(
        &g_zVideo_FxPass3ConfigLocal,
        0x0000abcdu,
        1.25
    );
    const bool rootConfigOk =
        FxPass3FieldAt<unsigned short>(kFxPass3RootElementOffset + kFxPass3RootPackedColorOffset) ==
            0xabcdu &&
        FxPass3FieldAt<double>(kFxPass3RootElementOffset + kFxPass3RootAlphaOffset) == 1.25 &&
        (g_zVideo_FxPass3ConfigLocal.rootElement.flags & 0x01u) != 0;

    ConstructGlobalFxPass3Config();
    HudUiRect inputRect0 = {1, 2, 3, 4};
    HudUiRect inputRect1 = {5, 6, 7, 8};
    HudUiRect ignoredInputRect = {9, 10, 11, 12};
    zVideo::FxPass3_SetInputRectByIndex(0, &inputRect0);
    zVideo::FxPass3_SetInputRectByIndex(1, &inputRect1);
    zVideo::FxPass3_SetInputRectByIndex(2, &ignoredInputRect);
    const bool inputRectWrapperOk =
        FxPass3FieldAt<HudUiRect *>(kFxPass3InputRect0Offset) == &inputRect0 &&
        FxPass3FieldAt<HudUiRect *>(kFxPass3InputRect1Offset) == &inputRect1;

    ConstructGlobalFxPass3Config();
    zVideo::FxPass3_QueueElementLocal(11, 22, 33, 44, 55, 1.5f, 2.5f);
    const zVideoFxPass3Slot &slot0 = g_zVideo_FxPass3ConfigLocal.slots[0];
    const bool queueWrapperOk =
        FxPass3FieldAt<int>(kFxPass3SlotWriteIndexOffset) == 1 && slot0.x == 11 &&
        slot0.y == 22 && slot0.timer == 0.0f && (slot0.flags & 0x01u) != 0 &&
        FxPass3FieldAt<int>(kFxPass3SlotsOffset + kFxPass3SlotCurrentRadiusOffset) == 33 &&
        FxPass3FieldAt<int>(kFxPass3SlotsOffset + kFxPass3SlotMaxRadiusOffset) == 44 &&
        FxPass3FieldAt<int>(kFxPass3SlotsOffset + kFxPass3SlotExtentOffset) == 55 &&
        FxPass3FieldAt<float>(kFxPass3SlotsOffset + kFxPass3SlotSinFreqOffset) == 1.5f &&
        FxPass3FieldAt<float>(kFxPass3SlotsOffset + kFxPass3SlotSinPhaseOffset) == 2.5f;

    ConstructGlobalFxPass3Config();
    FxPass3FieldAt<int>(kFxPass3SlotWriteIndexOffset) = 4;
    zVideo::zVideoFxPass3Config_QueueElementLocal(
        &g_zVideo_FxPass3ConfigLocal,
        1,
        2,
        3,
        4,
        5,
        6.0f,
        7.0f
    );
    const std::size_t slot4Offset = kFxPass3SlotsOffset + kFxPass3SlotSize * 4;
    const zVideoFxPass3Slot &slot4 = g_zVideo_FxPass3ConfigLocal.slots[4];
    const bool queueCapOk =
        FxPass3FieldAt<int>(kFxPass3SlotWriteIndexOffset) == 4 && slot4.x == 1 &&
        slot4.y == 2 &&
        FxPass3FieldAt<float>(slot4Offset + kFxPass3SlotSinPhaseOffset) == 7.0f;

    zVideoFxPass3Config config;
    std::memset(&config, 0, sizeof(config));
    TestFxPass3UpdateElement updateA;
    TestFxPass3UpdateElement updateB;
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

    ConstructGlobalFxPass3Config();
    TestFxPass3UpdateElement updateGlobalA;
    TestFxPass3UpdateElement updateGlobalB;
    g_zVideo_FxPass3ConfigLocal.childHead = &updateGlobalA;
    g_zVideo_FxPass3ConfigLocal.childTail = &updateGlobalB;
    updateGlobalA.next = &updateGlobalB;
    updateGlobalB.next = 0;
    g_zVideo_FxPass3ConfigLocal.slotWriteIndex = 2;
    g_fxPass3UpdateCount = 0;
    zVideo::FxPass3_UpdateLocal(0.5f);
    const bool updateWrapperOk =
        g_fxPass3UpdateCount == 2 && g_fxPass3UpdateDelta[0] == 0.5f &&
        g_fxPass3UpdateDelta[1] == 0.5f &&
        g_zVideo_FxPass3ConfigLocal.slotWriteIndex == 0;

    std::memcpy(FxPass3ConfigBytes(), savedConfig, sizeof(savedConfig));
    g_HudUi_InvalidateMask = oldInvalidateMask;

    return rootWrapperOk && rootConfigOk && inputRectWrapperOk && queueWrapperOk && queueCapOk &&
                   updateConfigOk && updateWrapperOk
               ? 0
               : 1;
}

extern "C" int zvideo_fxpass3_element_draw_smoke(void) {
    unsigned char savedConfig[kFxPass3ConfigSize];
    std::memcpy(savedConfig, FxPass3ConfigBytes(), sizeof(savedConfig));
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    const int oldFxWidth = g_zVideo_FxSurfaceWidth;
    const int oldFxHeight = g_zVideo_FxSurfaceHeight;
    const int oldFxPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldFxPitchPixels = g_zVideo_FxSurfacePitchPixels16;

    zVideoFxPass3Config config;
    std::memset(&config, 0, sizeof(config));
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

    std::memset(&config, 0, sizeof(config));
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

    std::memcpy(FxPass3ConfigBytes(), savedConfig, sizeof(savedConfig));
    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels;

    return parentConfigOk && nullFirstInputOk && noParentOk ? 0 : 1;
}

extern "C" int zvideo_fxpass3_config_constructor_destructor_smoke(void) {
    union ConfigStorage {
        void *alignPtr;
        double alignDouble;
        unsigned char bytes[sizeof(zVideoFxPass3Config)];
    } configStorage;

    std::memset(configStorage.bytes, 0xcc, sizeof(configStorage.bytes));
    zVideoFxPass3Config *const constructed =
        new (configStorage.bytes) zVideoFxPass3Config;
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

    const void *const baseVtable = HudUiElementBaseVtable();
    const bool constructorOk =
        constructed == &config && config.enabled == 1 && childChainOk &&
        config.inputRectsOrNull[0] == 0 && config.inputRectsOrNull[1] == 0 &&
        config.surfacePixels == 0 && config.surfaceWidth == 0 && config.surfaceHeight == 0 &&
        config.surfacePitchBytes == static_cast<int>(0xccccccccu) &&
        config.slotWriteIndex == 0 && config.rootElement.clipRectOrNull == 0 &&
        config.slots[0].clipRectOrNull == 0 && ReadVtable(&config.rootElement) != baseVtable &&
        ReadVtable(&config.slots[0]) != baseVtable &&
        (config.rootElement.flags & 0x10u) != 0 && (config.slots[0].flags & 0x10u) != 0;

    config.~zVideoFxPass3Config();
    bool destructorOk = ReadVtable(&config.rootElement) == baseVtable;
    for (int i = 0; i < 5; ++i) {
        destructorOk = destructorOk && ReadVtable(&config.slots[i]) == baseVtable;
    }

    unsigned char savedConfig[kFxPass3ConfigSize];
    std::memcpy(savedConfig, FxPass3ConfigBytes(), sizeof(savedConfig));
    std::memset(&g_zVideo_FxPass3ConfigLocal, 0xcc, sizeof(g_zVideo_FxPass3ConfigLocal));
    zVideoFxPass3Config *const globalConstructed =
        new (&g_zVideo_FxPass3ConfigLocal) zVideoFxPass3Config;
    const bool globalConstructOk =
        globalConstructed == &g_zVideo_FxPass3ConfigLocal &&
        g_zVideo_FxPass3ConfigLocal.enabled == 1;
    g_zVideo_FxPass3ConfigLocal.~zVideoFxPass3Config();
    const bool globalDestroyOk =
        ReadVtable(&g_zVideo_FxPass3ConfigLocal.rootElement) == baseVtable;
    std::memcpy(FxPass3ConfigBytes(), savedConfig, sizeof(savedConfig));

    return constructorOk && destructorOk && globalConstructOk && globalDestroyOk ? 0 : 1;
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
