# Mission constructor call-contract audit — 2026-09-05

This is diagnostic evidence for restoring the authored-call prerequisite before
authored-byte work. It accepts no source owner, hierarchy, table identity,
function order, call contract, byte group, or executable. Use the tracker and
fresh governed comparisons for current acceptance. The playground executable
was not rebuilt or deployed during this investigation.

## Direct retail facts

The authenticated, already-open Recoil database was inspected for assembly at
`0x419aa0`, `0x41a190`, and `0x41a200`, and table contents at `0x4cf8d0` and
`0x4cfa70`. Old constructor and table labels are not proof.

- `0x41a190` takes one stack argument, calls the numeric-input base at
  `0x4b49e0`, stamps `0x4cfa70`, allocates the requested text buffer through
  `0x4b4390`, calls `0x4b4e60` with an empty string, calls `0x4b4ba0` with zero,
  and returns with `RET 4`. It is not the removed placement-new wrapper.
- `0x41a200` calls `0x4b49e0` directly. It expands the same intermediate
  initialization, increments the requested digit count before allocation,
  stamps `0x4cfb00`, initializes signed clamp bounds at offsets `0x374` and
  `0x378`, and returns with `RET 4`. Current out-of-line C++ instead calls
  `0x41a190`, so its static call contract differs.
- Panel construction at `0x419aa0` calls `0x4b4ee0` directly for launch and
  cancel controls and then stamps their concrete tables. There are no separate
  child-constructor calls at these sites.
- Panel input construction is asymmetric: game-name/time-limit call
  `0x41a190` with `21`/`4`, while kills/max-players call `0x41a200` with `2`/`2`.
  The current time-limit initializer calls `0x41a200` with `3`.
- After constructing the game-name member at offset `0xabe8`, retail stamps
  `0x4cf8d0`. That table and `0x4cfa70` have identical 36 resolved slots,
  including the deleting destructor at `0x41c4a0`. Separate addresses alone do
  not prove another source class; the executable restamp is an additional fact
  any eventual model must explain.

## Bounded VC5 experiments

All checks used the registered `mission_417350_41cc10_authored_order` target,
the governed `/O2 /Ob1 /MD /GX /Gr /Zp4` profile, and fresh roots below
`build/live-validation`. No compiler flags, target registration, or expected
retail facts were changed to make a source variant pass.

| Experiment / root basename | Observed order result | Disposition |
| --- | --- | --- |
| Inline keyword on existing parameterized `.cpp` definition / `mission-constructor-inline-order-01` | Required `0x41a190` body missing; prefix 55/124 | Reverted |
| Both numeric constructor layers header-inline / `mission-constructor-header-inline-order-01` | Required `0x41a200` missing; `0x41a190` precedes `0x41a160` | Reverted |
| Only parameterized layer header-inline / `mission-constructor-header-inline-order-02` | Required `0x41a190` missing; prefix 55/124 | Reverted |
| Remove redundant out-of-line launch/cancel constructors / `mission-button-constructor-order-01` | PASS, 124/124 | Retained for fresh live order verification |
| Both numeric layers header-inline plus implicit button constructors / `mission-constructor-family-inline-order-01` | Required `0x41a200` missing; wrong neighboring order | Numeric experiment reverted |
| Add a data-less game-name subtype to that inline family / `mission-constructor-game-name-subtype-order-01` | Required `0x41a200` still missing; wrong neighboring order | Subtype and inline experiment reverted |
| Default digit argument and implicit kills/max-player constructors in the header-inline family / `mission-default-digit-input-order-01` | Required `0x41a200` still missing; wrong neighboring order | Reverted |
| Both numeric definitions marked inline at their existing `.cpp` positions / `mission-cpp-inline-family-order-01` | Required `0x41a200` missing; prefix 56/124 | Reverted |

Two additional nonaccepting compiler-profile diagnostics used the existing
`vc5_o2_ob2_md_gx_facs` profile through `verify vc5 --skip-bn-compare`, not the
canonical order-acceptance route. This profile changes `/Ob1` to `/Ob2` and
omits `/Gr`; production configuration remains unchanged. In
`mission-ob2-diagnostic-01`, all required authored contributions remain in
relative order and the clamped constructor reproduces the retail instruction
sequence outside relocations. However, the panel expands the game-name
constructor into four calls, and kills/max-player construction calls
`0x41a190` with 3 instead of `0x41a200` with 2. In
`mission-ob2-game-name-diagnostic-01`, a data-less game-name subtype adds a
table restamp but does not correct those calls. The subtype was reverted.
These observations neither establish an original compiler profile nor accept
the constructor bytes. They are evidence for further source-context recovery.

The retained constructor change removes two declarations and their empty
out-of-line definitions. Existing concrete button classes, virtual handlers,
base classes, member layout, and runtime fixes remain unchanged. VC5 can emit
their natural base calls and concrete table stores without these extra calls.
The direct order-acceptance route declined the request because authored order
is already complete and is not the current serial stage. It made no tracker
mutation; the diagnostic 124/124 order result is not a new acceptance record.

## Verifier corrections

The constructor dependency collector incorrectly required the removed
`HudUiNumericTextInput::BaseConstructor` wrapper from `zui.cpp`. Acquisition now
requires only the live numeric, cycle-selector, and ZRD base definitions, still
rejecting missing, duplicated, wrong-source, or extraneous definitions. The
obsolete expected-direct wrapper profile for `0x41a190` was removed. A fresh
160-body scan in `call-contract-mission-constructors-02` then passed 158 bodies;
only `0x419aa0` and `0x41a200` remained divergent. This diagnostic was not live
stage acceptance.

The historical ordinal-7 projection can rewrite a registered `0x41a200` call
to expected `0x41a190` after its snapshot guards pass. This is not exact target
identity. A rejection-only check now compares already-known authored physical
identities before legacy projections. It requires aligned static populations,
an exact direct E8 listing, and its matching zero-addend REL32 COFF relocation.
Accepted physical/ICF identities remain authoritative; unknown or malformed
evidence still goes through normal fail-closed extraction. The check never
accepts a body or derives expected truth from a candidate.

This stricter check also exposed the branch-order difference at `0x41b5a0`
(`NetSessionBrowserDialog::OnCreateSession`). Current source had emitted the
non-modem `KillTimer` branch before the modem-session call sequence, whereas
retail emits the modem branch first. The source branches were exchanged without
changing their conditions or effects. The obsolete invocation-population
profile, which returned the retail rows instead of the observed candidate
order, was removed. `mission-browser-call-order-01` passes all 124 authored
order entries. The fresh `call-contract-browser-order-01` scan passes 158/160
bodies, including `0x41b5a0`; it reports only `0x419aa0` ordinal 7 and `0x41a200`
ordinal 0 as mismatches. Generic proof-kernel tests pass 100/100 and all seven
doctor checks pass. These are scoped checks, not whole-stage acceptance.

## Advisory and unresolved work

### Follow-up experiments and retained corrections

The follow-up Pro exchange is recorded at
`.devspace/runs/2026-09-05T20-51-39-693Z-chatgpt-call/transcript.md`.
It distinguished an actual zero-argument overload from a default argument on
the existing parameterized constructor. This remains advice, not source truth.

`mission-ob1-default-calling-convention-diagnostic-01` uses the registered
`vc5_o2_ob1_md_gx_facs` profile without `/Gr`. It retains authored order but
does not change either baseline constructor call mismatch. The previous
expansion therefore follows `/Ob2` in this tested comparison, not merely the
absence of `/Gr`. Production compiler settings are unchanged.

The canonical `mission-fixed-default-constructor-order-01` check passes
124/124 with a distinct inline `HudUiClampedIntTextInput()` overload which
constructs `HudUiNetGameSetupTextInput(4)` and initializes the clamp bounds.
The time field selects this overload. The panel then emits the four required
input calls with arguments 21, 4, 2, and 2. The parameterized clamped body
still calls the intermediate constructor and is not byte/call accepted.

Direct panel assembly also proves six step-button initializations: null
target at offset `0x14c` and step 1 at `0x150`, followed by the concrete
table store. These were absent from the reconstructed implicit construction.
The added natural default constructor is retained;
`mission-step-initialization-order-01` passes 124/124. The fixed-size overload
and step initialization are source corrections under investigation, not an
accepted complete owner or source model.

Three further combinations with the fixed-size overload, step initialization,
an inline intermediate constructor and a game-name leaf were diagnostic:
`mission-fixed-default-name-leaf-inline-order-01`,
`mission-name-parameter-inline-order-01`, and
`mission-name-cpp-visibility-order-01`. Each passes 124/124 order and produces
the retail four-call sequence in the standalone clamped constructor. Each
still expands the game-name construction into four calls rather than the
required single intermediate-constructor call. The game-name leaf and inline
intermediate-constructor experiments were reverted. The game-name restamp
and standalone clamped expansion therefore remain unresolved together.

The fresh `call-contract-fixed-default-g68-01` check instead reaches an
obsolete WorldSelector prerequisite: it demands the retired numeric-wrapper
identity projection. This is not proof of a changed WorldSelector target.
Do not restore the removed wrapper or update historical snapshots to pass it.

The generation-67 whole replay at
`call-contract/401000-408210-r5228-replay-001` aborted because the new early
known-target rejection was classified as a fully evaluated mismatch despite
having no complete candidate transcript. No replay acceptance was recorded.
WSI-20260905-016 corrects that classification to a blocked candidate with the
precise target mismatch retained; a generic regression verifies the result
projection accepts no body and rejects the former malformed classification.
The generation-68 focused evidence/replay kernel passes 55 tests; the complete
kernel passes 102, and doctor passes 7/7. These do not replace live closeout.

Separately, WSI-20260905-015 removes historical Player compiler-helper offset
profiles in favor of same-ordinal retail provider facts and live E8/REL32
provenance. `call-contract-player-provider-g68-01` passes all 160 bodies.
The original finite caller/provider authority remains intact. No later stage
or byte fact is accepted by this diagnostic.

The direct ChatGPT Pro consultation is recorded at
`.devspace/runs/2026-09-05T20-09-48-105Z-chatgpt-call/transcript.md`. No files were
uploaded; the prompt contained source excerpts, retail observations, compiler
flags, and the first failed experiment. The reply recommended the header-family
and minimal-subtype differentials above, and rejecting historical constructor
equivalences as byte-readiness proof. Its suggestions do not prove a model.

Neither tested inline arrangement nor the subtype differential establishes the
complete required emitted population and order. Do not repeat these variants
without new discriminating evidence. The surviving constructor-call mismatch,
the time/kills asymmetry, the game-name table restamp, and legacy snapshot guards
must still be resolved before this slice and then the complete call-contract
closeout can pass. Authored-byte completion has not been claimed.

## Removal of obsolete cumulative constructor projections

`call-contract-mission-g78-01` passes 122/124 bodies. The panel is blocked by
the WorldSelector prerequisite demanding retired wrapper projections, while
the standalone clamped constructor fails the rejection-only raw call count.
WSI-20260905-027 records the former tool defect. Generation 80 removes the
six-function cumulative launch/numeric/world-selector/next/previous constructor
projection family. These old target equivalences and caller snapshots no
longer mediate ordinary registered constructor identity comparison. Generation
81 removes the second downstream merge of that retired result; the first
generation-80 diagnostic raised a NameError there and accepted nothing.

Fresh BN inspection again confirms that `0x41a200` writes the intermediate
`0x4cfa70` table before allocating/updating/disabling the input, then writes
`0x4cfb00` and the clamp fields. This rules out merely treating the clamped
class as a sibling with no intermediate construction. A follow-up Pro
consultation includes the exact stores, asymmetric panel call sites, and the
failed inline/profile experiments. It is advisory; no new hierarchy or TU
model has yet been accepted.

Generation 82 removes the remaining 22 dependent inline-input and panel-vptr
projection/guard functions. Generation 83 removes the current-artifact direct
identity profile, including its incorrect substitution of `CString::~CString`
(`0x4c5b88`) for the MSVC array-destructor helper (`0x4c5ec0`). Candidate calls
now remain their actually emitted identities. The fresh
`call-contract-mission-g83-01` comparison passes 122/124 bodies, including the
complete panel constructor at `0x419aa0`. Its two real failures are the clamped
constructor call count and the map-name array cleanup helper. WSI-20260905-028
records the removed false helper equivalence. No whole-stage acceptance follows
from this diagnostic.

The follow-up Pro exchange at
`.devspace/runs/2026-09-05T23-14-51-557Z-chatgpt-call/transcript.md` recommends
checking natural producer/consumer definition visibility only if independent
TU evidence supports the boundary. It also suggested one bounded `/Os`
diagnostic. That check, `mission-ob2-os-diagnostic-03`, uses the registered
`vc5_o2_ob2_os_md_gx_fastcall_facs` profile without changing any production TU
mapping. It fails the required shape: the panel and intermediate constructor
gain `__EH_prolog` calls, the panel calls concrete input leaf constructors,
and the clamped body still calls the intermediate constructor once rather than
the four retail calls. `/Os` is rejected as a resolution. No source split has
been implemented or accepted.

Direct map-array evidence: startup slot `0x4da048` references only `0x41c980`,
which calls `0x41c990` and tails to `0x41ca00`. Initialization constructs seven
adjacent four-byte `CString` objects beginning at `0x4f32d8`. Registration
passes `0x41ca10` to `atexit`; cleanup passes the array address, count seven,
stride four, and the CString destructor pointer to `0x4c5ec0`. This is a native
static array lifetime, not the reconstruction's raw integer storage, extra
pointer, placement construction, and manual reverse destructor loop. The
source now uses the ordinary initialized array at the lifecycle cluster and
removes the duplicate manual CRT entry from RecoilApp.cpp. Governed compiler
comparison confirms the native helper population. The fresh
`mission-native-map-array-helper-01` diagnostic reports zero unmasked byte
differences for all four helpers; direct COD inspection confirms their call
targets, array base/addends, count, and stride. Their reviewed pipeline-only
classification correction was applied at revision 5238, retaining required
non-authored full-order inventory. Registrations were synchronized at
5239/5240/5241. Fresh live order at revision 5243 passes all 120 authored
mission bodies, including the dialog's native provider-vector accessors.
The 97 focused proof-kernel tests and seven doctor checks pass, and
WSI-20260905-027/028 are resolved on their scoped evidence. These results do
not accept a provider owner, array data/linkage gate, authored byte group,
or whole-program linked image.

The subsequent `call-contract-mission-native-array-vector-g84-01` diagnostic
passes 119/120 authored bodies. Only `0x41a200` remains a real mismatch: one
intermediate-constructor call versus four retail calls. The panel, native map
array callers, and native vector consumers pass this scoped call comparison.

Additional retail EH inspection does not resolve the game-name subtype or TU
visibility ambiguity. Panel handler `0x4c9752` references FuncInfo `0x4d57b8`
and the 20-entry unwind map at `0x4d57d8`. Cleanup `0x4c9664` adjusts by `0xabe8`
for game-name input and tails to `0x41a3f0`; cleanup `0x4c969c` adjusts by
`0xb3fc` for time input and tails to that same destructor. The game-name table
`0x4cf8d0` has its only code reference at the panel restamp `0x419b0c`; the
intermediate table `0x4cfa70` is referenced by both standalone constructors.
These facts constrain the model but do not prove another original class or
a new translation-unit boundary.

The follow-up Pro exchange at
`.devspace/runs/2026-09-06T00-29-42-979Z-chatgpt-call/transcript.md` rejects
signed/unsigned constructor overloading as currently unsupported: literal
arguments and INC/RET4 do not prove either signedness or another overload.
No such overload was added. A new controlled single-definition-inline pair
uses only an `inline` keyword at the existing `.cpp` definition, with no
production compiler mapping change. `mission-single-inline-ob1-order-02`
passes 120/120 order and expands the clamped body into the correct four calls,
but still incorrectly expands the game-name input. The `/Ob1 /Os` comparison
`mission-single-inline-ob1-os-diagnostic-01` retains the game-name call but
introduces `__EH_prolog`, outlines the time leaf, and leaves one call in the
clamped body. It therefore fails the required simultaneous contract. The
temporary source-inline change was reverted.

Retail clamped EH handler `0x4c9788` loads FuncInfo `0x4d58a0` (one unwind
state), whose map at `0x4d58c0` invokes cleanup `0x4c9780`. That thunk loads
the saved object from EBP-0x10 and tails to numeric-base destructor
`0x4b4ac0`. State zero starts after successful numeric-base construction and
covers allocate/update/disable. The native intermediate-inline model's EH
metadata was compared directly in `mission-single-inline-ob1-order-02`:
the clamped COMDAT has one unwind state and its cleanup thunk loads ECX from
EBP-0x10 and tails to the numeric-base destructor, matching those retail
semantics. This supports intermediate inlining in the clamped body, but
shared panel cleanup and this EH match do not settle the panel's different
inlining behavior or establish the complete source model.

Two more bounded source differentials separate implicit default initialization
from the earlier explicit initializer trials. `mission-implicit-default-init-order-01`
adds a default argument of 21 to the same unsigned constructor declaration,
omits the panel member initializer, and marks the existing `.cpp` definition
inline. `mission-implicit-game-name-leaf-order-01` additionally uses a data-less
game-name subtype with an implicit default constructor. Both preserve 120/120
authored order; neither preserves the game-name call (both expand its four
leaves). The subtype adds its table restamp but does not solve the call
asymmetry. All temporary default-argument, inline, and subtype changes were
reverted; no overload, source model, or byte/call fact was accepted.

## Full panel dataflow comparison — 2026-09-06

The complete retail panel assembly exposed additional source differences that
the call-only comparison does not claim to detect. The retained corrections
initialize `reconfigureExistingSession` before `LoadFromZrd`, remove twelve
redundant post-load step-button stores already performed by native constructors,
reset `currentFocusWidget` immediately after the world-selector index update,
and place time/kills step bindings immediately after each input initialization.
The game-name constructor still receives 21; its later buffer allocation now
receives the distinct retail value 22. Enable predicates reload the current
reconfiguration field after potentially state-changing virtual calls.
`mission-panel-store-sequence-order-01` passes 120/120 order;
`call-contract-mission-panel-stores-g89-01` passes 119/120 calls and still rejects
only `0x41a200` (one candidate call versus four retail calls). These scoped
diagnostics do not accept bytes or complete the stage.

The fuller advisory exchange at
`.devspace/runs/2026-09-06T01-14-11-192Z-chatgpt-call/transcript.md`
successfully uploaded the current full mission TU, its two relevant headers,
this audit, and the complete retail constructor assembly. It identified one
concrete caller-dataflow differential: retail reloads the initialized clamp
bounds before signed comparisons, whereas the current helper clamps against
its by-value constant arguments. The proposed experiment must first establish
that difference in current generated output, and must not add volatile fields,
barriers, or artificial work to influence inlining. This is advisory only;
neither another class layer nor a new TU boundary has been established.

`mission-clamp-member-reads-order-01` tests only replacing the helper's bound
arguments with `input->minValue` and `input->maxValue`. Order remains 120/120,
but the generated panel still folds all three clamps to constants. The change
was reverted; it does not justify another constructor-inline experiment or
artificial caller-complexity adjustment.

The fresh generation-89 full-census replay under
`call-contract/401000-408210-r5248-replay-001` checked 96 targets and recorded
639 passing bodies in its first four serial slice projections, ending at
tracker revision 5251 (semantic/evidence 5248/5248). Its first divergence is
still `0x41a200`; no later slice or closeout was accepted. Later full-census
diagnostics also include a UI target-acquisition failure (`COMDAT header
provenance requires one fresh TU observation`) and a raw call-count mismatch
at `0x4c5550` (10 candidate versus 12 retail). Those need independent diagnosis;
the first divergence is not evidence that the remaining census is clean.

The separate declaration-placement differential
`mission-inline-declaration-only-order-01` puts `inline` only on the existing
header declaration, leaving the `.cpp` body in place. VC5 still expands the
panel game-name construction to four calls, just as with the previous
definition-only spelling, while retaining 120/120 order and expanding the
standalone clamped constructor. This rules out that untested declaration/body
spelling distinction for the current model. The keyword was reverted.

WSI-20260906-003 fixes the full replay's UI acquisition failure: its auxiliary
constructor build and primary UI build legitimately have the same target and
source but different absolute compilation directories. Header selection now
binds to the actual COD compilation context before requiring uniqueness, and
still enforces receipt stability and exact observed header provenance. Generic
tests reject duplicate same-context, absent-context, malformed-context, and
stale observations. Generation 90 passes 49 focused evidence tests, all 126
proof-kernel tests, and seven doctor checks. The issue is resolved on that tool
boundary; UI calls are not thereby accepted, and generation-89 call acceptance
requires fresh replay.

## Shared clamped-input value operation

Direct assembly at `0x41a2d0` confirms two consecutive clamp passes: the first
determines the return value, and the second formats/displays it. The latter
clamp/20-byte-buffer/`sprintf`/numeric-`Update` operation also appears at
`0x41a350`, `0x41a820`, `0x41a9c0`, and the panel's three initializations.
This supports investigating an inline input-owned value operation rather than
duplicated free-form caller code. `SetValue` is a same-engine naming default
found in the terminology catalog, not a recovered Recoil spelling.

`mission-value-method-order-01` centralizes that operation on the existing
clamped-input class and passes 120/120 order with no standalone setter call.
A candidate-to-candidate diagnostic across defined registered mission bodies
finds unchanged code outside relocation fields except at `0x41a350`. There,
the setter model retains the input object in EDI across `sprintf`, matching
retail's object lifetime and saved-register shape; the old source instead
reloaded `targetInput` afterward. Register allocation still differs from
retail, so this is not exact-byte acceptance. The direct expression variant
`mission-value-method-expression-order-01` also passes 120/120 and preserves
this same code shape. Neither variant restores the panel's constant-folded
clamp branches or resolves the constructor mismatch. Whole-target call
comparison remains required before retaining this source-structure candidate.

`call-contract-mission-value-method-g90-01` initially rejects the step handler
through an obsolete candidate-offset/storage-spelling profile, not a differing
call sequence. WSI-20260906-004 removes that two-caller profile and replaces
only its equivalent field/address rendering with side-local grammatical
canonicalization. No eligibility is established, no load is dropped, and no
retail row is copied. Generation 91 passes 50 focused evidence tests, all 127
proof-kernel tests, and seven doctor checks. The fresh
`call-contract-mission-value-method-g91-01` comparison passes 119/120, including
both former profile callers `0x41a350` and `0x41a5b0`; only `0x41a200` remains
divergent in this target. The value-operation source candidate is retained on
that scoped evidence, without source-model, owner-tier, data, or byte acceptance.

Current literal-xref inspection confirms that `D:\Proj\Battlesport\mission.cpp`
at `0x4db230` is referenced at `0x417fc2`, `0x4181b6`, `0x418209`, `0x4182ff`,
`0x418395`, `0x419091`, and `0x419304`, not directly by the panel/constructor
cluster. The `hud.cpp` literal at `0x4dadd8` has references only at `0x4101a3`
and `0x4141bb`. These delimit the actual literal proof; neither the historical
step-handler comment naming hud.cpp nor the absence of further mission.cpp
xrefs proves a new TU boundary. Both numeric constructor bodies have exactly
two code callers, all in the panel, and no data references.

## Separate game-name table and caller-owned range initialization

`mission-game-name-leaf-baseline-order-01` tests the game-name subtype separately
from intermediate-constructor inlining. With the intermediate constructor still
out of line, the panel emits the retail call with capacity 21 and the separate
game-name table write immediately afterward, while preserving 120/120 order.
`call-contract-mission-game-name-leaf-g91-01` passes 119/120, rejecting only
`0x41a200`. The game-name data target now names the leaf's compiler-generated
table instead of the intermediate table; the reviewed target synchronization
was applied at revision 5252. This records a candidate binding, not acceptance
of a data gate or the complete class model. Both retail tables contain 36
identical resolved slots, but their distinct constructor uses remain required.

`mission-direct-range-stores-order-01` removes the free `InitClampedInput`
wrapper and places each bound store directly in the panel, followed by the
existing input-owned `SetValue` call. Unlike merely reading fields inside the
wrapper, this restores the retail field-load/compare/clamp branches under VC5.
The time clamp starts at body offset `0x4aa`, as in retail, and all 120 authored
functions preserve order. Bound-store base registers still differ, so this is
not byte acceptance. The changed source structure, not a compiler flag change,
explains the previously constant-folded checks.

The subsequent `mission-direct-range-inline-order-01` differential adds only
`inline` to the intermediate constructor after that real caller correction.
It still incorrectly expands the panel's game-name construction to four calls,
despite retaining 120/120 order. That keyword was reverted. The corrected
caller checks do not by themselves resolve the constructor's inlining asymmetry.

`call-contract-mission-direct-range-stores-g91-01` freshly checks the retained
non-inline constructor plus direct range stores. It passes 119/120 with only
`0x41a200` divergent and reports no source changes during validation. The
game-name data diagnostic `mission-game-name-leaf-data-01` reports zero
unmasked mismatches and 36 DIR32 table slots. Relocation identity review still
requires the compiler-generated leaf deleting destructor's folded identity;
an all-relocation-masked table is not sufficient for data acceptance.

## Construction/provenance audit after restored clamps

The follow-up Pro exchange at
`.devspace/runs/2026-09-06T02-24-54-149Z-chatgpt-call/transcript.md` uploaded the
updated complete mission TU, both headers and this audit. It recommends tracing
the actual emitted constructor/destructor graph and searching other retail
initializations before considering another class layer. It explicitly rejects
treating the observed two-versus-three source-edge asymmetry as a proven VC5
inlining-depth rule. All four uploads completed; advice accepts no facts.

The read-only `audit coff-lifecycle` comparison of
`mission-direct-range-stores-order-01` and `mission-direct-range-inline-order-01`
finds one definition of each parameterized constructor. Both call sites in the
baseline panel target the same NetGameSetupTextInput constructor symbol; the
single-inline build retains only the time-input call to that symbol. The
standalone clamped COMDAT changes from a 48-byte padded section with one call
to a 144-byte padded section containing the four initialization calls and
associated EH metadata. These are section extents, not executable body sizes.
No duplicate constructor definition explains the disagreement.

The audit also identifies an incorrect historical binding for `0x41a3f0`:
the mission object has no definition of the base numeric-input destructor
symbol, which belongs to `0x4b4ac0`. Its implicit NetGameSetupTextInput,
GameNameInput and ClampedIntTextInput destructors instead have identical
five-byte tails with REL32 relocation at offset one to that base destructor.
The game-name and intermediate scalar-deleting wrappers have equal code but
different pre-link derived-destructor targets, which are themselves identical
tails. This supports the derived-input tail as a representative identity;
actual selected ICF population and addresses remain linked-stage obligations.
The manifest's two `0x41a3f0` entries now select the network-input destructor,
and its source-trace edge now attaches to that class instead of the numeric
base (target synchronization revision 5253, topology-only revision 5254).
No owner or provider classification was changed by that correction.

The complete numeric-base-constructor xrefs include four additional retail
construction sites. Cheat-dialog `0x406d20` uses its own `0x4ccc10` table,
capacity 80, then enables input and raw capture. Save/load constructors
`0x434680` and `0x434b90` use `0x4d1370`, capacity 20 and the same enabled-capture
extension. New-game panel `0x41c290` constructs the numeric base and writes
`0x4d0290` without the network buffer/update/disable initialization sequence.
None supplies positive evidence of another reusable network-input layer.

The existing `/Ob2 /Gr` profile was rechecked once against the now-corrected
caller in `mission-corrected-caller-ob2-diagnostic-01`. It still expands
game-name construction and replaces the required clamped-constructor calls
with intermediate-constructor calls for kills/max players. It remains rejected
for the mission TU; no production flag or source-inline change was retained.

## Byte-level caller context after the current rechecks

The fresh canonical `mission-current-native-byte-feedback-01` compilation was
inspected with the existing `verify asm --obj --cod --symbol` diagnostic.
This uses the complete registered mission TU, not an isolated constructor
source or a new test target. The panel has the same 1728-byte instruction
extent as retail after padding is excluded, with 147 differences outside
relocation fields. The first difference is at `0x419d1c`: retail writes the
derived table before the reconfiguration field, whereas the candidate's
member initializer is written before the table. Moving that scalar assignment
to the constructor body is a concrete source-structure differential, not an
inlining directive.

The three range-initialization pairs also differ in base register: retail
uses the current input in EBP, while candidate direct member stores use the
enclosing panel in ESI. The max-player step bindings show the converse issue:
retail stores through the panel, while the source-local configuration helper
emits stores through the button pointer. These observations motivate a bounded
local-pointer/direct-assignment correction before repeating constructor
inlining experiments. No such pending correction is accepted by this note.

Related diagnostic comparisons already match outside relocation fields at
`0x41a2d0`, `0x41a820`, and `0x41a9c0`. Preserve those bodies when investigating
the panel. The step handler `0x41a350` has 16 unmasked differences around its
addition/clamp register allocation, with the same 119-byte instruction extent
after nine padding NOPs. The historical separate-value and direct-expression
variants generated the same divergent allocation, so repeating only those
spellings adds no evidence.

The generation-95 full replay at
`call-contract/401000-408210-r5271-replay-001` compiles all 96 targets and
commits 639 passing bodies through the first divergent slice, ending at
transaction revision 5274 (semantic/evidence 5271/5271). The first divergence
remains `0x41a200`. The complete proof also reports later failures across
Player, RecoilApp, class/UI code, and several provider/switch bridges; it is
not a clean remainder. In particular, the Player constructor/vector/accessor
family now has physical call-count failures; zInput reports two switch-CFG
conflicts; DirectPlay `0x48afe0` reports a non-forward switch edge; and the
prepared loader still has ten rather than twelve calls. WOL-specific work is
deferred. Later slices and closeout are not accepted by this stopped replay.

`mission-panel-body-reconfigure-order-01` moves the scalar reconfiguration
assignment to the constructor body. It preserves 120/120 order and removes
exactly the first seven differing panel bytes, leaving 140. This directly
confirms the retail table/store sequence without introducing a compiler flag
or new source helper.

The same replay's later diagnostic divergence addresses are retained here to
avoid inferring that an unaccepted suffix is clean: Player `0x41eb30`,
`0x41eb90`, `0x41ecd0`, `0x41efa0`, `0x420c60`, `0x425060`, `0x429f80`,
`0x42a070`, `0x42a2c0`, `0x42a480`, `0x42a4a0`, `0x42a4b0`, `0x42a4d0`;
application `0x430250`, `0x432d60`, `0x4349a0`, `0x434a80`, `0x434df0`;
utility/provider `0x438350`; deferred WOL `0x43e1c0`; class `0x442d00`,
`0x443730`, `0x4437d0`, `0x443900`; input `0x46f450`, `0x46f690`; network
`0x48afe0`, `0x48daf0`; weapon `0x4ae660`, `0x4aee40`; UI `0x4b59f0`,
`0x4b7340`, `0x4b87c0`, `0x4bac10`, `0x4bb790`, `0x4bbca0`, `0x4bd020`,
`0x4bd2d0`, `0x4bf060`, `0x4bf560`; interpreter `0x4c5550`. These are
historical failures from one fresh diagnostic, not new tracker classifications
or workspace issues. Each still needs a scoped source/tool diagnosis.

The panel-local shared-input-pointer differential
`mission-panel-input-pointer-direct-bindings-order-01` preserves 120/120 order
but folds away retail clamp checks, shrinking the candidate to 1648 bytes.
That pointer unification was reverted. Direct step bindings alone preserve
the checks and reduce unmasked differences to 110. A separate input-owned
paired-range operation in `mission-input-owned-range-operation-order-01`
recovers the EBP-based stores without folding the checks, reducing differences
to 84. Its descriptive `SetRange` spelling is not a recovered symbol.

Modeling the repeated target/step pair as a step-button operation restores the
earlier time/kills scheduling but leaves later max-player differences (114
bytes); consistent enable-and-refresh use removes three more differences.
Moving that existing operation from a free helper to nonvirtual
`HudUiZrdWidget::SetEnabled` leaves output unchanged. The current
`mission-native-widget-enable-order-01` comparison preserves 120/120 order,
111 panel differences, and all three matching bodies `0x41a2d0`, `0x41a820`,
`0x41a9c0`. No vtable slot or layout changed, no new standalone retail helper
was claimed, and the new source operations have not accepted owner/model gates.
This materially revised caller context supports one new inline-constructor
differential; it does not itself resolve the constructor asymmetry.

With that revised caller context, the explicitly inline shared constructor in
`mission-native-panel-inline-constructor-order-01` preserves 120/120 order and
all 65 panel calls, while `0x41a200` matches outside relocation fields with
the required four calls. The full fresh current-slice diagnostic
`call-contract-mission-native-panel-inline-g95-01` passes all 160 bodies.
Fresh direct live acceptance at `call-contract/415ac0-41cb50-r5275` records
the previously failing constructor at transaction 5275, semantic/evidence
5272/5272. This resolves the current mission call-contract blocker, not the
remaining 111 panel byte differences or any authored-byte stage acceptance.
