#pragma once

#include <stddef.h>

#include "GameZRecoil/zHud/zhud_ui.h"

// Pass-3 HUD effect elements are HUD elements with an element-specific pass
// callback used by Draw after the shared source-surface setup.
struct zVideoFxPass3Element : HudUiElement {
    HudUiRect *clipRectOrNull;

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
