# Recoil Tooling

`python tools/recoil.py` is the only agent-facing entry point. Root
`AGENTS.md` and
`docs/reconstruction/retail_executable_reproduction.md` define the current
workflow.

## Live validation model

Validation operates on current source and newly generated build artifacts.
Candidate-file identity, sealed observations, cached compile bundles, and
persisted content summaries are not part of acceptance. A parent call-contract
acceptance operation authenticates its packet and physical output root, builds
current source once, reads expected facts directly from retail and the governed
Binary Ninja saved view, and directly compares each selected body. Only bodies
that pass in that invocation advance.

Progress concurrency uses the monotonic transaction, semantic,
evidence-generation, and scheduler revision domains in
`.agent/RECONSTRUCTION_PROGRESS.sqlite3`. Workspace issues are stored
independently in `.agent/WORKSPACE_ISSUES.sqlite3` and use their own monotonic
issue revision. Routine database no-mutation evidence is the before/after
revision, schema/user version, relevant row counts, and
`PRAGMA integrity_check`; CAS guards every mutation. After the paired cutover,
both runtime authorities and rollback are SQLite-only; tooling provides no JSON
backend, mirror, or export.

Git is the authored-workspace change-control mechanism. A packet requires a
clean reviewed branch, records its opaque baseline commit and exact writable
closure, and uses porcelain-v2 plus commit-relative name-status/diff at
closeout. Every changed path and both rename endpoints must be writable; an
ordinary copy whose source remains unchanged requires only destination write
authority, and unresolved Git state is an unconditional blocker. Git governs
maintained authored inputs. Ignored paths are generated or machine-local and
are nonauthoritative; ignored generated-file churn is not packet-closeout
evidence. Validation and build output should normally use external or isolated
roots to avoid clutter, but generated-file presence is a hygiene concern rather
than reconstruction acceptance. Generated files never supply source, expected
truth, manifest, profile, or acceptance evidence. Git commits are nonaccepting
workspace change bundles, and native object IDs remain opaque repository state
rather than retail or candidate evidence.

Workspace-issue packet isolation is orchestrator-owned:

```powershell
python tools/recoil.py workspace worktree status --json
python tools/recoil.py workspace worktree create --authority issue --id <packet-id> --expected-revision <issue-revision> --apply
python tools/recoil.py workspace worktree validate --id <packet-id> --json
python tools/recoil.py workspace worktree integrate --id <packet-id> --apply
python tools/recoil.py workspace worktree retire --id <packet-id> --apply
python tools/recoil.py workspace worktree hygiene --strict --json
```

Each active issue packet owns one `packet/` branch, one linked worktree, one
external sibling build root, and one central reservation. A strict Git
worktree-lock association binds the packet id and build-root path; the build
root also carries an exact ownership marker and Windows physical directory
identity. Handoff and closeout resolve the stored branch to that exact
worktree. A worker may stage only its exact writable closure and create one
nonaccepting packet-id commit; branch/worktree/integration/retirement remain
orchestrator operations. Integration completes every fallible compiler, test,
audit, and doctor check in a temporary worktree before canonical `master`
advances. After the fast-forward, only deterministic Git, topology, tag, and
physical-identity assertions are permitted. Linked validation uses tracked
source, tools, tests, manifests, policies, and `.agent/REFERENCE_EXECUTABLE.json`
from the executing worktree, while the validated canonical control root supplies
machine-local `support/Recoil.exe` and live progress/issue SQLite databases.
Those inputs are never copied or linked into the linked checkout. Retirement
removes the merged packet branch,
worktree, and authenticated build root. Absolute checkout and build-root
prefixes are diagnostic provenance, not retail expected truth. The progress
worktree adapter is `contained-disabled`: progress packets do not yet record a
native-Git baseline.

Packet output roots are owned by Windows physical directory identity, not just
path or marker text. Allocation records the volume identity, stable file
ID/index, packet id, allocation operation id, tracker identity, and canonical
path. Handoff, compiler/BN entry, worker return, and deletion reopen the root
reparse-safely and require the same physical identity plus exact marker fields;
a same-path replacement is rejected even if marker text was copied.

The core loop is:

```powershell
python tools/recoil.py progress next --json
python tools/recoil.py progress show <selector>
python tools/recoil.py progress work leases --json
python tools/recoil.py progress work claim-current --lane all --max-packets <available-child-slots> --expected-revision <revision> --apply --json
python tools/recoil.py progress handoff --packet-id <packet-id> --json
python tools/recoil.py verify vc5-order <registered-target> --build-root <new-isolated-root>
```

With no explicit target, a bare `Start` is sufficient. The root parent computes
remaining child slots from effective runtime capacity, applies the multi-lane
claim without waiting for more user confirmation, renders every returned
packet, and launches compatible workers. Fixed priority is primary order, full
authored byte, then subordinate authored-object byte; here the primary packet
category means the current primary lane, order or call-contract. A blocked
primary does
not suppress compatible bytes; full authored byte wins over overlapping new
object work. Actual resource conflicts and capacity skips are tool-owned.
Individual `--lane <primary|authored|object>` claims remain supported.

`progress status`, `next`, `show`, `report`, work preview, deterministic slice
projection, and closeout projection are operationally pure. They read explicit
SQLite rows and revision coordinates: they never compile, preprocess, link,
query Binary Ninja, derive expected facts, create temporary roots, or write a
cache, ledger, or generated document. A query may report whether current
evidence carries the reviewed integer generations, but it never verifies a
body. Fresh verification is performed only by the registered worker diagnostic
or parent acceptance command.

Explicit user-selected maintenance is a parent-only supplement to the sole
automatic scheduler, not another queue. `progress work create-explicit`
validates an exact registered scope, minimal file closure, a fully registered
nonaccepting public validation command, and normalized claims before any
compiler or Binary Ninja work. Dry-run is projection-only. Apply first records
a durable non-work allocation journal with no reservation or normal claims,
then creates a deterministic authenticated ownership sidecar, the exact
`build/` root, and its in-root marker. After all root, issue, revision, conflict,
claim, and response checks pass, one final scheduler CAS atomically creates the
active packet and reservation. No fallible root check follows that activation.
Failure leaves the journal recoverable and removes only sidecar-authenticated
content. Handoff, worker validation, and governed BN reads
reject pending, missing, replaced, marker-mismatched, or escaping roots.

The complete nonaccepting explicit-packet lifecycle is:

```powershell
python tools/recoil.py progress work create-explicit --payload-file build/diagnostics/<packet>.json --expected-scheduler-revision <scheduler-revision> --expected-semantic-revision <semantic-revision> --dry-run --json
python tools/recoil.py progress work create-explicit --payload-file build/diagnostics/<packet>.json --expected-scheduler-revision <scheduler-revision> --expected-semantic-revision <semantic-revision> --apply --json
python tools/recoil.py progress handoff --packet-id <packet-id> --json
python tools/recoil.py progress work return --id <packet-id> --result-json '<bounded-result>' --expected-scheduler-revision <scheduler-revision> --apply --json
python tools/recoil.py progress work return-binja --id <packet-id> --read-plan-json '<governed-plan>' --result-json '<bounded-result>' --expected-scheduler-revision <scheduler-revision> --apply --json
python tools/recoil.py progress work close <packet-id> --expected-scheduler-revision <scheduler-revision> --apply --json
python tools/recoil.py progress work recover-expired --id <packet-id> --expected-scheduler-revision <scheduler-revision> --apply --json
python tools/recoil.py progress work recover-allocation --id <packet-id> --expected-scheduler-revision <scheduler-revision> --expected-semantic-revision <semantic-revision> --dry-run --json
# Repeat with --apply only after reviewing the independently observed recovery projection.
```

`source-maintenance` requires exact writable source/header files;
`read-only-diagnostic` forbids them. A reviewed cross-owner override is carried
in the immutable related-owner snapshot, owner/resource claims, explicit
invalidation scope, handoff, and return receipt. It records a reviewed
dependency and grants no source ownership or acceptance. A worker PASS is
feedback only. Packet
lifecycle changes only transaction/scheduler state and cannot accept source,
calls, order, bytes, profiles, providers, owners, tiers, or phases.

`verify vc5-order` compiles the assigned source directly and reports the first
missing, duplicate, unexpected, or reordered retail identity with neighbors.
One registered target may cover one translation unit and several explicit,
contiguous physical-block slices. `claim-current` creates and reserves the real
compact packet; `handoff` only renders that active reservation and never exposes
a parent mutation command. Live work-item insertion is centralized in the
`claim-current` path. Every packet newly stored by `claim-current`, and every
compact rendering of that packet, carries the same `claim_provenance` object
with schema version 1, the fixed
`progress work claim-current` command, requested and selected lanes, and the
actual positive `max_packets` value. It contains no process, session, or
temporary-path identity. Read-only scheduler views, verification-target
synchronization, and work return/close transitions never insert packets.
An unresolved row anywhere in the registered interval blocks the whole
physical-block packet and acceptance. A resolved-subset raw diagnostic PASS
cannot launch or accept a partial block.
Ordinary comment prose never supplies acceptance evidence. Canonical source
markers are validated as mirrors of tracker-owned artifact relationships, and
`audit docblocks` separately enforces comment hygiene across Doxygen blocks,
ordinary multiline comments, and contiguous line-comment groups. Standalone
symbol titles are permitted as deliberate labels, with or without terminal
punctuation, but grant no evidence, artifact identity, source edge, or
acceptance. Standalone source paths, symbol-plus-path rows, routing/lifecycle
placeholders, and exact repeated semantic rows remain hygiene findings;
identities belong in canonical artifact rows, while prose explains purpose or
evidence. The `order-edit-v1` loop has no byte, BN, Pro, evidence-package, or
candidate-content qualification dependency.

Source guards share one VC5-era construct inventory for functions, data,
`__declspec` definitions, and recognized MFC source-generation macros. A
canonical direct `@recoil-artifact defines ...` edge is proof for the attached
construct; legacy `(path, name)` allowlists and free-text helper claims are
migration inventory, not proof. Generated artifacts use their exact physical or
reviewed logical tracker id and exact registered final section.

`guard vc5-manifest --strict-source-emissions` performs one strict manifest
corpus traversal. During that process it caches immutable production include
closures, parsed source-trace documents, exact artifact lookups, and the
tracker artifact index. An exact tracker `source_traceability.state:
unresolved` row remains a visible `source-emission-warning` and is counted in
the successful summary; missing/null state, malformed identity, a contradictory
canonical edge, or a resolved row without its exact canonical artifact still
fails closed.

Stable raw-assembly approval uses
`@recoil-raw-asm recoil:raw-asm:<id>` on the raw region and
`@recoil-raw-consumer recoil:raw-asm:<id> [recoil:function:0x...]` on each
consumer. The canonical raw-assembly allowlist keys each row by region id,
consumer artifact id, and token; every token's consumer set must match exactly.
The optional explicit function id supports shared header macros without
creating a production source edge.

`.devspace` is never a general generated-output exemption. A direct child may
exempt only the generated files listed with exact relative paths and byte sizes
in `.recoil-workspace-hygiene.json`, using schema
`recoil-governed-session-scratch-v1`, `status: complete`,
`purpose: compiler-probe`, and the exact current `repo_root`. Malformed,
escaping, unlisted, reparse-point, wrong-repository, or size-mismatched entries
exempt nothing.

After authored order completed, the reviewed one-time migration initialized
the then-current census of exactly 3,380 physical
`authored-body`/`authored-lifecycle-body` rows with independent
`binary_state.call_contract` state. That number is migration history, not a
permanent gate. The permanent `authored-call-contract` stage derives its live
gating population from current reviewed classification and selects deterministic
retail-monotonic slices capped at 160 bodies before allowing full order. A
`call-contract-edit-v1` worker uses one nonmutating command:

```powershell
python tools/recoil.py verify call-contract --slice <slice-id> --progress .agent/RECONSTRUCTION_PROGRESS.sqlite3 --build-root <fresh-root> --json
```

The verifier freshly compiles the accepted authored-order target/source
closure and compares exact static call count/order, target/provider/IAT
identity, direct versus indirect dispatch, virtual/interface slot or callback
storage, call versus tail form, and known cleanup with current retail Binary
Ninja evidence. Local branches are ignored and every unresolved identity
blocks. The contained parent route requires an active packet, its authenticated
physical root, the exact `binary-ninja-db:Recoil.bndb:read` claim, and separate
semantic and evidence-generation revision guards before any compiler or Binary
Ninja work. It performs one fresh build and direct retail comparison, then may
CAS-accept only bodies that passed in that same invocation. Stored body results
and prior scans are nonaccepting. Current evidence carries only the reviewed
integer coordinates `CALL_CONTRACT_VERIFIER_GENERATION = 8`,
`NORMALIZER_REGISTRY_GENERATION = 8`, and
`EXPECTED_FACT_SCHEMA_VERSION = 8`; governed component edits require the
corresponding increment and conservative invalidation. Compatible byte packets
remain independently launchable; full order remains blocked while any body is
not current or the fresh no-reuse zero-divergence closeout has not passed.

During `full-function-order`, routing is deliberately dual-target. The packet's
`target_id` and `linked_target_id` remain the exact registered linked acceptance
authority, while `object_target_id`/`worker_target_id` identify the exact
registered compiling target used by the worker's sole `verify vc5-order`
command. The parent command exposes both explicitly:

```powershell
python tools/recoil.py progress advance-live-order --target <linked-target-id> --object-target <object-target-id> --build-root <fresh-root> --expected-revision <revision> --apply --json
```

Both registrations must be current, unique Recoil VC5 targets with the same
exact retail interval and physical-block coverage. A missing/stale/ambiguous
target, a linked-only target used as the object compiler, or any binary,
interval, population, or coverage mismatch blocks before the build. The object
target is worker feedback only: parent full-order acceptance rebuilds and runs
the linked target's typed full linked-order verifier. Authored-order routing
continues to use its registered object target directly.

An order manifest may declare optional `order_edit_paths` when natural-order
repair needs existing C/C++ sources or headers beyond `source_from` and the
tracker's source-shape inputs. Each entry must be an exact normalized,
repository-local path to an existing C/C++ source/header file. After the
manifest is synchronized into its verification-target registration, these
paths expand only the writable paths and resource claims of a reservation-backed
primary `order-edit-v1` packet. They never become compiler inputs, registered
symbol identities, covered blocks, or order-acceptance evidence.

Ordinary `progress verification-target sync` enforces the current final source
policy. When a reviewed cross-file physical-TU recovery cannot satisfy that
policy until a worker moves the affected definitions, the parent may synchronize
exactly one explicit target with `--source-policy-bootstrap`. This registration-
only path requires a complete authored/full order sequence, an exact retail
interval, resolved order roles, and exact production `order_edit_paths` containing
the compile host plus at least one additional source/header. It accepts only a
missing-provenance placement failure and records `pending-source-placement` in
the registration and worker packet. It does not weaken `verify vc5-order` or
`advance-live-order`: both continue to enforce final source policy and exact
order before any order fact can be accepted. After the source move, an ordinary
sync clears the pending bootstrap state.

The byte lanes are also live:

```powershell
python tools/recoil.py audit relocation-expectations --at 0xNNNNNN --json
python tools/recoil.py verify authored-object-byte
python tools/recoil.py verify authored-byte
python tools/recoil.py verify linked-byte
```

Each command creates a new isolated build, selects only tracker-registered VC5
targets, and stops at the earliest real divergence. Object validation compares
authored body bytes outside relocation fields. Authored and linked validation
also compare the complete relocation population, COFF addends, symbolic target
identities, resolved target addresses, and relocation-normalized or final
linked bytes as appropriate. Expected relocation facts are derived live from
immutable retail plus accepted typed identity/provider/alias facts, never from
the candidate. Explicit empty sets are valid; genuine ambiguity blocks before
the expensive build and routes to `progress relocation-exception set`. That
revision-guarded manual mutation records exact current source/target context;
later extent, class, object, alias, provider, or evidence drift is reported as
a typed stale exception. Reports are
disposable explanations, not evidence tokens.

When immutable retail proves one exact physical `.rdata` target but the
original VC5 object-symbol provenance remains unresolved, the same command
accepts the fixed
`physical-target-unresolved-vc5-temporary` mode. Its input is declarative:
`object_symbol`, the complete `offsets` list, `type`, `target_symbol_id`,
zero `coff_addend` and `resolved_target_addend`, `retail_target`, `reason`, and
`evidence_ids`. It does not accept `target_symbol`, a candidate ordinal,
regular expression, witness structure, or binding snapshot. The tool derives
the source snapshot, exact physical data extent/section/storage snapshot,
immutable retail content, and this fixed witness contract:

```json
{
  "kind": "vc5-temporary-static-data",
  "symbol_family": "$T<digits>",
  "storage_class": 3,
  "symbol_type": 0,
  "section_name": ".rdata",
  "requires_initialized_data": true,
  "forbids_uninitialized_data": true,
  "forbids_writable_data": true,
  "one_symbol_for_all_sites": true
}
```

For the reviewed `0x402250` pair, a non-mutating invocation is:

```powershell
$payload = '{"reviewed":true,"exception_mode":"physical-target-unresolved-vc5-temporary","object_symbol":"?TickAiMode2AltGunAttackWindow@AINet@@SIXPAUzUtil_SaveGameState@@MM@Z","offsets":[245,276],"type":6,"target_symbol_id":"recoil:data:0x4cc820","coff_addend":0,"resolved_target_addend":0,"retail_target":"0x4cc820","reason":"Retail proves one exact four-byte physical 0.5f target at both DIR32 sites while original VC5 object-symbol provenance remains unresolved.","evidence_ids":["recoil:evidence:r725:000465"]}'
python tools/recoil.py progress relocation-exception set --source-symbol-id recoil:function:0x402250 --source-address 0x402250 --payload-json $payload --expected-revision <revision> --dry-run --json
```

The live authored-byte verifier expands that one reviewed group into exactly
the two expected source-body relocation rows. Separately, it derives the
complete immutable-retail reader universe for the physical target across the
current registered object. The one actual `$T<digits>` witness must have a
unique, nonempty object-wide incoming set that is a subset of that universe and
contains both reviewed `0x402250+245` and `0x402250+276` sites. Every actual
incoming row must map one-to-one by exact registered object symbol, function
offset, DIR32 type, and zero addend to a retail-proved reader. This permits VC5
to pool the same four-byte witness with other proved readers in the object
without treating the candidate-selected reader subset as expected truth.

The witness remains one unique repeated `$T<digits>` STATIC/type-0 symbol in
one exact initialized, non-writable `.rdata` extent with no internal
relocations and bytes equal to the stored immutable-retail content. A missing
reviewed site; an outside, unresolved, duplicate, provider, alias, wrong-type,
wrong-addend, or unregistered reader; mixed symbols; `$S` substitution;
duplicate storage; content/extent/type/section/characteristic drift; target-row
drift; linked placement ambiguity; or changed source/evidence context fails
closed. Neither the candidate ordinal nor its selected reader subset is
persisted or treated as retail truth.

VC5 target `data_symbols` rows may set `object_offset` to a non-negative
integer (default `0`) when the retail range corresponds to a member slice
inside the named COFF data symbol. The verifier compares exactly `byte_length`
bytes beginning at the named symbol plus that offset. It rejects offsets or
nonzero-offset slices outside the COFF symbol/section extent and relocation
fields crossing a slice boundary; relocation masks and reports are relative to
the selected slice.

When retail determines the operand but its typed existing or exact known-extent
target identity is missing, use the separate dry-run-first `progress
relocation-target bind` route. `relocation-exception set` is only for genuine
reviewed ambiguity. Neither command derives expected facts from candidate
output.

Final validation is:

```powershell
python tools/recoil.py audit final-image-catalog --json
python tools/recoil.py verify final-image --json
```

`audit final-image-catalog` derives live interval coverage from verified retail
and accepted tracker facts. It does not require a manually populated legacy
catalog blob and reports `legacy_catalog_required: false`. Concrete gaps,
overlaps, unknown extents, ambiguity, and missing
providers block. `verify final-image` fails before building until coverage is
complete, then performs one fresh unrestricted typed comparison. The COFF
timestamp and raw whole-file differences are diagnostic only.

## Tracker operations

`.agent/RECONSTRUCTION_PROGRESS.sqlite3` is the only progress authority. Do not
hand-edit it. Read operations include:

```powershell
python tools/recoil.py progress next --json
python tools/recoil.py progress status --json
python tools/recoil.py progress show <selector> --json
python tools/recoil.py progress find <query>
python tools/recoil.py progress audit --strict
python tools/recoil.py progress work leases --json
```

`progress audit` is fail-closed: any error finding returns a nonzero process
status whether or not the optional `--strict` compatibility flag is present.

The self-validating `progress advance-live-order` and `progress
advance-live-byte` commands normally use direct `--apply`: they build and
validate once, then CAS-mutate from that same result. The contained
`progress advance-live-call-contract` route is not currently accepting and
must not be used until an independently accepted parent producer enables it.
Full-order advance uses
the linked target for acceptance and names its separately validated object
worker target with `--object-target`. Optional `--dry-run` is diagnostic only.
Manual owner/block/provider/classification/catalog-exception/
tier mutations remain dry-run-first. All mutations use `--expected-revision`
as the sole optimistic-concurrency guard. IDs in schema v5 are revision/
sequence IDs; they do not describe content and changing a comment does not
invalidate them.

The supported owner and target maintenance routes are deliberately narrow.
Use `progress owner replace-batch` for reviewed owner topology or primary
membership replacement, `progress owner downgrade` for conservative gate/tier
decreases, `progress owner repair-primary-data-tier-x` for an exact existing
authored primary-data relationship whose conservative tier-X bookkeeping row
is absent, and `progress verification-target sync` for verification-target
registration metadata. No command supports arbitrary positive owner metadata,
gate, or tier mutation. When such a reviewed need is real, record a bounded
workspace tooling request with `python tools/recoil.py issue request ...`
instead of inventing a tracker command or hand-editing the ledger.

`progress source-trace replace-batch` also resolves the immutable legacy-claim
inventory without deleting or rewriting its original facts. A resolution-only
payload uses `updates: []` plus one or more
`legacy_claim_resolutions` records. Every record embeds the exact original
`expected_claim` and a `replacement_resolution`. An
`exact-existing-artifact` replacement names an existing artifact at the exact
claim address. An `interior-of-existing-artifact` replacement names an
existing physical artifact whose tracker row has a known, internally
consistent address/end/size extent; the claim address must be strictly inside
that extent, and the record must include a nonempty `reason` and nonempty
existing tracker `evidence_ids`. The original
`migration.source_traceability_v1.unresolved_legacy_claims` array remains
unchanged, while reviewed resolutions append to
`legacy_claim_resolutions`. The route never creates artifacts and returns
`topology_only: true` and `acceptance_changed: false`.

```json
{
  "operation": "replace-batch",
  "parent_reviewed": true,
  "updates": [],
  "legacy_claim_resolutions": [
    {
      "expected_claim": {
        "binary": "recoil",
        "kind_hint": "function",
        "address": "0xNNNNNN",
        "reason_code": "missing-artifact-identity",
        "source_path": "src/path/file.cpp"
      },
      "replacement_resolution": {
        "kind": "interior-of-existing-artifact",
        "artifact_id": "recoil:function:0xMMMMMM",
        "reason": "Reviewed retail evidence proves an interior compiler label.",
        "evidence_ids": ["recoil:evidence:rNNN:NNNNNN"]
      }
    }
  ]
}
```

Review the `--dry-run --json` result and repeat the unchanged payload and
revision with `--apply --json`.

When an interior legacy claim is proven to belong to an existing physical data
artifact but that row still has `extent_state: unknown`, register the reviewed
extent separately before resolving the claim:

```powershell
python tools/recoil.py progress data-extent register --expected-revision <revision> --payload-file <reviewed.json> --dry-run --json
```

The exact payload shape is:

```json
{
  "operation": "register-existing-data-extent",
  "parent_reviewed": true,
  "artifact_id": "recoil:data:0xNNNNNN",
  "expected_current": {
    "extent_state": "unknown",
    "address": "0xNNNNNN"
  },
  "replacement": {
    "extent_state": "known",
    "size": 62,
    "end_exclusive": "0xNNNNNN",
    "evidence_ids": [
      "recoil:evidence:rNNN:NNNNNN"
    ]
  }
}
```

The command accepts only one existing physical data artifact, requires the
current row to omit size/end while its extent is unknown, checks that
`end_exclusive == address + size`, requires every supplied evidence id to
already exist, and proves the replacement lies wholly inside the artifact's
exact retail output-section extent. It changes only `extent_state`, `size`,
`end_exclusive`, and the artifact's evidence-id union. It creates no artifact
or source edge and returns `acceptance_changed: false`. Review the dry-run and
repeat the unchanged payload and revision with `--apply --json`; workers do not
run either form.

When immutable/VC5 evidence proves a new physical data identity that is not yet
catalogued, use the separate creation route:

```powershell
python tools/recoil.py progress data-artifact register --expected-revision <revision> --payload-file <reviewed.json> --dry-run --json
```

Its `register-exact-data-artifact` payload requires an exact absent
`<binary>:data:<address>` id and an `artifact` object containing matching
`address`/`binary`, a reviewed navigation name, `output_section_id`, positive
`size`, arithmetic-exact `end_exclusive`, and a possibly-empty array of
`verification_target_ids` (every supplied target must already exist). The
artifact normally names nonempty existing `evidence_ids`. When no existing
evidence names the exact identity/address, the same atomic payload may instead
supply one strictly shaped `new_evidence` reviewed observation: the command,
target, exact observation, and repository-relative artifact sizes are
validated; one revision-scoped evidence id is allocated and linked to the new
artifact in the same CAS mutation. That non-gating observation is recorded as
`freshness: historical` with
`validation_mode: historical-observation`. It rejects an existing physical
identity at the same binary/address and proves the complete extent lies inside
the exact retail output section. The reviewed `disposition` is either:

- `authored`, paired with `source_traceability_state: unresolved`; or
- `provider`, paired with `source_traceability_state: not-applicable`.

Both require a governed lowercase `source_traceability_reason_code` and start
with no source edges. The new physical row has pending binary dimensions and
empty owner/storage relationships; the route changes no acceptance.

After a known-extent authored physical data artifact and its unique
`primary-data` owner relationship both exist, register its initially absent
storage contribution through the separate parent-only route:

```powershell
python tools/recoil.py progress storage register-authored-data --payload-file <recoil-authored-data-storage-register-v1.json> --expected-revision <revision> --dry-run --json
```

The exact payload snapshots the existing symbol and owner relationship and
names canonical matching `<binary>:data:<address>` and
`<binary>:storage:va:<address>` ids. The command requires exact
binary/section/known-extent equality, exactly one non-provider primary-data
owner, output-section containment, reciprocal current absence, and no overlap
with existing storage or physical data extents. It atomically creates one
pending `data-symbol` storage row and appends its id to the symbol. It creates
no source edge and changes no owner, gate, tier, order, byte, provider, link,
or final-image acceptance. Review the dry-run, then repeat the unchanged
payload and revision with `--apply --json`; workers do not run either form.

If an earlier invocation created a reviewed data-artifact observation with the
retired legacy currency/validation-mode schema pair, repair only that pair through the
parent-owned revision-guarded route:

```powershell
python tools/recoil.py progress data-artifact evidence repair-observation --expected-revision <revision> --payload-file <reviewed.json> --dry-run --json
```

The payload must name the exact physical artifact, exact linked evidence id,
`parent_reviewed: true`, operation
`repair-reviewed-data-artifact-observation-schema`, and the exact invalid pair
under `expected_invalid`. The route refuses drift in the artifact/evidence
link, reviewed observation kind/scope/result/disposition, non-gating status, or
`acceptance_effect: none`. It changes only freshness to `historical` and
validation mode to `historical-observation`; source edges, owner gates/tiers,
and acceptance remain unchanged. Review the dry-run and repeat the unchanged
payload/revision with `--apply --json`.

Reviewed authored logical occurrences of provider/compiler-pooled data use a
third, distinct route:

```powershell
python tools/recoil.py progress data-artifact logical-alias register-batch --expected-revision <revision> --payload-file <reviewed.json> --dry-run --json
```

The `register-logical-data-alias-batch` payload names one exact existing
physical provider data row and repeats its reviewed address, known extent,
provider disposition, and not-applicable physical source-trace state as stale
guards. Its `pooling` object must use mode `compiler-literal-pooling`, a
nonempty review reason, and existing evidence ids. Every authored logical-data
alias requires a unique exact logical id at the representative address, an
exact non-whitespace VC5 object symbol, a navigation name, independent
existing evidence ids, and an unresolved source-trace reason. Several
occurrences may deliberately share the same object symbol when the reviewed
compiler-pooling evidence says so. Address/byte equality without the object
identity and pooling evidence is rejected. The batch creates no physical
artifact, physical source edge, owner, storage contribution, or acceptance.
Canonical source edges are added later only through the separately reviewed
`source-trace replace-batch` route.

Reviewed function-row classifications use one exact, fail-closed JSON batch.
Each item supplies the existing `symbol_id` and `address`, `reviewed: true`,
the current `pipeline_class` and `authored_order_role` as staleness guards, and
one compatible replacement pair. Review the dry-run JSON, then repeat the
unchanged payload against the same revision with `--apply`:

```powershell
python tools/recoil.py progress symbol set-pipeline-class-batch --payload-json '[{"symbol_id":"recoil:function:0xNNNNNN","address":"0xNNNNNN","reviewed":true,"current_pipeline_class":"unresolved","current_authored_order_role":"unresolved","pipeline_class":"non-authored","authored_order_role":"non-authored"}]' --expected-revision <revision> --dry-run --json
```

The route accepts existing Recoil function rows only, rejects duplicate ids or
addresses and stale current values, and changes only `pipeline_class` and
`authored_order_role` before the normal revision-atomic tracker commit.

A reviewed physical ICF representative and its authored logical identities are
recorded through a separate parent-only route. First classify the physical row
as `non-authored` / `compiler-generated-icf-representative`. Then supply one
exact `recoil-logical-alias-group-v1` object whose `current` snapshot guards the
row's classification, physical block, neutral linked group, prior ICF group,
and prior logical aliases:

```powershell
python tools/recoil.py progress symbol set-logical-alias-group --payload-json '{"schema":"recoil-logical-alias-group-v1","reviewed":true,"parent_reviewed":true,"reason":"Retail vtables and body identity prove one folded address group.","symbol_id":"recoil:function:0xNNNNNN","address":"0xNNNNNN","current":{"pipeline_class":"non-authored","authored_order_role":"compiler-generated-icf-representative","physical_block_id":"recoil:block:0xNNNNNN","linked_address_group":null,"icf_address_group":null,"logical_aliases":null},"icf_address_group":{"winner_status":"selected-winner","winner_identity_key":"recoil:logical-function:0xNNNNNN:selected","evidence_ids":["recoil:evidence:rNNN:NNNNNN"]},"logical_aliases":{"recoil:logical-function:0xNNNNNN:selected":{"object_symbol":"?Selected@OwnerA@@QAEXXZ","original_name":"OwnerA::Selected","original_name_status":"recovered","source_owner_status":"authored-owner","owner_id":"recoil:owner:owner_a","pipeline_class":"authored","authored_order_role":"authored-body","fold_status":"selected-winner","evidence_ids":["recoil:evidence:rNNN:NNNNNN"]},"recoil:logical-function:0xNNNNNN:alias":{"object_symbol":"?Alias@OwnerB@@QAEXXZ","original_name":"OwnerB::Alias","original_name_status":"recovered","source_owner_status":"authored-owner","owner_id":"recoil:owner:owner_b","pipeline_class":"authored","authored_order_role":"authored-body","fold_status":"proven-fold-alias","evidence_ids":["recoil:evidence:rNNN:NNNNNN"]}}}' --expected-revision <revision> --dry-run --json
```

Review the dry-run and repeat the unchanged payload and revision with
`--apply`. The command changes only `icf_address_group` and `logical_aliases`.
For an exact reviewed v1-v4 object that exceeds the Windows command-line limit,
place the UTF-8 JSON file under workspace `build/` and use the mutually
exclusive file form:

```powershell
python tools/recoil.py progress symbol set-logical-alias-group --payload-file build/diagnostic/<recoil-logical-alias-group-v4.json> --expected-revision <revision> --dry-run --json
```

The file form is bounded to 16 MiB and feeds the same schema parser, semantic
guards, dry-run/apply behavior, revision CAS, authoritative wrapper, and README
synchronization path as `--payload-json`.
It requires exactly one selected winner and at least one proven fold alias;
address-qualified logical keys; unique decorated VC5 object symbols; recovered
authored-body identities; existing non-provider Recoil owners with accepted
source and owner-linkage gates; and existing tracker evidence ids. Unknown
owners/evidence, provider owners, stale snapshots, conflicting neutral address
groups, broken physical-block membership, no-op replacement, or revision drift
fail before mutation. Apply invalidates the affected block's order facts and
the physical row's order/byte facts, then the unified gate synchronizes the
generated README progress block. Workers never invoke this command.

One false function identity whose exact immutable-retail extent is NOP padding
is removed only through the parent-owned, dry-run-first padding correction
route. The `recoil-function-padding-correction-v1` payload contains complete
exact current function and semantic-span records, exact retained-block
scalar/count/membership guards, and `replacement_padding` with the identical
extent, retail bytes, retained block/span ids, `replacement_symbol_ids: []`,
and the function id to remove. The command reads only the registered
`support/Recoil.exe`, requires every byte to be `0x90`, and refuses accepted
order/byte state or owner, target, storage, work, and other relationships.
Apply retains the physical block and zero-symbol padding span while removing
the false symbol, one block membership, one span membership, and the two exact
schema-v4 migration references:

```powershell
python tools/recoil.py progress symbol replace-padding --payload-json '<recoil-function-padding-correction-v1-object>' --expected-revision <revision> --dry-run --json
```

An incorrect unresolved physical-block partition is corrected only through the
parent-owned, dry-run-first atomic replacement route. The reviewed v1 JSON
package must guard the exact current block fields and inventories, enumerate
contiguous replacement blocks, assign every selected symbol exactly once, and
replace every affected semantic span with complete contiguous coverage.
Replacement spans must exactly partition each current semantic span: they may
split only at a replacement-block seam and must preserve the current
`source_path`, status field, `confidence`, evidence, aliases, and other semantic
observations. A zero-symbol span is allowed only when it exactly preserves a
current reviewed `padding:` span with `status=padding`; padding spans cannot be
merged away, dropped, or invented. Every non-padding authored/provider span
must retain the exact address-derived symbol population. New
blocks must keep `original_source_path` and `provisional_original_path` null
and `mapping_state` unresolved; current compile-provider paths do not accept an
original filename. The first replacement keeps the old start-derived block id
so historical references remain resolvable. Review the dry-run JSON, then use
the identical payload and revision for apply:

```powershell
python tools/recoil.py progress block replace --payload-json '<recoil-physical-block-replace-v1-object>' --expected-revision <revision> --dry-run --json
```

When the exact reviewed object exceeds the Windows command-line limit, place
the UTF-8 JSON object under workspace `build/` and pass it through the mutually
exclusive file form. The file is bounded to 16 MiB and feeds the same v1
parser, guards, dry-run/apply behavior, revision CAS, authoritative
`tools/recoil.py` wrapper, and README synchronization path:

```powershell
python tools/recoil.py progress block replace --payload-file build/diagnostic/zui-block-split-r1272/recoil-physical-block-replace-v1.json --expected-revision <revision> --dry-run --json
```

After reviewing the dry-run, repeat the unchanged payload file and revision
with `--apply`.

A stale authored physical mapping may be reclassified only through the distinct
parent-owned, dry-run-first provider route. The exact
`recoil-provider-block-reclassify-v1` payload contains `reviewed=true`,
`parent_reviewed=true`, one complete `current_block` snapshot, canonical sorted
`expected_provider_owner_ids`,
`clear_provisional_compile_source_placement=true`, and a `replacement` object
with exactly `contribution_kind`, `source_path`, `agent_source_path`,
`provisional_original_path`, `mapping_status`, and `mapping_confidence`.
Replacement source labels must be the same `provider:` navigation label,
provisional original placement must be null, status must be
`provider-boundary`, and confidence must begin with `high `.

```powershell
python tools/recoil.py progress block reclassify-provider --payload-json '<recoil-provider-block-reclassify-v1-object>' --expected-revision <revision> --dry-run --json
```

The command rederives immutable interval and membership, requires every member
to be `non-authored` / `non-authored` and `primary-owned` by exactly one
accepted provider-boundary owner named in the payload, and rejects accepted
original-source provenance, configured order targets, active work conflicts,
stale snapshots, and revision drift. Apply changes only
`contribution_kind`, `source_path`, `agent_source_path`,
`provisional_original_path`, `mapping.status`, and `mapping.confidence`.
Existing mapping state/evidence, symbol navigation, complete semantic spans,
all authored/full order and byte state, owners, tiers, storage, sections, and
symbols remain unchanged; the complete derived scheduler must also remain
identical. Review the dry-run, then repeat the unchanged payload and revision
with `--apply`. This reclassification does not accept authored order.

A distinct parent-only route advances authored order across a reviewed batch of
physical blocks that contains no `authored-body` or
`authored-lifecycle-body` identity. The
`recoil-authored-non-gating-block-accept-v1` payload supplies the exact live
cursor, exact expected authored cursor-after, and a complete current tracker
record for every contiguous block. The command rederives exact contribution
membership and requires every row to be fully resolved, role/class compatible,
and non-gating. This includes classified compiler-generated lifecycle rows such
as `compiler-generated-eh-helper`; those rows remain inventoried for full
order. A nonzero block must be an explicit high-confidence provider boundary.
A zero-row block must be high-confidence provider-data or padding. Configured
order targets, active resource conflicts, stale snapshots, gaps, unresolved
rows, and any authored gating identity fail closed.

```powershell
python tools/recoil.py progress block accept-authored-non-gating --payload-json '<recoil-authored-non-gating-block-accept-v1-object>' --expected-revision <revision> --dry-run --json
```

For a large complete membership such as the 1,060-row compiler-generated EH
tail, pass the identical reviewed object with `--payload-file <reviewed-v1.json>`
instead of `--payload-json`; this avoids the Windows command-line limit without
replacing exact snapshots or memberships with a probabilistic summary.

Review the dry-run and repeat the unchanged payload/revision with `--apply`.
The command adds one live evidence record and changes only the five authored
block-order dimensions to current accepted `not-applicable`. It does not accept
full order or alter symbols, bytes, owners, provider targets, mapping/source
provenance, tiers, semantic spans, or storage. Post-derivation requires the
exact authored cursor-after; when the batch reaches the end of `.text`, the
ordinary phase transition to full order is allowed only while the full-order
cursor and counts remain unchanged.

Exact reviewed owner migrations use the parent-owned, dry-run-first atomic
replacement route. The v1 payload contains non-empty `current_owners` and
`replacement_owners` arrays; each row is `{ "id": "recoil:owner:...",
"record": { ... } }`. Current records are complete exact tracker snapshots.
Replacement records are the complete proposed owners. Schema v2 also requires
the three explicit arrays `primary_function_bootstraps`,
`primary_data_reassignments`, and `unknown_data_symbol_bootstraps`. These rows
guard each exact symbol/address/current-ownership tuple, constrain every target
to an explicit replacement owner, rehome every referenced storage contribution,
and permit an absent data symbol only as an unknown-extent record with no
invented size, end, or storage contribution. Schema v1 remains accepted for
owner replacements that reassign at least one primary function. A v2 batch may
instead be a data-only migration when its non-empty exact reviewed
`primary_data_reassignments` produce real primary-data membership changes;
v2 graph-only batches with no primary-function or primary-data change remain
fail-closed. The command requires
`reviewed=true`, `parent_reviewed=true`, a non-empty reason, one binary, and a
revision guard. It rejects stale or duplicate owners, no-ops, lost/extra or
duplicate primary memberships, inexact symbol/address/binary links, partial
retirement, dangling dependencies, unrelated changes to retained owners, and
stale moved-function tiers. V2 additionally rejects stale bootstrap states,
unreviewed currently-owned function claims, data loss, storage-owner mismatch,
unknown output sections, and arbitrary extra function or data memberships.
Retained owners preserve gates and every
non-primary relationship. A guarded retained owner may retarget an existing
`depends-on-owner` row only when its old target is retired by this same batch
and its new target is an explicit replacement owner; only `target_owner_id`
and `reason` may change, with no relationship additions or removals. Newly
created provider owners carry no authored tier entries. The canonical
source-owner invariants rerun before CAS commit.

```powershell
python tools/recoil.py progress owner replace-batch --payload-json '<recoil-owner-replace-batch-v2-object>' --expected-revision <revision> --dry-run --json
```

When the exact reviewed owner snapshots exceed the Windows command-line limit,
place the UTF-8 v1/v2 object under workspace `build/` and use the mutually
exclusive, 16 MiB-bounded file form. It feeds the same schema parser,
ownership/storage/no-loss guards, dry-run/apply behavior, revision CAS, and
README synchronization path:

```powershell
python tools/recoil.py progress owner replace-batch --payload-file build/diagnostic/<reviewed-owner-replace-v2.json> --expected-revision <revision> --dry-run --json
```

After reviewing the dry-run, repeat the unchanged payload file and revision
with `--apply`.

Use `progress owner repair-primary-data-tier-x` only to initialize absent
bookkeeping for exact, existing, uniquely same-owner authored primary-data
relationships. Its `recoil-owner-primary-data-tier-x-repair-v1` payload guards
the owner, symbol identity, address, current `primary-owned` state, and complete
current `primary-data` relationship. Apply creates only
`{"kind":"data","tier":"X","evidence_ids":[]}` entries. Existing entries,
function rows, provider owners, stale or ambiguous relationships, changed
membership, and any positive tier state fail closed. Review a dry-run before
repeating the unchanged payload and revision with `--apply`:

```powershell
python tools/recoil.py progress owner repair-primary-data-tier-x --payload-json '<recoil-owner-primary-data-tier-x-repair-v1-object>' --expected-revision <revision> --dry-run --json
```

Use `progress owner downgrade` when current live evidence disproves accepted
owner gates or primary-entry tiers but does not change ownership. Its
`recoil-owner-downgrade-v1` payload names one exact current authored owner and
contains paired `current_gates`/`new_gates` and
`current_entry_tiers`/`new_entry_tiers` objects. Selected gate keys and entry
ids must match exactly across each pair. Only accepted or not-applicable
(`none`) gates may move to `blocked`, `pending`, or `deferred`; entry tiers must
move strictly downward in `X < C < B < A < S`. Provider owners, non-primary
entries, stale current values, no-ops, and promotions fail closed. One atomic
apply creates current evidence on the owner and every affected entry, uses the
revision guard, and preserves all unrelated tracker facts.

```powershell
python tools/recoil.py progress owner downgrade --payload-json '<recoil-owner-downgrade-v1-object>' --expected-revision <revision> --dry-run --json
```

`progress provider-target register` accepts either an immutable-retail named
function import or a reviewed ordinal-function import. An import named `#N`
must include the exact integer `import_ordinal: N`; named imports must omit it.
The command compares that value directly with the retail PE thunk before
constructing the provider function, IAT data/storage contribution, and owner.

`progress provider-function register` is the separate path for an existing
exact known-extent non-authored function linked from a canonical VC5SP3 static
library. Its reviewed payload names one normalized library path below
`DEFAULT_VC5_ROOT`, one exact archive member, and one external function symbol.
The command parses the archive and COFF object in process, requires a defined
external code-COMDAT symbol whose natural extent equals the registered retail
extent, and compares immutable retail `.text` bytes while masking only
supported COFF relocation fields. Imports/IAT rows, authored or already-owned
rows, ambiguous or escaping paths/members/symbols, malformed archives, extent
or byte mismatches, owner/extent collisions, candidate-derived fields, and
revision drift fail closed. Dry-run creates no tracker mutation. Apply creates
only the accepted provider-boundary owner, exact anchor/primary-function
relationships, current owner-scoped evidence, and provider/object identity on
the existing function row.

The command rejects range gaps/overlaps, id collisions, stale old-block fields,
incomplete or duplicate symbol/span assignments, cross-block spans, accepted
original-source provenance, current relationship references it cannot safely
reassign, schedulable work tied to the old shape, and a broken derived
scheduler. It invalidates affected order/byte facts rather than carrying them
across a changed physical grouping. Workers never invoke this command.

Every successful authoritative mutation of the default tracker automatically
synchronizes the bounded generated progress block in the root `README.md`.
The tracker remains the sole authority. The public projection can also be
updated or checked explicitly without copying live values by hand:

```powershell
python tools/recoil.py docs readme-progress
python tools/recoil.py docs readme-progress --check --json
```

When an order block has an empty or stale target binding, use the
phase-specific command-scoped `--object-target` or `--linked-target` override.
The scheduler emits one only for a unique registered target with the exact
binary, range, phase, contiguous-block, and tracker-identity contract; otherwise
it reports a typed blocked cursor.

## Artifact cleanup

Normal artifact cleanup always treats the selected repository's direct
`build/` directory as one aggregate target. The default invocation is a dry
run; it reports the total local file size and direct-entry count for everything
beneath `build/`, without name, age, or file-type exclusions:

```powershell
python tools/recoil.py audit artifacts
python tools/recoil.py audit artifacts --delete
```

`--delete` removes every entry beneath `build/`, including loose files,
unknown directory names, fresh outputs, and formerly protected canonical build
children, then verifies that `build/` is still a real, empty directory. The
cleanup validates every selected target before mutation, rejects a build root
that is a file, symlink, junction, other reparse point, or resolved escape, and
never follows nested symlinks or junctions. An unknown nested reparse type
fails the complete preflight before anything is deleted.

`.vs` and `playground` remain excluded unless explicitly opted in with
`--include-vs` or `--include-playground`. `--older-than-days` and `--all` apply
only to those opted-in local roots; they never limit `build/` cleanup.
`--session-only` retains its separate governed `.devspace` semantics and cannot
be combined with normal cleanup selection flags.

## Health checks

Use focused checks in proportion to the change:

```powershell
python tools/recoil.py doctor --infrastructure-only
python tools/recoil.py audit agent-surface --strict
python tools/recoil.py audit workflow-contracts --strict
python tools/recoil.py audit pipeline-reachability --strict
python tools/recoil.py audit live-validation-surface --strict
python -m unittest discover -s tests/tools -p "recoil_*_tests.py"
```

`agent-surface` is a static syntax/reference audit. It also proves the `.claude`
pointer mirror still matches the canonical `.codex` surface: one stub per
canonical skill and role, no orphans, verbatim descriptions, a live canonical
path in every stub, and the `CLAUDE.md` import plus mechanical ledger and retail
deny rules. `workflow-contracts` and
`pipeline-reachability` operationally check command transitions, real
reservation-backed handoffs, target-to-slice coverage, and required expected-
fact producers. A static pass alone is not complete workflow health.

`audit live-validation-surface` rejects active validation mechanisms that bind
acceptance to candidate-file identity, observation artifacts, build reuse,
linker-clock qualification, or retired command routes. Reconstructed game
behavior that happens to use lookup or integrity algorithms is outside this
validation-policy check.
 
Other tracker and report views are deferred context and never peer schedulers.

## Complete Command Index

This index is generated from the command registry. Commands whose purpose
explicitly identifies them as retired are listed for discoverability, but are
not ordinary reconstruction workflow steps.

| Command | Category | Mutates | BN | Purpose |
| --- | --- | --- | --- | --- |
| `python tools/recoil.py audit agent-surface` | audit | no | no | Audit agent-facing tool, doc, skill, and role alignment. |
| `python tools/recoil.py audit artifacts` | audit | yes | no | Dry-run or empty the complete direct build directory while retaining build/, with explicit local-root and session-scratch modes. |
| `python tools/recoil.py audit bn-data-evidence` | audit | no | yes | Collect read-only Binary Ninja evidence for a data range. |
| `python tools/recoil.py audit call-contract-readiness` | audit | no | no | Preflight exact dependency closure for original authored call-contract slices. |
| `python tools/recoil.py audit current-metadata` | audit | no | no | Reject stale static current-cursor narratives and validate generated current metadata against progress next. |
| `python tools/recoil.py audit docblocks` | audit | no | no | Audit reconstruction docblocks and source-comment hygiene. |
| `python tools/recoil.py audit final-data` | audit | no | yes | Diagnose final-build .data section size and variable drift. |
| `python tools/recoil.py audit final-image-catalog` | audit | no | no | Derive and audit complete typed final-image coverage live from retail plus accepted tracker facts, without building a candidate. |
| `python tools/recoil.py audit live-validation-surface` | audit | no | no | Reject retired integrity, receipt, snapshot, clock, and content-derived identity mechanisms across tracked and ignored authored workspace surfaces. |
| `python tools/recoil.py audit pipeline-reachability` | audit | no | no | Prove every fail-closed live pipeline consumer has a reachable candidate-independent expected-fact producer. |
| `python tools/recoil.py audit provenance` | audit | no | no | Audit compiler/linker provenance assumptions. |
| `python tools/recoil.py audit provider-closure` | audit | no | yes | Audit provider/compiler-generated dependency closure. |
| `python tools/recoil.py audit relocation-expectations` | audit | no | no | Derive candidate-independent authored relocation expectations live from retail and accepted typed identities. |
| `python tools/recoil.py audit source-fragments` | audit | no | no | Inventory temporary production source fragments, quoted source includes, .inl files, and compatibility final-build exclusions. |
| `python tools/recoil.py audit source-trace` | audit | no | no | Audit attached source-to-retail artifact topology without changing acceptance state. |
| `python tools/recoil.py audit state-performance` | audit | no | no | Measure fresh-process SQLite ledger reads and bounded scratch-copy transactions against governed performance ceilings. |
| `python tools/recoil.py audit workflow-contracts` | audit | no | no | Exercise compact reservation, handoff, mutation-boundary, and single-validator live acceptance contracts. |
| `python tools/recoil.py audit workspace` | audit | no | no | Detect generated artifacts outside approved output roots. |
| `python tools/recoil.py audit zinterp` | audit | no | no | Audit zInterp dispatch literals against optional BN text dumps. |
| `python tools/recoil.py binja data-overlap` | binja | no | yes | Report overlapping Binary Ninja data-variable roots. |
| `python tools/recoil.py binja preflight` | binja | no | yes | Check Binary Ninja bridge/database availability. |
| `python tools/recoil.py build msvc-x86` | build | yes | no | Run an arbitrary command through the x86 MSVC environment wrapper. |
| `python tools/recoil.py build resource` | build | yes | no | Extract or compare source-style resources. |
| `python tools/recoil.py docs readme-progress` | docs | yes | no | Update or check the deterministic public README progress snapshot derived from the unified tracker. |
| `python tools/recoil.py doctor` | validation | no | yes | Run categorized full or infrastructure-only process-health checks. |
| `python tools/recoil.py env` | validation | no | no | Check local native build environment. |
| `python tools/recoil.py guard modern-cpp` | guard | no | no | Reject post-VC5 C++ constructs and forbidden call-convention helpers. |
| `python tools/recoil.py guard multiline` | guard | no | no | Check multiline declaration/call style. |
| `python tools/recoil.py guard original-symbol` | guard | no | no | Audit unsupported reconstruction helper dependencies. |
| `python tools/recoil.py guard provider` | guard | no | no | Reject fake provider internals and provider ABI shims. |
| `python tools/recoil.py guard raw-assembly` | guard | no | no | Reject unallowlisted or undocumented raw assembly and naked stubs. |
| `python tools/recoil.py guard raw-image` | guard | no | no | Reject raw original-image addresses. |
| `python tools/recoil.py guard raw-offset` | guard | no | no | Reject raw authored runtime-state offset access. |
| `python tools/recoil.py guard reinterpret-cast` | guard | no | no | Reject named reinterpret_cast usage. |
| `python tools/recoil.py guard source-data` | guard | no | no | Validate source data initializer rules recorded in unified progress. |
| `python tools/recoil.py guard source-fragments` | guard | no | no | Reject temporary production source fragments, quoted source includes, .inl files, and compatibility final-build exclusions. |
| `python tools/recoil.py guard source-goto` | guard | no | no | Reject source-level goto outside the exact reviewed migration baseline. |
| `python tools/recoil.py guard source-placement` | guard | no | no | Check source placement and provenance conventions. |
| `python tools/recoil.py guard source-shape` | guard | no | no | Reject source-shape scaffolds in production source. |
| `python tools/recoil.py guard vc5-manifest` | guard | no | no | Reject VC manifest-local source and generated header shadows. |
| `python tools/recoil.py issue audit` | issue | no | no | Validate the agent tooling/process issue ledger shape. |
| `python tools/recoil.py issue compact` | issue | yes | no | Parent-only no-archive active-only workspace-issue ledger compaction. |
| `python tools/recoil.py issue list` | issue | no | no | List open agent tooling/process issue reports. |
| `python tools/recoil.py issue reopen` | issue | yes | no | Reopen an agent tooling/process issue. |
| `python tools/recoil.py issue report` | issue | yes | no | Record an agent tooling/process problem for a future agent to fix. |
| `python tools/recoil.py issue request` | issue | yes | no | Record an agent tooling/process improvement request. |
| `python tools/recoil.py issue resolve` | issue | yes | no | Mark an agent tooling/process issue resolved. |
| `python tools/recoil.py issue show` | issue | no | no | Show one agent tooling/process issue report. |
| `python tools/recoil.py issue wont-fix` | issue | yes | no | Close an agent tooling/process issue without resolution and remove its terminal active-only rows. |
| `python tools/recoil.py issue work close` | issue | yes | no | Release and close one active workspace-issue packet. |
| `python tools/recoil.py issue work list` | issue | no | no | List explicit workspace-issue work packets. |
| `python tools/recoil.py issue work reserve` | issue | yes | no | Reserve one ready workspace-issue packet after global conflict checking. |
| `python tools/recoil.py issue work set` | issue | yes | no | Create one revision-guarded workspace-issue work packet. |
| `python tools/recoil.py issue work show` | issue | no | no | Show one workspace-issue packet and reservation history. |
| `python tools/recoil.py maintenance migrate-ledgers-sqlite` | validation | yes | no | Parent-only guarded direct cutover of both legacy JSON authorities to a matched SQLite database pair. |
| `python tools/recoil.py msvc eh-dump` | diagnostic | no | no | Decode MSVC EH metadata from the reference image. |
| `python tools/recoil.py policy show` | docs | no | no | Show one machine-readable reconstruction scheduling policy. |
| `python tools/recoil.py progress advance-live-byte` | progress | yes | no | Freshly compile and directly compare one byte lane, accepting only explicitly matched tracker physical groups. |
| `python tools/recoil.py progress advance-live-call-contract` | progress | yes | yes | Contained parent route that performs one fresh build and direct retail comparison, then CAS-accepts only bodies that passed in that invocation. |
| `python tools/recoil.py progress advance-live-order` | progress | yes | no | Freshly validate one registered order target, derive its complete contiguous block slices, and revision-atomically accept all slices only on exact PASS; full order keeps linked acceptance separate from its paired object worker target. |
| `python tools/recoil.py progress audit` | progress | no | no | Audit unified tracker schema, relationships, evidence, and derived pipeline invariants. |
| `python tools/recoil.py progress block accept-authored-non-gating` | progress | yes | no | Parent-only atomic acceptance of exact live-cursor physical blocks with zero authored gating identities. |
| `python tools/recoil.py progress block reclassify-provider` | progress | yes | no | Parent-only exact-snapshot reclassification of one stale authored physical block to a provider boundary. |
| `python tools/recoil.py progress block replace` | progress | yes | no | Parent-only atomic replacement of one reviewed physical block and all affected symbol/semantic assignments. |
| `python tools/recoil.py progress block show` | progress | no | no | Show one physical source block. |
| `python tools/recoil.py progress call-contract initialize` | progress | yes | no | Parent-only one-time initialization of the accepted-authored-order-derived call-contract census while preserving all order and byte facts. |
| `python tools/recoil.py progress call-contract prepare-live-convergence` | progress | yes | yes | Contained parent-only fresh no-reuse zero-divergence closeout; requires an active packet and is the only call-contract route that may authorize phase transition. |
| `python tools/recoil.py progress call-contract prepare-repair-continuation` | progress | yes | yes | Contained-disabled before ledger, output-root, evaluator, compiler, or BN work until a separately approved active-packet producer exists. |
| `python tools/recoil.py progress compact` | progress | yes | no | Parent-only no-archive active-only schema-v5 tracker compaction with exact scheduler parity. |
| `python tools/recoil.py progress current-metadata refresh` | progress | yes | no | Revision-guard regeneration of live scheduler metadata and historicalize audited stale cursor narratives. |
| `python tools/recoil.py progress data-artifact evidence repair-observation` | progress | yes | no | Parent-only revision-guarded repair of the known invalid freshness/validation-mode pair on one reviewed non-gating data-artifact observation. |
| `python tools/recoil.py progress data-artifact logical-alias register-batch` | progress | yes | no | Parent-only revision-guarded registration of reviewed authored logical-data occurrences under one provider/compiler-pooled physical representative. |
| `python tools/recoil.py progress data-artifact register` | progress | yes | no | Parent-only revision-guarded registration of one exact physical data identity and extent; creates no source edge, owner link, storage contribution, or acceptance. |
| `python tools/recoil.py progress data-extent register` | progress | yes | no | Parent-only revision-guarded exact extent registration for one existing physical data artifact; creates no artifact, source edge, or acceptance. |
| `python tools/recoil.py progress find` | progress | no | no | Search all unified reconstruction progress entities. |
| `python tools/recoil.py progress handoff` | progress | no | no | Render one compact worker packet from a real active reservation; fail closed when no matching reservation exists. |
| `python tools/recoil.py progress next` | progress | no | no | Select the sole authoritative next Recoil.exe reconstruction task from the unified progress tracker. |
| `python tools/recoil.py progress output-section show` | progress | no | no | Show one normalized PE output section. |
| `python tools/recoil.py progress owner audit` | progress | no | no | Audit unified source-owner invariants. |
| `python tools/recoil.py progress owner downgrade` | progress | yes | no | Parent-only atomic conservative downgrade of selected gates and primary-entry tiers on one exact current authored owner. |
| `python tools/recoil.py progress owner find` | progress | no | no | Search unified source owners. |
| `python tools/recoil.py progress owner relationships` | progress | no | no | Show normalized unified owner relationships. |
| `python tools/recoil.py progress owner repair-primary-data-tier-x` | progress | yes | no | Parent-only conservative initialization of absent tier-X records for exact existing same-owner authored primary data. |
| `python tools/recoil.py progress owner replace-batch` | progress | yes | no | Parent-only atomic replacement of exact reviewed source-owner snapshots and guarded primary-function/data memberships. |
| `python tools/recoil.py progress owner show` | progress | no | no | Show one unified source owner or address-linked owner set. |
| `python tools/recoil.py progress provider-function register` | progress | yes | no | Register one existing exact non-authored retail function as a canonical VC5 static-library provider function. |
| `python tools/recoil.py progress provider-function register-atlimpl-cluster` | progress | yes | no | Atomically replace the exact reviewed legacy zCom owner with the fixed three-body canonical VC5SP3 ATLIMPL provider cluster. |
| `python tools/recoil.py progress provider-target register` | progress | yes | no | Register one retail-proven named or reviewed ordinal-function IAT slot as an accepted typed provider target. |
| `python tools/recoil.py progress relocation-exception set` | progress | yes | no | Revision-guard one reviewed retail-relocation ambiguity exception against exact current source and target context. |
| `python tools/recoil.py progress relocation-target bind` | progress | yes | no | Bind one immutable-retail relocation operand to reviewed existing or exact known-extent target identity. |
| `python tools/recoil.py progress report` | progress | no | no | Render an on-demand unified progress report without creating a shadow tracker. |
| `python tools/recoil.py progress semantic show` | progress | no | no | Show one semantic span. |
| `python tools/recoil.py progress show` | progress | no | no | Show a joined owner/block/semantic/order/link/byte view. |
| `python tools/recoil.py progress source-trace replace-batch` | progress | yes | no | Parent-only revision-guarded replacement of reviewed source-trace topology rows and append-only resolution of immutable legacy claims. |
| `python tools/recoil.py progress source-trace show` | progress | no | no | Show read-only physical/logical source-trace topology rows. |
| `python tools/recoil.py progress status` | progress | no | no | Show derived unified pipeline or selector status. |
| `python tools/recoil.py progress storage register-authored-data` | progress | yes | no | Parent-only revision-guarded registration of one exact non-overlapping authored data-symbol storage contribution. |
| `python tools/recoil.py progress storage show` | progress | no | no | Show one normalized physical storage contribution. |
| `python tools/recoil.py progress symbol replace-padding` | progress | yes | no | Parent-only removal of one exact false function identity after immutable-retail NOP-padding verification. |
| `python tools/recoil.py progress symbol set-logical-alias-group` | progress | yes | no | Parent-only revision-guard one reviewed physical ICF row and its authored logical aliases. |
| `python tools/recoil.py progress symbol set-pipeline-class-batch` | progress | yes | no | Revision-guard a reviewed batch of exact function-row pipeline classifications. |
| `python tools/recoil.py progress verification-target retire` | progress | yes | no | Dry-run-first parent route that retires exactly one stale verification-target registration by exact id or unique name. |
| `python tools/recoil.py progress verification-target sync` | progress | yes | no | Synchronize selected verification-target registrations, with a fail-closed parent-only source-policy bootstrap for one reviewed order target. |
| `python tools/recoil.py progress work claim-current` | progress | yes | no | Atomically create and reserve compatible current packets through prioritized multi-lane or focused individual-lane claims. |
| `python tools/recoil.py progress work close` | progress | yes | no | Close one structured work item. |
| `python tools/recoil.py progress work create-explicit` | progress | yes | no | Parent-only journal-first output-root allocation followed by one final atomic activation of an exact explicitly user-selected maintenance or read-only diagnostic packet. |
| `python tools/recoil.py progress work leases` | progress | no | no | Show global reconstruction and workspace-issue leases, or conflicts for one packet. |
| `python tools/recoil.py progress work recover-allocation` | progress | yes | no | Authenticate and recover one journal-owned failed explicit output allocation without acceptance. |
| `python tools/recoil.py progress work recover-expired` | progress | yes | no | Release one expired explicit maintenance reservation back to ready state without acceptance. |
| `python tools/recoil.py progress work reserve` | progress | yes | no | Reserve one scheduler-launchable or exact retry-eligible returned packet with non-expiring normalized resource claims. |
| `python tools/recoil.py progress work return` | progress | yes | no | Return one active explicit maintenance packet with bounded nonaccepting feedback. |
| `python tools/recoil.py progress work return-binja` | progress | yes | yes | Parent-only governed Binary Ninja read-plan execution and scheduler-CAS return for one active BN-enabled explicit packet. |
| `python tools/recoil.py progress work show` | progress | no | no | Show structured work items. |
| `python tools/recoil.py style fix-multiline` | style | yes | no | Rewrite strict multiline style issues. |
| `python tools/recoil.py verify asm` | verification | no | yes | Extract or compare Binary Ninja assembly/bytes. |
| `python tools/recoil.py verify authored-byte` | verification | no | no | Freshly rebuild and directly scan authored object, relocation, target, and linked-body semantics. |
| `python tools/recoil.py verify authored-object-byte` | verification | no | no | Freshly compile and directly scan authored object bodies outside relocation fields. |
| `python tools/recoil.py verify authored-order scaffold` | verification | yes | no | Draft or explicitly write a fail-closed current-block authored-order VC5 manifest candidate. |
| `python tools/recoil.py verify authored-order sweep` | verification | no | no | Read-only mechanical scaffold-readiness sweep across remaining authored-order blocks. |
| `python tools/recoil.py verify call-contract` | verification | no | yes | Freshly compile one deterministic authored-body slice, or one nonaccepting registered-target convergence scope, and compare exact static invocation contracts with retail Binary Ninja evidence. |
| `python tools/recoil.py verify final-build` | verification | yes | no | Run the final VC5SP3 executable/DLL build pipeline. |
| `python tools/recoil.py verify final-image` | verification | yes | no | Freshly build and validate complete typed PE semantics against retail; raw file differences and COFF time are diagnostic only. |
| `python tools/recoil.py verify functional` | verification | no | no | List or run tier C functional smoke evidence. |
| `python tools/recoil.py verify functional-batch` | verification | no | no | Run multiple tier C functional smoke targets. |
| `python tools/recoil.py verify linked-byte` | verification | no | no | Freshly rebuild and directly scan the linked-byte lane, stopping at the earliest real divergence. |
| `python tools/recoil.py verify linked-order` | verification | yes | no | Compile/link current VC5SP3 code and report one authored/full linked-order divergence. |
| `python tools/recoil.py verify pe` | verification | no | no | Verify or compare PE reference executable/DLL facts. |
| `python tools/recoil.py verify vc5` | verification | no | yes | List or run owner-scoped VC5SP3 COFF function/data-symbol verification. |
| `python tools/recoil.py verify vc5-abi-equivalence` | verification | no | no | Prove one manifest-owned zero-argument identity is mechanically equivalent under fresh VC5 /Gd and /Gr builds. |
| `python tools/recoil.py verify vc5-order` | verification | no | no | Compile one VC5SP3 order target and report the first retail-order divergence directly. |
| `python tools/recoil.py workspace worktree create` | issue | yes | no | Parent-only creation and reservation of one linked workspace-issue packet worktree. |
| `python tools/recoil.py workspace worktree hygiene` | issue | no | no | Audit branch, linked-worktree, association, and external-build-root hygiene. |
| `python tools/recoil.py workspace worktree integrate` | issue | yes | no | Parent-only validated temporary-worktree integration of one packet branch into master. |
| `python tools/recoil.py workspace worktree retire` | issue | yes | no | Parent-only removal of one integrated packet worktree, branch, and authenticated build root. |
| `python tools/recoil.py workspace worktree status` | issue | no | no | Inspect canonical and linked Git worktrees, packet associations, build roots, and branch hygiene. |
| `python tools/recoil.py workspace worktree validate` | issue | no | no | Validate one linked workspace-issue packet commit and exact authored closure. |
