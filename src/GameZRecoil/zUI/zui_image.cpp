// zUI compilation unit for HudUiWidget, the image-backed HUD widget, inferred
// from the retail object boundary [0x4b3ce0, 0x4b4030): its HudUiWidget vtable
// [0x4d3428, 0x4d34a0) precedes zui_element.cpp's pooled constants and its
// .bss [0x56bd18, 0x56bd20) holds the exclusive-draw image pointer. Retail
// layout alone does not separate it from zui_element.cpp; the 1998-07-21 and
// 1998-09-28 demos link these functions apart from HudUiElement's, with the
// zui_widgets.cpp object between them. Original filename unresolved;
// zui_image.cpp is a provisional name (2026-10-03).

#include "recoil/Mfc42Abi.h"

#include "GameZRecoil/zHud/zhud_ui.h"

#include "GameZRecoil/include/zimage.h"

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduiwidget-exclusivedrawimage
 * @recoil-artifact defines .data recoil:data:0x56bd1c: g_HudUiWidget_ExclusiveDrawImage.
 * Purpose: preserve the recovered HUD global storage for g_HudUiWidget_ExclusiveDrawImage.
 */
zVidImagePartial* g_HudUiWidget_ExclusiveDrawImage = 0;

/**
 * Purpose: run the recovered HudUiWidget::DestructorCore teardown path.
 */
void HudUiWidget::DestructorCore()
{
    this->~HudUiWidget();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduiwidget-huduiwidget-0x4b3d00
 * @recoil-artifact defines .text recoil:function:0x4b3d00: HudUiWidget::HudUiWidget.
 * @recoil-match byte
 *
 * Purpose: Initialize the widget image, alignment, clipping and dirty rectangles.
 */
HudUiWidget::HudUiWidget(unsigned int initAlignFlags)
    : HudUiElement(0, 0)
{
    alignFlags = initAlignFlags;
    image = 0;
    ownsImage = 0;
    bltClipRectOrNull = 0;
    *((unsigned short*)(&imageStateWord)) = 0;
    dirtyRectCount = 0;

    {
        int dirtyRectIndex;
        for (dirtyRectIndex = 0; dirtyRectIndex < 4; ++dirtyRectIndex) {
            dirtyRects[dirtyRectIndex].framesRemaining = 0;
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduiwidget-destructor
 * @recoil-artifact defines .text recoil:function:0x4b3d50: HudUiWidget::~HudUiWidget.
 * @recoil-match byte
 *
 * Purpose: Release the widget's owned image before its element base is destroyed.
 * Evidence: VC5 supplies the vtable stores and native exception cleanup.
 */
HudUiWidget::~HudUiWidget()
{
    ReleaseImageIfOwned();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduiwidget-releaseimageifowned
 * @recoil-artifact defines .text recoil:function:0x4b3da0: HudUiWidget::ReleaseImageIfOwned.
 * @recoil-match byte
 *
 * Purpose: Release an owned image and clear the widget's ownership state.
 * Evidence: Retail stores the release helper's returned pointer into image.
 */
void HudUiWidget::ReleaseImageIfOwned()
{
    if (image != 0 && ownsImage != 0) {
        image = zVid_Image::ReleaseIfNotDefault(image);
    }

    ownsImage = 0;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-image.hud-ui-widget-set-pos
 * @recoil-artifact defines .text recoil:function:0x4b3dd0: HudUiWidget::SetPos.
 * @recoil-match byte
 */
void HudUiWidget::SetPos(int newX, int newY)
{
    if (alignFlags != 0 && image != 0) {
        x = newX - (image->width / 2);
        y = newY - (image->height / 2);
        Invalidate();
    } else {
        x = newX;
        y = newY;
        Invalidate();
    }
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-image.hud-ui-widget-set-image-by-path-owned
 * @recoil-artifact defines .text recoil:function:0x4b3e30: HudUiWidget::SetImageByPathOwned.
 * @recoil-match byte
 */
zVidImagePartial* HudUiWidget::SetImageByPathOwned(const char* imagePath)
{
    if (imagePath == 0) {
        return 0;
    }

    ReleaseImageIfOwned();
    image = zImage::TexDirFindOrCreateByPath(imagePath);
    if (image != 0) {
        ownsImage = 1;
    }

    Invalidate();
    return image;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-image.hud-ui-widget-set-image-borrowed-and-invalidate
 * @recoil-artifact defines .text recoil:function:0x4b3e70: HudUiWidget::SetImageBorrowedAndInvalidate.
 * @recoil-match byte
 */
zVidImagePartial* HudUiWidget::SetImageBorrowedAndInvalidate(zVidImagePartial* newImage)
{
    ownsImage = 0;
    image = newImage;
    Invalidate();
    return newImage;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-image.hud-ui-widget-invalidate-rect
 * @recoil-artifact defines .text recoil:function:0x4b3e90: HudUiWidget::InvalidateRect.
 */
void HudUiWidget::InvalidateRect(const HudUiRect* dirtyRect)
{
    HudUiRectDirty* slot = 0;
    if (image == 0) {
        return;
    }

    {
        for (int index = 0; index < 4; ++index) {
            if (dirtyRects[index].framesRemaining == 0) {
                slot = &dirtyRects[index];
                break;
            }
        }
    }

    if (slot == 0) {
        return;
    }

    slot->srcRect = *dirtyRect;

    if (slot->srcRect.left < x) {
        slot->srcRect.left = x;
    }

    if (slot->srcRect.right > image->width + x) {
        slot->srcRect.bottom = image->width + x;
    }

    if (slot->srcRect.top < y) {
        slot->srcRect.top = y;
    }

    if (slot->srcRect.bottom > image->height + y) {
        slot->srcRect.bottom = image->height + y;
    }

    if (slot->srcRect.right <= slot->srcRect.left || slot->srcRect.bottom <= slot->srcRect.top) {
        return;
    }

    ++dirtyRectCount;
    slot->framesRemaining = (g_HudUi_InvalidateMask == 0x0c ? 1u : 0u) + 1u;
    slot->drawX = slot->srcRect.left;
    slot->drawY = slot->srcRect.top;

    slot->srcRect.left -= GetCenterX();
    slot->srcRect.right -= GetCenterX();
    slot->srcRect.top -= GetCenterY();
    slot->srcRect.bottom -= GetCenterY();
    Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-image.hud-ui-widget-draw
 * @recoil-artifact defines .text recoil:function:0x4b3fb0: HudUiWidget::Draw.
 * @recoil-match byte
 */
void HudUiWidget::Draw()
{
    if (image == 0) {
        return;
    }

    if (dirtyRectCount != 0) {
        int dirtyRectIndex;
        for (dirtyRectIndex = 0; dirtyRectIndex < 4; ++dirtyRectIndex) {
            HudUiRectDirty& dirtyRect = dirtyRects[dirtyRectIndex];
            if (dirtyRect.framesRemaining == 0) {
                continue;
            }

            zVid_Image::BlitToActiveTarget(
                image,
                dirtyRect.drawX,
                dirtyRect.drawY,
                0,
                (zVidRect32*)(&dirtyRect.srcRect)
            );

            --dirtyRect.framesRemaining;
            if (dirtyRect.framesRemaining == 0) {
                --dirtyRectCount;
            }
        }
        return;
    }

    if (g_HudUiWidget_ExclusiveDrawImage != 0 && g_HudUiWidget_ExclusiveDrawImage != image) {
        return;
    }

    DrawBase();

    zVid_Image::BlitToActiveTarget(image, x, y, 0, (zVidRect32*)(bltClipRectOrNull));
}
