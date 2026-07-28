#pragma once

#include "GameZRecoil/zHud/zhud_ui.h"

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduitransitiontextpanel-huduitransitiontextpanel
 * @recoil-artifact defines .text recoil:function:0x4ba020: HudUiTransitionTextPanel::HudUiTransitionTextPanel.
 * Purpose: construct the transition text panel and initialize its flash state.
 *
 * Evidence: BN assembly calls HudUiPanel::ConstructorDefault, clears flash
 * fields, writes flashResetValue = 0.35f and flashDirectionSign = 1, and
 * installs the transition text panel table at 0x4cd388. HudUiBackground's
 * 50-element transition-panel member passes this constructor to VC5's EH
 * array-construction helper, retaining the standalone body at 0x4ba020, while
 * ordinary constructions such as the one at 0x4bb790 inline the same body.
 */
inline HudUiTransitionTextPanel::HudUiTransitionTextPanel()
    : HudUiPanel(
        0,
        0,
        0
    ) {
    flashResetValue = 0.349999994f;
    flashCountdown = 0;
    flashAltColor0 = 0;
    flashEnabled = 0;
    flashMode = 0;
    flashDirectionSign = 1;
}
