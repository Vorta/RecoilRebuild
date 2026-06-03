# Implementation Groups

Use this tracked file for temporary dependency-group notes during
reconstruction. The plan remains address-based; this file lists only active
multi-function, source-readiness, or coherent tier `S` groups currently being
coordinated. Keep the header and template available even when no groups are
active.

## Rules

- Create or update a group before editing when a task touches more than one
  function or a shared type/global/vtable.
- Keep groups scoped. Prefer one class, one source file cluster, one recursive
  cycle, or one call-chain frontier.
- Do not mark plan entries done from this file alone. Plan markers still require
  current source/build/Binary Ninja evidence.
- Keep notes concise and temporary. Move durable facts into source comments,
  Binary Ninja comments, tests, `docs/reconstruction/`, or narrow subsystem docs before
  pruning.
- Stage this file only when an active group update belongs with a qualifying
  source checkpoint under the root `AGENTS.md` git rules. Do not commit stale or
  group-only bookkeeping.
- Verification-only queues that no longer carry source blockers should not live
  in this active working file unless they are coordinating a current coherent
tier `S` pass. Use `.agent/RECOIL_PLAN.md`, `python tools/recoil_status.py
  0xNNNNNN`, VC verification manifests, and
  `python tools/recoil_verification_backlog.py` for current verification state.
- Recompute verification scope with `python tools/recoil_status.py 0xNNNNNN` or
  `python tools/recoil_frontier.py 0xNNNNNN --depth 1` after source blockers
  clear.
- Use `python tools/recoil_groups_audit.py --summary --wip-limit 4` to check
  for stale, completed, or overgrown groups.

## Active Group Template

```text
### Group: short descriptive name

- Anchor: 0xNNNNNN Name
- Reason: dependency closure / class cluster / recursive cycle / shared ABI layout / source file cluster
- Source blockers:
  - 0xNNNNNN Name
- Next action:
  - python tools/recoil_status.py 0xNNNNNN
```

## Active Groups

### Group: RecoilApp owner EH tier S

- Anchor: 0x42de60 RecoilApp::Destructor and 0x42dfa0 RecoilApp::Constructor
- Reason: class cluster / compiler-generated constructor/destructor cleanup-state model
- Source blockers:
  - 0x42de60 RecoilApp::Destructor is tier B but not tier S; BN shows an
    MSVC EH registration frame and cleanup state transitions for embedded
    RecoilApp state destruction, while the current authored source uses a
    manual non-EH `Destructor()` body.
  - 0x42dfa0 RecoilApp::Constructor is tier B but not tier S; BN shows the
    paired MSVC EH registration frame and constructor unwind map. Its former
    owner dependency at 0x442c70
    `RecoilApp_MfcOleModuleOwner::RecoilApp_MfcOleModuleOwner` is tier S under
    `recoil_app_mfc_ole_module_constructor_s`.
- Next action:
  - Recover the smallest source-faithful RecoilApp owner model that lets VC
    emit the paired member/base constructor and destructor cleanup chains
    before retrying byte verification with
    `python tools/recoil_vc6_verify.py 0x42dfa0` and
    `python tools/recoil_vc6_verify.py 0x42de60`.
    The BN EH unwind map has member/base cleanup states 0-4 plus parent-frame
    `IState*` local cleanup states 5-7; rejected probes show `try`/`catch`
    emits the wrong EBP/catch EH shape, a synthetic automatic local emits the
    correct compact EH family but introduces wrong state numbering and an extra
    destructor call, and a VC5 automatic owner destructor with nested or
    inherited IState base destructors adds per-FMV temporary pointer slots plus
    an extra saved register that retail does not have. A raw-member owner probe
    removed those temporaries and improved 0x42de60 to 97 mismatches, but its
    EH states remained `4`, `1`, and `0` where retail uses `5`, `6`, and `7`.
    Layering local IState cleanup guards over that raw-member owner produced
    the desired `5`, `6`, and `7` state writes, but forced extra stack space,
    saved `ebx`, guard-pointer stores, and duplicate vtable resets, regressing
    to 150 mismatches.
    Direct destructor binary-lane dependencies are now tier S:
    `RecoilApp::MfcOleModuleDestructor` at 0x4428b0 passes under the narrow
    `recoil_app_mfc_ole_module_destructor` VC target, and
    `zFMV_Script::Cleanup` at 0x462630 plus `zFMV_Script::Reset` at 0x462660
    pass under the shared `zfmv_script_cleanup_reset` VC target. See
    `docs/reconstruction/recoil_app_destructor_tier_s.md` and
    `docs/reconstruction/zfmv_script_cleanup_reset_verification.md`.
    `zFMV_Script::Init` at 0x4625e0 is tier S. Current VC5 comparisons also
    accept 0x42e220 `RecoilApp::StartEngine`, 0x442bc0
    `RecoilApp::ShutdownSubsystems`, 0x42e430
    `RecoilApp::ShutdownEngine`, 0x42e330
    `RecoilApp::InitializeDisplay`, and 0x4a5780
    `RecoilApp::InitStdLogFiles` as tier S.
    The FMV state constructor cleanup model is now accepted: 0x42eb70
    `RecoilApp_AttractFmvState::Constructor`, 0x42ed30
    `RecoilApp_MissionFmvState::Constructor`, and 0x42eb00
    `RecoilApp_FmvState::OnIdleOrDispatch` pass under
    `recoil_app_fmv_state_constructors` with VC5SP3
    `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs` zero-mismatch evidence.
    `RecoilApp_PlayState::Constructor` at 0x42eea0 is now tier S under
    `recoil_app_register_at_exit` after restoring the constructor shape that
    leaves offsets 0x04-0x0c untouched and clears only offsets 0x10 and 0x14.
    The RecoilApp vtable-order pass also makes 0x4429d0
    `RecoilApp::InitMainWindow` tier S by putting `CreateMainWnd` at the retail
    virtual slot, and keeps 0x42e220 `RecoilApp::StartEngine` plus 0x42e430
    `RecoilApp::ShutdownEngine` tier S with virtual method decorations.
    The recovered state-queue map helper 0x443690
    `RecoilApp_StateQueue::GrowAndCenterChunkBaseList` remains tier S; 0x443700
    `RecoilApp_StateQueueBlock::InitFromCursor` and the queue entrypoints
    remain tier B because current clean C++ spelling is source-equivalent but
    still differs in stack/register scheduling.
    Remaining current VC5 blockers in the owner cluster are 0x42dfa0
    `RecoilApp::Constructor` and 0x42de60 `RecoilApp::Destructor`, both still
    requiring the source-faithful owner/EH model rather than queue or vtable
    cleanup.

### Group: zSnd sample initialization tier S

- Anchor: 0x4a2ea0 zSndSample::InitFromWaveData
- Reason: source file cluster / backend initialization dependency closure
- Source blockers:
  - none visible; DirectSound and A3D backend paths are tier B and source-ready
- Next action:
  - Continue binary-lane shaping from the current closest baselines:
    0x4a2ea0 dispatcher is tier S with VC5 `vc5_o2_ob0_facs` zero-mismatch
    evidence, 0x4a3180 DirectSound is VC6 `vc6_o2_ob0_facs` at 543
    mismatches, and 0x4a2ec0 A3D is tier S with VC5 `vc5_o2_ob0_md_facs`
    zero-mismatch evidence. DirectSound tier S remains the source-cluster
    binary-lane blocker; see
    `docs/reconstruction/zsnd_sample_init_verification.md` for the current
    profile evidence and rejected probes.
