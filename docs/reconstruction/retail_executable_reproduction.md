# Retail Executable Reproduction

## Goal

The workspace is complete only when source-faithful VC5SP3 C/C++ compiles and
links into a file whose complete SHA-256 equals immutable
`support/Recoil.exe`. Matching behavior, selected functions, normalized bytes,
or PE shape is intermediate evidence, not completion. Post-link patching is
forbidden.

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
verification-target, work-item, blocker, and hashed-evidence entities. Binary
Ninja remains binary authority. Data symbols, owner data gates, physical
storage, PE sections, and final-image acceptance never imply one another.
Unknown extents retain their start address and `extent_state=unknown` but omit
size and end; a fabricated one-byte extent is forbidden.

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

Linked authored-order receipts use version 4 and keep these claims separate:
raw authored order is explicitly unevaluated by the link check; known-authored
linked relative order, required linked presence, scoped projection
completeness, and block precedence are reported independently. Exact selected
population plus linked seams/RVAs are explicitly unevaluated and remain
`full-function-order` work. Compatibility fields remain available for older
receipt consumers; version-3 linked authored receipts retain their historical
strict-extra validation.

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
helpers, fake wrappers, or duplicate bodies. New or disputed owner/block/order
or lifecycle-classification conclusions continue to require the root ChatGPT
Pro policy.

Stop at the first divergence. Work elsewhere is allowed only when recorded as
the required compile/link dependency of that cursor. Only while this phase is
blocked or incomplete, and only when its accepted authored-order prefix contains
an authored-order gating row that lacks authored-byte acceptance,
`progress next` may expose `fallback_authored_byte_cursor` for the earliest such
row. The fallback skips `non-authored` rows and explicit compiler-generated
lifecycle roles, does not move the primary
authored-order cursor, and never promotes the phase.

For a bounded whole-link COMDAT attribution problem, census existing diagnostic
builds before rebuilding:

```powershell
python tools/recoil.py audit vc5-comdat --run <diagnostic-receipt-or-directory> --run <comparison-receipt-or-directory> --symbol <exact-decorated-identity> --pattern <fullmatch-regex> --output-dir <evidence-directory>
```

The command writes `opening_object_manifest.json` and
`opening_comdat_census.json`. It preserves receipt object ordinals, compile
profiles, object hashes, relevant section/COMDAT metadata, bytes and relocation
signatures, recoverable direct-object references, and limited final-map
address/alias/provider observations. It deliberately does not infer discarded
same-name definitions, the pre-`/OPT:REF` survivor set, or causal
`/OPT:ICF` winner history that the receipt/object/map artifacts do not expose.
Missing or stale object files leave honest receipt-only fields rather than
silently analyzing different artifacts.

When a same-object-set fold discriminator is needed, the existing final-build
path is sufficient: compile the diagnostic bundle once, then relink that
unchanged validated bundle with:

```powershell
python tools/recoil.py verify final-build -- --reuse-compile --link-profile vc5sp3_ref_noicf <the-same-compile-and-source-profile-options>
```

Keep every source/compile-profile option identical to
the original diagnostic command; `--reuse-compile` rejects changed object,
resource, header-trace, or compile-context state. The `/OPT:NOICF` result is a
diagnostic control and never replaces the production `/OPT:ICF` linked gate.

### 2. `authored-byte-match`

After the complete authored-order prefix reaches `0x4cb9e8`, restart at the
first `authored-body` or `authored-lifecycle-body` retail row and skip proven
`non-authored` rows plus explicit compiler-generated lifecycle roles. Traverse
in retail order while editing and accepting the
complete source-shaped owner. Each row needs one synchronized receipt proving
exact extent and object bytes outside relocations, relocation types and
symbol/provider identities/addends, linked presence, symbolic call/reference
target identity, and exact relocation-normalized linked body bytes at the
candidate address. Exact retail RVA and fully resolved relocation operands are
diagnostic here: they are not authored-byte failures and become blocking in the
full-order and resolved linked-byte phases.

Accept only the current authored/lifecycle row or a contiguous authored bundle
beginning there after filtering out intervening proven non-authored rows. Owner
`Reimplemented [S]` remains governed by its complete unified-tracker owner/data
gates. Entry-local authored-byte evidence does not imply owner `S`, provider
acceptance, or later full/link prefix acceptance.

### 3. `full-function-order`

After authored-byte traversal is complete, restart at `0x401000`. This phase
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

### 4. `linked-byte-match`

After every full-order row is accepted, restart at `0x401000`. Traverse every
linked row by retail address. Require exact linked RVA/address, exact resolved
relocation operands, exact symbolic call/reference targets, and exact linked-
image bytes. Earlier authored-byte receipts remain useful input, but their
relocation-normalized candidate-address comparison cannot substitute for this
resolved retail-address gate. Accept only the current address or a contiguous
bundle beginning there.

### 5. `final-validation`

Run one unrestricted synchronized build:

```powershell
python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil.py verify final-build
python tools/recoil.py audit final-data --strict --json-out build/vc5-final/final_data_diff.json
python tools/recoil.py progress evidence import-final-data --report build/vc5-final/final_data_diff.json --expected-revision <revision> --dry-run
python tools/recoil.py audit final-repro --strict --output build/vc5-final/final_repro.json
python tools/recoil.py progress evidence import-final-repro --report build/vc5-final/final_repro.json --expected-revision <revision> --dry-run
python tools/recoil.py progress audit --scope pipeline --strict
```

Receipt imports record hash-bound observations only. Review normalized state
with `progress output-section show` and `progress storage show`; accept each
storage/section dimension explicitly with dry-run-first `progress accept
storage` and `progress accept section`. Final acceptance requires every
authored/full order and authored/linked byte row, every required order target,
every mandatory whole output section, resource comparison,
PE/data/import/provider/address validation, an
exact final-repro receipt, and candidate size/SHA-256 matching retail. `--no-pe-compare`,
`--no-resource-compare`, skipped required order checks, normalized comparison,
or a stale map/candidate makes the run diagnostic-only.

To census ordinary compiler failures without linking while required order
targets are active, run `python tools/recoil.py verify final-build --
--compile-only --keep-going --compile-only-skip-linked-order`. This explicit
compile-only diagnostic bypasses linked-order target evaluation only because no
map is produced. It is rejected without `--compile-only`, writes no linked-order
receipt, and cannot satisfy authored order, full order, or final acceptance.
The unrestricted final-validation command above never uses this option.

## Commands and Handoffs

Use:

```powershell
python tools/recoil.py progress status
python tools/recoil.py progress next --json
python tools/recoil.py progress audit --scope all --strict
```

Only the parent mutates the unified tracker, dry-run first and against the
reviewed revision, through narrow `progress accept`, `progress blocker`,
`progress owner`, `progress block`, `progress semantic`, `progress work`, or
`progress evidence` operations.
Receipts must bind the retail reference, VC5SP3 profile, source/manifest/object/
map/candidate identities, exact gates, command, scope, and conclusion; never
store a concrete `.devspace` dependency. SHA-256 is machine-managed integrity
metadata: tools and receipts calculate and validate it, while agents cite the
receipt/evidence path or imported evidence id instead of copying digests unless
diagnosing an integrity mismatch. Evidence generation/import never promotes
owner gates, tiers, provider classification, global prefixes, or `Model:`
metadata.

A source-worker/verifier handoff names the global phase and its primary cursor
(plus `fallback_authored_byte_cursor` only when the scheduler returns it),
physical block, complete owner, allowed files, first divergence, exact command,
required receipt, and exit gate. Reports include expected/actual definition counts,
informational raw extras, blocking missing/duplicate/reordered expected
identities, linked seam/address/byte results, and the first
unresolved address. Ordinary owner, final-data/final-repro, functional, and
`messages.dll` queues are `deferred_by_pipeline_phase` unless required by the
current Recoil.exe cursor.
Final-data and final-repro are evidence producers, not work units, queues, or
peer schedulers; they never generate owner-action batches.

## Debt-Free Session Close

Before final response, fix every regression introduced by the session; remove
temporary probes and failed variants; promote conclusions and receipt/evidence
paths or imported evidence ids;
leave exactly one truthful cursor/blocker with an exact next command; update or
close structured work items; run touched checks and strict workspace/progress/
agent audits; and
wait for workers/Pro consumers. A source blocker is legitimate backlog only
when its address/range, evidence, failed command/result, accepted checkpoint,
and next command are durable. Parent-only `.devspace` cleanup is the last
workspace action. Do not finalize with stale work items/cursors, hidden failures,
false claims, unregistered evidence, or any other session-introduced debt.
