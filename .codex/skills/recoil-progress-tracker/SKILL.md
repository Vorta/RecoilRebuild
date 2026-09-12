---
name: recoil-progress-tracker
description: Inspect and safely update the serial Recoil reconstruction tracker, its current task, relationships, gates, order, call-contract, byte, and final-image state.
---

# Recoil Progress Tracker

`.agent/RECONSTRUCTION_PROGRESS.sqlite3` is the sole reconstruction-progress authority. Never edit it with SQLite tools or application code. Use `python tools/recoil.py progress ...` only.

Schema versions are owned by tool migrations. Its revision vector contains
exactly:

- `transaction_revision`
- `semantic_revision`
- `evidence_generation_revision`

There is no scheduler revision, work-item collection, packet, claim, lease, reservation, lane allocator, or generated-current cache.

## Current task

With no explicit target, run:

```powershell
python tools/recoil.py progress next --json
```

It returns one `recoil-current-task-v2` object with one task, check command,
optional serial `stage_runner_command`, and—when ready—one direct acceptance
command. Work on that task in the canonical checkout. Do not allocate or hand
it off.

Strict stage order is:

1. authored function order
2. authored call contracts
3. authored bytes
4. full linked function order
5. linked bytes
6. final validation

## Mutation rules

For reviewed names-only synchronization, use `progress symbol rename-batch`
with `--payload-file <build/path.json> --expected-revision <revision>`; dry-run,
review, then apply. The `recoil-symbol-names-v1` payload requires `reviewed:true`,
`binary`, and `renames`. Each rename contains `symbol_id`, `expected_name`,
`name`, and `reason`. Optional `owner_renames` rows contain `owner_id`, `address`,
`expected_name`, `name`, and `reason`: a null address selects the owner display
name; an existing address selects its address-metadata name. These are exact
old-name guards. This command changes no identity, relationship, gate, tier,
source path, evidence, or acceptance fact. Use the owner commands for actual
ownership changes. Validate tool changes through `recoil-validation`.

After source identifiers and verification registrations are synchronized, use
`progress relocation-target refresh-source-names` for existing ordinary target,
native EH, and reviewed-exception source bindings. Its reviewed
`recoil-relocation-source-names-v1` payload contains `renames` with
`source_symbol_id`, `expected_object_symbol`, `object_symbol`,
`expected_occurrences`, and `reason`. Dry-run and review before apply with the
expected revision. Every affected source snapshot must equal the current
registration except for that spelling; complete retail/target/provider contexts
must still validate. This route changes no target spelling, ownership, extent,
operand, exception policy, or acceptance fact. Other binding families require
their owning reviewed procedure. Follow it with a fresh full match refresh.

Function match classification is distinct from serial stage acceptance. Use
`progress match refresh --all` or `--at <address>` with a fresh build root,
expected revision and dry-run/apply to record complete live `byte`/`instruction`/`commutative`
proofs and synchronize `@recoil-match` source mirrors. This does not move the
scheduler. Register specific Pro fallback eligibility through
`progress match review-instruction --payload-file ...` or
`progress match review-commutative --payload-file ...` (dry-run, review, apply).
The executable runbook owns their exact payloads and the commutative FP contract.
Old comparisons, annotations,
and Pro answers cannot replace a fresh build. Keep exact-byte dimensions false
for both relaxed levels; serial byte stages use their explicit approved
alternatives, retaining every relocation and placement obligation. Report
commutative matches separately with their conditional domain assumptions.

A reviewed provisional extension of a winner-unknown compiler ICF group may
have call-only authority while its owner's source/linkage gates remain pending.
Use `progress call-contract bind-icf-extension --payload-file ...
--expected-revision ... --dry-run --json`, review the exact diff, then apply.
The payload contains `reviewed`, `reason`, exact `expected_alias`, and a
`winner-unknown-icf-call-only-extension-v1` `contract`. It binds the exact group,
alias, semantic owner, reconstruction symbol, source edge, accepted review,
caller, instruction/operand, ABI, and accepted/withheld dimensions. This route
accepts no reconstruction facts. The base order members retain their strict
owner and governed-order requirements. The extension never enters a global
alias/group map or any order population.

V1 requires a void fastcall callback with one ECX record-pointer argument,
no stack arguments or cleanup, and no consumed return. Fresh call verification
requires one local external definition in the exact production TU and an exact
call relocation to it. It conservatively requires the complete caller's retail
instruction shape, operand positions/types and local control-flow targets;
changed shapes require a separate reviewed ABI proof. The empty callback's
current return behavior is also checked. These checks grant no body-byte,
original-TU, winner, provider, aggregate owner-gate or tier acceptance. Ordinary
call census, target comparisons and the final fresh closeout remain mandatory.

Live self-validating commands rebuild current source and apply their own CAS-guarded result:

```powershell
python tools/recoil.py progress advance-live-order --target <id> --build-root <fresh-root> --expected-revision <revision> --apply --json
python tools/recoil.py progress advance-live-call-contract --slice <slice-id> --build-root <fresh-root> --expected-semantic-revision <semantic> --expected-evidence-generation-revision <evidence> --apply --json
python tools/recoil.py progress call-contract close-live --build-root <fresh-root> --expected-semantic-revision <semantic> --expected-evidence-generation-revision <evidence> --apply --json
python tools/recoil.py progress advance-live-authored-byte --build-root <fresh-root> --expected-revision <revision> --apply --json
python tools/recoil.py progress advance-live-linked-byte --build-root <fresh-root> --expected-revision <revision> --apply --json
```

For authored call contracts, the normal serial whole-stage route is:

```powershell
python tools/recoil.py progress call-contract replay-live --dry-run --json
python tools/recoil.py progress call-contract replay-live --apply --json
```

Dry-run plans the complete original-slice census without building, querying
Binary Ninja, or mutating. Apply performs one invocation-local full-census
proof: it discovers source once, builds each unique target and separate
definition TU once, shares one COD index and target-qualified Binary Ninja fact
cache, then projects the proof back onto the immutable original slices. Those
projections use the same per-body evidence shape and serial semantic/evidence
CAS as direct slice acceptance. Replay revalidates already-current predecessor
slices, stops after committing only the passing bodies in the first divergent
current slice, leaves later slices untouched, and returns but never runs the
mandatory `close-live` command. Its fresh `-replay-NNN` sibling root never
consumes the scheduler-selected direct root; an interrupted root is inert and a
later invocation selects a new sibling.

`close-live` runs each fresh no-reuse slice verifier in an isolated build root
with bounded subprocess concurrency (eight by default; `--max-workers` accepts
one through 21).

It validates and stores the transcripts in retail order. Only the read-only
scans overlap. It next runs exactly one fresh canonical
whole-program linkability diagnostic before one serial mutation. The diagnostic
compiles sources, aliases, and resources and produces an executable and map,
but suppresses linked-order evaluation, playground deployment, and all byte,
linked-order, or final-image acceptance. Never link per slice.

Use `verify call-contract` and `advance-live-call-contract` directly only for
the first divergent current slice or another focused one-slice diagnosis.

For manual semantic mutations—owner topology, provider/classification decisions, catalog exceptions, target bindings, positive gates, or tiers—run the command with `--dry-run`, review the complete diff, then repeat unchanged with `--apply` and the expected revision. Conservative downgrades use the governed downgrade route.

Named internal read-only data may use its stable object-name stem in a reviewed
relocation-target binding. The byte verifier proves the compiler's suffixed
static symbol against exact retail storage and the complete registered reader
population; do not invent a compiler ordinal for the expected identity.
An exact single-TU `$S` decimal-suffix selector may coexist with that reviewed
stem. The verifier reconciles only that same named-static family and retains
the complete storage, contents, reader and linked proof. Competing patterns,
other source files, stale bindings and interior offsets remain unresolved.
For older data rows without a duplicate `ownership_state`, the current reviewed
binding's exact primary-data relationship supplies the ownership witness. Its
owner, target, relationship and source context must still pass live staleness
checks; recording an ownership flag is neither required nor a substitute.

For a native VC5 unwind-only EH parent, use
`progress relocation-target bind-native-eh --payload-file <reviewed-json> --expected-revision <revision> --dry-run --json`,
review its derived source/provider/retail/runtime context, then repeat with
`--apply`. The exact payload fields are `reviewed: true`, `source_symbol_id`,
`object_symbol`, `provider_owner_id`, `evidence_ids`, and `reason`. The handler
must retain its existing non-authored classification. If it has a primary owner,
that provider must be accepted and exclusive. A discovered packet lifecycle is
eligible when its provider boundary gate is accepted; retain that pending
lifecycle in the binding and require renewed review if it changes. This
dependency binding does not accept the packet owner's lifecycle. For an already classified generated
EH helper with unresolved ownership and no primary-owner relationship, use
`provider_owner_id: null` with existing parent-function evidence; ownership stays
unresolved and adding an owner later makes the binding stale. The command binds the native
handler role and the canonical runtime's absolute `__except_list` symbol;
it accepts no provider bytes, image storage, owner gates, or tiers. Fresh byte
verification requires the parent-associated COFF sections and the complete
linked packet relocation graph. Keep FS:[0] relocations; never invent a data
extent at address zero or use a generated label ordinal as expected identity.

After correcting source registrations, the native EH route accepts optional
`expected_binding` with the complete exact stored binding. Dry-run and review
before apply. It re-derives retail, runtime and provider context and permits only
the source snapshot's `registration_ids` to change. Source identity, extent,
handler, unwind facts, runtime proof, provider relationship and evidence must
remain identical. This refresh accepts no function or provider bytes; follow it
with fresh complete consumer and linked EH-packet verification.

For a compiler-generated implicit cleanup selected by native member-array
construction or destruction, use `progress relocation-target bind-native-array-cleanup` with
`--payload-file`, `--expected-revision`, and `--dry-run --json`, review, then
repeat with `--apply`. Its exact payload fields are `reviewed: true`,
`source_symbol_id`, `object_symbol`, `offset` (the relocation field), `owner_id`,
`evidence_ids`, and `reason`. Retail derives the cleanup/element/helper targets,
array count, stride, member offset, and helper callback ABI. Destruction also
requires one current reviewed construction dependency of the same parent class
and member array to derive its element identity; stale or ambiguous pairs block.
Current source must
retain the native array and implicit destructor hierarchy. Fresh verification
checks the cleanup's executable fragment, nested table relocation, linked symbol
resolution, and consistent physical mapping across checked references. This
binds a dependency to an existing generated lifecycle target; it does not create
an authored ICF group or accept an original alias census, source model, owner
gate, or tier. The original folded winner remains unresolved when unproved.

For a direct authored reference to an existing non-authored named MSVCRT or
AVIFIL32 import thunk, or an ordinal MFC42 import thunk, use
`progress relocation-target bind-native-import --payload-file <reviewed-json>
--expected-revision <revision> --dry-run --json`, review, then repeat with
`--apply`. The exact payload fields are `reviewed: true`, `source_symbol_id`,
`object_symbol`, `offset`, `target_symbol`, `evidence_ids`, and `reason`. By
default the target must be unowned. Optional `provider_owner_id` selects one
existing exclusive primary provider with accepted lifecycle, boundary and source
gates. This preserves the complete owner record, including any absent historical
owner evidence, and requires current parent-function evidence plus the same
fresh canonical import proof. Owner, relationship, gate, recipe or evidence
changes make the binding stale and require renewed review. Targets with a
separately registered typed identity use the ordinary target route. The
proof derives the exact retail IAT/DLL/name or ordinal, requires one matching canonical VC5
long import member and its code and lookup tables, then checks the current
COFF reference and linked thunk/import. Existing ownership and the inventory
extent remain unchanged; no provider body, padding, storage, or gate is accepted.
Named imports require exact name-table relocations. AVIFIL32 uses the canonical
VC5 `VC/LIB/VFW32.LIB`, `AVIFIL32.dll` member and AVIFIL32 descriptor dependency.
Ordinal MFC42 imports
require the exact ordinal word in both lookup tables, no lookup relocations or
name table, and the canonical MFC42 descriptor dependency. Neither ownership
case accepts a new owner or provider fact.

After a governed source registration move, the same native-import route accepts
an optional `expected_binding` containing the complete exact stored binding.
Dry-run and review before apply. It re-derives retail and canonical-import facts
and permits only the source binding's `registration_ids` to change; source
identity/extent, call operand, target, IAT, import, provider recipe, and evidence
must remain identical. A missing, duplicate, or stale old snapshot fails. The
refresh accepts no provider or function bytes and still requires fresh live proof.

To retract a superseded relocation exception, use
`progress relocation-exception remove` with the source id/address, the complete
stored exception as `--payload-json`, a `--reason`, and the expected revision.
Dry-run and review first. Removal changes only that exact exception; it retains
target/owner facts and accepts no replacement evidence. Bind the replacement
identity separately and require fresh live byte acceptance.

An ordinary reviewed relocation exception may select the one-past-end bound of
an existing data object for a retail `CMP reg32, imm32` operand. Register its
known extent first and use that exact size as both addends; supply the existing
object's identity, not the adjacent object's start. Mutation and fresh live
derivation verify the immutable comparison and current extent. Dereferences,
other instructions, unknown extents and out-of-range addends remain blocked.
This site-specific review changes neither ordinary half-open target lookup nor
data ownership, storage acceptance or function match requirements.

For a real function whose typed extent includes an unreferenced INT3 alignment
tail, use `progress symbol separate-tail-padding --payload-file ...` with an
exact `recoil-function-tail-padding-v1` snapshot, dry-run first. The route checks
current retail and BN instructions/references and preserves the function's
identity, block, owner and call sites. It accepts neither body bytes nor padding;
resume fresh serial byte acceptance after reviewing and applying the correction.

For a reviewed current implementation-path move, edit the repository paths,
synchronize every affected verification target first, then use the exact-match
relocation route rather than composing owner/block/source-trace mutations:

```powershell
python tools/recoil.py progress source-path relocate --payload-file build/reviewed-source-path-relocation.json --expected-revision <revision> --dry-run --json
python tools/recoil.py progress source-path relocate --payload-file build/reviewed-source-path-relocation.json --expected-revision <revision> --apply --json
```

The v1 payload must enumerate the complete matching physical-block,
semantic-span, owner, artifact, and pre-synchronized verification-target id
sets. This route changes only current implementation paths, retains historical
provenance, and conservatively invalidates dependent order, call-contract, and
byte facts.

For a reviewed partial TU extraction that retains both production files, use
`progress source-path extract` after editing the files and synchronizing all
affected verification targets. `--old-source`, `--new-source`, an expected
revision and `--prepare --json` produce an unreviewed exact snapshot payload.
Every definition requires its exact resolved defining edge in the old TU or an
already synchronized edge in the new TU. Register missing current topology
through `progress source-trace replace-batch` first; extraction does not invent
absent defining evidence.
Review it, set its `reviewed` and nonempty `reason` fields, then use
`--payload-file build/reviewed-extraction.json` with `--dry-run --json`, review
the proposed changes, and repeat with `--apply --json`. This route authenticates
the complete new TU's attached authored function definitions, retains current
semantic identities and the retail block grid, updates defining source edges
and owner paths, and invalidates affected order/call/byte and source/linkage
gates. It retracts broad filename inference only on already-unresolved mappings
without evidence, retaining the prior observation in history. For an unresolved
mapping with evidence, it preserves the original path, complete mapping and
evidence while recording the current implementation extraction separately.
Repeated extractions retain the complete current semantic-span path set from
resolved defining edges; unresolved members keep prior path observations.
It refuses accepted historical filename mappings. It accepts no original
filename, source model, provider, owner tier, or body match. Use the source-model
skill's required review before proposing a disputed compilation boundary.

For positive existing-storage or owner acceptance, use the scoped live commands
and review-template procedure in
`docs/reconstruction/retail_executable_reproduction.md`. Registration and owner
replacement are not substitutes for these guarded acceptance operations.

Use `recoil-validation` after tracker-tool changes. A passing validation changes only the dimension named by that command.
