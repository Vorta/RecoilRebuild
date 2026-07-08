#include "Battlesport/recoil_state_base.h"

/*
 * Provisional byte-match body for the unresolved 0x407170 / 0x4ccd50 default
 * state table. These minimal virtuals are intentionally separate from
 * RecoilApp_IState default hook bodies.
 */

/**
 * Original helper evidence: the complete destructor has no standalone retail
 * body in the RecoilStateBase default-table check; this inline definition
 * feeds the compiler-emitted scalar-deleting destructor at 0x407170.
 * Purpose: preserve the matched empty destructor shape outside the public
 * header without adding a standalone complete-destructor text symbol.
 */
inline RecoilStateBase::~RecoilStateBase() {}

/**
 * Original helper evidence: no standalone retail function exists; vtable slot 1 in
 * g_RecoilStateBase_Vtbl @ 0x4ccd50 folds to the one-argument no-op body at
 * 0x407150; verified through recoil_state_base_default_table.
 * Purpose: Accept window activation notifications for default states.
 */
void RecoilStateBase::OnWndActivate(
    int
) {}

/**
 * Original helper evidence: no standalone retail function exists; vtable slot 2 in
 * g_RecoilStateBase_Vtbl @ 0x4ccd50 folds to the zero-argument no-op body at
 * 0x404e80; verified through recoil_state_base_default_table and
 * zerror_report_old_noop.
 * Purpose: Provide an empty enter callback for default states.
 */
void RecoilStateBase::OnEnter() {}

/**
 * Original helper evidence: no standalone retail function exists; vtable slot 3 in
 * g_RecoilStateBase_Vtbl @ 0x4ccd50 folds to the return-one body at 0x407130;
 * verified through recoil_state_base_default_table.
 * Purpose: Allow a default state transition to become current.
 */
int RecoilStateBase::OnTryBecomeCurrent() {
    return 1;
}

/**
 * Original helper evidence: no standalone retail function exists; vtable slot 4 in
 * g_RecoilStateBase_Vtbl @ 0x4ccd50 folds to the return-zero body at 0x407140;
 * verified through recoil_state_base_default_table.
 * Purpose: Report that a default state does not request app shutdown.
 */
int RecoilStateBase::OnUpdateShouldQuit() {
    return 0;
}

/**
 * Original helper evidence: no standalone retail function exists; vtable slot 5 in
 * g_RecoilStateBase_Vtbl @ 0x4ccd50 folds to the zero-argument no-op body at
 * 0x404e80; verified through recoil_state_base_default_table and
 * zerror_report_old_noop.
 * Purpose: Provide an empty exit callback for default states.
 */
void RecoilStateBase::OnExit() {}

/**
 * Original helper evidence: no standalone retail function exists; vtable slot 6 in
 * g_RecoilStateBase_Vtbl @ 0x4ccd50 folds to the zero-argument no-op body at
 * 0x404e80; verified through recoil_state_base_default_table and
 * zerror_report_old_noop.
 * Purpose: Provide an empty deactivation callback for default states.
 */
void RecoilStateBase::OnDeactivate() {}

/**
 * Original helper evidence: no standalone retail function exists; vtable slot 7 in
 * g_RecoilStateBase_Vtbl @ 0x4ccd50 folds to the one-argument no-op body at
 * 0x407150; verified through recoil_state_base_default_table.
 * Purpose: Accept suspend notifications for default states.
 */
void RecoilStateBase::OnSuspend(
    int
) {}

/**
 * Original helper evidence: no standalone retail function exists; vtable slot 8 in
 * g_RecoilStateBase_Vtbl @ 0x4ccd50 folds to the one-argument no-op body at
 * 0x407150; verified through recoil_state_base_default_table.
 * Purpose: Accept resume notifications for default states.
 */
void RecoilStateBase::OnResume(
    int
) {}

/**
 * Original helper evidence: no standalone retail function exists; vtable slot 9 in
 * g_RecoilStateBase_Vtbl @ 0x4ccd50 folds to the two-argument return-one body
 * at 0x407160; verified through recoil_state_base_default_table.
 * Purpose: Keep the default idle/dispatch loop active.
 */
int RecoilStateBase::OnIdleOrDispatch(
    unsigned int,
    unsigned int
) {
    return 1;
}
