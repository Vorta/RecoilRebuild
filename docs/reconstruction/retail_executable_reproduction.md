# Retail Executable Reproduction

## Goal

The workspace is complete only when source-faithful VC5SP3 C/C++ reproduces
every catalogued retail semantic fact: functions and source blocks, selected
linked contribution order and addresses, code and data bytes, relocations and
targets, providers, resources, directories, padding, and typed PE layout.
The linker-written COFF timestamp and the resulting raw file difference are
diagnostic only. Post-link patching is forbidden.

Original retail COFF objects are unavailable. Prove the executable-bearing
facts that can be observed: the exact function/contribution inventory and
order, object bytes and relocations, linked addresses and bytes, provider
selection, and final executable identity. Do not claim equality of unavailable
COFF metadata.

## Authoritative Sequence

Run `python tools/recoil.py progress next` before any other no-target view.
`.agent/RECONSTRUCTION_PROGRESS.json` is the only reconstruction-progress
authority. It stores distinct linked physical-block, semantic-span, symbol,
source-owner/gate/tier, output-section, physical-storage-contribution,
verification-target, work-item, blocker, and semantic-observation entities. Binary
Ninja remains binary authority. Data symbols, owner data gates, physical
storage, PE sections, and final-image acceptance never imply one another.
Unknown extents retain their start address and `extent_state=unknown` but omit
size and end; a fabricated one-byte extent is forbidden.

A bare `Start` is a complete root-parent launch request. Without an explicit
target, the parent runs `progress next --json` and `progress work leases
--json`, computes remaining child slots from effective runtime capacity, and
immediately applies the compatible multi-lane claim without another user
confirmation:

```powershell
python tools/recoil.py progress work claim-current --lane all --max-packets <available-child-slots> --expected-revision <revision> --apply --json
```

Claim priority is fixed as primary order, full authored byte, then subordinate
authored-object byte; here the primary packet category means the current
primary lane, order or call-contract. A blocked primary does not suppress
compatible bytes;
full authored byte wins over an overlapping new object packet. Capacity and
resource-conflict skips are tool-owned. Render and launch each returned real
reservation with `python tools/recoil.py progress handoff --packet-id
<packet-id> --json`. Individual `--lane <primary|authored|object>` claims
remain available for focused retries and explicit assignments.

The sole scheduler exposes one six-stage pipeline across two independently
monotonic lanes. The primary source
lane runs `authored-function-order`, then `authored-call-contract`, and only
then restarts `full-function-order`. The
authored-byte lane independently follows retail physical address groups and may
be exposed as `parallel_authored_byte_cursor` while order or call-contract work
is primary. It pauses, without skipping ahead, when its next row shares an
active primary source-edit block. `fallback_authored_byte_cursor` is a deprecated compatibility
alias for that same parallel cursor, not an accepted-prefix prerequisite.

While an order phase is primary, `progress next` may additionally expose the
subordinate `parallel_authored_object_byte_cursor`. It scans only complete
physical blocks inside the current contiguous, semantically accepted
authored-order prefix and selects the first gating address group whose
`object_byte` dimension is not semantically accepted. Run `progress
advance-live-byte --lane object --build-root <fresh-root>
--expected-revision <revision> --apply`; it compiles current source in a new
isolated root, directly checks that exact cursor, and revision-atomically
accepts from the same in-memory result. Optional `--dry-run` is diagnostic only;
do not routinely build twice. A failure
expands the subsequent source repair handoff to the complete owner; that is not
batch acceptance. The live check uses production source, the registered
manifest/profile/toolchain, exact COFF extent,
body bytes, mask, and structurally observed relocation rows. It accepts only
`binary_state.object_byte` and explicitly claims no relocation identity,
linked state, order, owner/model, tier, or final result. A current launchable
overlapping full authored-byte packet has priority over new object preparation.
Object preparation may run only when the full-byte lane is unavailable or
blocked, no matching full-byte packet exists, compatible full-byte work is
active on proven non-overlapping surfaces, or the object packet is an explicit
exact-row prerequisite. Stale packets fail closed, active work is never
preempted, generated outputs cannot overlap, and whole-project builds remain
parent-serialized. Recompute the scheduler after every object-only packet
instead of automatically preparing another row.

### 1. `authored-function-order`

Start at `0x401000` and advance monotonically through retail `.text` ending at
`0x4cb9e8`. Before a selected retail function/address group can be skipped,
classify it as one of:

- `authored`: a function body attributable to original Recoil source;
- `authored-lifecycle`: an authored-class or authored-object lifetime
  contribution whose explicit `authored_order_role` distinguishes an actual
  authored lifecycle body from a compiler-generated/lowered companion;
- `non-authored`: a proven compiler, runtime, import, framework, SDK, provider,
  padding, or data contribution with no authored Recoil body; or
- `unresolved`: evidence is insufficient to choose one of the other classes.

Classification is row-scoped traversal evidence. It does not accept a provider
boundary, source owner, source block, owner gate, tier, source-faithful model,
byte result, or final result. An unresolved retail row blocks authored-order
traversal; an unclassified selected linked contribution blocks later exact
full-order population acceptance. Classify from current BN,
provider/compiler, class/lifecycle, vtable/message-map, source-block, and VC5
emission evidence; provider-looking bytes alone do not make a row non-authored.
For linked authored-order projection, an unclassified alias that maps to a
retail identity inside the active interval blocks the scoped gate. An unmapped
selected alias elsewhere keeps the global projection incomplete, but does not
acquire membership in the active retail interval merely because its candidate
address lies between two selected sentinels.

One unresolved row anywhere in a registered target's covered interval blocks
the whole physical-block packet and its live acceptance. A raw-object check of
only the resolved subset may report a useful diagnostic PASS, but that partial
result cannot launch or accept the covered block.

`authored_order_role` defaults fail-closed from the historical class:
`authored` becomes `authored-body`, and `authored-lifecycle` becomes
`authored-lifecycle-body`. Only those two roles gate authored raw/linked order
and authored-byte traversal. Use an explicit
`compiler-generated-deleting-variant`, `compiler-generated-eh-helper`,
`compiler-generated-thunk`, `compiler-generated-implicit-cleanup`, or
`compiler-generated-icf-representative` role only when current evidence proves
that exact generated kind. Such a row remains required and inventoried for its
class/block family and later exact full order, but its raw placement, linked
relative position, and ICF representative do not block the authored-first
phases.

For every physical block, compile the reconstructed translation unit with
VC5SP3. Every expected `authored-body` or `authored-lifecycle-body` gating
identity must resolve exactly once and those identities must retain their
natural retail relative order. Missing, duplicated, reordered, or unexpected
gating contributions fail this phase. Proven `non-authored`
contributions remain inventoried but their exact linked position, RVA, and seam
are deferred to `full-function-order`.

At the raw-object/TU gate, inventory every unlisted raw definition, but treat it
as mechanically non-blocking because VC5SP3 `/OPT:REF` can discard and
`/OPT:ICF` can fold compiled COMDATs after object emission. Expected gating
identities still resolve exactly once; every expected gating identity retains
the phase-appropriate relative order. An object pass proves no source owner,
tier, linked placement, byte
identity, provider classification, or final acceptance. Linker removal also
does not make arbitrary dead authored source a source-faithful model; that is a
separate source-owner judgment.

#### Fast authored-order source loop

The source worker's feedback loop is a direct compile-and-compare cycle:

```powershell
python tools/recoil.py verify vc5-order <registered-order-target> --build-root <packet-root>
```

The parent obtains real compact packets through the no-target multi-lane claim
above, or a focused `progress work claim-current --lane primary ... --apply`,
then renders the selected reservation with `progress handoff --packet-id
<packet-id> --json`. The order packet names that one command, the exact writable
source/header closure, an isolated build root, objective, and stop condition.
A registered target may cover one translation unit and several explicit,
contiguous physical-block slices. The worker may reshape the assigned
`.h`/`.cpp` layout, definition placement, include timing, and calls, then rerun
the same command as often as needed. The verifier resolves the retail decorated
identities in the newly compiled VC5SP3 output and prints `PASS` or `FAIL`,
followed on failure by the first blocking identity/order divergence and its
expected and candidate neighbors. Detailed raw inventory follows when needed.

This loop does not open Binary Ninja, compare bytes, produce or import an
acceptance artifact/evidence package, qualify a candidate hash, mutate the
tracker, or call ChatGPT Pro for routine order feedback. A passing target
comparison is immediate source feedback, not linked, byte, owner, model, or
tier acceptance. `progress handoff --packet-id <packet-id> --json` renders only
a real active reservation and never exposes a parent mutation command.

Authored relative order uses the same fold-aware predicate in the canonical
report, the live verifier, and tracker acceptance. A `selected-winner` or
`not-established` logical identity remains in the relative sequence. A proven
fold/cofold alias remains required inventory and must resolve exactly once, but
it does not impose a second physical relative-order position. Full order is
unchanged and inventories every selected linked contribution.

The linked authored-order check is a retail-identity partial-order projection,
not a candidate-address corridor scan. It joins decorated manifest selectors
to the unified tracker's current retail row and logical-ICF-alias roles, then
classifies every selected MAP alias independently. A non-authored alias never
transfers that class to authored or unresolved aliases folded at the same
candidate address. Required authored identities in the active interval still
need linked presence, and their known authored relative order and block
precedence remain gating. A mapped later-retail authored identity selected
before an earlier active identity is a real inversion and blocks. Unknown MAP
aliases remain explicit and set global `linked_projection_complete=false`, but
they are not treated as interval extras unless a retail mapping places them in
the interval.

The parent independently rebuilds and advances the complete target-covered
physical slices through one phase-aware live command:

```powershell
python tools/recoil.py progress advance-live-order --target <tracker-target-id> --build-root build/live-order/<cursor>/<target> --expected-revision <revision> --apply
```

If the current block's phase-appropriate `order_targets` field is empty or
stale, pass a reviewed command-scoped exact-range override instead of mutating
the tracker just to launch the check: `--object-target
<registered-object-target>` during authored order or `--linked-target
<registered-linked-target>` during full order. `--target` remains the tracker
acceptance target id. The override accepts an exact target id or one unique
registered name and must match the acceptance target's binary, half-open range,
phase scope, contiguous blocks, and tracker identity sequence. `progress next`
emits the override automatically only when exactly one registered target proves
that contract. Zero or multiple matches produce a typed blocked cursor, never a
launchable cursor with an empty command.

The advancement command owns the serialized current build. It validates once,
prints the first typed divergence and neighboring identities on failure, and
CAS-advances only complete covered slices from that same in-memory semantic
result while the tracker remains at the supplied revision. Target failure
accepts none of its slices. Direct `--apply` is normal; optional `--dry-run` is
diagnostic only. `--expected-revision` is the sole concurrency guard. Raw authored order stays
explicitly separate from linked presence, known-authored relative order,
scoped projection completeness, and block precedence. Exact selected
population plus linked seams/RVAs remain `full-function-order` work. Archived
order observations are historical context only and cannot drive current
acceptance without a live recheck.

Ordinary comments, `Purpose:` prose, narrative labels, and source-trace
descriptions are not order drift. The canonical `@recoil-anchor` and
`@recoil-artifact` rows are validated mirrors of tracker-owned source/artifact
relationships. Changing an artifact id, relationship, source anchor, translation
unit, or expected output section is semantic input and fails strict validation
unless the tracker graph changes through its reviewed mutation path. Changing
only a human description does not stale accepted order. Legacy
`Reimplements 0x...`, `Reimplements data 0x...`, and `Emits 0x...` address
markers are invalid after migration.

One source anchor may have many artifact rows, including compiler-generated
functions and `.rdata`/`.data` artifacts. Each row names exactly one physical
or reviewed logical artifact; lists of addresses are never packed into one row.
Trace relationships document emission topology only. They do not accept
function order, bytes, data extents, storage, provider classification, owner
gates, source shape, tiers, or final-image coverage.

Canonical MFC provider checks use VC5SP3 `/E` line directives because that
compiler does not implement `/showIncludes`. Every official project/build path
must resolve `AFXWIN.H` only from
`D:/Recoil Project/Compiler/VC5SP3/VC/MFC/INCLUDE`; the `support/sdk/MFC42` and
`D:/Recoil Project/Visual C++ 5.0` header trees are evidence only. A matched
alternate RTM `MFC42.LIB`/`MFCS42.LIB` pair may be an explicit diagnostic
library profile, never an acceptance provider selection or alternate header
root.

Correct source owners, physical blocks, header layering, helper placement,
linkage, and include timing whenever authored order shows that existing source
shape is wrong. Never force order with `.inl`, pragma/linker tricks, wrong-file
helpers, fake wrappers, or duplicate bodies. Registered target execution,
mechanical first-divergence interpretation, current-source rechecks, and edits
to an already-reviewed model are ChatGPT Pro-exempt. Escalate only under the
ambiguity-driven triggers in root `AGENTS.md` and
`recoil-source-model-recovery`.

Stop order work at the first order divergence. Independently, `progress next`
scans retail address groups for the earliest unaccepted authored byte gate. It
stops fail-closed at unresolved classification, skips proven `non-authored` and
explicit compiler-generated roles, and exposes
`parallel_authored_byte_cursor` only when that row's physical block differs
from the active order block. Byte acceptance never advances order state, and
order acceptance never advances byte state.

For a bounded whole-link COMDAT attribution problem, run two fresh isolated
compile/link diagnostics from the same reviewed source, one for each profile:

```powershell
python tools/recoil.py verify final-build --build-dir <fresh-icf-root> --link-profile vc5sp3_ref_icf
python tools/recoil.py verify final-build --build-dir <fresh-noicf-root> --link-profile vc5sp3_ref_noicf
```

Keep every source and compile-profile option identical except the intentional
link-profile difference. The `/OPT:NOICF` result is a diagnostic control and
never replaces the production `/OPT:ICF` linked gate. Inspect the corresponding
OBJ section/COMDAT records and linker MAP symbol/provider/alias rows directly;
do not substitute a retired aggregate wrapper or infer discarded same-name
definitions and causal ICF winner history that the observed OBJ/MAP artifacts
do not expose.

### 2. `authored-call-contract`

After every authored-order physical block is current, derive the live reviewed
census of `authored-body`/`authored-lifecycle-body` physical bodies in retail
order.
Logical ICF aliases remain linked identities on their one physical row; they do
not create duplicate call-contract bodies. The tracker migration initializes
one independent `binary_state.call_contract` dimension for each required body
and no others. Slices are deterministic, contiguous, and capped at 160 bodies
so one retail Binary Ninja assembly request per body stays within the governed
bridge budget.

The one-time migration used the reviewed 3,380-body census at that revision.
That value is historical initialization scope, not a permanent stage count.
The parent reviewed and applied that additive initialization through:

```powershell
python tools/recoil.py progress call-contract initialize --expected-revision <revision> --dry-run --json
python tools/recoil.py progress call-contract initialize --expected-revision <revision> --apply --json
```

The one-time route fails unless authored order is complete, full-order
acceptance remains zero, the migration census is exactly 3,380, and no partial
prior initialization exists. It preserves all existing order and byte facts.
Thereafter the permanent stage derives its current required population from
current reviewed classification.

The worker loop is:

```powershell
python tools/recoil.py verify call-contract --slice <slice-id> --progress .agent/RECONSTRUCTION_PROGRESS.json --build-root <fresh-worker-root> --json
```

It freshly compiles the accepted authored-order target/source closure and
compares the ordered static invocation signature of every body with current
retail Binary Ninja assembly. Signatures include exact call count/order,
direct versus indirect dispatch, direct/self/base/provider/IAT target identity,
virtual/interface slot displacement, callback storage identity, call versus
tail form, and known caller cleanup. Intra-body branches are ignored.
Unresolved caller, callee, provider, import, callback-storage, physical/logical
alias, or slot identity blocks; candidate output never supplies expected
truth.

The parent accepts from one fresh result:

```powershell
python tools/recoil.py progress advance-live-call-contract --slice <slice-id> --build-root <fresh-parent-root> --expected-revision <revision> --apply --json
```

The command exact-guards ordered slice membership, physical blocks, accepted
authored-order targets, source/header closure, and manifest dependencies.
Content-hash-free path/existence/size/mtime signatures are captured before the
build, rechecked after validation, and rechecked immediately before CAS. They
are staleness diagnostics and concurrency guards, never expected call truth or
candidate qualification. A PASS accepts only `call_contract` plus its narrow
facts/evidence. It does not accept or revoke order, byte, owner, provider,
gate, tier, storage, or final-image state. Full order remains blocked until all
bodies in the current reviewed live gating census are current; compatible
authored/authored-object byte work may continue independently.

### 3. `authored-byte-match`

Independently scan from the first retail function/address group for the first
unaccepted `authored-body` or `authored-lifecycle-body`, stopping at unresolved
classification and skipping proven `non-authored` rows plus explicit
compiler-generated roles. A physical ICF/address group is one traversal step;
logical aliases do not create duplicate byte steps. Traverse in retail order
while editing and accepting the complete source-shaped owner. Preflight the
expected relocation semantics and advance the live lane with:

```powershell
python tools/recoil.py audit relocation-expectations --at <cursor> --json
python tools/recoil.py progress advance-live-byte --lane authored --build-root build/live-byte/authored/<cursor> --expected-revision <revision> --apply
```

The command compiles and links current source, compares exact extent and object
bytes outside relocations, relocation types, symbol/provider/alias identities
and addends, linked presence, symbolic call/reference target identity, and
relocation-normalized linked body bytes at the candidate address. Expectations
are derived live from immutable retail plus accepted typed identity/provider/
alias facts, never from the candidate. An explicit empty set is valid. Genuine
ambiguity blocks before the build and routes to the narrow reviewed exception
command:

```powershell
python tools/recoil.py progress relocation-exception set --source-symbol-id <physical-symbol-id> --source-address <cursor> --payload-json '<json-object>' --expected-revision <revision> --dry-run --json
```

The governed row binds exact current source/target extent, object registration,
pipeline/provider/alias context, and evidence ids. Drift produces a typed stale
exception instead of a hash failure. After review, repeat with `--apply`.

One narrower mode covers one or more proved relocation sites targeting a
physical `.rdata` object whose original VC5 object-symbol provenance is still
unresolved:
`exception_mode=physical-target-unresolved-vc5-temporary`. The reviewed input
contains the current source object symbol, the complete sorted `offsets`,
DIR32 `type`, physical `target_symbol_id`, zero COFF and resolved-target
addends, exact retail target, reason, and tracker evidence ids. It must not
contain a target object symbol, candidate ordinal/RVA, caller-supplied regex,
witness contract, or binding snapshot. For the current `0x402250` review:

```powershell
$payload = '{"reviewed":true,"exception_mode":"physical-target-unresolved-vc5-temporary","object_symbol":"?TickAiMode2AltGunAttackWindow@AINet@@SIXPAUzUtil_SaveGameState@@MM@Z","offsets":[245,276],"type":6,"target_symbol_id":"recoil:data:0x4cc820","coff_addend":0,"resolved_target_addend":0,"retail_target":"0x4cc820","reason":"Retail proves one exact four-byte physical 0.5f target at both DIR32 sites while original VC5 object-symbol provenance remains unresolved.","evidence_ids":["recoil:evidence:r725:000465"]}'
python tools/recoil.py progress relocation-exception set --source-symbol-id recoil:function:0x402250 --source-address 0x402250 --payload-json $payload --expected-revision <revision> --dry-run --json
```

The mutation redecodes immutable retail and requires that `(245, DIR32)` and
`(276, DIR32)` are the complete site/type population targeting
`recoil:data:0x4cc820`. It derives the exact current source snapshot and a
physical target snapshot containing `0x4cc820..0x4cc824`, `.rdata`, known
four-byte extent, ownership state, and retail bytes `0000003f`. It also derives
the fixed admissibility contract: one repeated `$T<digits>` STATIC/type-0
symbol in initialized, non-writable `.rdata`. These derived fields are stored
for transparent staleness checks; the unrelated named `$S` binding from
another reader is not asserted as this source function's original object
provenance.

When the exact four-byte retail target has no tracker data identity yet, the
same reviewed payload may include an atomic `create_missing_data` request:
`target_owner_id`, `target_end_exclusive`, and `target_name`. The tool reuses
the relocation-target binding guards. It requires the deterministic
`recoil:data:0x<address>` identity, exact `[retail_target, retail_target+4)`
extent, file-backed retail `.rdata`, no existing or overlapping symbol, an
existing non-provider owner, and evidence that is both present on and scoped to
that owner. `target_name` is a neutral navigation label using only C/C++
identifier and namespace components; raw `$T`/`$S` object symbols, ordinals,
patterns, regex syntax, and candidate-derived fields are rejected.

Dry-run and apply stage one revision-atomic proposal containing only the pending
data symbol, its exact owner `primary-data` relationship, and the reviewed
exception plus derived non-tier staleness context. The input creation request
is not stored. The mutation does not update owner gates, owner tiers, provider
state, source ownership, order, byte acceptance, or final-image acceptance.
Later owner/relationship/evidence drift makes the exception stale.

The two current `0x4024a0` single-site requests are:

```powershell
$payload = '{"reviewed":true,"exception_mode":"physical-target-unresolved-vc5-temporary","object_symbol":"?SolveAltGunLeadTargetPoint@AINet@@SIXPAUzUtil_SaveGameState@@0PAUzVec3@@@Z","offsets":[517],"type":6,"target_symbol_id":"recoil:data:0x4cc838","coff_addend":0,"resolved_target_addend":0,"retail_target":"0x4cc838","reason":"Retail proves one exact anonymous four-byte physical float target at this DIR32 site while original VC5 object-symbol provenance and source identifier remain unresolved.","evidence_ids":["recoil:evidence:r725:000465"],"create_missing_data":{"target_owner_id":"recoil:owner:battlesport_gameplay.player_ai_mode2_top_level_steering","target_end_exclusive":"0x4cc83c","target_name":"g_AINetSolveAltGunLeadTargetPoint_FloatLiteral_4CC838"}}'
python tools/recoil.py progress relocation-exception set --source-symbol-id recoil:function:0x4024a0 --source-address 0x4024a0 --payload-json $payload --expected-revision <revision> --dry-run --json

$payload = '{"reviewed":true,"exception_mode":"physical-target-unresolved-vc5-temporary","object_symbol":"?SolveAltGunLeadTargetPoint@AINet@@SIXPAUzUtil_SaveGameState@@0PAUzVec3@@@Z","offsets":[529],"type":6,"target_symbol_id":"recoil:data:0x4cc83c","coff_addend":0,"resolved_target_addend":0,"retail_target":"0x4cc83c","reason":"Retail proves one exact anonymous four-byte physical float target at this DIR32 site while original VC5 object-symbol provenance and source identifier remain unresolved.","evidence_ids":["recoil:evidence:r725:000465"],"create_missing_data":{"target_owner_id":"recoil:owner:battlesport_gameplay.player_ai_mode2_top_level_steering","target_end_exclusive":"0x4cc840","target_name":"g_AINetSolveAltGunLeadTargetPoint_FloatLiteral_4CC83C"}}'
python tools/recoil.py progress relocation-exception set --source-symbol-id recoil:function:0x4024a0 --source-address 0x4024a0 --payload-json $payload --expected-revision <revision> --dry-run --json
```

Each request remains one site because the two DIR32 operands resolve to
different four-byte physical targets. Immutable retail supplies bytes
`00010038` at `0x4cc838` and `000000c0` at `0x4cc83c`; those values are derived
and reported by the tool rather than copied into either request.

Live authored-byte verification retains the exact complete source-body
relocation multiset, object-body comparison outside relocation fields,
one-to-one raw-witness/physical-target mapping, linked symbolic placement, and
normalized linked-target byte comparison. It separately derives the complete
immutable-retail reader universe for the physical target across the current
registered object. The witness symbol's complete object-wide incoming set must
be a unique, nonempty subset of that universe, must contain both reviewed
`0x402250+245` and `0x402250+276` sites, and must map every actual row
one-to-one by exact registered object symbol, function offset, DIR32 type, and
zero addend to a retail-proved reader. Thus VC5 may pool the witness with other
proved readers without making the candidate-selected subset expected truth.

A missing reviewed site; an outside, unresolved, duplicate, provider, alias,
wrong-type, wrong-addend, or unregistered reader; mixed or reused witnesses;
`$S` substitution; duplicate storage; writable/uninitialized/wrong-name
sections; internal data relocations; content/extent/type/storage drift;
physical target drift; source registration drift; or missing evidence blocks
fails closed. No rule globally canonicalizes `$T` and `$S`, and neither the
observed candidate ordinal nor its selected reader subset is stored as expected
truth.

The live byte command builds and validates once, then advances explicitly
matched physical groups from that same result. Optional `--dry-run` is
diagnostic only. Exact retail RVA and fully resolved relocation operands are
diagnostic here: they become blocking in the full-order and resolved
linked-byte phases. Ordinary comments and file identity do not participate;
the current tracker revision is the sole concurrency guard.

If retail already determines the operand but the tracker is missing its typed
target identity, use `progress relocation-target bind`, not an ambiguity
exception:

```powershell
python tools/recoil.py progress relocation-target bind --source-symbol-id <physical-symbol-id> --source-address <cursor> --payload-json '<reviewed-binding>' --expected-revision <revision> --dry-run --json
```

If that operand is a retail-proven named-function IAT slot and no target row
exists yet, first create the typed provider package through its separate
dry-run-first mutation:

```powershell
python tools/recoil.py progress provider-target register --address <iat-address> --payload-json '<reviewed-provider-target>' --expected-revision <revision> --dry-run --json
```

`provider-target register` parses immutable `support/Recoil.exe`, requires the
exact IAT address plus DLL/import name, derives the four-byte storage and
one-byte callable provider views in their retail section, and creates current
evidence plus an accepted provider-boundary owner with exact anchor,
primary-function, and primary-data relationships. Its reviewed VC5 `__imp_`
object identity must come from provider declarations/evidence, never candidate
output. Review and apply that proposal, then run `relocation-target bind` at
the next tracker revision to bind the individual call site. The two mutations
remain separate: provider registration creates no call-site binding, while
relocation binding creates no provider inventory.

For a statically linked VC5 provider body that already has an exact
known-extent non-authored retail function row, use the distinct provider
function registration route:

```powershell
python tools/recoil.py progress provider-function register --address <function-address> --payload-json '<reviewed-provider-function>' --expected-revision <revision> --dry-run --json
```

The reviewed payload names one normalized `.LIB` path under
`DEFAULT_VC5_ROOT`, one exact `.obj` archive member, one defined external
function symbol, and the new provider owner identity. The command parses the
archive and member directly, requires an exact code-COMDAT natural extent, and
compares immutable retail `.text` bytes outside only supported COFF relocation
fields. It rejects imports/IAT rows, authored or already-owned functions,
overlaps, path escape or archive/member/symbol ambiguity, malformed COFF,
non-code/nonexternal/nonfunction symbols, extent/section/body/relocation
mismatches, candidate-derived payload fields, owner collisions, and revision
drift. Apply records the exact library/member/symbol identity plus current live
evidence on the existing function and its new provider-boundary owner. It does
not create a call-site binding, accept an authored lane, promote a source
owner, or change an owner tier.

`relocation-target bind` remains the dry-run-first route for an existing target
or exact known-extent authored data identity. Repeat with `--apply` after
review. `relocation-exception set` remains reserved for genuine ambiguity;
none of these routes learns expected facts from candidate output.

Accept only the current authored/lifecycle row or a contiguous authored bundle
beginning there after filtering out intervening proven non-authored rows. Owner
`Reimplemented [S]` remains governed by its complete unified-tracker owner/data
gates. Entry-local authored-byte evidence does not imply owner `S`, provider
acceptance, or later full/link prefix acceptance.

### 4. `full-function-order`

After authored function order and every authored call contract are complete,
restart at `0x401000` without waiting for authored-byte traversal. Do not wait
for that independent lane. This phase
covers every selected linked contribution class, including authored,
authored-lifecycle, compiler, runtime, import, framework, SDK, provider,
padding, and data rows inside `.text`. Require the exact selected linked address groups in the
retail address-group set and order with no extra, missing, duplicated, or
reordered group;
exact retail linked RVAs; and exact predecessor/successor seams. The first block
uses `predecessor_section_boundary:
{"section":".text","address":"0x401000"}` instead of inventing a predecessor
function. Padding and data are not fake functions.

The raw-object extra-definition rule remains unchanged: inventory extras, but
do not fail merely because dead/foldable COMDATs exist before `/OPT:REF` and
`/OPT:ICF`. The selected linked population is the gate. Stop at the first full
linked divergence; authored-order or authored-byte acceptance does not excuse a
missing/misplaced non-authored row, address drift, or seam drift.

Use the same live semantic advancement path for full order:

```powershell
python tools/recoil.py progress advance-live-order --target <tracker-target-id> --build-root build/live-order/<cursor>/<target> --expected-revision <revision> --apply
```

The full live result records selected groups, providers, COMDAT/ICF selection,
exact RVAs, and predecessor/successor sentinels as inspectable semantic fields,
then CAS-applies all complete target-covered slices or, on target failure,
accepts none and stops with the first divergence plus candidate neighbors. The
checker evaluates the complete identity population
and computes exact blocker/count state, but large downstream alias, physical-
class, and extra-contribution inventories retain at most 25 prioritized examples
per category plus exact totals and state counts. Console output leads with the
first divergence and bounds follow-on diagnostics. Historical full-order
observations remain readable context, but only a current live semantic recheck
can satisfy linked-byte dependencies.

### 5. `linked-byte-match`

After every full-order row and every authored-byte gating address group are
accepted, restart at `0x401000` with `verify linked-byte`. Traverse every
linked row by retail address. Require exact linked RVA/address, exact resolved
relocation operands, exact symbolic call/reference targets, and exact linked-
image bytes. Earlier authored-byte state remains useful context, but its
relocation-normalized candidate-address comparison cannot substitute for this
resolved retail-address gate. Advance the live lane with:

```powershell
python tools/recoil.py progress advance-live-byte --lane linked --build-root build/live-byte/linked/<cursor> --expected-revision <revision> --apply
```

The command rebuilds and validates once, reports the first typed divergence,
and CAS-advances explicitly matched physical groups from that same result.
Optional `--dry-run` is diagnostic only. It may advance only the
exact `linked_address`, `linked_targets`, and `linked_byte` triplet that
passed. It does not backfill object, authored
relocation, owner, provider, model, or tier state. Provider-library,
compiler-runtime, and non-authored rows fail closed until their archive-member
and symbolic-target catalogs exist.

### 6. `final-validation`

Run one fresh unrestricted build and complete typed comparison:

```powershell
python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil.py audit final-image-catalog --json
python tools/recoil.py verify final-image --json
python tools/recoil.py progress audit --scope pipeline --strict
```

The catalog must cover every retail section and selected contribution without
gaps or overlap. `audit final-image-catalog` derives mechanical PE facts live
from verified retail and joins accepted tracker facts; it does not require a
manually populated legacy catalog blob and returns
`legacy_catalog_required: false`. It reports exact file-backed and loaded-RVA
gaps, overlaps, unknown extents, ambiguous padding, missing providers, and
unresolved entities. Persist only narrow reviewed ambiguity
annotations; deterministic retail facts are rederived and candidate output
never supplies expected facts. `verify final-image` fails before building while
typed coverage is incomplete. When complete, final acceptance requires every
authored/full-order and authored/linked-byte row, exact timestamp-excluded
headers, complete section and directory population, resources, data, imports,
providers, addresses, targets, padding, and overlay from one fresh unrestricted
build. The linker-written COFF timestamp and raw whole-file differences are
diagnostic only. A stale MAP, skipped comparison, or unmodelled range blocks.

To census ordinary compiler failures without linking while required order
targets are active, run `python tools/recoil.py verify final-build --
--compile-only --keep-going --compile-only-skip-linked-order`. This explicit
compile-only diagnostic bypasses linked-order target evaluation only because no
map is produced. It is rejected without `--compile-only` and cannot satisfy
authored order, full order, or final acceptance.
The unrestricted final-validation command above never uses this option.

### Play-test deployment

A normal successful run of the canonical
`tools/_recoil/config/vc5_final_build.json` manifest copies its freshly linked
`Recoil.exe` candidate to `playground/Recoil-rebuild.exe` for manual play
testing. The copy occurs only after compile, resource, link, and all required
linked-order checks succeed. Dry runs, compile-only runs, linked-order-only
runs, diagnostic manifests or profiles, isolated order diagnostics, custom
manifests, companion-DLL builds, probes, and every failed build path do not
deploy.

Deployment is play-test convenience only. The build-root `candidate_path`
remains the authoritative candidate, and the playground copy is not acceptance
evidence for order, bytes, source ownership, tiers, providers, or the final
image. The driver copies through a temporary sibling and atomically replaces
the destination. It does not create a missing `playground` directory. A missing
directory, locked destination, or copy/replace failure preserves the prior
play-test executable where one exists, removes the temporary copy when
possible, prints an explicit non-gating warning, records the result under
`playtest_deploy` in the normal successful canonical summary, and leaves the
successful build return code unchanged.

## Commands and Handoffs

Use:

```powershell
python tools/recoil.py progress next --json
python tools/recoil.py progress status
python tools/recoil.py progress work leases --json
python tools/recoil.py progress work claim-current --lane all --max-packets <available-child-slots> --expected-revision <revision> --apply --json
python tools/recoil.py progress handoff --packet-id <packet-id> --json
python tools/recoil.py progress audit --scope all --strict
```

Only the parent mutates the unified tracker. Active binary-lane advancement
uses direct `--apply` with `progress advance-live-order` and
`progress advance-live-byte`: each validates/builds once and CAS-applies from
that same semantic result. Optional `--dry-run` is diagnostic. Reviewed owner
topology/membership replacement uses dry-run-first `progress owner
replace-batch`; conservative owner gate/tier decreases use `progress owner
downgrade`; verification-target registration metadata uses `progress
verification-target sync`. Unsupported positive owner metadata, gate, or tier
changes route to a bounded `issue request`, never an improvised mutation. Other
governed block, provider, classification, ambiguity-exception, blocker,
semantic, and work mutations remain dry-run-first. `--expected-revision` is the
sole concurrency guard. Durable observations record semantic facts and direct
evidence paths when needed; they never store a concrete `.devspace`
dependency. Evidence generation never promotes owner gates, tiers, provider
classification, global prefixes, or `Model:` metadata.

A source-worker/verifier handoff is a compact mode-specific packet backed by a
real active reservation and lease. `progress handoff --packet-id <packet-id>
--json` fails when that state is absent, claims no writable path, or contains a
mutating worker command; it never fabricates current work. An `order-edit-v1`
packet carries only packet id, registered target, exact writable source/header
closure, isolated root, objective, stop condition, and its `verify vc5-order`
command. Results return
packet id, outcome, changed paths, exact validation result, first divergence,
and concrete scope contradictions. Ordinary owner, final-data/final-repro,
functional, and `messages.dll` queues are `deferred_by_pipeline_phase` unless
required by the current Recoil.exe cursor.

An order manifest may add optional `order_edit_paths` to that writable closure
when natural-order repair requires existing C/C++ sources or headers outside
`source_from` and the tracker source-shape inputs. Entries fail closed unless
they are exact normalized repository-local paths to existing C/C++
source/header files. Synchronization copies the metadata into the registered
target; reservation-backed primary-order packet construction then adds it only
to writable paths and resource claims. The field never changes compilation,
registered symbol identities, covered blocks, order semantics, or acceptance.
Final-data and final-repro are evidence producers, not work units, queues, or
peer schedulers; they never generate owner-action batches.

For workspace-process health, run static and operational checks together:

```powershell
python tools/recoil.py audit agent-surface --strict
python tools/recoil.py audit workflow-contracts --strict
python tools/recoil.py audit pipeline-reachability --strict
```

`agent-surface` validates syntax and canonical references.
`workflow-contracts` exercises command transitions and reservation-backed
handoffs. `pipeline-reachability` checks target-to-slice coverage and verifies
that every required fail-closed consumer has a live expected-fact producer or
reviewed ambiguity route. A static pass alone is not complete workflow health.

## Debt-Free Session Close

Before final response, fix every regression introduced by the session; remove
temporary probes and failed variants; promote material conclusions and direct
evidence/transcript paths with their semantic scope;
leave exactly one truthful cursor/blocker with an exact next command; update or
close structured work items; run touched checks and strict workspace/progress/
agent audits; and
wait for workers/Pro consumers. A source blocker is legitimate backlog only
when its address/range, evidence, failed command/result, accepted pipeline state,
and next command are durable. Agents never clear `.devspace` or make durable
facts depend on session scratch. Do not finalize with stale work items/cursors, hidden failures,
false claims, unregistered evidence, or any other session-introduced debt.
