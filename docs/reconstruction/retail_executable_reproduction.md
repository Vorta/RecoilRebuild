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
`.agent/RECONSTRUCTION_PROGRESS.sqlite3` is the only reconstruction-progress
authority. It stores distinct linked physical-block, semantic-span, symbol,
source-owner/gate/tier, output-section, physical-storage-contribution,
verification-target, work-item, blocker, and semantic-observation entities. Binary
Ninja remains binary authority. Data symbols, owner data gates, physical
storage, PE sections, and final-image acceptance never imply one another.
Unknown extents retain their start address and `extent_state=unknown` but omit
size and end; a fabricated one-byte extent is forbidden.

The independent workspace-process authority is
`.agent/WORKSPACE_ISSUES.sqlite3`. The progress and issue databases have
separate monotonic revisions; a command that mutates one guards that database's
revision, and the paired ledger migration guards both exact revisions. Neither
database is a mirror of the other.

### Direct paired SQLite cutover

The governed cutover converts both legacy JSON inputs and installs both SQLite
authorities as one paired operation:

```powershell
python tools/recoil.py maintenance migrate-ledgers-sqlite --progress-json <absolute-legacy-progress-json> --issues-json <absolute-legacy-issues-json> --progress-db <absolute-noncanonical-progress-db> --issues-db <absolute-noncanonical-issues-db> --expected-progress-revision <revision> --expected-issues-revision <revision> --dry-run --json
python tools/recoil.py maintenance migrate-ledgers-sqlite --progress-json <absolute-legacy-progress-json> --issues-json <absolute-legacy-issues-json> --progress-db <absolute-noncanonical-progress-db> --issues-db <absolute-noncanonical-issues-db> --expected-progress-revision <revision> --expected-issues-revision <revision> --apply --json
```

Review the dry-run before applying with the same paths and exact independent
revisions. The JSON sources remain untouched until the installed database pair
passes post-install validation; delete both legacy JSON files only after that
success. A failure before completion falls back to the untouched JSON sources.
After completion, runtime and rollback are SQLite-only: restore a paired SQLite
backup at the exact progress and issue revisions, or fix forward. Do not add a
runtime JSON backend, JSON export, JSON mirror, or mixed JSON/SQLite mode.

A bare `Start` is a complete root-parent launch request. Without an explicit
target, the parent runs `progress next --json` and `progress work leases
--json`, computes remaining child slots from effective runtime capacity, and
immediately applies the compatible multi-lane claim without another user
confirmation:

```powershell
python tools/recoil.py progress work claim-current --lane all --max-packets <available-child-slots> --expected-scheduler-revision <scheduler-revision> --apply --json
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

A registered VC5 target row is a projection of the reviewed tracker row, not a
second classification authority. Its `pipeline_class` and
`authored_order_role` must agree with the tracker before the target can supply
a call-contract identity; `progress verification-target sync` reports any
registration drift. An ordinary authored method remains `authored`/
`authored-body` even when its name or behavior includes shutdown, reset, or
thunk terminology. Exact constructor/destructor decoration checks continue to
apply to genuinely reviewed `authored-lifecycle` rows and must not be bypassed
by relabeling an ordinary method.

An exact decorated `symbol` in a registered target also preserves the reviewed
calling convention; it is not merely an order label. For example, a static C++
member compiled as `__fastcall` carries a `SIX...` decoration where the stale
`__cdecl` form carries `SAX...`, and the call sites have different cleanup
contracts. Keep every target view synchronized with the symbol emitted by the
current governed VC5 build and the retail call contract. A target-only ABI
spelling correction still routes through a revision-guarded
`progress verification-target sync` review before the tracker registration is
changed.

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
acceptance artifact/evidence package, qualify saved candidate content, mutate the
tracker, or call ChatGPT Pro for routine order feedback. A passing target
comparison is immediate source feedback, not linked, byte, owner, model, or
tier acceptance. `progress handoff --packet-id <packet-id> --json` renders only
a real active reservation and never exposes a parent mutation command.

#### Explicit user-selected maintenance packets

`progress next` and `progress work claim-current` remain the sole automatic
reconstruction scheduler.  When the user explicitly selects a registered
target, physical block, or reviewed source owner for maintenance that is not an
appropriate phase-frontier claim, the parent may submit one
`recoil-explicit-maintenance-packet-v1` object through:

```powershell
python tools/recoil.py progress work create-explicit `
  --payload-file build/diagnostics/<packet>.json `
  --expected-scheduler-revision <revision> `
  --expected-semantic-revision <revision> `
  --dry-run --json

# After reviewing the projection, repeat the unchanged payload and guards:
python tools/recoil.py progress work create-explicit `
  --payload-file build/diagnostics/<packet>.json `
  --expected-scheduler-revision <revision> `
  --expected-semantic-revision <revision> `
  --apply --json
```

The stable v1 payload shape is:

```json
{
  "schema": "recoil-explicit-maintenance-packet-v1",
  "packet_id": "recoil:explicit-work:<parent-chosen-id>",
  "kind": "source-maintenance",
  "selected_scope": {
    "verification_target_ids": ["<exact registered target id>"],
    "physical_block_ids": [],
    "source_owner_ids": []
  },
  "writable_paths": ["src/<exact-current-file>.cpp"],
  "writable_overrides": [],
  "read_dependencies": ["src/<exact-current-header>.h"],
  "output_root": "build/explicit-maintenance/<fresh-packet-root>",
  "resources": {
    "binary_ninja_saved_view_read": false,
    "whole_link_window": false,
    "tracker_read": true,
    "manifest_read": false,
    "support_read": true
  },
  "objective": "<bounded user-selected objective>",
  "stop_condition": "<exact stop condition>",
  "validation_command": "python -B tools/recoil.py verify <nonaccepting-check>",
  "worker_role": "recoil_source_worker",
  "return_schema": [
    "packet_id",
    "outcome",
    "changed_paths",
    "validation_result",
    "first_divergence",
    "scope_contradiction"
  ],
  "user_selected_rationale": "<the user's exact selection rationale>",
  "scheduler_inappropriate_reason": "<why the phase scheduler is not appropriate>"
}
```

`read-only-diagnostic` uses a read-only worker role and an empty
`writable_paths` list. The optional `lease_expires_at` member is an ISO-8601
UTC deadline. A reviewed writable override has exactly `path`, `relation`,
`selected_scope_id`, `related_owner_id`, `evidence`, and `rationale`; it can
authorize only the named current path of that exact related owner. The narrow
`reviewed-unregistered-declaration-debt` relation instead requires an empty
`related_owner_id` and authorizes one exact existing repository source/header
path bound to the exact selected scope plus nonempty direct evidence and
rationale. It records registration debt; it does not infer or accept an owner.
A reviewed cross-owner override is projected into the packet's immutable
related-owner snapshot, normalized owner/resource claims, explicit invalidation
scope, handoff, and return receipt. That projection records the reviewed
dependency for conflict and invalidation; it neither infers nor accepts source
ownership.

The payload records `source-maintenance` or `read-only-diagnostic`, exact
registered target/block/owner ids, exact writable and read-only files, one
fresh absent `build/` output root, objective, stop condition, one nonaccepting
validation command, worker role, return schema, the user's selection rationale,
and why ordinary phase scheduling is inappropriate. Source maintenance requires
at least one exact current C/C++ source/header write inside the derived
registration/owner closure. A path outside it requires one exact reviewed
cross-owner declaration/debt row with evidence and rationale. Diagnostics have
no source writes. Directories, globs, generated paths, and hidden closure
expansion fail.

The validation command is parsed as one command, not substring screened. It
must be repository `python`/`python.exe`, optional `-B`, then
`tools/recoil.py` and one exact command from the actual public registry. The
selected backend's real argument parser must accept the complete command before
packet creation. Only registered nonaccepting `verify`, `audit`, `guard`, or
`doctor` routes are eligible. Shell composition, redirection, arbitrary
executables/scripts, nonexistent subcommands, progress/docs mutations, and
`--apply` fail. A Binary-Ninja-backed route requires the exact saved-view read
claim; `verify final-build` additionally requires the packet's whole-link
window.

Git governs maintained authored inputs. Ignored paths are generated or
machine-local and are nonauthoritative; ignored generated-file churn is not
packet-closeout evidence. Validation and build output should normally use
external or isolated roots to avoid clutter, but generated-file presence is a
hygiene concern rather than reconstruction acceptance. Generated files never
supply source, expected truth, manifest, profile, or acceptance evidence.
Unresolved Git state is an unconditional blocker.

Dry-run performs only local tracker/ledger/path validation and claim projection.
Apply uses an explicit cross-filesystem state machine rather than claiming a
SQLite/filesystem atomicity guarantee. The first scheduler CAS inserts the
packet and reservation as nonrunnable `allocating`. The tool then exclusively
creates the exact output directory and records its Windows volume identity,
stable file ID/index, packet id, allocation operation id, tracker identity, and
canonical path alongside exact marker fields. It reopens the directory
reparse-safely and directly rechecks the physical identity and marker. A second
scheduler CAS activates the same packet. Only then may handoff, compile,
preprocess, LINK, or Binary Ninja access occur. A missing/non-directory root,
pre-existing unowned root or ownership sidecar, physical-identity or marker
mismatch, same-path replacement, or reparse escape fails closed. Before
filesystem work, apply records a durable non-work allocation journal that owns
no active reservation or normal claims. An independently authenticated sidecar
exists before root creation, so a marker-creation failure can never turn an
unauthenticated empty directory into cleanup debt. All root, revision, issue,
conflict, claim, and success-response checks precede the final activation CAS;
that transaction is the last semantic operation and atomically creates the
active packet, reservation, and claims.
Allocation failure leaves the journal recoverable and removes only content
authenticated to that operation. It never
accepts anything. Revision-domain storage guards both the scheduler revision
and the semantic revision used to derive the closure, then increments
transaction and scheduler only. The semantic and evidence revisions are not
incremented. Legacy live storage is not migrated by this route.
`progress handoff --packet-id <id> --json` renders the complete stored
closure only from an active reservation after reauthenticating the exact clean
Git baseline. For every tracked-write issue or progress packet it returns the
exact linked `worktree_root`, packet branch, opaque baseline commit,
physically authenticated external build root, and bounded Git restrictions.
The worker runs from that returned root, may stage only the exact writable
closure and create exactly one nonaccepting packet commit containing the packet id,
and performs no other branch, worktree, integration, or build-root lifecycle.
`progress work return` stores bounded
nonaccepting feedback and releases the lease; `progress work close` closes that
returned packet while retaining its immutable user selection, closure, result,
optional governed BN transcript, and released reservation as a terminal
nonaccepting record. Active failure may be abandoned with an explicit reason, and
an explicitly expired lease may be released with `progress work
recover-expired`. A worker PASS never changes source-semantic, order, byte,
profile, owner, tier, evidence, or phase state.

Failed pre-activation allocation debt is recovered only through `progress work
recover-allocation`. The command accepts no path override or caller assertion
about absence, ownership, marker validity, or cleanup success. It resolves the
journaled path, rejects reparse substitution and unexpected content,
authenticates the exact sidecar/marker identities, removes only that owned root,
reopens the path to prove absence, and CAS-records a nonaccepting opaque recovery
receipt. Repeating recovery is idempotent.

The parent lifecycle commands are revision guarded in the same scheduler
domain:

```powershell
python tools/recoil.py progress handoff --packet-id <id> --json
python tools/recoil.py progress work return --id <id> --result-json '<bounded-json>' --expected-scheduler-revision <revision> --apply --json
python tools/recoil.py progress work return-binja --id <id> --read-plan-json '<recoil-governed-binja-read-plan-v1>' --result-json '<bounded-json>' --expected-scheduler-revision <revision> --apply --json
python tools/recoil.py progress work close <id> --expected-scheduler-revision <revision> --apply --json
python tools/recoil.py progress work close <id> --outcome abandoned --abandonment-reason '<reason>' --expected-scheduler-revision <revision> --apply --json
python tools/recoil.py progress work recover-expired --id <id> --expected-scheduler-revision <revision> --apply --json
python tools/recoil.py progress work recover-allocation --id <id> --expected-scheduler-revision <revision> --expected-semantic-revision <revision> --dry-run --json
# Review, then repeat the unchanged command with --apply.
```

An optional saved-view reader claim uses the one canonical logical resource
`binary-ninja-db:Recoil.bndb`. Readers overlap only with readers and conflict
with every writer. Actual reads must occur after reservation through
`GovernedBinaryNinjaReadSession`, which target-qualifies the maintained
`ReferenceImage("recoil")`, requires available and equal provider-owned
begin/end snapshots carrying
`recoil-binja-authenticated-snapshot-v2`, authenticated=true, provider identity,
capability version 2, nonempty generation token/revision, and the exact
maintained saved-view identity, transcribes every registered JSON/hexdump response, and
reauthenticates the unchanged reservation before producing its opaque
nonaccepting receipt. Unknown/mutating endpoints, unavailable snapshot support,
saved-view mismatch, drift, incomplete/untyped snapshots, path/mtime substitutes,
or caller-authored snapshot/Boolean fields fail. Expected-fact extraction uses
only this typed parser, provider generation/revision coordinates, exact
begin/end equality, and direct expected-row content; there is no permissive
legacy snapshot adapter or database/transcript content summary.
The production session has no caller bridge URL, bridge factory, call-budget,
binary, target, database, filename, or saved-view override. It uses the fixed
repository bridge configuration and injects exact `Recoil.bndb` selection.
The seal prevents construction through the supported API; it is not described
as cryptographic authentication beyond the provider-owned snapshot capability.
Direct MCP reads and ordinary `BinaryNinjaBridge` reads are useful navigation
only and do not produce a governed receipt.

The only public BN-enabled return path is `progress work return-binja`. The
packet binds the canonical issue-ledger path and immutable SQLite/cutover
identity at creation; return derives that identity from packet provenance and
fails if the ledger is missing or different. The caller cannot select a return
ledger. Its
read plan contains exactly `schema` and `requests`; each request is either
`{"transport":"json","endpoint":"<registered-read>","parameters":{...}}`
or `{"transport":"hexdump","address":"0x...","length":N}`. The plan is
limited to 64 KiB and 256 requests, a hexdump to 1 MiB, the complete stored
transcript to 4 MiB, and the returned worker result to 64 KiB. The tool parses
all bounds before constructing the reader, independently authenticates the
active reservation before constructing the bridge, preserves every exact
payload read, and then returns the packet in the same scheduler-CAS invocation.
An over-limit exact payload fails without storing a partial transcript or a
probabilistic summary. No receipt, snapshot, freshness Boolean, or transcript supplied by
the caller is accepted.

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

When two or more distinct authored source members are proven to coalesce into
one retail body, use the reviewed `recoil-logical-alias-group-v4` mutation
contract. The physical function remains the sole authored order, byte, and
call-contract gate and retains one address-exclusive primary owner. Each
logical member remains authored but non-gating, carries its own exclusive owner
and resolved defining source edge with an attached canonical production-source
mirror, and is selected by immutable-retail direct call-site or
vtable-storage/slot evidence. Recovered or provisional descriptive names and
decorated candidate object symbols never replace those retail selectors. The
paired current-source VC5
diagnostics above have a narrower role: they must corroborate distinct eligible
object COMDAT definitions, exact numeric `IMAGE_COMDAT_SELECT_*` values for
every logical member and the base-implementation negative control, explicit
body relocation partitions, exact inbound
candidate caller/vtable relocation bindings for every selector, NOICF split,
ICF fold, and an object/report-backed fold-relevant difference that keeps the
base-implementation negative control outside the fold. Candidate
OBJ/MAP addresses or provider choices never supply retail expected identity or
address truth. Proof-v2 treats a real member/control COMDAT-selection mismatch
as such a fold-relevant difference even when the exact section lengths and raw
fold-relevant bytes match, both relocation partitions are empty, no associative
sections differ, and every definition is COMDAT-eligible. Legacy proof-v1
payloads remain valid only under their original exact field set and comparison
rules; they cannot supply this newer selection proof.

Generic VC5 verification-target source policy keeps the physical row as the
sole authored order identity and gate. When that exact physical row carries a
current reviewed authored-ICF group, an authored translation-unit order row is
satisfied only by the complete current logical-member population: exactly one
attached `.text` `defines` artifact per exclusive recorded source edge, with
the same anchor and translation unit, and no physical or extra same-address
function artifact in the target's source closure. A secondary top-level
function target remains a physical function gate but must expose exactly one
current logical member in its own source closure; zero, multiple, physical, or
unreviewed same-address mirrors fail closed. The loader revalidates the complete
selected group's current evidence contract, proof, member owners, owner/source
exclusivity, and logical source mirrors before either projection is allowed. A
missing, stale, unreviewed, incomplete, extra, overlapping, or duplicate-gate
representation fails closed; ordinary non-ICF physical source-trace behavior
is unchanged. During authored translation-unit COFF order comparison, that
validated physical gate is looked up through exactly the logical member whose
exclusive defining source edge names the compiled translation unit, using that
member's proof-bound decorated object symbol. Zero or multiple TU matches, a
stale group, or an object symbol not covered by the current proof fails closed.
This candidate lookup retains the physical address and order gate; it does not
turn the selected logical identity into another gate. The mutation changes no
owner gate or tier and must be reviewed dry-run-first.

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
python tools/recoil.py verify call-contract --slice <slice-id> --build-root <packet-root> --json
```

It derives one deterministic writable definition closure before compiling:
registered implementation roots contribute their transitive repository-local
quoted headers, and those headers contribute only final-build translation
units with matching active qualified declarations/definitions. The match uses
the exact per-TU VC5 final-build macro/profile context, ignores calling
convention spelling for identity matching so a declaration/definition ABI
change remains connected, excludes header-inline definitions, and fails closed
on unsupported preprocessor conditions or ambiguous TU ownership. Every newly
added definition TU is freshly compiled under its governed final-build profile
in an isolated subroot after every body has semantically compared and before
PASS. Target candidates are compiled lazily in deterministic retail order, so
an earlier exact divergence skips later target compilation. Thus a worker
cannot PASS while a writable declaration and its production definition
disagree, and callee identity alone never grants an unrelated source path.

The verifier then freshly compiles the accepted authored-order target/source
closure and compares the ordered static invocation signature of every body with
current retail Binary Ninja assembly. Signatures include exact call count/order,
direct versus indirect dispatch, direct/self/base/provider/IAT target identity,
virtual/interface slot displacement, callback storage identity, call versus
tail form, and known caller cleanup. Intra-body branches are ignored.
Unresolved caller, callee, provider, import, callback-storage, physical/logical
alias, or slot identity blocks; candidate output never supplies expected
truth.

The parent-only repair-continuation route consumes only the claimed diagnostic
producer packet, its retained predecessor, and the producer's allocated root:

```powershell
python tools/recoil.py progress call-contract prepare-repair-continuation --producer-packet <producer-packet-id> --returned-work-item <returned-work-id> --build-root <producer-root> --expected-revision <revision> --apply --json
```

Only `progress work claim-current --lane primary` creates and reserves the
branchless `call-contract-continuation-producer-v1`. Its one nonmutating command
performs a fresh exact-target, all-authored-body, all-caller-divergence scan.
The parent reruns that exact command, derives a
`call-contract-repair-route-descriptor-v1` from verifier provenance, and fails
closed on ambiguity, provider/out-of-policy routing, or anything other than one
controlling declaration and definition. No operator caller, owner, or path
facts are accepted. A descriptor-ready checkpoint creates no child; only a
later primary `claim-current` may create the one-hop nonaccepting
`call-contract-repair-continuation-edit-v2` with its exact native-Git closure.
Child PASS and commit feedback never accept evidence. Fresh parent
`advance-live-call-contract` with a separate parent-only
`call-contract-acceptance-v1` packet remains required.

Phase closeout requires a completely fresh phase-wide invocation before any
ordinary scheduling or transition decision can rely on convergence:

```powershell
python tools/recoil.py progress call-contract prepare-live-convergence --packet-id <packet-id> --closeout --build-root <packet-root> --jobs 2 --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --apply --json
```

The issue-ledger option defaults to that canonical SQLite authority. Before any
expensive build, the full closeout authenticates the active packet, its physical
output root, its exact resource vector and governed BN reader, then checks the
ledger together with tracker leases. Only a fresh no-reuse, zero-divergence
full scan may authorize the phase transition. A continuation checkpoint,
returned follow-up packet, target-wide PASS, or stored body result never
substitutes for that complete fresh scan and never becomes acceptance evidence.

Caller-specific register-storage bridges retain the same fail-closed rule. For
`GameNet::EndChatComposeAndSend` at `0x414590`, current source and its governed
VC5 `hud.obj` must expose the exact `__cdecl` symbol
`?EndChatComposeAndSend@GameNet@@YAXXZ`, attached source anchor and translation
unit, known extent, complete body and relocation population, and registered
order target. Those current-source facts authorize only the candidate-side ESI
name bridge. Immutable retail still supplies the MSVCRT `strncat` IAT identity,
while both sides must independently preserve all nine calls, the two-call ESI
lifetime, call form, cleanup, direct-target identities, and relocation shape.
The former `__fastcall` spelling, a stale body/source/target, or any conflicting
indexed identity blocks before comparison and grants no tracker, provider,
owner, gate, tier, or acceptance state.

The same rule covers a conditional virtual call whose ordinary candidate CFG
provenance is deliberately unresolved. For `HudUiMpExitDialog::Update` at
`0x419690`, the candidate-only bridge is available only when the complete
governed `mission.cpp` contribution, exact `0xb0`-byte VC5 body, complete
twelve-row COFF relocation table and masks, and reviewed
`g_HudUiTopMessageStack` symbol/storage/source/target authority are all current.
It recognizes only the relocation-backed load at `+0x7c`, the time load and
argument at `+0x82`/`+0x87`, the vptr load at `+0x88`, and the EDX slot-0 call
at `+0x8a`. The bridge supplies candidate-side storage provenance for that one
call site; retail still independently requires ordinal 5, indirect
virtual-slot 0, and `load(storage:recoil:data:0x56bd24)`. A stale body, missing,
extra, reordered, retargeted, or wrong-type relocation, changed call register
or slot, or conflicting tracker identity fails closed and cannot create any
expected fact or acceptance state.

The natural `delete dialog` in
`RecoilApp_MpExitDialogState::OnDeactivate` at `0x419940` has the same
candidate-only treatment. Retail independently requires ordinal 1, indirect
virtual slot `+8`, and `load(storage:recoil:data:0x4f329c)`. The candidate
bridge is available only for the complete governed `mission.cpp` contribution,
the exact `0x50`-byte VC5 body including tail padding, the complete nine-row
COFF relocation table/addends/masks, and the reviewed
`g_HudUiMpExitDialog` symbol/storage/source/target authority. Its COD unit must
load that relocation-backed global at `+0x0b`, null-check ECX at `+0x11`, load
EAX from `[ECX]` at `+0x15`, push deleting-destructor flag `1` at `+0x17`, and
call `[EAX+8]` at `+0x19`. The bridge supplies candidate-side storage
provenance only for that call; caller, source, translation-unit, target, body,
relocation, COD, register, topology, slot, or storage drift fails closed and
cannot become retail truth or acceptance state.

`RecoilApp_MpExitDialogState::OnUpdateShouldQuit` at `0x419990` uses an
equally narrow bridge for its natural local `dialog` call to `UpdateAll`.
Retail independently requires ordinal 2, indirect virtual slot 0, and
`load(storage:recoil:data:0x4f329c)`. Candidate eligibility requires the exact
governed `mission.cpp` contribution and focused target, the complete
`0x110`-byte body including tail padding, all nineteen COFF relocations with
zero addends and exact masks, and the reviewed `g_HudUiMpExitDialog`
symbol/storage/source/target authority. The complete COD topology must retain
the relocation-backed ECX load at `+0x12`, `_g_FrameDeltaTimeSec` load and EDX
argument push at `+0x18`/`+0x1e`, EAX vptr load at `+0x1f`, and `[EAX]` slot-0
call at `+0x21`. That proof authorizes candidate-side provenance only at the
one call site; caller, source, translation-unit, target, body, relocation, COD,
storage, argument, register, or slot drift fails closed and cannot supply
retail truth or acceptance state.

The later candidate call instruction at `+0x9e` remains the exact `call rel32`
form in that complete body. Its relocation at `+0x9f` must now name the exact
registered `__cdecl` symbol `?FlipToGDIIfAttached@zVideo_dd@@YAXXZ`; the former
`__fastcall` spelling `?FlipToGDIIfAttached@zVideo_dd@@YIXXZ` is stale and
fails closed. This caller-specific candidate freshness check does not infer a
generic target and does not supply expected retail truth.

It also covers only the `zSndSystem::Shutdown` call at `+0xa3`: relocation
`+0xa4` must name the exact registered `__cdecl` symbol
`?Shutdown@zSndSystem@@YAHXZ`. The former `__fastcall` spelling
`?Shutdown@zSndSystem@@YIHXZ` is stale and fails closed.

The same bridge separately covers only the `zNetwork::ShutdownSessionRuntime`
call at `+0xa8`: relocation `+0xa9` must name the exact current `__cdecl`
symbol `?ShutdownSessionRuntime@zNetwork@@YAHXZ`. The former default-`/Gr`
`__fastcall` spelling `?ShutdownSessionRuntime@zNetwork@@YIHXZ` is stale and
fails closed.

The bridge separately covers only the subsequent
`zVideo::ShutdownVideoSystem` call at `+0xad`: relocation `+0xae` must name the
exact current `__cdecl` symbol `?ShutdownVideoSystem@zVideo@@YAHXZ`. The former
default-`/Gr` `__fastcall` spelling `?ShutdownVideoSystem@zVideo@@YIHXZ` is
stale and fails closed. Each caller-specific refresh remains confined to its
one exact row; none normalizes another shutdown family, infers a generic
target, or derives expected retail facts from candidate evidence.

`HudUiNetGameSetupPanel::Constructor` at `0x419aa0` has separate exact
candidate-only projections for its concrete `playButton` and `cancelButton`
members. Retail independently requires ordinals 1 and 2 to call the authored
`HudUiZrdWidget::Constructor` identity at `0x4b4ee0`; current VC5 naturally
calls the same-object external
`??0HudUiNetGameSetupPanel_LaunchButton@@QAE@XZ` and
`??0HudUiNetGameSetupPanel_CancelButton@@QAE@XZ` constructors instead. The
projections require the exact authored caller extent and mission.cpp physical
block, functional and translation-unit target registrations, the exact
authored `0x4b4ee0` extent and zUI physical-block/functional/focused/order
target authority, and the exact candidate calls at `+0x33` and `+0x43` as
ordinals 1 and 2 between their reviewed receiver and EH-state neighbors.
Eligibility also requires the complete exact `0x640` caller body, the complete
ordered 102-row relocation population with zero addends and exact mask, and the
complete 381-instruction COD offset/mnemonic/byte topology through the `ret 4`
plus its nine padding NOPs. The `+0x34` and `+0x44` relocations must each be
one zero-addend, fully masked `REL32` reference to the corresponding exact
local decorated symbol. Both local constructors must remain exact natural
32-byte SELECT_NODUPLICATES COMDATs: seven COD instructions, a `REL32`
base-constructor call at `+4`, the corresponding LaunchButton or CancelButton
vtable `DIR32` stamp at `+10`, and no local control flow. Only those complete
caller-specific proofs may replace their generic `candidate-local-coff:`
comparison sentinels during late arbitration. They do not infer any other
member constructor, alter source, or supply retail expected truth; caller,
authority, ordinal, symbol, body, relocation, COD, vtable, or cleanup drift
fails closed.

The cumulative LaunchButton, CancelButton, numeric-wrapper, and WorldSelector
proof is entered only when the caller identity and normalized extent are all
exactly `recoil:function:0x419aa0` and `0x419aa0-0x41a160`. Any other caller,
including an unrelated caller that naturally invokes
`??0HudUiNumericTextInput@@QAE@XZ`, bypasses the cumulative proof before any of
its prerequisites are evaluated. The bypass contributes no candidate mapping;
ordinary fail-closed extraction remains authoritative for that caller. An
exact caller still rejects every incompatible pre-existing bridge and every
authority, body, relocation, COD, signature, ordinal, or topology drift named
below.

The same caller has one further exact candidate-only projection for the
concrete `gameNameInput` member. Retail independently requires ordinal 3 to
call the reviewed `HudUiNumericTextInput::BaseConstructor` wrapper identity at
`0x41a190`, while current VC5 naturally calls the authored zero-argument
`??0HudUiNumericTextInput@@QAE@XZ` constructor associated with `0x4b49e0`.
This is not a generic wrapper equivalence. It is eligible only after the
complete LaunchButton/CancelButton caller proof above remains current, the
ordinal-3 caller instruction is the exact `E8` at `+0x55`, and `+0x56` is its
sole zero-addend, fully masked `REL32` reference to that exact natural
constructor symbol. The reviewed `0x41a190` identity, extent, mission physical
block, functional/order registrations, mission contribution row, and resolved
zui.cpp source-trace edge must all remain exact. A fresh governed build of the
registered two-TU zUI target must also reproduce the source-trace wrapper as
the exact 16-byte SELECT_NODUPLICATES COMDAT with one direct `REL32` delegation
to the natural constructor, and reproduce the natural constructor as the exact
176-byte SELECT_NODUPLICATES COMDAT with its complete nine-row relocation
population and relevant 47-instruction construction topology. The natural
constructor's reviewed `0x4b49e0` identity, extent, zUI physical block, and
functional/focused/order registrations remain independent prerequisites.
Only that complete caller, authority, wrapper, delegation, and natural-body
proof may replace the ordinal-3 `candidate-local-coff:` comparison sentinel;
candidate output never supplies retail expected truth, and no other wrapper or
constructor relationship is inferred.

The following `worldSelector` construction at ordinal 4 is verified without
adding any candidate-name equivalence. After removal of the non-retail empty
`HudUiNetGameSetupPanel_WorldSelector` constructor, current VC5 naturally
emits a direct `??0HudUiCycleSelectorWidget@@QAE@XZ` call at `+0x6e`, whose
sole zero-addend, fully masked `REL32` relocation is at `+0x6f`, followed
immediately by the derived WorldSelector vtable install whose sole
zero-addend, fully masked `DIR32` relocation is at `+0x75`. The ordinary
reviewed candidate-name identity must already resolve that exact base
constructor to authored `0x4b7d60`; storage, direct-bridge, compiler-bridge,
or alternate-signature projections are forbidden. Eligibility replays the
complete LaunchButton, CancelButton, numeric-wrapper, caller-body, relocation,
mask, and COD proofs above, and independently requires immutable retail
ordinal 4 plus the exact reviewed `0x4b7d60` extent, zUI physical block,
functional/focused/order registrations, and sole authored-lifecycle
zui_widgets.cpp contribution. The same fresh governed two-TU zUI compilation
must reproduce the natural constructor as the exact 96-byte
SELECT_NODUPLICATES COMDAT: 22 COD instructions, a zero-addend `REL32` call to
`HudUiZrdWidget` at `+4`, a zero-addend CycleSelector-vtable `DIR32` relocation
at `+0xc`, the complete exact relocation mask/body, and the reviewed
initialization topology. The Mission object must contain exactly one undefined
natural base-constructor reference and the defined derived vtable, with no
defined or undefined local WorldSelector constructor, no local constructor
COMDAT, and no such call. This proof authorizes only the already-natural
caller row; candidate output supplies no retail truth and no generic
constructor relationship.

The following `nextWorldButton` construction at ordinal 5 is likewise a
verification-only natural base call, not a candidate-name equivalence. After
removal of the non-retail empty
`HudUiNetGameSetupPanel_NextWorldButton` constructor, current VC5 emits one
direct `??0HudUiZrdWidget@@QAE@XZ` call at `+0x86`, whose sole zero-addend,
fully masked `REL32` relocation is at `+0x87`, immediately followed by the
derived NextWorldButton vtable install at `+0x8b` with its sole zero-addend,
fully masked `DIR32` relocation at `+0x8d`. Eligibility is confined by the
outer exact caller identity/extent gate, replays every ordinal 0--4 proof
above, and requires immutable retail ordinal 5 plus the ordinary reviewed
authored `0x4b4ee0` identity. Storage, direct, compiler, alternate-signature,
or generic constructor projections are forbidden.

The exact reviewed `0x4b4ee0` extent, zUI physical block, functional, focused,
order, source, and sole authored-lifecycle zui_widgets.cpp contribution must
remain current. The same fresh governed two-TU zUI compilation must reproduce
the natural `HudUiZrdWidget` constructor as its complete 448-byte
SELECT_NODUPLICATES COMDAT: six ordered zero-addend relocations, the exact
24-byte relocation mask, 108 COD instructions, the exact `HudUiWidget(int)`
base call at `+0x23`/`REL32 +0x24`, and the `HudUiZrdWidget` vtable install at
`+0xa1`/`DIR32 +0xa3`. Mission COFF must contain exactly one undefined natural
constructor reference and the defined derived vtable, with no defined or
undefined local NextWorldButton constructor, no local constructor COMDAT, and
no local constructor reference or call. Caller body, relocation, mask, COD,
target, source, TU, contribution, natural-body, base-call, vtable, absence, or
projection drift fails closed. Candidate output supplies no retail truth and
the proof returns no mapping.

The following `prevWorldButton` construction at ordinal 6 continues that same
verification-only source-faithful pattern. After removal of the non-retail
empty `HudUiNetGameSetupPanel_PrevWorldButton` constructor, current VC5 emits
the exact direct `??0HudUiZrdWidget@@QAE@XZ` call at `+0x9e`, with its
zero-addend, fully masked `REL32` relocation at `+0x9f`, immediately followed
by the derived PrevWorldButton vtable installation at `+0xa3` and its sole
zero-addend, fully masked `DIR32` relocation at `+0xa5`. The verifier first
replays every exact caller and ordinal 0--5 proof above, then independently
requires immutable retail ordinal 6 to name the reviewed ordinary authored
`0x4b4ee0` identity. The ordinary target name, source/TU contribution, exact
448-byte natural constructor body, six relocations, 24-byte relocation mask,
108-instruction COD topology, `HudUiWidget(int)` base call, and ZrdWidget
vtable proof remain mandatory; storage, direct, compiler, alternate-signature,
and generic-equivalence projections remain forbidden.

Mission COFF must expose one undefined natural constructor identity shared by
the two natural call sites and define both derived vtables, while containing
no defined or undefined local PrevWorldButton constructor, local constructor
COMDAT, reference, or call. The ordinal-6 proof returns no mapping. The
complete refreshed caller is encoded directly from fresh governed COFF/COD;
candidate output is extraction-only and never becomes expected retail truth.

The coupled inline `TimeLimitInput(4)`, `KillsInput(2)`, and
`MaxPlayersInput(2)` construction refresh retains that same complete `0x640`
caller, 102-row relocation population, exact relocation mask, and
381-instruction COD topology. The three exact natural calls to
`??0HudUiClampedIntTextInput@@QAE@I@Z` occur at `+0xb8`, `+0x104`, and
`+0x154`, with their zero-addend, fully masked `REL32` relocations at `+0xb9`,
`+0x105`, and `+0x155`. The immediate bounds are exactly 4, 2, and 2, and each
call is followed by its exact derived TimeLimit, Kills, or MaxPlayers vtable
installation and zero-addend `DIR32` relocation at `+0xbf`, `+0x10f`, or
`+0x15f`. The removed out-of-line derived-constructor symbols must remain
absent, the ordinary clamped constructor must remain one same-object external
definition, and all three derived vtables must remain defined in that object.

Immutable retail truth remains deliberately asymmetric: ordinal 7 names the
reviewed `0x41a190` numeric-wrapper identity, while ordinals 10 and 13 name the
ordinary authored clamped-constructor identity at `0x41a200`. Therefore the
verifier may project only this exact caller's ordinal 7 after all complete
caller, bound, call, relocation, vtable, symbol-population, tracker-identity,
and cleanup guards pass. It must leave ordinals 10 and 13 naturally resolved
to `0x41a200`; a decorated-name equivalence would corrupt them. This authority
does not project or otherwise adjudicate any intervening Inc/Dec button
constructor. Candidate output remains extraction-only and
`candidate_expected_truth` remains false.

The separate six-step-button refresh verifies the current natural candidate
shape after removing the explicit zero-argument constructors for the distinct
Inc/Dec TimeLimit, Kills, and MaxPlayers button types. The containing caller
must directly invoke `??0HudUiZrdWidget@@QAE@XZ` at `+0xce`, `+0xe8`,
`+0x11e`, `+0x138`, `+0x16e`, and `+0x188`. Each call must have its unique
zero-addend, fully masked `REL32` relocation and must be followed immediately
by its distinct derived-vtable installation with the exact corresponding
zero-addend `DIR32` relocation. All six removed explicit constructor
identities must remain absent, while all six distinct derived vtables must
remain defined exactly once. The complete current `0x640` body, ordered
102-row relocation population, 408 masked bytes, 381-instruction COD, empty
local control flow, `ret 4`, nine-NOP tail, caller extent, Mission physical
block, source/TU contribution, and target registrations remain mandatory.
Compiler-local spellings used by the cumulative exact snapshots are also the
fresh current values: caller `$L86195`, numeric constructor `$L83621`, cycle
loop `$L80173`, and ZrdWidget constructor `$L83849`.

This step-button guard returns no candidate mapping. Immutable retail directly
authorizes only ordinal 8 as the reviewed authored `0x4b4ee0` identity; the
candidate must reach that identity through ordinary resolution of the exact
natural ZrdWidget call. Later step-button rows are not inferred from candidate
output and receive no generic six-button equivalence: they proceed through
ordinary extraction and comparison against their independently derived retail
rows. Registered raw-extra inventory for implicit constructors remains a
non-gating diagnostic and is not promoted into expected truth. The current
live slice clears the six natural rows through ordinal 15 and then enters the
separate toggle guard below.

The following two-toggle refresh verifies the exact current natural
construction of the distinct `AllowMapsToggle` and `NameTagsToggle` members
after their explicit zero-argument constructors were removed. The members are
contiguous `0x164`-byte `HudUiCheckToggleWidget` subclasses at offsets 50792
and 51148. Mission COD must load those exact receivers at `+0x197` and
`+0x1b1`, directly call the ordinary
`??0HudUiCheckToggleWidget@@QAE@XZ` constructor at `+0x1a2` and `+0x1bc`, and
immediately install the distinct derived vtables at `+0x1a7` and `+0x1c1`.
The corresponding zero-addend, fully masked relocations are exact `REL32`
rows at `+0x1a3` and `+0x1bd` and exact `DIR32` rows at `+0x1ad` and
`+0x1c7`. Both removed derived-constructor symbols must remain absent, while
both vtable symbols must remain defined exactly once.

Each vtable must also be the exact current 132-byte SELECT_ASSOCIATIVE COMDAT:
zero-filled relocation slots at every four-byte offset, one class-specific
deleting-destructor row followed by the complete 32-row common
`HudUiCheckToggleWidget` hierarchy and method population. Eligibility requires
the reviewed authored CheckToggle constructor identity and exact extent at
`0x4b6fc0`, plus the distinct reviewed authored derived-vtable identities at
`0x4cf1d0` and `0x4cf148`; unknown tracker extent is preserved and no extent
is inferred from candidate COFF. The complete cumulative caller, zUI
constructor, step-button, body, relocation, mask, COD, target, population, and
vtable checks above remain prerequisites.

This toggle guard returns no mapping or semantic projection. Immutable retail
directly authorizes only ordinal 16 as `0x4b6fc0`; the exact natural candidate
must reach it through ordinary reviewed-name resolution. Ordinal 17 is checked
only for current candidate topology and deliberately remains outside this
retail authority. Ordinary extraction and comparison therefore adjudicate it
and every later row. The exact ordinal-18 `killsSwitch(0)` relationship is
owned separately by the following guard. Candidate output remains
extraction-only and never supplies expected retail truth.

The `killsSwitch(0)` refresh verifies the exact natural overload selected by
the corrected source expression without adding a constructor equivalence.
The current complete Mission caller loads the member at offset 51504 with the
`lea` at `+0x1cb`, pushes the exact already-zero `edi` argument at `+0x1d1`,
updates the EH state at `+0x1d2`, and directly calls
`??0HudUiWidget@@QAE@I@Z` at `+0x1d7`. Its sole call relocation is the
zero-addend, fully masked `REL32` row at `+0x1d8`. Mission COFF must expose
exactly one ordinary undefined reference to that decorated symbol and no
defined, storage, direct-bridge, or compiler-bridge substitute.

Eligibility replays every cumulative WSI019--028 proof and requires the exact
current `0x640` body, 102-row relocation population, 408-byte mask,
381-instruction COD topology, empty local control flow, `ret 4`, and nine-NOP
tail. The one-byte argument push shifts every later instruction and relocation
by one byte while consuming one former tail NOP; the body size is unchanged.
The independently reviewed ordinary authored `HudUiWidget(int)` identity must
remain exactly `0x4b3d00` with its tracker-owned extent, physical block, and
functional/focused/order target registrations.

This kills-switch guard returns no mapping and directly checks only immutable
retail ordinal 18. It extracts the candidate prefix only through that call and
does not inspect, encode, project, or infer expected ordinal 19. The exact
ordinal-19 `lapsSwitch(0)` relationship is owned separately by the following
guard.

The `lapsSwitch(0)` refresh repeats the same verification-only natural-overload
pattern for the adjacent member at offset 51692. The complete current Mission
COD must load that receiver at `+0x1dc`, push the exact already-zero `edi`
argument at `+0x1e2`, update the EH state at `+0x1e3`, and directly call
`??0HudUiWidget@@QAE@I@Z` at `+0x1e8`. The call's sole relocation is the
zero-addend, fully masked `REL32` row at `+0x1e9`. The cumulative kills-switch
proof supplies the same exact reviewed authored `0x4b3d00` identity and
ordinary undefined-symbol population; no new name relationship is inferred.

The second one-byte argument push shifts every later instruction and relocation
by one further byte and consumes one further tail NOP. The complete caller
therefore remains `0x640` bytes with 102 relocations and 408 masked bytes, while
COD grows to 381 instructions, `ret 4` begins at `+0x634`, and the tail contains
nine NOPs. This laps-switch guard returns no mapping, checks only immutable
retail ordinal 19, and extracts the candidate prefix only through that call. It
does not inspect, encode, project, or infer any later expected row; ordinary
extraction and retail comparison retain sole authority beyond ordinal 19. The
fresh live slice matches through ordinal 19 and next stops at the independently
compared ordinal-20 mismatch: retail `0x4b98d0` versus current candidate
`0x4b4e40`.

The later actual `maxPlayersInput` else-branch guard is one cumulative,
callsite-local projection for ordinal 61 only. Eligibility retains every prior
caller and ordinal proof and requires the fresh complete Mission caller to be
exactly `0x650` bytes with 105 ordered COFF relocations, 420 masked bytes, 394
COD instructions ending at `+0x643`, and thirteen tail NOPs. In its focused
`+0x583..+0x5a4` block, the enabled-session value is loaded from the stack,
`this+0xc514` is formed in `ecx`, the exact min/max fields are stored, the vptr
is loaded through that member address, the enabled value is stored directly to
`this+0xc5d8`, and slot 120 is called without caller cleanup. None of those
focused bytes may carry a relocation. Immutable retail independently requires
ordinal 61 to be `load(this+0xc514)`; only after every exact object, operand,
ordinal, and cleanup check passes may the candidate's local
`load(address(this+0xc514))` provenance be projected to that storage identity
at call offset `+0x5a4`. The proof stops at that call and deliberately does not
inspect, encode, or project ordinal 62 or any later row; ordinary extraction
and retail comparison retain sole authority there. Candidate output remains
extraction-only and never supplies expected retail truth.

Within the earlier `HudUiMpExitDialog::Update` bridge, the sole candidate-name
normalization is the first relocation at `+0x1b`:
after that row independently proves `DIR32`, zero addend, exact mask/body, and
the complete `$T` followed by decimal digits compiler-local spelling, only its
numeric suffix is ignored. Every other COFF symbol identity remains exact, and
the normalization is unavailable at any other caller, row, offset, or type.
The `OnUpdateShouldQuit` bridge applies the same narrow rule independently only
to its sixth relocation at `+0x30`; every other row and symbol remains exact.

Complete candidate-shape authority is equally strict when a source-faithful
repair changes only VC5 register allocation. For
`HudLayoutHW::UpdateObjectiveDirtyRect` at `0x4132b0`, the reviewed current
`hud.obj` shape is exactly `0x90` bytes, ends with `ret` at `+0x89` and six
padding NOPs, carries the complete eleven-relocation table, and keeps the
`GetCenterX`/`GetCenterY`, `InvalidateRect`, direct `HudUiElement::Invalidate`,
and direct `HudUiTripletPanel::Draw` order, targets, dispatch, and cleanup.
Its `add ebx,edi` result must be stored from `ebx`; a crossover that combines
that add with the superseded `edi` store, or the superseded add with the
current store, is not another authorized shape. Truncated, extended, partially
mixed, and arbitrary bodies fail before semantic comparison. The current
candidate body authorizes only candidate-side extraction; immutable retail and
reviewed tracker identities remain expected truth.

The retained parent acceptance command contract is:

```powershell
python tools/recoil.py progress advance-live-call-contract --slice <slice-id> --packet-id <packet-id> --build-root <packet-root> --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --apply --json
```

The command authenticates the active packet, reservation, physical output root,
and governed BN read resource. In that same parent invocation it performs one
fresh build, derives expected facts directly from retail through the governed
saved view, and directly compares every body in the slice. Only bodies that
pass that direct comparison may have their `call_contract` dimension accepted
by the final revision-domain CAS.

No stored body result substitutes for fresh verification. Currency is
maintained through governed source/tool/manifest mutation, explicit
invalidation, and the reviewed integer coordinates
`CALL_CONTRACT_VERIFIER_GENERATION = 13`,
`NORMALIZER_REGISTRY_GENERATION = 12`, and
`EXPECTED_FACT_SCHEMA_VERSION = 12`. Any verifier component change invalidates all
current call-contract evidence; a normalizer change invalidates all users when
the exact user set cannot be proven. The result records exact ordered symbols,
targets, physical blocks, source/dependency paths, integer generations,
validation mode, and retail/tracker expected-truth mode. It does not accept or
revoke order, byte, owner, provider, gate, tier, storage, or final-image state.
Full order remains blocked until every body in the current reviewed live gating
census is current and the fresh complete no-reuse scan passes; compatible
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
alias facts, never from the candidate. An explicit empty set is valid.

The byte verifier snapshots exact path/existence/size/mtime facts for every
configured object and each lane-required summary, image,
and MAP before invoking final-build with `--clean`. Missing or unchanged
artifacts fail before semantic comparison. A nonzero final-build result is
usable only when its freshly written summary proves compile, alias-object,
link, image, and MAP completion and identifies linked-order as the independent
failure; other failed or incomplete builds cannot supply byte diagnostics.
The explicit final-build clean path also verifies that the selected isolated
root was actually removed before recreating outputs.

Genuine ambiguity blocks before the build and routes to the narrow reviewed
exception command:

```powershell
python tools/recoil.py progress relocation-exception set --source-symbol-id <physical-symbol-id> --source-address <cursor> --payload-json '<json-object>' --expected-revision <revision> --dry-run --json
```

The governed row binds exact current source/target extent, object registration,
pipeline/provider/alias context, and evidence ids. Drift produces a typed stale
exception instead of a generic currency failure. After review, repeat with `--apply`.

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

When an existing known-extent authored data symbol needs its first physical
storage row, use `progress storage register-authored-data` with the exact
reviewed symbol and primary-data relationship snapshots. The mutation retains
the canonical owner validator in `no-introduced-debt` mode: unrelated canonical
owner findings that existed before the mutation must remain byte-for-byte the
same multiset, while any finding before or after the mutation that intersects
the selected owner, symbol, or address fails closed. This scoped comparison
does not relax the command's exact symbol, owner relationship, extent, overlap,
output-section, storage identity, expected-revision, dry-run/apply, or README
synchronization guards. The result reports the preserved unrelated and touched
finding counts; it does not accept owner gates, tiers, storage verification, or
final-image state.

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

Retail and candidate are opened through stable read-only Windows handles and
parsed directly as PE images. The retail operation records canonical path,
volume identity, stable file ID, size, PE signature, and machine, and either
keeps the same handle or revalidates that physical identity through completion.
Acceptance compares governed headers, directories, sections, resources,
relocations, imports, selected linked rows, RVAs, operands, padding, zero-fill,
and bytes directly. Only the four-byte COFF `TimeDateStamp` field is excluded;
no historical date is forced, and a candidate differing only there passes that
dimension. Any other governed difference fails.

Configured VC5 tools, headers, and libraries are identified by absolute path,
Windows physical file identity where available, file size, version
resource/string, selected command-line identity, and direct tool output plus
negative controls. Binary Ninja identity is the exact maintained saved view,
provider identity and capability version, equal begin/end generation/revision
snapshots, and direct expected rows. Neither toolchain nor BN identity is a
substitute for candidate-versus-retail comparison.

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
python tools/recoil.py progress work claim-current --lane all --max-packets <available-child-slots> --expected-scheduler-revision <scheduler-revision> --apply --json
python tools/recoil.py progress handoff --packet-id <packet-id> --json
python tools/recoil.py progress audit --scope all --strict
```

### Native Git packet change control

Git is the sole authored-workspace change-control mechanism. The orchestrator
starts from a clean reviewed branch and creates one packet branch, linked
worktree, externally isolated build root, and central reservation. Handoff and closeout locate
the exact stored-branch worktree and use porcelain-v2 status plus commit-relative
name-status/diff to make only closure-authorized changes. A worker stages only its
exact writable closure and creates one nonaccepting packet-id commit. The parent
validates it, integrates first in a temporary worktree, freshly validates there,
and completes every fallible compiler, test, audit, and doctor check before
fast-forwarding canonical `master`. After the fast-forward, only deterministic
Git, topology, tag, and physical-identity assertions are permitted. Tracked
source, tools, tests, policies, target manifests, and
`.agent/REFERENCE_EXECUTABLE.json` come from the executing worktree;
machine-local `support/Recoil.exe` and the live progress/issue SQLite databases
come from the validated canonical control root and are never copied or linked
into the linked checkout. Retirement removes the merged
packet branch, linked worktree, and physically authenticated build root. Strict
worktree hygiene rejects inactive or stale lifecycle state. Ordinary copies
whose source remains unchanged write only the destination, while both rename
endpoints must be writable. Absolute checkout paths and external build-root
prefixes are diagnostic provenance, never semantic identities. Git commit and
object ids are opaque workspace state and never retail expected truth,
candidate equivalence, or reconstruction acceptance. The progress worktree
adapter is `native-git-v1`. `progress next` allocates nothing;
`progress work claim-current` is the only transition that creates a live
progress reservation. Newly claimed tracked-write packets use contract v4 and
remain non-handoff-visible until their allocation journal records an opaque
baseline commit, packet branch, exact linked association, normalized nonempty
writable closure, and physically authenticated external build root. Terminal
legacy v3 records remain readable but cannot relaunch. Read-only and
generated-output-only producers may be branchless. Generic validation,
integration, recovery, and retirement share the issue worktree/build-root
primitives under an explicit `issue|progress` authority tag; issue behavior is
unchanged.

### Governed active-ledger compaction

The schema-v5 progress database and version-2 workspace-issue database are
active-state stores, not archives. Only the parent may compact them, and every
invocation is guarded by the independent revision of the database it changes
and is dry-run/apply explicit:

```powershell
python tools/recoil.py progress compact --expected-revision <revision> --dry-run --json
python tools/recoil.py issue compact --expected-revision <revision> --dry-run --json
```

Review the canonical before/after byte counts, removed categories, retained
semantic categories, blockers, and parity before repeating with `--apply`.
Apply refuses any active reservation. Neither route creates an archive,
receipt, persisted content summary, or candidate qualification, and neither changes the
schema version.

Progress compaction requires exact pre/post `progress next` semantic parity at
one in-memory revision. It converts only exact current legacy call-contract
slices to their shared evidence representation, deletes only unreferenced
superseded call-contract evidence and its generic symbol links, removes
terminal work rows and the enumerated obsolete migration keys, and retains
`authored_call_contract_v1`, `source_traceability_v1`, and every unknown future
migration key. Any inconsistent facts, unexpected evidence reference,
source/dependency stat drift, audit error, scheduler divergence, or active
reservation fails closed.

Issue compaction first seeds `id_sequences.issue[YYYYMMDD]` high-water values
from every existing WSI id, then removes resolved/wont-fix issues, closed
packets, released reservations, and history arrays from retained current
issues. It retains open/in-progress issue fields, ready/active packets, and
active reservations. New issue ids advance the retained high-water map, so
deleting a terminal issue never permits id reuse.

Future close paths maintain the same active-only shape. `progress work close`
returns its normal terminal result and deletes the work row in the same CAS.
`issue work close` returns its normal released-reservation result and deletes
the packet and reservation in the same CAS. Resolving or marking an issue
wont-fix first proves that no unfinished packet exists, constructs the normal
response, then deletes the issue and its completed packet/reservation rows.
Those terminal ids are intentionally no longer valid `show` or `reserve`
handles; recompute the scheduler and claim fresh current work.

Only the parent mutates the unified tracker. Active binary-lane advancement
uses direct `--apply` with `progress advance-live-order` and
`progress advance-live-byte`: each validates/builds once and CAS-applies from
that same semantic result. Optional `--dry-run` is diagnostic. Reviewed owner
topology/membership replacement uses dry-run-first `progress owner
replace-batch`; conservative owner gate/tier decreases use `progress owner
downgrade`; verification-target registration metadata uses `progress
verification-target sync`. Unsupported positive owner metadata, gate, or tier
changes route to a bounded `issue request`, never an improvised mutation. Other
governed block, provider, classification, ambiguity-exception, blocker, and
semantic mutations remain dry-run-first under `--expected-revision`.
Scheduler-only work lifecycle commands use `--expected-scheduler-revision`;
live call-contract acceptance uses both semantic and evidence-generation
revision guards. Durable observations record semantic facts and direct
evidence paths when needed; they never store a concrete `.devspace`
dependency. Evidence generation never promotes owner gates, tiers, provider
classification, global prefixes, or `Model:` metadata.

A source-worker/verifier handoff is a compact mode-specific packet backed by a
real active reservation and lease. `progress handoff --packet-id <packet-id>
--json` fails when that state is absent, claims no writable path, or contains a
mutating worker command; it never fabricates current work. An `order-edit-v1`
packet carries only packet id, registered target, exact writable source/header
closure, exact returned worktree and external build roots, opaque baseline,
branch and bounded Git restrictions, objective, stop condition, and its `verify vc5-order`
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
