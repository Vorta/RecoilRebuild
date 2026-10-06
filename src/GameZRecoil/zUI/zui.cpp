#include "recoil/Mfc42Abi.h"

#include "GameZRecoil/zHud/zhud_ui.h"

#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/briefing.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud_ui_net_game_setup.h"
#include "Battlesport/player.h"
#include "Battlesport/recoil_state_credits.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zClass/cls_stubs.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zVideo/zvid_fx_pass3.h"

#include "Battlesport/turret.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zUtil/zbd.h"

#include <cctype>
#include <cstdarg>
#include <math.h>
#include <new>
#if defined(_MSC_VER) && _MSC_VER < 1200
#include <vector>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/**
 * Purpose: assign the existing panel and its seven flash-state fields without
 * changing its dynamic type. Retail 0x4bc3a0 uses the panel assignment body;
 * the separate range loop at 0x4bc320 is the provider's std::copy expansion.
 */
HudUiTransitionTextPanel& HudUiTransitionTextPanel::operator=(const HudUiTransitionTextPanel& source)
{
    HudUiPanel::operator=(source);
    flashCountdown = source.flashCountdown;
    flashResetValue = source.flashResetValue;
    flashAltColor0 = source.flashAltColor0;
    flashAltColor1 = source.flashAltColor1;
    flashEnabled = source.flashEnabled;
    flashMode = source.flashMode;
    flashDirectionSign = source.flashDirectionSign;
    return *this;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicompositepanelentry-copy-constructor
 * @recoil-artifact defines .text recoil:function:0x4bc410: HudUiTransitionTextPanel::HudUiTransitionTextPanel(const HudUiTransitionTextPanel &).
 * @recoil-match byte
 *
 * Purpose: copy-construct one composite-panel entry from another entry.
 */
HudUiTransitionTextPanel::HudUiTransitionTextPanel(const HudUiTransitionTextPanel& source)
    : HudUiPanel(source)
    , flashCountdown(source.flashCountdown)
    , flashResetValue(source.flashResetValue)
    , flashAltColor0(source.flashAltColor0)
    , flashAltColor1(source.flashAltColor1)
    , flashEnabled(source.flashEnabled)
    , flashMode(source.flashMode)
    , flashDirectionSign(source.flashDirectionSign)
{
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicircle-huduicircle
 * @recoil-artifact defines .text recoil:function:0x4bc480: HudUiCircle::HudUiCircle.
 * @recoil-match byte
 *
 * Purpose: initialize a circle element's position, radius, and color.
 *
 * Evidence: BN assembly calls the HudUiElement base constructor at object
 * offset zero, installs the derived circle C++ dispatch identity, stores
 * radius at 0x34, stores
 * radiusSquared as radius * radius at 0x38, stores color565 at 0x3c, and
 * returns this.
 */
HudUiCircle::HudUiCircle(int x, int y, int circleRadius, unsigned int circleColor565)
    : HudUiElement(x, y)
{
    radius = circleRadius;
    const unsigned int radiusBits = (unsigned int)(circleRadius);
    radiusSquared = (int)(radiusBits * radiusBits);
    color565 = circleColor565;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicircle-draw
 * @recoil-artifact defines .text recoil:function:0x4bc4c0: HudUiCircle::Draw.
 * @recoil-match byte
 *
 * Purpose: redraw the inherited base and circle outline for a dirty circle element.
 */
void HudUiCircle::Draw()
{
    DrawBase();
    zRndrDrawCircleOutline16Framebuffer(x, y, radius, color565, 0);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicircle-hittestcore
 * @recoil-artifact defines .text recoil:function:0x4bc4e0: HudUiCircle::HitTestCore.
 * @recoil-match byte
 *
 * Purpose: compare a point's squared distance against the circle radius.
 */
unsigned char HudUiCircle::HitTestCore(int px, int py)
{
    const unsigned int dx = (unsigned int)(px) - (unsigned int)(x);
    const unsigned int dy = (unsigned int)(py) - (unsigned int)(y);
    const unsigned int distanceSquared = dx * dx + dy * dy;
    return (int)(distanceSquared) < radiusSquared ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcontainer-huduibackgroundcontainer-0x4bc510
 * @recoil-artifact defines .text recoil:function:0x4bc510: HudUiBackgroundContainer::HudUiBackgroundContainer.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiBackgroundContainer::HudUiBackgroundContainer.
 */
HudUiBackgroundContainer::HudUiBackgroundContainer(int initFlag)
    : HudUiContainer()
{
    captureTransitionMask = initFlag;
    inputFocusElement = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcontainer-huduibackgroundcontainer-0x4bc540
 * @recoil-artifact defines .text recoil:function:0x4bc540: HudUiBackgroundContainer::~HudUiBackgroundContainer.
 * @recoil-match byte
 *
 * Purpose: Restores the background-container base state and tears down the inherited container.
 */
HudUiBackgroundContainer::~HudUiBackgroundContainer() { }

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zui.hud-ui-background-container-set-enabled
 * @recoil-artifact defines .text recoil:logical-function:0x42ee40:hud-ui-background-container-set-enabled: HudUiBackgroundContainer::SetEnabled.
 * Purpose: Store whether the background container participates in HUD updates.
 */
void HudUiBackgroundContainer::SetEnabled(int enabledValue)
{
    enabled = enabledValue;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcontainer-setinputfocus
 * @recoil-artifact defines .text recoil:function:0x4bc550: HudUiBackgroundContainer::SetInputFocus.
 * @recoil-match byte
 *
 * Purpose: Stores the child element that currently owns background input focus.
 */
void HudUiBackgroundContainer::SetInputFocus(HudUiElement* element)
{
    inputFocusElement = element;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcontainer-getinputfocus
 * @recoil-artifact defines .text recoil:function:0x4bc560: HudUiBackgroundContainer::GetInputFocus.
 * @recoil-match byte
 *
 * Purpose: Returns the child element that currently owns background input focus.
 */
HudUiElement* HudUiBackgroundContainer::GetInputFocus()
{
    return inputFocusElement;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcontainer-updateall
 * @recoil-artifact defines .text recoil:function:0x4bc570: HudUiBackgroundContainer::UpdateAll.
 *
 *
 * Purpose: Dispatch background mouse input, update child widgets, and move the focus cursor.
 */
void HudUiBackgroundContainer::UpdateAll(float deltaSeconds)
{
    if (enabled == 0) {
        return;
    }

    HudUiBackground* const background = (HudUiBackground*)this;

    memcpy(&mouseState, zInput::MouseGetStateSnapshotPtr(), sizeof(mouseState));

    for (HudUiElement* widget = childHead; widget != 0; widget = widget->next) {
        const int hit = widget->HitTest(mouseState.cursorClientX, mouseState.cursorClientY);
        const int hovered = hit == 1 ? 1 : 0;

        if (widget->ShouldHandleInput(background, hovered) != 0) {
            if ((mouseState.button2Transition & 4) != 0 && (widget->state & 2) == 2) {
                widget->state = (unsigned short)(widget->state & 0xfffd);
                widget->OnEndCapture();
            }

            if (hovered != 0) {
                if ((widget->state & 1) == 0) {
                    widget->state = (unsigned short)(widget->state | 1);
                    widget->ShowPreview();
                } else {
                    widget->OnHoverRepeat();
                }

                if ((mouseState.button1Transition & captureTransitionMask) != 0 && (widget->state & 2) == 0) {
                    widget->state = (unsigned short)(widget->state | 2);
                    widget->OnBeginCapture();
                }

                if ((mouseState.button1Transition & 4) != 0) {
                    widget->OnActivate();
                }

                if ((mouseState.button2Transition & 4) != 0) {
                    widget->OnClearBinding();
                }

                if ((mouseState.button1Transition & 3) != 0) {
                    widget->OnPointerButtonState(mouseState.cursorClientX, mouseState.cursorClientY);
                }

                if ((mouseState.button1Transition & 4) != 0 && (widget->state & 2) == 2) {
                    widget->OnCapturedPrimaryRelease();
                }
            } else {
                if ((mouseState.button1Transition & captureTransitionMask) != 0 && (widget->state & 2) == 2) {
                    widget->state = (unsigned short)(widget->state & 0xfffd);
                    widget->OnEndCapture();
                }

                if ((mouseState.button1Transition & 3) != 0 && (widget->state & 2) == 2) {
                    widget->OnPointerButtonState(mouseState.cursorClientX, mouseState.cursorClientY);
                }

                if ((widget->state & 1) == 1) {
                    widget->state = (unsigned short)(widget->state & 0xfffe);
                    widget->HidePreview();
                }
            }
        }

        widget->AfterInputUpdate(background, hovered);
    }

    HudUiElement* const focusBeforeUpdate = inputFocusElement;
    if (focusBeforeUpdate != 0) {
        focusBeforeUpdate->DrawBase();
    }

    HudUiContainer::UpdateAll(deltaSeconds);

    HudUiElement* const focusAfterUpdate = inputFocusElement;
    if (focusAfterUpdate != 0) {
        focusAfterUpdate->SetPos(mouseState.cursorClientX, mouseState.cursorClientY);
        focusAfterUpdate->Update(deltaSeconds);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudui-setinvalidatemode
 * @recoil-artifact defines .text recoil:function:0x4bc760: HudUi::SetInvalidateMode.
 * @recoil-match byte
 *
 * Purpose: apply the recovered HUD state change handled by HudUi::SetInvalidateMode.
 */
void __fastcall HudUi::SetInvalidateMode(int mode)
{
    g_HudUi_InvalidateMask = mode != 0 ? 0x0c : 0x04;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicontainer-huduicontainer
 * @recoil-artifact defines .text recoil:function:0x4bc780: HudUiContainer::HudUiContainer.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiContainer::HudUiContainer.
 */
HudUiContainer::HudUiContainer()
{
    HudUiContainer* const container = this;
    container->SetEnabled(0);
    childHead = 0;
    childTail = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicontainer-destructor
 * @recoil-artifact defines .text recoil:function:0x4bc7b0: HudUiContainer::~HudUiContainer.
 * @recoil-match byte
 *
 * Purpose: restore the container vptr during ordinary C++ teardown.
 */
HudUiContainer::~HudUiContainer() { }

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicontainer-addchild
 * @recoil-artifact defines .text recoil:function:0x4bc7c0: HudUiContainer::AddChild.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiContainer::AddChild.
 */
int HudUiContainer::AddChild(HudUiElement* child)
{
    if (childHead != 0 && childTail != 0) {
        childTail->next = child;
        childTail = child;
    } else {
        childTail = child;
        childHead = child;
    }

    child->next = 0;
    child->parent = this;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicontainer-findchildwithprev
 * @recoil-artifact defines .text recoil:function:0x4bc810: HudUiContainer::FindChildWithPrev.
 * @recoil-match byte
 *
 * Purpose: find a child in the container list and optionally report the
 * previous sibling.
 */
int HudUiContainer::FindChildWithPrev(HudUiElement* child, HudUiElement** previousOut)
{
    if (childHead == 0) {
        return 0;
    }

    if (child == childHead) {
        *previousOut = 0;
        return 1;
    }

    for (HudUiElement* previous = childHead; previous != 0; previous = previous->next) {
        HudUiElement* const current = previous->next;
        if (current == child) {
            if (previousOut != 0) {
                *previousOut = previous;
            }

            return 1;
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicontainer-removechild
 * @recoil-artifact defines .text recoil:function:0x4bc860: HudUiContainer::RemoveChild.
 * @recoil-match byte
 *
 * Purpose: unlink a child from this container and clear the child's owner
 * links.
 */
int HudUiContainer::RemoveChild(HudUiElement* child)
{
    HudUiElement* previous;
    if (FindChildWithPrev(child, &previous) != 0) {
        if (previous != 0) {
            previous->next = child->next;
            if (child == childTail) {
                childTail = previous;
            }
        } else {
            childHead = child->next;
            if (child == childTail) {
                childTail = childHead;
            }
        }

        child->next = 0;
        child->parent = 0;
        return 1;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicontainer-setchildflags
 * @recoil-artifact defines .text recoil:function:0x4bc8d0: HudUiContainer::SetChildFlags.
 * @recoil-match byte
 *
 * Purpose: apply a shared child flag mask to every child while preserving each
 * child's hidden/disabled bit 0x10.
 *
 * Evidence: BN assembly at 0x4bc8d0 walks HudUiContainer::childHead through
 * HudUiElement::next, writes childFlags directly when bit 0x10 is clear, and
 * writes childFlags|0x10 when the existing child flags preserve that bit.
 */
void HudUiContainer::SetChildFlags(unsigned int childFlags)
{
    for (HudUiElement* child = childHead; child != 0; child = child->next) {
        const unsigned int invertedFlags = ~child->flags;
        if ((invertedFlags & 0x10u) != 0) {
            child->flags = childFlags;
        } else {
            child->flags = childFlags | 0x10u;
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicontainer-updateall
 * @recoil-artifact defines .text recoil:function:0x4bc900: HudUiContainer::UpdateAll.
 * @recoil-match byte
 *
 * Purpose: Dispatch per-frame updates to every child in an enabled container.
 */
void HudUiContainer::UpdateAll(float deltaSeconds)
{
    if (enabled == 0) {
        return;
    }

    for (HudUiElement* child = childHead; child != 0; child = child->next) {
        child->Update(deltaSeconds);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitransitiontextpanel-resetflashstate
 * @recoil-artifact defines .text recoil:function:0x4bc930: HudUiTransitionTextPanel::ResetFlashState.
 * @recoil-match byte
 *
 * Purpose: enable flash state, update a positive flash rate to its half-period,
 * reset the countdown from that period, and restore forward flash direction.
 *
 * Evidence: BN assembly at 0x4bc930 writes flashEnabled, conditionally stores
 * flashRate*0.5 into flashResetValue when flashRate is positive, copies
 * flashResetValue into flashCountdown, and writes flashDirectionSign = 1.
 */
void HudUiTransitionTextPanel::ResetFlashState(float flashRate)
{
    flashEnabled = 1;
    if (flashRate > 0.0f) {
        flashResetValue = flashRate * 0.5f;
    }

    flashDirectionSign = 1;
    memcpy(&flashCountdown, &flashResetValue, sizeof(flashCountdown));
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitransitiontextpanel-setflashrate
 * @recoil-artifact defines .text recoil:function:0x4bc980: HudUiTransitionTextPanel::SetFlashRate.
 * @recoil-match byte
 *
 * Purpose: enter rate-only flashing by resetting flash state unless the panel
 * is already in rate-only flash mode.
 *
 * Evidence: BN assembly at 0x4bc980 returns when flashMode is 1; otherwise it
 * calls ResetFlashState(flashRate) and stores flashMode = 1.
 */
void HudUiTransitionTextPanel::SetFlashRate(float flashRate)
{
    if (flashMode == 1) {
        return;
    }

    ResetFlashState(flashRate);
    flashMode = 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitransitiontextpanel-setflashcolorandrate
 * @recoil-artifact defines .text recoil:function:0x4bc9b0: HudUiTransitionTextPanel::SetFlashColorAndRate.
 * @recoil-match byte
 *
 * Purpose: enter color-flash mode and store the alternate flash text colors.
 *
 * Evidence: BN assembly returns when flashMode is already color-flash mode,
 * calls ResetFlashState, writes flashMode = 2, and stores the same alternate
 * color into both flash color fields.
 */
void HudUiTransitionTextPanel::SetFlashColorAndRate(unsigned int flashColor, float flashRate)
{
    if (flashMode == 2) {
        return;
    }

    ResetFlashState(flashRate);
    flashMode = 2;
    flashAltColor0 = flashColor;
    flashAltColor1 = flashColor;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitransitiontextpanel-update
 * @recoil-artifact defines .text recoil:function:0x4bc9f0: HudUiTransitionTextPanel::Update.
 * @recoil-match byte
 *
 * Purpose: update timed visibility and flash-color state before drawing the panel.
 *
 * Evidence: BN assembly subtracts delta time from the base timer and flash
 * countdown, hides timed-out panels through the visibility slot, toggles
 * flashDirectionSign/textDirty, swaps text colors for color-flash modes, and
 * calls HudUiPanel::Draw on visible refresh paths.
 */
void HudUiTransitionTextPanel::Update(float deltaSeconds)
{
    const unsigned int elementFlags = flags;
    if (((~elementFlags) & 0x10u) == 0) {
        return;
    }

    if ((elementFlags & 1u) != 0) {
        timer -= deltaSeconds;
        if (timer <= 0.0) {
            SetVisible(0);
        }
    }

    if (flashEnabled == 0 || ((~flags) & 0x10u) == 0) {
        HudUiPanel::Draw();
        return;
    }

    flashCountdown -= deltaSeconds;
    switch (flashMode) {
    case 0:
        HudUiPanel::Draw();
    case 1:
        if (flashCountdown < 0.0) {
            flashCountdown += flashResetValue;
            textDirty = 1;
            flashDirectionSign = -flashDirectionSign;
        }

        if (flashDirectionSign == 1) {
            HudUiPanel::Draw();
        }
        return;

    case 2:
    case 3:
        if (flashCountdown < 0.0) {
            flashDirectionSign = -flashDirectionSign;
            flashCountdown = flashResetValue;
            textDirty = 1;

            const unsigned int oldTextColor0 = textColor0;
            const unsigned int oldTextColor1 = textColor1;
            textColor0 = (unsigned int)(flashAltColor0);
            textColor1 = (unsigned int)(flashAltColor1);
            flashAltColor0 = (int)(oldTextColor0);
            flashAltColor1 = (int)(oldTextColor1);
        }

        HudUiPanel::Draw();
        return;

    default:
        HudUiPanel::Draw();
        return;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextlabel-huduitextlabel-0x4bcb50
 * @recoil-artifact defines .text recoil:function:0x4bcb50: HudUiTextLabel::HudUiTextLabel.
 * @recoil-match byte
 *
 * Purpose: initialize label text, position, font handle, and alignment state.
 */
HudUiTextLabel::HudUiTextLabel(const char* text, int initX, int initY, int flags)
    : HudUiElement(0, 0)
{
    centerText = 0;
    SetTextFmt(text);
    x = initX;
    y = initY;
    ((HudUiElement*)(this))->Invalidate();
    fontHandle = flags;
    ((HudUiElement*)(this))->Invalidate();
    alignMode = 0;
}

/**
 * Original helper; no standalone retail function exists. Observed in the
 * HudUiTextLabel method cluster as the caller-owned storage wrapper around
 * the 0x4bcb50 address-backed constructor.
 * Purpose: construct a text label in caller-provided storage and return it.
 */
HudUiTextLabel* HudUiTextLabel::ConstructorWithPosAndFlags(const char* text, int initX, int initY, int flags)
{
    new (this) HudUiTextLabel(text, initX, initY, flags);
    return this;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextlabel-huduitextlabel-0x4bcbe0
 * @recoil-artifact defines .text recoil:function:0x4bcbe0: HudUiTextLabel::HudUiTextLabel(const HudUiTextLabel &).
 * @recoil-match byte
 *
 * Purpose: Copy-construct a text label from an existing label, including its text buffer.
 */
HudUiTextLabel::HudUiTextLabel(const HudUiTextLabel& source)
    : HudUiElement(source)
{
    strncpy(textBuffer, source.textBuffer, sizeof(textBuffer));
    fontHandle = source.fontHandle;
    centerText = source.centerText;
    centerBoundsLeft = source.centerBoundsLeft;
    centerBoundsRight = source.centerBoundsRight;
    alignMode = source.alignMode;
}

/**
 * Purpose: Initialize this text label by copying the source label state.
 */
HudUiTextLabel& HudUiTextLabel::operator=(const HudUiTextLabel& source)
{
    HudUiElement::operator=(source);
    strncpy(textBuffer, source.textBuffer, sizeof(textBuffer));
    fontHandle = source.fontHandle;
    centerText = source.centerText;
    centerBoundsLeft = source.centerBoundsLeft;
    centerBoundsRight = source.centerBoundsRight;
    alignMode = source.alignMode;
    return *this;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextlabel-settextfmt
 * @recoil-artifact defines .text recoil:function:0x4bccf0: HudUiTextLabel::SetTextFmt.
 * @recoil-match byte
 *
 * Purpose: format label text, refresh centered extents when needed, and
 * invalidate the element.
 */
void __cdecl HudUiTextLabel::SetTextFmt(const char* format, ...)
{
    if (format == 0) {
        memset(textBuffer, 0, sizeof(textBuffer));
        return;
    }

    va_list args;
    va_start(args, format);
    vsprintf(textBuffer, format, args);
    va_end(args);

    if (centerText != 0) {
        UpdateTextExtents();
    }

    Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextlabel-setbltsourceandcliprect
 * @recoil-artifact defines .text recoil:function:0x4bcd40: HudUiTextLabel::SetBltSourceAndClipRect.
 * @recoil-match byte
 *
 * Purpose: update the label image and optional clipping rectangle, then
 * invalidate its cached display through the inherited virtual operation.
 * Retail constructor 0x4bcb50 installs the table at 0x4d3c70, whose +0x18
 * slot selects this override. Panel and its derived classes inherit it.
 */
void HudUiTextLabel::SetBltSourceAndClipRect(void* bltSourceOrNull, const HudUiRect* rectOrNull)
{
    bltSource = bltSourceOrNull;
    if (rectOrNull != 0) {
        clipRect = *rectOrNull;
    }

    Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextlabel-rebuildtextbounds
 * @recoil-artifact defines .text recoil:function:0x4bcd80: HudUiTextLabel::RebuildTextBounds.
 * @recoil-match byte
 *
 * Purpose: rebuild the clip rectangle from the current formatted text size.
 */
void HudUiTextLabel::RebuildTextBounds()
{
    int widthPx;
    int lineAdvance;
    zImage_Font::MeasureString(textBuffer, fontHandle, &widthPx, &lineAdvance);
    clipRect.right = clipRect.left + widthPx;
    clipRect.bottom = clipRect.top + lineAdvance;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextlabel-measuretextwidth
 * @recoil-artifact defines .text recoil:function:0x4bcdc0: HudUiTextLabel::MeasureTextWidth.
 * @recoil-match byte
 *
 * Purpose: return the measured pixel width of the current label text.
 */
int HudUiTextLabel::MeasureTextWidth()
{
    int widthPx;
    int lineAdvance;
    zImage_Font::MeasureString(textBuffer, fontHandle, &widthPx, &lineAdvance);
    return widthPx;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextlabel-updatetextextents
 * @recoil-artifact defines .text recoil:function:0x4bcdf0: HudUiTextLabel::UpdateTextExtents.
 * @recoil-match byte
 *
 * Purpose: recenter the label inside its stored bounds and refresh clip
 * extents when a blit source is active.
 */
void HudUiTextLabel::UpdateTextExtents()
{
    const int widthPx = MeasureTextWidth();
    x = centerBoundsLeft + (centerBoundsRight - widthPx - centerBoundsLeft) / 2;

    if (bltSource != 0) {
        clipRect.top = y;
        clipRect.left = x;
        RebuildTextBounds();
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextlabel-ondraw
 * @recoil-artifact defines .text recoil:function:0x4bce30: HudUiTextLabel::OnDraw.
 * @recoil-match byte
 *
 * Purpose: draw non-empty label text with the recovered alignment handling.
 */
void HudUiTextLabel::Draw()
{
    DrawBase();

    if (textBuffer[0] == '\0') {
        return;
    }

    if (alignMode != 0) {
        int xOffset = MeasureTextWidth();
        if (alignMode == 1) {
            xOffset >>= 1;
        }

        x -= xOffset;
        zImage_Font::BlitStringToActiveTarget(textBuffer, x, y, fontHandle);
        x += xOffset;
        return;
    }

    zImage_Font::BlitStringToActiveTarget(textBuffer, x, y, fontHandle);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextlabel-hittest
 * @recoil-artifact defines .text recoil:function:0x4bcea0: HudUiTextLabel::HitTest.
 * @recoil-match byte
 *
 * Purpose: test coordinates against the visible text bounds unless input is
 * disabled.
 */
int HudUiTextLabel::HitTest(int px, int py)
{
    int hit = (~flags & 0x10u) != 0 && x <= px && y <= py;
    if (hit) {
        int textWidth;
        int lineAdvance;
        zImage_Font::MeasureString(textBuffer, fontHandle, &textWidth, &lineAdvance);
        hit = px <= x + textWidth && py <= y + lineAdvance;
    }

    return hit;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibar-huduibar
 * @recoil-artifact defines .text recoil:function:0x4bcf20: HudUiBar::HudUiBar.
 * @recoil-match byte
 *
 * Purpose: Constructs the HUD element base, clears bar point storage, and marks the bar dirty.
 */
HudUiBar::HudUiBar()
    : HudUiElement(0, 0)
{
    drawVertexCount = 0;
    memset(points, 0, sizeof(points));
    Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibar-setpointxy
 * @recoil-artifact defines .text recoil:function:0x4bcf80: HudUiBar::SetPointXY.
 * @recoil-match byte
 *
 * Binary Ninja evidence: bounds-checks pointIndex against the 21-element point
 * array, writes the HudUiBarPoint x/y fields, raises drawVertexCount, dispatches
 * SetPos for point zero, and always invalidates the element.
 * Purpose: Update one bar point and keep the element position/count state dirty.
 */
void HudUiBar::SetPointXY(int pointIndex, float x, float y)
{
    if (pointIndex < 21 && pointIndex > -1) {
        points[pointIndex].x = x;
        points[pointIndex].y = y;

        if (drawVertexCount < pointIndex + 1) {
            drawVertexCount = pointIndex + 1;
        }

        if (pointIndex == 0) {
            SetPos((int)(x), (int)(y));
        }
    }

    Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibar-draw
 * @recoil-artifact defines .text recoil:function:0x4bcff0: HudUiBar::Draw.
 * @recoil-match byte
 *
 * Binary Ninja evidence: dispatches the base DrawBase method, reads
 * drawVertexCount, and calls zRndr::RasterizePoly with points and drawParam
 * only when at least one vertex is active.
 * Purpose: Draw the bar base and rasterize the populated point list.
 */
void HudUiBar::Draw()
{
    DrawBase();
    if (drawVertexCount != 0) {
        zRndrRasterizePoly((zVec3*)(points), drawVertexCount, drawParam);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitopmessagestack-constructor
 * @recoil-artifact defines .text recoil:function:0x4bd020: HudUiTopMessageStack::HudUiTopMessageStack.
 *
 *
 * Purpose: construct the top-message four-line stack and configure ascending rows.
 * Retail constructs the container and panel array in this body, before the
 * derived table write and row loop; there is no second constructor wrapper.
 */
HudUiTopMessageStack::HudUiTopMessageStack()
{
    HudUiPanel* panel = lines;
    for (int y = 0x1e; y < 0x66; y += 0x12, ++panel) {
        HudUiElement* const element = (HudUiElement*)(panel);
        AddChild(element);
        panel->SetFont(g_HudFontName_Arial, 0x0d, 0x258, 7, 0, 0, 2);
        panel->SetShadow(1, -1, -1);
        panel->SetTextAlignment(1);
        element->SetPos(0x140, y);
        element->SetVisible(0);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextstack4-setfontall
 * @recoil-artifact defines .text recoil:function:0x4bd110: HudUiTextStack4::SetFontAll.
 * @recoil-match byte
 *
 * Purpose: apply one font definition to every row in the four-line stack.
 */
void HudUiTextStack4::SetFontAll(const char* faceName, int height, int weight, int width)
{
    for (HudUiPanel* panel = &lines[3]; panel >= lines; --panel) {
        panel->SetFont(faceName, height, weight, width, 0, 0, 2);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextstack4-pushline
 * @recoil-artifact defines .text recoil:function:0x4bd160: HudUiTextStack4::PushLine.
 * @recoil-match byte
 *
 * Purpose: push a visible timed message into the four-row text stack.
 */
HudUiPanel* HudUiTextStack4::PushLine(const char* message, float duration)
{
    SetEnabled(1);

    if (((~((HudUiElement*)(&lines[0]))->flags) & 0x10u) != 0 && strcmp(message, lines[0].GetLastTextPtr()) != 0) {
        for (HudUiPanel* source = &lines[2]; source >= &lines[0]; --source) {
            HudUiPanel* const dest = source + 1;
            HudUiElement* const sourceElement = (HudUiElement*)(source);

            if (((~sourceElement->flags) & 0x10u) != 0) {
                source->SetVisible(0);
                const float remaining = source->timer;
                dest->SetTimer(remaining);
                dest->SetTextFmt(source->GetLastTextPtr());
                dest->SetTextColorsAndMarkDirty(source->textColor0, source->textColor1);
                ((HudUiElement*)(dest))->SetVisible(1);
            }
        }
    }

    ((HudUiElement*)(&lines[0]))->SetTimer(duration);
    lines[0].SetTextFmt("%s", message);
    ((HudUiElement*)(&lines[0]))->SetVisible(1);
    return &lines[0];
}

namespace HudUi {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-pushtopmessageline
 * @recoil-artifact defines .text recoil:function:0x4bd280: HudUi::PushTopMessageLine.
 * @recoil-match byte
 *
 * Purpose: push a message directly into the global top-message stack.
 */
void __fastcall PushTopMessageLine(const char* message, float duration)
{
    g_HudUiTopMessageStack->PushLine(message, duration);
}
} // namespace HudUi

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextstack4-clear
 * @recoil-artifact defines .text recoil:function:0x4bd2a0: HudUiTextStack4::Clear.
 * @recoil-match byte
 *
 * Purpose: clear text and hide every row in the four-line stack.
 */
void HudUiTextStack4::Clear()
{
    for (int index = 0; index < 4; ++index) {
        HudUiPanel* const panel = &lines[index];
        panel->SetTextFmt("");
        panel->SetVisible(0);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduichatmessagestack-constructor
 * @recoil-artifact defines .text recoil:function:0x4bd2d0: HudUiChatMessageStack::HudUiChatMessageStack.
 *
 *
 * Purpose: construct the chat-message four-line stack and configure descending rows.
 * As in the top-message constructor, VC5 owns base/member construction and
 * exception cleanup; the row configuration belongs to this constructor body.
 */
HudUiChatMessageStack::HudUiChatMessageStack()
{
    HudUiPanel* panel = lines;
    for (int y = 0x159; y > 0x111; y -= 0x12, ++panel) {
        HudUiElement* const element = (HudUiElement*)(panel);
        AddChild(element);
        panel->SetTextColorsAndMarkDirty(0x00996a00, 0x0095c7ff);
        panel->SetFont(g_HudFontName_Arial, 0x0a, 0x1f4, 6, 0, 0, 2);
        panel->SetShadow(1, -1, -1);
        panel->SetTextAlignment(1);
        element->SetPos(0x140, y);
        element->SetVisible(0);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextstack4-settextcolors
 * @recoil-artifact defines .text recoil:function:0x4bd3d0: HudUiTextStack4::SetTextColors.
 * @recoil-match byte
 *
 * Purpose: assign both text colors to every row in the four-line stack.
 */
void HudUiTextStack4::SetTextColors(unsigned int color0, unsigned int color1)
{
    for (HudUiPanel* panel = &lines[3]; panel >= lines; --panel) {
        panel->textColor0 = color0;
        panel->textColor1 = color1;
        panel->textDirty = 1;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextstack4-setxall
 * @recoil-artifact defines .text recoil:function:0x4bd410: HudUiTextStack4::SetXAll.
 * @recoil-match byte
 *
 * Purpose: move every row in the four-line stack to a shared x position.
 */
void HudUiTextStack4::SetXAll(int newX)
{
    for (int index = 0; index < 4; ++index) {
        HudUiPanel* const panel = &lines[index];
        panel->SetX(newX);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextstack4-setydescending
 * @recoil-artifact defines .text recoil:function:0x4bd440: HudUiTextStack4::SetYDescending.
 * @recoil-match byte
 *
 * Purpose: place every row in the four-line stack at descending y positions.
 */
void HudUiTextStack4::SetYDescending(int yStart)
{
    int y = yStart;
    for (int index = 0; index < 4; ++index) {
        HudUiPanel* const panel = &lines[index];
        panel->SetY(y);
        y -= 0x12;
    }
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed
 * HudUiElement::GetCenterY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnHoverRepeat.
 */
void HudUiElement::OnHoverRepeat() { }

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: return the recovered HUD value exposed by HudUiElement::GetBoundsRectOrNull.
 */
HudUiRect* HudUiElement::GetBoundsRectOrNull()
{
    return 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnActivate.
 */
void HudUiElement::OnActivate() { }

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnClearBinding.
 */
void HudUiElement::OnClearBinding() { }

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::ShowPreview.
 */
void HudUiElement::ShowPreview() { }

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::HidePreview.
 */
void HudUiElement::HidePreview() { }

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnBeginCapture.
 */
void HudUiElement::OnBeginCapture() { }

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnEndCapture.
 */
void HudUiElement::OnEndCapture() { }

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnPointerButtonState.
 */
void HudUiElement::OnPointerButtonState(int, int) { }

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnCapturedPrimaryRelease.
 */
void HudUiElement::OnCapturedPrimaryRelease() { }

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::ShouldHandleInput.
 */
int HudUiElement::ShouldHandleInput(HudUiBackground*, int)
{
    return 1;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::AfterInputUpdate.
 */
void HudUiElement::AfterInputUpdate(HudUiBackground*, int) { }

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::HitTest.
 */
int HudUiElement::HitTest(int px, int py)
{
    return HitTestTrue(px, py);
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::EnableWordWrapWithRect.
 */
void HudUiElement::EnableWordWrapWithRect(const HudUiRect*) { }

/**
 * Original-source helper evidence: no standalone retail function exists.
 * No standalone retail function has been identified; restored as the default
 * HudUiZrdWidget post-load virtual observed as the ZRD widget tail slot before
 * numeric input adds raw-key virtuals.
 * Purpose: keep ZRD loading ownership on HudUiZrdWidget.
 */
void HudUiZrdWidget::PostLoadFromZrd() { }

/**
 * Purpose: reset the scrolling-text owner fade when the widget is activated;
 * retail source placement remains unresolved.
 */
void HudUiZrdScrollingText::OnActivate()
{
    OnActivateResetOwnerFade();
}

/**
 * Purpose: initialize the recovered HudUiWidget::Constructor state.
 */

/**
 * Purpose: initialize the recovered HudUiWidget::Constructor state.
 */
HudUiWidget* HudUiWidget::Constructor(unsigned int initAlignFlags)
{
    new (this) HudUiWidget(initAlignFlags);
    return this;
}

/**
 * Purpose: queue and clip one widget dirty rectangle before invalidating the widget.
 */

/**
 * Purpose: initialize the ZRD widget's base widget state, image/sound slots,
 * panel vectors, enabled mode, and initial invalidation state.
 */

/**
 * Purpose: initialize the recovered HudUiZrdWidget::Constructor state.
 */
HudUiZrdWidget* HudUiZrdWidget::Constructor()
{
    new (this) HudUiZrdWidget;
    return this;
}

/**
 * Purpose: bind a ZRD widget to its owner background, load images, sounds,
 * labels, flash settings, and initial clipping from the recovered ZRD section.
 */

/**
 * Purpose: scalar-delete an optional child widget through the recovered HudUiElement slot.
 */

/**
 * Purpose: release owned ZRD widget panels and alternate images before compiler-generated member cleanup.
 */

/**
 * No standalone retail function; source compatibility wrapper for recovered
 * HudUiZrdWidget cleanup callers that historically named the destructor body
 * DestructorCore in this reconstruction.
 * Purpose: release owned ZRD widget panels, alternate images, panel vectors, and the base widget.
 */
void HudUiZrdWidget::DestructorCore()
{
    this->~HudUiZrdWidget();
}

/**
 * Purpose: invalidate the widget and every base label panel owned by the ZRD widget.
 */

/**
 * Purpose: return the recovered HUD value exposed by HudUiZrdWidget::GetBoundsRectOrNull.
 */

/**
 * Purpose: switch the widget between normal and disabled image/label state.
 */

/**
 * Purpose: preserve the recovered HUD behavior for HudUiZrdWidget::ShowPreview.
 */

/**
 * Purpose: reset transition input and switch the widget from rollover to
 * activation visuals, labels, and sound.
 */

/**
 * Purpose: restore the widget's default image and normal label visibility after rollover preview.
 */

/**
 * Purpose: preserve the recovered HUD behavior for HudUiCheckToggleWidget::HudUiCheckToggleWidget.
 */

/**
 * the command-binding cleanup now instantiates the
 * canonical VC5 std::transform provider from zhud_ui.h.
 * vector::erase selects the canonical VC5 std::copy
 * provider rather than a hand-authored copy helper.
 * HudCmdBindingEntry's ordinary destructor owns the
 * display-string cleanup formerly modeled as a utility method.
 * retained as a legacy verification anchor pending
 * parent classification of the natural compiler-emitted contribution.
 * Purpose: retain precise provenance for the canonical command-binding
 * vector erase model used by modern non-VC5 builds of this consumer.
 */
#if !defined(_MSC_VER) || _MSC_VER >= 1200
/**
 * Original-source helper; no standalone retail function exists.
 * Restores the VC5 std::vector<HudCmdBindingEntry *>::erase(first,last)
 * dependency used by 0x40b680 after the caller destroys each pointed-to
 * binding entry. The caller-visible retail body invokes the vector erase
 * helper rather than only assigning end = begin.
 * Purpose: keep command-binding vector cleanup source-shaped as typed STL
 * storage while matching the retail caller's erase dependency.
 */
HudCmdBindingEntry** HudCmdBindingVector::erase(HudCmdBindingEntry** eraseFirst, HudCmdBindingEntry** eraseLast)
{
    HudCmdBindingEntry** write = eraseFirst;
    HudCmdBindingEntry** read = eraseLast;
    HudCmdBindingEntry** const oldEnd = last;
    if (read != oldEnd) {
        do {
            *write++ = *read++;
        } while (read != oldEnd);
    }
    ((StdPtrVector*)(this))->ClearNoOpDestroy((int*)(write), (int*)(oldEnd));
    last = write;
    return eraseFirst;
}
#endif

/**
 * Purpose: preserve the recovered HUD behavior for HudUiWidget::HudUiWidget.
 */

/**
 * Purpose: preserve the recovered HUD behavior for HudUiWidget::HitTest.
 */

/**
 * Purpose: draw pending widget dirty rectangles or the whole widget image after the base draw pass.
 */

/**
 * Purpose: release an owned widget image and clear the ownership bit.
 */

/**
 *
 * Purpose: install a borrowed widget image, clear ownership, invalidate the
 * widget, and return the borrowed image pointer.
 *
 * Evidence: BN assembly at 0x4b3e70 clears ownsImage at offset 0x34, stores
 * the incoming image at offset 0x3c, dispatches Invalidate through the
 * HudUiWidget class slot, and returns the image argument in eax.
 */

/**
 * Purpose: replace an owned widget image from a texture-directory path and invalidate the widget.
 */

/**
 * Purpose: apply the recovered HUD state change handled by HudUiWidget::SetPos.
 */

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x40e910 HudUiTriplet::InterpolateLayout callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnPrintableKey.
 */
void HudUiTextInput::OnPrintableKey(int key)
{
    InsertCharAtCursor(key);
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x40e910 HudUiTriplet::InterpolateLayout callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnAccept.
 */
void HudUiTextInput::OnAccept() { }

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b4370 HudUiTextInput::~HudUiTextInput callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnCancel.
 */
void HudUiTextInput::OnCancel() { }

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b4370 HudUiTextInput::~HudUiTextInput callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnBackspace.
 */
void HudUiTextInput::OnBackspace()
{
    BackspaceDeleteChar();
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b4370 HudUiTextInput::~HudUiTextInput callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnDeleteForward.
 */
void HudUiTextInput::OnDeleteForward()
{
    DeleteCharForward();
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b4370 HudUiTextInput::~HudUiTextInput callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnMoveCursorLeft.
 */
void HudUiTextInput::OnMoveCursorLeft()
{
    MoveCursorLeft();
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b4370 HudUiTextInput::~HudUiTextInput callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnMoveCursorRight.
 */
void HudUiTextInput::OnMoveCursorRight()
{
    MoveCursorRight();
}

/**
 * Source model note: Source-faithful helper recovered from address-backed callers in this
 * source file.
 * Purpose: run the recovered HudUiTextInput::~HudUiTextInput teardown path.
 */
void HudUiTextInput::OnOverflow() { }

/**
 * Current BN assembly resets the HudUiTextInput vptr, then deletes the owned
 * buffer. Modeling this as the authored C++ destructor preserves that class
 * cleanup shape without a hand-written table reset.
 * Purpose: tear down the base text-input buffer after derived text-input
 * cleanup has restored the base class identity.
 */

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextinput-destructorcore
 * @recoil-artifact defines .text recoil:function:0x4b4ab0: HudUiTextInput::DestructorCoreThunk.
 * Purpose: tail-call the recovered base text-input destructor from legacy
 * thunk entry points.
 */
void HudUiTextInput::DestructorCore()
{
    this->HudUiTextInput::~HudUiTextInput();
}

/**
 * Purpose: preserve the recovered HUD behavior for HudUiTextInput::AllocTextBuffer.
 */

/**
 * Purpose: preserve the recovered HUD behavior for HudUiTextInput::HudUiTextInput.
 */

/**
 * Purpose: apply the recovered HUD state change handled by HudUiTextInput::SetCursorPosition.
 */
HudUiTextInput* HudUiTextInput::Constructor(int bufferSize)
{
    new (this) HudUiTextInput(bufferSize);
    return this;
}

/**
 * Purpose: apply the recovered HUD state change handled by HudUiTextInput::SetCursorPosition.
 */

/**
 * Purpose: apply the recovered HUD state change handled by HudUiTextInput::SetContents.
 */

/**
 * Purpose: return the recovered HUD value exposed by HudUiTextInput::GetBuffer.
 */

/**
 * Purpose: make room in the edit buffer for inserted characters.
 */

/**
 * Purpose: close a deleted text range by shifting the following characters.
 */

/**
 * Purpose: delete the character at the cursor without moving the cursor.
 */

/**
 * Purpose: move the edit cursor one position left when possible.
 */

/**
 * Purpose: move the edit cursor one position right within the text contents.
 */

/**
 * Purpose: delete the character before the cursor and move the cursor back.
 */

/**
 * Purpose: insert one printable character at the current cursor position.
 */

/**
 * Binary Ninja shows the key action byte read from HudUiTextInput::keyActionMap
 * and dispatches action values 0 through 7 through the text-input virtual
 * methods; no authored globals are touched by this body.
 * Purpose: translate a raw key into the recovered text-input editing action.
 */

/**
 * Purpose: finish chat composition and relay the accepted text for sending;
 * retail source placement remains unresolved.
 */
void HudUiChatComposeTextInput::OnAccept()
{
    GameNet::EndChatComposeAndSendThunk();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zui.hud-ui-slot.constructor
 * @recoil-artifact defines .text recoil:function:0x40db20: HudUiSlot::HudUiSlot.
 * @recoil-match byte
 *
 * Historical explicit versus implicit spelling and emission TU remain unresolved.
 * Purpose: construct the element at (0, 0) and the two embedded slot widgets.
 */
HudUiSlot::HudUiSlot()
    : HudUiElement(0, 0)
{
}

/**
 * Purpose: preserve the recovered HUD behavior for HudUiPolyline::Draw.
 */
HudUiPolyline* HudUiPolyline::Constructor()
{
    new (this) HudUiPolyline;
    return this;
}

/**
 * Purpose: preserve the recovered HUD behavior for HudUiSliderBorder::HudUiSliderBorder.
 */

/**
 * Purpose: advance the recovered HUD update path for HudUiSliderBorder::Update.
 */
HudUiSliderBorder* HudUiSliderBorder::Constructor()
{
    new (this) HudUiSliderBorder;
    return this;
}

/**
 * Purpose: advance the recovered HUD update path for HudUiSliderBorder::Update.
 */

/**
 * Purpose: Stores slider border bounds and rebuilds the polyline outline points.
 */

/**
 * Purpose: Construct the ZRD widget base and owned numeric text-entry controls.
 */

/**
 * Purpose: preserve the recovered HUD behavior for HudUiNumericTextInput::AllocTextBuffer.
 */

/**
 * Purpose: return the recovered HUD value exposed by HudUiNumericTextInput::GetBuffer.
 */

/**
 * Purpose: Update the text-input buffer, mirror the visible label text, and invalidate the owning widget.
 */

/**
 * Purpose: advance the recovered HUD update path for HudUiNumericTextInput::UpdateCaptureUiAndClip.
 */

/**
 * Purpose: apply the recovered HUD state change handled by HudUiNumericTextInput::SetRawKeyboardCapture.
 */

/**
 * Purpose: handle the recovered HUD event path for HudUiNumericTextInput::OnActivate.
 */

/**
 * Compatibility wrapper for legacy native smoke call sites; the
 * address-backed retail bodies are the ordinary header-defined C++
 * destructor contributions.
 * Purpose: route compatibility calls through the recovered C++ destructor.
 */
void HudUiNumericTextInput::Destructor()
{
    this->HudUiNumericTextInput::~HudUiNumericTextInput();
}

/**
 * Purpose: preserve the recovered HUD behavior for HudUiNumericTextInput::RawKeyboardCallback.
 */

/**
 * Purpose: Show or hide the numeric text input, slider border, and first
 * label panel while returning the previous active state.
 */

/**
 * Purpose: handle the recovered HUD event path for HudUiNumericTextInput::OnRawKeyboardChar.
 */

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduipanel-constructordefaultthunk
 * @recoil-artifact defines .text recoil:function:0x4bd100: HudUiPanel::ConstructorDefaultThunk.
 * Purpose: preserve the recovered HUD behavior for HudUiPanel::ConstructorDefaultThunk.
 */
HudUiPanel* HudUiPanel::ConstructorDefaultThunk()
{
    return ConstructorDefault(0, 0, 0);
}

namespace HudScoreboard {

} // namespace HudScoreboard
