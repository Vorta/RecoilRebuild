#include "recoil/Mfc42Abi.h"

#include "GameZRecoil/zHud/zhud_ui.h"

#include "Battlesport/briefing.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud_ui_net_game_setup.h"
#include "Battlesport/recoil_state_credits.h"
#include "Battlesport/hud.h"
#include "Battlesport/player.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "GameZRecoil/Time/time.h"
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
#include "GameZRecoil/zVideo/zvid_fx_pass3.h"

#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "Battlesport/turret.h"
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


namespace {
/**
 * Recovered original inline/static helper with no standalone retail function.
 * Observed in text-stack constructors 0x4bd020 and 0x4bd2d0 after each
 * HudUiPanel row is constructed.
 * Purpose: attach and initialize one message-stack row with the recovered
 * panel font, shadow, alignment, position, and hidden state.
 */
void ConfigureTextStackLine(
    HudUiTextStack4 *stack,
    HudUiPanel *panel,
    int y,
    int fontSize,
    int fontWeight,
    int fontWidth
) {
    HudUiElement *const element = (HudUiElement *)(panel);
    stack->AddChild(element);
    panel->SetFont(
        g_HudFontName_Arial,
        fontSize,
        fontWeight,
        fontWidth,
        0,
        0,
        2
    );
    panel->SetShadow(
        1,
        -1,
        -1
    );
    panel->alignMode = 1;
    element->SetPos(
        0x140,
        y
    );
    element->SetVisible(0);
}

} // namespace

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicompositepanelentry-constructorcopyrange
 * @recoil-artifact defines .text recoil:function:0x4bc320: HudUiCompositePanelEntry::ConstructorCopyRange.
 * Purpose: copy-construct a range of composite-panel entries into destination
 * storage.
 */
HudUiCompositePanelEntry *__fastcall HudUiCompositePanelEntry::ConstructorCopyRange(
    const HudUiCompositePanelEntry *sourceBegin,
    const HudUiCompositePanelEntry *sourceEnd,
    HudUiCompositePanelEntry *destBegin
) {
    HudUiCompositePanelEntry *dest = destBegin;
    for (const HudUiCompositePanelEntry *source = sourceBegin; source != sourceEnd;
        ++source, ++dest) {
        dest->HudUiPanel::operator=(*source);
        dest->flashCountdown = source->flashCountdown;
        dest->flashResetValue = source->flashResetValue;
        dest->flashAltColor0 = source->flashAltColor0;
        dest->flashAltColor1 = source->flashAltColor1;
        dest->flashEnabled = source->flashEnabled;
        dest->flashMode = source->flashMode;
        dest->flashDirectionSign = source->flashDirectionSign;
    }

    return dest;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicompositepanelentry-assigncopy
 * @recoil-artifact defines .text recoil:function:0x4bc3a0: HudUiCompositePanelEntry::AssignCopy.
 * Purpose: copy one composite-panel entry into existing entry storage.
 */
HudUiCompositePanelEntry * HudUiCompositePanelEntry::AssignCopy(
    const HudUiCompositePanelEntry *source
) {
    HudUiPanel::operator=(*source);
    flashCountdown = source->flashCountdown;
    flashResetValue = source->flashResetValue;
    flashAltColor0 = source->flashAltColor0;
    flashAltColor1 = source->flashAltColor1;
    flashEnabled = source->flashEnabled;
    flashMode = source->flashMode;
    flashDirectionSign = source->flashDirectionSign;
    return this;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicompositepanelentry-constructorcopy
 * @recoil-artifact defines .text recoil:function:0x4bc410: HudUiCompositePanelEntry::ConstructorCopy.
 * Purpose: copy-construct one composite-panel entry from another entry.
 */
HudUiCompositePanelEntry * HudUiCompositePanelEntry::ConstructorCopy(
    const HudUiCompositePanelEntry *source
) {
    HudUiPanel::operator=(*source);
    flashCountdown = source->flashCountdown;
    flashResetValue = source->flashResetValue;
    flashAltColor0 = source->flashAltColor0;
    flashAltColor1 = source->flashAltColor1;
    flashEnabled = source->flashEnabled;
    flashMode = source->flashMode;
    flashDirectionSign = source->flashDirectionSign;
    return this;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicircle-huduicircle
 * @recoil-artifact defines .text recoil:function:0x4bc480: HudUiCircle::HudUiCircle.
 * Purpose: initialize a circle element's position, radius, and color.
 *
 * Evidence: BN assembly calls the HudUiElement base constructor at object
 * offset zero, installs the derived circle C++ dispatch identity, stores
 * radius at 0x34, stores
 * radiusSquared as radius * radius at 0x38, stores color565 at 0x3c, and
 * returns this.
 */
HudUiCircle::HudUiCircle(
    int x,
    int y,
    int circleRadius,
    unsigned int circleColor565
)
    : HudUiElement(
        x,
        y
    ) {
    radius = circleRadius;
    const unsigned int radiusBits = (unsigned int)(circleRadius);
    radiusSquared = (int)(radiusBits * radiusBits);
    color565 = circleColor565;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicircle-draw
 * @recoil-artifact defines .text recoil:function:0x4bc4c0: HudUiCircle::Draw.
 * Purpose: redraw the inherited base and circle outline for a dirty circle element.
 */
void HudUiCircle::Draw() {
    DrawBase();
    zRndr_DrawCircleOutline16_Framebuffer(
        x,
        y,
        radius,
        color565,
        0
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicircle-hittestcore
 * @recoil-artifact defines .text recoil:function:0x4bc4e0: HudUiCircle::HitTestCore.
 * Purpose: compare a point's squared distance against the circle radius.
 */
unsigned char HudUiCircle::HitTestCore(
    int px,
    int py
) {
    const unsigned int dx = (unsigned int)(px) - (unsigned int)(x);
    const unsigned int dy = (unsigned int)(py) - (unsigned int)(y);
    const unsigned int distanceSquared = dx * dx + dy * dy;
    return (int)(distanceSquared) < radiusSquared ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcontainer-huduibackgroundcontainer-0x4bc510
 * @recoil-artifact defines .text recoil:function:0x4bc510: HudUiBackgroundContainer::HudUiBackgroundContainer.
 * Purpose: preserve the recovered HUD behavior for HudUiBackgroundContainer::HudUiBackgroundContainer.
 */
HudUiBackgroundContainer::HudUiBackgroundContainer(
    int initFlag
) : HudUiContainer() {
    captureTransitionMask = initFlag;
    inputFocusElement = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcontainer-huduibackgroundcontainer-0x4bc540
 * @recoil-artifact defines .text recoil:function:0x4bc540: HudUiBackgroundContainer::~HudUiBackgroundContainer.
 * Purpose: Restores the background-container base state and tears down the inherited container.
 */
HudUiBackgroundContainer::~HudUiBackgroundContainer() {
    HudUiContainer::DestructorCore();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcontainer-setinputfocus
 * @recoil-artifact defines .text recoil:function:0x4bc550: HudUiBackgroundContainer::SetInputFocus.
 * Purpose: Stores the child element that currently owns background input focus.
 */
void HudUiBackgroundContainer::SetInputFocus(
    HudUiElement *element
) {
    inputFocusElement = element;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcontainer-getinputfocus
 * @recoil-artifact defines .text recoil:function:0x4bc560: HudUiBackgroundContainer::GetInputFocus.
 * Purpose: Returns the child element that currently owns background input focus.
 */
HudUiElement * HudUiBackgroundContainer::GetInputFocus() {
    return inputFocusElement;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcontainer-updateall
 * @recoil-artifact defines .text recoil:function:0x4bc570: HudUiBackgroundContainer::UpdateAll.
 * Purpose: Dispatch background mouse input, update child widgets, and move the focus cursor.
 */
void HudUiBackgroundContainer::UpdateAll(
    float deltaSeconds
) {
    if (enabled == 0) {
        return;
    }

    HudUiBackground *const background = (HudUiBackground *)this;

    memcpy(
        &mouseState,
        zInput::Mouse_GetStateSnapshotPtr(),
        sizeof(mouseState)
    );

    for (HudUiElement *widget = childHead; widget != 0; widget = widget->next) {
        const int hit = widget->HitTest(
            mouseState.cursorClientX,
            mouseState.cursorClientY
        );
        const int hovered = hit == 1 ? 1 : 0;

        if (widget->ShouldHandleInput(
            background,
            hovered
        ) != 0) {
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

                if ((mouseState.button1Transition & captureTransitionMask) != 0 &&
                    (widget->state & 2) == 0) {
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
                    widget->OnPointerButtonState(
                        mouseState.cursorClientX,
                        mouseState.cursorClientY
                    );
                }

                if ((mouseState.button1Transition & 4) != 0 && (widget->state & 2) == 2) {
                    widget->OnCapturedPrimaryRelease();
                }
            } else {
                if ((mouseState.button1Transition & captureTransitionMask) != 0 &&
                    (widget->state & 2) == 2) {
                    widget->state = (unsigned short)(widget->state & 0xfffd);
                    widget->OnEndCapture();
                }

                if ((mouseState.button1Transition & 3) != 0 && (widget->state & 2) == 2) {
                    widget->OnPointerButtonState(
                        mouseState.cursorClientX,
                        mouseState.cursorClientY
                    );
                }

                if ((widget->state & 1) == 1) {
                    widget->state = (unsigned short)(widget->state & 0xfffe);
                    widget->HidePreview();
                }
            }
        }

        widget->AfterInputUpdate(
            background,
            hovered
        );
    }

    HudUiElement *const focusBeforeUpdate = inputFocusElement;
    if (focusBeforeUpdate != 0) {
        focusBeforeUpdate->DrawBase();
    }

    HudUiContainer::UpdateAll(deltaSeconds);

    HudUiElement *const focusAfterUpdate = inputFocusElement;
    if (focusAfterUpdate != 0) {
        focusAfterUpdate->SetPos(
            mouseState.cursorClientX,
            mouseState.cursorClientY
        );
        focusAfterUpdate->Update(deltaSeconds);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudui-setinvalidatemode
 * @recoil-artifact defines .text recoil:function:0x4bc760: HudUi::SetInvalidateMode.
 * Purpose: apply the recovered HUD state change handled by HudUi::SetInvalidateMode.
 */
void __fastcall HudUi::SetInvalidateMode(
    int mode
) {
    g_HudUi_InvalidateMask = mode != 0 ? 0x0c : 0x04;
}



/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicontainer-huduicontainer
 * @recoil-artifact defines .text recoil:function:0x4bc780: HudUiContainer::HudUiContainer.
 * Purpose: preserve the recovered HUD behavior for HudUiContainer::HudUiContainer.
 */
HudUiContainer::HudUiContainer() {
    HudUiContainer *const container = this;
    container->SetEnabled(0);
    childHead = 0;
    childTail = 0;
}

/**
 * Current BN assembly restores the base HudUiContainer vptr and returns.
 * Purpose: tear down the common container base after derived HUD UI cleanup.
 */
HudUiContainer::~HudUiContainer() {
}

/**
 * Purpose: route legacy native smoke call sites through the recovered C++
 * destructor so base vptr restoration remains compiler-owned.
 */
void HudUiContainer::DestructorCore() {
    this->HudUiContainer::~HudUiContainer();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicontainer-addchild
 * @recoil-artifact defines .text recoil:function:0x4bc7c0: HudUiContainer::AddChild.
 * Purpose: preserve the recovered HUD behavior for HudUiContainer::AddChild.
 */
int HudUiContainer::AddChild(
    HudUiElement *child
) {
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
 * Purpose: find a child in the container list and optionally report the
 * previous sibling.
 */
int HudUiContainer::FindChildWithPrev(
    HudUiElement *child,
    HudUiElement **previousOut
) {
    HudUiElement *previous = childHead;
    if (previous == 0) {
        return 0;
    }

    if (child == previous) {
        *previousOut = 0;
        return 1;
    }

    while (previous != 0) {
        HudUiElement *const current = previous->next;
        if (current == child) {
            if (previousOut != 0) {
                *previousOut = previous;
            }

            return 1;
        }

        previous = current;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicontainer-removechild
 * @recoil-artifact defines .text recoil:function:0x4bc860: HudUiContainer::RemoveChild.
 * Purpose: unlink a child from this container and clear the child's owner
 * links.
 */
int HudUiContainer::RemoveChild(
    HudUiElement *child
) {
    HudUiElement *previous = child;
    if (FindChildWithPrev(
        child,
        &previous
    ) == 0) {
        return 0;
    }

    if (previous != 0) {
        previous->next = child->next;
        if (child == childTail) {
            childTail = previous;
        }
    } else {
        childHead = child->next;
        if (child == childTail) {
            childTail = child->next;
        }
    }

    child->next = 0;
    child->parent = 0;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduicontainer-setchildflags
 * @recoil-artifact defines .text recoil:function:0x4bc8d0: HudUiContainer::SetChildFlags.
 *
 * Purpose: apply a shared child flag mask to every child while preserving each
 * child's hidden/disabled bit 0x10.
 *
 * Evidence: BN assembly at 0x4bc8d0 walks HudUiContainer::childHead through
 * HudUiElement::next, writes childFlags directly when bit 0x10 is clear, and
 * writes childFlags|0x10 when the existing child flags preserve that bit.
 */
void HudUiContainer::SetChildFlags(
    unsigned int childFlags
) {
    for (HudUiElement *child = childHead; child != 0; child = child->next) {
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
 * Purpose: Dispatch per-frame updates to every child in an enabled container.
 */
void HudUiContainer::UpdateAll(
    float deltaSeconds
) {
    if (enabled == 0) {
        return;
    }

    for (HudUiElement *child = childHead; child != 0; child = child->next) {
        child->Update(deltaSeconds);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitransitiontextpanel-resetflashstate
 * @recoil-artifact defines .text recoil:function:0x4bc930: HudUiTransitionTextPanel::ResetFlashState.
 *
 * Purpose: enable flash state, update a positive flash rate to its half-period,
 * reset the countdown from that period, and restore forward flash direction.
 *
 * Evidence: BN assembly at 0x4bc930 writes flashEnabled, conditionally stores
 * flashRate*0.5 into flashResetValue when flashRate is positive, copies
 * flashResetValue into flashCountdown, and writes flashDirectionSign = 1.
 */
void HudUiTransitionTextPanel::ResetFlashState(
    float flashRate
) {
    flashEnabled = 1;
    if (flashRate > 0.0f) {
        flashResetValue = flashRate * 0.5f;
    }

    flashDirectionSign = 1;
    memcpy(
        &flashCountdown,
        &flashResetValue,
        sizeof(flashCountdown)
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitransitiontextpanel-setflashrate
 * @recoil-artifact defines .text recoil:function:0x4bc980: HudUiTransitionTextPanel::SetFlashRate.
 *
 * Purpose: enter rate-only flashing by resetting flash state unless the panel
 * is already in rate-only flash mode.
 *
 * Evidence: BN assembly at 0x4bc980 returns when flashMode is 1; otherwise it
 * calls ResetFlashState(flashRate) and stores flashMode = 1.
 */
void HudUiTransitionTextPanel::SetFlashRate(
    float flashRate
) {
    if (flashMode == 1) {
        return;
    }

    ResetFlashState(flashRate);
    flashMode = 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitransitiontextpanel-setflashcolorandrate
 * @recoil-artifact defines .text recoil:function:0x4bc9b0: HudUiTransitionTextPanel::SetFlashColorAndRate.
 * Purpose: enter color-flash mode and store the alternate flash text colors.
 *
 * Evidence: BN assembly returns when flashMode is already color-flash mode,
 * calls ResetFlashState, writes flashMode = 2, and stores the same alternate
 * color into both flash color fields.
 */
void HudUiTransitionTextPanel::SetFlashColorAndRate(
    unsigned int flashColor,
    float flashRate
) {
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
 * Purpose: update timed visibility and flash-color state before drawing the panel.
 *
 * Evidence: BN assembly subtracts delta time from the base timer and flash
 * countdown, hides timed-out panels through the visibility slot, toggles
 * flashDirectionSign/textDirty, swaps text colors for color-flash modes, and
 * calls HudUiPanel::Draw on visible refresh paths.
 */
void HudUiTransitionTextPanel::Update(
    float deltaSeconds
) {
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
            flashCountdown = flashResetValue;
            textDirty = 1;
            flashDirectionSign = -flashDirectionSign;

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
 * Purpose: initialize label text, position, font handle, and alignment state.
 */
HudUiTextLabel::HudUiTextLabel(
    const char *text,
    int initX,
    int initY,
    int flags
) : HudUiElement(
        0,
        0
    ) {
    centerText = 0;
    SetTextFmt(text);
    x = initX;
    y = initY;
    ((HudUiElement *)(this))->Invalidate();
    fontHandle = flags;
    ((HudUiElement *)(this))->Invalidate();
    alignMode = 0;
}

/**
 * Original helper; no standalone retail function exists. Observed in the
 * HudUiTextLabel method cluster as the caller-owned storage wrapper around
 * the 0x4bcb50 address-backed constructor.
 * Purpose: construct a text label in caller-provided storage and return it.
 */
HudUiTextLabel * HudUiTextLabel::ConstructorWithPosAndFlags(
    const char *text,
    int initX,
    int initY,
    int flags
) {
    new (this) HudUiTextLabel(
        text,
        initX,
        initY,
        flags
    );
    return this;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextlabel-huduitextlabel-0x4bcbe0
 * @recoil-artifact defines .text recoil:function:0x4bcbe0: HudUiTextLabel::HudUiTextLabel(const HudUiTextLabel &).
 * Purpose: Copy-construct a text label from an existing label, including its text buffer.
 */
HudUiTextLabel::HudUiTextLabel(
    const HudUiTextLabel &source
) : HudUiElement(source) {
    strncpy(
        textBuffer,
        source.textBuffer,
        sizeof(textBuffer)
    );
    fontHandle = source.fontHandle;
    centerText = source.centerText;
    centerBoundsLeft = source.centerBoundsLeft;
    centerBoundsRight = source.centerBoundsRight;
    alignMode = source.alignMode;
}

/**
 * Purpose: Initialize this text label by copying the source label state.
 */
HudUiTextLabel & HudUiTextLabel::operator=(
    const HudUiTextLabel &source
) {
    HudUiElement::operator=(source);
    strncpy(
        textBuffer,
        source.textBuffer,
        sizeof(textBuffer)
    );
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
 * Purpose: format label text, refresh centered extents when needed, and
 * invalidate the element.
 */
void HudUiTextLabel::SetTextFmt(
    const char *format,
    ...
) {
    if (format == 0) {
        memset(
            textBuffer,
            0,
            sizeof(textBuffer)
        );
        return;
    }

    va_list args;
    va_start(
        args,
        format
    );
    vsprintf(
        textBuffer,
        format,
        args
    );
    va_end(args);

    if (centerText != 0) {
        UpdateTextExtents();
    }

    Invalidate();
}

void HudUiPanel::SetClip(
    void *bltSourceOrNull,
    const HudUiRect *rectOrNull
) {
    bltSource = bltSourceOrNull;
    if (rectOrNull != 0) {
        clipRect = *rectOrNull;
    }

    Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextlabel-rebuildtextbounds
 * @recoil-artifact defines .text recoil:function:0x4bcd80: HudUiTextLabel::RebuildTextBounds.
 * Purpose: rebuild the clip rectangle from the current formatted text size.
 */
void HudUiTextLabel::RebuildTextBounds() {
    int widthPx;
    int lineAdvance;
    zImage_Font::MeasureString(
        textBuffer,
        fontHandle,
        &widthPx,
        &lineAdvance
    );
    clipRect.right = clipRect.left + widthPx;
    clipRect.bottom = clipRect.top + lineAdvance;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextlabel-measuretextwidth
 * @recoil-artifact defines .text recoil:function:0x4bcdc0: HudUiTextLabel::MeasureTextWidth.
 * Purpose: return the measured pixel width of the current label text.
 */
int HudUiTextLabel::MeasureTextWidth() {
    int widthPx;
    int lineAdvance;
    zImage_Font::MeasureString(
        textBuffer,
        fontHandle,
        &widthPx,
        &lineAdvance
    );
    return widthPx;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextlabel-updatetextextents
 * @recoil-artifact defines .text recoil:function:0x4bcdf0: HudUiTextLabel::UpdateTextExtents.
 * Purpose: recenter the label inside its stored bounds and refresh clip
 * extents when a blit source is active.
 */
void HudUiTextLabel::UpdateTextExtents() {
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
 * Purpose: draw non-empty label text with the recovered alignment handling.
 */
void HudUiTextLabel::OnDraw() {
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
        zImage_Font::BlitStringToActiveTarget(
            textBuffer,
            x,
            y,
            fontHandle
        );
        x += xOffset;
        return;
    }

    zImage_Font::BlitStringToActiveTarget(
        textBuffer,
        x,
        y,
        fontHandle
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextlabel-hittest
 * @recoil-artifact defines .text recoil:function:0x4bcea0: HudUiTextLabel::HitTest.
 * Purpose: test coordinates against the visible text bounds unless input is
 * disabled.
 */
int HudUiTextLabel::HitTest(
    int px,
    int py
) {
    if ((flags & 0x10u) != 0 || x > px || y > py) {
        return 0;
    }

    int textWidth = 0;
    int lineAdvance = 0;
    zImage_Font::MeasureString(
        textBuffer,
        fontHandle,
        &textWidth,
        &lineAdvance
    );

    if (px > x + textWidth) {
        return 0;
    }

    return py <= y + lineAdvance ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibar-huduibar
 * @recoil-artifact defines .text recoil:function:0x4bcf20: HudUiBar::HudUiBar.
 * Purpose: Constructs the HUD element base, clears bar point storage, and marks the bar dirty.
 */
HudUiBar::HudUiBar() : HudUiElement(
    0,
    0
) {
    drawVertexCount = 0;
    memset(
        points,
        0,
        sizeof(points)
    );
    Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibar-setpointxy
 * @recoil-artifact defines .text recoil:function:0x4bcf80: HudUiBar::SetPointXY.
 * Binary Ninja evidence: bounds-checks pointIndex against the 21-element point
 * array, writes the HudUiBarPoint x/y fields, raises drawVertexCount, dispatches
 * SetPos for point zero, and always invalidates the element.
 * Purpose: Update one bar point and keep the element position/count state dirty.
 */
void HudUiBar::SetPointXY(
    int pointIndex,
    float x,
    float y
) {
    if (pointIndex >= 0 && pointIndex < 21) {
        points[pointIndex].x = x;
        points[pointIndex].y = y;

        if (drawVertexCount < pointIndex + 1) {
            drawVertexCount = pointIndex + 1;
        }

        if (pointIndex == 0) {
            SetPos(
                (int)(x),
                (int)(y)
            );
        }
    }

    Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibar-draw
 * @recoil-artifact defines .text recoil:function:0x4bcff0: HudUiBar::Draw.
 * Binary Ninja evidence: dispatches the base DrawBase method, reads
 * drawVertexCount, and calls zRndr::RasterizePoly with points and drawParam
 * only when at least one vertex is active.
 * Purpose: Draw the bar base and rasterize the populated point list.
 */
void HudUiBar::Draw() {
    DrawBase();
    if (drawVertexCount != 0) {
        zRndr_RasterizePoly(
            (zVec3 *)(points),
            drawVertexCount,
            drawParam
        );
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitopmessagestack-constructor
 * @recoil-artifact defines .text recoil:function:0x4bd020: HudUiTopMessageStack::Constructor.
 * Purpose: construct the top-message four-line stack and configure ascending rows.
 */
HudUiTopMessageStack * HudUiTopMessageStack::Constructor() {
    new ((HudUiContainer *)this) HudUiContainer;

    {
        for (int index = 0; index < 4; ++index) {
            lines[index].ConstructorDefault(
                0,
                0,
                0
            );
        }
    }

    int y = 0x1e;
    {
        for (int index = 0; index < 4; ++index) {
            ConfigureTextStackLine(
                this,
                &lines[index],
                y,
                0x0d,
                0x258,
                7
            );
            y += 0x12;
        }
    }

    return this;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextstack4-setfontall
 * @recoil-artifact defines .text recoil:function:0x4bd110: HudUiTextStack4::SetFontAll.
 * Purpose: apply one font definition to every row in the four-line stack.
 */
void HudUiTextStack4::SetFontAll(
    const char *faceName,
    int height,
    int weight,
    int width
) {
    for (int index = 3; index >= 0; --index) {
        HudUiPanel *const panel = &lines[index];
        panel->SetFont(
            faceName,
            height,
            weight,
            width,
            0,
            0,
            2
        );
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextstack4-pushline
 * @recoil-artifact defines .text recoil:function:0x4bd160: HudUiTextStack4::PushLine.
 * Purpose: push a visible timed message into the four-row text stack.
 */
HudUiPanel * HudUiTextStack4::PushLine(
    const char *message,
    float duration
) {
    SetEnabled(1);

    if (((~((HudUiElement *)(&lines[0]))->flags) & 0x10u) != 0 &&
        strcmp(
            message,
            lines[0].GetLastTextPtr()
        ) != 0) {
        for (HudUiPanel *source = &lines[2]; source >= &lines[0]; --source) {
            HudUiPanel *const dest = source + 1;
            HudUiElement *const sourceElement = (HudUiElement *)(source);

            if (((~sourceElement->flags) & 0x10u) != 0) {
                source->SetVisible(0);
                ((HudUiElement *)(dest))->SetTimer(
                    ((HudUiElement *)(source))->timer
                );
                dest->SetTextFmt(source->GetLastTextPtr());
                dest->textColor0 = source->textColor0;
                dest->textColor1 = source->textColor1;
                dest->textDirty = 1;
                ((HudUiElement *)(dest))->SetVisible(1);
            }
        }
    }

    ((HudUiElement *)(&lines[0]))->SetTimer(duration);
    lines[0].SetTextFmt(
        "%s",
        message
    );
    ((HudUiElement *)(&lines[0]))->SetVisible(1);
    return &lines[0];
}

namespace HudUi {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-pushtopmessageline
 * @recoil-artifact defines .text recoil:function:0x4bd280: HudUi::PushTopMessageLine.
 * Purpose: push a message directly into the global top-message stack.
 */
void __fastcall PushTopMessageLine(
    const char *message,
    float duration
) {
    g_HudUiTopMessageStack->PushLine(
        message,
        duration
    );
}
} // namespace HudUi

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextstack4-clear
 * @recoil-artifact defines .text recoil:function:0x4bd2a0: HudUiTextStack4::Clear.
 * Purpose: clear text and hide every row in the four-line stack.
 */
void HudUiTextStack4::Clear() {
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = &lines[index];
        panel->SetTextFmt("");
        panel->SetVisible(0);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduichatmessagestack-constructor
 * @recoil-artifact defines .text recoil:function:0x4bd2d0: HudUiChatMessageStack::Constructor.
 * Purpose: construct the chat-message four-line stack and configure descending rows.
 */
HudUiChatMessageStack * HudUiChatMessageStack::Constructor() {
    new ((HudUiContainer *)this) HudUiContainer;

    {
        for (int index = 0; index < 4; ++index) {
            lines[index].ConstructorDefault(
                0,
                0,
                0
            );
        }
    }

    int y = 0x159;
    {
        for (int index = 0; index < 4; ++index) {
            HudUiPanel *const panel = &lines[index];
            panel->textColor0 = 0x00996a00;
            panel->textColor1 = 0x0095c7ff;
            panel->textDirty = 1;
            ConfigureTextStackLine(
                this,
                panel,
                y,
                0x0a,
                0x1f4,
                6
            );
            y -= 0x12;
        }
    }

    return this;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextstack4-settextcolors
 * @recoil-artifact defines .text recoil:function:0x4bd3d0: HudUiTextStack4::SetTextColors.
 * Purpose: assign both text colors to every row in the four-line stack.
 */
void HudUiTextStack4::SetTextColors(
    unsigned int color0,
    unsigned int color1
) {
    for (int index = 3; index >= 0; --index) {
        HudUiPanel *const panel = &lines[index];
        panel->textColor0 = color0;
        panel->textColor1 = color1;
        panel->textDirty = 1;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextstack4-setxall
 * @recoil-artifact defines .text recoil:function:0x4bd410: HudUiTextStack4::SetXAll.
 * Purpose: move every row in the four-line stack to a shared x position.
 */
void HudUiTextStack4::SetXAll(
    int newX
) {
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = &lines[index];
        panel->SetX(newX);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduitextstack4-setydescending
 * @recoil-artifact defines .text recoil:function:0x4bd440: HudUiTextStack4::SetYDescending.
 * Purpose: place every row in the four-line stack at descending y positions.
 */
void HudUiTextStack4::SetYDescending(
    int yStart
) {
    int y = yStart;
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = &lines[index];
        panel->SetY(y);
        y -= 0x12;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-ztimedtask-removefromactivelist
 * @recoil-artifact defines .text recoil:function:0x4bd470: zTimedTask::RemoveFromActiveList.
 * Purpose: preserve the recovered HUD behavior for zTimedTask::RemoveFromActiveList.
 */
void zTimedTask::RemoveFromActiveList() {
    zTimedTask *node = g_zTimedTask_ActiveHead;
    zTimedTask *previous = 0;
    if (node == 0) {
        return;
    }

    while (node != this) {
        previous = node;
        node = node->next;
        if (node == 0) {
            return;
        }
    }

    if (previous == 0) {
        g_zTimedTask_ActiveHead = g_zTimedTask_ActiveHead->next;
        --g_zTimedTask_ActiveCount;
        return;
    }

    if (node == g_zTimedTask_ActiveTail) {
        g_zTimedTask_ActiveTail = previous;
    }

    previous->next = node->next;
    --g_zTimedTask_ActiveCount;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-ztimedtask-runimmediateaction
 * @recoil-artifact defines .text recoil:function:0x4bd4d0: zTimedTask::RunImmediateAction.
 * Purpose: preserve the recovered HUD behavior for zTimedTask::RunImmediateAction.
 */
void zTimedTask::RunImmediateAction() {
    switch (kind) {
    case 1:
        if (actionArg2 != 0) {
            zVid_Image::BlitToActiveTarget(
                (zVidImagePartial *)(actionArg2),
                actionArg0,
                actionArg1,
                (unsigned short)(actionArg3),
                (zVidRect32 *)(actionArg4)
            );
        }
        break;

    case 2:
        zRndr_DrawImmediateLine(
            actionArg0,
            actionArg1,
            actionArg2,
            actionArg3,
            actionArg4
        );
        break;

    case 3:
        zRndr_RasterizePoly(
            (zVec3 *)(&actionArg0),
            rasterVertexCount,
            rasterDrawParam
        );
        break;

    case 4: {
        const char *text = (const char *)(&actionArg2) + 2;
        if (*text != '\0') {
            zImage_Font::BlitStringToActiveTarget(
                text,
                (short)(actionArg0),
                (short)(actionArg1),
                (short)(actionArg2)
            );
        }
        break;
    }

    case 5: {
        const char *text = (const char *)(actionArg3);
        if (text != 0 && *text != '\0') {
            zImage_Font::BlitStringToActiveTarget(
                text,
                (short)(actionArg0),
                (short)(actionArg1),
                (short)(actionArg2)
            );
        }
        break;
    }

    case 6:
        zRndr_SpanOcclusion_TestSample(
            actionArg0,
            actionArg1,
            actionArg2
        );
        break;

    case 7: {
        zVec3 point0;
        zVec3 point1;
        int point0Clipped;
        int point1Clipped;
        point0.x = (float)(actionArg0);
        point0.y = (float)(actionArg1);
        point1.x = (float)(actionArg2);
        point1.y = (float)(actionArg3);

        if (HudLineClip::ClipSegmentToCurrentBounds(
                &point0,
                &point1,
                &point0Clipped,
                &point1Clipped
            ) != 0) {
            zRndr_DrawImmediateLine(
                (int)(point0.x),
                (int)(point0.y),
                (int)(point1.x),
                (int)(point1.y),
                actionArg4
            );
        }
        break;
    }

    case 8:
        zRndr_DrawClippedImmediateLineStrip(
            (const zRndr_LinePoint2I *)(&actionArg0),
            alphaPointCount - 1,
            (void *)(alpha255),
            alphaVariantIndex
        );
        break;

    default:
        break;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-ztimedtask-tickactivelist
 * @recoil-artifact defines .text recoil:function:0x4bd660: zTimedTask::TickActiveList.
 * Purpose: preserve the recovered HUD behavior for zTimedTask::TickActiveList.
 */
void zTimedTask::TickActiveList() {
    zTimedTask *task = g_zTimedTask_ActiveHead;
    while (task != 0) {
        if ((task->flags & 0x02) == 0) {
            task->RunImmediateAction();
        } else if ((task->flags & 0x04) != 0) {
            task->RunImmediateAction();
            task->flags &= ~0x04;
        } else if ((task->flags & 0x08) != 0) {
            task->RunImmediateAction();
            task->flags &= ~0x08;
        }

        if ((task->flags & 0x01) != 0) {
            task->remainingSeconds -= g_FrameDeltaTimeSec;
            if (task->remainingSeconds <= 0.0f) {
                task->kind = 9;
                task->RemoveFromActiveList();
            }
        }

        task = task->next;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudlineclip-setcurrentboundsfromrecti
 * @recoil-artifact defines .text recoil:function:0x4bd6f0: HudLineClip::SetCurrentBoundsFromRectI
 * Purpose: Copy integer rectangle edges into the current float clip bounds.
 */
void __fastcall HudLineClip::SetCurrentBoundsFromRectI(
    const HudRectI *rect
) {
    g_HudLineClip_CurrentLeft = (float)(rect->left);
    g_HudLineClip_CurrentTop = (float)(rect->top);
    g_HudLineClip_CurrentRight = (float)(rect->right);
    g_HudLineClip_CurrentBottom = (float)(rect->bottom);
}

namespace zMath {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-cliplinesegmenttozrange
 * @recoil-artifact defines .text recoil:function:0x4bd720: zMath::ClipLineSegmentToZRange
 * Purpose: clips a mutable segment against the current zMath lower and upper
 * Z clipping planes, rejecting segments fully outside the range.
 * Data: reads g_zMath_ClipZLowerBound at 0x4e4880 and
 * g_zMath_ClipZUpperBound at 0x4e4890.
 */
int __fastcall ClipLineSegmentToZRange(
    zVec3 *pointA,
    zVec3 *pointB
) {
    if (pointA->z > g_zMath_ClipZUpperBound && pointB->z > g_zMath_ClipZUpperBound) {
        return 0;
    }

    if (pointA->z < g_zMath_ClipZLowerBound && pointB->z < g_zMath_ClipZLowerBound) {
        return 0;
    }

    if (pointA->z < g_zMath_ClipZLowerBound) {
        ClipLineSegmentPointToZ(
            pointA,
            pointB,
            g_zMath_ClipZLowerBound
        );
    }
    if (pointB->z < g_zMath_ClipZLowerBound) {
        ClipLineSegmentPointToZ(
            pointB,
            pointA,
            g_zMath_ClipZLowerBound
        );
    }

    if (pointB->z > g_zMath_ClipZUpperBound) {
        ClipLineSegmentPointToZ(
            pointB,
            pointA,
            g_zMath_ClipZUpperBound
        );
    }
    if (pointA->z > g_zMath_ClipZUpperBound) {
        ClipLineSegmentPointToZ(
            pointA,
            pointB,
            g_zMath_ClipZUpperBound
        );
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-cliplinesegmentpointtoz
 * @recoil-artifact defines .text recoil:function:0x4bd800: zMath::ClipLineSegmentPointToZ
 * Purpose: moves one segment endpoint onto the caller-supplied Z clip plane by
 * interpolating toward the other endpoint.
 * Data: writes only the caller-supplied endpoint and reads no authored globals.
 */
void __fastcall ClipLineSegmentPointToZ(
    zVec3 *pointToClip,
    const zVec3 *otherPoint,
    float clipZ
) {
    const float t = (clipZ - pointToClip->z) / (otherPoint->z - pointToClip->z);

    pointToClip->x = (otherPoint->x - pointToClip->x) * t + pointToClip->x;
    pointToClip->y = (otherPoint->y - pointToClip->y) * t + pointToClip->y;
    pointToClip->z = clipZ;
}

} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudlineclip-clipsegmenttocurrentbounds
 * @recoil-artifact defines .text recoil:function:0x4bd840: HudLineClip::ClipSegmentToCurrentBounds
 * Purpose: Clip a segment against the current X bounds, then the current Y bounds.
 */
int __fastcall HudLineClip::ClipSegmentToCurrentBounds(
    zVec3 *point0,
    zVec3 *point1,
    int *point0Clipped,
    int *point1Clipped
) {
    const int result = ClipSegmentToCurrentXBounds(
        point0,
        point1,
        point0Clipped,
        point1Clipped
    );
    if (result == 0) {
        return 0;
    }

    return ClipSegmentToCurrentYBounds(
        point0,
        point1,
        point0Clipped,
        point1Clipped
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudlineclip-clipsegmenttocurrentxbounds
 * @recoil-artifact defines .text recoil:function:0x4bd880: HudLineClip::ClipSegmentToCurrentXBounds
 * Purpose: Reject or clamp a segment against the current left and right bounds.
 */
int __fastcall HudLineClip::ClipSegmentToCurrentXBounds(
    zVec3 *point0,
    zVec3 *point1,
    int *point0Clipped,
    int *point1Clipped
) {
    if (point0->x > g_HudLineClip_CurrentRight && point1->x > g_HudLineClip_CurrentRight) {
        *point0Clipped = 1;
        *point1Clipped = 1;
        return 0;
    }
    if (point0->x < g_HudLineClip_CurrentLeft && point1->x < g_HudLineClip_CurrentLeft) {
        *point0Clipped = 1;
        *point1Clipped = 1;
        return 0;
    }

    if (point0->x < g_HudLineClip_CurrentLeft) {
        ClipEndpointToX(
            point0,
            point1,
            g_HudLineClip_CurrentLeft
        );
        *point0Clipped = 1;
    } else if (point0->x > g_HudLineClip_CurrentRight) {
        ClipEndpointToX(
            point0,
            point1,
            g_HudLineClip_CurrentRight
        );
        *point0Clipped = 1;
    }

    if (point1->x < g_HudLineClip_CurrentLeft) {
        ClipEndpointToX(
            point1,
            point0,
            g_HudLineClip_CurrentLeft
        );
        *point1Clipped = 1;
    } else if (point1->x > g_HudLineClip_CurrentRight) {
        ClipEndpointToX(
            point1,
            point0,
            g_HudLineClip_CurrentRight
        );
        *point1Clipped = 1;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudlineclip-clipendpointtox
 * @recoil-artifact defines .text recoil:function:0x4bd9c0: HudLineClip::ClipEndpointToX
 * Purpose: Move one segment endpoint to an X clipping plane and interpolate Y.
 */
void __fastcall HudLineClip::ClipEndpointToX(
    zVec3 *endpoint,
    const zVec3 *otherEndpoint,
    float clipX
) {
    endpoint->y += (otherEndpoint->y - endpoint->y) *
                   ((clipX - endpoint->x) / (otherEndpoint->x - endpoint->x));
    endpoint->x = clipX;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudlineclip-clipsegmenttocurrentybounds
 * @recoil-artifact defines .text recoil:function:0x4bd9f0: HudLineClip::ClipSegmentToCurrentYBounds
 * Purpose: Reject or clamp a segment against the current top and bottom bounds.
 */
int __fastcall HudLineClip::ClipSegmentToCurrentYBounds(
    zVec3 *point0,
    zVec3 *point1,
    int *point0Clipped,
    int *point1Clipped
) {
    if (point0->y > g_HudLineClip_CurrentBottom && point1->y > g_HudLineClip_CurrentBottom) {
        *point0Clipped = 1;
        *point1Clipped = 1;
        return 0;
    }
    if (point0->y < g_HudLineClip_CurrentTop && point1->y < g_HudLineClip_CurrentTop) {
        *point0Clipped = 1;
        *point1Clipped = 1;
        return 0;
    }

    if (point0->y < g_HudLineClip_CurrentTop) {
        ClipEndpointToY(
            point0,
            point1,
            g_HudLineClip_CurrentTop
        );
        *point0Clipped = 1;
    } else if (point0->y > g_HudLineClip_CurrentBottom) {
        ClipEndpointToY(
            point0,
            point1,
            g_HudLineClip_CurrentBottom
        );
        *point0Clipped = 1;
    }

    if (point1->y < g_HudLineClip_CurrentTop) {
        ClipEndpointToY(
            point1,
            point0,
            g_HudLineClip_CurrentTop
        );
        *point1Clipped = 1;
    } else if (point1->y > g_HudLineClip_CurrentBottom) {
        ClipEndpointToY(
            point1,
            point0,
            g_HudLineClip_CurrentBottom
        );
        *point1Clipped = 1;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudlineclip-clipendpointtoy
 * @recoil-artifact defines .text recoil:function:0x4bdb30: HudLineClip::ClipEndpointToY
 * Purpose: Move one segment endpoint to a Y clipping plane and interpolate X.
 */
void __fastcall HudLineClip::ClipEndpointToY(
    zVec3 *endpoint,
    const zVec3 *otherEndpoint,
    float clipY
) {
    endpoint->x += (otherEndpoint->x - endpoint->x) *
                   ((clipY - endpoint->y) / (otherEndpoint->y - endpoint->y));
    endpoint->y = clipY;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-zvideofxpass3element-draw
 * @recoil-artifact defines .text recoil:function:0x4bdb60: zVideoFxPass3Element::Draw.
 * Draws the common HUD base, publishes the parent pass-3 source surface, then dispatches the
 * element-specific pass callback once for each configured input rectangle.
 * Purpose: provide the recovered zVideoFxPass3Element::Draw behavior.
 */
void zVideoFxPass3Element::Draw() {
    zVideoFxPass3Config *const parentConfig = (zVideoFxPass3Config *)(parent);
    DrawBase();

    if (parentConfig == 0) {
        ApplyPass3();
        return;
    }

    if (parentConfig->surfacePixels != 0) {
        zVideo::Fx_SetSurfaceState(
            parentConfig->surfacePixels,
            parentConfig->surfaceWidth,
            parentConfig->surfaceHeight,
            parentConfig->surfacePitchBytes
        );
    }

    int index;
    for (index = 0; index < 2; ++index) {
        HudUiRect *const inputRect = parentConfig->inputRectsOrNull[index];
        if (inputRect != 0) {
            clipRectOrNull = inputRect;
            ApplyPass3();
        }
    }

    clipRectOrNull = parentConfig->inputRectsOrNull[0];
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-zvideofxpass3rootelement-applypass3
 * @recoil-artifact defines .text recoil:function:0x4bdbc0: zVideoFxPass3RootElement::ApplyPass3.
 * Root pass-3 callback submits the currently selected input rectangle as a framebuffer overlay
 * using the root element's recovered color and alpha.
 * Purpose: provide the recovered zVideoFxPass3RootElement::ApplyPass3 behavior.
 */
void zVideoFxPass3RootElement::ApplyPass3() {
    zRndr_OverlayRect_Submit(
        (unsigned int)(packedColor16),
        (zVidRect32 *)(clipRectOrNull),
        alpha
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-zvideofxpass3slot-zvideofxpass3slot
 * @recoil-artifact defines .text recoil:function:0x4bdbe0: zVideoFxPass3Slot::Constructor.
 * Constructs the pass-3 slot element and clears the input clip consumed by
 * Purpose: provide the recovered zVideoFxPass3Slot constructor behavior.
 */
zVideoFxPass3Slot::zVideoFxPass3Slot() : zVideoFxPass3Element(
    0,
    0
) {
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-zvideofxpass3slot-setrectandpayload
 * @recoil-artifact defines .text recoil:function:0x4bdc00: zVideoFxPass3Slot::SetRectAndPayload.
 * Purpose: provide the recovered zVideoFxPass3Slot::SetRectAndPayload behavior.
 */
void zVideoFxPass3Slot::SetRectAndPayload(
    int rectLeftPixels,
    int rectTopPixels,
    int currentRadiusPixels,
    int maxRadiusPixels,
    int extentPixels,
    float sinFreqValue,
    float sinPhaseValue
) {
    SetPos(
        rectLeftPixels,
        rectTopPixels
    );

    currentRadius = currentRadiusPixels;
    maxRadius = maxRadiusPixels;
    extent = extentPixels;
    sinFreq = sinFreqValue;
    sinPhase = sinPhaseValue;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-zvideofxpass3slot-applypass3
 * @recoil-artifact defines .text recoil:function:0x4bdc40: zVideoFxPass3Slot::ApplyPass3.
 * The pass callback forwards the slot position, integer radius payload, sine parameters, and
 * active input clip to the shared pass-3 radial warp routine.
 * Purpose: provide the recovered zVideoFxPass3Slot::ApplyPass3 behavior.
 */
void zVideoFxPass3Slot::ApplyPass3() {
    zVideo::FxPass3_ApplyToCurrentSurface(
        x,
        y,
        currentRadius,
        maxRadius,
        extent,
        sinFreq,
        sinPhase,
        (zVidRect32 *)(clipRectOrNull)
    );
}

namespace {
const int kHudWeatherFxRainSlantDelta = 1;
const int kHudWeatherFxSnowTextureWidth = 16;
const int kHudWeatherFxSnowTextureHeight = 8;
const int kHudWeatherFxSnowTextureTexels =
    kHudWeatherFxSnowTextureWidth * kHudWeatherFxSnowTextureHeight;

/**
 * Original inline helper; no standalone retail function exists.
 * Observed callers: 0x4be2f0 HudWeatherFxSnow::Update and 0x4be880 HudWeatherFxRain::Update.
 * Purpose: Compute a weather particle velocity vector's squared length before normalization.
 */
inline float HudWeatherFxVec3LengthSq(
    const zVec3 *value
) {
    return value->x * value->x + value->y * value->y + value->z * value->z;
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed callers: 0x4be2f0 HudWeatherFxSnow::Update.
 * Purpose: Decide whether a snow particle left the visible weather cone and must respawn.
 */
inline int HudWeatherFxSnowNeedsReset(
    const zVec3 *position
) {
    const float absZ = (float)(fabs(position->z));
    if ((float)(fabs(position->y)) > absZ) {
        return 1;
    }
    if ((float)(fabs(position->x)) > absZ) {
        return 1;
    }
    if (position->z > 1.0) {
        return 1;
    }
    if (position->z < 0.5) {
        return 1;
    }
    return 0;
}

enum zVideoRendererBackend {
    ZVID_RENDERER_BACKEND_SOFTWARE = 0,
};

} // namespace

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-g-hudweatherfxsnow-lastcameratarget
 * @recoil-artifact defines .data recoil:data:0x56bf48: g_HudWeatherFxSnow_LastCameraTarget.
 * Purpose: Retain the previous snow camera target coordinates for frame-to-frame drift.
 */
HudWeatherFxCameraTargetHistory g_HudWeatherFxSnow_LastCameraTarget = {
    0.0f,
    0.0f,
    0.0f,
    0.0f
};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-g-hudweatherfxrain-lastcameratarget
 * @recoil-artifact defines .data recoil:data:0x56bf58: g_HudWeatherFxRain_LastCameraTarget.
 * Purpose: Retain the previous rain camera target coordinates for frame-to-frame drift.
 */
HudWeatherFxCameraTargetHistory g_HudWeatherFxRain_LastCameraTarget = {
    0.0f,
    0.0f,
    0.0f,
    0.0f
};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-f-0x56bf68
 * @recoil-artifact defines .data recoil:data:0x56bf68: g_HudWeatherFxSnow_TimeAccumulator.
 * Purpose: Accumulate elapsed snow update time.
 */
float g_HudWeatherFxSnow_TimeAccumulator = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-f-0x56bf6c
 * @recoil-artifact defines .data recoil:data:0x56bf6c: g_HudWeatherFxRain_TimeAccumulator.
 * Purpose: Accumulate elapsed rain update time.
 */
float g_HudWeatherFxRain_TimeAccumulator = 0.0f;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfx-hudweatherfx-0x4bdc70
 * @recoil-artifact defines .text recoil:function:0x4bdc70: HudWeatherFx::HudWeatherFx(int).
 * Purpose: Initialize the base weather particle emitter, allocate particle buffers, reset
 * particles, and create the hardware SnowFX texture resources when needed.
 */
HudWeatherFx::HudWeatherFx(
    int newParticleCount
) {
    HudUiElement::Constructor(
        0,
        0
    );
    clipRectOrNull = 0;
    maxParticles = newParticleCount;
    particleCount = newParticleCount;
    particleQuads = (HudWeatherFxParticleQuad *)(::operator new(
        sizeof(HudWeatherFxParticleQuad) * newParticleCount
    ));

    for (int index = 0; index < newParticleCount; ++index) {
        particleQuads[index].x = -1;
        particleQuads[index].y = -1;
        particleQuads[index].width = -1;
        particleQuads[index].height = -1;
    }

    packedColor16 = 0x7fff;
    alphaStartScale = 1.0f;
    alphaEndScale = 0.0500000007f;
    camera = 0;
    activeParticleCount = 0;
    sourceBufferIndex = 0;
    destBufferIndex = 1;

    const unsigned int positionBytes = sizeof(zVec3) * newParticleCount;
    particlePositions[sourceBufferIndex] = (zVec3 *)(::operator new(positionBytes));
    particlePositions[destBufferIndex] = (zVec3 *)(::operator new(positionBytes));

    for (int resetIndex = 0; resetIndex < newParticleCount; ++resetIndex) {
        ResetParticleSlot(
            resetIndex,
            1
        );
    }

    basisVector.x = 0.0f;
    basisVector.y = 1.0f;
    basisVector.z = 0.0f;
    gravity = 1.0f;
    windDirection = 0.0f;
    windVelocity = 1.0f;
    textureName = 0;
    softwareImage = 0;
    textureRecord = 0;

    if (g_zVideo_ActiveRendererPath != 0) {
        textureName = "SnowFX";
        softwareImage = zVid_Image::Create();
        zVid_Image::SetFormatCode(
            softwareImage,
            0x0b
        );
        char *const alphaMap =
            (char *)(malloc(kHudWeatherFxSnowTextureTexels));
        void *const surfacePixels =
            malloc(kHudWeatherFxSnowTextureTexels * sizeof(unsigned short));
        zVid_Image_SetPixels(
            softwareImage,
            surfacePixels,
            alphaMap
        );
        softwareImage->formatFlagsPacked |= 0x20;
        zVid_Image::SetSize(
            softwareImage,
            kHudWeatherFxSnowTextureWidth,
            kHudWeatherFxSnowTextureHeight
        );
        textureRecord = g_zVideo_pfnCreateTextureRecord(
            textureName,
            softwareImage,
            softwareImage->formatFlagsPacked & 0x02,
            1,
            1
        );
    }

}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfx-hudweatherfx-0x4bde40
 * @recoil-artifact defines .text recoil:function:0x4bde40: HudWeatherFx::~HudWeatherFx.
 * Purpose: Release particle buffers and renderer-backed weather texture resources.
 */
HudWeatherFx::~HudWeatherFx() {
    if (particleQuads != 0) {
        ::operator delete(particleQuads);
    }
    if (particlePositions[0] != 0) {
        ::operator delete(particlePositions[0]);
    }
    if (particlePositions[1] != 0) {
        ::operator delete(particlePositions[1]);
    }

    if (g_zVideo_ActiveRendererPath != ZVID_RENDERER_BACKEND_SOFTWARE) {
        if (textureRecord != 0) {
            g_zVideo_pfnTextureRecordDestroy(textureRecord);
        }
        if (softwareImage != 0) {
            zVid_Image::ReleaseIfNotDefault(softwareImage);
            softwareImage = 0;
        }
    }

}

/**

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfx-resetparticleslot
 * @recoil-artifact defines .text recoil:function:0x4bdee0: HudWeatherFx::ResetParticleSlot.
 * Purpose: Respawn one particle in the weather cone and copy it into the destination buffer.
 */
void HudWeatherFx::ResetParticleSlot(
    int particleIndex,
    int
) {
    zVec3 *const sourcePosition = &particlePositions[sourceBufferIndex][particleIndex];
    zVec3 *const destPosition = &particlePositions[destBufferIndex][particleIndex];

    sourcePosition->z = 0.5f - (float)(rand()) * -0.0000152592547f;

    sourcePosition->x = -1.0f - (float)(rand()) * -0.0000457777642f;
    if (sourcePosition->x < -sourcePosition->z) {
        sourcePosition->x -= -1.5f;
        sourcePosition->z = 1.5f - sourcePosition->z;
    }

    sourcePosition->y = -1.0f - (float)(rand()) * -0.0000457777642f;
    if (sourcePosition->y < -sourcePosition->z) {
        sourcePosition->y -= -1.5f;
        sourcePosition->z = 1.5f - sourcePosition->z;
    }

    *destPosition = *sourcePosition;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfx-applypass3
 * @recoil-artifact defines .text recoil:function:0x4bdfd0: HudWeatherFx::ApplyPass3.
 * Purpose: Draw software weather lines or submit hardware textured weather quads
 * through the pass-3 HUD element callback.
 */
void HudWeatherFx::ApplyPass3() {
    if (g_zVideo_ActiveRendererPath == ZVID_RENDERER_BACKEND_SOFTWARE) {
        zVideo_FxSurface::DrawColoredLinesBatch(
            (zVideoFxColoredLineRecord *)(particleQuads),
            particleCount,
            (zVidRect32 *)(clipRectOrNull)
        );
        return;
    }

    const int swSurfaceWasLocked = zVideo::GetSwSurfaceLockedFlag();
    if (swSurfaceWasLocked != 0) {
        zVideo::Dispatch_UnlockSwSurfaceState();
    }

    unsigned short *surfacePixels = (unsigned short *)(softwareImage->pixels);
    if (*surfacePixels != packedColor16) {
        char *surfaceAlphaMap = softwareImage->alphaMap;
        int alphaValue = 0;
        while (alphaValue < 4080) {
            *surfacePixels = packedColor16;
            ++surfacePixels;
            *surfaceAlphaMap = (char)(alphaValue >> 4);
            ++surfaceAlphaMap;
            alphaValue += 255;
        }
    }

    g_zVideo_pfnTextureRecordFinalizeUpload(
        textureRecord,
        0,
        softwareImage
    );
    zVideoD3D::SceneEnter();

    for (int particleIndex = 0; particleIndex < particleCount; ++particleIndex) {
        HudWeatherFxParticleQuad *particleQuad = &particleQuads[particleIndex];
        float xSlant = 0.0f;
        float ySlant = 0.0f;
        if (particleQuad->width > particleQuad->height) {
            xSlant = (float)(particleQuad->slantOffset);
        } else {
            ySlant = (float)(particleQuad->slantOffset);
        }

        const float depth = particlePositions[sourceBufferIndex][particleIndex].z;
        zVideo_XyzVertex clipVerts[4];
        zVideo_TexCoord texCoords[4];
        clipVerts[0].x = (float)(particleQuad->x);
        clipVerts[0].y = (float)(particleQuad->y);
        clipVerts[0].z = depth;
        texCoords[0].u = particleQuad->texCoordUStart;
        texCoords[0].v = 0.0f;

        clipVerts[1].x = (float)(particleQuad->x) + xSlant;
        clipVerts[1].y = (float)(particleQuad->y) + ySlant;
        clipVerts[1].z = depth;
        texCoords[1].u = particleQuad->texCoordUStart;
        texCoords[1].v = 0.0f;

        clipVerts[2].x = (float)(particleQuad->x + particleQuad->width) + xSlant;
        clipVerts[2].y = (float)(particleQuad->y + particleQuad->height) + ySlant;
        clipVerts[2].z = depth;
        texCoords[2].u = particleQuad->texCoordUEnd;
        texCoords[2].v = 0.0f;

        clipVerts[3].x = (float)(particleQuad->x + particleQuad->width);
        clipVerts[3].y = (float)(particleQuad->y + particleQuad->height);
        clipVerts[3].z = depth;
        texCoords[3].u = particleQuad->texCoordUEnd;
        texCoords[3].v = 0.0f;

        if (((HudWeatherFxPointBatch *)(clipVerts))
                ->ArePointBatchInsideRect(
                    4,
                    clipRectOrNull
                ) != 0) {
            g_zVideo_pfnSubmitPolyRenderClass(
                clipVerts,
                texCoords,
                4,
                (zVideo_RenderClass *)(textureRecord),
                1,
                1.0f,
                0
            );
        }
    }

    g_zVideo_pfnFlushSortedPolys();
    zVideoD3D::SceneLeave();
    if (swSurfaceWasLocked != 0) {
        zVideo::RunPostprocessOnSwBuffer();
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfxpointbatch-arepointbatchinsiderect
 * @recoil-artifact defines .text recoil:function:0x4be210: HudWeatherFx::ArePointBatchInsideRect.
 * Purpose: Accept a projected weather quad only when all points lie inside the viewport.
 */
int HudWeatherFxPointBatch::ArePointBatchInsideRect(
    int pointCount,
    const HudUiRect *viewportRect
) {
    if (viewportRect == 0 || pointCount <= 0) {
        return 1;
    }

    for (int index = 0; index < pointCount; ++index) {
        if (this[index].x < (float)(viewportRect->left)) {
            return 0;
        }
        if ((float)(viewportRect->right) < this[index].x) {
            return 0;
        }
        if (this[index].y < (float)(viewportRect->top)) {
            return 0;
        }
        if ((float)(viewportRect->bottom) < this[index].y) {
            return 0;
        }
    }

    return 1;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfxsnow-hudweatherfxsnow-0x4be280
 * @recoil-artifact defines .text recoil:function:0x4be280: HudWeatherFxSnow::HudWeatherFxSnow(int).
 * Purpose: Construct the shared weather emitter and initialize snow emitter defaults.
 */
HudWeatherFxSnow::HudWeatherFxSnow(
    int particleCount
) : HudWeatherFx(particleCount) {
    emitEnabled = 1;
    emitRadius = 20.0f;
    emitDepth = 400.0f;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfxsnow-hudweatherfxsnow-0x4be2e0
 * @recoil-artifact defines .text recoil:function:0x4be2e0: HudWeatherFxSnow::~HudWeatherFxSnow.
 * Purpose: Tear down the snow emitter and continue through the shared C++ base destructor.
 */
HudWeatherFxSnow::~HudWeatherFxSnow() {
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfxsnow-update
 * @recoil-artifact defines .text recoil:function:0x4be2f0: HudWeatherFxSnow::Update.
 * Purpose: Advance snow particles from camera drift, gravity, and wind, then project quads.
 */
void HudWeatherFxSnow::Update(
    float deltaSeconds
) {
    if ((flags & 0x10) != 0) {
        return;
    }

    g_HudWeatherFxSnow_TimeAccumulator += deltaSeconds;
    if (camera == 0) {
        return;
    }

    int viewportWidth = 0;
    int viewportHeight = 0;
    if (clipRectOrNull != 0) {
        viewportWidth = clipRectOrNull->right - clipRectOrNull->left;
        viewportHeight = clipRectOrNull->bottom - clipRectOrNull->top;
    } else {
        const zVidRect32 *const primaryRect = zVideo::GetPrimarySurfaceRectScratch();
        viewportWidth = primaryRect->right - primaryRect->left;
        viewportHeight = primaryRect->bottom - primaryRect->top;
    }
    const float viewportWidthF = (float)(viewportWidth);
    const float viewportHeightF = (float)(viewportHeight);

    zVec3 cameraTarget;
    zClass_Camera::gwCameraGetTarget(
        camera,
        &cameraTarget.x,
        &cameraTarget.y,
        &cameraTarget.z
    );

    zVec3 cameraAngles;
    zClass_Camera::gwCameraGetPosition(
        camera,
        &cameraAngles.x,
        &cameraAngles.y,
        &cameraAngles.z
    );

    zVec3 cameraTargetDrift;
    cameraTargetDrift.x =
        (g_HudWeatherFxSnow_LastCameraTarget.x - cameraTarget.x) * -0.100000001f;
    cameraTargetDrift.y =
        (g_HudWeatherFxSnow_LastCameraTarget.y - cameraTarget.y) * -0.100000001f;
    cameraTargetDrift.z =
        (g_HudWeatherFxSnow_LastCameraTarget.z - cameraTarget.z) * -0.100000001f;
    g_HudWeatherFxSnow_LastCameraTarget.x = cameraTarget.x;
    g_HudWeatherFxSnow_LastCameraTarget.y = cameraTarget.y;
    g_HudWeatherFxSnow_LastCameraTarget.z = cameraTarget.z;

    zMat4x3 slotBuffer;
    zMath::MatStackPushPtr((float *)(&slotBuffer));
    zMath::MatLoadIdentity();
    zMath::MatRotateX(-cameraAngles.x);
    zMath::MatRotateY(-cameraAngles.y);
    zMath::MatTransformPointBatchInPlace(
        &cameraTargetDrift,
        1
    );
    zMath::MatStackPopPtr();

    zMath::MatStackPushPtr((float *)(&slotBuffer));
    zMath::MatLoadIdentity();
    zMath::MatRotateZ(cameraAngles.z);
    zMath::MatRotateY(cameraAngles.y);
    zMath::MatRotateX(cameraAngles.x);

    zVec3 gravityOffset;
    const float gravityScale = (float)(gravity * 0.1);
    gravityOffset.x = basisVector.x * gravityScale;
    gravityOffset.y = basisVector.y * gravityScale;
    gravityOffset.z = basisVector.z * gravityScale;
    zMath::MatTransformPointBatchInPlace(
        &gravityOffset,
        1
    );

    zVec3 windOffset;
    const float windScale = (float)(windVelocity * 0.1);
    windOffset.x = (float)(sin(windDirection)) * windScale;
    windOffset.y = 0.0f;
    windOffset.z = (float)(cos(windDirection)) * windScale;
    zMath::MatTransformPointBatchInPlace(
        &windOffset,
        1
    );
    zMath::MatStackPopPtr();

    zVec3 particleVelocity;
    particleVelocity.x = cameraTargetDrift.x + gravityOffset.x + windOffset.x;
    particleVelocity.y = cameraTargetDrift.y + gravityOffset.y + windOffset.y;
    particleVelocity.z = cameraTargetDrift.z + gravityOffset.z + windOffset.z;
    if (HudWeatherFxVec3LengthSq(&particleVelocity) >= 1.0) {
        zMath::Vec3Normalize(&particleVelocity);
    }

    zVec3 probeVelocity = particleVelocity;
    if (HudWeatherFxVec3LengthSq(&probeVelocity) >= 0.010000000000000002) {
        zMath::Vec3Normalize(&probeVelocity);
        probeVelocity.x *= 0.100000001f;
        probeVelocity.y *= 0.100000001f;
        probeVelocity.z *= 0.100000001f;
    }

    for (int particleIndex = 0; particleIndex < particleCount; ++particleIndex) {
        const zVec3 *const sourcePosition = &particlePositions[sourceBufferIndex][particleIndex];
        zVec3 *const destPosition = &particlePositions[destBufferIndex][particleIndex];
        destPosition->x = sourcePosition->x + particleVelocity.x;
        destPosition->y = sourcePosition->y + particleVelocity.y;
        destPosition->z = sourcePosition->z + particleVelocity.z;

        zVec3 probePosition;
        probePosition.x = sourcePosition->x + probeVelocity.x;
        probePosition.y = sourcePosition->y + probeVelocity.y;
        probePosition.z = sourcePosition->z + probeVelocity.z;

        const float sourceDepthFactor = 1.5f - sourcePosition->z;
        const float probeDepthFactor = 1.5f - probePosition.z;
        HudWeatherFxParticleQuad *const particleQuad = &particleQuads[particleIndex];
        particleQuad->x =
            (int)(((probeDepthFactor * probePosition.x) - -0.5f) *
                  viewportWidthF);
        particleQuad->y =
            (int)(((probeDepthFactor * probePosition.y) - -0.5f) *
                  viewportHeightF);
        particleQuad->width =
            (int)(((sourceDepthFactor * sourcePosition->x) - -0.5f) *
                  viewportWidthF) -
            particleQuad->x;
        particleQuad->height =
            (int)(((sourceDepthFactor * sourcePosition->y) - -0.5f) *
                  viewportHeightF) -
            particleQuad->y;
        particleQuad->color16 = packedColor16;
        particleQuad->texCoordUStart = probeDepthFactor * alphaStartScale;
        particleQuad->texCoordUEnd = sourceDepthFactor * alphaEndScale;
        particleQuad->slantOffset = (int)(((float)(activeParticleCount + 1)) * sourceDepthFactor *
                                          3.5);

        if (HudWeatherFxSnowNeedsReset(destPosition) != 0) {
            ResetParticleSlot(
                particleIndex,
                0
            );
        }
    }

    HudUiElement::Update(deltaSeconds);

    const int oldSourceBufferIndex = sourceBufferIndex;
    sourceBufferIndex = destBufferIndex;
    destBufferIndex = oldSourceBufferIndex;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfxrain-hudweatherfxrain-0x4be810
 * @recoil-artifact defines .text recoil:function:0x4be810: HudWeatherFxRain::HudWeatherFxRain(int).
 * Purpose: Construct the shared weather emitter and initialize rain emitter defaults.
 */
HudWeatherFxRain::HudWeatherFxRain(
    int particleCount
) : HudWeatherFx(particleCount) {
    emitEnabled = 1;
    emitRadius = 20.0f;
    emitDepth = 400.0f;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfxrain-hudweatherfxrain-0x4be870
 * @recoil-artifact defines .text recoil:function:0x4be870: HudWeatherFxRain::~HudWeatherFxRain.
 * Purpose: Tear down the rain emitter and continue through the shared C++ base destructor.
 */
HudWeatherFxRain::~HudWeatherFxRain() {
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfxrain-update
 * @recoil-artifact defines .text recoil:function:0x4be880: HudWeatherFxRain::Update.
 * Purpose: Advance rain particles from camera drift, gravity, and wind, then project quads.
 */
void HudWeatherFxRain::Update(
    float deltaSeconds
) {
    if ((flags & 0x10) != 0) {
        return;
    }

    g_HudWeatherFxRain_TimeAccumulator += deltaSeconds;
    if (camera == 0) {
        return;
    }

    int viewportWidth = 0;
    int viewportHeight = 0;
    if (clipRectOrNull != 0) {
        viewportWidth = clipRectOrNull->right - clipRectOrNull->left;
        viewportHeight = clipRectOrNull->bottom - clipRectOrNull->top;
    } else {
        const zVidRect32 *const primaryRect = zVideo::GetPrimarySurfaceRectScratch();
        viewportWidth = primaryRect->right - primaryRect->left;
        viewportHeight = primaryRect->bottom - primaryRect->top;
    }
    const float viewportWidthF = (float)(viewportWidth);
    const float viewportHeightF = (float)(viewportHeight);

    zVec3 cameraTarget;
    zClass_Camera::gwCameraGetTarget(
        camera,
        &cameraTarget.x,
        &cameraTarget.y,
        &cameraTarget.z
    );

    zVec3 cameraAngles;
    zClass_Camera::gwCameraGetPosition(
        camera,
        &cameraAngles.x,
        &cameraAngles.y,
        &cameraAngles.z
    );

    zVec3 cameraTargetDrift;
    cameraTargetDrift.x =
        (g_HudWeatherFxRain_LastCameraTarget.x - cameraTarget.x) * -0.100000001f;
    cameraTargetDrift.y =
        (g_HudWeatherFxRain_LastCameraTarget.y - cameraTarget.y) * -0.100000001f;
    cameraTargetDrift.z =
        (g_HudWeatherFxRain_LastCameraTarget.z - cameraTarget.z) * -0.100000001f;
    g_HudWeatherFxRain_LastCameraTarget.x = cameraTarget.x;
    g_HudWeatherFxRain_LastCameraTarget.y = cameraTarget.y;
    g_HudWeatherFxRain_LastCameraTarget.z = cameraTarget.z;

    zMat4x3 slotBuffer;
    zMath::MatStackPushPtr((float *)(&slotBuffer));
    zMath::MatLoadIdentity();
    zMath::MatRotateX(-cameraAngles.x);
    zMath::MatRotateY(-cameraAngles.y);
    zMath::MatTransformPointBatchInPlace(
        &cameraTargetDrift,
        1
    );
    zMath::MatStackPopPtr();

    zMath::MatStackPushPtr((float *)(&slotBuffer));
    zMath::MatLoadIdentity();
    zMath::MatRotateZ(cameraAngles.z);
    zMath::MatRotateY(cameraAngles.y);
    zMath::MatRotateX(cameraAngles.x);

    zVec3 gravityOffset;
    const float gravityScale = (float)(gravity * 0.1);
    gravityOffset.x = basisVector.x * gravityScale;
    gravityOffset.y = basisVector.y * gravityScale;
    gravityOffset.z = basisVector.z * gravityScale;
    zMath::MatTransformPointBatchInPlace(
        &gravityOffset,
        1
    );

    zVec3 windOffset;
    const float windScale = (float)(windVelocity * 0.1);
    windOffset.x = (float)(sin(windDirection)) * windScale;
    windOffset.y = 0.0f;
    windOffset.z = (float)(cos(windDirection)) * windScale;
    zMath::MatTransformPointBatchInPlace(
        &windOffset,
        1
    );
    zMath::MatStackPopPtr();

    zVec3 particleVelocity;
    particleVelocity.x = cameraTargetDrift.x + gravityOffset.x + windOffset.x;
    particleVelocity.y = cameraTargetDrift.y + gravityOffset.y + windOffset.y;
    particleVelocity.z = cameraTargetDrift.z + gravityOffset.z + windOffset.z;
    if (HudWeatherFxVec3LengthSq(&particleVelocity) >= 1.0) {
        zMath::Vec3Normalize(&particleVelocity);
    }

    zVec3 probeVelocity = particleVelocity;
    if (HudWeatherFxVec3LengthSq(&probeVelocity) >= 0.010000000000000002) {
        zMath::Vec3Normalize(&probeVelocity);
        probeVelocity.x *= 0.100000001f;
        probeVelocity.y *= 0.100000001f;
        probeVelocity.z *= 0.100000001f;
    }

    for (int particleIndex = 0; particleIndex < particleCount; ++particleIndex) {
        const zVec3 *const sourcePosition = &particlePositions[sourceBufferIndex][particleIndex];
        zVec3 *const destPosition = &particlePositions[destBufferIndex][particleIndex];
        destPosition->x = sourcePosition->x + particleVelocity.x;
        destPosition->y = sourcePosition->y + particleVelocity.y;
        destPosition->z = sourcePosition->z + particleVelocity.z;

        zVec3 probePosition;
        probePosition.x = sourcePosition->x + probeVelocity.x;
        probePosition.y = sourcePosition->y + probeVelocity.y;
        probePosition.z = sourcePosition->z + probeVelocity.z;

        const float sourceDepthFactor = 1.5f - sourcePosition->z;
        const float probeDepthFactor = 1.5f - probePosition.z;
        HudWeatherFxParticleQuad *const particleQuad = &particleQuads[particleIndex];
        particleQuad->x =
            (int)(((probeDepthFactor * probePosition.x) - -0.5f) *
                  viewportWidthF);
        particleQuad->y =
            (int)(((probeDepthFactor * probePosition.y) - -0.5f) *
                  viewportHeightF);
        particleQuad->width =
            (int)(((sourceDepthFactor * sourcePosition->x) - -0.5f) *
                  viewportWidthF) -
            particleQuad->x;
        particleQuad->height =
            (int)(((sourceDepthFactor * sourcePosition->y) - -0.5f) *
                  viewportHeightF) -
            particleQuad->y;
        particleQuad->color16 = packedColor16;
        particleQuad->texCoordUStart = probeDepthFactor * alphaStartScale;
        particleQuad->texCoordUEnd = sourceDepthFactor * alphaEndScale;
        particleQuad->slantOffset = kHudWeatherFxRainSlantDelta;

        ResetParticleSlot(
            particleIndex,
            0
        );
    }

    HudUiElement::Update(deltaSeconds);

    const int oldSourceBufferIndex = sourceBufferIndex;
    sourceBufferIndex = destBufferIndex;
    destBufferIndex = oldSourceBufferIndex;
}


namespace zVideo {

/**
 * Purpose: provide the provisional local pass-3 update helper while retail
 * source placement remains unresolved.
 */
void __fastcall zVideoFxPass3Config_UpdateLocal(
    zVideoFxPass3Config *config,
    float deltaTime
) {
    config->HudUiContainer::UpdateAll(deltaTime);
    config->slotWriteIndex = 0;
}

/**
 * Purpose: provide the provisional local pass-3 primary-element helper while
 * retail source placement remains unresolved.
 */
void __fastcall zVideoFxPass3Config_SetPrimaryElementParamsLocal(
    zVideoFxPass3Config *config,
    unsigned int packedColor,
    double primaryAlpha
) {
    config->rootElement.packedColor16 = (unsigned short)(packedColor);
    config->rootElement.alpha = primaryAlpha;
    config->rootElement.SetVisible(1);
    config->rootElement.timer = 0.0f;
    config->rootElement.flags |= 0x01u;
}

/**
 * Purpose: provide the provisional local pass-3 queue helper while retail
 * source placement remains unresolved.
 */
void __fastcall zVideoFxPass3Config_QueueElementLocal(
    zVideoFxPass3Config *config,
    int rectLeftPixels,
    int rectTopPixels,
    int currentRadiusPixels,
    int maxRadiusPixels,
    int extentPixels,
    float sinFreq,
    float sinPhase
) {
    const int slotIndex = config->slotWriteIndex;
    zVideoFxPass3Slot *const slot = &config->slots[slotIndex];
    if (slotIndex < 4) {
        config->slotWriteIndex = slotIndex + 1;
    }

    slot->SetRectAndPayload(
        rectLeftPixels,
        rectTopPixels,
        currentRadiusPixels,
        maxRadiusPixels,
        extentPixels,
        sinFreq,
        sinPhase
    );
    slot->SetVisible(1);
    slot->timer = 0.0f;
    slot->flags |= 0x01u;
}

} // namespace zVideo

/**
 * Purpose: store a provisional pass-3 input rectangle while retail source
 * placement remains unresolved.
 */
void zVideoFxPass3Config::SetInputRectByIndex(
    int index,
    HudUiRect *rectOrNull
) {
    if (index < 2) {
        inputRectsOrNull[index] = rectOrNull;
    }
}

/**
 * Purpose: store provisional raw pass-3 surface input while retail source
 * placement remains unresolved.
 */
void zVideoFxPass3Config::QueuePrimitiveRaw(
    void *primitive,
    int width,
    int height,
    int pitchBytes
) {
    surfacePixels = (unsigned short *)(primitive);
    surfaceWidth = width;
    surfaceHeight = height;
    surfacePitchBytes = pitchBytes;
}

/**
 * Purpose: provide the provisional pass-3 configuration destructor while
 * retail source placement remains unresolved.
 */
zVideoFxPass3Config::~zVideoFxPass3Config() {
}

namespace zVideo {

/**
 * Purpose: relay provisional local pass-3 primary-element state while retail
 * source placement remains unresolved.
 */
void __fastcall FxPass3_SetPrimaryElementParamsLocal(
    unsigned int packedColor,
    double primaryAlpha
) {
    zVideoFxPass3Config_SetPrimaryElementParamsLocal(
        &g_zVideo_FxPass3ConfigLocal,
        packedColor,
        primaryAlpha
    );
}

/**
 * Purpose: relay provisional local pass-3 queue state while retail source
 * placement remains unresolved.
 */
void __fastcall FxPass3_QueueElementLocal(
    int rectLeftPixels,
    int rectTopPixels,
    int currentRadiusPixels,
    int maxRadiusPixels,
    int extentPixels,
    float sinFreq,
    float sinPhase
) {
    zVideoFxPass3Config_QueueElementLocal(
        &g_zVideo_FxPass3ConfigLocal,
        rectLeftPixels,
        rectTopPixels,
        currentRadiusPixels,
        maxRadiusPixels,
        extentPixels,
        sinFreq,
        sinPhase
    );
}

/**
 * Purpose: relay a provisional local pass-3 input rectangle while retail
 * source placement remains unresolved.
 */
void __fastcall FxPass3_SetInputRectByIndex(
    int index,
    HudUiRect *rectOrNull
) {
    g_zVideo_FxPass3ConfigLocal.SetInputRectByIndex(
        index,
        rectOrNull
    );
}

/**
 * Purpose: relay provisional raw pass-3 surface input while retail source
 * placement remains unresolved.
 */
void __fastcall FxPass3_QueuePrimitive(
    void *primitive,
    int width,
    int height,
    int pitchBytes
) {
    g_zVideo_FxPass3ConfigLocal.QueuePrimitiveRaw(
        primitive,
        width,
        height,
        pitchBytes
    );
}

/**
 * Purpose: relay the provisional local pass-3 update while retail source
 * placement remains unresolved.
 */
void __fastcall FxPass3_UpdateLocal(
    float deltaTime
) {
    zVideoFxPass3Config_UpdateLocal(
        &g_zVideo_FxPass3ConfigLocal,
        deltaTime
    );
}

} // namespace zVideo

/**
 * Purpose: provide the provisional pass-3 configuration constructor while
 * retail source placement remains unresolved.
 */
zVideoFxPass3Config::zVideoFxPass3Config() {
    rootElement.HudUiElement::Constructor(
        0,
        0
    );
    rootElement.clipRectOrNull = 0;
    new (&rootElement) zVideoFxPass3RootElement;

    int slotIndex;
    inputRectsOrNull[0] = 0;
    inputRectsOrNull[1] = 0;
    surfacePixels = 0;
    surfaceWidth = 0;
    surfaceHeight = 0;

    HudUiContainer::AddChild((HudUiElement *)(&rootElement));
    rootElement.SetVisible(0);

    for (slotIndex = 0; slotIndex < 5; ++slotIndex) {
        HudUiContainer::AddChild((HudUiElement *)(&slots[slotIndex]));
        slots[slotIndex].SetVisible(0);
    }

    slotWriteIndex = 0;
    HudUiContainer::SetEnabled(1);
}

extern char k_msgBoxWidgetName_Message[8];
extern char k_msgBoxWidgetName_Title[6];
extern char k_msgBoxWidgetName_Cancel[10];
extern char k_msgBoxWidgetName_OK[6];

/**
 * Recovered original-source helper, no standalone retail function.
 * Observed caller: 0x4bf060 HudUiMessageBoxDialog::Constructor.
 * Purpose: divide signed fallback layout coordinates by a power of two with the
 * same toward-zero correction pattern emitted in the message-box constructor.
 */
static inline int HudUiDialogSignedDivPow2(
    int value,
    int shift
) {
    const int signMask = value >> 31;
    return (value + (signMask & ((1 << shift) - 1))) >> shift;
}

/**
 * Recovered original-source helper, no standalone retail function.
 * Observed caller: 0x4bf060 HudUiMessageBoxDialog::Constructor.
 * Purpose: allocate a 16-bit solid-color zVid image for the message-box
 * fallback path when no ZRD layout section is supplied.
 */
static inline zVidImagePartial *HudUiMessageBoxCreateSolidImage(
    int width,
    int height,
    unsigned short color565
) {
    zVidImagePartial *const image = zVid_Image::Create();
    zVid_Image::SetFormatCode(
        image,
        1
    );
    zVid_Image::SetSize(
        image,
        (short)(width),
        (short)(height)
    );

    void *const pixels = malloc(zVid_Image::QueryBytesPerPixel(image) * width * height);
    zVid_Image_SetPixels(
        image,
        pixels,
        0
    );

    unsigned short *const pixelWords = (unsigned short *)(pixels);
    for (int index = 0; index < image->pixelCount; ++index) {
        pixelWords[index] = color565;
    }

    return image;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduimessageboxdialog-constructor
 * @recoil-artifact defines .text recoil:function:0x4bf060: HudUiMessageBoxDialog::Constructor.
 * Source model: class-first constructor for HudUiMessageBoxDialog; BN table
 * 0x4d4028 is installed at object offset zero by the constructed C++ object.
 * Purpose: bind the ZRD-backed message-box widgets, or build the original
 * solid-image fallback dialog and child widget graph.
 * Touched data: owns runtime image pointers only; dialog/button table globals
 * are class identity evidence, not separately promoted data.
 */
HudUiMessageBoxDialog * HudUiMessageBoxDialog::Constructor(
    const char *zrdPath,
    const char *sectionName
) {
    new ((HudUiBackground *)this) HudUiBackground;
    backdropWidget.Constructor(0);
    messagePanel.ConstructorDefault(
        0,
        0,
        0
    );
    titlePanel.ConstructorDefault(
        0,
        0,
        0
    );
    okButton.Constructor();
    cancelButton.Constructor();
    const zVidRect32 *const primaryRect = zVideo::GetPrimarySurfaceRectScratch();
    blitRect = *primaryRect;

    if (zrdPath != 0 && sectionName != 0) {
        backgroundImage = 0;
        okButtonNormalImage = 0;
        okButtonPressedImage = 0;

        zReader::Node *const loadedSection = LoadFromZrd(
            zrdPath,
            sectionName,
            0
        );
        if (loadedSection != 0) {
            BindWidgetByName(
                loadedSection,
                &okButton,
                k_msgBoxWidgetName_OK
            );
            BindWidgetByName(
                loadedSection,
                &cancelButton,
                k_msgBoxWidgetName_Cancel
            );
            BindPrimitiveNodeToElement(
                loadedSection,
                &titlePanel,
                k_msgBoxWidgetName_Title
            );
            BindPrimitiveNodeToElement(
                loadedSection,
                &messagePanel,
                k_msgBoxWidgetName_Message
            );
        }

        FreeLoadedTreeRoots(0);
        titlePanel.SetVisible(1);
        messagePanel.SetVisible(1);
        return this;
    }

    const int centerX = HudUiDialogSignedDivPow2(
        blitRect.right,
        1
    );
    const int centerY = HudUiDialogSignedDivPow2(
        blitRect.bottom,
        1
    );
    fallbackWidth = 300;
    fallbackHeight = 200;

    backgroundImage = HudUiMessageBoxCreateSolidImage(
        fallbackWidth,
        fallbackHeight,
        (unsigned short)(zVid_PackColorRGB(
            128,
            128,
            128
        ))
    );

    const int buttonWidth = HudUiDialogSignedDivPow2(
        fallbackWidth,
        2
    );
    const int buttonHeight = HudUiDialogSignedDivPow2(
        fallbackHeight,
        2
    );
    okButtonNormalImage = HudUiMessageBoxCreateSolidImage(
        buttonWidth,
        buttonHeight,
        (unsigned short)(zVid_PackColorRGB(
            192,
            192,
            192
        ))
    );
    okButtonPressedImage = HudUiMessageBoxCreateSolidImage(
        buttonWidth,
        buttonHeight,
        (unsigned short)(zVid_PackColorRGB(
            160,
            192,
            160
        ))
    );

    backdropWidget.SetImageBorrowedAndInvalidate(backgroundImage);
    messagePanel.SetTextFmt("");
    titlePanel.SetTextFmt("");
    okButton.LoadFromZrd(
        0,
        this
    );
    okButton.defaultImage = okButton.SetImageBorrowedAndInvalidate(okButtonNormalImage);
    okButton.rolloverImage = okButtonPressedImage;

    backdropWidget.SetPos(
        centerX - 150,
        centerY - 100
    );
    titlePanel.SetPos(
        centerX - 140,
        centerY - 90
    );
    messagePanel.SetPos(
        centerX - 140,
        centerY - 70
    );
    okButton.SetPos(
        centerX - 150 + HudUiDialogSignedDivPow2(
            fallbackWidth,
            1
        ) -
            HudUiDialogSignedDivPow2(
                fallbackWidth,
                3
            ),
        centerY - 100 - HudUiDialogSignedDivPow2(
            fallbackHeight,
            2
        ) + fallbackHeight - 10
    );

    AddChild(&backdropWidget);
    AddChild(&messagePanel);
    AddChild(&titlePanel);
    AddChild(&okButton);
    messagePanel.SetVisible(1);
    titlePanel.SetVisible(1);
    okButton.SetVisible(0);
    SetChildFlags(0);
    return this;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduimessageboxdialog-destructor
 * @recoil-artifact defines .text recoil:function:0x4bf560: HudUiMessageBoxDialog::Destructor.
 * Source model: HudUiMessageBoxDialog class destructor; BN shows the dialog
 * table 0x4d4028 at offset zero for this owner.
 * Purpose: release fallback images and tear down message-box child widgets in
 * the recovered member cleanup order.
 * Touched data: no authored globals; releases runtime-owned image storage.
 */
void HudUiMessageBoxDialog::Destructor() {
    if (backgroundImage != 0) {
        if (backgroundImage->pixels != 0) {
            free(backgroundImage->pixels);
            backgroundImage->pixels = 0;
        }

        zVid_Image::Destroy(backgroundImage);
        backgroundImage = 0;
    }

    cancelButton.DestructorCore();
    okButton.DestructorCore();
    titlePanel.~HudUiPanel();
    messagePanel.~HudUiPanel();
    backdropWidget.DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduimessageboxdialog-runmodal
 * @recoil-artifact defines .text recoil:function:0x4bf630: HudUiMessageBoxDialog::RunModal.
 * Source model: direct HudUiMessageBoxDialog method called by
 * HudUi::ShowMessageBox, not a dialog-table slot.
 * Purpose: switch render/input state to modal drawing, pump frames until the
 * dialog records a result, then restore the previous framebuffer region.
 * Touched data: direct globals are renderer/time owners already linked by the
 * parent handoff; no dialog-owned plan-tracked data is promoted here.
 */
int HudUiMessageBoxDialog::RunModal(
    const char *messageText,
    const char *titleText,
    void *modalContext,
    float timeoutSeconds
) {
    (void)modalContext;
    (void)timeoutSeconds;

    if (g_zVideo_ActiveRendererPath != 0) {
        g_zVideo_pfnBltSwToPrimaryRectDirect(
            0,
            0
        );
    }

    const int previousHalfResMode = zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);
    HudUi::SetInvalidateMode(0);

    zVidRect32 previousRegionRect = {0, 0, 0, 0};
    int previousBitsPerPixel = 0;
    int previousPitchBytes = 0;
    void *const previousPixels = zRndr::GetActiveRegionState(
        &previousRegionRect.right,
        &previousRegionRect.bottom,
        &previousBitsPerPixel,
        &previousPitchBytes
    );

    int dialogPitchBytes;
    int dialogBitsPerPixel;
    void *dialogPixels;
    if (g_zVideo_ActiveRendererPath == 0) {
        dialogPitchBytes = zVideo::GetSwSurfacePitch();
        dialogBitsPerPixel = zVideo::GetDisplayModeBpp();
        dialogPixels = zVideo::GetSwSurfacePixels();
    } else {
        dialogPitchBytes = zVideo::GetPrimarySurfacePitch();
        dialogBitsPerPixel = zVideo::GetDisplayModeBpp();
        dialogPixels = zVideo::GetPrimarySurfacePixels();
    }

    zRndr::SetFrameBufferRegion(
        dialogPixels,
        (zOpt_ViewRectSection *)(&blitRect),
        dialogBitsPerPixel,
        dialogPitchBytes
    );

    modalResult = 0;
    modalFrameCountdown = 100000;
    SetEnabled(1);
    messagePanel.SetTextFmt(messageText);
    titlePanel.SetTextFmt(titleText);
    okButton.SetVisible(1);

    int framesRemaining = modalFrameCountdown;
    modalFrameCountdown = framesRemaining - 1;
    while (framesRemaining > 0) {
        zInput::PollActiveDevices(0);
        Time::Tick();
        zVideo::RunPostprocessOnPrimaryBuffer();
        UpdateAll(g_FrameDeltaTimeSec);
        zVideo::Dispatch_UnlockPrimarySurfaceState();
        zVideo::AdjustSurfacesIfEnabled(
            &blitRect,
            &blitRect,
            1,
            1
        );
        framesRemaining = modalFrameCountdown;
        modalFrameCountdown = framesRemaining - 1;
    }

    ((HudUiDialogController *)(this))->BlitOwnedSurfaceToPrimary();
    SetEnabled(0);
    zVideo::SetHalfResAdjustMode(previousHalfResMode);
    HudUi::SetInvalidateMode(previousHalfResMode);
    zRndr::SetFrameBufferRegion(
        previousPixels,
        (zOpt_ViewRectSection *)(&previousRegionRect),
        previousBitsPerPixel,
        previousPitchBytes
    );
    return modalResult;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduimessageboxdialog-onok
 * @recoil-artifact defines .text recoil:function:0x4bf7c0: HudUiMessageBoxDialog::OnOk.
 * Source model: HudUiMessageBoxDialog table slot +0x0c in table 0x4d4028.
 * Purpose: accept the modal dialog and force the modal loop to exit.
 * Touched data: no authored globals; writes dialog modal fields only.
 */
void HudUiMessageBoxDialog::OnOk() {
    modalResult = 1;
    modalFrameCountdown = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduimessageboxdialog-oncancel
 * @recoil-artifact defines .text recoil:function:0x4bf7e0: HudUiMessageBoxDialog::OnCancel.
 * Source model: HudUiMessageBoxDialog table slot +0x10 in table 0x4d4028.
 * Purpose: cancel the modal dialog and force the modal loop to exit.
 * Touched data: no authored globals; writes dialog modal fields only.
 */
void HudUiMessageBoxDialog::OnCancel() {
    modalResult = 2;
    modalFrameCountdown = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduimessageboxokbutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x4bf800: HudUiMessageBoxOkButton::OnActivate.
 * Source model: HudUiMessageBoxOkButton activation override; button table
 * 0x4d40c8 overrides slot +0x30 with this method.
 * Purpose: dispatch through the owner dialog to OnOk, then run the base
 * HudUiZrdWidget activation behavior.
 * Touched data: no authored globals; owner vptr dispatch reaches the dialog
 * table slot before HudUiZrdWidget::OnActivate.
 */
void HudUiMessageBoxOkButton::OnActivate() {
    HudUiMessageBoxDialog *const dialog = (HudUiMessageBoxDialog *)(owner);
    dialog->OnOk();

    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduimessageboxcancelbutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x4bf820: HudUiMessageBoxCancelButton::OnActivate.
 * Source model: HudUiMessageBoxCancelButton activation override; button table
 * 0x4d4040 overrides slot +0x30 with this method.
 * Purpose: dispatch through the owner dialog to OnCancel, then run the base
 * HudUiZrdWidget activation behavior.
 * Touched data: no authored globals; owner vptr dispatch reaches the dialog
 * table slot before HudUiZrdWidget::OnActivate.
 */
void HudUiMessageBoxCancelButton::OnActivate() {
    HudUiMessageBoxDialog *const dialog = (HudUiMessageBoxDialog *)(owner);
    dialog->OnCancel();

    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduipolyline-huduipolyline
 * @recoil-artifact defines .text recoil:function:0x4bf840: HudUiPolyline::HudUiPolyline.
 * Purpose: preserve the recovered HUD behavior for HudUiPolyline::HudUiPolyline.
 */
HudUiPolyline::HudUiPolyline()
    : HudUiElement(
          0,
          0
      ) {
    pointCount = 0;
    memset(
        points,
        0,
        sizeof(points)
    );
    Invalidate();
    clipRect = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduipolyline-setpoint
 * @recoil-artifact defines .text recoil:function:0x4bf8b0: HudUiPolyline::SetPoint.
 * Purpose: apply the recovered HUD state change handled by HudUiPolyline::SetPoint.
 */
void HudUiPolyline::SetPoint(
    int index,
    int pointX,
    int pointY
) {
    points[index].x = pointX;
    points[index].y = pointY;

    if (pointCount <= index) {
        pointCount = index + 1;
    }

    if (index == 0) {
        SetPos(
            pointX,
            pointY
        );
    }

    Invalidate();
}

/**
 * Purpose: preserve the recovered HUD behavior for HudUiPolyline::Draw.
 */
void HudUiPolyline::Draw() {
    DrawBase();

    const int currentPointCount = pointCount;
    if (currentPointCount == 0) {
        return;
    }

    if (clipRect != 0) {
        zRndr_DrawClippedImmediateLineStrip(
            (const zRndr_LinePoint2I *)(points),
            currentPointCount - 1,
            clipRect,
            color565
        );
        return;
    }

    {
        for (int index = 0; index < currentPointCount - 1; ++index) {
            const HudUiPolylinePoint &point = points[index];
            const HudUiPolylinePoint &nextPoint = points[index + 1];
            zRndr_DrawImmediateLine(
                point.x,
                point.y,
                nextPoint.x,
                nextPoint.y,
                color565
            );
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-huduibackgroundcursorwidget-0x4bf980
 * @recoil-artifact defines .text recoil:function:0x4bf980: HudUiBackgroundCursorWidget::HudUiBackgroundCursorWidget.
 * Purpose: preserve the recovered HUD behavior for HudUiBackgroundCursorWidget::HudUiBackgroundCursorWidget.
 */
HudUiBackgroundCursorWidget::HudUiBackgroundCursorWidget(
    const char *imagePath,
    int initCaptureEnabled
) : HudUiWidget(0) {
    captureEnabled = initCaptureEnabled;
    capturedImage = 0;
    if (imagePath != 0) {
        SetImageByPathOwnedAndRefresh(imagePath);
    }

    reservedC8 = 0;
    reservedCC = 0;
    captureSourceSelector = 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-huduibackgroundcursorwidget-0x4bfa20
 * @recoil-artifact defines .text recoil:function:0x4bfa20: HudUiBackgroundCursorWidget::~HudUiBackgroundCursorWidget.
 * Purpose: restore the cursor widget dispatch state, release a captured image, and tear down the widget base.
 */
HudUiBackgroundCursorWidget::~HudUiBackgroundCursorWidget() {
    if (capturedImage != 0) {
        zVid_Image::Destroy(capturedImage);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-setimagebypathownedandrefresh
 * @recoil-artifact defines .text recoil:function:0x4bfa50: HudUiBackgroundCursorWidget::SetImageByPathOwnedAndRefresh.
 * Purpose: apply the recovered HUD state change handled by HudUiBackgroundCursorWidget::SetImageByPathOwnedAndRefresh.
 */
void HudUiBackgroundCursorWidget::SetImageByPathOwnedAndRefresh(
    const char *imagePath
) {
    if (HudUiWidget::SetImageByPathOwned(imagePath) != 0) {
        SetImageBorrowedAndRefresh();
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-setimageborrowedandrefreshifchanged
 * @recoil-artifact defines .text recoil:function:0x4bfa70: HudUiBackgroundCursorWidget::SetImageBorrowedAndRefreshIfChanged.
 * Purpose: apply the recovered HUD state change handled by HudUiBackgroundCursorWidget::SetImageBorrowedAndRefreshIfChanged.
 */
void HudUiBackgroundCursorWidget::SetImageBorrowedAndRefreshIfChanged(
    zVidImagePartial *image
) {
    if (HudUiWidget::SetImageBorrowedAndInvalidate(image) != 0) {
        SetImageBorrowedAndRefresh();
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-setimageownedandrefresh
 * @recoil-artifact defines .text recoil:function:0x4bfa90: HudUiBackgroundCursorWidget::SetImageOwnedAndRefresh.
 * Purpose: apply the recovered HUD state change handled by HudUiBackgroundCursorWidget::SetImageOwnedAndRefresh.
 */
void HudUiBackgroundCursorWidget::SetImageOwnedAndRefresh(
    int newCaptureEnabled
) {
    captureEnabled = newCaptureEnabled;
    if (newCaptureEnabled == 0 && capturedImage != 0) {
        zVid_Image::Destroy(capturedImage);
        capturedImage = 0;
        HudUiElement::SetBltSourceAndClipRect(
            0,
            0
        );
        return;
    }

    if (capturedImage == 0) {
        SetImageBorrowedAndRefresh();
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-setimageborrowedandrefresh
 * @recoil-artifact defines .text recoil:function:0x4bfae0: HudUiBackgroundCursorWidget::SetImageBorrowedAndRefresh.
 * Purpose: apply the recovered HUD state change handled by HudUiBackgroundCursorWidget::SetImageBorrowedAndRefresh.
 */
void HudUiBackgroundCursorWidget::SetImageBorrowedAndRefresh() {
    if (captureEnabled == 0 || image == 0) {
        return;
    }

    if (capturedImage != 0) {
        zVid_Image::Destroy(capturedImage);
    }

    capturedImage = zVid_Image::Create();
    if (capturedImage == 0) {
        return;
    }

    zVid_Image::SetSize(
        capturedImage,
        image->width,
        image->height
    );
    void *const pixels = malloc((size_t)(capturedImage->pixelCount) * sizeof(unsigned short));
    zVid_Image_SetPixels(
        capturedImage,
        pixels,
        0
    );
    capturedImage->formatFlagsPacked = (unsigned char)(capturedImage->formatFlagsPacked | 0x20u);

    const int y = HudUiElement::GetCenterY();
    const int x = HudUiElement::GetCenterX();
    RebuildCapturedImage(
        x,
        y
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-setpos
 * @recoil-artifact defines .text recoil:function:0x4bfb70: HudUiBackgroundCursorWidget::SetPos.
 * Purpose: apply the recovered HUD state change handled by HudUiBackgroundCursorWidget::SetPos.
 */
void HudUiBackgroundCursorWidget::SetPos(
    int newX,
    int newY
) {
    HudUiWidget::SetPos(
        newX,
        newY
    );
    RebuildCapturedImage(
        x,
        y
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-rebuildcapturedimage
 * @recoil-artifact defines .text recoil:function:0x4bfba0: HudUiBackgroundCursorWidget::RebuildCapturedImage.
 * Purpose: preserve the recovered HUD behavior for HudUiBackgroundCursorWidget::RebuildCapturedImage.
 */
void HudUiBackgroundCursorWidget::RebuildCapturedImage(
    int originX,
    int originY
) {
    if (capturedImage == 0) {
        return;
    }

    zVidRect32 sourceRect;
    sourceRect.left = originX;
    sourceRect.top = originY;
    sourceRect.right = originX + image->width;
    sourceRect.bottom = originY + image->height;

    if (zVideo_buff::CopySurfaceRectToImage(captureSourceSelector, &sourceRect, capturedImage) !=
        0) {
        const HudUiRect clipRect = {sourceRect.left - originX,
            sourceRect.top - originY,
            sourceRect.right - originX,
            sourceRect.bottom - originY};
        SetBltSourceAndClipRect(
            capturedImage,
            &clipRect
        );
        return;
    }

    SetBltSourceAndClipRect(
        0,
        0
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-draw
 * @recoil-artifact defines .text recoil:function:0x4bfc50: HudUiBackgroundCursorWidget::Draw.
 * Purpose: preserve the recovered HUD behavior for HudUiBackgroundCursorWidget::Draw.
 */
void HudUiBackgroundCursorWidget::Draw() {
    HudUiWidget::Draw();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-drawbase
 * @recoil-artifact defines .text recoil:function:0x4bfc60: HudUiBackgroundCursorWidget::DrawBase.
 * Purpose: preserve the recovered HUD behavior for HudUiBackgroundCursorWidget::DrawBase.
 */
inline void HudUiBackgroundCursorWidget::DrawBase() {
    if (bltSource != 0) {
        zVid_Image::BlitToActiveTarget(
            (zVidImagePartial *)(bltSource),
            x,
            y,
            0,
            (zVidRect32 *)(&clipRect)
        );
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundvideowidget-huduibackgroundvideowidget
 * @recoil-artifact defines .text recoil:function:0x4bfc80: HudUiBackgroundVideoWidget::HudUiBackgroundVideoWidget.
 * Purpose: Initializes the background video element state before a stream is assigned.
 */
HudUiBackgroundVideoWidget::HudUiBackgroundVideoWidget()
    : HudUiElement(0, 0) {
    mediaPath[0] = '\0';
    stream = 0;
    elapsedTimeSec = 0.0f;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4bfc80 HudUiBackgroundVideoWidget::HudUiBackgroundVideoWidget callers.
 * Purpose: run the recovered HudUiBackgroundVideoWidget::~HudUiBackgroundVideoWidget teardown path.
 */
HudUiBackgroundVideoWidget::~HudUiBackgroundVideoWidget() {
    zFMV_Stream *const oldStream = stream;
    if (oldStream != 0) {
        oldStream->Destructor();
        ::operator delete(oldStream);
        stream = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundvideowidget-destructor
 * @recoil-artifact defines .text recoil:function:0x4bfcd0: HudUiBackgroundVideoWidget::Destructor.
 * Purpose: Runs the authored video-widget destructor entry used by the HUD UI owner.
 */
void HudUiBackgroundVideoWidget::Destructor() {
    this->~HudUiBackgroundVideoWidget();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundvideowidget-setmediapathownedandrefresh
 * @recoil-artifact defines .text recoil:function:0x4bfd40: HudUiBackgroundVideoWidget::SetMediaPathOwnedAndRefresh.
 * Purpose: Stores the movie path, resolves missing media, opens the stream, and refreshes clipping.
 */
void HudUiBackgroundVideoWidget::SetMediaPathOwnedAndRefresh(
    const char *path
) {
    strncpy(
        mediaPath,
        path,
        0x104
    );

    struct _stat statBuffer;
    if (_stat(
        mediaPath,
        &statBuffer
    ) == -1) {
        char *const resolvedPath = zSys::FindFileOnDriveType(
            5,
            mediaPath,
            0
        );
        if (resolvedPath != 0) {
            strncpy(
                mediaPath,
                resolvedPath,
                0x104
            );
        }
    }

    if (_stat(
        mediaPath,
        &statBuffer
    ) == -1) {
        stream = 0;
        return;
    }

    zFMV_Stream *const newStream = (zFMV_Stream *)(::operator new(sizeof(zFMV_Stream)));
    stream = newStream != 0 ? newStream->Init(
        mediaPath,
        0
    ) : 0;

    RebuildBltRect();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundvideowidget-setcolorkey565
 * @recoil-artifact defines .text recoil:function:0x4bfe20: HudUiBackgroundVideoWidget::SetColorKey565.
 * Purpose: Marks the active video stream format dirty and stores the 565 color key.
 */
void HudUiBackgroundVideoWidget::SetColorKey565(
    unsigned short colorKey
) {
    if (stream != 0) {
        stream->formatFlagsPacked |= 0x02;
    }

    colorKey565 = colorKey;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundvideowidget-update
 * @recoil-artifact defines .text recoil:function:0x4bfe40: HudUiBackgroundVideoWidget::Update.
 * Purpose: Advances decoded video frames while preserving the base element update behavior.
 */
void HudUiBackgroundVideoWidget::Update(
    float deltaSeconds
) {
    if ((flags & 0x10u) != 0) {
        return;
    }

    if (stream != 0) {
        const int frameTick = (int)((float)(stream->videoFramesPerSecond) * elapsedTimeSec);
        stream->ReadAndDecodeFrame((unsigned int)(frameTick % stream->videoFrameCount));
    }

    HudUiElement::Update(deltaSeconds);
    elapsedTimeSec += deltaSeconds;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundvideowidget-draw
 * @recoil-artifact defines .text recoil:function:0x4bfe90: HudUiBackgroundVideoWidget::Draw.
 * Purpose: Draws the background layer and blits the active stream with the stored color key.
 */
void HudUiBackgroundVideoWidget::Draw() {
    DrawBase();

    if (stream != 0) {
        zVid_Image::BlitToActiveTarget(
            (zVidImagePartial *)(stream),
            x,
            y,
            colorKey565,
            0
        );
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundvideowidget-drawbase
 * @recoil-artifact defines .text recoil:function:0x4bfec0: HudUiBackgroundVideoWidget::DrawBase.
 * Purpose: Blits the configured background source into the current clipped video area.
 */
void HudUiBackgroundVideoWidget::DrawBase() {
    zVidImagePartial *const bltSource = (zVidImagePartial *)(this->bltSource);
    if (bltSource != 0) {
        const int dstX = x > 0 ? x : 0;
        const int dstY = y > 0 ? y : 0;
        zVid_Image::BlitToActiveTarget(
            bltSource,
            dstX,
            dstY,
            0,
            (zVidRect32 *)(&clipRect)
        );
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundvideowidget-rebuildbltrect
 * @recoil-artifact defines .text recoil:function:0x4bff00: HudUiBackgroundVideoWidget::RebuildBltRect.
 * Purpose: Recomputes the stream clip rectangle against the background blit source.
 */
void HudUiBackgroundVideoWidget::RebuildBltRect() {
    HudUiRect rect;
    rect.left = GetCenterX() > 0 ? GetCenterX() : 0;
    rect.top = GetCenterY() > 0 ? GetCenterY() : 0;

    if (stream == 0) {
        return;
    }

    const int streamRight = rect.left + stream->width;
    const int streamBottom = rect.top + stream->height;

    zVidImagePartial *const bltSource = (zVidImagePartial *)(this->bltSource);
    if (bltSource != 0) {
        rect.right = streamRight < bltSource->width ? streamRight : bltSource->width;
        rect.bottom = streamBottom < bltSource->height ? streamBottom : bltSource->height;
    } else {
        rect.right = streamRight;
        rect.bottom = streamBottom;
    }

    SetClipRect(&rect);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduiprimitivebindtarget-setsegmentendpoints
 * @recoil-artifact defines .text recoil:function:0x4bffb0: HudUiPrimitiveBindTarget::SetSegmentEndpoints.
 * Purpose: apply the recovered HUD state change handled by HudUiPrimitiveBindTarget::SetSegmentEndpoints.
 */
void HudUiPrimitiveBindTarget::SetSegmentEndpoints(
    int startX,
    int startY,
    int newEndX,
    int newEndY
) {
    SetPos(
        startX,
        startY
    );
    endX = newEndX;
    endY = newEndY;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed
 * HudUiElement::GetCenterY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnHoverRepeat.
 */
void HudUiElement::OnHoverRepeat() {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: return the recovered HUD value exposed by HudUiElement::GetBoundsRectOrNull.
 */
HudUiRect * HudUiElement::GetBoundsRectOrNull() {
    return 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnActivate.
 */
void HudUiElement::OnActivate() {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnClearBinding.
 */
void HudUiElement::OnClearBinding() {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::ShowPreview.
 */
void HudUiElement::ShowPreview() {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::HidePreview.
 */
void HudUiElement::HidePreview() {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnBeginCapture.
 */
void HudUiElement::OnBeginCapture() {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnEndCapture.
 */
void HudUiElement::OnEndCapture() {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnPointerButtonState.
 */
void HudUiElement::OnPointerButtonState(
    int,
    int
) {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: handle the recovered HUD event path for HudUiElement::OnCapturedPrimaryRelease.
 */
void HudUiElement::OnCapturedPrimaryRelease() {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::ShouldHandleInput.
 */
int HudUiElement::ShouldHandleInput(
    HudUiBackground *,
    int
) {
    return 1;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::AfterInputUpdate.
 */
void HudUiElement::AfterInputUpdate(
    HudUiBackground *,
    int
) {}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::HitTest.
 */
int HudUiElement::HitTest(
    int px,
    int py
) {
    return HitTestTrue(
        px,
        py
    );
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x404d60 HudUiElement::GetY callers.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::EnableWordWrapWithRect.
 */
void HudUiElement::EnableWordWrapWithRect(
    const HudUiRect *
) {}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * No standalone retail function has been identified; restored as the default
 * HudUiZrdWidget post-load virtual observed as the ZRD widget tail slot before
 * numeric input adds raw-key virtuals.
 * Purpose: keep ZRD loading ownership on HudUiZrdWidget.
 */
void HudUiZrdWidget::PostLoadFromZrd() {}

/**
 * Purpose: reset the scrolling-text owner fade when the widget is activated;
 * retail source placement remains unresolved.
 */
void HudUiZrdScrollingText::OnActivate() {
    OnActivateResetOwnerFade();
}

/**
 * Purpose: initialize the recovered HudUiWidget::Constructor state.
 */

/**
 * Purpose: initialize the recovered HudUiWidget::Constructor state.
 */
HudUiWidget * HudUiWidget::Constructor(
    unsigned int initAlignFlags
) {
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
HudUiZrdWidget * HudUiZrdWidget::Constructor() {
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
void HudUiZrdWidget::DestructorCore() {
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
 * Original inline helper; no standalone retail function exists.
 * Observed in transition-panel owners that destroy embedded panel members.
 * Evidence: destructor callers tear down the HudUiPanel base without a
 * separate retail HudUiTransitionTextPanel destructor body.
 * Purpose: restore the source-level destructor for transition text panels.
 */
HudUiTransitionTextPanel::~HudUiTransitionTextPanel() {
    HudUiPanel::~HudUiPanel();
}

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
HudCmdBindingEntry **HudCmdBindingVector::erase(
    HudCmdBindingEntry **eraseFirst,
    HudCmdBindingEntry **eraseLast
) {
    HudCmdBindingEntry **write = eraseFirst;
    HudCmdBindingEntry **read = eraseLast;
    HudCmdBindingEntry **const oldEnd = last;
    if (read != oldEnd) {
        do {
            *write++ = *read++;
        } while (read != oldEnd);
    }
    ((StdPtrVector *)(this))->ClearNoOpDestroy(
        (int *)(write),
        (int *)(oldEnd)
    );
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
void HudUiTextInput::OnPrintableKey(
    int key
) {
    InsertCharAtCursor(key);
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x40e910 HudUiTriplet::InterpolateLayout callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnAccept.
 */
void HudUiTextInput::OnAccept() {
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b4370 HudUiTextInput::~HudUiTextInput callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnCancel.
 */
void HudUiTextInput::OnCancel() {
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b4370 HudUiTextInput::~HudUiTextInput callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnBackspace.
 */
void HudUiTextInput::OnBackspace() {
    BackspaceDeleteChar();
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b4370 HudUiTextInput::~HudUiTextInput callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnDeleteForward.
 */
void HudUiTextInput::OnDeleteForward() {
    DeleteCharForward();
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b4370 HudUiTextInput::~HudUiTextInput callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnMoveCursorLeft.
 */
void HudUiTextInput::OnMoveCursorLeft() {
    MoveCursorLeft();
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b4370 HudUiTextInput::~HudUiTextInput callers.
 * Purpose: handle the recovered HUD event path for HudUiTextInput::OnMoveCursorRight.
 */
void HudUiTextInput::OnMoveCursorRight() {
    MoveCursorRight();
}

/**
 * Source model note: Source-faithful helper recovered from address-backed callers in this
 * source file.
 * Purpose: run the recovered HudUiTextInput::~HudUiTextInput teardown path.
 */
void HudUiTextInput::OnOverflow() {
}

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
void HudUiTextInput::DestructorCore() {
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
HudUiTextInput * HudUiTextInput::Constructor(
    int bufferSize
) {
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
void HudUiChatComposeTextInput::OnAccept() {
    GameNet::EndChatComposeAndSendThunk();
}

/**
 * Purpose: construct the common HUD element base and embedded slot widgets
 * through ordinary C++ member construction; the shared retail identity with
 * the compatibility constructor wrapper remains unresolved.
 */
HudUiSlot::HudUiSlot() : HudUiElement(
        0,
        0
) {
}

/**
 * Purpose: preserve the recovered HUD behavior for HudUiPolyline::Draw.
 */
HudUiPolyline * HudUiPolyline::Constructor() {
    new (this) HudUiPolyline;
    return this;
}

/**
 * Purpose: preserve the recovered HUD behavior for HudUiSliderBorder::HudUiSliderBorder.
 */

/**
 * Purpose: advance the recovered HUD update path for HudUiSliderBorder::Update.
 */
HudUiSliderBorder * HudUiSliderBorder::Constructor() {
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduinumerictextinput-baseconstructor
 * @recoil-artifact defines .text recoil:function:0x41a190: HudUiNumericTextInput::Constructor.
 * Purpose: initialize the recovered HudUiNumericTextInput::Constructor state.
 */
HudUiNumericTextInput * HudUiNumericTextInput::BaseConstructor() {
    new (this) HudUiNumericTextInput;
    return this;
}

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
void HudUiNumericTextInput::Destructor() {
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduinumerictextinput-commitandgetvalue
 * @recoil-artifact defines .text recoil:function:0x41a2a0: HudUiClampedIntTextInput::OnRawKeyboardChar.
 * No standalone retail function has been identified for the base numeric
 * text-input commit slot; clamped/save-game owners override the slot when they
 * need committed values.
 * Purpose: provide the base numeric input commit default.
 */
int HudUiNumericTextInput::CommitAndGetValue() {
    return 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4bd100 HudUiPanel::ConstructorDefaultThunk callers.
 * Purpose: preserve the recovered HUD behavior for HudUiPanel::ConstructorDefault.
 */
HudUiPanel * HudUiPanel::ConstructorDefault(
    const char *text,
    int initX,
    int initY
) {
    new (this) HudUiPanel(
        text,
        initX,
        initY
    );
    return this;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduipanel-constructordefaultthunk
 * @recoil-artifact defines .text recoil:function:0x4bd100: HudUiPanel::ConstructorDefaultThunk.
 * Purpose: preserve the recovered HUD behavior for HudUiPanel::ConstructorDefaultThunk.
 */
HudUiPanel * HudUiPanel::ConstructorDefaultThunk() {
    return ConstructorDefault(
        0,
        0,
        0
    );
}

namespace HudScoreboard {

} // namespace HudScoreboard
