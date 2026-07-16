#include "Battlesport/Mfc42Abi.h"

#include "Battlesport/hud.h"
#include "GameZRecoil/zHud/zhud_ui.h"

#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zRndr/zrndr.h"

#include <new>
#include <stdlib.h>

/**
 * Reimplements data 0x4dd1c8: g_HudUiMessageBoxDialog_SectionName.
 * BN source path: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
 * Source model: local MESSAGEBOX ZRD section-name data for the
 * HudUi::ShowMessageBox entrypoint wrapper; exact .data extent is the
 * writable char[11] bytes "MESSAGEBOX\0" with the sole xref in 0x438350.
 * Purpose: name the dialog.zrd section loaded by the modal message-box
 * wrapper.
 */
char g_HudUiMessageBoxDialog_SectionName[11] = "MESSAGEBOX";
/**
 * Reimplements data 0x4e489c: k_msgBoxWidgetName_Message.
 * Source model: writable ZRD widget-name literal used only by
 * HudUiMessageBoxDialog::Constructor.
 * Purpose: bind the message text primitive from a loaded message-box layout.
 */
char k_msgBoxWidgetName_Message[8] = "MESSAGE";
/**
 * Reimplements data 0x4e48a4: k_msgBoxWidgetName_Title.
 * Source model: writable ZRD widget-name literal used only by
 * HudUiMessageBoxDialog::Constructor.
 * Purpose: bind the title primitive from a loaded message-box layout.
 */
char k_msgBoxWidgetName_Title[6] = "TITLE";
/**
 * Reimplements data 0x4e48ac: k_msgBoxWidgetName_Cancel.
 * Source model: writable ZRD widget-name literal used only by
 * HudUiMessageBoxDialog::Constructor.
 * Purpose: bind the cancel button from a loaded message-box layout.
 */
char k_msgBoxWidgetName_Cancel[10] = "MB_CANCEL";
/**
 * Reimplements data 0x4e48b8: k_msgBoxWidgetName_OK.
 * Source model: writable ZRD widget-name literal used only by
 * HudUiMessageBoxDialog::Constructor.
 * Purpose: bind the OK button from a loaded message-box layout.
 */
char k_msgBoxWidgetName_OK[6] = "MB_OK";

/**
 * Recovered original-source helper, no standalone retail function.
 * Observed caller: 0x4bf060 HudUiMessageBoxDialog::Constructor.
 * Purpose: divide signed fallback layout coordinates by a power of two with the
 * same toward-zero correction pattern emitted in the message-box constructor.
 */
int HudUiDialogSignedDivPow2(
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
zVidImagePartial *HudUiMessageBoxCreateSolidImage(
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
 * Reimplements 0x4bf060: HudUiMessageBoxDialog::Constructor.
 * Original source path: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
 * BN source path: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
 * Source model: class-first constructor for HudUiMessageBoxDialog; BN table
 * 0x4d4028 is installed at object offset zero by the constructed C++ object.
 * Purpose: bind the ZRD-backed message-box widgets, or build the original
 * solid-image fallback dialog and child widget graph.
 * Touched data: owns runtime image pointers only; dialog/button table globals
 * are class identity evidence, not separately promoted data.
 * Provenance: Reimplements 0x4bf060 from HudUiMessageBoxDialog.cpp.
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
 * Reimplements 0x4bf560: HudUiMessageBoxDialog::Destructor.
 * BN source path: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
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
 * Reimplements 0x4bf630: HudUiMessageBoxDialog::RunModal.
 * Original source path: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
 * BN source path: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
 * Source model: direct HudUiMessageBoxDialog method called by
 * HudUi::ShowMessageBox, not a dialog-table slot.
 * Purpose: switch render/input state to modal drawing, pump frames until the
 * dialog records a result, then restore the previous framebuffer region.
 * Touched data: direct globals are renderer/time owners already linked by the
 * parent handoff; no dialog-owned plan-tracked data is promoted here.
 * Provenance: Reimplements 0x4bf630 from HudUiMessageBoxDialog.cpp.
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
 * Reimplements 0x4bf7c0: HudUiMessageBoxDialog::OnOk.
 * BN source path: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
 * Source model: HudUiMessageBoxDialog table slot +0x0c in table 0x4d4028.
 * Purpose: accept the modal dialog and force the modal loop to exit.
 * Touched data: no authored globals; writes dialog modal fields only.
 */
void HudUiMessageBoxDialog::OnOk() {
    modalResult = 1;
    modalFrameCountdown = 0;
}

/**
 * Reimplements 0x4bf7e0: HudUiMessageBoxDialog::OnCancel.
 * BN source path: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
 * Source model: HudUiMessageBoxDialog table slot +0x10 in table 0x4d4028.
 * Purpose: cancel the modal dialog and force the modal loop to exit.
 * Touched data: no authored globals; writes dialog modal fields only.
 */
void HudUiMessageBoxDialog::OnCancel() {
    modalResult = 2;
    modalFrameCountdown = 0;
}

/**
 * Reimplements 0x4bf800: HudUiMessageBoxOkButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
 * BN source path: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
 * Source model: HudUiMessageBoxOkButton activation override; button table
 * 0x4d40c8 overrides slot +0x30 with this method.
 * Purpose: dispatch through the owner dialog to OnOk, then run the base
 * HudUiZrdWidget activation behavior.
 * Touched data: no authored globals; owner vptr dispatch reaches the dialog
 * table slot before HudUiZrdWidget::OnActivate.
 * Provenance: Reimplements 0x4bf800 from HudUiMessageBoxDialog.cpp.
 */
void HudUiMessageBoxOkButton::OnActivate() {
    HudUiMessageBoxDialog *const dialog = (HudUiMessageBoxDialog *)(owner);
    dialog->OnOk();

    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x4bf820: HudUiMessageBoxCancelButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
 * BN source path: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
 * Source model: HudUiMessageBoxCancelButton activation override; button table
 * 0x4d4040 overrides slot +0x30 with this method.
 * Purpose: dispatch through the owner dialog to OnCancel, then run the base
 * HudUiZrdWidget activation behavior.
 * Touched data: no authored globals; owner vptr dispatch reaches the dialog
 * table slot before HudUiZrdWidget::OnActivate.
 * Provenance: Reimplements 0x4bf820 from HudUiMessageBoxDialog.cpp.
 */
void HudUiMessageBoxCancelButton::OnActivate() {
    HudUiMessageBoxDialog *const dialog = (HudUiMessageBoxDialog *)(owner);
    dialog->OnCancel();

    HudUiZrdWidget::OnActivate();
}

namespace HudUi {
/**
 * Reimplements 0x438350: HudUi::ShowMessageBox.
 * Original source path: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
 * BN source path: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
 * Source model: HudUiMessageBoxDialog.cpp entrypoint wrapper that constructs
 * the stack HudUiMessageBoxDialog, not a broad HudUi owner or table scaffold.
 * Purpose: load the MESSAGEBOX section from dialog.zrd, run the dialog modally
 * with the caller strings/context and infinite timeout, then destroy it.
 * Touched data: g_HudUiMessageBoxDialog_SectionName at 0x4dd1c8 is the local
 * writable char[11] MESSAGEBOX section-name data; dialog.zrd is the accepted
 * shared dialog path literal.
 * Provenance: Reimplements 0x438350 from HudUiMessageBoxDialog.cpp.
 */
int __fastcall ShowMessageBox(
    const char *messageText,
    const char *titleText,
    void *modalContext
) {
    HudUiMessageBoxDialog dialog;
    dialog.Constructor(
        "dialog.zrd",
        g_HudUiMessageBoxDialog_SectionName
    );
    const int result = dialog.RunModal(
        messageText,
        titleText,
        modalContext,
        -1.0f
    );
    dialog.Destructor();
    return result;
}
} // namespace HudUi
