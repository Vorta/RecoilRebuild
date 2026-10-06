// zUI compilation unit for HudUiPanel, HudUiFlashPanel and HudUiCompositePanel,
// inferred from the retail object boundary [0x4ba740, 0x4bc3a0): its .rdata run
// is the HudUiPanel vtable at 0x4d3a88, pooled constants [0x4d3b20, 0x4d3b40)
// (0.001, 0.999 and 1.0 doubles, 0.35f, 0.0f) and the HudUiCompositePanel
// vtable at 0x4d3b40. zui_widgets.cpp pools its constants ahead of its own
// vtables at [0x4d34b0, 0x4d34d8), and its template instantiations end the
// preceding .text at 0x4ba740. Original filename unresolved; zui_panel.cpp is a
// provisional name (2026-10-03).

#include "recoil/Mfc42Abi.h"

#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zHud/zhud_ui_defs.h"

#include <stdio.h>
#include <string.h>

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-huduipanel-0x4ba740
 * @recoil-artifact defines .text recoil:function:0x4ba740: HudUiPanel::HudUiPanel.
 * @recoil-match byte
 *
 * Purpose: Construct a text panel with default font, color, wrapping, and bounds state.
 */
HudUiPanel::HudUiPanel(const char* text, int initX, int initY)
    : HudUiTextLabel(text, initX, initY, 0)
{
    textPick = 0;
    textColor0 = 0x00ffffff;
    textColor1 = 0x00ffffff;
    textDirty = 1;
    hFont = GetStockObject(OEM_FIXED_FONT);
    cachedText[0] = '\0';
    shadowEnabled = 0;
    textDirty = 1;
    alignMode = 0;
    bkMode = TRANSPARENT;
    wrapRect.right = 0;
    wrapRect.left = 0;
    wrapRect.bottom = 0;
    wrapRect.top = 0;
    textRect = wrapRect;
    wordWrapEnabled = 0;
    unknown274 = 0;
    textHeightPx = 0;
    textWidthPx = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-huduipanel-0x4ba850
 * @recoil-artifact defines .text recoil:function:0x4ba850: HudUiPanel::HudUiPanel(const HudUiPanel &).
 * @recoil-match byte
 *
 * Purpose: Copy-construct panel-owned text and font state from another panel.
 */
HudUiPanel::HudUiPanel(const HudUiPanel& source)
    : HudUiTextLabel(source)
{
    textPick = 0;
    textColor0 = source.textColor0;
    textColor1 = source.textColor1;

    LOGFONTA logFont;
    if (GetObjectA(source.hFont, sizeof(logFont), &logFont) != 0) {
        hFont = CreateFontIndirectA(&logFont);
    }

    cachedTextLength = source.cachedTextLength;
    strncpy(cachedText, source.cachedText, 0x100);

    textWidthPx = source.textWidthPx;
    textHeightPx = source.textHeightPx;
    shadowEnabled = source.shadowEnabled;
    bkMode = source.bkMode;
    bkColor = source.bkColor;
    textDirty = source.textDirty;
    unknown274 = source.unknown274;
    wordWrapEnabled = source.wordWrapEnabled;
    wrapRect = source.wrapRect;
    textRect = source.textRect;
    alignMode = source.alignMode;
    shadowOffsetX = source.shadowOffsetX;
    shadowOffsetY = source.shadowOffsetY;
}

/**
 * Purpose: Initialize this panel by copying text, font, and layout state from another panel.
 */
HudUiPanel& HudUiPanel::operator=(const HudUiPanel& source)
{
    HudUiTextLabel::operator=(source);

    textPick = 0;
    textColor0 = source.textColor0;
    textColor1 = source.textColor1;

    LOGFONTA logFont;
    if (GetObjectA(source.hFont, sizeof(logFont), &logFont) != 0) {
        hFont = CreateFontIndirectA(&logFont);
    }

    cachedTextLength = source.cachedTextLength;
    strncpy(cachedText, source.cachedText, 0x100);

    textWidthPx = source.textWidthPx;
    textHeightPx = source.textHeightPx;
    shadowEnabled = source.shadowEnabled;
    bkMode = source.bkMode;
    bkColor = source.bkColor;
    textDirty = 1;
    unknown274 = source.unknown274;
    wordWrapEnabled = source.wordWrapEnabled;
    wrapRect = source.wrapRect;
    textRect = source.textRect;
    alignMode = source.alignMode;
    shadowOffsetX = source.shadowOffsetX;
    shadowOffsetY = source.shadowOffsetY;
    return *this;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-huduipanel-0x4bab40
 * @recoil-artifact defines .text recoil:function:0x4bab40: HudUiPanel::~HudUiPanel.
 * @recoil-match byte
 *
 * Purpose: Release the panel-owned text image and font during object teardown.
 * Evidence: Retail calls Destroy for a non-null text image, clears that field,
 * and releases the font through DeleteObject. VC5 supplies the vtable stores
 * and native exception cleanup for this ordinary destructor.
 */
HudUiPanel::~HudUiPanel()
{
    if (textPick != 0) {
        zVid_Image::Destroy(textPick);
        textPick = 0;
    }

    DeleteObject(hFont);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-setfont
 * @recoil-artifact defines .text recoil:function:0x4babb0: HudUiPanel::SetFont.
 * @recoil-match byte
 *
 * Purpose: Replace the panel font handle and mark text layout dirty.
 */
void HudUiPanel::SetFont(
    const char* faceName,
    int height,
    int weight,
    int width,
    int italic,
    int charSet,
    int pitchAndFamily
)
{
    DeleteObject(hFont);
    hFont = CreateFontA(
        -height,
        width,
        0,
        0,
        weight,
        italic,
        0,
        0,
        charSet,
        OUT_TT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DRAFT_QUALITY,
        pitchAndFamily,
        faceName
    );
    textDirty = 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-rebuildtextrect
 * @recoil-artifact defines .text recoil:function:0x4bac10: HudUiPanel::RebuildTextRect.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiPanel::RebuildTextRect.
 */
void HudUiPanel::RebuildTextRect()
{
    const int textLength = (int)(strlen(textBuffer));
    if (textLength == 0) {
        textRect.left = textRect.top = textRect.right = textRect.bottom = 0;
        textHeightPx = 0;
        textWidthPx = 0;
    } else {
        HDC measureDc = CreateCompatibleDC(0);
        if (measureDc != 0) {
            SelectObject(measureDc, hFont);

            UINT drawFormat;
            BOOL measured;
            if (wordWrapEnabled != 0) {
                textRect = wrapRect;
                drawFormat = DT_WORDBREAK;
                measured = TRUE;
            } else {
                drawFormat = DT_LEFT;
                measured = DrawTextA(measureDc, textBuffer, -1, (RECT*)(&textRect), DT_CALCRECT);
            }

            if (measured != 0) {
                if (shadowEnabled != 0) {
                    textRect.bottom += abs(shadowOffsetY);
                    textRect.right += abs(shadowOffsetX);
                }

                textWidthPx = textRect.right - textRect.left;
                textHeightPx = textRect.bottom - textRect.top;

                if (textPick != 0) {
                    if (textWidthPx > textPick->width || textHeightPx > textPick->height) {
                        zVid_Image::Destroy(textPick);
                        textPick = zVid_Image::Create();
                        zVid_Image::SetFormatCode(textPick, 3);
                        zVid_Image::SetSize(textPick, (short)(textWidthPx), (short)(textHeightPx));
                        void* const pixels
                            = malloc(zVid_Image::QueryBytesPerPixel(textPick) * textWidthPx * textHeightPx);
                        zVidImageSetPixels(textPick, pixels, 0);
                        textPick->formatFlagsPacked |= 0x20;
                    }
                } else {
                    textPick = zVid_Image::Create();
                    zVid_Image::SetFormatCode(textPick, 3);
                    zVid_Image::SetSize(textPick, (short)(textWidthPx), (short)(textHeightPx));
                    void* const pixels = malloc(zVid_Image::QueryBytesPerPixel(textPick) * textWidthPx * textHeightPx);
                    zVidImageSetPixels(textPick, pixels, 0);
                    textPick->formatFlagsPacked |= 0x20;
                }

                if (textPick != 0) {
                    memset(textPick->pixels, 0, zVid_Image::QueryBytesPerPixel(textPick) * textPick->pixelCount);

                    HDC drawDc;
                    if (g_zVideo_pfnImageUploadPixelsToSurface(textPick, &drawDc) != 0) {
                        RECT mainRect = *(RECT*)(&textRect);
                        SelectObject(drawDc, hFont);

                        if (shadowEnabled != 0) {
                            RECT shadowRect = *(RECT*)(&textRect);
                            if (shadowOffsetX > 0) {
                                shadowRect.left += shadowOffsetX;
                            } else {
                                mainRect.left -= shadowOffsetX;
                            }

                            if (shadowOffsetY > 0) {
                                shadowRect.top += shadowOffsetY;
                            } else {
                                mainRect.top -= shadowOffsetY;
                            }

                            ::SetTextColor(drawDc, 0x00141414);
                            if (bkMode == OPAQUE) {
                                SetBkColor(drawDc, 0x20);
                            }

                            SetBkMode(drawDc, bkMode);
                            DrawTextA(drawDc, textBuffer, -1, &shadowRect, drawFormat);
                        }

                        if (textColor0 != textColor1) {
                            ::SetTextColor(drawDc, 0x00ffffff);
                        } else {
                            ::SetTextColor(drawDc, textColor0);
                        }
                        if (bkMode == OPAQUE) {
                            SetBkColor(drawDc, bkColor);
                        }

                        SetBkMode(drawDc, bkMode);
                        DrawTextA(drawDc, textBuffer, -1, &mainRect, drawFormat);

                        g_zVideo_pfnImageReleaseSurface(textPick, drawDc);
                    }

                    TEXTMETRICA metrics;
                    if (GetTextMetricsA(measureDc, &metrics) != 0) {
                        if (textColor0 != textColor1) {
                            const unsigned short sourceWhite = (unsigned short)(zVidPackColorRGB(0xff, 0xff, 0xff));
                            unsigned short* pixel = (unsigned short*)(textPick->pixels);
                            {
                                for (int row = 0; row < textPick->height; ++row) {
                                    int rowPhase;
                                    if (shadowEnabled != 0) {
                                        rowPhase
                                            = (row + shadowOffsetY) % (metrics.tmHeight + metrics.tmExternalLeading);
                                    } else {
                                        rowPhase = row % (metrics.tmHeight + metrics.tmExternalLeading);
                                    }
                                    const float blend = (float)(rowPhase - metrics.tmInternalLeading)
                                        / (float)(metrics.tmAscent - metrics.tmInternalLeading);
                                    const unsigned int blendedColor
                                        = HudUiFlashPanel::ComputeFlashBlendColor(textColor0, textColor1, blend);
                                    const unsigned short packedColor = (unsigned short)(zVidPackColorRGB(
                                        GetRValue(blendedColor),
                                        GetGValue(blendedColor),
                                        blendedColor >> 16
                                    ));

                                    {
                                        for (int col = 0; col < textPick->width; ++col) {
                                            if (*pixel == sourceWhite) {
                                                *pixel = packedColor;
                                            }
                                            ++pixel;
                                        }
                                    }
                                }
                            }
                        }

                        unknown274 = (int)(metrics.tmExternalLeading);
                    }
                }
            } else {
                GetLastError();
            }

            DeleteDC(measureDc);
        }
    }

    textDirty = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduiflashpanel-computeflashblendcolor
 * @recoil-artifact defines .text recoil:function:0x4bb0c0: HudUiFlashPanel::ComputeFlashBlendColor.
 *
 *
 * Purpose: clamp endpoint flash colors and blend RGB channels for intermediate flash values.
 */
unsigned int __fastcall HudUiFlashPanel::ComputeFlashBlendColor(unsigned int color0, unsigned int color1, float blend)
{
    if (blend < 0.001) {
        return color0;
    }
    if (blend > 0.999) {
        return color1;
    }

    const float inverseBlend = 1.0 - blend;
    const int blue = (int)(GetRValue(color0) * inverseBlend + GetRValue(color1) * blend);
    const int green = (int)(GetGValue(color0) * inverseBlend + GetGValue(color1) * blend);
    const int red = (int)((int)((color0 >> 16) & 0xffu) * inverseBlend + (int)((color1 >> 16) & 0xffu) * blend);
    return RGB(blue, green, red);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-measuretextprefixrect
 * @recoil-artifact defines .text recoil:function:0x4bb1c0: HudUiPanel::MeasureTextPrefixRect.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiPanel::MeasureTextPrefixRect.
 */
int HudUiPanel::MeasureTextPrefixRect(int maxChars, RECT* outRect)
{
    int result = 0;
    HDC hdc = CreateCompatibleDC(0);
    if (hdc != 0) {
        SelectObject(hdc, hFont);
        if (maxChars > 0) {
            char* const textCopy = _strdup(textBuffer);
            if (maxChars <= (int)(strlen(textCopy))) {
                textCopy[maxChars] = '\0';
                if (DrawTextA(hdc, textCopy, -1, outRect, DT_CALCRECT) != 0) {
                    result = 1;
                }
            }

            free(textCopy);
        } else {
            if (DrawTextA(hdc, "W", -1, outRect, DT_CALCRECT) != 0) {
                result = 1;
                outRect->right = outRect->left;
            }
        }

        DeleteDC(hdc);
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-updatetextboundsfromcontent
 * @recoil-artifact defines .text recoil:function:0x4bb2a0: HudUiPanel::UpdateTextBoundsFromContent.
 * @recoil-match byte
 *
 * Purpose: update the panel clip rectangle from current text contents, alignment, wrapping, and shadow state.
 */
void HudUiPanel::UpdateTextBoundsFromContent()
{
    char* const panelText = textBuffer;
    const int textLength = (int)(strlen(panelText));

    if (wordWrapEnabled != 0) {
        clipRect.left = x;
        clipRect.top = y;
        clipRect.right = x + wrapRect.right;
        clipRect.bottom = y + wrapRect.bottom;
        return;
    }

    HDC hdc = CreateCompatibleDC(0);
    if (hdc == 0) {
        return;
    }

    SelectObject(hdc, hFont);
    SIZE textSize;
    if (GetTextExtentPoint32A(hdc, panelText, textLength, &textSize) != 0) {
        int left;
        if (alignMode == 0) {
            left = clipRect.left;
        } else {
            const int textWidth = QueryTextWidth();
            if (alignMode == 1) {
                left = clipRect.left + ((clipRect.right - clipRect.left) / 2) - (textWidth >> 1);
            } else {
                left = clipRect.right - textWidth;
            }
        }

        clipRect.left = left;
        clipRect.right = left + textSize.cx;
        clipRect.bottom = clipRect.top + textSize.cy;

        if (shadowEnabled != 0) {
            clipRect.bottom += abs(shadowOffsetY);
            clipRect.right += abs(shadowOffsetX);
        }
    }

    DeleteDC(hdc);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-hittest
 * @recoil-artifact defines .text recoil:function:0x4bb3d0: HudUiPanel::HitTest.
 * @recoil-match byte
 *
 * Purpose: test a point against the current visible text bounds, rebuilding dirty text metrics first.
 */
int HudUiPanel::HitTest(int px, int py)
{
    if ((~flags & 0x10u) != 0 && x <= px && y <= py) {
        if (px < x + QueryTextWidth() && py < y + QueryTextHeight()) {
            return 1;
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-getlasttextptr
 * @recoil-artifact defines .text recoil:function:0x4bb440: HudUiPanel::GetLastTextPtr.
 * @recoil-match byte
 *
 * Purpose: return the cached panel text after ensuring dirty text rendering state is rebuilt.
 */
char* HudUiPanel::GetLastTextPtr()
{
    if (textDirty != 0) {
        RebuildTextRect();
    }

    return cachedText;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-draw
 * @recoil-artifact defines .text recoil:function:0x4bb460: HudUiPanel::Draw.
 * @recoil-match byte
 *
 * Purpose: rebuild dirty panel text, draw the panel base, and blit the rendered text image with recovered alignment
 * behavior.
 */
void HudUiPanel::Draw()
{
    if (textDirty != 0) {
        RebuildTextRect();
    }

    if (textPick == 0) {
        return;
    }

    if (textBuffer[0] != '\0') {
        if (alignMode != 0) {
            if (textDirty != 0) {
                RebuildTextRect();
            }

            int frameWidth = clipRect.right - clipRect.left;
            int textWidth = textWidthPx;
            if (alignMode == 1) {
                frameWidth >>= 1;
                textWidth >>= 1;
            }

            x -= frameWidth;
            DrawBase();

            x += frameWidth - textWidth;
            zVid_Image::BlitToActiveTarget(textPick, x, y, 0, (zVidRect32*)(&textRect));
            x += textWidth;
            return;
        }

        DrawBase();
        zVid_Image::BlitToActiveTarget(textPick, x, y, 0, (zVidRect32*)(&textRect));
        return;
    }

    DrawBase();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-settextfmt
 * @recoil-artifact defines .text recoil:function:0x4bb540: HudUiPanel::SetTextFmt.
 * @recoil-match byte
 *
 * Purpose: format stack varargs into the panel text buffer and refresh cached
 * panel text state when the content changes.
 */
void __cdecl HudUiPanel::SetTextFmt(const char* format, ...)
{
    if (format == 0) {
        memset(textBuffer, 0, sizeof(textBuffer));
        textDirty = 1;
        return;
    }

    va_list args;
    va_start(args, format);
    _vsnprintf(textBuffer, 0x100, format, args);
    va_end(args);

    if (strncmp(cachedText, textBuffer, 0x100) == 0) {
        return;
    }

    if (centerText != 0) {
        HudUiTextLabel::UpdateTextExtents();
    }

    Invalidate();
    textDirty = 1;
    strncpy(cachedText, textBuffer, 0x100);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-settextfmtv
 * @recoil-artifact defines .text recoil:function:0x4bb5e0: HudUiPanel::SetTextFmtV.
 * @recoil-match byte
 *
 * Purpose: format a va_list into the panel text buffer and refresh cached
 * panel text state when the content changes.
 */
void __cdecl HudUiPanel::SetTextFmtV(const char* format, va_list args)
{
    if (format == 0) {
        memset(textBuffer, 0, sizeof(textBuffer));
        textDirty = 1;
        return;
    }

    _vsnprintf(textBuffer, 0x100, format, args);

    if (strncmp(cachedText, textBuffer, 0x100) == 0) {
        return;
    }

    if (centerText != 0) {
        HudUiTextLabel::UpdateTextExtents();
    }

    Invalidate();
    textDirty = 1;
    strncpy(cachedText, textBuffer, 0x100);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-settext
 * @recoil-artifact defines .text recoil:function:0x4bb680: HudUiPanel::SetText.
 * @recoil-match byte
 *
 * Purpose: copy literal panel text and refresh cached panel text state when
 * the content changes.
 */
void HudUiPanel::SetText(const char* text)
{
    if (text == 0) {
        memset(textBuffer, 0, sizeof(textBuffer));
        textDirty = 1;
        return;
    }

    strncpy(textBuffer, text, 0x100);

    if (strncmp(cachedText, textBuffer, 0x100) == 0) {
        return;
    }

    if (centerText != 0) {
        HudUiTextLabel::UpdateTextExtents();
    }

    Invalidate();
    textDirty = 1;
    strncpy(cachedText, textBuffer, 0x100);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-querytextheight
 * @recoil-artifact defines .text recoil:function:0x4bb710: HudUiPanel::QueryTextHeight.
 * @recoil-match byte
 *
 * Purpose: return the panel text height without external leading after rebuilding dirty text metrics.
 */
int HudUiPanel::QueryTextHeight()
{
    if (textDirty != 0) {
        RebuildTextRect();
    }

    return textHeightPx - unknown274;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-gettextrect
 * @recoil-artifact defines .text recoil:function:0x4bb740: HudUiPanel::GetTextRect.
 * @recoil-match byte
 *
 * Purpose: report the inherited element rectangle extended to the current rendered panel text dimensions.
 */
void HudUiPanel::GetTextRect(HudUiRect* outRect)
{
    HudUiElement::GetTextRect(outRect);

    if (textDirty != 0) {
        RebuildTextRect();
    }

    outRect->right = outRect->left + textWidthPx;
    outRect->bottom = outRect->top + QueryTextHeight();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-initialize-layout
 * Purpose: establish text measurement, entry layout, and visibility after
 * construction of the entry vector and cleanup of its template entry.
 *
 * Inferred inline source boundary: retail 0x4bb916..0x4bb940 performs these
 * operations after the temporary's destructor. An ordinary member preserves
 * the final virtual visibility dispatch. VC5 also reproduces the constructor's
 * vector helper calls with this phase present. The original helper name and
 * lexical boundary are unknown; no standalone retail body is attributed here.
 */
inline void HudUiCompositePanel::InitializeLayout(int entryCount)
{
    HudUiPanel::SetTextFmt("W");
    HudUiCompositePanel::SetPos(0, 0);
    ResizeEntryVectorAndRelayout(entryCount);
    SetVisible(1);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-huduicompositepanel
 * @recoil-artifact defines .text recoil:function:0x4bb790: HudUiCompositePanel::HudUiCompositePanel.
 * @recoil-match byte
 *
 * Purpose: initialize a composite panel and allocate its entry history vector.
 *
 * Evidence: BN retail 0x4bb790 constructs the HudUiPanel base, initializes the
 * vector member, installs g_HudUiCompositePanel_FTable, builds a stack
 * HudUiCompositePanelEntry template entry, resizes the entry vector, applies
 * text "W", relayouts, and sets the panel visible. BN caller 0x403930 invokes
 * this entry-count construction for HudUiBriefingRuntime::messagesPanel before
 * constructing locatorPanels, matching a C++ member-initializer constructor.
 * Source model note: the recovered constructor symbol is
 * ??0HudUiCompositePanel@@QAE@H@Z.
 */
HudUiCompositePanel::HudUiCompositePanel(int entryCount)
    : HudUiPanel(0, 0, 0)
{
    activeEntryCount = 0;
    {
        HudUiCompositePanelEntry templateEntry;
        entryVector.resize((unsigned int)(entryCount), templateEntry);
    }

    InitializeLayout(entryCount);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-update
 * @recoil-artifact defines .text recoil:function:0x4bb980: HudUiCompositePanel::Update.
 * @recoil-match byte
 *
 * Purpose: tick flash state for each visible composite-panel entry.
 */
void HudUiCompositePanel::Update(float deltaSeconds)
{
    if ((~flags & 0x10u) != 0) {
        for (unsigned int index = 0; index < entryVector.size(); ++index) {
            entryVector[index].Update(deltaSeconds);
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-setpos
 * @recoil-artifact defines .text recoil:function:0x4bb9f0: HudUiCompositePanel::SetPos.
 * @recoil-match byte
 *
 * Purpose: position the composite panel and lay out each text entry below the
 * panel origin.
 * Evidence: BN table g_HudUiCompositePanel_FTable stores 0x4bb9f0 in slot 3,
 * the inherited HudUiElement::SetPos slot; the body writes x/y, invalidates,
 * measures one entry height, and dispatches SetPos on each transition entry.
 */
void HudUiCompositePanel::SetPos(int x, int y)
{
    this->x = x;
    this->y = y;
    Invalidate();

    const int entryHeight = QueryTextHeight();
    unsigned int index = 0;
    int yOffset = 0;
    for (; index < entryVector.size(); ++index, yOffset += entryHeight) {
        entryVector[index].SetPos(GetCenterX(), GetCenterY() + yOffset);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-settextfmt
 * @recoil-artifact defines .text recoil:function:0x4bbaa0: HudUiCompositePanel::SetTextFmt.
 * @recoil-match byte
 *
 * Purpose: format text into the next composite-panel history entry.
 */
void __cdecl HudUiCompositePanel::SetTextFmt(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    SetTextFmtV(format, args);
    va_end(args);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-settextfmtv
 * @recoil-artifact defines .text recoil:function:0x4bbac0: HudUiCompositePanel::SetTextFmtV.
 * @recoil-match byte
 *
 * Purpose: write formatted text into the active composite entry and scroll
 * history as needed.
 */
void __cdecl HudUiCompositePanel::SetTextFmtV(const char* format, va_list args)
{
    entryVector[activeEntryCount].SetTextFmtV(format, args);
    HudUiTransitionTextPanel* const entry = &entryVector[activeEntryCount];
    entry->SetVisible(1);
    ScrollHistory();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-scrollhistory
 * @recoil-artifact defines .text recoil:function:0x4bbb20: HudUiCompositePanel::ScrollHistory.
 * @recoil-match byte
 *
 * Purpose: shift composite text history entries and keep the newest entry
 * active.
 */
void HudUiCompositePanel::ScrollHistory()
{
    ++activeEntryCount;

    if ((unsigned int)(activeEntryCount) >= (unsigned int)(entryVector.size())) {
        {
            for (unsigned int index = 0; index < (unsigned int)(entryVector.size()) - 1; ++index) {
                entryVector[index].SetText(entryVector[index + 1].GetLastTextPtr());
            }
        }
        --activeEntryCount;
    }

    Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-setfont
 * @recoil-artifact defines .text recoil:function:0x4bbbe0: HudUiCompositePanel::SetFont.
 * @recoil-match byte
 *
 * Purpose: apply font parameters to all composite-panel entries and relayout
 * the panel.
 */
void HudUiCompositePanel::SetFont(
    const char* faceName,
    int height,
    int weight,
    int width,
    int italic,
    int charSet,
    int pitchAndFamily
)
{
    for (unsigned int index = 0; index < entryVector.size(); ++index) {
        HudUiTransitionTextPanel* const entry = &entryVector[index];
        entry->SetFont(faceName, height, weight, width, italic, charSet, pitchAndFamily);
    }

    HudUiPanel::SetFont(faceName, height, weight, width, italic, charSet, pitchAndFamily);

    SetPos(GetCenterX(), GetCenterY());
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-resizeentryvectorandrelayout
 * @recoil-artifact defines .text recoil:function:0x4bbca0: HudUiCompositePanel::ResizeEntryVectorAndRelayout.
 * @recoil-match byte
 *
 * Purpose: resize the composite-entry vector, update active entries, and
 * relayout the panel.
 */
void HudUiCompositePanel::ResizeEntryVectorAndRelayout(int entryCount)
{
    const int oldCount = (int)(entryVector.size());

    if (entryCount != oldCount) {
        {
            HudUiCompositePanelEntry templateEntry;

            entryVector.resize((unsigned int)(entryCount), templateEntry);
        }

        ResizeEntryCount(oldCount, entryCount);
    } else {
        ReapplyEntryCount();
    }

    SetPos(GetCenterX(), GetCenterY());
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-reapplyentrycount
 * @recoil-artifact defines .text recoil:function:0x4bbe90: HudUiCompositePanel::ReapplyEntryCount.
 * @recoil-match byte
 *
 * Purpose: reapply the current composite-entry count after vector changes.
 */
void HudUiCompositePanel::ReapplyEntryCount()
{
    ResizeEntryCount(0, (int)(entryVector.size()));
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicompositepanel-resizeentrycount
 * @recoil-artifact defines .text recoil:function:0x4bbed0: HudUiCompositePanel::ResizeEntryCount.
 * @recoil-match byte
 *
 * Purpose: update composite-entry visibility for the requested active count.
 */
void HudUiCompositePanel::ResizeEntryCount(int oldCount, int entryCount)
{
    if (oldCount > entryCount) {
        oldCount = entryCount;
    } else if (oldCount < 0) {
        oldCount = 0;
    }

    if ((unsigned int)(entryCount) > entryVector.size()) {
        entryCount = (int)(entryVector.size());
    } else if (oldCount > entryCount) {
        entryCount = oldCount;
    }

    {
        for (int index = oldCount; index < entryCount; ++index) {
            entryVector[index].SetTextFmt("");
            HudUiTransitionTextPanel* const entry = &entryVector[index];
            entry->SetVisible(0);
        }
    }

    activeEntryCount = oldCount;
}
