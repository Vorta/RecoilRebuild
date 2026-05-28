#include "Battlesport/hud.h"

#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zSound/zSound.h"

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

struct HudUiBackgroundConfirmQuitVirtual
{
    virtual void RECOIL_THISCALL Update(float deltaSeconds) = 0;
    virtual void RECOIL_THISCALL SetEnabled(int enabled) = 0;
    virtual HudUiBackgroundConfirmQuitVirtual *RECOIL_THISCALL
    ScalarDeletingDestructor(unsigned int flags) = 0;
};

RecoilApp_IState_Vtbl g_RecoilStateConfirmQuit_Vtbl = {0};

struct RecoilStateConfirmQuitBaseVtableGuard
{
    RecoilStateConfirmQuit *self;

    ~RecoilStateConfirmQuitBaseVtableGuard()
    {
        self->vftable = kRecoilStateBase_VtblAddress;
    }
};
} // namespace

const HudUiCommon_FTable g_HudWeatherFx_Vtable = MakeHudWeatherFxFTable();
const HudUiCommon_FTable g_HudWeatherFxSnow_Vtable = MakeHudWeatherFxFTable();
const HudUiCommon_FTable g_HudWeatherFxRain_Vtable = MakeHudWeatherFxFTable();

// Reimplements 0x4bdc70: HudWeatherFx::Constructor
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE HudWeatherFx *RECOIL_THISCALL HudWeatherFx::Constructor(int newParticleCount)
{
    HudUiElement::Constructor(0, 0);
    viewportRect = 0;
    ftable = &g_HudWeatherFx_Vtable;
    maxParticles = newParticleCount;
    particleCount = newParticleCount;
    particleQuads = static_cast<HudWeatherFxParticleQuad *>(
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
        static_cast<zVec3 *>(::operator new(positionBytes));
    particlePositions[destBufferIndex] =
        static_cast<zVec3 *>(::operator new(positionBytes));

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
        char *const alphaMap = static_cast<char *>(malloc(0x80));
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
        kHudWeatherFxDepthBase - static_cast<float>(rand()) * kHudWeatherFxDepthRandScale;

    sourcePosition->x =
        kHudWeatherFxConeBase - static_cast<float>(rand()) * kHudWeatherFxConeRandScale;
    if (sourcePosition->x < -sourcePosition->z)
    {
        sourcePosition->x += kHudWeatherFxReflectBias;
        sourcePosition->z = kHudWeatherFxReflectBias - sourcePosition->z;
    }

    sourcePosition->y =
        kHudWeatherFxConeBase - static_cast<float>(rand()) * kHudWeatherFxConeRandScale;
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

// Reimplements 0x40d1c0: HudUiOptionsPanelOverlayOwner::QueueEnter
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
void RECOIL_CDECL HudUiOptionsPanelOverlayOwner::QueueEnter()
{
    g_RecoilApp.QueuePushState(&g_HudUiOptionsPanelOverlayOwner, 0);
}

// Reimplements 0x4159b0: RecoilStateConfirmQuit::QueueEnter
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
void RECOIL_CDECL RecoilStateConfirmQuit::QueueEnter()
{
    g_RecoilApp.QueuePushState(&g_RecoilState_ConfirmQuit, 0);
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

// Reimplements 0x407110: HudUiCallback::QueueCheatCodeState
// (D:\Proj\Battlesport\hud.cpp)
int RECOIL_CDECL HudUiCallback::QueueCheatCodeState()
{
    g_RecoilApp.QueuePushState(&g_RecoilStateCheatCode, 0);
    return 1;
}

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
