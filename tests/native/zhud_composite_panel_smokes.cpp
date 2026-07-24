#include "GameZRecoil/zHud/zhud_ui.h"

#include <cstdarg>
#include <cstring>

namespace {
void InitCompositeEntry(
    HudUiCompositePanelEntry *entry,
    int marker
) {
    entry->textColor0 = (unsigned int)(0x1000 + marker);
    entry->textColor1 = (unsigned int)(0x2000 + marker);
    entry->cachedTextLength = (unsigned int)(marker);
    entry->textWidthPx = marker * 3;
    entry->textHeightPx = marker * 5;
    entry->textDirty = (unsigned int)(marker + 7);
    entry->flashCountdown = (float)(marker);
    entry->flashResetValue = (float)(marker + 1);
    entry->flashAltColor0 = 0x3000 + marker;
    entry->flashAltColor1 = 0x4000 + marker;
    entry->flashEnabled = marker & 1;
    entry->flashMode = marker & 3;
    entry->flashDirectionSign = marker & 1 ? -1 : 1;
    std::memset(
        entry->cachedText,
        0,
        sizeof(entry->cachedText)
    );
    entry->cachedText[0] = (char)('A' + (marker % 20));
}

int EntryMarker(
    const HudUiCompositePanelEntry *entry
) {
    return entry->flashAltColor0 - 0x3000;
}

void TestPanelSetTextFmtV(
    HudUiPanel *panel,
    const char *format,
    ...
) {
    va_list args;
    va_start(
        args,
        format
    );
    panel->SetTextFmtV(
        format,
        args
    );
    va_end(args);
}

void TestCompositePanelSetTextFmtV(
    HudUiCompositePanel *panel,
    const char *format,
    ...
) {
    va_list args;
    va_start(
        args,
        format
    );
    panel->SetTextFmtV(
        format,
        args
    );
    va_end(args);
}

int CompositeEntryCount(
    const HudUiCompositePanel *panel
) {
    return (int)(panel->entryVector.size());
}

HudUiCompositePanel * AllocateCompositePanel(
    int entryCount
) {
    return new HudUiCompositePanel(entryCount);
}

void ReleaseCompositePanel(
    HudUiCompositePanel *panel
) {
    delete panel;
}

bool EntryMatchesMarker(
    const HudUiCompositePanelEntry *entry,
    int marker,
    unsigned int expectedDirty
) {
    return entry->textColor0 == (unsigned int)(0x1000 + marker) &&
        entry->textColor1 == (unsigned int)(0x2000 + marker) &&
        entry->cachedTextLength == (unsigned int)(marker) &&
        entry->textWidthPx == marker * 3 &&
        entry->textHeightPx == marker * 5 &&
        entry->textDirty == expectedDirty &&
        entry->flashCountdown == (float)(marker) &&
        entry->flashResetValue == (float)(marker + 1) &&
        entry->flashAltColor0 == 0x3000 + marker &&
        entry->flashAltColor1 == 0x4000 + marker &&
        entry->flashEnabled == (marker & 1) &&
        entry->flashMode == (marker & 3) &&
        entry->flashDirectionSign == (marker & 1 ? -1 : 1) &&
        entry->cachedText[0] == (char)('A' + (marker % 20));
}
} // namespace

extern "C" int zhud_transition_text_panel_update_smoke(void) {
    HudUiTransitionTextPanel panel;
    HudUiElement *const element = &panel;
    int failure = 0;

    panel.flags = 0x10u;
    panel.timer = 0.25f;
    panel.flashEnabled = 1;
    panel.flashCountdown = 0.25f;
    element->Update(0.5f);
    if (failure == 0 && (panel.timer != 0.25f || panel.flashCountdown != 0.25f)) {
        failure = 2;
    }

    panel.flags = 1u;
    panel.timer = 0.25f;
    panel.flashEnabled = 0;
    element->Update(0.5f);
    if (failure == 0 && (panel.flags & 0x10u) == 0) {
        failure = 3;
    }

    panel.flags = 0;
    panel.textColor0 = 0x00010203u;
    panel.textColor1 = 0x00040506u;
    panel.flashEnabled = 1;
    panel.flashMode = 2;
    panel.flashCountdown = 0.25f;
    panel.flashResetValue = 0.5f;
    panel.flashAltColor0 = 0x00070809;
    panel.flashAltColor1 = 0x000a0b0c;
    panel.flashDirectionSign = 1;
    panel.textDirty = 0;
    element->Update(0.5f);
    if (failure == 0 &&
        (panel.flashCountdown != 0.5f || panel.flashDirectionSign != -1 ||
            panel.textDirty != 0 || panel.textColor0 != 0x00070809u ||
            panel.textColor1 != 0x000a0b0cu ||
            panel.flashAltColor0 != 0x00010203 ||
            panel.flashAltColor1 != 0x00040506)) {
        failure = 4;
    }

    return failure;
}

extern "C" int zhud_transition_text_panel_flash_rate_smoke(void) {
    HudUiTransitionTextPanel resetPanel;
    resetPanel.flashEnabled = 0;
    resetPanel.flashResetValue = 0.25f;
    resetPanel.flashCountdown = 0.125f;
    resetPanel.flashDirectionSign = -1;
    resetPanel.ResetFlashState(6.0f);

    float expectedHalfRate = 3.0f;
    unsigned int expectedHalfRateBits = 0;
    unsigned int actualCountdownBits = 0;
    std::memcpy(
        &expectedHalfRateBits,
        &expectedHalfRate,
        sizeof(expectedHalfRateBits)
    );
    std::memcpy(
        &actualCountdownBits,
        &resetPanel.flashCountdown,
        sizeof(actualCountdownBits)
    );
    const bool resetState =
        resetPanel.flashEnabled == 1 &&
        resetPanel.flashResetValue == 3.0f &&
        actualCountdownBits == expectedHalfRateBits &&
        resetPanel.flashDirectionSign == 1;

    HudUiTransitionTextPanel ratePanel;
    ratePanel.flashMode = 0;
    ratePanel.SetFlashRate(4.0f);
    expectedHalfRate = 2.0f;
    std::memcpy(
        &expectedHalfRateBits,
        &expectedHalfRate,
        sizeof(expectedHalfRateBits)
    );
    std::memcpy(
        &actualCountdownBits,
        &ratePanel.flashCountdown,
        sizeof(actualCountdownBits)
    );
    const bool rateSet =
        ratePanel.flashEnabled == 1 &&
        ratePanel.flashMode == 1 &&
        ratePanel.flashResetValue == 2.0f &&
        actualCountdownBits == expectedHalfRateBits &&
        ratePanel.flashDirectionSign == 1;

    ratePanel.flashResetValue = 0.25f;
    ratePanel.flashCountdown = 0.5f;
    ratePanel.flashDirectionSign = -1;
    ratePanel.flashMode = 1;
    ratePanel.SetFlashRate(8.0f);
    const bool rateAlreadySet =
        ratePanel.flashMode == 1 &&
        ratePanel.flashResetValue == 0.25f &&
        ratePanel.flashCountdown == 0.5f &&
        ratePanel.flashDirectionSign == -1;

    HudUiTransitionTextPanel colorPanel;
    colorPanel.flashMode = 0;
    colorPanel.SetFlashColorAndRate(0x00112233, 6.0f);
    expectedHalfRate = 3.0f;
    std::memcpy(
        &expectedHalfRateBits,
        &expectedHalfRate,
        sizeof(expectedHalfRateBits)
    );
    std::memcpy(
        &actualCountdownBits,
        &colorPanel.flashCountdown,
        sizeof(actualCountdownBits)
    );
    const bool colorSet =
        colorPanel.flashEnabled == 1 &&
        colorPanel.flashMode == 2 &&
        colorPanel.flashResetValue == 3.0f &&
        actualCountdownBits == expectedHalfRateBits &&
        colorPanel.flashDirectionSign == 1 &&
        colorPanel.flashAltColor0 == 0x00112233 &&
        colorPanel.flashAltColor1 == 0x00112233;

    colorPanel.flashAltColor0 = 1;
    colorPanel.flashAltColor1 = 2;
    colorPanel.flashResetValue = 0.75f;
    colorPanel.flashMode = 2;
    colorPanel.SetFlashColorAndRate(0x00445566, 8.0f);
    const bool colorAlreadySet =
        colorPanel.flashMode == 2 &&
        colorPanel.flashAltColor0 == 1 &&
        colorPanel.flashAltColor1 == 2 &&
        colorPanel.flashResetValue == 0.75f;

    return resetState && rateSet && rateAlreadySet && colorSet && colorAlreadySet ? 0 : 1;
}

extern "C" int zhud_composite_panel_entry_copy_smoke(void) {
    HudUiCompositePanelEntry source;
    InitCompositeEntry(
        &source,
        9
    );

    HudUiCompositePanelEntry assigned;
    assigned = source;

    HudUiCompositePanelEntry copied(source);

    HudUiCompositePanelEntry rangeSource[2];
    InitCompositeEntry(
        &rangeSource[0],
        4
    );
    InitCompositeEntry(
        &rangeSource[1],
        5
    );
    HudUiCompositePanelVector rangeDest(
        rangeSource,
        rangeSource + 2
    );

    int failure = 0;
    if (rangeDest.size() != 2) {
        failure = 4;
    } else if (!EntryMatchesMarker(
            &assigned,
            9,
            source.textDirty
        )) {
        failure = 5;
    } else if (!EntryMatchesMarker(
            &copied,
            9,
            source.textDirty
        )) {
        failure = 6;
    } else if (!EntryMatchesMarker(
            &rangeDest[0],
            4,
            rangeSource[0].textDirty
        )) {
        failure = 7;
    } else if (!EntryMatchesMarker(
            &rangeDest[1],
            5,
            rangeSource[1].textDirty
        )) {
        failure = 8;
    }

    return failure;
}

extern "C" int zhud_composite_panel_vector_clear_smoke(void) {
    HudUiCompositePanelVector vector(2);
    InitCompositeEntry(
        &vector[0],
        11
    );
    InitCompositeEntry(
        &vector[1],
        12
    );

    vector.clear();
    return vector.empty() ? 0 : 1;
}

extern "C" int zhud_composite_panel_vector_insert_copies_smoke(void) {
    HudUiCompositePanelEntry templateEntry;
    InitCompositeEntry(
        &templateEntry,
        18
    );

    HudUiCompositePanelVector growVector(2);
    InitCompositeEntry(
        &growVector[0],
        1
    );
    InitCompositeEntry(
        &growVector[1],
        2
    );
    growVector.insert(
        growVector.begin() + 1,
        2,
        templateEntry
    );
    const bool growOk =
        growVector.size() == 4 &&
        EntryMarker(&growVector[0]) == 1 &&
        EntryMarker(&growVector[1]) == 18 &&
        EntryMarker(&growVector[2]) == 18 &&
        EntryMarker(&growVector[3]) == 2;

    HudUiCompositePanelVector longTailVector(3);
    longTailVector.reserve(5);
    InitCompositeEntry(
        &longTailVector[0],
        10
    );
    InitCompositeEntry(
        &longTailVector[1],
        11
    );
    InitCompositeEntry(
        &longTailVector[2],
        12
    );
    longTailVector.insert(
        longTailVector.begin() + 1,
        1,
        templateEntry
    );
    const bool longTailOk =
        longTailVector.size() == 4 &&
        EntryMarker(&longTailVector[0]) == 10 &&
        EntryMarker(&longTailVector[1]) == 18 &&
        EntryMarker(&longTailVector[2]) == 11 &&
        EntryMarker(&longTailVector[3]) == 12;

    HudUiCompositePanelVector shortTailVector(2);
    shortTailVector.reserve(5);
    InitCompositeEntry(
        &shortTailVector[0],
        20
    );
    InitCompositeEntry(
        &shortTailVector[1],
        21
    );
    shortTailVector.insert(
        shortTailVector.begin() + 1,
        3,
        templateEntry
    );
    const bool shortTailOk =
        shortTailVector.size() == 5 &&
        EntryMarker(&shortTailVector[0]) == 20 &&
        EntryMarker(&shortTailVector[1]) == 18 &&
        EntryMarker(&shortTailVector[2]) == 18 &&
        EntryMarker(&shortTailVector[3]) == 18 &&
        EntryMarker(&shortTailVector[4]) == 21;

    return growOk && longTailOk && shortTailOk ? 0 : 1;
}

extern "C" int zhud_composite_panel_constructor_with_entry_count_smoke(void) {
    HudUiCompositePanel *const panel = AllocateCompositePanel(2);

    const bool ok = CompositeEntryCount(panel) == 2 &&
        panel->entryVector.capacity() >= panel->entryVector.size() &&
        panel->activeEntryCount == 0 &&
        (panel->flags & 0x10u) == 0 &&
        panel->entryVector[0].textBuffer[0] == '\0' &&
        panel->entryVector[1].textBuffer[0] == '\0' &&
        (panel->entryVector[0].flags & 0x10u) != 0 &&
        (panel->entryVector[1].flags & 0x10u) != 0;

    ReleaseCompositePanel(panel);
    return ok ? 0 : 1;
}

extern "C" int zhud_composite_panel_destructor_smoke(void) {
    HudUiCompositePanel *const panel = AllocateCompositePanel(2);
    const bool ok = panel->entryVector.size() == 2;
    delete panel;
    return ok ? 0 : 1;
}

extern "C" int zhud_composite_panel_update_smoke(void) {
    HudUiCompositePanel *const panel = AllocateCompositePanel(2);

    panel->entryVector[0].flashDirectionSign = 7;
    panel->entryVector[1].flashDirectionSign = 8;
    panel->Update(0.1f);
    const bool hiddenSkipped =
        panel->entryVector[0].flashDirectionSign == 7 &&
        panel->entryVector[1].flashDirectionSign == 8;

    ReleaseCompositePanel(panel);
    if (!hiddenSkipped) {
        return 3;
    }
    return 0;
}

extern "C" int zhud_composite_panel_layout_entries_smoke(void) {
    HudUiCompositePanel *const panel = AllocateCompositePanel(2);
    panel->textDirty = 0;
    panel->textHeightPx = 14;
    panel->unknown274 = 2;
    const int entryHeight = -panel->unknown274;

    panel->LayoutEntries(
        7,
        11
    );

    int result = 0;
    if (panel->x != 7) {
        result = 2;
    } else if (panel->y != 11) {
        result = 3;
    } else if (panel->entryVector[0].x != 7) {
        result = 4;
    } else if (panel->entryVector[0].y != 11) {
        result = 5;
    } else if (panel->entryVector[1].x != 7) {
        result = 6;
    } else if (panel->entryVector[1].y != 11 + entryHeight) {
        result = (panel->entryVector[1].y & 0xff);
    }

    ReleaseCompositePanel(panel);
    return result;
}

extern "C" int zhud_composite_panel_set_text_fmt_smoke(void) {
    HudUiCompositePanel *const panel = AllocateCompositePanel(2);

    panel->SetTextFmt(
        "A%02d",
        3
    );
    const bool firstText = panel->activeEntryCount == 1 &&
        std::strcmp(
            panel->entryVector[0].textBuffer,
            "A03"
        ) == 0 &&
        (panel->entryVector[0].flags & 0x10u) == 0;

    panel->SetTextFmt(
        "B%02d",
        4
    );
    const bool scrolledText = panel->activeEntryCount == 1 &&
        std::strcmp(
            panel->entryVector[0].textBuffer,
            "B04"
        ) == 0 &&
        std::strcmp(
            panel->entryVector[1].textBuffer,
            "B04"
        ) == 0;

    TestCompositePanelSetTextFmtV(
        panel,
        "V%02d",
        5
    );
    const bool fmtV = panel->activeEntryCount == 1 &&
        std::strcmp(
            panel->entryVector[0].textBuffer,
            "V05"
        ) == 0 &&
        std::strcmp(
            panel->entryVector[1].textBuffer,
            "V05"
        ) == 0;

    int result = 0;
    if (!firstText) {
        if (panel->activeEntryCount != 1) {
            result = 20 + panel->activeEntryCount;
        } else if (std::strcmp(
                       panel->entryVector[0].textBuffer,
                       "A03"
                   ) != 0) {
            result = 30;
        } else if ((panel->entryVector[0].flags & 0x10u) != 0) {
            result = 31;
        } else {
            result = 32;
        }
    } else if (!scrolledText) {
        result = 3;
    } else if (!fmtV) {
        result = 4;
    }

    ReleaseCompositePanel(panel);
    return result;
}

extern "C" int zhud_composite_panel_set_font_smoke(void) {
    HudUiCompositePanel *const panel = AllocateCompositePanel(2);
    panel->textDirty = 0;
    panel->entryVector[0].textDirty = 0;
    panel->entryVector[1].textDirty = 0;

    panel->SetFont(
        "Arial",
        8,
        400,
        0,
        0,
        ANSI_CHARSET,
        DEFAULT_PITCH
    );

    const bool ok = panel->hFont != 0 &&
        panel->entryVector[0].textDirty == 1 &&
        panel->entryVector[1].textDirty == 1;

    int result = 0;
    if (panel->hFont == 0) {
        result = 2;
    } else if (panel->entryVector[0].textDirty != 1) {
        result = 10 + (int)(panel->entryVector[0].textDirty & 0xf);
    } else if (panel->entryVector[1].textDirty != 1) {
        result = 4;
    }

    ReleaseCompositePanel(panel);
    return result;
}

extern "C" int zhud_composite_panel_resize_entry_count_smoke(void) {
    HudUiCompositePanel *const panel = AllocateCompositePanel(3);

    panel->ResizeEntryCount(
        5,
        2
    );
    const bool clampOld = panel->activeEntryCount == 2;

    panel->ResizeEntryCount(
        -1,
        2
    );
    const bool clampNegative = panel->activeEntryCount == 0 &&
        (panel->entryVector[0].flags & 0x10u) != 0 &&
        (panel->entryVector[1].flags & 0x10u) != 0;

    panel->activeEntryCount = 7;
    panel->ReapplyEntryCount();
    const bool reapply = panel->activeEntryCount == 0 &&
        (panel->entryVector[2].flags & 0x10u) != 0;

    ReleaseCompositePanel(panel);
    return clampOld && clampNegative && reapply ? 0 : 1;
}

extern "C" int zhud_composite_panel_resize_vector_relayout_smoke(void) {
    HudUiCompositePanel *const panel = AllocateCompositePanel(2);
    panel->textDirty = 0;
    panel->textHeightPx = 10;
    panel->unknown274 = 0;
    panel->HudUiElement::SetPos(
        4,
        6
    );

    panel->ResizeEntryVectorAndRelayout(3);
    const bool grew = CompositeEntryCount(panel) == 3 &&
        panel->activeEntryCount == 2 &&
        (panel->entryVector[2].flags & 0x10u) != 0;

    panel->ResizeEntryVectorAndRelayout(1);
    const bool shrank = CompositeEntryCount(panel) == 1 &&
        panel->activeEntryCount == 1 &&
        panel->entryVector[0].x == 4 &&
        panel->entryVector[0].y == 6;

    panel->ResizeEntryVectorAndRelayout(1);
    const bool reapplied = CompositeEntryCount(panel) == 1 &&
        panel->activeEntryCount == 0 &&
        (panel->entryVector[0].flags & 0x10u) != 0;

    int result = 0;
    if (!grew) {
        if (CompositeEntryCount(panel) != 3) {
            result = 20 + CompositeEntryCount(panel);
        } else if (panel->activeEntryCount != 2) {
            result = 30 + panel->activeEntryCount;
        } else if ((panel->entryVector[2].flags & 0x10u) == 0) {
            result = 60;
        } else {
            result = 62;
        }
    } else if (!shrank) {
        result = 3;
    } else if (!reapplied) {
        result = 4;
    }

    ReleaseCompositePanel(panel);
    return result;
}

extern "C" int zhud_panel_set_text_fmt_smoke(void) {
    HudUiPanel panel(
        "",
        4,
        5
    );

    panel.SetTextFmt(
        "A%02d",
        7
    );
    const bool firstUpdate = std::strcmp(
        panel.textBuffer,
        "A07"
    ) == 0 && std::strncmp(
        panel.cachedText,
        "A07",
        3
    ) == 0 && panel.textDirty == 1;

    panel.textDirty = 0;
    panel.SetTextFmt(
        "A%02d",
        7
    );
    const bool unchangedSkipped = panel.textDirty == 0;

    panel.SetTextFmt(0);
    const bool cleared = panel.textBuffer[0] == '\0' && panel.textDirty == 1;

    panel.textDirty = 0;
    panel.SetText("Plain");
    const bool setText = std::strcmp(
        panel.textBuffer,
        "Plain"
    ) == 0 && std::strncmp(
        panel.cachedText,
        "Plain",
        5
    ) == 0 && panel.textDirty == 1;

    panel.textDirty = 0;
    panel.SetText("Plain");
    const bool setTextUnchanged = panel.textDirty == 0;

    panel.SetText(0);
    const bool setTextCleared = panel.textBuffer[0] == '\0' && panel.textDirty == 1;

    panel.textDirty = 0;
    TestPanelSetTextFmtV(
        &panel,
        "V%02d",
        9
    );
    const bool fmtV = std::strcmp(
        panel.textBuffer,
        "V09"
    ) == 0 && std::strncmp(
        panel.cachedText,
        "V09",
        3
    ) == 0 && panel.textDirty == 1;

    return firstUpdate && unchangedSkipped && cleared && setText && setTextUnchanged &&
        setTextCleared && fmtV ? 0 : 1;
}

extern "C" int zhud_panel_set_font_smoke(void) {
    HudUiPanel panel(
        "",
        4,
        5
    );

    panel.textDirty = 0;
    panel.SetFont(
        "Arial",
        10,
        FW_NORMAL,
        6,
        0,
        ANSI_CHARSET,
        DEFAULT_PITCH
    );

    const bool updated = panel.hFont != 0 && panel.textDirty == 1;
    return updated ? 0 : 1;
}
