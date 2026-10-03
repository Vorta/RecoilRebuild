// zWeapon compilation unit after zwep_light.c, inferred from the retail object
// boundary [0x4b25a0, 0x4b2960): its .rdata holds one unreferenced 1.0f
// (0x4d3424) beside zwep_light.c's pooled copy, and the 1998 demos link it
// before zwep_light.c. Those demos place the hit-context getters and mine
// iterator after zwep_light.c instead, so they may form a further object.
// Original filename unresolved; zwep_dmg.c is a provisional name (2026-10-02).

#include "opt_catalog.h"
#include "zwep.h"

#include "Battlesport/game_net.h"
#include "Battlesport/player.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zdi.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalog-mineiteratorcursor
 * @recoil-artifact defines .data recoil:data:0x56bcb0: g_OptCatalog_MineIteratorCursor.
 * BN xrefs: OptCatalog_MineIterator::Begin and
 * OptCatalog_MineIterator::Next.
 * Purpose: cursor for MineIterator_Begin/Next traversal of an entry's active
 * runtime-instance list.
 */
OptCatalogRuntimeInstanceStorage* g_OptCatalog_MineIteratorCursor = 0;
}

namespace
{
    const unsigned int kOptCatalogFlagSkipDamageMaskStamp = 0x200000;

    struct OptCatalogDamageHealthOverlay {
        unsigned char unknown_00[0x7c];
        float health;
    };
} // namespace

namespace CZNode
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-setdamagehitcallback
     * @recoil-artifact defines .text recoil:function:0x4b25a0: CZNode::SetDamageHitCallback
     * @recoil-match byte
     *
     * Purpose: create or reuse a damage handler, install its hit callback, and
     * propagate the handler through the node subtree.
     */
    int __fastcall SetDamageHitCallback(void* context, CZNodePartial* node, void* callback)
    {
        OptCatalogDamageHandlerPartial* handler
            = (OptCatalogDamageHandlerPartial*)(((CZNodeFreeListSlot*)(node))->damageHandler);
        if (handler == 0) {
            handler = (OptCatalogDamageHandlerPartial*)(calloc(1, sizeof(OptCatalogDamageHandlerPartial)));
        } else if (handler->hitContext != 0) {
            return 0;
        }

        handler->hitCallback = context;
        handler->hitContext = callback;
        AssignDamageHandlerRecursiveIfMissing(node, handler);
        CZClass::gwNodeSetHasHitCallback(node, 1);
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-assigndamagehandlerrecursiveifmissing
     * @recoil-artifact defines .text recoil:function:0x4b25f0: CZNode::AssignDamageHandlerRecursiveIfMissing
     * @recoil-match byte
     *
     * Purpose: assign a shared damage handler to nodes in a child-list subtree
     * that do not already own one.
     */
    void __fastcall AssignDamageHandlerRecursiveIfMissing(
        CZNodePartial * node,
        OptCatalogDamageHandlerPartial * handler
    )
    {
        if (((CZNodeFreeListSlot*)(node))->damageHandler != 0) {
            return;
        }

        if (node->listCountB != 0) {
            for (int i = 0; i < node->listCountB; ++i) {
                AssignDamageHandlerRecursiveIfMissing(node->listB[i], handler);
            }
        }

        ((CZNodeFreeListSlot*)(node))->damageHandler = handler;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-cleardamagehandler
     * @recoil-artifact defines .text recoil:function:0x4b2630: CZNode::ClearDamageHandler
     * @recoil-match byte
     *
     * Purpose: detach and free a node subtree's shared damage handler.
     */
    int __fastcall ClearDamageHandler(CZNodePartial * node)
    {
        if (node == 0) {
            return 0;
        }

        OptCatalogDamageHandlerPartial* handler
            = (OptCatalogDamageHandlerPartial*)(((CZNodeFreeListSlot*)(node))->damageHandler);
        if (handler != 0) {
            ClearDamageHandlerRecursive(node, handler);
            if (handler->hitContext != 0) {
                CZClass::gwNodeSetHasHitCallback(node, 0);
            }
            free(handler);
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-cleardamagehandlerrecursive
     * @recoil-artifact defines .text recoil:function:0x4b2670: CZNode::ClearDamageHandlerRecursive
     * @recoil-match byte
     *
     * Purpose: clear a matching shared damage handler through a node subtree.
     */
    void __fastcall ClearDamageHandlerRecursive(CZNodePartial * node, OptCatalogDamageHandlerPartial * handler)
    {
        if (node->listCountB != 0) {
            for (int i = 0; i < node->listCountB; ++i) {
                ClearDamageHandlerRecursive(node->listB[i], handler);
            }
        }

        if (((CZNodeFreeListSlot*)(node))->damageHandler == handler) {
            ((CZNodeFreeListSlot*)(node))->damageHandler = 0;
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-setdamagetimercallback
     * @recoil-artifact defines .text recoil:function:0x4b26b0: CZNode::SetDamageTimerCallback
     * @recoil-match byte
     *
     * Purpose: create or reuse a damage handler, install its timer callback,
     * and propagate the handler through the node subtree.
     */
    int __fastcall SetDamageTimerCallback(void* context, CZNodePartial* node, void* callback)
    {
        OptCatalogDamageHandlerPartial* handler
            = (OptCatalogDamageHandlerPartial*)(((CZNodeFreeListSlot*)(node))->damageHandler);
        if (handler == 0) {
            handler = (OptCatalogDamageHandlerPartial*)(calloc(1, sizeof(OptCatalogDamageHandlerPartial)));
        }

        handler->timerContext = context;
        handler->timerCallback = callback;
        AssignDamageHandlerRecursiveIfMissing(node, handler);
        return 0;
    }
} // namespace CZNode

namespace OptCatalog
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-invokedamagefeedbackandhitcallback
     * @recoil-artifact defines .text recoil:function:0x4b26f0: OptCatalog::InvokeDamageFeedbackAndHitCallback
     * @recoil-match byte
     *
     * Purpose: apply per-hit damage feedback and handler callback state.
     * Behavior: clears current damage context, optionally stamps the damage
     * mask, dispatches health or handler callbacks, captures hit snapshots,
     * selects feedback effects, and counts hits for the tracked owner node.
     */
    int __fastcall InvokeDamageFeedbackAndHitCallback(
        OptCatalogEntryDef * self,
        CZNodePartial * damageOwnerNode,
        zVec3 * sourcePos,
        OptCatalogHitEventPartial * hitEvent,
        float damageAmount
    )
    {
        int result = 0;
        g_OptCatalog_DamageContextKind = 0;
        g_OptCatalog_DamageContextHitEvent = 0;

        if ((self->flags & kOptCatalogFlagSkipDamageMaskStamp) == 0) {
            ApplyDamageMaskStampOnHit(hitEvent);
        }

        OptCatalogDamageHandlerPartial* const handler
            = (OptCatalogDamageHandlerPartial*)(((CZNodeFreeListSlot*)(hitEvent->hitNode))->damageHandler);
        if (handler != 0) {
            if (handler == (OptCatalogDamageHandlerPartial*)(1)) {
                OptCatalogDamageHealthOverlay* const healthOverlay
                    = (OptCatalogDamageHealthOverlay*)(hitEvent->hitNode->callbackContext);
                healthOverlay->health -= damageAmount;
            } else if (handler->hitContext != 0) {
                if (g_OptCatalog_CaptureHitSnapshotEnabled == 1) {
                    g_OptCatalog_CapturedDamageSourcePos = *sourcePos;
                    g_OptCatalog_CapturedDamageHitPos = hitEvent->hitPos;
                }

                g_OptCatalog_CurrentDamageOwnerOrCtx = damageOwnerNode;
                g_OptCatalogDamageFeedbackIntensityScalar = 1.0f;

                OptCatalogDamageFeedbackCallback feedbackCallback
                    = (OptCatalogDamageFeedbackCallback)(g_OptCatalogDamageFeedbackCallback);
                if (feedbackCallback != 0) {
                    feedbackCallback(handler, hitEvent->hitNode, damageAmount);
                }

                OptCatalogHitCallback hitCallback = (OptCatalogHitCallback)(handler->hitContext);
                result = hitCallback(handler->hitCallback, self, hitEvent, damageAmount);

                if (g_OptCatalog_DamageContextKind != 0) {
                    if (self->damageContextEffect != 0) {
                        zEffectAnim::SetTransformRotAndVelocityThunk(
                            self->damageContextEffect,
                            0,
                            hitEvent->hitPos.x,
                            hitEvent->hitPos.y,
                            hitEvent->hitPos.z,
                            0.0f,
                            0.0f,
                            0.0f,
                            0.0f,
                            0.0f,
                            0.0f
                        );
                    }
                } else if (self->damageFeedbackVariantCount != 0) {
                    for (unsigned int i = 0; i < self->damageFeedbackVariantCount; ++i) {
                        if (g_OptCatalogDamageFeedbackIntensityScalar
                            <= self->damageFeedbackVariants[i].minFeedbackScale) {
                            zEffectAnim::SetTransformRotAndVelocityThunk(
                                self->damageFeedbackVariants[i].effect,
                                0,
                                hitEvent->hitPos.x,
                                hitEvent->hitPos.y,
                                hitEvent->hitPos.z,
                                0.0f,
                                0.0f,
                                0.0f,
                                0.0f,
                                0.0f,
                                0.0f
                            );
                            break;
                        }
                    }
                }
            }

            if (g_OptCatalogDamageFeedbackTrackedNode == damageOwnerNode) {
                ++g_OptCatalog_DamageFeedbackHitCount;
            }

            return result;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-capturehitsnapshotandinvokedamagetimercallback
     * @recoil-artifact defines .text recoil:function:0x4b2880: OptCatalog::CaptureHitSnapshotAndInvokeDamageTimerCallback
     * @recoil-match byte
     *
     * Purpose: capture hit positions and forward damage to the timer callback.
     * Behavior: looks up the hit node damage handler, optionally copies source
     * and hit positions to the captured globals, invokes the timer callback,
     * and returns the callback float result.
     */
    float __fastcall CaptureHitSnapshotAndInvokeDamageTimerCallback(
        zVec3 * sourcePos,
        OptCatalogHitEventPartial * hitEvent,
        float damageAmount
    )
    {
        OptCatalogDamageHandlerPartial* handler
            = (OptCatalogDamageHandlerPartial*)(((CZNodeFreeListSlot*)(hitEvent->hitNode))->damageHandler);

        if (g_OptCatalog_CaptureHitSnapshotEnabled == 1) {
            g_OptCatalog_CapturedDamageSourcePos = *sourcePos;
            g_OptCatalog_CapturedDamageHitPos = hitEvent->hitPos;
        }

        OptCatalogDamageTimerCallback callback = (OptCatalogDamageTimerCallback)(handler->timerCallback);
        return callback(handler->timerContext, damageAmount);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-setdamagecontext
     * @recoil-artifact defines .text recoil:function:0x4b28e0: OptCatalog::SetDamageContext
     * @recoil-match byte
     *
     * Purpose: publish the active damage-context kind and optional hit event.
     * Behavior: stores the damage-context kind and captures the hit event only
     * when the event and its hit node are non-null.
     */
    void __fastcall SetDamageContext(int contextKind, OptCatalogHitEventPartial* contextHitEvent)
    {
        if (contextHitEvent != 0 && contextHitEvent->hitNode != 0) {
            g_OptCatalog_DamageContextHitEvent = contextHitEvent;
        }

        g_OptCatalog_DamageContextKind = contextKind;
    }
} // namespace OptCatalog

namespace DamageFeedback
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-setintensityscalar
     * @recoil-artifact defines .text recoil:function:0x4b2900: DamageFeedback::SetIntensityScalar
     * @recoil-match byte
     *
     * Purpose: update the active damage-feedback intensity scalar.
     * Behavior: stores the per-hit damage-feedback intensity scalar used by
     * OptCatalog feedback variant selection.
     */
    void __stdcall SetIntensityScalar(float scalar)
    {
        g_OptCatalogDamageFeedbackIntensityScalar = scalar;
    }
} // namespace DamageFeedback

namespace OptCatalog
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-getcapturedhitsourceptr
     * @recoil-artifact defines .text recoil:function:0x4b2910: OptCatalog::GetCapturedHitSourcePtr
     * @recoil-match byte
     *
     * Purpose: expose the captured damage source vector buffer.
     * Behavior: returns the captured damage source-position global; callers
     * consume the adjacent captured hit-position vector.
     */
    zVec3* __cdecl GetCapturedHitSourcePtr()
    {
        return &g_OptCatalog_CapturedDamageSourcePos;
    }
} // namespace OptCatalog

namespace HitContext
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-getcurrentownerorctx
     * @recoil-artifact defines .text recoil:function:0x4b2920: HitContext::GetCurrentOwnerOrCtx
     * @recoil-match byte
     *
     * Purpose: expose the current OptCatalog damage owner/context pointer.
     * Behavior: returns the current OptCatalog damage owner/context pointer.
     */
    void* __cdecl GetCurrentOwnerOrCtx()
    {
        return g_OptCatalog_CurrentDamageOwnerOrCtx;
    }
} // namespace HitContext

namespace OptCatalog_MineIterator
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-begin
     * @recoil-artifact defines .text recoil:function:0x4b2930: OptCatalog_MineIterator::Begin
     * @recoil-match byte
     *
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * BN behavior: ECX is OptCatalogEntryDef*, load activeRuntimeListHead,
     * store it to g_OptCatalog_MineIteratorCursor, and return the same
     * runtime-instance pointer.
     * Data touch: writes the BSS global g_OptCatalog_MineIteratorCursor
     * at 0x56bcb0.
     * Purpose: start iterating the active runtime-instance list for a mine
     * OptCatalog entry.
     */
    OptCatalogRuntimeInstanceStorage* __fastcall Begin(OptCatalogEntryDef * entry)
    {
        g_OptCatalog_MineIteratorCursor = entry->activeRuntimeListHead;
        return entry->activeRuntimeListHead;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-next
     * @recoil-artifact defines .text recoil:function:0x4b2940: OptCatalog_MineIterator::Next
     * @recoil-match byte
     *
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * BN behavior: read g_OptCatalog_MineIteratorCursor; when non-null,
     * advance through OptCatalogRuntimeInstanceStorage::next, write the new
     * cursor back, and return it; when null, return null without changing the
     * global.
     * Data touch: reads and conditionally writes the BSS global
     * g_OptCatalog_MineIteratorCursor at 0x56bcb0.
     * Purpose: advance the current mine runtime-instance iterator cursor.
     */
    OptCatalogRuntimeInstanceStorage* __fastcall Next(OptCatalogEntryDef * entry)
    {
        OptCatalogRuntimeInstanceStorage* result = g_OptCatalog_MineIteratorCursor;
        if (result != 0) {
            result = result->next;
            g_OptCatalog_MineIteratorCursor = result;
        }

        return result;
    }
} // namespace OptCatalog_MineIterator
