// zWeapon compilation unit after zwep_init.c, inferred from the retail object
// boundary [0x4b2160, 0x4b25a0): its .rdata pooled constants [0x4d3408,
// 0x4d3424) repeat 1.0f, -1.0f and 0.0f that zwep_ammo.c and zwep_init.c pool
// separately, and its thermal glow label (0x4e4658) follows zwep_init.c's
// strings. The 1998 demos link this object after zwep_dmg.c. Original
// filename unresolved; zwep_light.c is a provisional name (2026-10-02).

#include "opt_catalog.h"
#include "zwep.h"

#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "GameZRecoil/zWeapon/zwep_api.h"
#include "zdi.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

RECOIL_STATIC_ASSERT(sizeof(PlayerTimedHitStatus) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, runtimeFlags) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, hitSource) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, currentLevel) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, targetLevel) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, lightNode) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, nextUpdateTime) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, lightParentNode) == 0x18);

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-zweapon-thermalglowlabel
 * @recoil-artifact defines .data recoil:data:0x4e4658: g_zWeapon_ThermalGlowLabel.
 * Purpose: stores the fixed node name assigned to pooled thermal glow
 * lights during initialization.
 * Retail keeps it in writable .data, where VC5 places only non-const
 * arrays, so it is not declared const.
 */
static char g_zWeapon_ThermalGlowLabel[] = "Thermal glow";

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-initthermalglowpool
 * @recoil-artifact defines .text recoil:function:0x4b2160: CZLight::InitThermalGlowPool
 * @recoil-match byte
 *
 * Purpose: allocate the fixed eight-node thermal glow light pool, initialize
 * names, positions, and ranges, then link every node onto the free list.
 */
int __fastcall InitThermalGlowPool(void)
{
    int i;
    for (i = 0; i < 8; ++i) {
        CZNodePartial* const light = gwLightNew();
        gwNodeSetName(light, g_zWeapon_ThermalGlowLabel);
        gwLightSetPosition(light, 0.0f, 0.0f, 0.0f);
        gwLightSetRange(light, 0.1f, 0.2f);
        light->callbackContext = g_OptCatalogThermalGlowFreeList;
        g_OptCatalogThermalGlowFreeList = light;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-playertimedhitstatus-resetfields
 * @recoil-artifact defines .text recoil:function:0x4b21c0: PlayerTimedHitStatusResetFields
 * @recoil-match byte
 *
 * Purpose: clear the active and interpolation flags and reset the timed-hit
 * light, level, and update timer fields.
 */
void __fastcall PlayerTimedHitStatusResetFields(PlayerTimedHitStatus* status)
{
    const unsigned int flags = status->runtimeFlags & ~3u;
    status->lightNode = 0;
    status->currentLevel = 0.0f;
    status->targetLevel = 0.0f;
    status->runtimeFlags = flags;
    status->nextUpdateTime = 0.0f;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-destroythermalglowpool
 * @recoil-artifact defines .text recoil:function:0x4b21e0: CZLight::DestroyThermalGlowPool
 * @recoil-match byte
 *
 * Purpose: delete every thermal glow light still on the free list and clear
 * the pool head.
 */
int __cdecl DestroyThermalGlowPool(void)
{
    CZNodePartial* node = g_OptCatalogThermalGlowFreeList;
    while (node != 0) {
        CZNodePartial* next = node->callbackContext;
        node->callbackContext = 0;
        DeleteNodeByType(node);
        node = next;
    }

    g_OptCatalogThermalGlowFreeList = 0;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-updatetimedstatus
 * @recoil-artifact defines .text recoil:function:0x4b2210: HitSource::UpdateTimedStatus
 * @recoil-match byte
 *
 * Purpose: apply a hit source's timed-status contribution, allocate its
 * status light when needed, and report the current damage band.
 */
int __fastcall UpdateTimedStatus(OptCatalogEntryDef* self, PlayerTimedHitStatus* status, float amount)
{
    status->hitSource = self;
    status->runtimeFlags |= 3u;

    if ((self->flags & 0x200u) != 0) {
        status->targetLevel -= amount;
    } else {
        status->targetLevel += amount;
    }

    if (status->targetLevel > 1.0f) {
        status->targetLevel = 1.0f;
    } else if (status->targetLevel < -1.0f) {
        status->targetLevel = -1.0f;
    }

    if (status->lightNode == 0) {
        CZNodePartial* const light = AllocFromFreeListAndAttach(&self->timedStatusLightSpecularColor);
        status->lightNode = light;
        if (light != 0) {
            AddChild(status->lightParentNode, light);
        }
    }

    if (status->currentLevel < -0.5f) {
        return 2;
    }
    if (status->currentLevel <= 0.0f) {
        return 1;
    }
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-playertimedhitstatus-clearlightandreset
 * @recoil-artifact defines .text recoil:function:0x4b22d0: PlayerTimedHitStatusClearLightAndReset
 * @recoil-match byte
 *
 * Purpose: detach and recycle the active timed-hit light, then reset the
 * status fields.
 */
void __fastcall PlayerTimedHitStatusClearLightAndReset(PlayerTimedHitStatus* status)
{
    if (status->lightNode != 0) {
        RemoveChild(status->lightParentNode, status->lightNode);
        ReturnToFreeList(status->lightNode);
        PlayerTimedHitStatusResetFields(status);
    }
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-playertimedhitstatus-tickandupdatelight
 * @recoil-artifact defines .text recoil:function:0x4b2300: PlayerTimedHitStatusTickAndUpdateLight
 * @recoil-match byte
 *
 * Purpose: advance timed-hit interpolation or decay, update the status light,
 * and return the current damage band.
 */
int __fastcall PlayerTimedHitStatusTickAndUpdateLight(PlayerTimedHitStatus* status, float hitStatus)
{
    OptCatalogEntryDef* const source = status->hitSource;

    if ((status->runtimeFlags & 2u) != 0) {
        const float previousLevel = status->currentLevel;
        const float delta = status->targetLevel - status->currentLevel;
        float lightScale;
        if (fabs(delta) > 0.001) {
            float step = source->timedStatusInterpRate * g_FrameDeltaTimeSec;
            if (1.0f < step) {
                step = 1.0f;
            }

            status->currentLevel += delta * step;
            if (status->currentLevel > 1.0f) {
                status->currentLevel = 1.0f;
            } else if (status->currentLevel < -1.0f) {
                status->currentLevel = -1.0f;
            }
        } else {
            status->runtimeFlags &= ~2u;
            status->currentLevel = status->targetLevel;
        }

        status->nextUpdateTime = source->timedStatusUpdateDelay + g_Time_AccumulatedTimeSec;
        lightScale = (float)(fabs(hitStatus * status->currentLevel));

        if (status->lightNode != 0) {
            gwLightSetRange(
                status->lightNode,
                source->timedStatusLightRangeMin * lightScale,
                source->timedStatusLightRangeMax * lightScale
            );

            if ((previousLevel > 0.0f && status->currentLevel < 0.0f)
                || (previousLevel < 0.0f && status->currentLevel > 0.0f)) {
                gwLightSetSpecularColor(
                    status->lightNode,
                    source->timedStatusLightSpecularColor.red,
                    source->timedStatusLightSpecularColor.green,
                    source->timedStatusLightSpecularColor.blue
                );
            }
        }
    } else if (g_Time_AccumulatedTimeSec >= status->nextUpdateTime) {
        status->currentLevel = ApproxExpNeg(g_FrameDeltaTimeSec * 0.75f) * status->currentLevel;
        status->targetLevel = status->currentLevel;

        if (fabs(status->currentLevel) < 0.001) {
            PlayerTimedHitStatusClearLightAndReset(status);
        } else if (status->lightNode != 0) {
            const float lightScale = (float)(fabs(hitStatus * status->currentLevel));
            gwLightSetRange(
                status->lightNode,
                source->timedStatusLightRangeMin * lightScale,
                source->timedStatusLightRangeMax * lightScale
            );
        }
    }

    if (status->currentLevel < -0.5f) {
        return 2;
    }
    if (status->currentLevel < 0.0f) {
        return 1;
    }
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-allocfromfreelistandattach
 * @recoil-artifact defines .text recoil:function:0x4b2520: CZLight::AllocFromFreeListAndAttach
 * @recoil-match byte
 *
 * Purpose: pop a thermal glow light from the free list, reset its range and
 * specular color, and attach it to the active runtime world.
 */
CZNodePartial* __fastcall AllocFromFreeListAndAttach(zColorRgb* specularColor)
{
    CZNodePartial* light = 0;
    if (g_OptCatalogThermalGlowFreeList != 0) {
        light = g_OptCatalogThermalGlowFreeList;
        g_OptCatalogThermalGlowFreeList = light->callbackContext;
        gwLightSetRange(light, 0.1f, 0.2f);
        gwLightSetSpecularColor(light, specularColor->red, specularColor->green, specularColor->blue);
        AddLight(g_OptCatalogRuntimeWorld, light);
    }

    return light;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-returntofreelist
 * @recoil-artifact defines .text recoil:function:0x4b2570: CZLight::ReturnToFreeList
 * @recoil-match byte
 *
 * Purpose: reset a thermal glow light's range, detach it from the runtime
 * world, and push it back onto the thermal glow free list.
 */
void __fastcall ReturnToFreeList(CZNodePartial* lightNode)
{
    gwLightSetRange(lightNode, 0.1f, 0.2f);
    RemoveLight(g_OptCatalogRuntimeWorld, lightNode);
    lightNode->callbackContext = g_OptCatalogThermalGlowFreeList;
    g_OptCatalogThermalGlowFreeList = lightNode;
}
