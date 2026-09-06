# UI native construction and call inventory

Fresh `call-contract-zui-labels-g108-01` completes all 259 bodies and reports
ten divergences. Restoring the narrow private-label normalizers fixes the
verifier's NameError (WSI-20260906-020), not those source differences.

## Four-row message stacks

Retail `0x4bd020` and `0x4bd2d0` call the container constructor and VC5 EH
array constructor directly, initialize four 0x2a4-byte panels at +0x10,
write the derived table, and configure the rows. The allocation sites at
`0x40f8bd`/`0x40f8ee` allocate 0xaa0 bytes and call those constructors at
`0x40f8d7`/`0x40f908`. Their EH state transitions and member construction
support ordinary native constructors, not a public initializer invoking a
second implicit constructor.

The existing class family, inheritance, member layout, globals, and source
locations are retained. `Constructor()` is replaced with the actual C++
constructor, and the allocation sites use ordinary `new`. The row loops
follow retail's y-coordinate termination. Chat colors are assigned after
AddChild, matching retail. The claimed original inline configuration helper
is removed: retail proves the expanded operations but not that helper's
historical existence. Exact original class spelling and TU placement are
not established by this edit.

The initial native version passes 259/259 in
`zui-native-message-constructors-order-01`. Direct byte diagnostics still
show register and loop differences, so no byte equality is claimed. The
subsequent coordinate-loop refinement passes fresh live order at revision
5305: 259/259 rows across both UI blocks. The allocation-site HUD target
also passes its separate 407/407 order diagnostic. No byte equality follows
from those order results.

Generation 111 removes the 225-line message-stack wrapper expansion and
its invocation from the verifier (WSI-20260906-023). The verifier must
compare the actual physical constructor calls, not replace one call with
two expected leaves. The 65-case generic proof kernel passes.

## ZRD label resolver

All eight label-construction occurrences in retail `0x4b59f0` invoke the
message resolver directly on the ZRD string and pass its return straight
to the virtual text formatter. Current source added null guards around
both input and output. Those guards are removed from the existing shared
label expansion; this does not change the provider or class model.

The previous candidate's sole extra raw call in this body was an out-of-line
vector `size()` inside the final expanded insertion. Whether the simplified
source restores VC5's retail inlining must be determined by the next fresh
comparison, not assumed from the semantic correction.

No UI owner, data, provider ABI, authored-byte, or linked-image acceptance
is asserted by this audit.

## Lifecycle classification review

The fresh order gate found 56 historical rows with generic `authored` class
and no order role, contrary to the registered lifecycle entries. Each row's
current retail assembly was inspected for initialization, table writes,
base/member construction, resource cleanup, or destructor tail behavior.
The reviewed batch at revision 5304 changes only these classifications to
`authored-lifecycle` / `authored-lifecycle-body`; every row remains gating.
The font-style initializer/clear pair is supplied to the background's EH
array constructor/destructor (references 0x4b95c3/0x4b95c8/0x4b97cd).
The snow destructor tail is called from its separate deleting destructor
at 0x4be2c3. Neither body is reclassified as a provider merely for being small.

Reviewed rows (current source spelling is not original-name proof):

- `0x4b3d00`: `??0HudUiWidget@@QAE@I@Z`.
- `0x4b3d50`: `??1HudUiWidget@@UAE@XZ`.
- `0x4b4070`: `??0HudUiElement@@QAE@HH@Z`.
- `0x4b42f0`: `??0HudUiTextInput@@QAE@H@Z`.
- `0x4b4370`: `??1HudUiTextInput@@QAE@XZ`.
- `0x4b4620`: `??0HudUiSliderBorder@@QAE@XZ`.
- `0x4b49e0`: `??0HudUiNumericTextInput@@QAE@XZ`.
- `0x4b4ac0`: `??1HudUiNumericTextInput@@UAE@XZ`.
- `0x4b4ee0`: `??0HudUiZrdWidget@@QAE@XZ`.
- `0x4b50c0`: `??1HudUiZrdWidget@@UAE@XZ`.
- `0x4b6fc0`: `??0HudUiCheckToggleWidget@@QAE@XZ`.
- `0x4b7020`: `??1HudUiCheckToggleWidget@@UAE@XZ`.
- `0x4b7d60`: `??0HudUiCycleSelectorWidget@@QAE@XZ`.
- `0x4b7de0`: `??1HudUiCycleSelectorWidget@@UAE@XZ`.
- `0x4b8450`: `??0HudUiFillBitmap@@QAE@XZ`.
- `0x4b84d0`: `??1HudUiFillBitmap@@UAE@XZ`.
- `0x4b8760`: `??0HudUiZrdWidgetEx17C_Item@@QAE@XZ`.
- `0x4b87c0`: `??1HudUiZrdWidgetEx17C_Item@@UAE@XZ`.
- `0x4b8b10`: `??0HudUiZrdWidgetEx17C@@QAE@XZ`.
- `0x4b8b60`: `??1HudUiZrdWidgetEx17C@@UAE@XZ`.
- `0x4b8d30`: `??0HudCmdBindButtonBase@@QAE@XZ`.
- `0x4b92a0`: `??0HudUiListSelectorItem@@QAE@XZ`.
- `0x4b9540`: `??0HudUiBackground@@QAE@XZ`.
- `0x4b9760`: `??1HudUiBackground@@UAE@XZ`.
- `0x4ba020`: `??0HudUiTransitionTextPanel@@QAE@XZ`.
- `0x4ba4a0`: `??0HudFontStyle@@QAE@XZ`.
- `0x4ba4c0`: `?Destructor@HudFontStyle@@QAEXXZ`.
- `0x4ba740`: `??0HudUiPanel@@QAE@PBDHH@Z`.
- `0x4bab40`: `??1HudUiPanel@@UAE@XZ`.
- `0x4bb790`: `??0HudUiCompositePanel@@QAE@H@Z`.
- `0x4bc410`: `??0HudUiCompositePanelEntry@@QAE@ABU0@@Z`.
- `0x4bc480`: `??0HudUiCircle@@QAE@HHHI@Z`.
- `0x4bc510`: `??0HudUiBackgroundContainer@@QAE@H@Z`.
- `0x4bc540`: `??1HudUiBackgroundContainer@@QAE@XZ`.
- `0x4bc780`: `??0HudUiContainer@@QAE@XZ`.
- `0x4bc7b0`: `??1HudUiContainer@@QAE@XZ`.
- `0x4bcb50`: `??0HudUiTextLabel@@QAE@PBDHHH@Z`.
- `0x4bcf20`: `??0HudUiBar@@QAE@XZ`.
- `0x4bd020`: `??0HudUiTopMessageStack@@QAE@XZ`.
- `0x4bd2d0`: `??0HudUiChatMessageStack@@QAE@XZ`.
- `0x4bdbe0`: `??0zVideoFxPass3Slot@@QAE@XZ`.
- `0x4bdc70`: `??0HudWeatherFx@@QAE@H@Z`.
- `0x4bde40`: `??1HudWeatherFx@@UAE@XZ`.
- `0x4be280`: `??0HudWeatherFxSnow@@QAE@H@Z`.
- `0x4be2e0`: `??1HudWeatherFxSnow@@UAE@XZ`.
- `0x4be810`: `??0HudWeatherFxRain@@QAE@H@Z`.
- `0x4be870`: `??1HudWeatherFxRain@@UAE@XZ`.
- `0x4bee80`: `??1zVideoFxPass3Config@@QAE@XZ`.
- `0x4bef90`: `??0zVideoFxPass3Config@@QAE@XZ`.
- `0x4bf060`: `??0HudUiMessageBoxDialog@@QAE@PBD0@Z`.
- `0x4bf560`: `??1HudUiMessageBoxDialog@@UAE@XZ`.
- `0x4bf840`: `??0HudUiPolyline@@QAE@XZ`.
- `0x4bf980`: `??0HudUiBackgroundCursorWidget@@QAE@PBDH@Z`.
- `0x4bfa20`: `??1HudUiBackgroundCursorWidget@@UAE@XZ`.
- `0x4bfc80`: `??0HudUiBackgroundVideoWidget@@QAE@XZ`.
- `0x4bfcd0`: `??1HudUiBackgroundVideoWidget@@UAE@XZ`.

This aligns lifecycle classification only, not owner boundaries, provider
status, source-model gates, tiers, or byte equality. A new live order build
must independently pass before the call-contract census can resume.

## Retired message-box wrapper rules

Generation 112 removes the two expected-direct entries that demanded retired
DestructorCore wrappers at 0x4b87c0 and 0x4bf560. It also removes the two
message-box projections which replaced initializer/destructor wrapper targets
with their callees. Current source already uses native constructors and
destructors for those owners; the replacement targets must be resolved by the
ordinary physical identity comparison, not this historical substitution.

The old message-box static-vptr profile was tied to the retired initializer
symbol and supplied retail table identities at candidate offsets. It is
removed for this caller so that current native-source receiver evidence must
stand on its own. This intentionally does not claim that every current virtual
receiver is already proven. WSI-20260906-025 records the removed false bridges;
the generation-112 UI census now completes 259 bodies with seven divergences,
and the HUD census completes 406 bodies with two divergences. The native
top/chat constructors, their allocator, the ZRD-widget destructor, and the
message-box destructor pass their direct call comparisons. The message-box
constructor remains blocked by missing candidate member-vptr provenance;
removing the old profile is not evidence of a passing receiver contract.

## Confirm-quit native lifetime

Retail 0x415680 calls the background constructor and the ZRD-widget base
constructor twice, stamps the two child tables inline, then loads and binds
the dialog. The old verifier mapped two separately called derived constructor
COMDATs onto that one base identity. Generation 113 removes that substitution
and its caller-specific package (WSI-20260906-026). No physical target may be
replaced merely because its body calls the desired target.

The dialog now has an ordinary constructor and allocation by `new`. Governed
VC5 output in `hud-native-confirm-quit-order-01` passes 407/407 selected HUD
order entries, and its constructor matches all retail instruction bytes
outside COFF relocation fields. An explicit empty destructor adds a six-byte
derived-table store absent from retail. The implicit destructor instead
matches retail outside relocation fields in
`hud-implicit-confirm-quit-order-01`, which also passes 407/407 order entries.
Its existing compiler-generated-implicit-cleanup role is retained; revision
5307 changes only its source-trace edge from a manual function definition to
the complete class's generated contribution.

The two camera-state visibility calls now use the inherited element interface,
matching retail's actual table-slot dispatch at 0x405dd7 and 0x405e35 rather
than VC5's direct call caused by naming the concrete global type.

Live order reacceptance first exposed an existing registration/tracker class
disagreement at 0x40fb70. Direct assembly calls the bar base constructor,
stamps the derived table and initializes the two meter fields. Revision 5309
aligns this one row to authored-lifecycle/authored-lifecycle-body, retaining
its gating status and provisional shield-leaf name. No owner, provider, tier,
or byte acceptance follows from that classification repair.

Revision 5310 freshly accepts all 407 HUD order entries. The subsequent
`call-contract-hud-native-quit-g113-02` census passes all 406 selected call
bodies with no divergence and unchanged source. WSI-20260906-026 is resolved.
These are scoped order/call results, not whole-stage or authored-byte
acceptance.

## Composite-vector resize

The repeated unsigned size branches in retail 0x4bbca0 follow the original
VC5 `vector::resize` implementation. Replacing the handwritten growth/shrink
selection with native resize reduces its main-body calls from 24 to 10 and
preserves the 259-entry UI order in `zui-native-vector-resize-order-01`.
The constructor's erase path still inlines `std::copy` and calls entry
assignment, whereas retail calls the separate copy-loop body. The resize
function is different: retail 0x4bbdee calls 0x4bc3a0 directly from its inline
loop. Pro's independent review caught this distinction in the supplied raw
operands; direct decoding confirms it. The resize edit may therefore already
have the right assignment edge. Entry type, special-member visibility, and
the provisional translation-unit boundary remain under investigation;
matching the count does not accept the calls. The advisory exchange is in
`.devspace/runs/2026-09-06T13-01-44-995Z-chatgpt-call/transcript.md`.

## Text-image allocation and obsolete occurrence rules

Retail 0x4bac10 reloads the panel width and height fields across image
allocation calls. Replacing cached local dimensions with those actual member
reads restores both physical Create/SetFormatCode pairs; the former candidate
merged them into one pair. The empty-text rectangle assignment also follows
the retail store order. Unnecessary initialization of the GDI output handle
and metrics record is removed; each is read only after its output operation
succeeds, as in retail. `zui-text-rect-member-order-01` passes 259/259 order.

Generation-113 call comparison confirms the text body now has the complete
30-call census, but its old tail-merge projection rejects the new body because
it requires the retired 28-call shape. Generation 114 removes that projection
and the composite-vector projection which replaced an assignment target with
the range provider or collapsed 15 physical insert-related calls into one.
WSI-20260906-027 tracks those obsolete rules. The direct physical comparison
remains authoritative. Focused proof-kernel tests pass 72 cases at generation
114; full UI receiver and target convergence is still required.

## Controlled entry special-member experiments

The visible explicit-assignment variant and the implicit-assignment variant
both retain the entry type and its out-of-line copy constructor. Both emit an
assignment body in zui_widgets.cpp that matches retail outside relocations,
but leave the composite constructor calling assignment directly. Neither
emits the required standalone range copy in that TU. Both fail the current
order target at the assignment's registered zui.cpp definition (prefix
147/259), so neither result accepts a replacement owner or seam.

Removing the copy constructor entirely is blocked before compilation by the
existing authored-function source-emission contract, which requires a defines
edge for 0x4bc410. That failed attempt is not compiler evidence. The next
controlled variant keeps an explicit copy constructor but makes its natural
definition header-visible, retaining its defines edge and current class
classification. No ownership/classification change is used to make an
experiment appear to pass.

The header-visible copy constructor also matches the retail copy constructor
outside relocation fields, but does not change the constructor's unwanted
inlined range copy. Using the transition panel directly as the vector element
likewise retains that assignment call and loses the registered entry-vector
symbols (order prefix 145/259). The entry-vector type and its out-of-line
special members are restored. These experiments do not establish a replacement
class model or translation-unit seam.

Using the default resize argument instead of the scoped entry temporary also
preserves order (259/259 in `zui-default-resize-argument-order-01`) but leaves
the same assignment edge. The explicit scoped temporary is restored.

## Native virtual dispatch and complete-body proof

Retail 0x4b8837 dispatches HidePreview at slot 0x40; source incorrectly called
the nonvirtual HidePreviewIfNotSelected helper. Retail 0x4b8fb9 and 0x4b90c6
likewise dispatch the clip operation through slot 0x18 rather than the SetClip
helper. Source now uses those virtual interfaces. The option activation body
matches all instruction bytes outside relocation fields, and
`zui-native-virtual-dispatch-order-01` passes all 259 order entries.

Generation 115 removes the direct-to-virtual substitution and static-vptr
profiles that inferred a candidate table from length/count/slot alone
(WSI-20260906-028). Its fresh 259-body UI census has 18 divergences rather than
silently supplying missing receiver proof. Generation 116 adds a generic,
strict complete-body equivalence proof: live retail instruction bytes, decoded
address-operand population, relocation types, unique physical target identities,
zero addends, masks and extent must all agree. Only then may the corresponding
virtual call carry the same retail lineage. Unsupported or different bodies
still require independent receiver analysis. It changes no call count or form
and is not byte-stage acceptance. The focused proof kernel passes 73 cases,
including same-length changed receiver, slot, opcode, target, addend and mask
counterexamples. Fresh `call-contract-zui-body-proof-g116-01` verifies 259
bodies with stable source and 16 remaining divergences; corrected activation
and composite SetFont pass through the new exact proof. WSI-20260906-028 is
resolved. Generation 117 applies the same complete-body proof to targetless
virtual receiver lineages as well and removes the old composite cdecl-stack
offset substitution. Its focused kernel passes 74 cases and doctor passes
all seven gates.

## Composite entry loops and callback reloads

The complete composite method group exposes several source-shape differences:
retail SetPos indexes the vector and recomputes its size, rather than retaining
an iterator across callbacks; Update tests the complemented flags; and
ScrollHistory uses the indexed receiver expression. Those structured forms
restore the Update and ScrollHistory instruction bodies outside relocations.

ResizeEntryCount has an else-if lower clamp, an unsigned vector-size comparison,
a second size read when clamping, and an alternate lower bound on the requested
count that the old source omitted. Its visibility call reloads the vector entry
after text formatting. SetTextFmtV similarly reloads the active count and vector
storage after formatting. Both corrected bodies match retail outside relocation
fields. These changes do not introduce raw assembly or custom vector machinery.

All intermediate registered UI order checks retain 259/259 selected entries.
SetPos's declaration-order adjustment also restores its instruction bytes
outside relocations (`zui-composite-layout-initializers-order-01`). The fresh
generation-117 UI census checks 259 bodies with stable source: Update, SetPos,
ScrollHistory and SetTextFmtV pass; ResizeEntryCount still needs independent
relocation/receiver proof. Fourteen callers remain divergent, including two
obsolete message-box receiver-spelling profiles (WSI-20260906-029).
These diagnostic byte results are not authored-byte stage
acceptance or a claim that the whole composite constructor family is complete.

Generation 118 removes the obsolete message-box spelling profiles. Its fresh
259-body UI scan (`call-contract-zui-receiver-cleanup-g118-01`) passes both
callbacks and retains 12 unrelated divergences. The focused evidence kernel
passes 67 tests and doctor passes all seven gates; WSI-20260906-029 is resolved.

## Pass-3 configuration member ABI

Retail 0x4bed90 takes this in ECX and seven stack arguments, returning with
`ret 0x1c`. The reconstructed namespace fastcall helper incorrectly consumed
the first rectangle argument from EDX and returned with `ret 0x18`.
Retail 0x4bed50 likewise takes its packed-color word and double on the stack;
its public wrapper at 0x4beee0 explicitly spills the incoming CX word.
These instructions establish native configuration members, not free fastcall
helpers. UpdateLocal is recovered consistently as a member too. Existing class
layout, source-file placement and selected function population are unchanged.

`zui-native-fx-config-order-01` passes 259/259 entries. UpdateLocal,
QueueElementLocal and both public forwarding wrappers match retail instruction
bytes outside relocations. The primary-element method's receiver local also
restores its complete instruction body in
`zui-native-fx-root-receiver-order-01` (259/259 order). Fresh live UI order
acceptance at r5312 records all 259 identities, including the native member
symbols. Stale detached comments in zvid_main.c that treated
candidate fastcall symbols as original-source evidence have been removed.

Generations 119-120 consume accepted typed compiler-literal pooling aliases
rather than inventing a physical target from a string's contents. The alias
and manifest suppliers must agree on the physical target and extent; missing
evidence and collisions fail closed. The 68-case focused kernel and all seven
doctor gates pass. Fresh `call-contract-zui-native-fx-pooled-g120-01` passes
ResizeEntryCount and retains 11 unrelated divergences. WSI-20260906-030 is
resolved.

QueueElementLocal's remaining proof failure is a lost offset on VC5's wrapped
first register-relative MOV, despite complete COFF/retail bytes and relocation
identity agreeing. Generation 121 extends exact adjacent five-plus-one MOV
listing provenance to non-SIB register-relative disp32 loads. Its 69-case
kernel rejects nonadjacent, duplicate-coordinate, wrong-opcode and SIB cases;
doctor passes all seven gates. WSI-20260906-031 awaits the fresh caller scan.

## Binding-slot rebuilding and composite follow-up

Retail 0x4b910d clears the old array pointer after deletion and before allocating
the replacement. The recovered source now retains that state transition. Row
layout uses floating-point addition of the origin and spacing product, rather
than adding them as integers before conversion. The y calculation precedes
the receiver and x conversion. `zui-binding-slot-y-before-receiver-order-01`
passes 259/259 order and 0x4b90e0 matches all instruction bytes outside its
relocation fields. This is not relocation-semantic or call-stage acceptance.

A second Pro exchange (2026-09-06T14-27-54-204Z-chatgpt-call, transcript integrity
verified) rejects assignment visibility as a sufficient explanation of the
constructor edge. It favors the direct transition-panel type structurally,
but recommends one cohesive-TU experiment with the V2 header model fixed before
changing the type. K remains uncompiled, not disproven. Read-only inventory of
both retained V2 objects finds no emitted native mutable-pointer std::copy
specialization for the composite entry type. No consumer or retention device
is introduced to manufacture one. Advisory conclusions accept no owner, TU,
provider, order or bytes.

Fresh generation-121 UI verification checks 259 stable-source bodies and
passes QueueElementLocal; ten callers remain divergent. WSI-20260906-031 is
resolved. RebuildBindingSlotWidgets still needs exact relocation/receiver
proof despite its matching instruction body.

The new V2 baseline (`zui-v2-current-methods-baseline-order-01`) reproduces the
147/259 order prefix and lacks an emitted standalone std::copy specialization.
Both constructor and resize contain the inline copy loop with an Entry
assignment call. The first cohesive-TU compile omitted the later
zhud_ui_defs.h include from the broad TU, so its outlined transition constructor
and different assignment target are a setup difference, not a valid controlled
TU conclusion. The corrected paired compile retains that inline-definition
header. No stage or source-model acceptance is taken from these experiments.

The corrected cohesive-TU control (`zui-cohesive-composite-v2-order-02`)
reproduces the baseline constructor and resize extents (both 0x1f0) and their
same direct relocation/call sequences, including entry assignment at operand
offsets 0x149 and 0x14f respectively. No native std::copy specialization emits.
The tested TU regrouping does not explain the constructor edge. The class TU
has been removed, its methods restored unchanged to zui_widgets.cpp, the
canonical build/target TU lists restored, and the explicit out-of-line Entry
special members restored. No unrelated implementation bodies were moved.
The secondary constructor manifest now names the actual native copy/assignment
and the already classified provider copy target instead of retired wrappers;
all physical function obligations remain present.

## Native transition entry and binding-loader control flow

The direct transition-panel experiment retains the existing type and replaces
the empty derived Entry class with a compatibility typedef. Its explicit copy
constructor and assignment remain in zui.cpp at their original relative
positions. Both bodies match retail instructions outside relocation fields;
the default temporary now installs its table before initializing the flash
fields, as retail does. The explicitly empty transition destructor introduced
an extra table write during temporary cleanup. Removing that declaration lets
the native implicit destructor perform the base cleanup directly.
`zui-transition-implicit-destructor-order-01` passes 259/259 order and the whole
0x4bbca0 instruction body matches outside relocations. The constructor still
expands the copy loop rather than calling the retail provider specialization.
An inline-assignment experiment did not change that edge and moved the required
assignment contribution; the out-of-line definition was restored.

The fresh 259-body generation-121 scan
`call-contract-zui-native-transition-g121-03` has ten divergent callers and no
source changes during validation. Three composite-family diagnostics now stop
at stale registered selectors, which require target synchronization; this is
not an accepted model or call result. No owner merge or authored-byte acceptance
has been performed. The typedef is a current implementation hypothesis, not
proof of the original spelling or complete owner boundary.

Direct inspection of 0x4b8de0 establishes another source correction. Retail
checks presence of LIST_OFFSET/LISTSIZE, then consumes their typed payloads
without the extra node-type/fallback guards. Its outer child-addition loop
contains a complete nested font-application loop, with the style lookup inside
the outer loop. Array storage is reloaded after callbacks and between the color
and shadow updates. The former reconstruction cached one row/style and applied
the font only once per outer row. The recovered structured nested loops preserve
the retail behavior, including its redundant repeated font pass. No attempt is
made to optimize that retail behavior away. The initial correction passes all
259 order entries; exact instruction convergence is still being checked.

The binding-slot rebuilding body at 0x4b90e0 also demonstrates the current
complete-relocated-body proof's limit: FS-relative __except_list operands, a
caller-associated EH dispatcher and the implicit selector-item destructor lack
ordinary global-name identities. Matching instructions alone do not close these
relocation obligations. No exception operand is ignored or assigned a target
from candidate contents.

The adjacent selection body 0x4b9330 used signed casts of vector size where
retail branches unsigned after its separate negative-index guard. It now keeps
the vector's unsigned comparison, loads the selected binding entry before
writing the row's entryIndex, and reloads the vector for the selected central
panel after storing its index. Scoped visibility receivers and the native
pointer-expression DrawBase call reproduce the complete retail instruction
body outside relocations. No raw dispatch scaffolding or helper was added.

Target registration synchronization at r5313-r5315 updates the UI native type
selectors, the secondary constructor manifest, and the pass-3 configuration
member target. Fresh live order at r5316
(`live-order-zui-native-transition-bindings-r5315-01`) passes all 259 UI entries
and all 13 source-policy checks. This incorporates the native transition model,
implicit destructor, binding-slot rebuilding, loader-loop correction and
selection correction. The loader is still not instruction-exact. The canonical
generation-121 replay is started against the current 3,301-body/96-target census;
these diagnostic matches do not preaccept its call or byte results.

The generation-121 whole-census replay completes its fresh 96-target proof in
795,990 ms. It commits seven complete 160-body slices and 159 passing bodies
in the eighth, reaching transaction 5324 (semantic/evidence 5321). Its first
divergence is the deferred WOL caller 0x43e1c0. The full proof also reports ten
UI callers and the prepared-index loader 0x4c5550; later slices are diagnostic
only and were not committed. The current selected UI source still has the
retired 0x4b9330/0x4bb790 candidate-profile blockers, despite the selection
body's independently matching instructions.

WSI-20260906-034 identifies why serial commits continue for many minutes after
the census completes: each body's acceptance recursively scans the full tracker
for superseded evidence references. Generation 122 installs the complete set of
individual body/evidence bindings before performing one retirement scan per
slice. Generic regression coverage preserves separate transcripts, shared
evidence still used by unselected bodies, external references and unrelated
dimensions. All 151 proof-kernel cases, seven doctor checks, strict pipeline
audit and whitespace check pass. The issue is resolved at issue revision 3994.
The generation change conservatively makes earlier call evidence stale; it
does not reuse or upgrade the completed generation-121 proof.

The third composite source-model advisory exchange at
`.devspace/runs/2026-09-06T15-26-50-178Z-chatgpt-call/` favors testing native
implicit transition copy members while retaining the physical authored gates.
The transcript passed integrity verification and was shown verbatim. That
experiment remains uncompiled: WSI-20260906-033 records the missing narrowly
typed class-to-implicit-copy emission route. No classification is weakened to
permit it.

WSI-20260906-032 removes a separate unsupported expected-side premise. The old
static-vptr adapter checked an independent constructor table write and table
cell but never connected the constructor receiver to the runtime call. Its
removal also removes the three candidate cycle-entry table projections and the
two obsolete candidate dynamic-vptr profiles. Ordinary receiver extraction and
the complete relocated-body proof remain; missing lineage must block rather
than acquire a static target. Generation 123/expected schema 68 is under fresh
UI verification. A new generic clobber regression additionally reproduces
WSI-20260906-036: the local retail targetless shortcut ignores an intervening
dispatch-register write. That defect is not yet resolved by the adapter removal.

Generation 124/expected schema 69 separately fixes that clobber defect: the
local shortcut requires the vptr definition to reach the call without a write,
and unknown transfers kill partial-register and implicit outputs. Generic
tests reject XOR, MOV DL, XCHG and CDQ clobbers while retaining distinct,
targetless receiver fields. All 152 compact proof-kernel cases and seven
infrastructure checks pass. WSI-20260906-036 is resolved at issue revision
3997; WSI-20260906-032's broader receiver-provenance work remains open.

Generation 129/expected schema 74 adds exact incoming scalar argument indexes
to the CFG array-load proof while preserving their entry-stack coordinates.
The object must still be rooted in this, not in an unknown stack value.
Executable paths through COD data directives remain unresolved. All 95 focused
call-contract cases and seven infrastructure checks pass; the strict pipeline
audit passes at r5326. The fresh UI scan
`call-contract-zui-argument-array-g129-01` clears AddTextEntry (0x4b7fd0) and
ApplyFontStyleForEntry (0x4b8100). It reports 20 caller divergences, including
an actual AddBitmapEntry (0x4b8200) receiver mismatch and a newly exposed
TextRect exact-IAT proof conflict. The remaining receiver proofs are not
replaced with constructor-derived static targets.

AddBitmapEntry now uses native `new HudUiWidget(0)` and reloads entriesB[index]
for SetPos, SetVisible and AddChild after callbacks, as retail does. The old
manual allocation/Constructor wrapper and cached receiver obscured those
semantics. Fresh `zui-bitmap-native-order-01` passes all 259 UI order identities;
its 0x4b8200 object body matches retail outside COFF relocation fields. A fresh
call scan is required before claiming call convergence; relocation masking
alone does not accept byte-stage operand semantics or linked bytes.

The bitmap correction's fresh live order passes all 259 identities and all
13 source-policy checks at r5328, covering both UI physical blocks. The old
AddBitmapEntry Constructor-wrapper profile is removed in generation 130;
native construction must be compared through its actual registered constructor.
Its remaining candidate receiver proof is not treated as a source mismatch.

Generation 130 adds an exact CFG recurrence proof for zero-entry nonvolatile
array cursors. Entry initialization must dominate the queried call, remain
outside its cycle, and have exactly one positive update on every returning
cycle with no other reaching register write. The proof retains the array base
and byte stride, but establishes neither an array capacity nor a static virtual
callee. All 156 compact proof-kernel cases and seven doctor checks pass. Fresh
`call-contract-zui-cfg-array-g130-01` reduces the UI divergence count from 20 to
16: 0x4bb980, 0x4bb9f0, 0x4bbb20 and 0x4bbbe0 now pass. The 0x4b9330 failure
moves from retail provenance to candidate provenance; it is not accepted.

Direct inspection of 0x4b7340 confirms retail's three assignments to the same
checked-label field, including both disabled branches. Each field assignment
precedes text/position/font/visibility callbacks and later calls reload the
field. The former CreateHudZrdTextPanel helper added unsupported payload
fallbacks, manually allocated/placement-constructed a panel and returned it
only after those callbacks. The three branches now use native allocation,
direct typed node payloads and the observed field timing. Bitmap lookups also
retain node-presence guards without invented payload checks. Fresh
`zui-toggle-native-loader-order-01` preserves all 259 order identities.

The toggle's extra CALL sites are concentrated in a partially expanded STL
insertion. A counted-overload experiment (`zui-toggle-counted-label-order-02`)
still passes order but expands both insertion sites and does not reproduce
retail; that experiment is reverted. The native receiver/payload corrections
remain. Neither this loader's call contract nor its bytes are converged.

Generation 131 extends candidate CFG proof to exact array receivers. Input
listing bytes must match the current contiguous COFF prefix. Relocated register
definitions become unknown instead of being interpreted as literal addends;
only later independently exact definitions recover them. The receiver and
loaded vptr must agree after all CFG arrivals. Generic tests reject missing
initialization, clobbers, zero strides, listing/object disagreement and a
relocated array displacement. All 156 compact proof-kernel cases and whitespace
checks pass; fresh UI and AppFrame convergence scans are in progress.
