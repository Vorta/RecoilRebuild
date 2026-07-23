#pragma once

#include <stddef.h>

#include "GameZRecoil/zHud/zhud_ui.h"

/**
 * Pass-3 HUD effect elements are HUD elements with an element-specific pass
 * callback used by Draw after the shared source-surface setup.
 */
struct zVideoFxPass3Element : HudUiElement {
    HudUiRect *clipRectOrNull;

    /**
     * Original inline helper; no standalone retail function exists. Observed
     * in the global config owner whose retail constructor at 0x4bef90
     * explicitly constructs the HudUiElement base and clears clipRectOrNull.
     * Purpose: leave default storage initialization inert for owner-managed
     * pass-3 elements.
     */
    zVideoFxPass3Element() {}
    /**
     * Original inline helper; no standalone retail function exists. Observed in
     * constructors 0x41eb30 and 0x41eb90 as HudUiElement::Constructor(0, 0)
     * followed by clearing the pass-3 clip pointer.
     * Purpose: construct a pass-3 HUD element while preserving derived virtual
     * dispatch identity.
     */
    zVideoFxPass3Element(
        int x,
        int y
    ) : HudUiElement(
            x,
            y
        ) {
        clipRectOrNull = 0;
    }

    void Draw();
    virtual void ApplyPass3();
};

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
    void SetInputRectByIndex(
        int index,
        HudUiRect *rectOrNull
    );
    void QueuePrimitiveRaw(
        void *primitive,
        int width,
        int height,
        int pitchBytes
    );
};

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(sizeof(zVideoFxPass3Element) == 0x38);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideoFxPass3Element,
        clipRectOrNull
    ) == 0x34
);
#endif
