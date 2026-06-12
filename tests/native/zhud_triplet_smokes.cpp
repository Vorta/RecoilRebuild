#include "Battlesport/GameNet.h"
#include "Battlesport/HudSensorTracker.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zNetwork/zNetwork.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int TripletEntryCount(
    const HudUiTriplet &triplet
) {
    return (int)(triplet.entries.end - triplet.entries.begin);
}

static bool TripletSortOrderMatches(
    const HudUiScoreboardEntry *entries,
    const int *expectedKeys,
    int count
) {
    for (int index = 0; index < count; ++index) {
        if (entries[index].playerKey != expectedKeys[index]) {
            return false;
        }
    }

    return true;
}

static HudUiPanel *TextStackLineAtForSmoke(
    HudUiTextStack4 *stack,
    int index
) {
    return &stack->lines[index];
}

static void DeleteTextStackLineFontsForSmoke(
    HudUiTextStack4 *stack
) {
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = TextStackLineAtForSmoke(
            stack,
            index
        );
        if (panel->hFont != 0) {
            DeleteObject(panel->hFont);
            panel->hFont = 0;
        }
    }
}

static bool TextStackLineFontHandleValidForSmoke(
    HudUiPanel *panel
) {
    LOGFONTA fontInfo = {};
    return panel->hFont != 0 && GetObjectA(
        panel->hFont,
        sizeof(fontInfo),
        &fontInfo
    ) != 0;
}

extern "C" int zhud_triplet_scoreboard_entry_update_smoke(void) {
    const int oldRaceMode = g_HudSensorTracker.raceCheckpointMode;
    const int oldGoalValue = g_HudSensorTracker.runtimeGoalValue;

    HudUiTriplet triplet = {};
    triplet.Constructor();

    GameNetPlayerRow *const alpha = (GameNetPlayerRow *)calloc(
        20,
        sizeof(GameNetPlayerRow)
    );
    if (alpha == 0) {
        triplet.DestructorCore();
        g_HudSensorTracker.raceCheckpointMode = oldRaceMode;
        g_HudSensorTracker.runtimeGoalValue = oldGoalValue;
        return 2;
    }

    GameNetPlayerRow *const bravo = alpha + 1;
    GameNetPlayerRow *const largeRows = alpha + 2;

    alpha->playerKey = 101;
    alpha->playerColorPackedRgb = 0x00112233;
    strcpy(
        alpha->displayName,
        "Alpha"
    );

    bravo->playerKey = 202;
    bravo->playerColorPackedRgb = 0x00445566;
    strcpy(
        bravo->displayName,
        "Bravo"
    );

    g_HudSensorTracker.raceCheckpointMode = 0;
    g_HudSensorTracker.runtimeGoalValue = 5;
    triplet.AddEntry(
        alpha
    );
    triplet.AddEntry(
        bravo
    );

    alpha->score = 9;
    alpha->lapCount = 3;
    alpha->playerColorPackedRgb = 0x00778899;
    triplet.UpdateEntryData(
        alpha
    );

    HudUiPanel *const row0Name = triplet.rowCells[0];
    HudUiPanel *const row0Score = triplet.rowCells[1];
    HudUiPanel *const row0Kills = triplet.rowCells[2];
    int scoreModeFail = 0;
    if (TripletEntryCount(triplet) != 2) {
        scoreModeFail = 21;
    } else if (triplet.entries.begin[0].playerKey != alpha->playerKey) {
        scoreModeFail = 22;
    } else if (strcmp(
        row0Name->GetLastTextPtr(),
        "Alpha"
    ) != 0) {
        scoreModeFail = 23;
    } else if (strcmp(
        row0Score->GetLastTextPtr(),
        "9"
    ) != 0) {
        scoreModeFail = 24;
    } else if ((((HudUiElement *)(row0Kills))->flags & 0x10u) == 0) {
        scoreModeFail = 25;
    } else if (row0Name->textColor0 != 0x00778899) {
        scoreModeFail = 26;
    }
    const bool scoreMode = scoreModeFail == 0;

    g_HudSensorTracker.raceCheckpointMode = 1;
    alpha->score = 1;
    alpha->lapCount = 1;
    triplet.UpdateEntryData(
        alpha
    );
    bravo->score = 5;
    bravo->lapCount = 4;
    bravo->playerColorPackedRgb = 0x00010203;
    triplet.UpdateEntryData(
        bravo
    );

    int lapModeFail = 0;
    if (triplet.entries.begin[0].playerKey != bravo->playerKey) {
        lapModeFail = 31;
    } else if (strcmp(
        row0Name->GetLastTextPtr(),
        "Bravo"
    ) != 0) {
        lapModeFail = 32;
    } else if (strcmp(
        row0Score->GetLastTextPtr(),
        "4"
    ) != 0) {
        lapModeFail = 33;
    } else if (strcmp(
        row0Kills->GetLastTextPtr(),
        "5"
    ) != 0) {
        lapModeFail = 34;
    } else if ((((HudUiElement *)(row0Kills))->flags & 0x10u) != 0) {
        lapModeFail = 35;
    } else if (row0Name->textColor0 != 0x00010203) {
        lapModeFail = 36;
    }
    const bool lapMode = lapModeFail == 0;

    HudUiTriplet largeTriplet = {};
    largeTriplet.Constructor();
    for (int index = 0; index < 18; ++index) {
        largeRows[index].playerKey = 1000 + ((index * 7) % 18);
        largeRows[index].playerColorPackedRgb = 0x00010101u * (unsigned int)(index + 1);
        sprintf(
            largeRows[index].displayName,
            "Large%02d",
            largeRows[index].playerKey - 1000
        );
        largeTriplet.AddEntry(
            &largeRows[index]
        );
    }

    int largeSortFail = 0;
    if (TripletEntryCount(largeTriplet) != 18) {
        largeSortFail = 41;
    } else {
        int expectedKeys[18] = {};
        for (int index = 0; index < 18; ++index) {
            expectedKeys[index] = 1017 - index;
        }
        if (!TripletSortOrderMatches(
            largeTriplet.entries.begin,
            expectedKeys,
            18
        )) {
            largeSortFail = 42;
        }
    }

    if (largeSortFail == 0 &&
        strcmp(
            largeTriplet.rowCells[0]->GetLastTextPtr(),
            "Large17"
        ) != 0) {
        largeSortFail = 43;
    }

    const bool largeSort = largeSortFail == 0;
    largeTriplet.DestructorCore();

    g_HudSensorTracker.raceCheckpointMode = oldRaceMode;
    g_HudSensorTracker.runtimeGoalValue = oldGoalValue;
    triplet.DestructorCore();
    free(alpha);

    if (!scoreMode) {
        return scoreModeFail;
    }
    if (!lapMode) {
        return lapModeFail;
    }
    if (!largeSort) {
        return largeSortFail;
    }

    return 0;
}

extern "C" int zhud_list_menu_entry_sort_smoke(void) {
    const int oldRaceMode = g_HudSensorTracker.raceCheckpointMode;
    const int oldGoalValue = g_HudSensorTracker.runtimeGoalValue;
    g_HudSensorTracker.raceCheckpointMode = 1;
    g_HudSensorTracker.runtimeGoalValue = 5;

    HudUiTriplet triplet = {};
    triplet.Constructor();

    GameNetPlayerRow rows[5] = {};
    rows[0].playerKey = 99;
    rows[0].score = 99;
    rows[0].lapCount = 0;
    rows[1].playerKey = 7;
    rows[1].score = 0;
    rows[1].lapCount = 0;
    rows[2].playerKey = 11;
    rows[2].score = 4;
    rows[2].lapCount = 2;
    rows[3].playerKey = 2;
    rows[3].score = 8;
    rows[3].lapCount = 0;
    rows[4].playerKey = 44;
    rows[4].score = 4;
    rows[4].lapCount = 2;

    for (int index = 0; index < 5; ++index) {
        triplet.AddEntry(
            &rows[index]
        );
        triplet.UpdateEntryData(
            &rows[index]
        );
    }

    const int sortedExpected[5] = {44, 11, 99, 2, 7};
    const bool mixedSort =
        triplet.entries.end - triplet.entries.begin == 5 &&
        TripletSortOrderMatches(
            triplet.entries.begin,
            sortedExpected,
            5
        );

    HudUiTriplet largeTriplet = {};
    largeTriplet.Constructor();
    GameNetPlayerRow largeRows[18] = {};
    for (int index = 0; index < 18; ++index) {
        largeRows[index].playerKey = 1000 + ((index * 7) % 18);
        largeTriplet.AddEntry(
            &largeRows[index]
        );
    }

    int largeExpected[18] = {};
    for (int index = 0; index < 18; ++index) {
        largeExpected[index] = 1017 - index;
    }
    const bool tieSort =
        largeTriplet.entries.end - largeTriplet.entries.begin == 18 &&
        TripletSortOrderMatches(
            largeTriplet.entries.begin,
            largeExpected,
            18
        );

    largeTriplet.DestructorCore();
    triplet.DestructorCore();
    g_HudSensorTracker.raceCheckpointMode = oldRaceMode;
    g_HudSensorTracker.runtimeGoalValue = oldGoalValue;

    return mixedSort && tieSort ? 0 : 1;
}

extern "C" int zhud_triplet_interpolate_layout_smoke(void) {
    HudUiTriplet triplet = {};
    triplet.baseXStart = 10;
    triplet.baseXEnd = 22;
    triplet.baseYStart = 50;
    triplet.baseYEnd = 30;
    triplet.rowPitchYStart = -10;
    triplet.rowPitchYEnd = 10;
    triplet.lapsColumnOffsetXStart = 3;
    triplet.lapsColumnOffsetXEnd = 13;
    triplet.killsColumnOffsetXStart = 20;
    triplet.killsColumnOffsetXEnd = -10;
    triplet.fontSizeStart = 9;
    triplet.fontSizeEnd = 12;
    triplet.fontWeightStart = 500;
    triplet.fontWeightEnd = 299;

    triplet.InterpolateLayout(0.25f);

    const bool quarter = triplet.baseX == 13 && triplet.baseY == 45 &&
                         triplet.rowPitchY == -5 && triplet.lapsColumnOffsetX == 5 &&
                         triplet.killsColumnOffsetX == 12 && triplet.fontSize == 9 &&
                         triplet.fontWeight == 449;

    triplet.InterpolateLayout(1.25f);

    const bool beyondEnd = triplet.baseX == 25 && triplet.baseY == 25 &&
                           triplet.rowPitchY == 15 && triplet.lapsColumnOffsetX == 15 &&
                           triplet.killsColumnOffsetX == -17 && triplet.fontSize == 12 &&
                           triplet.fontWeight == 248;

    return quarter && beyondEnd ? 0 : 1;
}

extern "C" int zhud_scoreboard_set_scale_and_rebuild_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;

    HudUiStatsListElement statsList = {};
    HudUiTriplet triplet = {};
    triplet.Constructor();
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    triplet.baseXStart = 10;
    triplet.baseXEnd = 30;
    triplet.baseYStart = 40;
    triplet.baseYEnd = 20;
    triplet.rowPitchYStart = 4;
    triplet.rowPitchYEnd = 12;
    triplet.lapsColumnOffsetXStart = 5;
    triplet.lapsColumnOffsetXEnd = 15;
    triplet.killsColumnOffsetXStart = 30;
    triplet.killsColumnOffsetXEnd = 50;
    triplet.fontSizeStart = 8;
    triplet.fontSizeEnd = 12;
    triplet.fontWeightStart = 200;
    triplet.fontWeightEnd = 600;

    HudScoreboard::SetScaleAndRebuild(0.5f);

    const bool interpolated = triplet.baseX == 20 && triplet.baseY == 30 &&
                              triplet.rowPitchY == 8 && triplet.lapsColumnOffsetX == 10 &&
                              triplet.killsColumnOffsetX == 40 && triplet.fontSize == 10 &&
                              triplet.fontWeight == 400;

    bool rowsHidden = true;
    for (int index = 0; index < 24; ++index) {
        rowsHidden =
            rowsHidden && (((HudUiElement *)(triplet.rowCells[index]))->flags & 0x10u) != 0;
    }

    g_HudUiMgrStatsList = oldStatsList;
    triplet.DestructorCore();
    return interpolated && rowsHidden ? 0 : 1;
}

extern "C" int zhud_text_stack_constructors_smoke(void) {
    HudUiTopMessageStack top = {};
    top.Constructor();

    const int topY[4] = {0x1e, 0x30, 0x42, 0x54};
    bool topLines = top.enabled == 0 &&
                    top.childHead == (HudUiElement *)(TextStackLineAtForSmoke(
                        &top,
                        0
                    )) &&
                    top.childTail == (HudUiElement *)(TextStackLineAtForSmoke(
                        &top,
                        3
                    ));
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = TextStackLineAtForSmoke(
            &top,
            index
        );
        topLines = topLines && panel->x == 0x140 && panel->y == topY[index] &&
                   panel->alignMode == 1 && panel->shadowEnabled == 1 &&
                   panel->shadowOffsetX == -1 && panel->shadowOffsetY == -1 &&
                   (panel->flags & 0x10u) != 0 && panel->hFont != 0;
    }

    HudUiChatMessageStack chat = {};
    chat.Constructor();

    const int chatY[4] = {0x159, 0x147, 0x135, 0x123};
    bool chatLines = chat.enabled == 0 &&
                     chat.childHead == (HudUiElement *)(TextStackLineAtForSmoke(
                         &chat,
                         0
                     )) &&
                     chat.childTail == (HudUiElement *)(TextStackLineAtForSmoke(
                         &chat,
                         3
                     ));
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = TextStackLineAtForSmoke(
            &chat,
            index
        );
        chatLines = chatLines && panel->x == 0x140 && panel->y == chatY[index] &&
                    panel->textColor0 == 0x00996a00 &&
                    panel->textColor1 == 0x0095c7ff && panel->alignMode == 1 &&
                    panel->shadowEnabled == 1 && panel->shadowOffsetX == -1 &&
                    panel->shadowOffsetY == -1 && (panel->flags & 0x10u) != 0 &&
                    panel->hFont != 0;
    }

    DeleteTextStackLineFontsForSmoke(&top);
    DeleteTextStackLineFontsForSmoke(&chat);
    return topLines && chatLines ? 0 : 1;
}

extern "C" int zhud_text_stack_set_font_all_smoke(void) {
    HudUiTopMessageStack stack = {};
    stack.Constructor();

    stack.SetFontAll(
        "Arial Narrow",
        18,
        500,
        7
    );

    bool fontsSet = true;
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = TextStackLineAtForSmoke(
            &stack,
            index
        );
        fontsSet = fontsSet && panel->hFont != 0;
    }

    DeleteTextStackLineFontsForSmoke(&stack);
    return fontsSet ? 0 : 1;
}

extern "C" int zhud_text_stack_push_line_smoke(void) {
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    HudUiTextStack4 *const oldChatStack = g_HudUiChatMessageStack;

    HudUiTopMessageStack top = {};
    top.Constructor();
    g_HudUiTopMessageStack = &top;

    HudUiChatMessageStack chat = {};
    chat.Constructor();
    g_HudUiChatMessageStack = &chat;

    top.enabled = 0;
    HudUi::ShowTopMessageLine(
        "ignored",
        1.0f
    );
    HudUiPanel *const line0 = TextStackLineAtForSmoke(
        &top,
        0
    );
    HudUiPanel *const line1 = TextStackLineAtForSmoke(
        &top,
        1
    );
    const bool disabledShowSkipped = (line0->flags & 0x10u) != 0;

    top.PushLine(
        "alpha",
        2.5f
    );
    const bool firstPush = top.enabled == 1 && (line0->flags & 0x10u) == 0 &&
                           line0->timer == 2.5f &&
                           strcmp(
                               line0->GetLastTextPtr(),
                               "alpha"
                           ) == 0;

    top.PushLine(
        "alpha",
        3.5f
    );
    const bool repeatedTopDoesNotShift = (line1->flags & 0x10u) != 0 &&
                                         line0->timer == 3.5f;

    top.PushLine(
        "beta",
        4.5f
    );
    const bool shifted =
        strcmp(
            line0->GetLastTextPtr(),
            "beta"
        ) == 0 &&
        strcmp(
            line1->GetLastTextPtr(),
            "alpha"
        ) == 0 &&
        (line1->flags & 0x10u) == 0 && line1->timer == 3.5f;

    HudUi::ShowTopMessageLine(
        "gamma",
        5.0f
    );
    const bool showPushesWhenEnabled =
        strcmp(
            line0->GetLastTextPtr(),
            "gamma"
        ) == 0 &&
        strcmp(
            line1->GetLastTextPtr(),
            "beta"
        ) == 0;

    HudUi::PushTopMessageLine(
        "epsilon",
        7.0f
    );
    const bool directTopPush =
        strcmp(
            line0->GetLastTextPtr(),
            "epsilon"
        ) == 0 &&
        strcmp(
            line1->GetLastTextPtr(),
            "gamma"
        ) == 0 &&
        line0->timer == 7.0f;

    chat.enabled = 0;
    HudUi::ShowChatLine(
        "ignored",
        1.0f
    );
    HudUiPanel *const chatLine0 = TextStackLineAtForSmoke(
        &chat,
        0
    );
    const bool disabledChatSkipped = (chatLine0->flags & 0x10u) != 0;

    chat.enabled = 1;
    HudUi::ShowChatLine(
        "delta",
        6.0f
    );
    const bool showChatPushesWhenEnabled =
        strcmp(
            chatLine0->GetLastTextPtr(),
            "delta"
        ) == 0 &&
        chatLine0->timer == 6.0f;

    DeleteTextStackLineFontsForSmoke(&top);
    DeleteTextStackLineFontsForSmoke(&chat);
    g_HudUiTopMessageStack = oldTopStack;
    g_HudUiChatMessageStack = oldChatStack;

    return disabledShowSkipped && firstPush && repeatedTopDoesNotShift && shifted &&
                   showPushesWhenEnabled && directTopPush && disabledChatSkipped &&
                   showChatPushesWhenEnabled
               ? 0
               : 1;
}

extern "C" int zhud_text_stack_clear_and_enable_smoke(void) {
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    HudUiTextStack4 *const oldChatStack = g_HudUiChatMessageStack;

    HudUiTopMessageStack top = {};
    top.Constructor();
    HudUiChatMessageStack chat = {};
    chat.Constructor();
    g_HudUiTopMessageStack = &top;
    g_HudUiChatMessageStack = &chat;

    top.PushLine(
        "top alpha",
        2.0f
    );
    top.PushLine(
        "top beta",
        3.0f
    );
    chat.PushLine(
        "chat alpha",
        4.0f
    );

    top.enabled = 0;
    chat.enabled = 0;
    HudUiMgr::EnableTopAndChatStacks();

    bool cleared = top.enabled == 1 && chat.enabled == 1;
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const topLine = TextStackLineAtForSmoke(
            &top,
            index
        );
        HudUiPanel *const chatLine = TextStackLineAtForSmoke(
            &chat,
            index
        );
        cleared = cleared && (topLine->flags & 0x10u) != 0 &&
                  (chatLine->flags & 0x10u) != 0 &&
                  strcmp(
                      topLine->GetLastTextPtr(),
                      ""
                  ) == 0 &&
                  strcmp(
                      chatLine->GetLastTextPtr(),
                      ""
                  ) == 0;
    }

    DeleteTextStackLineFontsForSmoke(&top);
    DeleteTextStackLineFontsForSmoke(&chat);
    g_HudUiTopMessageStack = oldTopStack;
    g_HudUiChatMessageStack = oldChatStack;
    return cleared ? 0 : 1;
}

extern "C" int zhud_text_stack_clear_and_disable_smoke(void) {
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    HudUiTextStack4 *const oldChatStack = g_HudUiChatMessageStack;

    HudUiTopMessageStack top = {};
    top.Constructor();
    HudUiChatMessageStack chat = {};
    chat.Constructor();
    g_HudUiTopMessageStack = &top;
    g_HudUiChatMessageStack = &chat;

    top.PushLine(
        "top alpha",
        2.0f
    );
    top.PushLine(
        "top beta",
        3.0f
    );
    chat.PushLine(
        "chat alpha",
        4.0f
    );

    top.enabled = 1;
    chat.enabled = 1;
    HudUiMgr::DisableTopAndChatStacks();

    bool cleared = top.enabled == 0 && chat.enabled == 0;
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const topLine = TextStackLineAtForSmoke(
            &top,
            index
        );
        HudUiPanel *const chatLine = TextStackLineAtForSmoke(
            &chat,
            index
        );
        cleared = cleared && (topLine->flags & 0x10u) != 0 &&
                  (chatLine->flags & 0x10u) != 0 &&
                  strcmp(
                      topLine->GetLastTextPtr(),
                      ""
                  ) == 0 &&
                  strcmp(
                      chatLine->GetLastTextPtr(),
                      ""
                  ) == 0;
    }

    DeleteTextStackLineFontsForSmoke(&top);
    DeleteTextStackLineFontsForSmoke(&chat);
    g_HudUiTopMessageStack = oldTopStack;
    g_HudUiChatMessageStack = oldChatStack;
    return cleared ? 0 : 1;
}

extern "C" int zhud_text_stack_destructor_core_smoke(void) {
    HudUiTopMessageStack top = {};
    top.Constructor();
    bool topHadFonts = true;
    for (int index = 0; index < 4; ++index) {
        topHadFonts = topHadFonts && TextStackLineFontHandleValidForSmoke(
            TextStackLineAtForSmoke(
                &top,
                index
            )
        );
    }

    top.DestructorCore();
    bool topDestroyed = topHadFonts;
    for (int index = 0; index < 4; ++index) {
        topDestroyed = topDestroyed && !TextStackLineFontHandleValidForSmoke(
            TextStackLineAtForSmoke(
                &top,
                index
            )
        );
    }

    HudUiChatMessageStack chat = {};
    chat.Constructor();
    bool chatHadFonts = true;
    for (int index = 0; index < 4; ++index) {
        chatHadFonts = chatHadFonts && TextStackLineFontHandleValidForSmoke(
            TextStackLineAtForSmoke(
                &chat,
                index
            )
        );
    }

    chat.DestructorCore();
    bool chatDestroyed = chatHadFonts;
    for (int index = 0; index < 4; ++index) {
        chatDestroyed = chatDestroyed && !TextStackLineFontHandleValidForSmoke(
            TextStackLineAtForSmoke(
                &chat,
                index
            )
        );
    }

    return topDestroyed && chatDestroyed ? 0 : 1;
}

extern "C" int zhud_triplet_is_local_player_first_entry_smoke(void) {
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;

    HudUiTriplet triplet = {};
    HudUiScoreboardEntry entries[2] = {};
    entries[0].playerKey = 1001;
    entries[1].playerKey = 2002;

    const bool emptyNull = triplet.IsLocalPlayerFirstEntry() == -1;

    triplet.entries.begin = entries;
    triplet.entries.end = entries;
    triplet.entries.cap = entries + 2;
    const bool emptyRange = triplet.IsLocalPlayerFirstEntry() == -1;

    triplet.entries.end = entries + 2;
    g_zNetwork_LocalPlayerKey = 1001;
    const bool matchFirst = triplet.IsLocalPlayerFirstEntry() == 1;

    g_zNetwork_LocalPlayerKey = 2002;
    const bool otherEntryDoesNotMatch = triplet.IsLocalPlayerFirstEntry() == 0;

    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    return emptyNull && emptyRange && matchFirst && otherEntryDoesNotMatch ? 0 : 1;
}

extern "C" int zhud_mgr_is_local_player_first_in_stats_list_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;

    HudUiStatsListElement statsList = {};
    HudUiTriplet triplet = {};
    HudUiScoreboardEntry entries[2] = {};
    entries[0].playerKey = 1001;
    entries[1].playerKey = 2002;

    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    const bool emptyNull = HudUiMgr::IsLocalPlayerFirstInStatsList() == -1;

    triplet.entries.begin = entries;
    triplet.entries.end = entries + 2;
    triplet.entries.cap = entries + 2;

    g_zNetwork_LocalPlayerKey = 1001;
    const bool matchFirst = HudUiMgr::IsLocalPlayerFirstInStatsList() == 1;

    g_zNetwork_LocalPlayerKey = 2002;
    const bool otherEntryDoesNotMatch = HudUiMgr::IsLocalPlayerFirstInStatsList() == 0;

    g_HudUiMgrStatsList = oldStatsList;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    return emptyNull && matchFirst && otherEntryDoesNotMatch ? 0 : 1;
}

extern "C" int zhud_text_stack_layout_mutators_smoke(void) {
    HudUiTopMessageStack stack = {};
    stack.Constructor();

    stack.SetTextColors(
        0x00112233,
        0x00445566
    );
    stack.SetXAll(0x155);
    stack.SetYDescending(0x88);

    bool updated = true;
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = TextStackLineAtForSmoke(
            &stack,
            index
        );
        updated = updated && panel->textColor0 == 0x00112233 &&
                  panel->textColor1 == 0x00445566 && panel->textDirty == 1 &&
                  panel->x == 0x155 && panel->y == 0x88 - index * 0x12;
    }

    DeleteTextStackLineFontsForSmoke(&stack);
    return updated ? 0 : 1;
}
