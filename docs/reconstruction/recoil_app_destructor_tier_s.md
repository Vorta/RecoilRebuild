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
- Accepted source model: no explicit authored `RecoilApp::~RecoilApp` body.
  VC5SP3 emits the retail destructor from the implicit C++ destructor over the
  embedded state members and the MFC/OLE base once `RecoilApp_FmvScript` owns
  the embedded `zFMV_Script::Cleanup` call through an original inline member
  destructor.
- Current byte evidence: `python tools/recoil.py verify vc5 0x42de60
  --build-root build/vc5-final-implicit-recoilapp-dtor-42de60` passes under
  `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs` with zero unmasked mismatches
  after 36 relocation-masked bytes. BN body size is 168 bytes; the VC object
  symbol is 176 bytes with 8 trailing VC NOP bytes trimmed.

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
  `python tools/recoil.py verify vc5 0x42dfa0 --build-root
  build/vc5-verify-recoilapp-dfa0-inline-intro-body-timer` passes under
  `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs` with zero unmasked mismatches
  after COFF relocation masking. BN body size is 194 bytes; the VC object
  symbol is 208 bytes with 14 trailing VC NOP bytes trimmed. The accepted source
  shape keeps `RecoilApp_IntroFmvState` construction as an inline embedded-member
  helper observed in `0x42dfa0`, does not emit a constructor write for the
  BSS-zeroed `m_skipIntroFmv`, and assigns `m_transitionFadeTimer` in the
  constructor body so VC5 writes it after the derived RecoilApp vptr install.
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
  `python tools/recoil.py msvc eh-dump 0x4d5e68 0x4d5f18`.
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
  unwind state. `python tools/recoil.py msvc eh-dump 0x4d5fc0 0x4d5fe8`
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

- `python tools/recoil.py verify vc5 0x42de60` resolves to
  `recoil_app_register_at_exit` and now passes under
  `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs` with zero unmasked mismatches
  after 36 relocation-masked bytes. BN body size is 168 bytes, the VC object
  symbol is 176 bytes, and 8 trailing VC NOP bytes are trimmed. The accepted
  shape comes from removing the explicit root destructor and letting VC5SP3
  synthesize the owner cleanup over embedded members.
- `python tools/recoil.py verify vc5 recoil_app_fmv_state_destructors` covers
  the direct FMV-state destructors 0x42df10, 0x42df50, and 0x42e070. Each now
  passes with zero unmasked mismatches under
  `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs` after 24 relocation-masked bytes.
  BN body size is 62 bytes, the VC object body is 64 bytes, and 2 trailing VC
  NOP bytes are trimmed. The accepted source moves cleanup into the embedded
  `RecoilApp_FmvScript` member destructor and lets the three containing state
  destructors remain implicit.
- `python tools/recoil.py verify vc5 0x42dfa0 --build-root
  build/vc5-final-implicit-recoilapp-dtor-42dfa0` supplies the constructor
  half of the owner byte evidence: zero unmasked mismatches after 60
  relocation-masked bytes. BN body size is 194 bytes, the VC object body is 208
  bytes, and 14 trailing VC NOP bytes are trimmed.
- `python tools/recoil.py verify vc5 0x4f3ca8 --build-root
  build/vc5-final-recoilapp-g_recoilapp-data` compares the authored
  `g_RecoilApp` singleton data symbol `?g_RecoilApp@@3VRecoilApp@@A` against
  BN's `struct RecoilApp g_RecoilApp` at `0x4f3ca8`. The 552-byte BSS record
  passes with zero unmasked data-byte mismatches and no relocation bytes.
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
- `python tools/recoil.py verify vc5 recoil_app_fmv_state_constructors
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
  `python tools/recoil.py verify vc5 0x4428b0` resolves to the local
  `recoil_app_mfc_ole_module_destructor` VC target and passes under
  `vc5_o2_ob1_md_facs` with zero unmasked byte mismatches after 16
  relocation-masked bytes. BN body size and VC object body size are both
  256 bytes.
- The verification layout used for the tier S byte match opts the MFC/OLE
  owner manifests into `RECOILAPP_VC5_STL_STATE_QUEUE_MEMBER`, so VC5SP3 sees
  the state queue as the original `std::deque<RecoilApp_StateQueueItem*>`
  member for this constructor/destructor pair. Native builds and the
  standalone queue-helper VC5 target keep the recovered manual queue owner
  because host STL deque layout is not ABI-compatible with the retail VC5
  storage contract.
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

- `0x442c70 RecoilApp_MfcOleModule::RecoilApp_MfcOleModule` is now tier S.
  `python tools/recoil.py verify vc5 0x442c70` resolves to
  `recoil_app_mfc_ole_module_constructor_s` and passes with zero unmasked byte
  mismatches after 8 relocation-masked bytes. BN body size is 138 bytes, the VC
  object body is 144 bytes, and 6 trailing VC NOP bytes are trimmed.
- The accepted constructor source keeps the current class-shaped
  `RecoilApp_MfcOleModule` owner and opts only this owner manifest into
  `RECOILAPP_VC5_STL_STATE_QUEUE_MEMBER`. That makes VC5 emit the retail deque
  member constructor shape: saved `this` stack byte copied into the queue
  allocator byte at `0x118`, dword zeroing through `0x144`, then the late vptr
  install, `m_currentStateIndex = -1`, and the state-stack `rep stosd`.

## Resolved Source Model

The former blocker was the hand-written `RecoilApp::Destructor()` body over
embedded-state storage. Current source removes that explicit body and lets
VC5SP3 synthesize the owner destructor. Cleanup of the embedded FMV scripts is
source-owned by `RecoilApp_FmvScript::~RecoilApp_FmvScript`, an original inline
member helper observed through the direct FMV-state destructors. This preserves
the real class/member owner model and explains the retail C++ EH cleanup-state
frame without raw storage shells or synthetic guard locals.

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
- Moving the real FMV-state destructor definitions above `RecoilApp::~RecoilApp`
  and marking them `inline` was rejected in production source. It preserved the
  improved 45-mismatch direct-destructor baseline and did not regress the
  accepted FMV-state constructor target, but it made the root
  `0x42de60 RecoilApp::Destructor` comparison worse: 152 unmasked mismatches
  after 42 relocation-masked bytes, BN size 168, VC object size 208, and 3
  trailing VC NOP bytes trimmed. VC5 inlined the wrong cleanup ordering and grew
  the owner destructor, so the definitions were restored after the root
  destructor while keeping the beneficial inline base-state destructor.
- Making the three FMV-state destructors class-body inline definitions in
  `RecoilApp.h` was also rejected. The root `0x42de60` comparison reproduced
  the same 152 unmasked mismatches, 42 relocation-masked bytes, BN size 168,
  VC object size 208, and 3 trimmed VC NOP bytes; the grouped
  `recoil_app_fmv_state_destructors` target stayed at 45 unmasked mismatches for
  0x42df10, 0x42df50, and 0x42e070. This confirms that merely making the real
  virtual destructors visible for inlining does not remove the direct-destructor
  derived-vptr entry store or recover the root sparse cleanup-state order.
- Adding `__declspec(novtable)` to the three concrete FMV-state classes was
  rejected. It reduced the direct FMV destructor target from 45 to 38 unmasked
  mismatches by removing the derived-vptr entry store, but it broke the
  accepted RecoilApp constructor shape: `verify vc5 0x42dfa0` regressed from
  zero to 116 unmasked mismatches, while `verify vc5 0x42de60` stayed at 92.
  The concrete state classes therefore cannot use `novtable` as the source
  model for the retail direct-destructor body.
- Adding `__declspec(novtable)` only to the intermediate `RecoilApp_FmvState`
  base was neutral and rejected. It preserved the accepted
  `verify vc5 0x42dfa0` constructor match at zero unmasked mismatches, but
  `verify vc5 recoil_app_fmv_state_destructors` stayed at 45 unmasked
  mismatches for all three direct FMV destructors and `verify vc5 0x42de60`
  stayed at 92 unmasked mismatches. This proves that suppressing only the
  intermediate-base vtable construction does not remove the derived direct
  destructor entry store or recover the root sparse cleanup-state order.
- Adding an explicit empty class-body `RecoilApp_FmvState::~RecoilApp_FmvState`
  was also neutral and rejected. It preserved the accepted
  `verify vc5 0x42dfa0` constructor match, but left
  `recoil_app_fmv_state_destructors` at 45 mismatches for all three direct
  destructors and left `verify vc5 0x42de60` at 92 mismatches. Making the
  intermediate base destructor explicit therefore does not make VC5 inline the
  FMV cleanup funclets down to the retail `RecoilApp_IState` cleanup shape.
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
  queue entrypoints remains rejected. Although it used the provider header
  directly, deriving a wrapper around `std::deque` made the queue entrypoint
  bodies much larger than retail: `0x443160` grew to 576 bytes versus the
  431-byte retail body under `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs`.
  The accepted owner-only deque member is therefore limited to the
  `RECOILAPP_VC5_STL_STATE_QUEUE_MEMBER` MFC/OLE constructor/destructor
  manifests; the standalone `recoil_app_state_queue` target keeps the recovered
  VC5 deque-shaped ABI mirror with `bool Empty()` and `PushBack(const T&)`
  spelling.
