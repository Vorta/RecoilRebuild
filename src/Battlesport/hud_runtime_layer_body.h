#pragma once

namespace {
struct HudReticleAttachStatePartial {
    unsigned char unknown_00[0x0c];
    zClass_NodePartial *projectileNode;
};

struct HudReticleAltGunControllerPartial {
    OptCatalogEntryDef *optCatalogEntry;
    unsigned char unknown_04[0x24];
    HudReticleAttachStatePartial *attachState;
};

struct HudReticlePlayerStatePartial {
    unsigned char unknown_000[0x58c];
    int cameraState;
    unsigned char unknown_590[0x54];
    HudReticleAltGunControllerPartial *activeAltGunController;
    unsigned char unknown_5e8[0x8e8];
    zClass_NodePartial *rootNode;
};

RECOIL_STATIC_ASSERT(offsetof(HudReticleAttachStatePartial, projectileNode) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(HudReticleAltGunControllerPartial, attachState) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(HudReticlePlayerStatePartial, cameraState) == 0x58c);
RECOIL_STATIC_ASSERT(offsetof(HudReticlePlayerStatePartial, activeAltGunController) == 0x5e4);
RECOIL_STATIC_ASSERT(offsetof(HudReticlePlayerStatePartial, rootNode) == 0xed0);
} // namespace

struct zTimedTask {
    zTimedTask *next;
    int kind;
    int flags;
    float remainingSeconds;
    int actionArg0;
    int actionArg1;
    int actionArg2;
    int actionArg3;
    int actionArg4;
    unsigned char payload_24[0x94];
    int alphaPointCount;
    int alphaVariantIndex;
    int alpha255;
    unsigned char payload_c4[0x48];
    int rasterVertexCount;
    int rasterDrawParam;

    void RemoveFromActiveList();
    void RunImmediateAction();
    static void TickActiveList();
};

RECOIL_STATIC_ASSERT(offsetof(zTimedTask, next) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, kind) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, flags) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, remainingSeconds) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, actionArg0) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, actionArg4) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, alphaPointCount) == 0xb8);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, alphaVariantIndex) == 0xbc);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, alpha255) == 0xc0);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, rasterVertexCount) == 0x10c);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, rasterDrawParam) == 0x110);

extern char g_HudCfgKey_Modes[6];
extern char g_HudCfgKey_Weapon[7];
extern char g_HudCfgKey_Target[7];
extern char g_HudCfgKey_Shield[7];
extern char g_HudUiBlankSpaces8[9];
extern char g_HudCfgKey_Stats[6];
extern char g_HudCfgKey_Reticule[9];
extern char g_HudCfgKey_Objective[10];
extern char g_HudCfgKey_Sensor[7];
extern char g_HudCfgKey_Nanite[7];
extern char g_HudCfgKey_Ammo[5];
extern char g_HudCfgKey_Strings[8];
extern char g_HudCfgKey_ObjectiveDescription[16];
extern char g_HudCfgKey_ObjectiveSummary[12];
extern char g_HudCfgKey_Fonts[6];
extern char g_Hud_ImageSearchPath_Hud[26];
extern char g_Hud_SourceFile_HudCpp[28];
extern char g_HudLayout_TypeISectionName[];
extern char g_HudLayout_TypeIISectionName[];
extern char g_HudUiBlankSpaces3[4];
extern char g_HudUiTimerPanel_ZeroTimeString[9];
extern char g_HudUiMessage_ClearSpecialToken165[4];
union HudUiSensorWindowStorage {
    unsigned long align;
    unsigned char bytes[0x40];
};
RECOIL_STATIC_ASSERT(sizeof(HudUiSensorWindowStorage) == 0x40);
extern HudUiSensorWindowStorage g_HudUiSensorWindow;
extern zFMV_Playback *g_HudUiSensorWindowPlayback;
extern char g_Hud_CheckpointOverflowMsg[20];

namespace {
const float kHudUiMessageClearSpecialTokenValue = 123456792.0f;

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4137c0 HudUiAuxOverlay::ClearTextLines callers.
 * Purpose: preserve the recovered HUD behavior for HudUiZrdPayload.
 */
zReader::Node *HudUiZrdPayload(
    zReader::Node *node
) {
    return node != 0 && node->type == zReader::ZRDR_NODE_ARRAY ? node->value.nodes : 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4137c0 HudUiAuxOverlay::ClearTextLines callers.
 * Purpose: preserve the recovered HUD behavior for HudUiZrdStringAt.
 */
const char *HudUiZrdStringAt(
    zReader::Node *payload,
    int index
) {
    return payload != 0 ? payload[index].value.str : 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4137c0 HudUiAuxOverlay::ClearTextLines callers.
 * Purpose: preserve the recovered HUD behavior for HudUiEnsureLoaderWidgetsConstructed.
 */
void HudUiEnsureLoaderWidgetsConstructed() {
    g_HudUiMgrSensorPanel.Constructor(0);
    g_HudUiMgrSensorOverlay.Constructor(0);
    new (&g_HudUiMgrSensorMeter) HudUiMeter;
    g_HudUiMgrObjectiveWidget.Constructor(0);
    g_HudUiMgrObjectiveSensorRect.Constructor(0);
    new ((HudUiBar *)(&g_HudUiMgrObjectiveBar)) HudUiBar;
    g_HudUiMgrReticleWidget.Constructor(0);
    ((HudUiTripletPanel *)(&g_HudUiMgrNanitePanel))->Constructor();

    {
        int messageIndex1;
        for (messageIndex1 = 0;
            messageIndex1 < (int)(sizeof(g_HudUiMgrMessages) / sizeof(g_HudUiMgrMessages[0]));
            ++messageIndex1) {
            HudUiMessage &message = g_HudUiMgrMessages[messageIndex1];
            message.Constructor();
        }
    }

    {
        int counterIndex2;
        for (counterIndex2 = 0; counterIndex2 < (int)(sizeof(g_HudUiMgrModeCounters) /
                                                      sizeof(g_HudUiMgrModeCounters[0]));
            ++counterIndex2) {
            HudUiCounter &counter = g_HudUiMgrModeCounters[counterIndex2];
            new (&counter) HudUiCounter;
        }
    }

    {
        int slotIndex3;
        for (slotIndex3 = 0;
            slotIndex3 < (int)(sizeof(g_HudUiMgrWeaponSlots) / sizeof(g_HudUiMgrWeaponSlots[0]));
            ++slotIndex3) {
            HudUiSlot &slot = g_HudUiMgrWeaponSlots[slotIndex3];
            slot.Constructor();
        }
    }
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x40d7e0 HudUiMgr::Constructor callers.
 * Purpose: preserve the recovered HUD behavior for HudUiSetFontFromRect.
 */
void HudUiSetFontFromRect(
    HudUiPanel *panel,
    const HudUiRect &fontSpec
) {
    panel->SetFont(
        (const char *)(fontSpec.left),
        fontSpec.right,
        fontSpec.bottom,
        fontSpec.top,
        0,
        0,
        2
    );
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x40d7e0 HudUiMgr::Constructor callers.
 * Purpose: preserve the recovered HUD behavior for HudUiSetPanelClipWithSource.
 */
void HudUiSetPanelClipWithSource(
    HudUiPanel *panel,
    void *source,
    const HudUiRect *clipRect
) {
    panel->SetClip(
        source,
        clipRect
    );
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x40d7e0 HudUiMgr::Constructor callers.
 * Purpose: preserve the recovered HUD behavior for HudUiApplyStatsTripletInt3.
 */
void HudUiApplyStatsTripletInt3(
    zReader::Node *payload,
    int nodeIndex,
    int &outX,
    int &outY,
    int *outZ = 0
) {
    HudUiLayoutNode::ReadInt3(
        &payload[nodeIndex],
        &outX,
        &outY,
        outZ
    );
}

} // namespace

namespace HudUiMgr {
/**
 * Reimplements 0x410160: HudUiMgr::EnsureHudLoaded.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: load the HUD archive tree, construct the HudUiMgr singleton-owned
 * widgets, initialize layout resources, and finalize HUD visibility state.
 */
int __fastcall EnsureHudLoaded(
    const char *entryPath
) {
    if (g_HudUiMgrHudLoaded != 0) {
        return 1;
    }

    zReader::Node *const root = zReader::LoadNodeFromPath(
        entryPath,
        0,
        0
    );
    if (root == 0) {
        zError::ReportOld(
            0x200,
            g_Hud_SourceFile_HudCpp,
            0x60d,
            g_HudSensorTracker_ReadFileFailedFmt,
            entryPath
        );
        return 0;
    }

    HudUiEnsureLoaderWidgetsConstructed();

    zImage_InitMissionResources(g_Hud_ImageSearchPath_Hud);
    g_HudLayoutSW.LoadTypeIFromZarRoot(root);
    g_HudLayoutHW.LoadTypeIIFromZarRoot(root);
    SwitchActiveDialog(&g_HudLayoutSW);

    HudUiRect objectiveSummaryFont = {0};
    HudUiRect objectiveDescriptionFont = {0};
    HudUiRect ammoFont = {0};

    zReader::Node *const fontsNode = zReader_GetNamedNode(
        root,
        g_HudCfgKey_Fonts
    );
    if (fontsNode != 0) {
        if (zReader::Node *const node = zReader_GetNamedNode(
            fontsNode,
            g_HudCfgKey_ObjectiveSummary
        )) {
            HudUiLayoutNode::ReadRect(
                node,
                &objectiveSummaryFont
            );
        }
        if (zReader::Node *const node = zReader_GetNamedNode(
            fontsNode,
            g_HudCfgKey_ObjectiveDescription
        )) {
            HudUiLayoutNode::ReadRect(
                node,
                &objectiveDescriptionFont
            );
        }
        if (zReader::Node *const node = zReader_GetNamedNode(
            fontsNode,
            g_HudCfgKey_Strings
        )) {
            HudUiPanelFontParams *const fontArgs =
                (HudUiPanelFontParams *)(&g_HudUiMgrStringMenu->unknown_10[0]);
            HudUiLayoutNode::ReadRect(
                node,
                (HudUiRect *)(fontArgs)
            );
            {
                int itemIndex4;
                for (itemIndex4 = 0; itemIndex4 < (int)(sizeof(g_HudUiMgrStringMenu->items) /
                                                        sizeof(g_HudUiMgrStringMenu->items[0]));
                    ++itemIndex4) {
                    HudUiPanelSimple &item = g_HudUiMgrStringMenu->items[itemIndex4];
                    item.SetFont(
                        fontArgs->faceName,
                        fontArgs->height,
                        fontArgs->weight,
                        fontArgs->width,
                        0,
                        0,
                        2
                    );
                }
            }
        }
        if (zReader::Node *const node = zReader_GetNamedNode(
            fontsNode,
            "MESSAGES"
        )) {
            HudUiRect messagesFont = {0};
            HudUiLayoutNode::ReadRect(
                node,
                &messagesFont
            );
            if (g_HudUiTopMessageStack != 0) {
                g_HudUiTopMessageStack->SetFontAll(
                    (const char *)(messagesFont.left),
                    messagesFont.right,
                    messagesFont.bottom,
                    messagesFont.top
                );
            }
            if (g_HudUiChatMessageStack != 0) {
                g_HudUiChatMessageStack->SetFontAll(
                    (const char *)(messagesFont.left),
                    messagesFont.right,
                    messagesFont.bottom,
                    messagesFont.top
                );
            }
        }
        if (zReader::Node *const node = zReader_GetNamedNode(
            fontsNode,
            g_HudCfgKey_Ammo
        )) {
            HudUiLayoutNode::ReadRect(
                node,
                &ammoFont
            );
        }
    }

    if (zReader::Node *const naniteNode = zReader_GetNamedNode(
        root,
        g_HudCfgKey_Nanite
    )) {
        g_HudUiMgrNanitePanel.InitLayout(naniteNode);
    }

    zReader::Node *const sensorNode = zReader_GetNamedNode(
        root,
        g_HudCfgKey_Sensor
    );
    int sensorCenterX = 0;
    int sensorCenterY = 0;
    if (zReader::Node *const sensorPayload = HudUiZrdPayload(sensorNode)) {
        HudUiLayoutNode::ApplyImageWidget(
            &sensorPayload[1],
            &g_HudUiMgrSensorPanel,
            0,
            g_HudUiMgrHudOriginY,
            0,
            0,
            &g_HudUiMgrSensorBlock.sensorViewportRect
        );

        sensorCenterX = g_HudUiMgrSensorPanel.GetCenterX();
        sensorCenterY = g_HudUiMgrSensorPanel.GetCenterY();
        g_HudUiMgrSensorBlock.sensorParam = sensorPayload[2].value.f32;

        int sensorOffsetX = 0;
        int sensorOffsetY = 0;
        int sensorWidth = 0;
        int sensorHeight = 0;
        HudUiLayoutNode::ReadInt3(
            &sensorPayload[3],
            &sensorOffsetX,
            &sensorOffsetY,
            0
        );
        HudUiLayoutNode::ReadInt3(
            &sensorPayload[4],
            &sensorWidth,
            &sensorHeight,
            0
        );

        const int sensorX = sensorCenterX + sensorOffsetX;
        const int sensorY = sensorCenterY + sensorOffsetY;
        g_HudUiMgrSensorFxRect.left = sensorX;
        g_HudUiMgrSensorFxRect.top = sensorY;
        g_HudUiMgrSensorFxRect.right = sensorX + sensorWidth;
        g_HudUiMgrSensorFxRect.bottom = sensorY + sensorHeight;
        g_HudUiMgrSensorFxViewportWidth = sensorWidth;
        g_HudUiMgrSensorFxViewportHeight = sensorHeight;
        HudUiMgrSensor::SetViewportRect(
            sensorX,
            sensorY,
            sensorWidth,
            sensorHeight
        );

        const float range = sensorPayload[5].value.f32;
        float rangeBitsValue = range * range * 0.5f;
        unsigned int rangeBits = 0;
        memcpy(
            &rangeBits,
            &rangeBitsValue,
            sizeof(rangeBits)
        );
        rangeBits = (rangeBits >> 1) + 0x1fc00000u;
        memcpy(
            &rangeBitsValue,
            &rangeBits,
            sizeof(rangeBitsValue)
        );
        g_HudUiMgrSensorBlock.sensorRangeSq = rangeBitsValue + rangeBitsValue;

        const int overlayAnchor[2] = {sensorCenterX, sensorCenterY};
        HudUiLayoutNode::ApplyImageWidget(
            &sensorPayload[6],
            &g_HudUiMgrSensorOverlay,
            0,
            0,
            overlayAnchor,
            0,
            0
        );

        HudUiRect meterRect = {0};
        HudUiLayoutNode::ApplyMeterQuad(
            &sensorPayload[7],
            &g_HudUiMgrSensorMeter,
            0,
            0,
            overlayAnchor,
            &meterRect
        );
        g_HudUiMgrSensorMeter.color565 = 0x7e0;
        ((HudUiElement *)(&g_HudUiMgrSensorMeter))
            ->SetBltSourceAndClipRect(
                g_HudUiMgrSensorPanel.image,
                &meterRect
            );

        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrSensorPanel));
        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrSensorOverlay));
        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrSensorMeter));
    }

    if (zReader::Node *const objectivePayload =
            HudUiZrdPayload(zReader_GetNamedNode(
                root,
                g_HudCfgKey_Objective
            ))) {
        g_HudUiMgrObjectivePhaseDurationSec = objectivePayload[1].value.f32;

        const int panelCenter[2] = {sensorCenterX != 0 ? sensorCenterX
                                                       : g_HudUiMgrSensorPanel.GetCenterX(),
            sensorCenterY != 0 ? sensorCenterY : g_HudUiMgrSensorPanel.GetCenterY()};
        HudUiLayoutNode::ApplyImageWidget(
            &objectivePayload[2],
            &g_HudUiMgrObjectiveWidget,
            0,
            0,
            panelCenter,
            0,
            0
        );

        int objectiveCenter[2] = {g_HudUiMgrObjectiveWidget.GetCenterX(),
            g_HudUiMgrObjectiveWidget.GetCenterY()};
        HudUiRect objectiveBarRect = {0};
        HudUiLayoutNode::ApplyCornerTextQuad(
            &objectivePayload[3],
            &g_HudUiMgrObjectiveBar,
            objectiveCenter,
            &objectiveBarRect
        );
        g_HudUiMgrObjectiveBar.slideRangeX =
            (float)(panelCenter[0] - objectiveBarRect.left);

        int red = 0;
        int green = 0;
        int blue = 0;
        HudUiLayoutNode::ReadInt3(
            &objectivePayload[4],
            &red,
            &green,
            &blue
        );
        g_HudUiMgrObjectiveBar.drawParam =
            zVid_PackColorRGB(
                (unsigned char)(red),
                (unsigned char)(green),
                (unsigned char)(blue)
            ) &
            0xffffu;

        int x = 0;
        int y = 0;
        HudUiLayoutNode::ReadInt3(
            &objectivePayload[5],
            &x,
            &y,
            0
        );
        ((HudUiElement *)(g_HudUiMgrObjectiveSummaryTextPanel))
            ->SetPos(
                objectiveCenter[0] + x,
                objectiveCenter[1] + y
            );
        HudUiLayoutNode::ReadInt3(
            &objectivePayload[6],
            &x,
            &y,
            0
        );
        ((HudUiElement *)(g_HudUiMgrObjectiveDescTextPanel))
            ->SetPos(
                objectiveCenter[0] + x,
                objectiveCenter[1] + y
            );

        HudUiRect wrapRect = {0};
        wrapRect.left = 0;
        wrapRect.top = 0;
        wrapRect.right = panelCenter[0] - x * 2 - objectiveBarRect.left;
        wrapRect.bottom = panelCenter[1] - objectiveBarRect.bottom;
        g_HudUiMgrObjectiveDescTextPanel->EnableWordWrapWithRect(&wrapRect);

        HudUiLayoutNode::ApplyMeterQuad(
            &objectivePayload[7],
            &g_HudUiMgrObjectiveMeter,
            0,
            0,
            objectiveCenter,
            &objectiveBarRect
        );
        HudUiMgrObjective::UpdateMeterXPoints();
        const int meterTop = (int)(g_HudUiMgrObjectiveMeter.points[1].y) -
                             (int)(ceil((double)(g_HudUiMgrObjectiveMeter.fillPixelsMax)));
        g_HudUiMgrObjectiveMeter.color565 = 0x1f;
        g_HudUiMgrObjectiveMeter.points[0].y = (float)(meterTop);
        g_HudUiMgrObjectiveMeter.points[3].y = (float)(meterTop);

        HudUiLayoutNode::ReadInt3(
            &objectivePayload[8],
            &x,
            &y,
            0
        );
        ((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))->SetPos(
            x,
            y + g_HudUiMgrHudOriginY
        );
        g_HudUiMgrObjectiveLabelTextPanel->SetTextFmt(
            "%s",
            zLoc::GetMessageString(0x906)
        );
        ((HudUiElement *)(&g_HudUiMgrObjectiveSensorRect))
            ->SetPos(
                g_HudUiMgrSensorFxRect.left,
                g_HudUiMgrSensorFxRect.top
            );

        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrObjectiveWidget));
        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrObjectiveSensorRect));
        g_HudUiMgr.AddChild(&g_HudUiMgrObjectiveBar);
        g_HudUiMgr.AddChild((HudUiElement *)(g_HudUiMgrObjectiveSummaryTextPanel));
        g_HudUiMgr.AddChild((HudUiElement *)(g_HudUiMgrObjectiveDescTextPanel));
        g_HudUiMgr.AddChild((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel));
        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrObjectiveMeter));
        g_HudUiMgrObjectiveBar.SetVisible(0);

        g_HudUiMgrObjectiveState = 0;
        g_HudUiMgrObjectivePhase = 0;
        g_HudUiMgrObjectivePhaseTimerSec = 0.0f;
        g_HudUiMgrObjectiveChatComposeActive = 0;
        HudUiSetFontFromRect(
            g_HudUiMgrObjectiveDescTextPanel,
            objectiveDescriptionFont
        );
        HudUiSetFontFromRect(
            g_HudUiMgrObjectiveSummaryTextPanel,
            objectiveSummaryFont
        );
    }

    if (zReader::Node *const reticlePayload =
            HudUiZrdPayload(zReader_GetNamedNode(
                root,
                g_HudCfgKey_Reticule
            ))) {
        g_HudUiMgrReticleImages[0] =
            zImage::TexDir_FindOrCreateByPath(HudUiZrdStringAt(
                reticlePayload,
                1
            ));
        g_HudUiMgrReticleImages[1] =
            zImage::TexDir_FindOrCreateByPath(HudUiZrdStringAt(
                reticlePayload,
                2
            ));
        g_HudUiMgrReticleImages[2] =
            zImage::TexDir_FindOrCreateByPath(HudUiZrdStringAt(
                reticlePayload,
                3
            ));
        g_HudUiMgrReticleWidget.SetImageBorrowedAndInvalidate(g_HudUiMgrReticleImages[0]);
        g_HudUiMgrReticleWidget.imageStateWord =
            (g_HudUiMgrReticleWidget.imageStateWord & 0xffff0000u) | 1u;
        ((HudUiElement *)(&g_HudUiMgrReticleWidget))->Invalidate();
        zVidImagePartial *const image = g_HudUiMgrReticleWidget.image;
        g_HudUiMgrReticleWidgetHalfW = image != 0 ? (short)(image->width) / 2 : 0;
        g_HudUiMgrReticleWidgetHalfH = image != 0 ? (short)(image->height) / 2 : 0;
        ((HudUiElement *)(&g_HudUiMgrReticleWidget))->SetVisible(0);
    }

    if (zReader::Node *const statsPayload = HudUiZrdPayload(zReader_GetNamedNode(
        root,
        g_HudCfgKey_Stats
    ))) {
        HudUiWidget *const layoutWidget = &g_HudLayoutHW.widget1;
        const int layoutCenterX = layoutWidget->GetCenterX();
        const int layoutCenterY = layoutWidget->GetCenterY();
        int x = 0;
        int y = 0;
        int z = 0;
        HudUiLayoutNode::ReadInt3(
            &statsPayload[1],
            &x,
            &y,
            0
        );
        const int counterX = (g_HudUiMgrHudOriginX / 2) + x;
        ((HudUiElement *)(g_HudUiMgrObjectiveCounterTextPanel))
            ->SetPos(
                counterX + layoutCenterX,
                y + layoutCenterY
            );
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->alignMode = 1;
        HudUiRect counterClip = {counterX - 0x14, y, counterX + 0x14, y + 0x0a};
        HudUiSetPanelClipWithSource(
            g_HudUiMgrObjectiveCounterTextPanel,
            0,
            &counterClip
        );
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->SetTextFmt(g_HudUiBlankSpaces8);
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->UpdateTextBoundsFromContent();
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->SetTextFmt(
            "%d",
            0
        );
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->UpdateTextBoundsFromContent();

        HudUiLayoutNode::ReadInt3(
            &statsPayload[2],
            &x,
            &y,
            0
        );
        const int timerX = x + g_HudUiMgrHudOriginX;
        ((HudUiElement *)(g_HudUiMgrTimerPanel))->SetPos(
            timerX + layoutCenterX,
            y + layoutCenterY
        );
        HudUiRect timerClip = {timerX, y, 0, 0};
        HudUiSetPanelClipWithSource(
            g_HudUiMgrTimerPanel,
            0,
            &timerClip
        );
        ((HudUiPanel *)(g_HudUiMgrTimerPanel))->SetTextFmt(g_HudUiTimerPanel_ZeroTimeString);

        HudUiTriplet *const triplet = g_HudUiMgrStatsList->triplet;
        HudUiApplyStatsTripletInt3(
            statsPayload,
            3,
            x,
            y,
            &z
        );
        triplet->baseXStart = x + layoutCenterX + g_HudUiMgrHudOriginX;
        triplet->baseYStart = y + layoutCenterY;
        triplet->rowPitchYStart = z;
        HudUiApplyStatsTripletInt3(
            statsPayload,
            4,
            x,
            y,
            &z
        );
        triplet->baseXEnd = x + layoutCenterX + g_HudUiMgrHudOriginX;
        triplet->baseYEnd = y + layoutCenterY;
        triplet->rowPitchYEnd = z;
        HudUiApplyStatsTripletInt3(
            statsPayload,
            5,
            triplet->lapsColumnOffsetXStart,
            triplet->lapsColumnOffsetXEnd
        );
        HudUiApplyStatsTripletInt3(
            statsPayload,
            6,
            triplet->killsColumnOffsetXStart,
            triplet->killsColumnOffsetXEnd
        );
        HudUiApplyStatsTripletInt3(
            statsPayload,
            7,
            triplet->fontSizeStart,
            triplet->fontSizeEnd
        );
        HudUiApplyStatsTripletInt3(
            statsPayload,
            8,
            triplet->fontWeightStart,
            triplet->fontWeightEnd
        );
        triplet->InterpolateLayout(0.0f);
        triplet->RebuildDisplay();
    }

    if (zReader::Node *const shieldNode = zReader_GetNamedNode(
        root,
        g_HudCfgKey_Shield
    )) {
        HudUiShieldMessageWidget::ApplyLayout(shieldNode);
    }

    if (zReader::Node *const targetPayload =
            HudUiZrdPayload(zReader_GetNamedNode(
                root,
                g_HudCfgKey_Target
            ))) {
        {
            for (int index = 0; index < 5; ++index) {
                zImage::TexDir_FindOrCreateByPath(HudUiZrdStringAt(
                    targetPayload,
                    index + 1
                ));
            }
        }

        {
            int slotIndex5;
            for (slotIndex5 = 0; slotIndex5 < (int)(sizeof(g_HudUiMgrWeaponSlots) /
                                                    sizeof(g_HudUiMgrWeaponSlots[0]));
                ++slotIndex5) {
                HudUiSlot &slot = g_HudUiMgrWeaponSlots[slotIndex5];
                slot.trackMarkerWidget.imageStateWord =
                    (slot.trackMarkerWidget.imageStateWord & 0xffff0000u) | 1u;
                ((HudUiElement *)(&slot.trackMarkerWidget))->Invalidate();
            }
        }

        {
            int slotIndex6;
            for (slotIndex6 = 0; slotIndex6 < (int)(sizeof(g_HudUiMgrWeaponSlots) /
                                                    sizeof(g_HudUiMgrWeaponSlots[0]));
                ++slotIndex6) {
                HudUiSlot &slot = g_HudUiMgrWeaponSlots[slotIndex6];
                ((HudUiElement *)(&slot.slotWidget))->Invalidate();
                g_HudUiMgr.AddChild(&slot);
                ((HudUiElement *)(&slot.trackMarkerWidget))->SetVisible(0);
                ((HudUiElement *)(&slot.slotWidget))->SetVisible(0);
            }
        }
        g_HudUiMgrSensorTargetMarkerCount = 0;
        g_HudUiMgrWeaponState = 0;
    }

    zReader::Node *weaponPayload = HudUiZrdPayload(zReader_GetNamedNode(
        root,
        g_HudCfgKey_Weapon
    ));
    if (weaponPayload != 0) {
        {
            for (int index = 1; index < 10; ++index) {
                g_HudUiMgrMessages[index].LoadWeaponLayoutFromNode(
                    &weaponPayload[index],
                    (const HudUiPanelFontParams *)(&ammoFont)
                );
            }
        }
    }

    zReader::Node *modesPayload = HudUiZrdPayload(zReader_GetNamedNode(
        root,
        g_HudCfgKey_Modes
    ));
    if (modesPayload != 0) {
        {
            for (int index = 0; index < 4; ++index) {
                g_HudUiMgrModeCounters[index].ApplyFromLayoutNode(&modesPayload[index + 1]);
            }
        }
    }

    SetModeCounterState(
        0,
        2
    );
    zReader::FreeLoadedTree(root);
    SetFloatTimerVisible(0);
    SetAuxOverlayVisible(0);
    g_HudUiMgrHudLoaded = 1;
    return 1;
}

} // namespace HudUiMgr

namespace HudUiSensorWindow {
CWnd *StaticInit();
int RegisterAtExit();
void AtExitDestructor();

/**
 * Reimplements 0x4136f0: HudUiSensorWindow::StaticInitAndRegisterAtExit.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: construct the global HUD sensor CWnd and register its static
 * destructor during CRT startup.
 */
void StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

/**
 * Reimplements 0x413700: HudUiSensorWindow::StaticInit.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: default-construct the global HUD sensor CWnd in its static storage.
 */
CWnd *StaticInit() {
    return new (&g_HudUiSensorWindow) CWnd;
}

/**
 * Reimplements 0x413710: HudUiSensorWindow::RegisterAtExit.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: register the global HUD sensor CWnd destructor with the CRT
 * at-exit list.
 */
int RegisterAtExit() {
    return atexit(AtExitDestructor);
}

/**
 * Reimplements 0x413720: HudUiSensorWindow::AtExitDestructor.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: destroy the global HUD sensor CWnd during CRT shutdown.
 */
void AtExitDestructor() {
    ((CWnd *)&g_HudUiSensorWindow)->~CWnd();
}
} // namespace HudUiSensorWindow

namespace HudUiMgr {
/**
 * Reimplements 0x413730: HudUiMgr::DestroySensorWindow.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::DestroySensorWindow.
 */
void DestroySensorWindow() {
    zFMV_Playback *playback = g_HudUiSensorWindowPlayback;
    if (playback == 0) {
        return;
    }

    playback->StopAndClose();

    playback = g_HudUiSensorWindowPlayback;
    if (playback != 0) {
        playback->Destructor();
        ::operator delete(playback);
    }

    g_HudUiSensorWindowPlayback = 0;
    ((CWnd *)&g_HudUiSensorWindow)->CWnd::DestroyWindow();
}

/**
 * Reimplements 0x413770: HudUiMgr::SetFloatTimerVisible.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: apply the recovered HUD state change handled by HudUiMgr::SetFloatTimerVisible.
 */
void __fastcall SetFloatTimerVisible(
    int visible
) {
    g_HudUiMgrTimerPanelFloat->SetVisible(visible != 0 ? 1 : 0);

    if (visible == 0) {
        TriggerCurrentLayoutOnActivated();
    }
}

/**
 * Reimplements 0x4137a0: HudUiMgr::SetAuxOverlayVisible.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: apply the recovered HUD state change handled by HudUiMgr::SetAuxOverlayVisible.
 */
void __fastcall SetAuxOverlayVisible(
    int visible
) {
    g_HudUiMgrStringMenu->SetEnabled(visible != 0 ? 1 : 0);
}
} // namespace HudUiMgr

namespace HudUiAuxOverlay {
/**
 * Reimplements 0x4137c0: HudUiAuxOverlay::ClearTextLines.
 * Purpose: clear and hide every sensor overlay text line.
 */
void ClearTextLines() {
    {
        for (int index = 0; index < 23; ++index) {
            UpdateTextLine(
                2,
                index,
                ""
            );
            UpdateTextLine(
                0,
                index,
                0
            );
        }
    }
}

/**
 * Reimplements 0x4137f0: HudUiAuxOverlay::ApplyTextLineOp.
 * Purpose: apply one sensor overlay text-line operation to a string-menu item.
 */
void __fastcall UpdateTextLine(
    int op,
    int index,
    const char *format
) {
    HudUiPanel *const panel = (HudUiPanel *)(&g_HudUiMgrStringMenu->items[index]);

    if (op == 1) {
        panel->SetTextFmt(format);
        panel->SetVisible(1);
        return;
    }

    if (op == 0) {
        panel->SetVisible(0);
        return;
    }

    if (op == 2) {
        if (*format != '\0') {
            panel->SetTextFmt(format);
            panel->SetVisible(1);
        } else {
            panel->SetVisible(0);
        }
    }
}
} // namespace HudUiAuxOverlay

namespace HudUi {
/**
 * Reimplements 0x4138d0: HudUi::ShowTopMessageLine.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: show a top HUD message when the top-message stack is enabled.
 */
void __fastcall ShowTopMessageLine(
    const char *message,
    float duration
) {
    HudUiTextStack4 *const topStack = g_HudUiTopMessageStack;
    if (topStack->enabled != 0) {
        topStack->PushLine(
            message,
            duration
        );
    }
}

/**
 * Reimplements 0x4138f0: HudUi::ShowChatLine.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: show a chat HUD message when the chat stack is enabled.
 */
void __fastcall ShowChatLine(
    const char *message,
    float duration
) {
    HudUiTextStack4 *const chatStack = g_HudUiChatMessageStack;
    if (chatStack->enabled != 0) {
        chatStack->PushLine(
            message,
            duration
        );
    }
}
} // namespace HudUi

namespace HudUiMgr {
/**
 * Reimplements 0x413910: HudUiMgr::EnableTopAndChatStacks.
 * Purpose: clear and enable the global top-message and chat text stacks.
 */
void EnableTopAndChatStacks() {
    g_HudUiTopMessageStack->Clear();
    g_HudUiTopMessageStack->SetEnabled(1);
    g_HudUiChatMessageStack->Clear();
    g_HudUiChatMessageStack->SetEnabled(1);
}

/**
 * Reimplements 0x413950: HudUiMgr::DisableTopAndChatStacks.
 * Purpose: clear and disable the global top-message and chat text stacks.
 */
void DisableTopAndChatStacks() {
    g_HudUiTopMessageStack->Clear();
    g_HudUiTopMessageStack->SetEnabled(0);
    g_HudUiChatMessageStack->Clear();
    g_HudUiChatMessageStack->SetEnabled(0);
}
} // namespace HudUiMgr

namespace HudUiLayoutNode {
/**
 * Reimplements 0x413990: HudUiLayoutNode::ApplyTextLabel.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: apply the recovered HUD layout or option state handled by HudUiLayoutNode::ApplyTextLabel.
 */
int __fastcall ApplyTextLabel(
    zReader::Node *layoutNode,
    HudUiPanel *target,
    int baseX,
    int baseY,
    const int *offsetXY
) {
    if (layoutNode->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const payload = layoutNode->value.nodes;
    const char *const text = payload[1].value.str;
    int x = payload[2].value.i32 + baseX;
    int y = payload[3].value.i32 + baseY;
    if (offsetXY != 0) {
        x += offsetXY[0];
        y += offsetXY[1];
    }

    target->SetPos(
        x,
        y
    );
    target->SetTextFmt(
        text != 0 ? text : ""
    );
    return 1;
}

/**
 * Reimplements 0x413a10: HudUiLayoutNode::ReadRectOffsetAndSize.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: read recovered HUD ZRD/layout data for HudUiLayoutNode::ReadRectOffsetAndSize.
 */
int __fastcall ReadRectOffsetAndSize(
    zReader::Node *node,
    HudUiRect *outRect,
    const int *offsetXY,
    int *outWidth,
    int *outHeight
) {
    if (node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const arrayBase = node->value.nodes;
    outRect->left = arrayBase[1].value.i32;
    outRect->top = arrayBase[2].value.i32;
    outRect->right = arrayBase[3].value.i32;
    outRect->bottom = arrayBase[4].value.i32;

    if (offsetXY != 0) {
        outRect->left += offsetXY[0];
        outRect->top += offsetXY[1];
        outRect->right += offsetXY[0];
        outRect->bottom += offsetXY[1];
    }

    if (outWidth != 0) {
        *outWidth = outRect->right - outRect->left;
    }

    if (outHeight != 0) {
        *outHeight = outRect->bottom - outRect->top;
    }

    return 1;
}

/**
 * Reimplements 0x413aa0: HudUiLayoutNode::ReadRect.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: read recovered HUD ZRD/layout data for HudUiLayoutNode::ReadRect.
 */
int __fastcall ReadRect(
    zReader::Node *node,
    HudUiRect *outRect
) {
    if (node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const arrayBase = node->value.nodes;
    outRect->left = arrayBase[1].value.i32;
    outRect->right = arrayBase[2].value.i32;
    outRect->top = arrayBase[3].value.i32;
    outRect->bottom = arrayBase[4].value.i32;
    return 1;
}

/**
 * Reimplements 0x413ad0: HudUiLayoutNode::ReadInt3.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: read recovered HUD ZRD/layout data for HudUiLayoutNode::ReadInt3.
 */
int __fastcall ReadInt3(
    zReader::Node *node,
    int *out0,
    int *out1,
    int *out2
) {
    if (node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const arrayBase = node->value.nodes;
    if (out0 != 0) {
        *out0 = arrayBase[1].value.i32;
    }

    if (out1 != 0) {
        *out1 = arrayBase[2].value.i32;
    }

    if (out2 != 0) {
        *out2 = arrayBase[3].value.i32;
    }

    return 1;
}

/**
 * Reimplements 0x413b10: HudUiLayoutNode::ApplyCornerTextQuad.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: apply the recovered HUD layout or option state handled by HudUiLayoutNode::ApplyCornerTextQuad.
 */
int __fastcall ApplyCornerTextQuad(
    zReader::Node *node,
    HudUiBar *target,
    const int *offsetXY,
    HudUiRect *outRect
) {
    if (node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const arrayBase = node->value.nodes;
    int left = arrayBase[1].value.i32;
    int top = arrayBase[2].value.i32;
    int right = arrayBase[3].value.i32;
    int bottom = arrayBase[4].value.i32;

    if (offsetXY != 0) {
        left += offsetXY[0];
        top += offsetXY[1];
        right += offsetXY[0];
        bottom += offsetXY[1];
    }

    const float leftF = (float)(left);
    const float topF = (float)(top);
    const float rightF = (float)(right);
    const float bottomF = (float)(bottom);
    target->SetPointXY(
        0,
        leftF,
        topF
    );
    target->SetPointXY(
        1,
        leftF,
        bottomF
    );
    target->SetPointXY(
        2,
        rightF,
        bottomF
    );
    target->SetPointXY(
        3,
        rightF,
        topF
    );

    if (outRect != 0) {
        outRect->left = left;
        outRect->top = top;
        outRect->right = right;
        outRect->bottom = bottom;
    }

    return 1;
}

/**
 * Reimplements 0x413c10: HudUiLayoutNode::ApplyMeterQuad.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: apply the recovered HUD layout or option state handled by HudUiLayoutNode::ApplyMeterQuad.
 */
int __fastcall ApplyMeterQuad(
    zReader::Node *node,
    HudUiMeter *target,
    int xBase,
    int yBase,
    const int *offsetXY,
    HudUiRect *outRect
) {
    if (node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const arrayBase = node->value.nodes;
    int left = arrayBase[1].value.i32;
    const int top = arrayBase[2].value.i32;
    int right = arrayBase[3].value.i32 + 1;
    const int bottom = arrayBase[4].value.i32 + 1;

    if (outRect != 0) {
        outRect->left = left;
        outRect->top = top;
        outRect->right = right;
        outRect->bottom = bottom;
    }

    right += xBase;
    int topY = top + yBase;
    int bottomY = bottom + yBase;

    if (offsetXY != 0) {
        left += offsetXY[0];
        topY += offsetXY[1];
        right += offsetXY[0];
        bottomY += offsetXY[1];
    }

    const int width = right - left;
    const int height = bottomY - topY;
    HudUiBar *const bar = (HudUiBar *)(target);
    bar->SetPointXY(
        0,
        (float)(left),
        (float)(topY)
    );
    bar->SetPointXY(
        1,
        (float)(left),
        (float)(height + topY)
    );
    bar->SetPointXY(
        2,
        (float)(width + left + 1),
        (float)(height + topY)
    );
    bar->SetPointXY(
        3,
        (float)(width + left + 1),
        (float)(topY)
    );

    target->fillPixelsMax = height;
    target->meterFlags = (unsigned int)(width);
    return 1;
}

/**
 * Reimplements 0x413d30: HudUiLayoutNode::ApplyImageWidget.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: apply the recovered HUD layout or option state handled by HudUiLayoutNode::ApplyImageWidget.
 */
zVidImagePartial *__fastcall ApplyImageWidget(
    zReader::Node *layoutNode,
    HudUiWidget *widget,
    int baseX,
    int baseY,
    const int *anchorOrNull,
    zVidImagePartial *preloadedImageOrNull,
    HudUiRect *outRectOrNull
) {
    if (layoutNode->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const payload = layoutNode->value.nodes;
    const char *const imagePath = payload[1].value.str;
    int x = payload[2].value.i32 + baseX;
    int y = payload[3].value.i32 + baseY;

    if (anchorOrNull != 0) {
        x += anchorOrNull[0];
        y += anchorOrNull[1];
    }

    unsigned short visibleState = 0;
    int centerImage = 0;
    if (payload[0].value.i32 == 6) {
        visibleState = (unsigned short)(strcmp(
            payload[4].value.str,
            "TRUE"
        ) == 0 ? 1 : 0);
        centerImage = strcmp(
            payload[5].value.str,
            "TRUE"
        ) == 0 ? 1 : 0;
    }

    zVidImagePartial *image = preloadedImageOrNull;
    if (image != 0) {
        widget->SetImageBorrowedAndInvalidate(image);
    } else {
        image = widget->SetImageByPathOwned(imagePath);
    }

    if (image == 0) {
        return 0;
    }

    if (centerImage != 0) {
        x -= (int)(image->width) / 2;
        y -= (int)(image->height) / 2;
    }

    widget->SetPos(
        x,
        y
    );
    widget->imageStateWord = (widget->imageStateWord & 0xffff0000u) | visibleState;
    widget->Invalidate();

    if (outRectOrNull != 0) {
        outRectOrNull->left = x;
        outRectOrNull->top = y;
        outRectOrNull->right = x + image->width;
        outRectOrNull->bottom = y + image->height;
    }

    return image;
}
} // namespace HudUiLayoutNode

namespace {
/**
 * Provider boundary for 0x413eb0: NoOp::MethodStub.
 * Purpose: preserve the compiler-generated no-op method target used by HUD
 * teardown stubs without modeling it as authored HUD source.
 */
void __fastcall HudUiNoOpMethodStub(
    void *
) {}
} // namespace

/**
 * Reimplements 0x413ec0: HudUiMessage::LoadWeaponLayoutFromNode.
 * Purpose: Load weapon-message images/layout and register the message owner and side widget with the HUD manager.
 */
int HudUiMessage::LoadWeaponLayoutFromNode(
    zReader::Node *layoutNode,
    const HudUiPanelFontParams *fontParams
) {
    if (layoutNode->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const payload = layoutNode->value.nodes;
    variantImages[0] = zImage::TexDir_FindOrCreateByPath(payload[1].value.str);
    variantImages[1] = zImage::TexDir_FindOrCreateByPath(payload[2].value.str);
    variantImages[2] = zImage::TexDir_FindOrCreateByPath(payload[3].value.str);
    variantImages[3] = zImage::TexDir_FindOrCreateByPath(payload[4].value.str);
    variantImages[4] = zImage::TexDir_FindOrCreateByPath(payload[5].value.str);
    sideImageSwaps[0] = zImage::TexDir_FindOrCreateByPath(payload[6].value.str);
    sideImageSwaps[1] = zImage::TexDir_FindOrCreateByPath(payload[7].value.str);
    panel.layoutX = payload[8].value.i32;
    panel.layoutY = payload[9].value.i32;

    RebuildWeaponLayout();

    imageStateWord = (imageStateWord & 0xffff0000u) | 1u;
    Invalidate();

    panel.centerText = 1;
    panel.textColor0 = 0x0020bf40;
    panel.textColor1 = 0x0020bf40;
    panel.textDirty = 1;
    panel.shadowOffsetX = -1;
    panel.shadowOffsetY = -1;
    panel.shadowEnabled = 1;

    panel.SetFont(
        fontParams->faceName,
        fontParams->height,
        fontParams->weight,
        fontParams->width,
        0,
        0,
        2
    );
    panel.SetTextFmt(g_HudUiBlankSpaces3);

    g_HudUiMgr.AddChild(this);
    g_HudUiMgr.AddChild(&widget);
    return 1;
}

/**
 * Reimplements 0x413ff0: HudUiMessage::ReleaseImages.
 * Purpose: Releases all borrowed weapon-message variant and side-image swap references and clears their storage.
 */
void HudUiMessage::ReleaseImages() {
    zVid_Image::ReleaseIfNotDefault(variantImages[0]);
    zVid_Image::ReleaseIfNotDefault(variantImages[1]);
    zVid_Image::ReleaseIfNotDefault(variantImages[2]);
    zVid_Image::ReleaseIfNotDefault(variantImages[3]);
    zVid_Image::ReleaseIfNotDefault(variantImages[4]);
    zVid_Image::ReleaseIfNotDefault(sideImageSwaps[0]);
    zVid_Image::ReleaseIfNotDefault(sideImageSwaps[1]);

    sideImageSwaps[1] = 0;
    sideImageSwaps[0] = 0;
    variantImages[4] = 0;
    variantImages[3] = 0;
    variantImages[2] = 0;
    variantImages[1] = 0;
    variantImages[0] = 0;
}

/**
 * Reimplements 0x414070: HudUiMessage::RebuildWeaponLayout.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Rebuilds the message base, text panel, and side widget geometry from the current layout anchor.
 */
void HudUiMessage::RebuildWeaponLayout() {
    HudUiWidget *const layoutWidget2 = &g_HudLayoutHW.widget2;
    const int anchorX = layoutWidget2->GetCenterX();
    const int anchorY = layoutWidget2->GetCenterY();

    const int clipLeft = panel.layoutX + (g_HudUiMgrHudOriginX / 2);
    zVidImagePartial *const baseImage = variantImages[0];
    HudUiRect widgetClipRect;
    widgetClipRect.left = clipLeft;
    widgetClipRect.top = panel.layoutY;
    widgetClipRect.right = clipLeft + baseImage->width;
    widgetClipRect.bottom = panel.layoutY + baseImage->height;

    SetPos(
        clipLeft + anchorX,
        panel.layoutY + anchorY
    );
    SetBltSourceAndClipRect(
        0,
        &widgetClipRect
    );

    HudUiRect panelClipRect;
    panelClipRect.left = clipLeft + 3;
    panelClipRect.top = widgetClipRect.bottom;
    panelClipRect.right = widgetClipRect.right - 2;
    panelClipRect.bottom = widgetClipRect.bottom + 12;

    const int textX =
        panelClipRect.left + ((panelClipRect.right - panelClipRect.left) / 2) + anchorX;
    panel.SetPos(
        textX,
        widgetClipRect.bottom + anchorY
    );
    panel.SetClip(
        0,
        &panelClipRect
    );

    zVidImagePartial *const sideImage = sideImageSwaps[0];
    widget.SetPos(
        anchorX - sideImage->width + widgetClipRect.right - 1,
        anchorY - sideImage->height + widgetClipRect.bottom - 1
    );
}

namespace HudUiLoadingCheckpoint {
/**
 * Reimplements 0x414180: HudUiLoadingCheckpoint::AdvanceAndLog.
 * Purpose: advance the embedded HudUiMgr loading checkpoint table, report
 * overflow, optionally log the supplied message, and update briefing progress.
 */
void __fastcall AdvanceAndLog(
    const char *messageOrNull
) {
    const unsigned int currentIndex = g_HudUiLoadingCheckpointCurrentIndex;
    const unsigned int maxIndex = g_HudUiLoadingCheckpointMaxIndex;
    if (currentIndex > maxIndex) {
        zError::ReportOld(
            0x800,
            "D:\\Proj\\Battlesport\\hud.cpp",
            0x1184,
            g_Hud_CheckpointOverflowMsg
        );
    } else {
        g_HudUiLoadingCheckpointCurrentProgress = g_HudUiLoadingCheckpointProgress[currentIndex];
        const unsigned int nextIndex = currentIndex + 1;
        g_HudUiLoadingCheckpointCurrentIndex = nextIndex;
        if (nextIndex > maxIndex) {
            g_HudUiLoadingCheckpointCurrentIndex = maxIndex;
        }
    }

    if (messageOrNull != 0) {
        puts(messageOrNull);
        fflush(stdout);
    }

    zGame::ReturnOnlyStub();
    Briefing::SetProgressAndSleep(g_HudUiLoadingCheckpointCurrentProgress);
}

/**
 * Reimplements 0x414210: HudUiLoadingCheckpoint::InitTable.
 * Purpose: seed the embedded HudUiMgr loading checkpoint table and derive
 * normalized briefing progress from the retail checkpoint second values.
 */
void InitTable() {
    static const float kRawProgress[] = {
        0.00100000005f,
        0.136999995f,
        0.237000003f,
        0.340000004f,
        0.899999976f,
        9.30000019f,
        12.3999996f,
        13.3999996f,
        20.0f,
        26.0f,
        26.2999992f,
        28.7000008f,
        31.5f,
        34.0f,
        36.2000008f,
        36.4000015f,
        53.2999992f,
        53.5999985f,
        53.7000008f,
    };

    g_HudUiLoadingCheckpointMaxIndex = 18;
    g_HudUiLoadingCheckpointCurrentIndex = 0;
    {
        for (unsigned int index = 0; index <= g_HudUiLoadingCheckpointMaxIndex; ++index) {
            g_HudUiLoadingCheckpointRawProgress[index] = kRawProgress[index];
            g_HudUiLoadingCheckpointProgress[index] =
                g_HudUiLoadingCheckpointRawProgress[index] * g_HudUiLoadingCheckpointProgressScale;
        }
    }
}
} // namespace HudUiLoadingCheckpoint

extern "C" char g_Hud_TripleStringFmt[9];

namespace HudUiListMenuEntry {
int __fastcall CompareSortKey(
    const HudUiScoreboardEntry *entryA,
    const HudUiScoreboardEntry *entryB
);
}

namespace {
const int kGameNetChatComposeTextCapacity = 0x20;
const int kGameNetChatComposeShiftModifierMask = 0x400;
const int kGameNetChatComposeDigitFirstDik = 0x02;
const int kGameNetChatComposeDigitLastDik = 0x0e;
const int kGameNetChatComposeLetterRowFirstDik = 0x10;
const int kGameNetChatComposeLetterRowLastDik = 0x2b;
const int kGameNetChatComposeHomeRowFirstDik = 0x1e;
const int kGameNetChatComposeHomeRowLastDik = 0x28;
const int kGameNetChatComposeBottomRowFirstDik = 0x2c;
const int kGameNetChatComposeBottomRowLastDik = 0x35;
const int kGameNetChatComposeSpaceDik = 0x39;

/**
 * Source: D:\Proj\Battlesport\hud.cpp
 * Original helper evidence: no standalone retail function; caller 0x4143d0
 * repeats this unregister/register pair across the chat-compose key ranges and
 * the standalone space-bar binding.
 * Purpose: Register one chat-compose keyboard callback binding.
 */
inline void HudRuntimeRegisterChatComposeKey(
    int comboIdx
) {
    zInput::Keyboard_UnregisterKeyCallback(comboIdx);
    zInput::Keyboard_RegisterKeyCallback(
        comboIdx,
        (void *)(&GameNet::ChatComposeKeyCallback),
        ""
    );
}

/**
 * Source: D:\Proj\Battlesport\hud.cpp
 * Original helper evidence: no standalone retail function; caller 0x4143d0
 * repeats contiguous chat-compose key registration for unmodified and modified
 * DIK ranges.
 * Purpose: Register a contiguous range of chat-compose keyboard bindings.
 */
inline void HudRuntimeRegisterChatComposeKeyRange(
    int firstComboIdx,
    int lastComboIdx
) {
    for (int comboIdx = firstComboIdx; comboIdx <= lastComboIdx; ++comboIdx) {
        HudRuntimeRegisterChatComposeKey(comboIdx);
        HudRuntimeRegisterChatComposeKey(comboIdx | kGameNetChatComposeShiftModifierMask);
    }
}

/**
 * Original helper evidence: no standalone retail function; observed callers
 * 0x414710, 0x414930, and 0x414980 in the hud.cpp list-menu layer.
 * Purpose: expose the recovered comparator as a boolean ordering predicate for
 * local sort helpers.
 */
inline bool HudRuntimeListMenuEntryComesBefore(
    const HudUiScoreboardEntry &lhs,
    const HudUiScoreboardEntry &rhs
) {
    return HudUiListMenuEntry::CompareSortKey(
        &lhs,
        &rhs
    ) != 0;
}

/**
 * Original helper evidence: no standalone retail function; observed caller
 * 0x414710 in the hud.cpp list-menu layer.
 * Purpose: select the median scoreboard entry among first, middle, and last
 * candidates for quicksort partitioning.
 */
inline HudUiScoreboardEntry *HudRuntimeListMenuMedianOfThree(
    HudUiScoreboardEntry *first,
    HudUiScoreboardEntry *middle,
    HudUiScoreboardEntry *last
) {
    if (HudRuntimeListMenuEntryComesBefore(
        *first,
        *middle
    )) {
        if (HudRuntimeListMenuEntryComesBefore(
            *middle,
            *last
        )) {
            return middle;
        }

        return HudRuntimeListMenuEntryComesBefore(
            *first,
            *last
        ) ? last : first;
    }

    if (HudRuntimeListMenuEntryComesBefore(
        *first,
        *last
    )) {
        return first;
    }

    return HudRuntimeListMenuEntryComesBefore(
        *middle,
        *last
    ) ? last : middle;
}
} // namespace

namespace HudUiMgrSensor {
/**
 * Reimplements 0x414300: HudUiMgrSensor::GetFxRect.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: return the recovered HUD value exposed by HudUiMgrSensor::GetFxRect.
 */
void __fastcall GetFxRect(
    HudUiRect *outRect
) {
    *outRect = g_HudUiMgrSensorFxRect;
}
} // namespace HudUiMgrSensor

namespace GameNet {
/**
 * Reimplements 0x414330: GameNet::ShowPlayerKillMessage
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Format and display a multiplayer kill-feed message.
 */
void __fastcall ShowPlayerKillMessage(
    GameNetPlayerRow *victimRow,
    OptCatalogEntryDef *killEntry,
    GameNetPlayerRow *killerRow
) {
    const char *killVerb = "";
    if (killEntry == 0) {
        killVerb = zLoc::GetMessageString(0x253);
    } else if (killEntry->killVerbString != 0) {
        killVerb = killEntry->killVerbString;
    }

    char message[0x50];
    sprintf(
        message,
        g_Hud_TripleStringFmt,
        victimRow->displayName,
        killVerb,
        killerRow->displayName
    );
    HudUi::ShowTopMessageLine(
        message,
        2.0f
    );
}

/**
 * Reimplements 0x414390: GameNet::RefreshPlayerListMenu
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Forward a player row to the HUD stats list triplet for scoreboard
 * entry insertion.
 */
void __fastcall RefreshPlayerListMenu(
    GameNetPlayerRow *playerRow
) {
    g_HudUiMgrStatsList->triplet->AddEntry(playerRow);
}
} // namespace GameNet

namespace HudUiMgr {
/**
 * Reimplements 0x4143a0: HudUiMgr::IsLocalPlayerFirstInStatsList.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::IsLocalPlayerFirstInStatsList.
 */
int IsLocalPlayerFirstInStatsList() {
    return g_HudUiMgrStatsList->triplet->IsLocalPlayerFirstEntry();
}
} // namespace HudUiMgr

namespace HudUi {
/**
 * Reimplements 0x4143b0: HudUi::RefreshScoreboardEntryRow.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUi::RefreshScoreboardEntryRow.
 */
void __fastcall RefreshScoreboardEntryRow(
    GameNetPlayerRow *entryData
) {
    g_HudUiMgrStatsList->triplet->UpdateEntryData(entryData);
}

/**
 * Reimplements 0x4143c0: HudUi::RemoveScoreboardEntryRow.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: forward a multiplayer row removal to the active scoreboard triplet.
 */
void __fastcall RemoveScoreboardEntryRow(
    GameNetPlayerRow *entryKey
) {
    g_HudUiMgrStatsList->triplet->RemoveEntry(entryKey);
}
} // namespace HudUi

namespace GameNet {
/**
 * Reimplements 0x4143d0: GameNet::BeginChatCompose
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Open chat-compose mode and bind text-entry keys.
 */
void BeginChatCompose() {
    if (zOpt::GetNetworkEnabled() == 0) {
        return;
    }

    HudUiMgrObjective::Show(
        0,
        g_HudUiMessage_NodeName,
        "",
        0.0f
    );
    g_HudUiMgrObjectiveChatComposeActive = 1;
    g_HudUiMgrObjectiveChatComposeTextInput.AllocTextBuffer(kGameNetChatComposeTextCapacity);
    g_HudUiMgrObjectiveChatComposeTextInput.SetContents("");
    zInput::BindMapContext_Push(0);

    HudRuntimeRegisterChatComposeKeyRange(
        kGameNetChatComposeDigitFirstDik,
        kGameNetChatComposeDigitLastDik
    );
    HudRuntimeRegisterChatComposeKeyRange(
        kGameNetChatComposeLetterRowFirstDik,
        kGameNetChatComposeLetterRowLastDik
    );
    HudRuntimeRegisterChatComposeKeyRange(
        kGameNetChatComposeHomeRowFirstDik,
        kGameNetChatComposeHomeRowLastDik
    );
    HudRuntimeRegisterChatComposeKeyRange(
        kGameNetChatComposeBottomRowFirstDik,
        kGameNetChatComposeBottomRowLastDik
    );
    HudRuntimeRegisterChatComposeKey(kGameNetChatComposeSpaceDik);
}

/**
 * Reimplements 0x414550: GameNet::ChatComposeKeyCallback
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Append a translated key to active chat-compose text and mirror the
 * buffer into the objective description panel.
 */
void __fastcall ChatComposeKeyCallback(
    int dikCodeWithMods
) {
    const int key = zInput::Keyboard_TranslateDikToAscii(dikCodeWithMods);
    if (key == 0) {
        return;
    }

    g_HudUiMgrObjectiveChatComposeTextInput.DispatchKeyAction(key);

    g_HudUiMgrObjectiveDescTextPanel->SetTextFmt(
        g_HudUiMgrObjectiveChatComposeTextInput.GetBuffer()
    );
}

/**
 * Reimplements 0x414590: GameNet::EndChatComposeAndSend
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Close chat compose, show the local chat line, and send packet 0x0b.
 */
void EndChatComposeAndSend() {
    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    GameNetPlayerRow *const playerRow = saveState->netPlayerRow;
    char chatLine[0x51];
    chatLine[0x50] = '\0';

    g_HudUiMgrObjectiveChatComposeActive = 0;
    zInput::BindMapContext_Pop();
    HudUiMgrObjective::Begin();

    if (strlen(g_HudUiMgrObjectiveChatComposeTextInput.GetBuffer()) == 0) {
        return;
    }

    strncpy(
        chatLine,
        playerRow->displayName,
        0x50
    );
    strncat(
        chatLine,
        g_HudUiMessage_SeparatorColon,
        0x50 - strlen(chatLine)
    );
    strncat(
        chatLine,
        g_HudUiMgrObjectiveChatComposeTextInput.GetBuffer(),
        0x50 - strlen(chatLine)
    );
    HudUi::ShowChatLine(
        chatLine,
        5.0f
    );
    SendPkt0B_ChatMessage(chatLine);
}

/**
 * Reimplements 0x414660: GameNet::EndChatComposeAndSendThunk
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Forward the chat-compose dispatch callback to EndChatComposeAndSend.
 */
void EndChatComposeAndSendThunk() {
    EndChatComposeAndSend();
}
} // namespace GameNet

/**
 * Reimplements 0x414670: HudUiTripletEntries::GetCount.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: return the number of populated entries in the recovered scoreboard vector.
 */
int HudUiTripletEntries::GetCount() {
    if (begin == 0) {
        return 0;
    }

    return (int)(end - begin);
}

/**
 * Reimplements 0x4146a0: HudUiTripletEntries::CopyRange.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: copy a range of scoreboard entries into destination vector storage.
 */
HudUiScoreboardEntry *__stdcall HudUiTripletEntries::CopyRange(
    HudUiScoreboardEntry *sourceBegin,
    HudUiScoreboardEntry *sourceEnd,
    HudUiScoreboardEntry *dest
) {
    HudUiScoreboardEntry *cursor = dest;
    while (sourceBegin != sourceEnd) {
        if (cursor != 0) {
            *cursor = *sourceBegin;
        }
        ++sourceBegin;
        ++cursor;
    }

    return cursor;
}

/**
 * Reimplements 0x4146e0: HudUiTripletEntries::FillN.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: fill consecutive scoreboard vector slots from one source entry.
 */
void __stdcall HudUiTripletEntries::FillN(
    HudUiScoreboardEntry *dest,
    unsigned int count,
    const HudUiScoreboardEntry *sourceValue
) {
    HudUiScoreboardEntry *cursor = dest;
    while (count != 0) {
        if (cursor != 0) {
            *cursor = *sourceValue;
        }
        ++cursor;
        --count;
    }
}

namespace HudUiListMenuEntry {
/**
 * Reimplements 0x414710: HudUiListMenuEntry::SortRange.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: partition larger scoreboard-entry ranges before the final insertion-sort pass.
 */
void __fastcall SortRange(
    HudUiScoreboardEntry *begin,
    HudUiScoreboardEntry *end,
    int unusedFlags
) {
    while (end - begin > 16) {
        HudUiScoreboardEntry *left = begin;
        HudUiScoreboardEntry *right = end - 1;
        HudUiScoreboardEntry *const middle = begin + ((end - begin) / 2);
        HudUiScoreboardEntry pivot = *HudRuntimeListMenuMedianOfThree(
            begin,
            middle,
            right
        );

        for (;;) {
            while (HudRuntimeListMenuEntryComesBefore(
                *left,
                pivot
            )) {
                ++left;
            }

            while (HudRuntimeListMenuEntryComesBefore(
                pivot,
                *right
            )) {
                --right;
            }

            if (right <= left) {
                break;
            }

            HudUiScoreboardEntry temp = *left;
            *left = *right;
            *right = temp;
            ++left;
        }

        if (end - left <= left - begin) {
            SortRange(
                left,
                end,
                unusedFlags
            );
            end = left;
        } else {
            SortRange(
                begin,
                left,
                unusedFlags
            );
            begin = left;
        }
    }
}

/**
 * Reimplements 0x414930: HudUiListMenuEntry::InsertPivotIntoSortedPrefix.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: shift a sorted prefix forward and store the pivot entry at its sorted position.
 */
void InsertPivotIntoSortedPrefix(
    HudUiScoreboardEntry *slot,
    const HudUiScoreboardEntry &pivot
) {
    HudUiScoreboardEntry *insertSlot = slot;
    HudUiScoreboardEntry *previousEntry = insertSlot - 1;
    while (HudRuntimeListMenuEntryComesBefore(
        pivot,
        *previousEntry
    )) {
        *insertSlot = *previousEntry;
        insertSlot = previousEntry;
        --previousEntry;
    }

    *insertSlot = pivot;
}

/**
 * Reimplements 0x414980: HudUiListMenuEntry::InsertionSortRange.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: insertion-sort a scoreboard-entry range in place using the recovered list-menu ordering.
 */
void __fastcall InsertionSortRange(
    HudUiScoreboardEntry *begin,
    HudUiScoreboardEntry *end,
    int
) {
    if (begin == end) {
        return;
    }

    HudUiScoreboardEntry *current = begin + 1;
    while (current != end) {
        HudUiScoreboardEntry candidate = *current;
        if (HudRuntimeListMenuEntryComesBefore(
            candidate,
            *begin
        )) {
            HudUiScoreboardEntry *shiftCursor = current;
            while (shiftCursor != begin) {
                *shiftCursor = *(shiftCursor - 1);
                --shiftCursor;
            }

            *begin = candidate;
        } else {
            InsertPivotIntoSortedPrefix(
                current,
                candidate
            );
        }

        ++current;
    }
}
} // namespace HudUiListMenuEntry

namespace HudUiMgrSensor {
/**
 * Reimplements 0x410d10: HudUiMgrSensor::SetViewportRect.
 * Original source path: D:\Proj\GameZRecoil\zhud_ui.cpp.
 * Purpose: store raw/scaled HUD sensor viewport bounds and update the active source rectangle.
 */
void __fastcall SetViewportRect(
    int x,
    int y,
    int width,
    int height
) {
    const int right = x + width;
    const int bottom = y + height;

    g_HudUiMgrSensorBlock.sensorRectRaw.left = x;
    g_HudUiMgrSensorBlock.sensorRectRaw.right = right;
    g_HudUiMgrSensorBlock.sensorRectRaw.top = y;
    g_HudUiMgrSensorBlock.sensorRectRaw.bottom = bottom;

    if (zOpt::GetReplicateMode() == 0) {
        g_HudUiMgrSensorBlock.sensorRectScaled = g_HudUiMgrSensorBlock.sensorRectRaw;
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.left = (float)(x);
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.top = (float)(y);
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.right = (float)(right);
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.bottom = (float)(bottom);
    } else {
        const int halfX = x / 2;
        const int halfY = y / 2;
        const int halfWidth = width / 2;
        const int halfHeight = height / 2;

        g_HudUiMgrSensorBlock.sensorPiVSrcRect.left = (float)(halfX);
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.top = (float)(halfY);
        g_HudUiMgrSensorBlock.sensorRectScaled.left = halfX;
        g_HudUiMgrSensorBlock.sensorRectScaled.top = halfY;
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.right =
            (float)(halfWidth) + g_HudUiMgrSensorBlock.sensorPiVSrcRect.left;
        g_HudUiMgrSensorBlock.sensorRectScaled.right = halfX + halfWidth;
        g_HudUiMgrSensorBlock.sensorRectScaled.bottom = halfY + halfHeight;
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.bottom =
            (float)(halfHeight) + g_HudUiMgrSensorBlock.sensorPiVSrcRect.top;
    }

    g_HudUiMgrSensorBlock.sensorClampHalfW = (g_HudUiMgrSensorBlock.sensorPiVSrcRect.right -
                                                 g_HudUiMgrSensorBlock.sensorPiVSrcRect.left) /
                                             g_HudUiMgrSensorBlock.sensorParam;
    g_HudUiMgrSensorBlock.sensorClampHalfH = (g_HudUiMgrSensorBlock.sensorPiVSrcRect.bottom -
                                                 g_HudUiMgrSensorBlock.sensorPiVSrcRect.top) /
                                             g_HudUiMgrSensorBlock.sensorParam;
    zClipAlt::SetSourceRect(&g_HudUiMgrSensorBlock.sensorPiVSrcRect);
}

} // namespace HudUiMgrSensor

namespace HudUiMgr {
/**
 * Reimplements 0x410e90: HudUiMgr::EnableHud.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::EnableHud.
 */
int EnableHud() {
    const int previouslyEnabled = g_HudUiMgr.enabled;
    g_HudUiMgr.SetEnabled(1);

    g_HudUiMgrCurrentLayout->Enable();

    HudUiMgrObjective::Update();
    zClipAlt::SetSourceRect(&g_HudUiMgrSensorBlock.sensorPiVSrcRect);
    gAltClipPassEnabled = 1;
    return previouslyEnabled;
}

/**
 * Reimplements 0x410ed0: HudUiMgr::DisableHud.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::DisableHud.
 */
int DisableHud() {
    const int previouslyEnabled = g_HudUiMgr.enabled;
    DestroySensorWindow();

    {
        int slotIndex;
        for (slotIndex = 0;
            slotIndex < (int)(sizeof(g_HudUiMgrWeaponSlots) / sizeof(g_HudUiMgrWeaponSlots[0]));
            ++slotIndex) {
            HudUiSlot &slot = g_HudUiMgrWeaponSlots[slotIndex];
            slot.trackMarkerWidget.SetVisible(0);
            slot.slotWidget.SetVisible(0);
        }
    }

    g_HudUiMgrSensorTargetMarkerCount = 0;
    g_HudUiMgrWeaponState = 0;
    g_HudUiMgr.SetEnabled(0);

    g_HudUiMgrCurrentLayout->Disable();

    g_HudUiMgrObjectiveWidget.SetVisible(0);
    g_HudUiMgrObjectiveDescTextPanel->SetVisible(0);
    g_HudUiMgrObjectiveBar.SetVisible(0);
    g_HudUiMgrObjectiveSensorRect.SetVisible(0);
    g_HudUiMgrObjectiveSummaryTextPanel->SetVisible(0);
    g_HudUiMgrObjectiveLabelTextPanel->SetVisible(0);
    g_HudUiMgrObjectiveMeter.SetVisible(0);

    gAltClipPassEnabled = 0;
    if (zVid::GetAccelerationOption() == 0) {
        zRndr::SpanOcclusionResetFrame();
    }

    const int hudType = zOpt::GetHudTypeForCurrentHwMode();
    if (hudType == 2) {
        g_HudUiMgrLayoutDelayFrames = hudType;
    }

    g_HudUiMgrTimerPanel->SetVisible(1);
    return previouslyEnabled;
}

/**
 * Reimplements 0x410fe0: HudUiMgr::UpdateFrame.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: run the per-frame HudUiMgr update sequence for the active layout,
 * HUD containers, timers, reticle widget, and transient weapon slot state.
 */
void UpdateFrame() {
    g_HudUiMgrCurrentLayout->LayoutPreUpdate();

    if (g_HudUiMgr.enabled != 0) {
        if (g_HudUiMgrObjectiveState != 0) {
            HudUiMgrObjective::StartHide();
        }
    } else {
        if (g_HudUiMgrObjectiveChatComposeActive != 0) {
            g_HudUiMgrObjectiveSummaryTextPanel->Draw();
            g_HudUiMgrObjectiveDescTextPanel->Draw();
        }

        g_HudUiMgrTimerPanel->Update(
            g_Time_UnscaledDeltaTimeSec
        );
    }

    if (g_HudUiMgrObjectiveMeterFillAnimEnabled != 0) {
        HudUiMgrObjective::TickMeterFillAnimation();
    }

    g_HudSensorTracker.Update();
    zTimedTask::TickActiveList();

    g_HudUiMgrCurrentLayout->UpdateAll(g_Time_UnscaledDeltaTimeSec);
    g_HudUiMgr.UpdateAll(g_Time_UnscaledDeltaTimeSec);
    g_HudUiTopMessageStack->UpdateAll(g_Time_UnscaledDeltaTimeSec);
    g_HudUiChatMessageStack->UpdateAll(g_Time_UnscaledDeltaTimeSec);
    g_HudUiMgrStringMenu->UpdateAll(g_Time_UnscaledDeltaTimeSec);

    const float sampleElapsedSec =
        g_HudUiMgrTimerPanelFloat->sampleElapsedSec + g_FrameDeltaTimeSec;
    g_HudUiMgrTimerPanelFloat->sampleElapsedSec = sampleElapsedSec;

    const float sampleFrameCount =
        g_HudUiMgrTimerPanelFloat->sampleFrameCount + 1.0f;
    g_HudUiMgrTimerPanelFloat->sampleFrameCount = sampleFrameCount;
    if (sampleElapsedSec >= 1.0f) {
        g_HudUiMgrTimerPanelFloat->sampleFrameCount = 0.0f;
        g_HudUiMgrTimerPanelFloat->sampleElapsedSec = 0.0f;
        g_HudUiMgrTimerPanelFloat->displayValue =
            sampleFrameCount / sampleElapsedSec;
    }

    HudUiElement *const floatingTimerElement = (HudUiElement *)(g_HudUiMgrTimerPanelFloat);
    if ((floatingTimerElement->flags & 0x10) == 0) {
        g_HudUiMgrTimerPanelFloat->Draw();
    }

    g_HudUiMgrReticleWidget.Update(
        g_Time_UnscaledDeltaTimeSec
    );

    {
        for (int slotIndex = 0; slotIndex < 32; ++slotIndex) {
            HudUiSlot &slot = g_HudUiMgrWeaponSlots[slotIndex];
            slot.trackMarkerWidget.SetVisible(0);
            slot.slotWidget.SetVisible(0);
        }
    }

    g_HudUiMgrSensorTargetMarkerCount = 0;
    g_HudUiMgrWeaponState = 0;
}

/**
 * Reimplements 0x411170: HudUiMgr::ProjectPointToNormalizedClamped.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::ProjectPointToNormalizedClamped.
 */
int __fastcall ProjectPointToNormalizedClamped(
    const zVec3 *srcPoint,
    zVec3 *projectedPoint
) {
    if (zMath::ProjectPointAndClampToScreenClip(
        srcPoint,
        projectedPoint
    ) == 0x10) {
        return 1;
    }

    const float halfHudWidth = g_HudUiMgrHudRectW * 0.5f;
    const float halfHudHeight = g_HudUiMgrHudRectH * 0.5f;
    if (zOpt::GetReplicateMode() != 0) {
        projectedPoint->x += projectedPoint->x;
        projectedPoint->y += projectedPoint->y;
    }

    projectedPoint->x = (projectedPoint->x - halfHudWidth) / halfHudWidth;
    projectedPoint->y =
        (projectedPoint->y - (float)(g_HudUiMgrHudRect.top) - halfHudHeight) / halfHudHeight;

    if (projectedPoint->x > 1.0f) {
        projectedPoint->x = 1.0f;
    } else if (projectedPoint->x < -1.0f) {
        projectedPoint->x = -1.0f;
    }

    if (projectedPoint->y > 1.0f) {
        projectedPoint->y = 1.0f;
    } else if (projectedPoint->y < -1.0f) {
        projectedPoint->y = -1.0f;
    }

    return 0;
}

/**
 * Reimplements 0x411270: HudUiMgr::UpdateTargetReticleFromCursor.
 * Purpose: advance the recovered HUD update path for HudUiMgr::UpdateTargetReticleFromCursor.
 */
int __fastcall UpdateTargetReticleFromCursor(
    int reticleMode,
    zVec3 *worldHitPoint,
    float normalizedX,
    float normalizedY
) {
    HudUiElement *const reticleElement = (HudUiElement *)(&g_HudUiMgrReticleWidget);

    if (reticleMode == 0) {
        reticleElement->SetVisible(0);
        return 0;
    }

    if (reticleMode == 1) {
        reticleElement->SetVisible(1);
        return 0;
    }

    if (reticleMode != 2) {
        return 0;
    }

    float screenX =
        (normalizedX + 1.0f) * g_HudUiMgrReticleMapScaleHalfW + g_HudUiMgrReticleMapBiasX;
    float screenY =
        (normalizedY + 1.0f) * g_HudUiMgrReticleMapScaleHalfH + g_HudUiMgrReticleMapBiasY;

    const int projectedX = (int)(screenX);
    const int projectedY = (int)(screenY);
    g_HudUiMgrReticleProjectedX = projectedX;
    g_HudUiMgrReticleProjectedY = projectedY;

    reticleElement->SetPos(
        projectedX - g_HudUiMgrReticleWidgetHalfW,
        projectedY - g_HudUiMgrReticleWidgetHalfH
    );

    if ((g_HudLayoutHW.reticleClipInitFlags & 1) == 0) {
        g_HudLayoutHW.reticleClipInitFlags =
            (unsigned char)(g_HudLayoutHW.reticleClipInitFlags | 1);
        atexit(&HudUiMgr::ReticleStaticAtexitStub);
    }

    RECT reticleBounds = {0};
    reticleBounds.top = g_HudUiMgrReticleWidget.GetCenterY();
    reticleBounds.bottom =
        g_HudUiMgrReticleWidget.GetCenterY() +
        (g_HudUiMgrReticleWidget.image != 0 ? g_HudUiMgrReticleWidget.image->height : 0);
    reticleBounds.left = g_HudUiMgrReticleWidget.GetCenterX();
    reticleBounds.right =
        g_HudUiMgrReticleWidget.GetCenterX() +
        (g_HudUiMgrReticleWidget.image != 0 ? g_HudUiMgrReticleWidget.image->width : 0);

    if (IntersectRect(
            (RECT *)(&g_HudLayoutHW.reticleClipRect),
            &reticleBounds,
            (const RECT *)(zOpt::GetDisplaySection())
        ) != 0) {
        g_HudLayoutHW.reticleClipRect.top -= g_HudUiMgrReticleWidget.GetCenterY();
        g_HudLayoutHW.reticleClipRect.bottom -= g_HudUiMgrReticleWidget.GetCenterY();
        g_HudLayoutHW.reticleClipRect.left -= g_HudUiMgrReticleWidget.GetCenterX();
        g_HudLayoutHW.reticleClipRect.right -= g_HudUiMgrReticleWidget.GetCenterX();

        reticleElement->SetPos(
            g_HudUiMgrReticleWidget.GetCenterX() + g_HudLayoutHW.reticleClipRect.left,
            g_HudUiMgrReticleWidget.GetCenterY() + g_HudLayoutHW.reticleClipRect.top
        );
        g_HudUiMgrReticleWidget.bltClipRectOrNull = &g_HudLayoutHW.reticleClipRect;
    }

    zProjectedPoint projectedPoint = {screenX, screenY, 0.0f};
    ScreenToWorld(&projectedPoint.x);

    HudReticlePlayerStatePartial *const playerState =
        (HudReticlePlayerStatePartial *)(g_GameStateOrMapTable->playerState);

    float nearClip = 0.0f;
    float farClip = 0.0f;
    zClass_Camera::gwCameraGetNearFarClip(
        g_MainCamera,
        &nearClip,
        &farClip
    );

    zVec3 nearPoint = {0};
    projectedPoint.reciprocalZ = 1.0f / nearClip;
    zMath_UnprojectPointBatchZBuf(
        &projectedPoint,
        &nearPoint,
        1
    );

    zVec3 farPoint = {0};
    projectedPoint.reciprocalZ = 1.0f / playerState->activeAltGunController->optCatalogEntry->range;
    zMath_UnprojectPointBatchZBuf(
        &projectedPoint,
        &farPoint,
        1
    );

    zClass_Class::gwNodeSetRaycastable(
        playerState->rootNode,
        0
    );
    if (playerState->cameraState == 7) {
        zClass_Class::gwNodeSetRaycastable(
            playerState->activeAltGunController->attachState->projectileNode,
            0
        );
    }

    zClass_cls_di::SetStopAfterFirstHit(0x40000);
    PlayerProbeSampleCandidateBuffer rayData = {0};
    const int raycastResult = zClass_cls_di::RaycastSelectClosestHitBetweenPoints(
        g_Player_RuntimeDiScene,
        &nearPoint,
        &farPoint,
        &rayData
    );

    zClass_Class::gwNodeSetRaycastable(
        playerState->rootNode,
        0
    );
    if (playerState->cameraState == 7) {
        zClass_Class::gwNodeSetRaycastable(
            playerState->activeAltGunController->attachState->projectileNode,
            1
        );
    }

    zVidImagePartial *reticleImage = 0;
    if (raycastResult != 0) {
        g_HudUiMgrReticleProjection[0] = farPoint.x;
        g_HudUiMgrReticleProjection[1] = farPoint.y;
        g_HudUiMgrReticleProjection[2] = farPoint.z;
        reticleImage = g_HudUiMgrReticleImages[1];
    } else {
        const zClassDiPickCandidateEntry &candidate = rayData.entries[rayData.candidateCount];
        g_HudUiMgrReticleProjection[0] = candidate.hitPos.x;
        g_HudUiMgrReticleProjection[1] = candidate.hitPos.y;
        g_HudUiMgrReticleProjection[2] = candidate.hitPos.z;

        zClass_NodeFreeListSlot *const hitSlot = (zClass_NodeFreeListSlot *)(candidate.node);
        reticleImage =
            hitSlot->damageHandler != 0 ? g_HudUiMgrReticleImages[2] : g_HudUiMgrReticleImages[0];
    }

    g_HudUiMgrReticleWidget.SetImageBorrowedAndInvalidate(reticleImage);

    worldHitPoint->x = g_HudUiMgrReticleProjection[0];
    worldHitPoint->y = g_HudUiMgrReticleProjection[1];
    worldHitPoint->z = g_HudUiMgrReticleProjection[2];

    zOpt_ViewRectSection *const renderRect = zOpt::GetRenderSection();
    const float minX = (float)(renderRect->x) + g_HudUiMgrSensorBlock.sensorClampHalfW;
    if (!(screenX >= minX)) {
        screenX = minX;
    } else {
        const float maxX =
            (float)(renderRect->rightExclusive) - g_HudUiMgrSensorBlock.sensorClampHalfW;
        if (screenX > maxX) {
            screenX = maxX;
        }
    }

    const float minY = (float)(renderRect->y) + g_HudUiMgrSensorBlock.sensorClampHalfH;
    if (!(screenY >= minY)) {
        screenY = minY;
    } else {
        const float maxY =
            (float)(renderRect->bottomExclusive) - g_HudUiMgrSensorBlock.sensorClampHalfH;
        if (screenY > maxY) {
            screenY = maxY;
        }
    }

    zClipAltFloatRect targetRect = {screenX - g_HudUiMgrSensorBlock.sensorClampHalfW,
        screenY - g_HudUiMgrSensorBlock.sensorClampHalfH,
        screenX + g_HudUiMgrSensorBlock.sensorClampHalfW,
        screenY + g_HudUiMgrSensorBlock.sensorClampHalfH};
    zClipAlt::SetTargetRect(
        &targetRect,
        zOpt::GetReplicateMode()
    );
    return 0;
}

/**
 * Reimplements 0x411710: HudUiMgr::ReticleStaticAtexitStub.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::ReticleStaticAtexitStub.
 */
void ReticleStaticAtexitStub() {}

/**
 * Reimplements 0x411720: HudUiMgr::CopyReticleProjection.
 * Purpose: copy the HudUiMgr reticle projection vector into the caller-owned
 * three-float output buffer.
 */
void __fastcall CopyReticleProjection(
    float *outProjection
) {
    unsigned int *const outBits = (unsigned int *)(outProjection);
    const unsigned int *const projectionBits = (const unsigned int *)(g_HudUiMgrReticleProjection);
    outBits[0] = projectionBits[0];
    outBits[1] = projectionBits[1];
    outBits[2] = projectionBits[2];
}

/**
 * Reimplements 0x411740: HudUiMgr::SetReticleMode.
 * Purpose: store the active HUD reticle mode.
 */
void __fastcall SetReticleMode(
    int mode
) {
    g_HudUiMgrReticleMode = mode;
}

/**
 * Reimplements 0x411750: HudUiMgr::SetNanitePanelCount.
 * Purpose: apply the recovered HUD state change handled by HudUiMgr::SetNanitePanelCount.
 */
void __fastcall SetNanitePanelCount(
    int count
) {
    g_HudUiMgrNanitePanel.SetVisibleCount(count);
}

} // namespace HudUiMgr

namespace HudUiMgrObjective {
/**
 * Recovered original helper with no standalone retail function. Observed in
 * caller 0x411ac0: HudUiMgrObjective::StartHide.
 * Evidence basis: repeated objective phase runtime update of the widget right
 * edge after slide-position changes.
 * Purpose: refresh the cached objective widget right edge from its current
 * center position and borrowed image width.
 */
static void HudUiMgrObjective_UpdateWidgetRightX() {
    const zVidImagePartial *const image = g_HudUiMgrObjectiveWidget.image;
    const int width = image != 0 ? image->width : 0;
    g_HudUiMgrObjectiveWidgetRightX = g_HudUiMgrObjectiveWidget.GetCenterX() + width;
}

/**
 * Recovered original helper with no standalone retail function. Observed in
 * caller 0x411ac0: HudUiMgrObjective::StartHide.
 * Evidence basis: repeated phase animation sequence updates the objective bar
 * slide edge, invalidates the bar, moves the widget, and recomputes meter X
 * points as one source-level operation.
 * Purpose: apply the objective panel slide X position and dependent meter
 * geometry.
 */
static void HudUiMgrObjective_SetSlidePosition(
    float slideX
) {
    g_HudUiMgrObjectiveBar.points[2].x = slideX;
    g_HudUiMgrObjectiveBar.points[3].x = slideX;
    g_HudUiMgrObjectiveBar.Invalidate();
    ((HudUiElement *)(&g_HudUiMgrObjectiveWidget))->SetX((int)(slideX)-1);
    HudUiMgrObjective::UpdateMeterXPoints();
}

/**
 * Recovered original helper with no standalone retail function. Observed in
 * caller 0x411ac0: HudUiMgrObjective::StartHide.
 * Evidence basis: phase-3 animation branches share the same hardware-HUD dirty
 * rectangle gate through zOpt::GetHudTypeForCurrentHwMode.
 * Purpose: update the hardware HUD objective dirty rectangle only for the
 * hardware perspective HUD mode.
 */
static void HudUiMgrObjective_UpdateHwDirtyRectIfNeeded() {
    if (zOpt::GetHudTypeForCurrentHwMode() == 2) {
        g_HudLayoutHW.UpdateObjectiveDirtyRect();
    }
}

/**
 * Recovered original helper with no standalone retail function. Observed in
 * caller 0x411ac0: HudUiMgrObjective::StartHide.
 * Evidence basis: phase-1 and phase-3 animation branches share the sensor
 * image null guard, mirrored fade-to-noise calculation, visibility update, and
 * zVid::DrawNoiseRect call sequence.
 * Purpose: draw objective sensor transition noise while optionally revealing or
 * hiding the sensor rectangle when the fade passes the midpoint.
 */
static void HudUiMgrObjective_DrawSensorNoise(
    float fade,
    int visibleWhenCovered
) {
    if (g_HudUiMgrObjectiveSensorRect.image == 0) {
        return;
    }

    float noise = fade + fade;
    if (noise < 1.0f) {
        zVid::DrawNoiseRect(
            (zVidRect32 *)(&g_HudUiMgrSensorBlock.sensorRectRaw),
            (double)(noise)
        );
        return;
    }

    g_HudUiMgrObjectiveSensorRect.SetVisible(visibleWhenCovered);
    zVid::DrawNoiseRect(
        (zVidRect32 *)(&g_HudUiMgrSensorBlock.sensorRectRaw),
        (double)(2.0f - noise)
    );
}

/**
 * Reimplements 0x411760: HudUiMgrObjective::SetVisibleAndResetMeterFill.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: toggle the objective label and meter visibility, and restart the
 * objective meter fill animation from the meter bottom when showing.
 */
void __fastcall SetVisibleAndResetMeterFill(
    int visible
) {
    if (visible == 0) {
        g_HudUiMgrObjectiveLabelTextPanel->SetVisible(0);
        g_HudUiMgrObjectiveMeter.SetVisible(0);
        return;
    }

    g_HudUiMgrObjectiveLabelTextPanel->SetVisible(1);
    g_HudUiMgrObjectiveMeter.SetVisible(1);

    const int meterTop = (int)(g_HudUiMgrObjectiveMeter.points[1].y);
    g_HudUiMgrObjectiveMeterFillAnimTimerSec = 0.0f;
    g_HudUiMgrObjectiveMeterFillAnimEnabled = 1;
    g_HudUiMgrObjectiveMeter.points[0].y = (float)(meterTop);
    g_HudUiMgrObjectiveMeter.points[3].y = (float)(meterTop);
}

/**
 * Reimplements 0x4117f0: HudUiMgrObjective::TickMeterFillAnimation.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: advance the objective meter fill timer, update the animated top
 * edge, and stop the animation once the meter reaches full height.
 */
void TickMeterFillAnimation() {
    g_HudUiMgrObjectiveMeterFillAnimTimerSec += g_Time_UnscaledDeltaTimeSec;

    int fillPixels;
    if (g_HudUiMgrObjectiveMeterFillAnimTimerSec >= 3.0f) {
        fillPixels = (int)(ceil((double)(g_HudUiMgrObjectiveMeter.fillPixelsMax)));
        g_HudUiMgrObjectiveMeterFillAnimEnabled = 0;
    } else {
        const double fillRatio = (double)(g_HudUiMgrObjectiveMeterFillAnimTimerSec * 0.333332986f) *
                                 (double)(g_HudUiMgrObjectiveMeter.fillPixelsMax);
        fillPixels = (int)(ceil(fillRatio));
    }

    const int top = (int)(g_HudUiMgrObjectiveMeter.points[1].y) - fillPixels;
    g_HudUiMgrObjectiveMeter.points[0].y = (float)(top);
    g_HudUiMgrObjectiveMeter.points[3].y = (float)(top);
}

/**
 * Reimplements 0x4118b0: HudUiMgrObjective::UpdateMeterXPoints.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: recompute the objective meter X edges from the objective widget
 * center position.
 */
void UpdateMeterXPoints() {
    const float left = (float)(g_HudUiMgrObjectiveWidget.GetCenterX()) + 5.0f;
    const float right = left + 7.0f;
    g_HudUiMgrObjectiveMeter.points[0].x = left;
    g_HudUiMgrObjectiveMeter.points[1].x = left;
    g_HudUiMgrObjectiveMeter.points[2].x = right;
    g_HudUiMgrObjectiveMeter.points[3].x = right;
}

/**
 * Reimplements 0x411900: HudUiMgrObjective::Show.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Start or update the objective HUD panel with summary text, description text, and image state.
 */
int __fastcall Show(
    zVidImagePartial *objectiveImage,
    const char *summaryFormat,
    const char *descText,
    float autoHideDelay
) {
    if (summaryFormat == 0 || descText == 0 || g_HudUiMgrObjectiveChatComposeActive != 0) {
        return 0;
    }

    g_HudUiMgrObjectiveSummaryTextPanel->SetTextFmt(summaryFormat);
    g_HudUiMgrObjectiveDescTextPanel->SetTextFmt(descText);
    g_HudUiMgrSensorOverlay.SetVisible(0);

    const int phase = g_HudUiMgrObjectivePhase;
    if (phase == 0) {
        g_HudUiMgrObjectiveSensorRect.SetImageBorrowedAndInvalidate(objectiveImage);
        zVidImagePartial *const widgetImage = g_HudUiMgrObjectiveWidget.image;
        g_HudUiMgrObjectiveState = 1;
        g_HudUiMgrObjectivePhase = 1;
        g_HudUiMgrObjectivePhaseTimerSec = 0.0f;
        g_HudUiMgrObjectiveShowResetUnused = 0;
        g_HudUiMgrObjectiveAutoHideDelaySec = autoHideDelay;

        const int imageWidth = widgetImage != 0 ? widgetImage->width : 0;
        g_HudUiMgrObjectiveWidgetRightX = g_HudUiMgrObjectiveWidget.GetCenterX() + imageWidth;
        g_HudUiMgrObjectiveBar.SetVisible(1);
        gAltClipPassEnabled = 0;
        return 1;
    }

    if (phase == 3) {
        g_HudUiMgrObjectivePhase = 1;
        g_HudUiMgrObjectivePhaseTimerSec =
            g_HudUiMgrObjectivePhaseDurationSec - g_HudUiMgrObjectivePhaseTimerSec;
        return 1;
    }

    g_HudUiMgrObjectiveSensorRect.SetImageBorrowedAndInvalidate(objectiveImage);
    return 0;
}

/**
 * Reimplements 0x411a20: HudUiMgrObjective::Begin.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Transition the objective panel into its begin/close phase while respecting chat-compose input.
 */
void Begin() {
    if (g_HudUiMgrObjectiveChatComposeActive != 0) {
        return;
    }

    const int phase = g_HudUiMgrObjectivePhase;
    if (phase == 2) {
        g_HudUiMgrObjectiveState = 1;
        g_HudUiMgrObjectivePhase = 3;
        g_HudUiMgrObjectivePhaseTimerSec = 0.0f;

        g_HudUiMgrObjectiveSummaryTextPanel->SetVisible(0);
        g_HudUiMgrObjectiveDescTextPanel->SetVisible(0);
        g_HudUiMgrObjectiveSensorRect.SetVisible(0);
        g_HudUiMgrObjectiveAutoHideDelaySec = 0.0f;
        return;
    }

    if (phase == 1) {
        g_HudUiMgrObjectivePhase = 3;
        g_HudUiMgrObjectivePhaseTimerSec =
            g_HudUiMgrObjectivePhaseDurationSec - g_HudUiMgrObjectivePhaseTimerSec;
        g_HudUiMgrObjectiveAutoHideDelaySec = 0.0f;
    }
}

/**
 * Reimplements 0x411ac0: HudUiMgrObjective::StartHide.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: advance objective panel show/hide phases, keep slide and meter
 * geometry synchronized, manage transition visibility, and trigger auto-hide
 * completion.
 */
void StartHide() {
    g_HudUiMgrObjectivePhaseTimerSec += g_Time_UnscaledDeltaTimeSec;

    if (g_HudUiMgrObjectivePhase == 1) {
        if (g_HudUiMgrObjectivePhaseTimerSec < g_HudUiMgrObjectivePhaseDurationSec) {
            const float fade =
                g_HudUiMgrObjectivePhaseTimerSec / g_HudUiMgrObjectivePhaseDurationSec;
            const float slideX =
                g_HudUiMgrObjectiveBar.points[1].x +
                fade * g_HudUiMgrObjectiveBar.slideRangeX;
            HudUiMgrObjective_SetSlidePosition(slideX);
            HudUiMgrObjective_UpdateWidgetRightX();
            HudUiMgrObjective_DrawSensorNoise(
                fade,
                1
            );
        } else {
            const float slideX =
                g_HudUiMgrObjectiveBar.points[1].x + g_HudUiMgrObjectiveBar.slideRangeX;
            g_HudUiMgrObjectivePhase = 2;
            g_HudUiMgrObjectivePhaseTimerSec = 0.0f;
            HudUiMgrObjective_SetSlidePosition(slideX);
            HudUiMgrObjective_UpdateWidgetRightX();
            g_HudUiMgrObjectiveSummaryTextPanel->SetVisible(1);
            g_HudUiMgrObjectiveDescTextPanel->SetVisible(1);
            g_HudUiMgrObjectiveSensorRect.SetVisible(1);
        }
    } else if (g_HudUiMgrObjectivePhase == 2) {
        ((HudUiElement *)(g_HudUiMgrObjectiveSummaryTextPanel))->Invalidate();
        ((HudUiElement *)(g_HudUiMgrObjectiveDescTextPanel))->Invalidate();
        g_HudUiMgrObjectiveBar.Invalidate();
        ((HudUiElement *)(&g_HudUiMgrObjectiveSensorRect))->Invalidate();
    } else if (g_HudUiMgrObjectivePhase == 3) {
        if (g_HudUiMgrObjectivePhaseTimerSec < g_HudUiMgrObjectivePhaseDurationSec) {
            const float fade =
                1.0f - g_HudUiMgrObjectivePhaseTimerSec / g_HudUiMgrObjectivePhaseDurationSec;
            const float slideX =
                g_HudUiMgrObjectiveBar.points[1].x +
                fade * g_HudUiMgrObjectiveBar.slideRangeX;
            HudUiMgrObjective_SetSlidePosition(slideX);
            HudUiMgrObjective_UpdateHwDirtyRectIfNeeded();
            HudUiMgrObjective_UpdateWidgetRightX();
            HudUiMgrObjective_DrawSensorNoise(
                fade,
                0
            );
        } else {
            g_HudUiMgrObjectiveState = 0;
            g_HudUiMgrObjectivePhase = 0;
            g_HudUiMgrObjectivePhaseTimerSec = 0.0f;
            ((HudUiElement *)(&g_HudUiMgrObjectiveWidget))
                ->SetX((int)(g_HudUiMgrObjectiveBar.points[1].x));
            HudUiMgrObjective::UpdateMeterXPoints();
            HudUiMgrObjective_UpdateHwDirtyRectIfNeeded();
            HudUiMgrObjective_UpdateWidgetRightX();
            g_HudUiMgrObjectiveBar.SetVisible(0);
            g_HudUiMgrSensorOverlay.SetVisible(1);
            gAltClipPassEnabled = 1;
        }
    }

    if (g_HudUiMgrObjectiveAutoHideDelaySec != 0.0f) {
        if (g_HudUiMgrObjectivePhaseTimerSec >= g_HudUiMgrObjectiveAutoHideDelaySec) {
            HudUiMgrObjective::Begin();
        }

        g_HudUiMgrObjectiveState = 1;
    }
}

/**
 * Reimplements 0x411eb0: HudUiMgrObjective::Update.
 * Purpose: advance the recovered HUD update path for HudUiMgrObjective::Update.
 */
void Update() {
    g_HudUiMgrObjectiveWidget.SetVisible(1);
    if (g_HudUiMgrObjectivePhase == 0) {
        return;
    }

    g_HudUiMgrObjectiveBar.SetVisible(1);
    if (g_HudUiMgrObjectivePhase != 2) {
        return;
    }

    if (g_HudUiMgrObjectiveDescTextPanel != 0) {
        ((HudUiElement *)(g_HudUiMgrObjectiveDescTextPanel))->SetVisible(1);
    }

    if (g_HudUiMgrObjectiveLabelTextPanel != 0) {
        ((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))->SetVisible(1);
    }

    g_HudUiMgrObjectiveSensorRect.SetVisible(1);
}

} // namespace HudUiMgrObjective

namespace HudUiMgrSensor {
/**
 * Reimplements 0x411f10: HudUiMgrSensor::SetShieldMessageRatio.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: clamp the shield ratio, update the HudUiMgr shield meter, and
 * refresh the shield percent text.
 */
void __fastcall SetShieldMessageRatio(
    float ratio
) {
    if (ratio > 1.0f) {
        ratio = 1.0f;
    } else if (ratio < 0.0f) {
        ratio = 0.0f;
    }

    HudUiShieldMessageWidget *const shieldMessageWidget = g_HudUiMgrShieldMessageWidget;
    HudUiMeter *const meter = &shieldMessageWidget->meter;
    const unsigned char green = ratio < 0.25f ? 0 : 255;
    meter->color565 = zVid_PackColorRGB(
        255,
        green,
        0
    ) & 0xffffu;

    const int fillPixels = (int)(ceil((double)(meter->fillPixelsMax) * (double)(ratio)));
    const int top = (int)(meter->points[1].y) - fillPixels;
    meter->points[0].y = (float)(top);
    meter->points[3].y = (float)(top);
    meter->Invalidate();

    HudUiPanel *const percentTextPanel = (HudUiPanel *)(&shieldMessageWidget->percentTextPanel);
    const int percent = (int)(ceil((double)(ratio) * 100.0));
    percentTextPanel->SetTextFmt(
        "%d",
        percent
    );
    percentTextPanel->Invalidate();
}

} // namespace HudUiMgrSensor

namespace HudUiMgrObjective {
/**
 * Reimplements 0x412050: HudUiMgrObjective::RefreshCounterText.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: format the objective counter panel from the supplied integer value
 * and rebuild its text bounds.
 */
void __fastcall RefreshCounterText(
    int counterValue
) {
    HudUiPanel *const panel = (HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel);
    panel->SetTextFmt(
        "%d",
        counterValue
    );
    panel->UpdateTextBoundsFromContent();
}

} // namespace HudUiMgrObjective

namespace HudUiMgrSensor {
/**
 * Reimplements 0x412070: HudUiMgrSensor::PlaceTrackCounterWidget.
 * Original source path: D:\Proj\Battlesport\HudUiMgrSensor.cpp.
 * Binary Ninja/source evidence keeps this in the sensor-target runtime owner:
 * one typed HudUiSlot is taken from g_HudUiMgrWeaponSlots, projected through
 * zMath into the slot screen fields, then clamped against the recovered
 * HudUiMgrSensorBlock viewport bounds for edge marker placement.
 * Purpose: reserve and position one sensor target marker slot for a tracked
 * player or turret world point.
 */
int __fastcall PlaceTrackCounterWidget(
    HudUiMgrSensorTrackNode *trackNode,
    const zVec3 *worldPoint
) {
    const int targetMarkerCount = g_HudUiMgrSensorTargetMarkerCount;
    int inBounds = 0;
    if (targetMarkerCount >= 32) {
        return 0;
    }

    HudUiSlot *const slot = &g_HudUiMgrWeaponSlots[targetMarkerCount];
    g_HudUiMgrSensorTargetMarkerCount = targetMarkerCount + 1;

    const int screenEdgeCode =
        zMath::ProjectPointAndClampToScreenClip(
            worldPoint,
            (zVec3 *)(&slot->screenX)
        );

    int slotX = (int)(slot->screenX);
    int slotY = (int)(slot->screenY);
    if (zOpt::GetReplicateMode() != 0) {
        slotX = (int)(slot->screenX + slot->screenX);
        slotY = (int)(slot->screenY + slot->screenY);
    }
    slot->SetPos(
        slotX,
        slotY
    );

    switch (screenEdgeCode) {
    case 0:
        inBounds = 1;
        break;

    case 1: {
        HudUiWidget *const counterWidget = &slot->slotWidget;
        counterWidget->SetVisible(1);
        counterWidget->SetImageBorrowedAndInvalidate(g_HudUiMgrSensorTargetMarkerImages[1]);

        const int halfHeight = counterWidget->image->height / 2;
        int top = slot->GetCenterY() - halfHeight;
        if (top <= g_HudUiMgrHudRect.top + halfHeight) {
            top = g_HudUiMgrHudRect.top;
        } else if (top > g_HudUiMgrSensorBlock.sensorViewportRect.top - halfHeight) {
            top = g_HudUiMgrSensorBlock.sensorViewportRect.top - halfHeight * 2;
        }
        counterWidget->SetPos(
            0,
            top
        );
        break;
    }

    case 2: {
        HudUiWidget *const counterWidget = &slot->slotWidget;
        counterWidget->SetVisible(1);
        counterWidget->SetImageBorrowedAndInvalidate(g_HudUiMgrSensorTargetMarkerImages[2]);

        const zVidImagePartial *const image = counterWidget->image;
        const int height = image->height;
        int top = slot->GetCenterY() - height;
        if (top <= g_HudUiMgrHudRect.top + height) {
            top = g_HudUiMgrHudRect.top;
        } else if (top > g_HudUiMgrHudRect.bottom - height) {
            top = g_HudUiMgrHudRect.bottom - height * 2;
        }

        const int left = slot->GetCenterX() + 1 - image->width;
        counterWidget->SetPos(
            left,
            top
        );
        break;
    }

    case 4: {
        HudUiWidget *const counterWidget = &slot->slotWidget;
        counterWidget->SetVisible(1);
        counterWidget->SetImageBorrowedAndInvalidate(g_HudUiMgrSensorTargetMarkerImages[3]);

        const zVidImagePartial *const image = counterWidget->image;
        const int top = slot->GetCenterY() + 1;
        const int left = slot->GetCenterX() - image->width / 2;
        counterWidget->SetPos(
            left,
            top
        );
        break;
    }

    case 8: {
        HudUiWidget *const counterWidget = &slot->slotWidget;
        counterWidget->SetVisible(1);
        counterWidget->SetImageBorrowedAndInvalidate(g_HudUiMgrSensorTargetMarkerImages[4]);

        int left = slot->GetCenterX();
        int top = slot->GetCenterY();
        if (left < g_HudUiMgrObjectiveWidgetRightX) {
            top = g_HudUiMgrSensorBlock.sensorViewportRect.top;
        }

        const zVidImagePartial *const image = counterWidget->image;
        top -= image->height;
        left -= image->width / 2;
        counterWidget->SetPos(
            left,
            top
        );
        break;
    }
    }

    slot->screenEdgeCode = screenEdgeCode;
    slot->trackNode = trackNode;
    return inBounds;
}

/**
 * Reimplements 0x4122c0: HudUiMgrSensor::PlaceTrackMarker.
 * Original source path: D:\Proj\Battlesport\HudUiMgrSensor.cpp.
 * The recovered source model walks the typed HudUiSlot sensor-marker range,
 * preserves the selected HudUiSlot pointer for progress updates, and uses the
 * track node kind as the discriminant for zUtil_SaveGameState versus
 * zTurret_Runtime payload casts before filling PlayerProgressTargetSlotRuntime.
 * Purpose: collect visible progress targets and highlight the nearest in-bounds
 * sensor marker when snap targeting is active.
 */
int __fastcall PlaceTrackMarker(
    int markerMode,
    PlayerProgressTargetSlotRuntime *outputSlots
) {
    const int HUD_SENSOR_MARKER_MODE_NEAREST = 1;
    const int HUD_SENSOR_MARKER_MODE_ALL = 2;

    HudUiSlot *const endSlot = &g_HudUiMgrWeaponSlots[g_HudUiMgrSensorTargetMarkerCount];
    HudUiSlot *slot = &g_HudUiMgrWeaponSlots[0];
    PlayerProgressTargetSlotRuntime *const firstOutputSlot = outputSlots;
    int result = 0;
    int nearestDistSq = 0x98967f;
    g_HudUiMgrSensorTrackedProgressSlot = 0;

    while (slot < endSlot) {
        if (slot->screenEdgeCode == 0) {
            if (markerMode == HUD_SENSOR_MARKER_MODE_ALL) {
                HudUiMgrSensorTrackNode *const trackNode =
                    (HudUiMgrSensorTrackNode *)(slot->trackNode);
                if (trackNode->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
                    zUtil_SaveGameState *const saveState =
                        (zUtil_SaveGameState *)(trackNode->payload);
                    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
                    outputSlots->targetPos = &playerState->fxOffsetWorld;
                    outputSlots->targetVelocity = &playerState->projectileSpawnVel;
                    ++outputSlots;
                    ++result;
                } else if (trackNode->trackKind == HUD_SENSOR_TRACK_KIND_TURRET) {
                    zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(trackNode->payload);
                    outputSlots->targetPos = &turretRuntime->firePos;
                    outputSlots->targetVelocity = 0;
                    ++outputSlots;
                    ++result;
                }
            }

            const int dx = slot->GetCenterX() - g_HudUiMgrReticleProjectedX;
            const int dy = slot->GetCenterY() - g_HudUiMgrReticleProjectedY;
            const int distSq = dx * dx + dy * dy;
            if (distSq < nearestDistSq) {
                g_HudUiMgrSensorTrackedProgressSlot = slot;
                nearestDistSq = distSq;
            }
        }

        ++slot;
    }

    outputSlots = firstOutputSlot;
    if (markerMode != HUD_SENSOR_MARKER_MODE_NEAREST ||
        nearestDistSq >= g_HudUiMgrReticleSnapRadiusSq ||
        g_HudUiMgrSensorTrackedProgressSlot == 0) {
        return result;
    }

    HudUiSlot *const trackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;
    trackedProgressSlot->trackMarkerWidget.SetImageBorrowedAndInvalidate(
        g_HudUiMgrSensorTargetMarkerImages[0]
    );

    const zVidImagePartial *const image = trackedProgressSlot->trackMarkerWidget.image;
    const int markerY = ((HudUiElement *)(trackedProgressSlot))->GetCenterY() - image->height / 2;
    const int markerX = ((HudUiElement *)(trackedProgressSlot))->GetCenterX() - image->width / 2;
    trackedProgressSlot->trackMarkerWidget.SetPos(
        markerX,
        markerY
    );
    trackedProgressSlot->trackMarkerWidget.SetVisible(1);

    HudUiMgrSensorTrackNode *const trackNode =
        (HudUiMgrSensorTrackNode *)(trackedProgressSlot->trackNode);
    if (trackNode->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
        zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(trackNode->payload);
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        outputSlots->targetPos = &playerState->fxOffsetWorld;
        outputSlots->targetVelocity = &playerState->projectileSpawnVel;
        return 1;
    }

    if (trackNode->trackKind == HUD_SENSOR_TRACK_KIND_TURRET) {
        zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(trackNode->payload);
        outputSlots->targetVelocity = 0;
        outputSlots->targetPos = &turretRuntime->firePos;
    }

    return 1;
}

} // namespace HudUiMgrSensor

namespace HudUiMgrTarget {
/**
 * Reimplements 0x4124b0: HudUiMgrTarget::UpdateSelectedProgressMeter.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * The selected progress meter consumes the HudUiSlot pointer saved by the
 * sensor-target runtime, casts the HudUiMgrSensorTrackNode payload according to
 * its track-kind discriminant, remaps the slot projection through zClipAlt, and
 * updates the recovered HudUiMgrSensorBlock-owned meter.
 * Purpose: show the selected target health meter at the projected sensor marker
 * position, or clear the selection when requested.
 */
void __fastcall UpdateSelectedProgressMeter(
    int clearSelectedTrack
) {
    HudUiSlot *trackedProgressSlot = 0;
    if (clearSelectedTrack != 0) {
        g_HudUiMgrSensorTrackedProgressSlot = 0;
    } else {
        trackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;
    }

    if (g_HudUiMgr.enabled == 0 || g_HudUiMgrObjectivePhase != 0 || trackedProgressSlot == 0) {
        return;
    }

    HudUiMgrSensorTrackNode *const selectedTrackNode =
        (HudUiMgrSensorTrackNode *)(trackedProgressSlot->trackNode);
    float selectedHealthCurrent = 0.0f;
    float selectedHealthMax = 1.0f;
    if (selectedTrackNode->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
        zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(selectedTrackNode->payload);
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        selectedHealthCurrent = playerState->statusMeterValue;
        selectedHealthMax = playerState->masterCommonData->maxHealth;
    } else if (selectedTrackNode->trackKind == HUD_SENSOR_TRACK_KIND_TURRET) {
        zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(selectedTrackNode->payload);
        selectedHealthCurrent = turretRuntime->healthCurrent;
        selectedHealthMax = turretRuntime->healthMax;
    }

    if (selectedHealthCurrent == 0.0f) {
        g_HudUiMgrSensorMeter.SetVisible(0);
        trackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;
    }

    if (zClipAlt::RemapPointXYInPlace(&trackedProgressSlot->screenX) == 0) {
        return;
    }

    if (zOpt::GetReplicateMode() != 0) {
        g_HudUiMgrSensorTrackedProgressSlot->screenX +=
            g_HudUiMgrSensorTrackedProgressSlot->screenX;
        g_HudUiMgrSensorTrackedProgressSlot->screenY +=
            g_HudUiMgrSensorTrackedProgressSlot->screenY;
    }

    float healthRatio = selectedHealthCurrent / selectedHealthMax;
    if (healthRatio > 1.0f) {
        healthRatio = 1.0f;
    } else if (healthRatio < 0.0f) {
        healthRatio = 0.0f;
    }

    const int fillPixels =
        (int)(ceil((double)(g_HudUiMgrSensorMeter.fillPixelsMax) * (double)(healthRatio)));
    const int top = (int)(g_HudUiMgrSensorMeter.points[1].y) - fillPixels;
    g_HudUiMgrSensorMeter.points[0].y = (float)(top);
    g_HudUiMgrSensorMeter.points[3].y = (float)(top);
    g_HudUiMgrSensorMeter.Invalidate();
    g_HudUiMgrSensorMeter.SetVisible(1);
}

} // namespace HudUiMgrTarget

namespace HudUiMgr {
/**
 * Reimplements 0x412620: HudUiMgr::HideTrackedProgressMeterIfOwnerMatches.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::HideTrackedProgressMeterIfOwnerMatches.
 */
void __fastcall HideTrackedProgressMeterIfOwnerMatches(
    void *ownerPayload
) {
    HudUiSlot *const trackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;
    if (trackedProgressSlot == 0) {
        return;
    }

    HudUiMgrSensorTrackNode *const trackNode =
        (HudUiMgrSensorTrackNode *)(trackedProgressSlot->trackNode);
    if (trackNode->payload == ownerPayload) {
        g_HudUiMgrSensorMeter.SetVisible(0);
    }
}

} // namespace HudUiMgr

/**
 * Reimplements 0x412650: HudUiMessage::SetValueIfOwnerMatches.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Updates a message panel value only when the requested owner side matches the active side.
 */
void __fastcall HudUiMessage::SetValueIfOwnerMatches(
    int messageIndex,
    int ownerSideIndex,
    float valueOrClearToken
) {
    HudUiMessage &message = g_HudUiMgrMessages[messageIndex];
    if (ownerSideIndex != message.panel.activeSideIndex) {
        return;
    }

    if (valueOrClearToken == kHudUiMessageClearSpecialTokenValue) {
        message.panel.SetText(g_HudUiMessage_ClearSpecialToken165);
        return;
    }

    message.panel.SetTextFmt(
        "%d",
        (int)(ceil(valueOrClearToken))
    );
    message.Invalidate();
}

/**
 * Reimplements 0x4126e0: HudUiMessage::SelectVariantDisplay.
 * Purpose: Selects the visible weapon-message variant image and refreshes the active side-image state.
 */
void __fastcall HudUiMessage::SelectVariantDisplay(
    int messageIndex,
    int variantIndex
) {
    HudUiMessage &message = g_HudUiMgrMessages[messageIndex];
    message.SetImageBorrowedAndInvalidate(message.variantImages[variantIndex]);

    if (variantIndex == 0 || variantIndex == 3) {
        message.activeSideImages[0] = message.sideImageSwaps[0];
        message.widget.SetImageBorrowedAndInvalidate(message.activeSideImages[1]);
        message.panel.activeSideIndex = 0;
    }

    if (variantIndex == 5) {
        message.panel.activeSideIndex = 0;
    }

    if (variantIndex == 1 || variantIndex == 4) {
        message.activeSideImages[1] = message.sideImageSwaps[1];
        message.widget.SetImageBorrowedAndInvalidate(message.activeSideImages[0]);
        message.panel.activeSideIndex = 1;
    }

    if (variantIndex == 6) {
        message.panel.activeSideIndex = 1;
    }
}

/**
 * Reimplements 0x412790: HudUiMessage::ApplySideImageSwap.
 * Purpose: Applies a side-image replacement for the selected message slot and preserves the visible flag.
 */
void __fastcall HudUiMessage::ApplySideImageSwap(
    int messageIndex,
    int sideIndex
) {
    HudUiMessage &message = g_HudUiMgrMessages[messageIndex];
    zVidImagePartial *const image = message.sideImageSwaps[sideIndex];
    message.activeSideImages[sideIndex] = image;
    message.widget.SetImageBorrowedAndInvalidate(image);
    message.widget.flags &= 0x10u;
}

/**
 * Reimplements 0x4127d0: HudUiMessage::ClearDisplay.
 * Purpose: Clears the message image, side image, and displayed text for one weapon-message slot.
 */
void __fastcall HudUiMessage::ClearDisplay(
    int messageIndex
) {
    HudUiMessage &message = g_HudUiMgrMessages[messageIndex];
    message.SetImageBorrowedAndInvalidate(0);
    message.widget.SetImageBorrowedAndInvalidate(0);

    message.panel.SetText("");
    message.Invalidate();
}

/**
 * Reimplements 0x412820: HudUiMessage::UpdateSelectedWeaponDisplay.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Updates active weapon-message images, selected side ownership, and value text.
 */
void __fastcall HudUiMessage::UpdateSelectedWeaponDisplay(
    int weaponBankIndex,
    int weaponSideIndex,
    float valueOrClearToken
) {
    int messageIndexForText = weaponBankIndex;
    if (weaponBankIndex > 1) {
        SelectVariantDisplay(
            g_HudUiMgrActiveWeaponMessageIndex,
            g_HudUiMgrActiveWeaponSideIndex
        );
        g_HudUiMgrActiveWeaponMessageIndex = weaponBankIndex;
        g_HudUiMgrActiveWeaponSideIndex = weaponSideIndex;
        if (valueOrClearToken > 0.0f) {
            SelectVariantDisplay(
                weaponBankIndex,
                weaponSideIndex + 3
            );
        }
    } else if (weaponBankIndex == 1) {
        SelectVariantDisplay(
            1,
            weaponSideIndex + 3
        );
        messageIndexForText = 1;
    } else {
        g_HudUiMgrActiveWeaponMessageIndex = 0;
        g_HudUiMgrActiveWeaponSideIndex = 0;
        return;
    }

    HudUiMessage &message = g_HudUiMgrMessages[messageIndexForText];
    if (weaponSideIndex != message.panel.activeSideIndex) {
        return;
    }

    if (valueOrClearToken == kHudUiMessageClearSpecialTokenValue) {
        message.panel.SetTextFmt(g_HudUiMessage_ClearSpecialToken165);
        return;
    }

    message.panel.SetTextFmt(
        "%d",
        (int)(ceil(valueOrClearToken))
    );
    message.Invalidate();
}

/**
 * Reimplements 0x412b60: HudLayoutSW::Constructor.
 * Source file evidence: BN labels this function as a Battlesport hud.cpp helper.
 * Purpose: construct the software HUD layout container and attach its base widget child.
 */
HudLayoutSW * HudLayoutSW::Constructor() {
    new ((HudUiContainer *)this) HudUiContainer;
    HudUiWidget *const childWidget = (HudUiWidget *)(&widget0);
    childWidget->Constructor(0);
    AddChild((HudUiElement *)(childWidget));
    return this;
}

/**
 * Reimplements 0x412bd0: HudLayoutBase::SetActive.
 * Purpose: provide the default layout activation result for base layout callers.
 */
int HudLayoutBase::SetActive(
    int
) {
    return 1;
}

/**
 * Reimplements 0x412be0: HudLayoutBase::UpdateAll.
 * Purpose: forward per-frame layout updates through the recovered container base.
 */
void HudLayoutBase::UpdateAll(
    float deltaSeconds
) {
    HudUiContainer::UpdateAll(deltaSeconds);
}

/**
 * Reimplements 0x412bf0: HudLayoutBase::Enable.
 * Purpose: activate this HUD layout through the recovered base SetActive slot.
 */
void HudLayoutBase::Enable() {
    SetActive(1);
}

/**
 * Reimplements 0x412c00: HudLayoutBase::Disable.
 * Purpose: deactivate this HUD layout through the recovered base SetActive slot.
 */
void HudLayoutBase::Disable() {
    SetActive(0);
}

/**
 * Reimplements 0x412c10: HudLayoutSW::LoadTypeIFromZarRoot.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: load the TYPEI HUD layout rectangle from the ZRD root.
 */
void HudLayoutBase::LoadTypeIFromZarRoot(
    zReader::Node *parentNode
) {
    zReader::Node *const typeINode = zReader_GetNamedNode(
        parentNode,
        g_HudLayout_TypeISectionName
    );
    if (typeINode == 0) {
        return;
    }

    HudUiLayoutNode::ReadRectOffsetAndSize(
        &typeINode->value.nodes[1],
        &layoutRect,
        0,
        0,
        0
    );
    activeRect = layoutRect;
}

/**
 * Reimplements 0x412c60: HudLayoutSW::SetActive.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: apply the software HUD viewport and active sensor occlusion state.
 */
int HudLayoutSW::SetActive(
    int active
) {
    if (zVid::GetAccelerationOption() == ZVID_HW_MODE_SOFTWARE) {
        zRndr::SpanOcclusionResetFrame();
    }

    activeRect.right = zVideo::GetPrimarySurfaceWidth();
    activeRect.bottom = layoutRect.bottom + g_HudUiMgrHudOriginY;
    HudLayout::ApplyViewportRect(&activeRect);

    if (active != 0) {
        HudUiRect outerRect;
        outerRect.top = activeRect.top + 1;
        outerRect.left = activeRect.left + 1;
        outerRect.right = activeRect.right - 1;
        outerRect.bottom = activeRect.bottom - 1;

        g_HudSensorTracker.SetBounds(
            &outerRect,
            &g_HudUiMgrSensorBlock.sensorViewportRect
        );
        g_HudUiMgr.SetChildFlags(0);
        SetChildFlags(0);
        zClipAlt::SetSourceRect(&g_HudUiMgrSensorBlock.sensorPiVSrcRect);

        if (g_HudUiMgr.enabled != 0 && zVid::GetAccelerationOption() == ZVID_HW_MODE_SOFTWARE) {
            const int replicateMode = zOpt::GetReplicateMode();
            float nearClip = 0.0f;
            float farClip = 0.0f;
            zClass_Camera::gwCameraGetNearFarClip(
                g_MainCamera,
                &nearClip,
                &farClip
            );

            const float invNearClip = 1.0f / nearClip;
            zRndr::SpanOcclusionSubmitOccluderRect(
                &g_HudUiMgrSensorBlock.sensorViewportRect,
                replicateMode,
                invNearClip
            );
            zRndr::SpanOcclusionSubmitOccluderRect(
                &g_HudUiMgrShieldMessageWidget->screenRect,
                replicateMode,
                invNearClip
            );

            {
                for (int index = 0; index < 4; ++index) {
                    zRndr::SpanOcclusionSubmitOccluderRect(
                        &g_HudUiMgrModeCounters[index].clipViewportRect,
                        replicateMode,
                        invNearClip
                    );
                }
            }
        }
    }

    return 1;
}

namespace HudLayout {
/**
 * Reimplements 0x412db0: HudLayout::ApplyViewportRect.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: update display and render viewport sections from the active HUD rectangle.
 */
int __fastcall ApplyViewportRect(
    HudUiRect *activeRect
) {
    const int replicateMode = zOpt::GetReplicateMode();
    const int left = activeRect->left;
    const int top = activeRect->top;

    zOpt::DisplaySection_SetPosition(
        left,
        top
    );

    int renderX = left;
    int renderY = top;
    if (replicateMode != 0) {
        renderX = (left - (left >> 31)) >> 1;
        renderY = (top - (top >> 31)) >> 1;
    }

    zOpt::RenderSection_SetPosition(
        renderX,
        renderY
    );

    int width = activeRect->right - left;
    int height = activeRect->bottom - top;
    zOpt::DisplaySection_SetSize(
        width,
        height
    );

    const float viewportWidth = (float)(width);
    const float viewportHeight = (float)(height);

    if (replicateMode != 0) {
        width = (width - (width >> 31)) >> 1;
        height = (height - (height >> 31)) >> 1;
    }

    zOpt::RenderSection_SetSize(
        width,
        height
    );

    zClass_NodePartial *const camera = g_HudSensorTracker.cameraNode;
    if (camera != 0) {
        float fovX = 0.0f;
        float fovY = 0.0f;
        zClass_Camera::gwCameraGetFOV(
            camera,
            &fovX,
            &fovY
        );
        fovY = viewportHeight / viewportWidth * fovX;
        zClass_Camera::gwCameraSetFOV(
            camera,
            fovX,
            fovY
        );
    }

    zOpt_ViewRectSection *const renderSection = zOpt::GetRenderSection();
    HudUiMgr::OnViewportChanged(
        (const HudUiRect *)(zOpt::GetDisplaySection()),
        (const HudUiRect *)(renderSection)
    );
    return 1;
}

} // namespace HudLayout

/**
 * Reimplements 0x412ea0: HudLayoutHW::Constructor.
 * Source file evidence: BN labels this function as a Battlesport hud.cpp helper.
 * Purpose: construct the hardware HUD layout container and attach its three image widgets.
 */
HudLayoutHW * HudLayoutHW::Constructor() {
    new ((HudUiContainer *)this) HudUiContainer;
    HudUiWidget *const baseWidget = (HudUiWidget *)(&widget0);
    baseWidget->Constructor(0);
    AddChild((HudUiElement *)(baseWidget));

    HudUiWidget *const widget1Object = (HudUiWidget *)(&widget1);
    widget1Object->Constructor(0);
    HudUiWidget *const widget2Object = (HudUiWidget *)(&widget2);
    widget2Object->Constructor(0);
    HudUiWidget *const widget3Object = (HudUiWidget *)(&widget3);
    widget3Object->Constructor(0);

    AddChild((HudUiElement *)(widget1Object));
    AddChild((HudUiElement *)(widget3Object));
    AddChild((HudUiElement *)(widget2Object));
    return this;
}

/**
 * Reimplements 0x412f70: HudLayoutHW::LoadTypeIIFromZarRoot.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: load the TYPEII HUD layout widgets and alternate image variants from ZRD data.
 */
int HudLayoutHW::LoadTypeIIFromZarRoot(
    zReader::Node *parentNode
) {
    zReader::Node *const typeIINode = zReader_GetNamedNode(
        parentNode,
        g_HudLayout_TypeIISectionName
    );
    if (typeIINode == 0) {
        return 1;
    }

    zReader::Node *const typeIIPayload = typeIINode->value.nodes;
    HudLayoutBase *const layout = (HudLayoutBase *)(this);

    HudUiLayoutNode::ReadRectOffsetAndSize(
        &typeIIPayload[1],
        &layout->layoutRect,
        0,
        0,
        0
    );
    layout->activeRect = layout->layoutRect;

    HudUiLayoutNode::ApplyImageWidget(
        &typeIIPayload[2],
        &widget1,
        0,
        0,
        0,
        0,
        0
    );
    HudUiLayoutNode::ApplyImageWidget(
        &typeIIPayload[3],
        &widget3,
        0,
        g_HudUiMgrHudOriginY,
        0,
        0,
        0
    );
    HudUiLayoutNode::ApplyImageWidget(
        &typeIIPayload[4],
        &widget2,
        0,
        g_HudUiMgrHudOriginY,
        0,
        0,
        0
    );

    zReader::Node *const imageNames = typeIIPayload[5].value.nodes;
    widget1ImageDefault = widget1.image;
    widget1Image320 = zImage::TexDir_FindOrCreateByPath(imageNames[1].value.str);
    widget1Image400 = zImage::TexDir_FindOrCreateByPath(imageNames[2].value.str);
    widget2ImageDefault = widget2.image;
    widget2Image320 = zImage::TexDir_FindOrCreateByPath(imageNames[3].value.str);
    widget2Image400 = zImage::TexDir_FindOrCreateByPath(imageNames[4].value.str);

    return 1;
}

/**
 * Reimplements 0x413080: HudLayoutHW::ReleaseImages.
 * Purpose: release hardware HUD layout alternate images and clear their cached pointers.
 */
void HudLayoutHW::ReleaseImages() {
    zVid_Image::ReleaseIfNotDefault(widget1Image320);
    zVid_Image::ReleaseIfNotDefault(widget1Image400);
    zVid_Image::ReleaseIfNotDefault(widget2Image320);
    zVid_Image::ReleaseIfNotDefault(widget2Image400);

    widget2Image400 = 0;
    widget2Image320 = 0;
    widget1Image400 = 0;
    widget1Image320 = 0;
}

/**
 * Reimplements 0x4130d0: HudLayoutHW::SetActive.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: apply the hardware HUD viewport and connect or clear widget blit sources.
 */
int HudLayoutHW::SetActive(
    int active
) {
    if (zVid::GetAccelerationOption() == 0) {
        zRndr::SpanOcclusionResetFrame();
    }

    HudLayoutBase *const layout = (HudLayoutBase *)(this);
    layout->activeRect.right = zVideo::GetPrimarySurfaceWidth();
    layout->activeRect.bottom = layout->layoutRect.bottom + g_HudUiMgrHudOriginY;
    HudLayout::ApplyViewportRect(&layout->activeRect);

    if (active == 0) {
        ((HudUiElement *)(g_HudUiMgrObjectiveCounterTextPanel))->SetBltSourceAndClipRect(
            0,
            0
        );
        ((HudUiElement *)(g_HudUiMgrTimerPanel))->SetBltSourceAndClipRect(
            0,
            0
        );
        ((HudUiElement *)(&g_HudUiMgrNanitePanel))->SetBltSourceAndClipRect(
            0,
            0
        );

        {
            for (int index = 0; index < 10; ++index) {
                HudUiMessage &message = g_HudUiMgrMessages[index];
                message.SetBltSourceAndClipRect(
                    0,
                    0
                );
                message.panel.SetBltSourceAndClipRect(
                    0,
                    0
                );
            }
        }

        const int clearState = zVideo::ExchangeClearScreenBufferEnabled(1);
        zVideo::CallClearPrimarySurfaceAndZBuffer(0);
        zVideo::ExchangeClearScreenBufferEnabled(clearState);
        return 1;
    }

    layout->OnActivated();

    zVidImagePartial *const widget1Image = widget1.image;
    zVidImagePartial *const widget2Image = widget2.image;
    ((HudUiElement *)(g_HudUiMgrObjectiveCounterTextPanel))
        ->SetBltSourceAndClipRect(
            widget1Image,
            0
        );
    ((HudUiElement *)(g_HudUiMgrTimerPanel))->SetBltSourceAndClipRect(
        widget1Image,
        0
    );

    {
        for (int index = 1; index < 10; ++index) {
            HudUiMessage &message = g_HudUiMgrMessages[index];
            message.SetBltSourceAndClipRect(
                widget2Image,
                0
            );
            message.panel.SetBltSourceAndClipRect(
                widget2Image,
                0
            );
        }
    }

    ((HudUiElement *)(&g_HudUiMgrNanitePanel))->SetBltSourceAndClipRect(
        widget2Image,
        0
    );
    zClipAlt::SetSourceRect(&g_HudUiMgrSensorBlock.sensorPiVSrcRect);

    if (g_HudUiMgr.enabled != 0 && zVid::GetAccelerationOption() == 0) {
        HudUiRect occluderRect;
        occluderRect.left = g_HudUiMgrSensorBlock.sensorViewportRect.left;
        occluderRect.top = g_HudUiMgrSensorBlock.sensorViewportRect.top;
        occluderRect.right = g_HudUiMgrSensorBlock.sensorViewportRect.right;
        occluderRect.bottom = g_HudUiMgrHudRect.bottom;

        float nearClip = 0.0f;
        float farClip = 0.0f;
        zClass_Camera::gwCameraGetNearFarClip(
            g_MainCamera,
            &nearClip,
            &farClip
        );
        zRndr::SpanOcclusionSubmitOccluderRect(
            &occluderRect,
            zOpt::GetReplicateMode(),
            1.0f / nearClip
        );
    }

    return 1;
}

/**
 * Reimplements 0x4132b0: HudLayoutHW::UpdateObjectiveDirtyRect.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Rebuilds the objective dirty rectangle and refreshes the nanite panel after HUD layout changes.
 */
void HudLayoutHW::UpdateObjectiveDirtyRect() {
    zVidImagePartial *const image = g_HudUiMgrObjectiveWidget.image;
    const int width = image != 0 ? image->width : 0;

    const int centerX = g_HudUiMgrObjectiveWidget.GetCenterX();
    const int centerY = g_HudUiMgrObjectiveWidget.GetCenterY();

    const int height = image != 0 ? image->height : 0;
    HudUiRect dirtyRect;
    dirtyRect.left = centerX + width;
    dirtyRect.top = centerY;
    dirtyRect.right = g_HudUiMgrObjectiveWidgetRightX;
    dirtyRect.bottom = centerY + height;

    widget2.InvalidateRect(&dirtyRect);
    ((HudUiElement *)(&g_HudUiMgrNanitePanel))->Invalidate();
    ((HudUiTripletPanel *)(&g_HudUiMgrNanitePanel))->Draw();
}

/**
 * Reimplements 0x413340: HudLayoutHW::OnActivated.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: activate hardware HUD widgets, image variants, and sensor bounds.
 */
void HudLayoutHW::OnActivated() {
    HudUi::SetInvalidateMode(zOpt::GetReplicateMode() == 0 ? 1 : 0);

    g_HudUiMgr.SetChildFlags(0x0e);
    SetChildFlags(0x0e);

    widget2.flags = (unsigned int)((unsigned char)(widget2.flags) & 0x10u);

    g_HudUiMgrObjectiveWidget.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrObjectiveWidget.flags) & 0x10u);
    g_HudUiMgrObjectiveMeter.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrObjectiveMeter.flags) & 0x10u);
    ((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))->flags =
        (unsigned int)((unsigned char)(((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))
                               ->flags) &
                       0x10u);
    g_HudUiMgrSensorOverlay.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrSensorOverlay.flags) & 0x10u);

    g_HudUiMgrStatsList->triplet->RebuildDisplay();

    {
        for (int index = 1; index < 10; ++index) {
            HudUiMessage &message = g_HudUiMgrMessages[index];
            if (message.widget.image != 0) {
                message.widget.flags =
                    (unsigned int)((unsigned char)(message.widget.flags) & 0x10u);
            }
        }
    }

    HudLayoutBase *const layout = (HudLayoutBase *)(this);
    HudUiRect outerRect;
    outerRect.left = layout->activeRect.left + 1;
    outerRect.top = layout->activeRect.top + 1;
    outerRect.right = layout->activeRect.right - 1;
    outerRect.bottom = layout->activeRect.bottom - 1;

    HudUiRect *innerRect = 0;
    if (zOpt::GetReplicateMode() == 0) {
        innerRect = &g_HudUiMgrSensorBlock.sensorViewportRect;
    }
    g_HudSensorTracker.SetBounds(
        &outerRect,
        innerRect
    );

    zVidImagePartial *widget1Image = widget1ImageDefault;
    zVidImagePartial *widget2Image = widget2ImageDefault;
    if (layout->activeRect.right == 0x320) {
        widget1Image = widget1Image320;
        widget2Image = widget2Image320;
    } else if (layout->activeRect.right == 0x400) {
        widget1Image = widget1Image400;
        widget2Image = widget2Image400;
    }

    widget2.SetImageBorrowedAndInvalidate(widget2Image);
    widget1.SetImageBorrowedAndInvalidate(widget1Image);

    if (g_HudUiMgrHudLayoutsInitialized != 0) {
        widget2.InvalidateRect(&g_HudUiMgrViewRect);
    }
}

/**
 * Reimplements 0x4134e0: HudUiMessage::Draw.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Draws the weapon-message base widget and its embedded text panel.
 */
void HudUiMessage::Draw() {
    HudUiWidget::Draw();
    panel.Draw();
}

/**
 * Reimplements 0x413500: HudLayoutHW::UpdateAll.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: refresh hardware HUD replication blits before container child updates.
 */
void HudLayoutHW::UpdateAll(
    float deltaSeconds
) {
    if (g_HudUiMgr.enabled != 0 && zOpt::GetReplicateMode() != 0 && g_HudUiMgrObjectivePhase == 0) {
        g_zVideo_pfnBltSwToPrimaryRectDirect(
            (zVidRect32 *)(&g_HudUiMgrSensorBlock.sensorRectScaled),
            (zVidRect32 *)(&g_HudUiMgrSensorBlock.sensorRectRaw)
        );
    }

    HudUiContainer::UpdateAll(deltaSeconds);
}

/**
 * Reimplements 0x413540: HudLayoutHW::Enable.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: enable hardware HUD layout children and mark dependent widgets visible.
 */
void HudLayoutHW::Enable() {
    g_HudUiMgr.SetChildFlags(0x0e);
    SetChildFlags(0x0e);

    widget2.flags = (unsigned int)((unsigned char)(widget2.flags) & 0x10u);

    g_HudUiMgrObjectiveWidget.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrObjectiveWidget.flags) & 0x10u);
    g_HudUiMgrObjectiveMeter.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrObjectiveMeter.flags) & 0x10u);
    ((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))->flags =
        (unsigned int)((unsigned char)(((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))
                               ->flags) &
                       0x10u);
    g_HudUiMgrSensorOverlay.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrSensorOverlay.flags) & 0x10u);

    {
        for (int index = 1; index < 10; ++index) {
            HudUiMessage &message = g_HudUiMgrMessages[index];
            if (message.widget.image != 0) {
                message.widget.flags =
                    (unsigned int)((unsigned char)(message.widget.flags) & 0x10u);
            }
        }
    }

    SetEnabled(1);
}

/**
 * Reimplements 0x4135f0: HudLayoutHW::Disable.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: disable the hardware HUD layout container.
 */
void HudLayoutHW::Disable() {
    SetEnabled(0);
}

namespace zOpt {
/**
 * Reimplements 0x413600: zOpt::ToggleHudTypeForCurrentHwMode.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Toggle the HUD type between standard and perspective for the current hardware mode.
 */
int ToggleHudTypeForCurrentHwMode() {
    const int currentHudType = GetHudTypeForCurrentHwMode();
    if (currentHudType == ZOPT_HUD_TYPE_STANDARD) {
        return SetHudTypeForCurrentHwMode(ZOPT_HUD_TYPE_PERSPECTIVE);
    }
    if (currentHudType == ZOPT_HUD_TYPE_PERSPECTIVE) {
        return SetHudTypeForCurrentHwMode(ZOPT_HUD_TYPE_STANDARD);
    }
    return GetHudTypeForCurrentHwMode();
}

} // namespace zOpt

namespace HudUiMgr {
/**
 * Reimplements 0x413630: HudUiMgr::TriggerCurrentLayoutOnActivated.
 * Purpose: Re-run the active HUD layout activation hook when a layout is present.
 */
void TriggerCurrentLayoutOnActivated() {
    if (g_HudUiMgrCurrentLayout != 0) {
        g_HudUiMgrCurrentLayout->OnActivated();
    }
}

/**
 * Reimplements 0x413640: HudUiMgr::ToggleHud.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::ToggleHud.
 */
int ToggleHud() {
    if (g_HudUiMgr.enabled != 0) {
        DisableHud();
    } else {
        EnableHud();
    }
    return 1;
}

/**
 * Reimplements 0x413660: HudUiMgr::SwitchActiveDialog.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::SwitchActiveDialog.
 */
void __fastcall SwitchActiveDialog(
    HudLayoutBase *newDialog
) {
    const int enabled = g_HudUiMgr.enabled;
    if (enabled != 0) {
        DisableHud();
    } else {
        g_HudUiMgrLayoutDelayFrames = 2;
    }

    if (g_HudUiMgrCurrentLayout != 0) {
        g_HudUiMgrCurrentLayout->SetActive(0);
    }

    newDialog->SetActive(1);
    g_HudUiMgrCurrentLayout = newDialog;

    if (enabled != 0) {
        EnableHud();
    }
}

/**
 * Reimplements 0x4136b0: HudUiMgr::ApplyHudModeSwitch.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: apply the recovered HUD layout or option state handled by HudUiMgr::ApplyHudModeSwitch.
 */
int __fastcall ApplyHudModeSwitch(
    int hudType
) {
    const int currentType = zOpt::GetHudTypeForCurrentHwMode();
    if (g_HudUiMgrHudLayoutsInitialized != 0) {
        if (hudType == 1) {
            SwitchActiveDialog((HudLayoutBase *)(&g_HudLayoutSW));
        } else if (hudType == 2) {
            SwitchActiveDialog((HudLayoutBase *)(&g_HudLayoutHW));
        }
    }

    return currentType;
}

} // namespace HudUiMgr
