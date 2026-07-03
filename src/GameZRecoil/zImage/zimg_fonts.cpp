/* This source-layout fragment is included by the current compatibility container.
 * Parent build/manifests must compile this path directly after retiring the container include.
 */

/**
 * Reimplements 0x46efc0: zImage_Font::GetByIndexOrDefault.
 * Original file: D:\Proj\GameZRecoil\zImage\zimg_fonts.cpp.
 * Purpose: return a requested font table slot, falling back to slot 0 when
 * the requested slot is empty.
 *
 * Evidence: BN performs one g_zImage_FontTable indexed load, tests it, and
 * loads g_zImage_FontTable[0] only on the null-slot path.
 */
zImage_Font *__fastcall zImage_Font::GetByIndexOrDefault(
    int fontIndex
) {
    zImage_Font *const font = g_zImage_FontTable[fontIndex];
    if (font != 0) {
        return font;
    }

    return g_zImage_FontTable[0];
}

namespace zImage {
/**
 * Reimplements 0x46efe0: zImage::FontsLoadFromPath.
 * Original file: D:\Proj\GameZRecoil\zImage\zimg_fonts.cpp.
 * Purpose: load the FONTS node, create font records, load each font image,
 * and build glyph rectangles for the font table.
 *
 * Evidence: BN reports the same error paths, mission resource initialization,
 * FONTS node lookup, per-font image load, alpha flag update, glyph build call,
 * and loaded-tree release.
 */
int __fastcall FontsLoadFromPath(
    const char *path
) {
    zReader::Node *tree = zReader::LoadNodeFromPath(
        path,
        0,
        0
    );
    if (tree == 0) {
        zError::ReportOld(
            0x200,
            "D:\\Proj\\GameZRecoil\\zImage\\zimg_fonts.cpp",
            0x48,
            g_HudSensorTracker_ReadFileFailedFmt,
            path
        );
        return -1;
    }

    zImage_InitMissionResources("..\\data\\common\\fonts");
    zReader::Node *fontsNode = zReader_GetNamedNode(
        tree,
        g_HudCfgKey_Fonts
    );
    if (fontsNode == 0) {
        zError::ReportOld(
            0x800,
            "D:\\Proj\\GameZRecoil\\zImage\\zimg_fonts.cpp",
            0x52,
            "%s file empty",
            path
        );
        return -1;
    }

    zReader::Node *fontArray = fontsNode->value.nodes;
    const int count = fontArray[0].value.i32;
    zImage_Font *font = (zImage_Font *)(malloc((size_t)(count - 1) * sizeof(zImage_Font)));

    for (int i = 1; i < count; ++i) {
        zImage_Font **slot = &g_zImage_FontTable[i - 1];
        *slot = font;
        const char *fontImagePath = fontArray[i].value.str;
        font->image = TexDir_FindOrCreateByPath(fontImagePath);
        if (font->image != 0) {
            font->image->formatFlagsPacked |= 0x02;
            const int glyphCount = font->BuildGlyphRects();
            if (glyphCount != 0x5f) {
                zError::ReportOld(
                    0x200,
                    "D:\\Proj\\GameZRecoil\\zImage\\zimg_fonts.cpp",
                    0x68,
                    "Only found %d characters in font %s",
                    glyphCount,
                    path
                );
            }
            ++font;
        }
    }

    zReader::FreeLoadedTree(tree);
    return 0;
}
} // namespace zImage

/**
 * Reimplements 0x46f130: zImage_Font::BuildGlyphRects.
 * Original file: D:\Proj\GameZRecoil\zImage\zimg_fonts.cpp.
 * Purpose: scan the font image into glyph rectangles and compute the space
 * width used by font text layout.
 *
 * Evidence: BN initializes space width from image width divided by 95 minus
 * one, scans transparent and non-transparent column runs through
 * IsImageColumnTransparent, fills glyph RECT bounds, and returns the glyph
 * count.
 */
int zImage_Font::BuildGlyphRects() {
    zVidImagePartial *image = this->image;
    int x = 0;
    int result = 1;
    this->spaceWidth = image->width / 95 - 1;

    while (x < image->width) {
        RECT *glyph = &this->glyphRects[result - 1];
        glyph->top = 0;
        glyph->bottom = image->height - 1;

        if (IsImageColumnTransparent(
            image,
            x
        ) != 0) {
            do {
                ++x;
            } while (IsImageColumnTransparent(
                image,
                x
            ) != 0);
        }

        const int left = x;
        while (IsImageColumnTransparent(
            image,
            x
        ) == 0) {
            ++x;
        }

        const int right = x;
        if (IsImageColumnTransparent(
            image,
            x
        ) != 0) {
            do {
                ++x;
            } while (IsImageColumnTransparent(
                image,
                x
            ) != 0);
        }

        x -= (x - right) / 2;
        glyph->left = left;
        glyph->right = right + 1;

        ++result;
        if (result >= 95) {
            break;
        }
    }

    return result;
}
/**
 * Reimplements 0x46f210: zImage_Font::IsImageColumnTransparent.
 * Original file: D:\Proj\GameZRecoil\zImage\zimg_fonts.cpp.
 * Purpose: test whether a vertical image column contains only the configured
 * transparent font color.
 *
 * Evidence: BN rejects columns beyond image width, treats non-positive height
 * as transparent, then walks one 16-bit pixel per row using image width as the
 * row stride.
 */
int __fastcall zImage_Font::IsImageColumnTransparent(
    zVidImagePartial *image,
    int columnX
) {
    const int width = image->width;
    unsigned short *column = (unsigned short *)(image->pixels) + columnX;
    if (columnX >= width) {
        return 0;
    }

    const int height = image->height;
    if (height <= 0) {
        return 1;
    }

    for (int y = 0; y < height; ++y) {
        if (*column != (unsigned short)(g_zImage_FontTransparentColor)) {
            return 0;
        }
        column += width;
    }

    return 1;
}

/**
 * Reimplements 0x46f260: zImage_Font::MeasureString.
 * Original file: D:\Proj\GameZRecoil\zImage\zimg_fonts.cpp.
 * Purpose: measure wrapped font text width and total line advance.
 *
 * Evidence: BN gets the requested font with fallback, uses image height as
 * line advance, treats space, carriage return, and newline specially, clamps
 * printable glyph indexes to the 95-glyph table, and writes both outputs.
 */
void __fastcall zImage_Font::MeasureString(
    const char *text,
    int fontIndex,
    int *outWidthPx,
    int *outLineAdvance
) {
    zImage_Font *const font = GetByIndexOrDefault(fontIndex);
    if (font == 0) {
        return;
    }

    const int lineAdvance = font->image->height;
    int currentLineWidth = 0;
    int maxLineWidth = 0;
    int totalLineAdvance = lineAdvance;

    for (const char *cursor = text; *cursor != '\0'; ++cursor) {
        const signed char ch = *cursor;
        if (ch == ' ') {
            currentLineWidth += font->spaceWidth;
        } else if (ch == '\r') {
        } else if (ch == '\n') {
            if (currentLineWidth > maxLineWidth) {
                maxLineWidth = currentLineWidth;
            }

            currentLineWidth = 0;
            totalLineAdvance += lineAdvance;
        } else {
            int glyphIndex = (int)(ch)-0x21;
            if (glyphIndex < 0 || glyphIndex >= 0x5f) {
                glyphIndex = 0;
            }

            const RECT &glyph = font->glyphRects[glyphIndex];
            currentLineWidth += glyph.right - glyph.left;
        }

        if (currentLineWidth > maxLineWidth) {
            maxLineWidth = currentLineWidth;
        }
    }

    *outWidthPx = maxLineWidth;
    *outLineAdvance = totalLineAdvance;
}

