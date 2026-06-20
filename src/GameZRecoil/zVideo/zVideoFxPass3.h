#pragma once

#include <stddef.h>

#include "GameZRecoil/zHud/zhud_ui.h"

// Pass-3 HUD effect elements are HUD elements with an element-specific pass
// callback used by Draw after the shared source-surface setup.
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

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(sizeof(zVideoFxPass3Element) == 0x38);
RECOIL_STATIC_ASSERT(
    offsetof(
        zVideoFxPass3Element,
        clipRectOrNull
    ) == 0x34
);
#endif
