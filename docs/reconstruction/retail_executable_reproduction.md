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
authored-object byte. A blocked primary does not suppress compatible bytes;
full authored byte wins over an overlapping new object packet. Capacity and
resource-conflict skips are tool-owned. Render and launch each returned real
reservation with `python tools/recoil.py progress handoff --packet-id
<packet-id> --json`. Individual `--lane <primary|authored|object>` claims
remain available for focused retries and explicit assignments.

The sole scheduler exposes two independent monotonic lanes. The primary order
lane runs `authored-function-order` and then `full-function-order`. The
authored-byte lane independently follows retail physical address groups and may
be exposed as `parallel_authored_byte_cursor` while either order phase is
primary. It pauses, without skipping ahead, when its next row shares the active
order block. `fallback_authored_byte_cursor` is a deprecated compatibility
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
tier acceptance. `progress handoff --json` renders only a real active
reservation and never exposes a parent mutation command.

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

Ordinary comments, `Purpose:` prose, and narrative function labels are not
order drift. Exact address-bearing provenance markers remain semantic input:
`Reimplements 0x...`, `Emits 0x...`, and provider/compiler marker addresses
must still name the expected identity, so removing or changing such an address
fails strict order validation. A narrative-only name change does not stale
accepted order; a function/address/classification/order expectation change
does.

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

For a bounded whole-link COMDAT attribution problem, census existing diagnostic
builds before rebuilding:

```powershell
python tools/recoil.py audit vc5-comdat --run <diagnostic-run-directory> --run <comparison-run-directory> --symbol <exact-decorated-identity> --pattern <fullmatch-regex> --output-dir <evidence-directory>
```

The command writes `opening_object_manifest.json` and
`opening_comdat_census.json`. It preserves object ordinals, compile profiles,
relevant section/COMDAT metadata, bytes and relocation signatures,
recoverable direct-object references, and limited final-map
address/alias/provider observations. It deliberately does not infer discarded
same-name definitions, the pre-`/OPT:REF` survivor set, or causal
`/OPT:ICF` winner history that the observed object/map outputs do not expose.
Missing object files remain explicit instead of silently substituting a
different run.

When a fold discriminator is needed, run two fresh isolated compile/link
diagnostics from the same reviewed source, one for each profile:

```powershell
python tools/recoil.py verify final-build --build-dir <fresh-icf-root> --link-profile vc5sp3_ref_icf
python tools/recoil.py verify final-build --build-dir <fresh-noicf-root> --link-profile vc5sp3_ref_noicf
```

Keep every source and compile-profile option identical except the intentional
link-profile difference. The `/OPT:NOICF` result is a diagnostic control and
never replaces the production `/OPT:ICF` linked gate.

### 2. `authored-byte-match`

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

This dry-run-first reviewed mutation binds an existing target or an exact
known-extent target identity for live expectation derivation. Repeat with
`--apply` after review. `relocation-exception set` remains reserved for genuine
ambiguity; neither route learns expected facts from candidate output.

Accept only the current authored/lifecycle row or a contiguous authored bundle
beginning there after filtering out intervening proven non-authored rows. Owner
`Reimplemented [S]` remains governed by its complete unified-tracker owner/data
gates. Entry-local authored-byte evidence does not imply owner `S`, provider
acceptance, or later full/link prefix acceptance.

### 3. `full-function-order`

After authored function order is complete, restart at `0x401000` without
waiting for authored-byte traversal. This phase
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

### 4. `linked-byte-match`

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

### 5. `final-validation`

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
manually populated legacy catalog blob. It reports exact file-backed and
loaded-RVA gaps, overlaps, unknown extents, ambiguous padding, missing
providers, and unresolved entities. Persist only narrow reviewed ambiguity
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
that same semantic result. Optional `--dry-run` is diagnostic. Other narrow
owner, block, provider, classification, ambiguity-exception, tier, blocker,
semantic, and work mutations remain dry-run-first. `--expected-revision` is the
sole concurrency guard. Durable observations record semantic facts and direct
evidence paths when needed; they never store a concrete `.devspace`
dependency. Evidence generation never promotes owner gates, tiers, provider
classification, global prefixes, or `Model:` metadata.

A source-worker/verifier handoff is a compact mode-specific packet backed by a
real active reservation and lease. `progress handoff --json` fails when that
state is absent, claims no writable path, or contains a mutating worker command;
it never fabricates current work. An `order-edit-v1` packet carries only packet
id, registered target, exact writable source/header closure, isolated root,
objective, stop condition, and its `verify vc5-order` command. Results return
packet id, outcome, changed paths, exact validation result, first divergence,
and concrete scope contradictions. Ordinary owner, final-data/final-repro,
functional, and `messages.dll` queues are `deferred_by_pipeline_phase` unless
required by the current Recoil.exe cursor.
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
