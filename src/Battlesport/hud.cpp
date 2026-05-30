#include "Battlesport/hud.h"

#include "Battlesport/GameNet.h"
#include "Battlesport/Mfc42Abi.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "Battlesport/zStr.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zUtil/zSaveGame.h"

#include <stdlib.h>
#include <string.h>

HudUiNewGamePanelOverlayOwner g_HudUiNewGamePanelOverlayOwner;
HudUiOptionsPanelOverlayOwner g_HudUiOptionsPanelOverlayOwner;
RecoilStateConfirmQuit g_RecoilState_ConfirmQuit;
RecoilStateControls g_RecoilStateControls;
RecoilStateCheatCode g_RecoilStateCheatCode;
zSndSample *g_Hud_LowMeterBeepSample = 0;
zSndSample *g_Hud_LowMeterLoopSample = 0;
int g_Hud_LowMeterLoopActive = 0;
float g_Hud_LowMeterBeepInterval = 0.0f;
float g_Hud_LowMeterNextBeepTime = 0.0f;

extern "C" int g_RecoilState_MainMenuSkipExitDelay;

namespace {
template <typename Method> unsigned int HudMethodAddress(Method method)
{
    RECOIL_STATIC_ASSERT(sizeof(method) <= sizeof(unsigned int));
    unsigned int address = 0;
    memcpy(&address, &method, sizeof(method));
    return address;
}

HudUiCommon_FTable MakeHudWeatherFxFTable()
{
    HudUiCommon_FTable table = {0};
    table.slots[3] = HudMethodAddress(&HudUiElement::SetPos);
    table.slots[4] = HudMethodAddress(&HudUiElement::SetX);
    table.slots[5] = HudMethodAddress(&HudUiElement::SetY);
    table.slots[6] = HudMethodAddress(&HudUiElement::SetBltSourceAndClipRect);
    table.slots[7] = HudMethodAddress(&HudUiElement::SetClipRect);
    table.slots[8] = HudMethodAddress(&HudUiElement::Invalidate);
    table.slots[24] = HudMethodAddress(&HudUiElement::SetVisible);
    table.slots[25] = HudMethodAddress(&HudUiElement::GetX);
    table.slots[26] = HudMethodAddress(&HudUiElement::GetY);
    return table;
}

const float kHudWeatherFxDepthRandScale = -0.0000152592547f;
const float kHudWeatherFxConeRandScale = -0.0000457777642f;
const float kHudWeatherFxDepthBase = 0.5f;
const float kHudWeatherFxConeBase = -1.0f;
const float kHudWeatherFxReflectBias = 1.5f;

enum zVideoRendererBackend
{
    ZVID_RENDERER_BACKEND_SOFTWARE = 0,
};

struct HudUiBackgroundConfirmQuitVirtual
{
    virtual void RECOIL_THISCALL Update(float deltaSeconds) = 0;
    virtual void RECOIL_THISCALL SetEnabled(int enabled) = 0;
    virtual HudUiBackgroundConfirmQuitVirtual *RECOIL_THISCALL
    ScalarDeletingDestructor(unsigned int flags) = 0;
};

struct HudUiBackgroundConfirmQuit_FTable
{
    unsigned int slots[3];
};

struct HudUiCheatCodeDialog_FTable
{
    unsigned int slots[3];
};

struct HudUiCheatCodeDialogVirtual
{
    virtual void RECOIL_THISCALL Update(float deltaSeconds) = 0;
    virtual void RECOIL_THISCALL SetEnabled(int enabled) = 0;
    virtual HudUiCheatCodeDialogVirtual *RECOIL_THISCALL
    ScalarDeletingDestructor(unsigned int flags) = 0;
};

RECOIL_NOINLINE void RECOIL_CDECL HudUiConfirmQuitPostLoadNoOp()
{
}

HudUiWidget_FTable MakeConfirmQuitButtonFTable(unsigned int activateCallback)
{
    HudUiWidget_FTable table = {0};
    table.slots[0] = HudMethodAddress(&HudUiZrdWidget::ScalarDeletingDestructor);
    table.slots[1] = HudMethodAddress(&HudUiWidget::Draw);
    table.slots[3] = HudMethodAddress(&HudUiElement::SetPos);
    table.slots[4] = HudMethodAddress(&HudUiElement::SetX);
    table.slots[5] = HudMethodAddress(&HudUiElement::SetY);
    table.slots[6] = HudMethodAddress(&HudUiElement::SetBltSourceAndClipRect);
    table.slots[7] = HudMethodAddress(&HudUiElement::SetClipRect);
    table.slots[8] = HudMethodAddress(&HudUiZrdWidget::Invalidate);
    table.slots[12] = activateCallback;
    table.slots[15] = HudMethodAddress(&HudUiZrdWidget::ShowPreview);
    table.slots[16] = HudMethodAddress(&HudUiZrdWidget::HidePreview);
    table.slots[24] = HudMethodAddress(&HudUiElement::SetVisible);
    table.slots[25] = HudMethodAddress(&HudUiElement::GetX);
    table.slots[26] = HudMethodAddress(&HudUiElement::GetY);
    table.slots[30] = HudMethodAddress(&HudUiZrdWidget::RefreshState);
    table.slots[31] = HudMethodAddress(&HudUiZrdWidget::LoadFromZrd);
    table.slots[32] = (unsigned int)&HudUiConfirmQuitPostLoadNoOp;
    return table;
}

HudUiBackgroundConfirmQuit_FTable MakeConfirmQuitDialogFTable()
{
    HudUiBackgroundConfirmQuit_FTable table = {0};
    table.slots[0] = HudMethodAddress(&HudUiBackground::Update);
    table.slots[1] = HudMethodAddress(&HudUiBackground::SetEnabled);
    table.slots[2] = HudMethodAddress(&HudUiBackgroundConfirmQuit::ScalarDeletingDestructor);
    return table;
}

HudUiZrdWidget_FTable MakeCheatCodeTitleWidgetFTable()
{
    HudUiZrdWidget_FTable table = {0};
    table.slots[0] = HudMethodAddress(&HudUiZrdWidget::ScalarDeletingDestructor);
    table.slots[1] = HudMethodAddress(&HudUiWidget::Draw);
    table.slots[3] = HudMethodAddress(&HudUiElement::SetPos);
    table.slots[4] = HudMethodAddress(&HudUiElement::SetX);
    table.slots[5] = HudMethodAddress(&HudUiElement::SetY);
    table.slots[6] = HudMethodAddress(&HudUiElement::SetBltSourceAndClipRect);
    table.slots[7] = HudMethodAddress(&HudUiElement::SetClipRect);
    table.slots[8] = HudMethodAddress(&HudUiZrdWidget::Invalidate);
    table.slots[12] = HudMethodAddress(&HudUiCheatTextInputWidget::OnActivate);
    table.slots[15] = HudMethodAddress(&HudUiZrdWidget::ShowPreview);
    table.slots[16] = HudMethodAddress(&HudUiZrdWidget::HidePreview);
    table.slots[24] = HudMethodAddress(&HudUiElement::SetVisible);
    table.slots[25] = HudMethodAddress(&HudUiElement::GetX);
    table.slots[26] = HudMethodAddress(&HudUiElement::GetY);
    table.slots[30] = HudMethodAddress(&HudUiZrdWidget::RefreshState);
    table.slots[31] = HudMethodAddress(&HudUiZrdWidget::LoadFromZrd);
    table.slots[32] = (unsigned int)&HudUiConfirmQuitPostLoadNoOp;
    return table;
}

HudUiNumericTextInput_Base_FTable MakeCheatCodeInputWidgetFTable()
{
    HudUiNumericTextInput_Base_FTable table = {0};
    table.slots[0] = HudMethodAddress(&HudUiNumericTextInput::ScalarDeletingDestructor);
    table.slots[1] = HudMethodAddress(&HudUiWidget::Draw);
    table.slots[2] = HudMethodAddress(&HudUiNumericTextInput::UpdateCaptureUiAndClip);
    table.slots[3] = HudMethodAddress(&HudUiElement::SetPos);
    table.slots[4] = HudMethodAddress(&HudUiElement::SetX);
    table.slots[5] = HudMethodAddress(&HudUiElement::SetY);
    table.slots[6] = HudMethodAddress(&HudUiElement::SetBltSourceAndClipRect);
    table.slots[7] = HudMethodAddress(&HudUiElement::SetClipRect);
    table.slots[8] = HudMethodAddress(&HudUiZrdWidget::Invalidate);
    table.slots[12] = HudMethodAddress(&HudUiCheatTextInputWidget::OnActivate);
    table.slots[15] = HudMethodAddress(&HudUiZrdWidget::ShowPreview);
    table.slots[16] = HudMethodAddress(&HudUiZrdWidget::HidePreview);
    table.slots[24] = HudMethodAddress(&HudUiElement::SetVisible);
    table.slots[25] = HudMethodAddress(&HudUiElement::GetX);
    table.slots[26] = HudMethodAddress(&HudUiElement::GetY);
    table.slots[30] = HudMethodAddress(&HudUiZrdWidget::RefreshState);
    table.slots[31] = HudMethodAddress(&HudUiZrdWidget::LoadFromZrd);
    table.slots[32] = (unsigned int)&HudUiConfirmQuitPostLoadNoOp;
    table.slots[34] = HudMethodAddress(&HudUiNumericTextInput::OnRawKeyboardChar);
    return table;
}

HudUiWidget_FTable MakeHudUiNewGamePanelStartButtonFTable()
{
    HudUiWidget_FTable table = {0};
    table.slots[0] = HudMethodAddress(&HudUiZrdWidget::ScalarDeletingDestructorThunk);
    table.slots[1] = HudMethodAddress(&HudUiWidget::Draw);
    table.slots[3] = HudMethodAddress(&HudUiElement::SetPos);
    table.slots[4] = HudMethodAddress(&HudUiElement::SetX);
    table.slots[5] = HudMethodAddress(&HudUiElement::SetY);
    table.slots[6] = HudMethodAddress(&HudUiElement::SetBltSourceAndClipRect);
    table.slots[7] = HudMethodAddress(&HudUiElement::SetClipRect);
    table.slots[8] = HudMethodAddress(&HudUiZrdWidget::Invalidate);
    table.slots[12] = HudMethodAddress(&HudUiNewGamePanel_StartButton::OnActivate);
    table.slots[15] = HudMethodAddress(&HudUiZrdWidget::ShowPreview);
    table.slots[16] = HudMethodAddress(&HudUiZrdWidget::HidePreview);
    table.slots[24] = HudMethodAddress(&HudUiElement::SetVisible);
    table.slots[25] = HudMethodAddress(&HudUiElement::GetX);
    table.slots[26] = HudMethodAddress(&HudUiElement::GetY);
    table.slots[30] = HudMethodAddress(&HudUiZrdWidget::RefreshState);
    table.slots[31] = HudMethodAddress(&HudUiZrdWidget::LoadFromZrd);
    table.slots[32] = (unsigned int)&HudUiConfirmQuitPostLoadNoOp;
    return table;
}

HudUiNumericTextInput_Base_FTable MakeHudUiNewGamePanelNameInputFTable()
{
    HudUiNumericTextInput_Base_FTable table = {0};
    table.slots[0] = HudMethodAddress(&HudUiNumericTextInput::ScalarDeletingDestructorThunk);
    table.slots[1] = HudMethodAddress(&HudUiWidget::Draw);
    table.slots[2] = HudMethodAddress(&HudUiNumericTextInput::UpdateCaptureUiAndClip);
    table.slots[3] = HudMethodAddress(&HudUiElement::SetPos);
    table.slots[4] = HudMethodAddress(&HudUiElement::SetX);
    table.slots[5] = HudMethodAddress(&HudUiElement::SetY);
    table.slots[6] = HudMethodAddress(&HudUiElement::SetBltSourceAndClipRect);
    table.slots[7] = HudMethodAddress(&HudUiElement::SetClipRect);
    table.slots[8] = HudMethodAddress(&HudUiZrdWidget::Invalidate);
    table.slots[12] = HudMethodAddress(&HudUiNewGamePanel_NameInput::OnActivate);
    table.slots[15] = HudMethodAddress(&HudUiZrdWidget::ShowPreview);
    table.slots[16] = HudMethodAddress(&HudUiZrdWidget::HidePreview);
    table.slots[24] = HudMethodAddress(&HudUiElement::SetVisible);
    table.slots[25] = HudMethodAddress(&HudUiElement::GetX);
    table.slots[26] = HudMethodAddress(&HudUiElement::GetY);
    table.slots[30] = HudMethodAddress(&HudUiZrdWidget::RefreshState);
    table.slots[31] = HudMethodAddress(&HudUiZrdWidget::LoadFromZrd);
    table.slots[32] = (unsigned int)&HudUiConfirmQuitPostLoadNoOp;
    table.slots[34] = HudMethodAddress(&HudUiNumericTextInput::OnRawKeyboardChar);
    return table;
}

HudUiNewGamePanel_FTableHeader MakeHudUiNewGamePanelFTable()
{
    HudUiNewGamePanel_FTableHeader table = {0};
    table.primarySlots[0] = HudMethodAddress(&HudUiBackground::Update);
    table.primarySlots[1] = HudMethodAddress(&HudUiBackground::SetEnabled);
    table.primarySlots[2] = HudMethodAddress(&HudUiNewGamePanel::ScalarDeletingDestructor);

    table.SecondaryAction.slots[0] =
        HudMethodAddress(&HudUiZrdWidgetEx17C::ScalarDeletingDestructorThunk);
    table.SecondaryAction.slots[1] = HudMethodAddress(&HudUiWidget::Draw);
    table.SecondaryAction.slots[3] = HudMethodAddress(&HudUiElement::SetPos);
    table.SecondaryAction.slots[4] = HudMethodAddress(&HudUiElement::SetX);
    table.SecondaryAction.slots[5] = HudMethodAddress(&HudUiElement::SetY);
    table.SecondaryAction.slots[6] = HudMethodAddress(&HudUiElement::SetBltSourceAndClipRect);
    table.SecondaryAction.slots[7] = HudMethodAddress(&HudUiElement::SetClipRect);
    table.SecondaryAction.slots[8] = HudMethodAddress(&HudUiZrdWidget::Invalidate);
    table.SecondaryAction.slots[12] = HudMethodAddress(&HudUiZrdWidget::OnActivate);
    table.SecondaryAction.slots[15] = HudMethodAddress(&HudUiZrdWidget::ShowPreview);
    table.SecondaryAction.slots[16] = HudMethodAddress(&HudUiZrdWidget::HidePreview);
    table.SecondaryAction.slots[24] = HudMethodAddress(&HudUiZrdWidgetEx17C::EnableChildAtIndex);
    table.SecondaryAction.slots[25] = HudMethodAddress(&HudUiElement::GetX);
    table.SecondaryAction.slots[26] = HudMethodAddress(&HudUiElement::GetY);
    table.SecondaryAction.slots[30] = HudMethodAddress(&HudUiZrdWidget::RefreshState);
    table.SecondaryAction.slots[31] = HudMethodAddress(&HudUiZrdWidgetEx17C::LoadFromZrd);
    table.SecondaryAction.slots[32] = (unsigned int)&HudUiConfirmQuitPostLoadNoOp;
    return table;
}

HudUiCheatCodeDialog_FTable MakeCheatCodeDialogFTable()
{
    HudUiCheatCodeDialog_FTable table = {0};
    table.slots[0] = HudMethodAddress(&HudUiBackground::Update);
    table.slots[1] = HudMethodAddress(&HudUiBackground::SetEnabled);
    table.slots[2] = HudMethodAddress(&HudUiCheatCodeDialog::ScalarDeletingDestructor);
    return table;
}

const HudUiWidget_FTable g_HudUiConfirmQuitCancelButton_FTable =
    MakeConfirmQuitButtonFTable(
        HudMethodAddress(&HudUiZrdWidget::OnActivateQueueExitCurrentState));
const HudUiWidget_FTable g_HudUiConfirmQuitOkButton_FTable =
    MakeConfirmQuitButtonFTable(HudMethodAddress(&HudUiConfirmQuitOkButton::OnActivate));
const HudUiBackgroundConfirmQuit_FTable g_HudUiBackgroundConfirmQuit_FTable =
    MakeConfirmQuitDialogFTable();
const HudUiCheatCodeDialog_FTable g_HudUiCheatCodeDialog_FTable =
    MakeCheatCodeDialogFTable();

RecoilApp_IState_Vtbl g_RecoilStateConfirmQuit_Vtbl = {0};
RecoilApp_IState_Vtbl g_RecoilStateCheatCode_Vtbl = {0};

struct HudUiOptionsPanelOverlayVirtual
{
    virtual void RECOIL_THISCALL Update(float deltaSeconds) = 0;
    virtual void RECOIL_THISCALL SetEnabled(int enabled) = 0;
    virtual HudUiOptionsPanelOverlayVirtual *RECOIL_THISCALL
    ScalarDeletingDestructor(unsigned int flags) = 0;
};

struct RecoilStateConfirmQuitBaseVtableGuard
{
    RecoilStateConfirmQuit *self;

    ~RecoilStateConfirmQuitBaseVtableGuard()
    {
        self->vftable = kRecoilStateBase_VtblAddress;
    }
};

struct RecoilStateCheatCodeBaseVtableGuard
{
    RecoilStateCheatCode *self;

    ~RecoilStateCheatCodeBaseVtableGuard()
    {
        self->vftable = kRecoilStateBase_VtblAddress;
    }
};

struct HudUiOptionsPanelOverlayOwnerBaseVtableGuard
{
    HudUiOptionsPanelOverlayOwner *self;

    ~HudUiOptionsPanelOverlayOwnerBaseVtableGuard()
    {
        self->vftable = kRecoilStateBase_VtblAddress;
    }
};
} // namespace

RecoilApp_IState_Vtbl g_HudUiOptionsPanelOverlayOwner_Vtbl = {0};

const HudUiCommon_FTable g_HudWeatherFx_Vtable = MakeHudWeatherFxFTable();
const HudUiCommon_FTable g_HudWeatherFxSnow_Vtable = MakeHudWeatherFxFTable();
const HudUiCommon_FTable g_HudWeatherFxRain_Vtable = MakeHudWeatherFxFTable();
extern const HudUiZrdWidget_FTable g_HudUiCheatCodeTitleWidget_FTable =
    MakeCheatCodeTitleWidgetFTable();
extern const HudUiNumericTextInput_Base_FTable g_HudUiCheatCodeInputWidget_FTable =
    MakeCheatCodeInputWidgetFTable();
extern const HudUiNewGamePanel_FTableHeader g_HudUiNewGamePanel_FTableHeader =
    MakeHudUiNewGamePanelFTable();
extern const HudUiWidget_FTable g_HudUiNewGamePanel_StartButton_Vtbl =
    MakeHudUiNewGamePanelStartButtonFTable();
extern const HudUiNumericTextInput_Base_FTable g_HudUiNewGamePanel_NameInput_Vtbl =
    MakeHudUiNewGamePanelNameInputFTable();

// Reimplements 0x4bdc70: HudWeatherFx::Constructor
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE HudWeatherFx *RECOIL_THISCALL HudWeatherFx::Constructor(int newParticleCount)
{
    HudUiElement::Constructor(0, 0);
    viewportRect = 0;
    ftable = &g_HudWeatherFx_Vtable;
    maxParticles = newParticleCount;
    particleCount = newParticleCount;
    particleQuads = (HudWeatherFxParticleQuad *)(
        ::operator new(sizeof(HudWeatherFxParticleQuad) * newParticleCount));

    for (int index = 0; index < newParticleCount; ++index)
    {
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
    particlePositions[sourceBufferIndex] =
        (zVec3 *)(::operator new(positionBytes));
    particlePositions[destBufferIndex] =
        (zVec3 *)(::operator new(positionBytes));

    for (int resetIndex = 0; resetIndex < newParticleCount; ++resetIndex)
    {
        ResetParticleSlot(resetIndex, 1);
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

    if (g_zVideo_ActiveRendererPath != 0)
    {
        textureName = "SnowFX";
        softwareImage = zVid_Image::Create();
        zVid_Image::SetFormatCode(softwareImage, 0x0b);
        char *const alphaMap = (char *)(malloc(0x80));
        void *const surfacePixels = malloc(0x100);
        zVid_Image_SetPixels(softwareImage, surfacePixels, alphaMap);
        softwareImage->formatFlagsPacked |= 0x20;
        zVid_Image::SetSize(softwareImage, 16, 8);
        textureRecord = g_zVideo_pfnCreateTextureRecord(
            textureName, softwareImage, softwareImage->formatFlagsPacked & 0x02, 1, 1);
    }

    return this;
}

// Reimplements 0x4bdee0: HudWeatherFx::ResetParticleSlot
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
HudWeatherFx::ResetParticleSlot(int particleIndex, int)
{
    zVec3 *const sourcePosition = &particlePositions[sourceBufferIndex][particleIndex];
    zVec3 *const destPosition = &particlePositions[destBufferIndex][particleIndex];

    sourcePosition->z =
        kHudWeatherFxDepthBase - (float)(rand()) * kHudWeatherFxDepthRandScale;

    sourcePosition->x =
        kHudWeatherFxConeBase - (float)(rand()) * kHudWeatherFxConeRandScale;
    if (sourcePosition->x < -sourcePosition->z)
    {
        sourcePosition->x += kHudWeatherFxReflectBias;
        sourcePosition->z = kHudWeatherFxReflectBias - sourcePosition->z;
    }

    sourcePosition->y =
        kHudWeatherFxConeBase - (float)(rand()) * kHudWeatherFxConeRandScale;
    if (sourcePosition->y < -sourcePosition->z)
    {
        sourcePosition->y += kHudWeatherFxReflectBias;
        sourcePosition->z = kHudWeatherFxReflectBias - sourcePosition->z;
    }

    *destPosition = *sourcePosition;
}

// Reimplements 0x4be280: HudWeatherFxSnow::Constructor
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE HudWeatherFxSnow *RECOIL_THISCALL
HudWeatherFxSnow::Constructor(int particleCount)
{
    HudWeatherFx::Constructor(particleCount);
    ftable = &g_HudWeatherFxSnow_Vtable;
    emitEnabled = 1;
    emitRadius = 20.0f;
    emitDepth = 400.0f;
    return this;
}

// Reimplements 0x4be810: HudWeatherFxRain::Constructor
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE HudWeatherFxRain *RECOIL_THISCALL
HudWeatherFxRain::Constructor(int particleCount)
{
    HudWeatherFx::Constructor(particleCount);
    ftable = &g_HudWeatherFxRain_Vtable;
    emitEnabled = 1;
    emitRadius = 20.0f;
    emitDepth = 400.0f;
    return this;
}

// Reimplements 0x41c6c0: HudUiNewGamePanelOverlayOwner::QueueEnter
// (D:\Proj\Battlesport\hud.cpp)
void RECOIL_CDECL HudUiNewGamePanelOverlayOwner::QueueEnter()
{
    g_RecoilApp.QueuePushState(&g_HudUiNewGamePanelOverlayOwner, 0);
}

// Reimplements 0x41c560: HudUiNewGamePanelOverlayOwner::OnTryBecomeCurrent
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
HudUiNewGamePanelOverlayOwner::OnTryBecomeCurrent()
{
    HudUiNewGamePanel *panel =
        (HudUiNewGamePanel *)::operator new(sizeof(HudUiNewGamePanel));
    if (panel != 0)
    {
        panel = panel->Constructor();
    }

    m_panel = (RecoilPtr32)(unsigned int)panel;
    panel->SyncIntensityFromDifficulty();
    ((HudUiBackground *)panel)->SetEnabled(1);
    return 1;
}

// Reimplements 0x41c290: HudUiNewGamePanel::Constructor
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
HudUiNewGamePanel *RECOIL_THISCALL HudUiNewGamePanel::Constructor()
{
    HudUiBackground::Constructor();

    backWidget.Constructor();
    backWidget.base.ftable = &g_HudUiMainMenu_BackButton_FTable;
    startWidget.Constructor();
    startWidget.base.ftable = &g_HudUiNewGamePanel_StartButton_Vtbl;
    nameInput.BaseConstructor();
    nameInput.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNewGamePanel_NameInput_Vtbl);
    intensity.Constructor();
    intensity.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNewGamePanel_FTableHeader.SecondaryAction);

    base.base.vptr = (const HudUiContainer_FTable *)(&g_HudUiNewGamePanel_FTableHeader);

    zReader::Node *const loadedSection =
        HudUiBackground::LoadFromZrd("dialog.zrd", "NEWGAMEPANEL", 0);
    if (loadedSection != 0)
    {
        HudUiBackground::BindWidgetByName(loadedSection, &backWidget.base, "BACK");
        HudUiBackground::BindWidgetByName(loadedSection, &startWidget.base, "START");
        HudUiBackground::BindWidgetByName(loadedSection, &nameInput.base.base, "NAME");
        HudUiBackground::BindWidgetByName(loadedSection, &intensity.base.base, "INTENSITY");
        HudUiBackground::FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
    }

    nameInput.Update(zOpt_GetPlayerName());
    return this;
}

// Reimplements 0x41c3b0: HudUiNewGamePanel_NameInput::OnActivate
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
void RECOIL_THISCALL HudUiNewGamePanel_NameInput::OnActivate()
{
    HudUiNumericTextInput::AllocTextBuffer(21);
    HudUiNumericTextInput::Update(zOpt_GetPlayerName());
    HudUiNumericTextInput::OnActivate();
    HudUiNumericTextInput::SetRawKeyboardCapture(1);
}

// Reimplements 0x41c400: HudUiNewGamePanel::Destructor
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiNewGamePanel::Destructor()
{
    intensity.DestructorCore();
    nameInput.Destructor();
    startWidget.DestructorCore();
    backWidget.DestructorCore();
    HudUiBackground::Destructor();
}

// Reimplements 0x41c3e0: HudUiNewGamePanel::ScalarDeletingDestructor
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
RECOIL_NOINLINE HudUiNewGamePanel *RECOIL_THISCALL
HudUiNewGamePanel::ScalarDeletingDestructor(unsigned int flags)
{
    Destructor();
    if ((flags & 1u) != 0)
    {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x41c4e0: HudUiNewGamePanel::SyncIntensityFromDifficulty
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
void RECOIL_THISCALL HudUiNewGamePanel::SyncIntensityFromDifficulty()
{
    intensity.SetSelectedIndex(zOpt::GetGameDifficultyMode());
}

// Reimplements 0x41c500: HudUiNewGamePanel::StartGameFromFields
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
void RECOIL_THISCALL HudUiNewGamePanel::StartGameFromFields()
{
    HudCheat::ClearNanitePanelCheatSentinel();
    zOpt::SetPlayerName(nameInput.GetBuffer());
    zOpt::SetGameDifficultyMode(intensity.selectedIndex);
    ((HudUiBackgroundContainer *)(&g_RecoilApp.m_missionFmvState_1d8))->SetEnabled(1);
    g_RecoilApp.QueueExitCurrentState(1);
    g_RecoilApp.QueueExitCurrentState(1);
    g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_missionFmvState_1d8.base, 0);
}

// Reimplements 0x41c270: HudUiNewGamePanel_StartButton::OnActivate
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
void RECOIL_THISCALL HudUiNewGamePanel_StartButton::OnActivate()
{
    HudUiNewGamePanel *const panel = (HudUiNewGamePanel *)(owner);
    if (panel != 0) {
        panel->StartGameFromFields();
    }

    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x40d1c0: HudUiOptionsPanelOverlayOwner::QueueEnter
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
void RECOIL_CDECL HudUiOptionsPanelOverlayOwner::QueueEnter()
{
    g_RecoilApp.QueuePushState(&g_HudUiOptionsPanelOverlayOwner, 0);
}

// Reimplements 0x40d070: HudUiOptionsPanelOverlayOwner::StaticInitAndRegisterAtExit
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
RECOIL_NOINLINE void RECOIL_CDECL
HudUiOptionsPanelOverlayOwner::StaticInitAndRegisterAtExit()
{
    StaticInit();
    RegisterAtExit();
}

// Reimplements 0x40d080: HudUiOptionsPanelOverlayOwner::StaticInit
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
RECOIL_NOINLINE HudUiOptionsPanelOverlayOwner *RECOIL_CDECL
HudUiOptionsPanelOverlayOwner::StaticInit()
{
    return g_HudUiOptionsPanelOverlayOwner.Constructor();
}

// Reimplements 0x40d090: HudUiOptionsPanelOverlayOwner::RegisterAtExit
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
RECOIL_NOINLINE void RECOIL_CDECL HudUiOptionsPanelOverlayOwner::RegisterAtExit()
{
    atexit(AtExitDestructor);
}

// Reimplements 0x40d0a0: HudUiOptionsPanelOverlayOwner::AtExitDestructor
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
RECOIL_NOINLINE void RECOIL_CDECL HudUiOptionsPanelOverlayOwner::AtExitDestructor()
{
    g_HudUiOptionsPanelOverlayOwner.DestructorCore();
}

// Reimplements 0x40d0b0: HudUiOptionsPanelOverlayOwner::Constructor
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
HudUiOptionsPanelOverlayOwner *RECOIL_THISCALL
HudUiOptionsPanelOverlayOwner::Constructor()
{
    vftable = (RecoilPtr32)(unsigned int)&g_HudUiOptionsPanelOverlayOwner_Vtbl;
    m_panel = 0;
    return this;
}

// Reimplements 0x40d0e0: HudUiOptionsPanelOverlayOwner::DestructorCore
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
HudUiOptionsPanelOverlayOwner::DestructorCore()
{
    vftable = (RecoilPtr32)(unsigned int)&g_HudUiOptionsPanelOverlayOwner_Vtbl;
    HudUiOptionsPanelOverlayOwnerBaseVtableGuard baseVtableOnExit = {this};

    HudUiOptionsPanelOverlayVirtual *panel =
        (HudUiOptionsPanelOverlayVirtual *)m_panel;
    if (panel != 0)
    {
        panel->SetEnabled(0);

        panel = (HudUiOptionsPanelOverlayVirtual *)m_panel;
        if (panel != 0)
        {
            panel->ScalarDeletingDestructor(1);
        }

        m_panel = 0;
    }
}

// Reimplements 0x40d0c0: HudUiOptionsPanelOverlayOwner::ScalarDeletingDestructor
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
RECOIL_NOINLINE HudUiOptionsPanelOverlayOwner *RECOIL_THISCALL
HudUiOptionsPanelOverlayOwner::ScalarDeletingDestructor(unsigned int flags)
{
    DestructorCore();
    if ((flags & 1u) != 0)
    {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x40d150: HudUiOptionsPanelOverlayOwner::OnTryBecomeCurrent
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
HudUiOptionsPanelOverlayOwner::OnTryBecomeCurrent()
{
    HudOptionsDialog *panel = (HudOptionsDialog *)::operator new(sizeof(HudOptionsDialog));
    if (panel != 0)
    {
        panel = panel->Constructor();
    }

    m_panel = (RecoilPtr32)(unsigned int)panel;

    HudUiOptionsPanelOverlayVirtual *panelView =
        (HudUiOptionsPanelOverlayVirtual *)panel;
    panelView->SetEnabled(1);
    return 1;
}

// Reimplements 0x4159b0: RecoilStateConfirmQuit::QueueEnter
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
void RECOIL_CDECL RecoilStateConfirmQuit::QueueEnter()
{
    g_RecoilApp.QueuePushState(&g_RecoilState_ConfirmQuit, 0);
}

// Reimplements 0x409160: HudUiZrdWidget::OnActivateQueueExitCurrentState
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
void RECOIL_THISCALL HudUiZrdWidget::OnActivateQueueExitCurrentState()
{
    g_RecoilApp.QueueExitCurrentState(0);
    OnActivate();
}

// Reimplements 0x409180: HudUiCreditsQuitButton::OnActivate
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
void RECOIL_THISCALL HudUiCreditsQuitButton::OnActivate()
{
    g_RecoilApp.QueueExitCurrentState(1);
    g_RecoilApp.m_missionShutdownMode = RECOILAPP_MISSION_SHUTDOWN_SKIP_GAMEPLAY;
    g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_leaveNetworkState_1d0.base, 0);
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x415810: RecoilStateConfirmQuit::StaticInitAndRegisterAtExit
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
RECOIL_NOINLINE void RECOIL_CDECL RecoilStateConfirmQuit::StaticInitAndRegisterAtExit()
{
    StaticInit();
    RegisterAtExit();
}

// Reimplements 0x415820: RecoilStateConfirmQuit::StaticInit
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
RECOIL_NOINLINE RecoilStateConfirmQuit *RECOIL_CDECL RecoilStateConfirmQuit::StaticInit()
{
    return g_RecoilState_ConfirmQuit.Constructor();
}

// Reimplements 0x415830: RecoilStateConfirmQuit::RegisterAtExit
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
RECOIL_NOINLINE void RECOIL_CDECL RecoilStateConfirmQuit::RegisterAtExit()
{
    atexit(AtExitDestructor);
}

// Reimplements 0x415840: RecoilStateConfirmQuit::AtExitDestructor
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
RECOIL_NOINLINE void RECOIL_CDECL RecoilStateConfirmQuit::AtExitDestructor()
{
    g_RecoilState_ConfirmQuit.~RecoilStateConfirmQuit();
}

// Reimplements 0x415740: HudUiConfirmQuitOkButton::OnActivate
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
void RECOIL_THISCALL HudUiConfirmQuitOkButton::OnActivate()
{
    g_RecoilState_MainMenuSkipExitDelay = 1;
    g_RecoilApp.QueueExitCurrentState(1);
    g_RecoilApp.QueueExitCurrentState(0);
    g_RecoilApp.m_missionShutdownMode = RECOILAPP_MISSION_SHUTDOWN_SKIP_GAMEPLAY;
    g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_leaveNetworkState_1d0.base, 0);
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x415680: HudUiBackgroundConfirmQuit::Constructor
// (D:\Proj\Battlesport\HudUiBackgroundConfirmQuit.cpp)
HudUiBackgroundConfirmQuit *RECOIL_THISCALL HudUiBackgroundConfirmQuit::Constructor()
{
    HudUiBackground::Constructor();
    okButton.Constructor();
    okButton.base.ftable = &g_HudUiConfirmQuitOkButton_FTable;
    cancelButton.Constructor();
    cancelButton.base.ftable = &g_HudUiConfirmQuitCancelButton_FTable;
    base.base.vptr = (const HudUiContainer_FTable *)&g_HudUiBackgroundConfirmQuit_FTable;

    zReader::Node *const dialogRoot =
        HudUiBackground::LoadFromZrd("dialog.zrd", "CONFIRM_QUIT", 0);
    if (dialogRoot != 0)
    {
        HudUiBackground::BindWidgetByName(dialogRoot, &okButton.base, "OK_TO_QUIT");
        HudUiBackground::BindWidgetByName(dialogRoot, &cancelButton.base, "CANCEL_QUIT");
        HudUiBackground::FreeLoadedTreeRoots((int)dialogRoot);
    }

    return this;
}

// Reimplements 0x4157b0: HudUiBackgroundConfirmQuit::Destructor
// (D:\Proj\Battlesport\HudUiBackgroundConfirmQuit.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiBackgroundConfirmQuit::Destructor()
{
    cancelButton.DestructorCore();
    okButton.DestructorCore();
    HudUiBackground::Destructor();
}

// Reimplements 0x415790: HudUiBackgroundConfirmQuit::ScalarDeletingDestructor
// (D:\Proj\Battlesport\HudUiBackgroundConfirmQuit.cpp)
RECOIL_NOINLINE HudUiBackgroundConfirmQuit *RECOIL_THISCALL
HudUiBackgroundConfirmQuit::ScalarDeletingDestructor(unsigned int flags)
{
    Destructor();

    if ((flags & 1u) != 0)
    {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4070e0: HudUiCheatTextInputWidget::OnActivate
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
void RECOIL_THISCALL HudUiCheatTextInputWidget::OnActivate()
{
    g_RecoilApp.QueueExitCurrentState(0);
    base.OnActivate();
}

// Reimplements 0x406d20: HudUiCheatCodeDialog::Constructor
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
HudUiCheatCodeDialog *RECOIL_THISCALL HudUiCheatCodeDialog::Constructor()
{
    HudUiBackground::Constructor();

    titleWidget.Constructor();
    titleWidget.base.ftable = (const HudUiWidget_FTable *)&g_HudUiCheatCodeTitleWidget_FTable;

    cheatInputWidget.BaseConstructor();
    cheatInputWidget.base.base.ftable =
        (const HudUiWidget_FTable *)&g_HudUiCheatCodeInputWidget_FTable;
    cheatInputWidget.textInput.AllocTextBuffer(80);
    cheatInputWidget.Update("");
    cheatInputWidget.SetInputActive(1);
    cheatInputWidget.SetRawKeyboardCapture(1);

    base.base.vptr = (const HudUiContainer_FTable *)&g_HudUiCheatCodeDialog_FTable;

    zReader::Node *const dialogRoot =
        HudUiBackground::LoadFromZrd("dialog.zrd", "CHEAT_CODE_DIALOG", 0);
    if (dialogRoot != 0)
    {
        HudUiBackground::BindWidgetByName(dialogRoot, &titleWidget.base, "GO");
        HudUiBackground::BindWidgetByName(dialogRoot, &cheatInputWidget.base.base,
                                          "CHEATCODE");
        HudUiBackground::FreeLoadedTreeRoots((int)dialogRoot);
    }

    return this;
}

// Reimplements 0x406e30: HudUiCheatCodeDialog::Destructor
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiCheatCodeDialog::Destructor()
{
    cheatInputWidget.Destructor();
    titleWidget.DestructorCore();
    HudUiBackground::Destructor();
}

// Reimplements 0x406e10: HudUiCheatCodeDialog::ScalarDeletingDestructor
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
RECOIL_NOINLINE HudUiCheatCodeDialog *RECOIL_THISCALL
HudUiCheatCodeDialog::ScalarDeletingDestructor(unsigned int flags)
{
    Destructor();

    if ((flags & 1u) != 0)
    {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x406e90: RecoilStateCheatCode::StaticInitAndRegisterAtExit
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
RECOIL_NOINLINE void RECOIL_CDECL RecoilStateCheatCode::StaticInitAndRegisterAtExit()
{
    ConstructGlobal();
    StaticInit();
}

// Reimplements 0x406ea0: RecoilStateCheatCode::ConstructGlobal
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
RECOIL_NOINLINE RecoilStateCheatCode *RECOIL_CDECL RecoilStateCheatCode::ConstructGlobal()
{
    return g_RecoilStateCheatCode.Constructor();
}

// Reimplements 0x406eb0: RecoilStateCheatCode::StaticInit
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
RECOIL_NOINLINE void RECOIL_CDECL RecoilStateCheatCode::StaticInit()
{
    atexit(AtExitDestructor);
}

// Reimplements 0x406ec0: RecoilStateCheatCode::AtExitDestructor
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
RECOIL_NOINLINE void RECOIL_CDECL RecoilStateCheatCode::AtExitDestructor()
{
    g_RecoilStateCheatCode.~RecoilStateCheatCode();
}

// Reimplements 0x406ed0: RecoilStateCheatCode::Constructor
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
RecoilStateCheatCode *RECOIL_THISCALL RecoilStateCheatCode::Constructor()
{
    vftable = (RecoilPtr32)(unsigned int)&g_RecoilStateCheatCode_Vtbl;
    m_dialog = 0;
    return this;
}

// Reimplements 0x406f60: RecoilStateCheatCode::OnTryBecomeCurrent
// (D:\Proj\Battlesport\RecoilStateCheatCode.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL RecoilStateCheatCode::OnTryBecomeCurrent()
{
    if (g_zVideo_ActiveRendererPath != ZVID_RENDERER_BACKEND_SOFTWARE)
    {
        g_zVideo_pfnBltSwToPrimaryRectDirect(0, 0);
    }

    m_prevHalfResAdjustMode =
        (zVideoHalfResAdjustMode)zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);
    HudUi::SetInvalidateMode(0);

    zSndPlayHandleSnapshot *const audioSnapshot =
        zSndPlayHandleSnapshot::CreateFromActiveSamples();
    m_audioSnapshot = (RecoilPtr32)(unsigned int)audioSnapshot;
    audioSnapshot->StopAllIfPlaying();

    zSndSampleSet_InitByName("DIALOG");

    HudUiCheatCodeDialog *dialog =
        (HudUiCheatCodeDialog *)::operator new(sizeof(HudUiCheatCodeDialog));
    if (dialog != 0)
    {
        dialog = dialog->Constructor();
    }
    m_dialog = (RecoilPtr32)(unsigned int)dialog;

    HudUiCheatCodeDialogVirtual *const dialogView =
        (HudUiCheatCodeDialogVirtual *)dialog;
    dialogView->SetEnabled(1);
    return 1;
}

// Reimplements 0x407010: RecoilStateCheatCode::OnDeactivate
// (D:\Proj\Battlesport\RecoilStateCheatCode.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL RecoilStateCheatCode::OnDeactivate()
{
    CString commandString;

    HudUiCheatCodeDialog *dialog =
        (HudUiCheatCodeDialog *)(unsigned int)m_dialog;
    if (dialog != 0)
    {
        commandString = dialog->cheatInputWidget.GetBuffer();

        zVideo::RunPostprocessOnPrimaryBuffer();

        HudUiCheatCodeDialogVirtual *dialogView =
            (HudUiCheatCodeDialogVirtual *)(unsigned int)m_dialog;
        dialogView->SetEnabled(0);

        ((HudUiDialogController *)(unsigned int)m_dialog)->BlitOwnedSurfaceToPrimary();
        zVideo::Dispatch_UnlockPrimarySurfaceState();

        dialogView = (HudUiCheatCodeDialogVirtual *)(unsigned int)m_dialog;
        if (dialogView != 0)
        {
            dialogView->ScalarDeletingDestructor(1);
        }

        m_dialog = 0;
    }

    zSndSampleSet_DestroyByName("DIALOG");

    zSndPlayHandleSnapshot *const audioSnapshot =
        (zSndPlayHandleSnapshot *)(unsigned int)m_audioSnapshot;
    if (audioSnapshot != 0)
    {
        audioSnapshot->RestoreAllWithGlobalVolumeDelta();
    }

    zVideo::SetHalfResAdjustMode(m_prevHalfResAdjustMode);
    HudUi::SetInvalidateMode(m_prevHalfResAdjustMode);
    HudUiMgr::TriggerCurrentLayoutOnActivated();
    HudCheat::ExecuteCommandString(&commandString);
}

// Reimplements 0x406f00: RecoilStateCheatCode::Destructor
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
RECOIL_NOINLINE RecoilStateCheatCode::~RecoilStateCheatCode()
{
    vftable = (RecoilPtr32)(unsigned int)&g_RecoilStateCheatCode_Vtbl;
    RecoilStateCheatCodeBaseVtableGuard baseVtableOnExit = {this};

    HudUiCheatCodeDialogVirtual *dialogView =
        (HudUiCheatCodeDialogVirtual *)m_dialog;
    if (dialogView != 0)
    {
        dialogView->ScalarDeletingDestructor(1);
    }

    m_dialog = 0;
}

// Reimplements 0x406ee0: RecoilStateCheatCode::ScalarDeletingDestructor
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
RECOIL_NOINLINE RecoilStateCheatCode *RECOIL_THISCALL
RecoilStateCheatCode::ScalarDeletingDestructor(unsigned int flags)
{
    this->~RecoilStateCheatCode();

    if ((flags & 1u) != 0)
    {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x415850: RecoilStateConfirmQuit::Constructor
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
RecoilStateConfirmQuit *RECOIL_THISCALL RecoilStateConfirmQuit::Constructor()
{
    vftable = (RecoilPtr32)(unsigned int)&g_RecoilStateConfirmQuit_Vtbl;
    m_dialog = 0;
    return this;
}

// Reimplements 0x4158f0: RecoilStateConfirmQuit::OnTryBecomeCurrent
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL RecoilStateConfirmQuit::OnTryBecomeCurrent()
{
    HudUiBackgroundConfirmQuit *dialog =
        (HudUiBackgroundConfirmQuit *)::operator new(sizeof(HudUiBackgroundConfirmQuit));
    if (dialog != 0)
    {
        dialog = dialog->Constructor();
    }
    m_dialog = (RecoilPtr32)(unsigned int)dialog;

    HudUiBackgroundConfirmQuitVirtual *dialogView =
        (HudUiBackgroundConfirmQuitVirtual *)m_dialog;
    dialogView->SetEnabled(1);

    return 1;
}

// Reimplements 0x415960: RecoilStateConfirmQuit::OnDeactivate
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL RecoilStateConfirmQuit::OnDeactivate()
{
    if (m_dialog == 0)
    {
        return;
    }

    zVideo::RunPostprocessOnPrimaryBuffer();

    HudUiBackgroundConfirmQuitVirtual *dialogView =
        (HudUiBackgroundConfirmQuitVirtual *)m_dialog;
    dialogView->SetEnabled(0);

    ((HudUiDialogController *)(unsigned int)m_dialog)->BlitOwnedSurfaceToPrimary();
    zVideo::Dispatch_UnlockPrimarySurfaceState();

    dialogView = (HudUiBackgroundConfirmQuitVirtual *)m_dialog;
    if (dialogView != 0)
    {
        dialogView->ScalarDeletingDestructor(1);
    }

    m_dialog = 0;
    Sleep(1000);
}

// Reimplements 0x415880: RecoilStateConfirmQuit::~RecoilStateConfirmQuit
// (D:\Proj\Battlesport\RecoilStateConfirmQuit.cpp)
RECOIL_NOINLINE RecoilStateConfirmQuit::~RecoilStateConfirmQuit()
{
    vftable = (RecoilPtr32)(unsigned int)&g_RecoilStateConfirmQuit_Vtbl;
    RecoilStateConfirmQuitBaseVtableGuard baseVtableOnExit = {this};

    HudUiBackgroundConfirmQuitVirtual *dialogView =
        (HudUiBackgroundConfirmQuitVirtual *)m_dialog;
    if (dialogView != 0)
    {
        dialogView->SetEnabled(0);

        dialogView = (HudUiBackgroundConfirmQuitVirtual *)m_dialog;
        if (dialogView != 0)
        {
            dialogView->ScalarDeletingDestructor(1);
        }

        m_dialog = 0;
    }
}

// Reimplements 0x415860: RecoilStateConfirmQuit::ScalarDeletingDestructor
// (D:\Proj\Battlesport\RecoilStateConfirmQuit.cpp)
RECOIL_NOINLINE RecoilStateConfirmQuit *RECOIL_THISCALL
RecoilStateConfirmQuit::ScalarDeletingDestructor(unsigned int flags)
{
    this->~RecoilStateConfirmQuit();

    if ((flags & 1u) != 0)
    {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x408ff0: RecoilStateControls::QueueEnter
// (D:\Proj\Battlesport\recoil_state.cpp)
void RECOIL_CDECL RecoilStateControls::QueueEnter()
{
    g_RecoilApp.QueuePushState(&g_RecoilStateControls, 0);
}

// Reimplements 0x407100: HudUiCallback::QueueExitCurrentState
// (D:\Proj\Battlesport\hud.cpp)
void RECOIL_CDECL HudUiCallback::QueueExitCurrentState()
{
    g_RecoilApp.QueueExitCurrentState(0);
}

// Reimplements 0x407110: HudUiCallback::QueueCheatCodeState
// (D:\Proj\Battlesport\hud.cpp)
int RECOIL_CDECL HudUiCallback::QueueCheatCodeState()
{
    g_RecoilApp.QueuePushState(&g_RecoilStateCheatCode, 0);
    return 1;
}

namespace HudCheat {

const int kNanitePanelCheatSentinel = 123456789; // 0x075bcd15
const unsigned int kHudCheatPickup901MessageId = 4096;
const unsigned int kHudCheatRespawnMessageId = 4097;
const unsigned int kHudCheatPickup903MessageId = 4098;
const unsigned int kHudCheatBindCommand36MessageId = 4100;
const unsigned int kHudCheatBindCommand31MessageId = 4101;
const int kHudCheatPickup901TypeId = 901;
const int kHudCheatRespawnPickupTypeId = 902;
const int kHudCheatPickup903TypeId = 903;
const int kHudCheatBindCommand31 = 31;
const int kHudCheatBindCommand36 = 36;
const int kHudCheatLifecycleLocal = 1;
const int kHudCheatLifecycleInactive = 4;
const int kHudCheatMasterTypeSub = 2;
const int kHudCheatMasterTypeHover = 4;
const int kHudCheatMasterTypeAmphib = 5;
const int kHudCheatAltGunTransitionReset = 16;

// Reimplements 0x406af0: HudCheat::ExecuteCommandString
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ExecuteCommandString(CString *commandString)
{
    if (commandString->IsEmpty())
    {
        return 0;
    }

    char *const command = commandString->GetBuffer(1);
    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;

    if (zStr::ContainsCaseInsensitive(command,
                                      zLoc::GetMessageString(kHudCheatPickup901MessageId)) != 0)
    {
        return Pickup::ApplyEffect(kHudCheatPickup901TypeId, 0, saveState);
    }

    if (zStr::ContainsCaseInsensitive(command,
                                      zLoc::GetMessageString(kHudCheatRespawnMessageId)) != 0)
    {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        if (playerState->recentHitValid != 0)
        {
            zEffectAnim::Stop(playerState->recentHitLightHandle);
            playerState->recentHitLightHandle = 0;
            playerState->recentHitValid = 0;
        }

        if (playerState->lifecycleState == kHudCheatLifecycleInactive)
        {
            playerState->lifecycleState = kHudCheatLifecycleLocal;
            zOpt::SetSteeringMode(g_PlayerPrevSteeringMode);
            Player::ApplyCameraState(g_PlayerPrevCameraState);
            Player::ResetMouseControlStateAndRecenterCursor(saveState);
            zEffect_Anim::NodeActionCallback(playerState->destroyedRespawnFxEntry,
                                             playerState->rootNode);
            Player::ResetDamageStateAndTimedHitStatus(saveState);

            const int masterType = saveState->primaryModalState->masterModalData->masterType;
            playerState->aiMode = 0;
            playerState->nextModeSwitchAllowedTime = 0.0f;
            playerState->autoTurnSign = 0;
            playerState->motionInput = 0;
            Player::TransitionToMasterTypeTrack(saveState, 1);
            playerState->primaryGunGateUntilTime = g_Time_AccumulatedTimeSec;

            if (masterType == kHudCheatMasterTypeSub)
            {
                Player::TransitionToMasterTypeAmphib(saveState, 0, 1);
                playerState->primaryGunGateUntilTime = g_Time_AccumulatedTimeSec;
                Player::TransitionToMasterTypeSub(saveState, 0);
            }
            else if (masterType == kHudCheatMasterTypeHover)
            {
                Player::TransitionToMasterTypeHover(saveState, 0);
            }
            else if (masterType == kHudCheatMasterTypeAmphib)
            {
                Player::TransitionToMasterTypeAmphib(saveState, 0, 0);
            }
        }

        playerState->altGunTransitionState = kHudCheatAltGunTransitionReset;
        return Pickup::ApplyEffect(kHudCheatRespawnPickupTypeId, 0, saveState);
    }

    if (zStr::ContainsCaseInsensitive(command,
                                      zLoc::GetMessageString(kHudCheatPickup903MessageId)) != 0)
    {
        return Pickup::ApplyEffect(kHudCheatPickup903TypeId, 0, saveState);
    }

    if (zStr::ContainsCaseInsensitive(command,
                                      zLoc::GetMessageString(kHudCheatBindCommand31MessageId)) != 0)
    {
        zInput::BindMap_Current_SetCommandCallback(
            kHudCheatBindCommand31, (zInputCommandCallbackFn)(HudUi::HandleHotkeyCommand));
    }

    if (zStr::ContainsCaseInsensitive(command,
                                      zLoc::GetMessageString(kHudCheatBindCommand36MessageId)) != 0)
    {
        zInput::BindMap_Current_SetCommandCallback(
            kHudCheatBindCommand36, (zInputCommandCallbackFn)(HudUi::HandleHotkeyCommand));
    }

    return 0;
}

// Reimplements 0x406cf0: HudCheat::ClearNanitePanelCheatSentinel
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_CDECL ClearNanitePanelCheatSentinel()
{
    if (g_GameStateOrMapTable == 0)
    {
        return;
    }

    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);
    if (playerState->nanitePanelLevel == kNanitePanelCheatSentinel)
    {
        playerState->nanitePanelLevel = 0;
    }
}

} // namespace HudCheat

namespace zOpt {

// Reimplements 0x413600: zOpt::ToggleHudTypeForCurrentHwMode
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE int RECOIL_CDECL ToggleHudTypeForCurrentHwMode()
{
    const int currentHudType = GetHudTypeForCurrentHwMode();
    if (currentHudType == ZOPT_HUD_TYPE_STANDARD)
    {
        return SetHudTypeForCurrentHwMode(ZOPT_HUD_TYPE_PERSPECTIVE);
    }
    if (currentHudType == ZOPT_HUD_TYPE_PERSPECTIVE)
    {
        return SetHudTypeForCurrentHwMode(ZOPT_HUD_TYPE_STANDARD);
    }
    return GetHudTypeForCurrentHwMode();
}

} // namespace zOpt

namespace HudLowMeterLoopSound {

// Reimplements 0x439b20: HudLowMeterLoopSound::SetLoopActive
// (D:\Proj\Battlesport\Hud.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SetLoopActive(int enabled)
{
    const int wasActive = g_Hud_LowMeterLoopActive;
    if (enabled == 0)
    {
        if (wasActive != 0)
        {
            g_Hud_LowMeterLoopSample->StopActiveVoicesIfPlaying();
            g_Hud_LowMeterLoopActive = 0;
        }
        return;
    }

    if (wasActive == 0)
    {
        g_Hud_LowMeterLoopSample->PlayA3DSimple(1.0f);
        g_Hud_LowMeterLoopActive = 1;
    }
}

// Reimplements 0x439b70: HudLowMeterLoopSound::Disable
// (D:\Proj\Battlesport\Hud.cpp)
RECOIL_NOINLINE void RECOIL_CDECL Disable()
{
    g_Hud_LowMeterBeepSample->StopActiveVoicesIfPlaying();
    g_Hud_LowMeterLoopSample->StopActiveVoicesIfPlaying();
    g_Hud_LowMeterLoopActive = 0;
}

} // namespace HudLowMeterLoopSound
