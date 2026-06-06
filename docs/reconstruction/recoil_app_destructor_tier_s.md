# RecoilApp Destructor Tier S Notes

## 0x42de60 RecoilApp::Destructor

- Current source: `src/Battlesport/RecoilApp.cpp`.
- Current functional target: `tools/functional_verify_targets/recoil_app_destructor.json`.
- Current VC target: `tools/vc5_verify_targets/recoil_app_register_at_exit.json`.
- Binary Ninja evidence: the retail body at `0x42de60` installs an MSVC C++
  EH registration frame, pushes `MsvcEh::RecoilApp_Dtor_EhFrameHandlerThunk`
  at `0x4c9d78`, tracks cleanup state values `5`, `-1`, `6`, and `7`, then
  restores the previous `fs:[0]` registration before returning.
- The main body resets embedded state vtables in reverse construction order,
  calls `zFMV_Script::Cleanup` for mission, intro, and attract FMV states, and
  finishes with `RecoilApp_MfcOleModule::Destructor`.

## 0x42dfa0 RecoilApp::Constructor

- Current source: `src/Battlesport/RecoilApp.cpp`.
- Current functional target: `tools/functional_verify_targets/recoil_app_constructor.json`.
- Current VC target: `tools/vc5_verify_targets/recoil_app_register_at_exit.json`.
- Binary Ninja evidence: the retail body at `0x42dfa0` installs the paired
  MSVC C++ EH registration frame, pushes
  `MsvcEh::RecoilApp_Ctor_EhFrameHandlerThunk` at `0x4c9e26`, constructs the
  MFC/OLE base, attract FMV state, intro FMV script, mission FMV state, play
  state, MP exit state vtable, RecoilApp vtable, and transition timer.
- Current byte evidence:
  `python tools/recoil_vc5_verify.py 0x42dfa0 --build-root
  build/vc5-verify-smoke-42dfa0-owner-call` fails under
  `vc5_o2_ob1_facs` with 164 unmasked mismatches after 20 relocation-masked
  bytes. BN body size is 194 bytes and the VC object symbol is 128 bytes.
- The former constructor owner dependency, 0x442c70
  `RecoilApp_MfcOleModuleOwner::RecoilApp_MfcOleModuleOwner`, is now tier S
  under `recoil_app_mfc_ole_module_constructor_s`; the accepted VC5SP3
  `vc5_o2_ob1_facs` comparison has zero unmasked mismatches after relocation
  masking. The FMV constructor callees
  0x42eb70 `RecoilApp_AttractFmvState::Constructor` and 0x42ed30
  `RecoilApp_MissionFmvState::Constructor` are tier S under
  `recoil_app_fmv_state_constructors`; the shared zFMV dependency
  `0x4625e0 zFMV_Script::Init` is tier S under `vc5_o2_ob0_facs`.

## EH Unwind Evidence

- Binary Ninja types the destructor EH record at `0x4d5e68` as
  `g_RecoilApp_Dtor_EhFuncInfo`. Its VC5SP3 FuncInfo header has magic
  `0x19930520`, `maxState == 8`, and unwind map pointer `0x4d5e88`.
- Local evidence command:
  `python tools/recoil_msvc_eh_dump.py 0x4d5e68 0x4d5f18`.
- Destructor unwind map entries:
  - state 0 -> -1: `0x4c9d20`
    `MsvcEh::RecoilApp_Dtor_EhCleanup_MfcOleModule`, tail-calls
    `0x4428b0 RecoilApp::MfcOleModuleDestructor` on parent-frame `this`.
  - state 1 -> 0: `0x4c9d28`
    `MsvcEh::RecoilApp_Dtor_EhCleanup_AttractFmvState`, destructs
    `this + 0x160` through `0x42df10`.
  - state 2 -> 1: `0x4c9d36`
    `MsvcEh::RecoilApp_Dtor_EhCleanup_IntroFmvState`, destructs
    `this + 0x1a0` through `0x42df50`.
  - state 3 -> 2: `0x4c9d44`
    `MsvcEh::RecoilApp_Dtor_EhCleanup_MainMenuPrepState`, destructs
    `this + 0x1c8` through `0x42df90`.
  - state 4 -> 3: `0x4c9d52`
    `MsvcEh::RecoilApp_Dtor_EhCleanup_LeaveNetworkState`, destructs
    `this + 0x1d0` through `0x42df90`.
  - state 5 -> -1: `0x4c9d60`
    `MsvcEh::RecoilApp_Dtor_EhCleanup_IStateLocal0`, destructs the
    parent-frame `IState*` at `EBP-0x0c` through `0x42df90`.
  - state 6 -> 1: `0x4c9d68`
    `MsvcEh::RecoilApp_Dtor_EhCleanup_IStateLocal1`, destructs the
    parent-frame `IState*` at `EBP-0x18` through `0x42df90`.
  - state 7 -> 0: `0x4c9d70`
    `MsvcEh::RecoilApp_Dtor_EhCleanup_IStateLocal2`, destructs the
    parent-frame `IState*` at `EBP-0x1c` through `0x42df90`.
- The shared parent-frame `this` slot used by the member/base cleanup funclets
  is `EBP-0x10`. The three `IState*` cleanup locals are distinct parent-frame
  slots at `EBP-0x0c`, `EBP-0x18`, and `EBP-0x1c`, so they are not just BN
  renderings of the same owner pointer or member offsets.
- The matching constructor EH record at `0x4d5f18` has magic `0x19930520`,
  `maxState == 7`, unwind map pointer `0x4d5f38`, no try blocks, no IP map,
  and no ES type list. Its decoded unwind transitions are:
  - state 0 -> -1: `0x4c9dd0`
  - state 1 -> 0: `0x4c9dd8`
  - state 2 -> 1: `0x4c9de6`
    `MsvcEh::RecoilApp_Ctor_EhCleanup_IStateLocal0`, destructs the
    parent-frame `IState*` at `EBP-0x10` through `0x42df90`.
  - state 3 -> 1: `0x4c9dee`
  - state 4 -> 3: `0x4c9dfc`
  - state 5 -> 4: `0x4c9e0a`
  - state 6 -> 5: `0x4c9e18`
  Constructor member/base cleanup funclets use the parent-frame `this` slot at
  `EBP-0x14`, while the local `IState*` cleanup state uses `EBP-0x10`.
  The cleanup order is MFC/OLE base, attract FMV state, one parent-frame
  `IState*` local, intro FMV state, main-menu prep state, leave-network state,
  and mission FMV state.
- The attract and mission FMV state constructors each have one constructor
  unwind state. `python tools/recoil_msvc_eh_dump.py 0x4d5fc0 0x4d5fe8`
  decodes `0x4d5fc0` and `0x4d5fe8` with magic `0x19930520`,
  `maxState == 1`, no try/IP/ES maps, and cleanup actions `0x4c9e70` and
  `0x4c9e90`. Both action funclets load the parent-frame
  `RecoilApp_IState*` from `EBP-0x10` and tail-call
  `0x42df90 RecoilApp_IState::Destructor`. This proves the FMV constructor EH
  debt is the base-state constructor cleanup model, not the
  `zFMV_Script::Init` call itself.
- The accepted source model for those FMV state constructors uses real C++
  constructor symbols for the retail bodies and an inline `RecoilApp_IState`
  base-member constructor. This makes VC5SP3 emit the base vtable store before
  the three null `zFMV_Script::Init` arguments and preserves the one-state
  cleanup funclet shape.

## Current Byte Evidence

- `python tools/recoil_vc5_verify.py 0x42de60` resolves to
  `recoil_app_register_at_exit` and fails under the manifest's
  `vc5_o2_ob0_facs` profile with 146 unmasked mismatches after 16
  relocation-masked bytes. BN body size is 168 bytes, the VC object symbol is
  96 bytes, and 2 trailing VC NOP bytes are trimmed.
- A focused profile sweep over `vc5_o2_ob0_facs`, `vc5_o2_ob1_gx_facs`,
  `vc5_o2_ob1_md_gx_facs`, and `vc5_o2_ob2_gx_facs` produced the same
  146-mismatch, 96-byte non-EH object body. `Mfc42Abi.h` now guards the
  `LVBKIMAGEA` and `COLORSCHEME` provider declarations against VC5SP3 SDK header
  definitions; VC5SP3 profiles compile the MFC/OLE constructor path but do not
  improve the 0x442c70 baseline shape.
- The byte drift starts at function entry: the retail function begins with the
  EH registration prologue (`push -1`, frame-handler thunk, `fs:[0]` chain),
  while the authored object starts with the plain manual destructor prologue
  (`push esi`, `mov esi, ecx`, `push edi`).
- `python tools/recoil_vc5_verify.py recoil_app_fmv_state_constructors
  --build-root build/vc5-verify-final-recoil-app-fmv-state-constructors`
  passes with zero unmasked byte mismatches for 0x42eb70
  `RecoilApp_AttractFmvState::Constructor`, 0x42ed30
  `RecoilApp_MissionFmvState::Constructor`, and 0x42eb00
  `RecoilApp_FmvState::OnIdleOrDispatch` under VC5SP3
  `vc5_o2_ob1_gx_facs`.

## 0x4428b0 MFC/OLE Module Destructor Dependency

- `0x4428b0 RecoilApp::MfcOleModuleDestructor` is the final direct callee in
  the main destructor and is tier S. Its dependency frontier has no visible
  binary blockers: `operator delete` is an import/provider thunk and
  `CWinApp::~CWinApp` is the MFC provider chain.
- Current tier S evidence:
  `python tools/recoil_vc5_verify.py 0x4428b0 --build-root
  build/vc5-verify-final-4428b0-tier-s` resolves to the local
  `recoil_app_mfc_ole_module_destructor` VC target and passes under
  `vc5_o2_ob1_facs` with zero unmasked byte mismatches after 12
  relocation-masked bytes. BN body size and VC object body size are both
  256 bytes.
- The verification layout used for the tier S byte match uses the VC5SP3 STL `std::deque<RecoilPtr32>`
  destructor only under the 32-bit VC5 verification guard, with a static size
  check against `RecoilApp_StateQueue`. Native builds keep the recovered manual
  queue teardown because host STL deque layout is not ABI-compatible with the
  retail VC5 storage contract.
- The queue record matches the VC5SP3 STL `deque` layout in
  `D:\Recoil Project\Compiler\VC5SP3\VC\INCLUDE\DEQUE`: allocator/pad,
  `_First` iterator, `_Last` iterator, `_Map`, `_Mapsize`, and `_Size`.
  Retail `0x4428b0` closely follows VC5 `~deque() { while (!empty())
  pop_front(); }`, with `_First._Next++`, `--_Size`, `_Freefront()`,
  `iterator()` zero construction, `iterator(*_First._Map, _First._Map)`, and
  `_Freemap()` deallocation all visible in the assembly.
- Previous manual-source drift was structural: retail held the count in `eax`,
  tested the first empty flag in `cl`, saved `edi`/`ebp`/`ebx`, and carried the
  loop cursor in `ebx` with `ebp == 4`. The VC5 deque overlay with `/Ob1`
  emits that saved-register loop shape exactly.

## 0x442c70 MFC/OLE Module Constructor Dependency

- `0x442c70 RecoilApp_MfcOleModuleOwner::RecoilApp_MfcOleModuleOwner` is
  tier S. Current tier S evidence:
  `python tools/recoil_vc5_verify.py
  recoil_app_mfc_ole_module_constructor_s --build-root
  build/vc5-verify-final-recoil-app-mfc-ole-module-constructor-s` passes
  under `vc5_o2_ob1_facs` with zero unmasked byte mismatches after 4
  relocation-masked bytes. BN body size is 138 bytes, VC object size is
  144 bytes, and 6 trailing VC NOP bytes are trimmed.
- The verification layout used for the tier S byte match uses a VC5-only `RecoilApp_MfcOleModuleOwner` with a
  real `CWinApp` provider subobject at offset zero, an explicit pad through
  offset `0x0c0`, the recovered Recoil-owned fields beginning at offset
  `0x0c4`, and a VC5SP3 `std::deque<RecoilPtr32>` member at offset `0x118`.
  `RecoilApp::Constructor` calls that owner constructor directly in the VC5
  verification path; native builds keep the existing ABI mirror and manual
  queue initialization because host STL deque storage is not compatible with
  the retail VC5 layout.

## Source-Model Blocker

The current authored implementation is behavior-correct tier C but models the
owner teardown as a hand-written `RecoilApp::Destructor()` body over raw
embedded-state storage. The retail body has the shape of an MSVC-generated C++
destructor cleanup chain for embedded member/base destruction. Tier S likely
requires recovering the RecoilApp owner boundary so the compiler sees the
embedded state destructors and final MFC/OLE base destructor as real C++
destructor cleanup, rather than attempting more isolated edits to the manual
body.

The full destructor unwind map narrows that blocker: states 0-4 are ordinary
member/base cleanups from parent-frame `this`, but states 5-7 are separate
parent-frame `IState*` locals. That pattern is not explained by the current
manual destructor body or by merely enabling C++ EH on the function. A credible
next source-model probe should account for those three pointer-local cleanups
without adding unrelated synthetic locals or catch handling.

## Rejected Probes

- Wrapping the current manual destructor body in `try { ... } catch (...) {
  throw; }` and compiling with `vc5_o2_ob1_gx_facs` produced an EH-framed
  object body but did not match the retail destructor. The result was 121
  unmasked mismatches after 41 relocation-masked bytes, BN size 168, and VC
  object size 176. The generated body used an EBP-based local EH record and a
  catch-oriented handler shape, while the retail body uses the compact
  destructor cleanup-state frame with `MsvcEh::RecoilApp_Dtor_EhFrameHandlerThunk`.
  The probe was reverted.
- Adding a synthetic automatic local object with a non-inline destructor to the
  current manual body and compiling with `vc5_o2_ob1_gx_facs` did produce the
  compact no-EBP EH prologue family. Declaring it at function entry failed with
  117 unmasked mismatches after 36 relocation-masked bytes, BN size 168, VC
  object size 160, and 3 trailing VC NOPs trimmed. Declaring it just before the
  mission FMV cleanup failed with 118 unmasked mismatches, the same 36
  relocation-masked bytes, VC object size 160, and 5 trailing VC NOPs trimmed.
  These probes confirm that the needed EH family comes from real C++ destructor
  cleanup objects, but a synthetic local introduces the wrong state numbering,
  extra local storage, and a final synthetic destructor call. Both probes were
  reverted.
- Modeling the three observed parent-frame `IState*` cleanup locals as scoped
  guards around the mission, intro, and attract `zFMV_Script::Cleanup` calls
  was rejected. Without `/GX`, `vc5_o2_ob0_facs` improved only slightly to 138
  unmasked mismatches after 28 relocation-masked bytes, with a 144-byte object
  body and no retail EH registration frame. With `vc5_o2_ob1_gx_facs`, VC
  emitted EH but numbered those locals as states 0, 1, and 2, saved an extra
  register, and grew to 170 unmasked mismatches with a 224-byte object body.
  This confirms the three parent-frame locals are real evidence, but they must
  live inside the larger owner/member cleanup model that supplies the earlier
  states.
- A temporary VC5-only actual C++ destructor overlay for `~RecoilApp` and the
  embedded state destructors got closer to the retail owner-cleanup shape but
  still failed. Out-of-line subobject destructors under `vc5_o2_ob1_gx_facs`
  produced 108 unmasked mismatches after 44 relocation-masked bytes, with a
  176-byte object body. The generated root destructor had the right EH family
  and state range, but called subobject destructors out of line, omitted the
  final MFC/OLE base destructor, and used a different member cleanup order.
  The generated `RecoilApp::Destructor` symbol was a jump wrapper into
  `??1RecoilApp@@QAE@XZ`; the C++ destructor normal path counted down through
  member destructor states `5`, `4`, `3`, `2`, `1`, and `0`, unlike retail's
  sparse normal-path writes `5`, `-1`, `6`, `7`, and `-1`.
  Forcing those probe destructors inline worsened the comparison to 172
  unmasked mismatches with a 240-byte object body under `vc5_o2_ob1_gx_facs`
  and 130 mismatches with a 176-byte body under `vc5_o2_ob2_gx_facs`. The
  probe source and local manifest were reverted.
- A follow-up VC5 owner-destructor probe after the accepted 0x442c70
  MFC/OLE constructor model used a raw 0x148-byte MFC/OLE base cleanup owner,
  inline FMV member destructors, inline IState vtable resets, and an empty
  `RecoilApp_OwnerForDestructor::~RecoilApp_OwnerForDestructor` body. It was
  rejected. Under `vc5_o2_ob1_gx_facs`, the comparison failed with 143
  unmasked mismatches after 28 relocation-masked bytes, BN size 168, VC object
  size 192, and 11 trailing VC NOPs trimmed. The body emitted the right compact
  EH family, but added `sub esp, 8`, saved `ebx`, and stored a temporary
  pointer before each FMV cleanup so EH could run the inlined member base
  destructor. Retail keeps only the parent `this` slot, uses `edi` for
  the base `RecoilApp_IState` vtable constant, and has no per-FMV temporary pointer slots in the
  normal body. A profile sweep of the same source shape also failed:
  `vc5_o2_ob0_facs` produced 128 mismatches with a 96-byte non-EH body,
  `vc5_o2_ob1_facs` returned to the 146-mismatch non-EH baseline, and
  `vc5_o2_ob2_gx_facs` grew to 380 mismatches with a 416-byte body. The probe
  source was reverted; any next source-model attempt should preserve the
  manual normal-path shape while explaining the compiler cleanup-state frame
  and parent-frame `IState*` locals.
- A raw-member variant of the same owner probe removed nested IState base
  destructors from the FMV member shells and kept direct vtable fields at
  offset zero. This was the closest owner-generated normal path so far:
  `vc5_o2_ob1_gx_facs` failed with 97 unmasked mismatches after 32
  relocation-masked bytes, BN size 168, and VC object size 160. The prologue,
  parent `this` slot, `edi` vtable constant, and absence of per-FMV temporary
  pointer slots matched the retail shape. Remaining drift was structural:
  VC emitted `lea` plus EH state `4` before the mission cleanup and states `1`
  and `0` before intro/attract cleanup, while retail stores MP/play vtables
  first and uses states `5`, `6`, and `7`. Converting those raw member shells
  to inherit from an IState base at offset zero regressed to the previous
  temp-slot failure: `vc5_o2_ob1_gx_facs` returned to 143 mismatches, 28
  relocation-masked bytes, BN size 168, VC object size 192, and 11 trailing VC
  NOPs trimmed. Both probe source variants were reverted.
- Combining the raw-member owner shell with scoped local IState cleanup guards
  around mission, intro, and attract FMV cleanup was also rejected. This source
  shape did make VC5 write EH states `5`, `6`, and `7` before the three
  cleanup calls, confirming those retail states can come from local guard
  lifetimes layered over an owner destructor. The generated body failed under
  `vc5_o2_ob1_gx_facs` with 150 unmasked mismatches after 24
  relocation-masked bytes, BN size 168, VC object size 208, and 14 trailing VC
  NOPs trimmed. The guard objects forced `sub esp, 8`, saved `ebx`, stored the
  guard pointer in a stack slot before every cleanup call, loaded the vtable
  constant late, and duplicated MP/play/leave/main vtable resets after the
  attract cleanup. Retail has no guard-pointer stores in the normal body and
  keeps the vtable constant in `edi` from the start. The probe source and local
  manifest were reverted.
- Before the accepted 0x442c70 owner-constructor model, the flat-mirror
  placement construction source stayed at 113 unmasked mismatches, 8
  relocation-masked bytes, BN size 138, and VC object size 112. Qualifying the
  placement expression as global placement new still emitted an allocation
  helper, changed it from `CObject::operator new(size, p)` to global
  `operator new(size, p)`, and worsened to 129 unmasked mismatches. An
  explicit VC5-accepted `CWinApp::CWinApp` call removed the placement helper
  and emitted the direct provider constructor call, but worsened to 127
  unmasked mismatches with a 96-byte object body. Keeping a saved `this` value
  live across that direct call recovered the retail stack-byte reload exactly
  and improved the direct call variant to 116 unmasked mismatches, but still
  missed the retail `xor edi; push edi` and queue-zeroing shape. Rewriting the
  queue clear as explicit field assignments reproduced the retail dword zero
  stores but inserted an extra saved-`this` store before the constructor call,
  grew the VC object body to 144 bytes, and worsened to 127 unmasked
  mismatches. The accepted VC5 owner-constructor model supersedes those
  isolated flat-mirror probes.
- For `0x4428b0`, forcing the vtable store through a volatile lvalue did not
  prevent VC5SP3 from hoisting the queue-count load before the vtable store and
  remained at 233 unmasked mismatches, 12 relocation-masked bytes, and a
  224-byte VC object body. The probe was reverted.
- Rewriting the queue loop with an `int` empty flag improved the comparison to
  221 unmasked mismatches but still folded the empty checks into direct
  branches and shortened the VC object body to 208 bytes. Adding `register`
  hints to the retained `bool` model was neutral at 212 mismatches. These probes
  were reverted.
- Rewriting the queue block clear as explicit field assignments made the VC
  object body length match the 256-byte retail body and reproduced one branch
  length, but worsened the comparison to 224 unmasked mismatches by moving the
  prologue to `push ebx; push esi` and changing the zero comparisons to
  `cmp ..., ebx`. The source was reverted to the `memset` block clears.
- Modeling `0x4428b0` with explicit `RecoilApp_StateQueueBlock *` read/write
  block pointers worsened the comparison to 236 unmasked mismatches and
  shortened the VC object body to 208 bytes. Modeling the iterator reset paths
  as whole-record block assignments was closer but still worse than retained
  source at 205 unmasked mismatches under both `vc5_o2_ob0_facs` and
  `vc5_o2_ob1_facs`. Both probes were reverted.
- Adding inline `RecoilApp_StateQueue` owner methods for `IsEmpty`,
  `PopFront`, `FreeFront`, and `DestroyDeque`, then calling
  `m_stateQueue_118.DestroyDeque()` from `0x4428b0`, was also rejected. Under
  the manifest `vc5_o2_ob0_facs` profile the body collapsed to a 32-byte
  call-based wrapper with 245 unmasked mismatches; under `vc5_o2_ob1_facs`,
  `vc5_o2_ob1_gx_facs`, `vc5_o2_ob2_facs`, and `vc5_o2_ob2_gx_facs`, the
  inline form produced the correct 240-byte body size but still worsened to
  209 unmasked mismatches. This confirms that simply naming the deque owner
  operations is not enough to reproduce the retail STL destructor/pop-front
  lowering.
- A guarded VC5 `std::deque<RecoilApp_StateQueueItem *>` overlay for the
  queue entrypoints was rejected too. Although it used the provider header
  directly, deriving a wrapper around `std::deque` made the queue entrypoint
  bodies much larger than retail: `0x443160` grew to 576 bytes versus the
  431-byte retail body under `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs`.
  The closer entrypoint source model remains the recovered VC5 deque-shaped
  ABI mirror with `bool Empty()` and `PushBack(const T&)` spelling.
