// zUI compilation unit for the HudUiElement base, inferred from the retail
// object boundary [0x4b4030, 0x4b42f0): its pooled .rdata [0x4d34a0, 0x4d34ac)
// holds double 0.0 and 0.0f, which zui_widgets.cpp pools again at 0x4d34c8 and
// 0x4d34d4. The 1998 demos link it between the zui_widgets.cpp and zui_panel.cpp
// objects, apart from zui_image.cpp. Original filename unresolved;
// zui_element.cpp is a provisional name (2026-10-03).

#include "recoil/Mfc42Abi.h"

#include "GameZRecoil/zHud/zhud_ui.h"

int HudUiWidget::HitTest(int px, int py)
{
    if ((~flags & 0x10) == 0) {
        return 0;
    }

    HudUiRect* const bounds = GetBoundsRectOrNull();
    if (bounds == 0) {
        return 0;
    }

    return px >= bounds->left && px <= bounds->right && py >= bounds->top && py <= bounds->bottom ? 1 : 0;
}

/**
 * Purpose: Initializes the common HUD element position, links, timer, invalidation state, and blit source.
 */
HudUiElement::HudUiElement(int initX, int initY)
{
    HudUiElement* const element = this;
    parent = 0;
    next = 0;
    timer = 0.0f;
    element->x = initX;
    element->y = initY;
    element->Invalidate();

    flags = 0;
    state = 0;
    HudUiElement::SetBltSourceAndClipRect(0, 0);
}

/**
 * Purpose: initialize the recovered HudUiElement::Constructor state.
 */
HudUiElement* HudUiElement::Constructor(int initX, int initY)
{
    new (this) HudUiElement(initX, initY);
    return this;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduielement-huduielement
 * @recoil-artifact defines .text recoil:function:0x4b40c0: HudUiElement::HudUiElement(const HudUiElement &).
 * @recoil-match byte
 *
 * Purpose: initialize a HUD element from another element while clearing owner links.
 */
HudUiElement::HudUiElement(const HudUiElement& source)
{
    next = 0;
    parent = 0;
    flags = source.flags;
    state = source.state;
    timer = source.timer;
    x = source.x;
    y = source.y;
    bltSource = source.bltSource;
    clipRect = source.clipRect;
}

/**
 * Purpose: copy another HUD element's runtime fields while preserving dispatch identity.
 */
HudUiElement& HudUiElement::operator=(const HudUiElement& source)
{
    next = 0;
    parent = 0;
    flags = source.flags;
    state = source.state;
    timer = source.timer;
    x = source.x;
    y = source.y;
    bltSource = source.bltSource;
    clipRect = source.clipRect;
    return *this;
}

/**
 * Purpose: reset the HudUiElement virtual table during class destruction.
 *
 * Evidence: the definition is kept inline in zhud_ui.h so VC5 can inline the
 * base table reset into derived destructors while still emitting the standalone
 * element destructor COMDAT when the address-backed symbol is required.
 */

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduielement-invalidate
 * @recoil-artifact defines .text recoil:function:0x4b4180: HudUiElement::Invalidate.
 * @recoil-match byte
 *
 * Purpose: mark the element dirty by OR-ing the current HUD invalidation mask into its flags.
 */
void HudUiElement::Invalidate()
{
    flags |= g_HudUi_InvalidateMask;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduielement-setbltsourceandcliprect
 * @recoil-artifact defines .text recoil:function:0x4b4190: HudUiElement::SetBltSourceAndClipRect.
 * @recoil-match byte
 *
 * Purpose: apply the recovered HUD state change handled by HudUiElement::SetBltSourceAndClipRect.
 */
void HudUiElement::SetBltSourceAndClipRect(void* bltSourceOrNull, const HudUiRect* rectOrNull)
{
    bltSource = bltSourceOrNull;
    SetClipRect(rectOrNull);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduielement-setcliprect
 * @recoil-artifact defines .text recoil:function:0x4b41b0: HudUiElement::SetClipRect.
 * @recoil-match byte
 *
 * Purpose: replace the element clip rectangle when a source rectangle is supplied.
 * Binary Ninja: 0x4b41b0 returns immediately for a null argument; otherwise it
 * copies the four HudUiRect fields into the clipRect member at offset 0x20.
 */
void HudUiElement::SetClipRect(const HudUiRect* rect)
{
    if (rect == 0) {
        return;
    }

    clipRect = *rect;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduielement-update
 * @recoil-artifact defines .text recoil:function:0x4b41e0: HudUiElement::Update.
 * @recoil-match byte
 *
 * Purpose: dispatch visible or hidden dirty drawing and hide the element when its timer expires.
 */
void HudUiElement::Update(float deltaSeconds)
{
    unsigned int currentFlags = flags;

    if (((~currentFlags) & 0x10) != 0) {
        if ((currentFlags & 0x02) == 0) {
            Draw();
        } else if ((currentFlags & 0x04) != 0) {
            Draw();
            currentFlags = flags & ~0x04u;
            flags = currentFlags;
        } else if ((currentFlags & 0x08) != 0) {
            Draw();
            currentFlags = flags & ~0x08u;
            flags = currentFlags;
        }

        if ((flags & 0x01) != 0) {
            timer -= deltaSeconds;
            if (timer <= 0.0) {
                SetVisible(0);
            }
        }
    } else if ((currentFlags & 0x02) != 0) {
        if ((currentFlags & 0x04) != 0) {
            DrawBase();
            flags &= ~0x04u;
        } else if ((currentFlags & 0x08) != 0) {
            DrawBase();
            flags &= ~0x08u;
        }
    }
}

/**
 * Recovered original helper slot with no standalone HudUiElement retail function.
 * Binary Ninja vtables for HudUiElement/HudUiZrdWidget/HudUiPanel and
 * HudUiNumericTextInput place a one-argument no-op provider target at slot
 * +0x28 between Update and GetBoundsRectOrNull.
 * Purpose: preserve the source-faithful HudUiElement virtual order used by
 * retail input/update dispatch without recreating table data under src/.
 */
void HudUiElement::OnUpdateIdle(float) { }

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduielement-settimer
 * @recoil-artifact defines .text recoil:function:0x4b4280: HudUiElement::SetTimer.
 * @recoil-match byte
 *
 * Purpose: set the element timer and update the timed-visible flag state.
 */
void HudUiElement::SetTimer(float duration)
{
    timer = duration;

    if (duration >= 0.0f) {
        flags |= 0x01u;
    } else {
        flags = (flags & ~0x01u) | 0x10u;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduielement-gettextrect
 * @recoil-artifact defines .text recoil:function:0x4b42c0: HudUiElement::GetTextRect.
 * @recoil-match byte
 *
 * Purpose: fill a degenerate rectangle from the element position.
 * Binary Ninja: 0x4b42c0 dispatches the HudUiElement virtual GetCenterX and GetCenterY
 * methods from the base text-rectangle slot, then writes right/left and
 * bottom/top in that order.
 */
void HudUiElement::GetTextRect(HudUiRect* outRect)
{
    const int rectX = GetCenterX();
    outRect->right = rectX;
    outRect->left = rectX;

    const int rectY = GetCenterY();
    outRect->bottom = rectY;
    outRect->top = rectY;
}
