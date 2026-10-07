#pragma once

#include <stddef.h>

#include "GameZRecoil/zHud/zhud_ui.h"

/**
 * Pass-3 HUD effect elements are HUD elements with an element-specific pass
 * callback used by Draw after the shared source-surface setup.
 */
struct zVideoFxPass3Element : HudUiElement {
    HudUiRect* clipRectOrNull;

    /**
     * Original inline helper; no standalone retail function exists. Observed
     * in the global config owner whose retail constructor at 0x4bef90
     * explicitly constructs the HudUiElement base and clears clipRectOrNull.
     * Purpose: leave default storage initialization inert for owner-managed
     * pass-3 elements.
     */
    zVideoFxPass3Element() { }
    /**
     * Original inline helper; no standalone retail function exists. Observed in
     * constructors 0x41eb30 and 0x41eb90 as HudUiElement::Constructor(0, 0)
     * followed by clearing the pass-3 clip pointer.
     * Purpose: construct a pass-3 HUD element while preserving derived virtual
     * dispatch identity.
     */
    zVideoFxPass3Element(int x, int y)
        : HudUiElement(x, y)
    {
        clipRectOrNull = 0;
    }

    void Draw();
    virtual void ApplyPass3();
};

struct zVideoFxPass3RootElement : zVideoFxPass3Element {
    unsigned short packedColor16;
    unsigned char unknown_3a[0x06];
    double alpha;

    /**
     * Inline helper emitted by the config constructor at 0x4bef90 before
     * its five-element slot array; no standalone retail body exists.
     * Purpose: Construct the root pass-3 element at the origin through its base.
     */
    zVideoFxPass3RootElement()
        : zVideoFxPass3Element(0, 0)
    {
    }

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

/**
 * @recoil-anchor recoil:anchor:zui.zui-fx.z-video-fx-pass3-config-destroy-z-video-fx-pass3-config
 * @recoil-artifact emits .text recoil:function:0x4bee80: VC5 compiler-generated implicit destructor (no vptr store; destroys the slot array and root element, then ~HudUiContainer) anchored to this complete type definition; not an authored body.
 * Purpose: define the pass-3 config container whose implicit destructor runs from the zui_fx.cpp singleton's atexit
 * cleanup.
 */
struct zVideoFxPass3Config : HudUiContainer {
    HudUiRect* inputRectsOrNull[2];
    unsigned short* surfacePixels;
    int surfaceWidth;
    int surfaceHeight;
    int surfacePitchBytes;
    zVideoFxPass3RootElement rootElement;
    zVideoFxPass3Slot slots[5];
    int slotWriteIndex;

    zVideoFxPass3Config();
    void UpdateLocal(float deltaTime);
    void SetPrimaryElementParamsLocal(unsigned short packedColor, double primaryAlpha);
    void QueueElementLocal(
        int rectLeftPixels,
        int rectTopPixels,
        int currentRadiusPixels,
        int maxRadiusPixels,
        int extentPixels,
        float sinFreq,
        float sinPhase
    );
    void SetInputRectByIndex(int index, HudUiRect* rectOrNull);
    void QueuePrimitiveRaw(void* primitive, int width, int height, int pitchBytes);
};

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(sizeof(zVideoFxPass3Element) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zVideoFxPass3Element, clipRectOrNull) == 0x34);
#endif

extern "C" {
extern int g_zVid_NoiseByteTableSize;
extern unsigned char* g_zVid_NoiseByteTable;
extern unsigned short* g_zVideo_FxPass3_ScratchPixels16;
extern unsigned short* g_zVideo_FxSurfacePixels16;
extern int g_zVideo_FxSurfaceWidth;
extern int g_zVideo_FxSurfaceHeight;
extern int g_zVideo_FxSurfacePitchBytes;
extern int g_zVideo_FxSurfacePitchPixels16;
extern int g_zVideo_FxPass3_ScratchOffsetX;
extern int g_zVideo_FxPass3_ScratchOffsetY;
extern int g_zVideo_FxPass3_ClipMinX;
extern int g_zVideo_FxPass3_ClipMinY;
extern int g_zVideo_FxPass3_ClipMaxX;
extern int g_zVideo_FxPass3_ClipMaxY;
}
