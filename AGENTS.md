# AGENTS.md

Mandatory workflow for AI- or human-assisted Recoil reconstruction.

## 0. Mission And Authority

Reconstruct a source-faithful native C/C++ codebase for the 1999 Windows x86
*Recoil* engine. An unrestricted VC5SP3 build must reproduce every catalogued
typed semantic fact in `support/Recoil.exe`.

Acceptance is live and semantic. The linker-written COFF timestamp and the raw
whole-file difference are diagnostic only. No source, object, executable,
receipt, snapshot, or artifact hash qualifies a candidate. Every acceptance
operation rebuilds current production source and compares the resulting typed
code, data, targets, providers, layout, resources, directories, padding, and PE
facts directly with retail.

Repository-local authority, subject to system/developer/host constraints:

1. the explicit user task;
2. this file;
3. `.agent/RECONSTRUCTION_PROGRESS.sqlite3` through `tools/recoil.py`;
4. the canonical skill or runbook that owns a specialized procedure.

Never hand-edit `.agent/RECONSTRUCTION_PROGRESS.sqlite3` or
`.agent/WORKSPACE_ISSUES.sqlite3`. They are independent SQLite authorities
with independent monotonic revisions: progress mutations guard the progress
revision, and workspace-issue mutations guard the issue revision. Tracker ids
and revision numbers identify ledger rows and concurrency state; they are never
candidate content identities. Runtime, backup, restore, and rollback are
SQLite-only after cutover; there is no JSON backend, mirror, or export.

Primary evidence:

- the already-open `Recoil.bndb`, a maintained analysis artifact, for current behavior, ABI, layout, globals,
  xrefs, imports, tables, function boundaries/order, and assembly;
- `support/Recoil.exe` as the immutable retail executable;
- the already-open `messages.bndb` and `support/messages.dll` only for an
  explicitly selected companion-DLL task;
- current source and fresh governed VC5SP3 output for candidate behavior.

BN function names and comments are provisional navigation labels. Source
comments, old notes, current filenames, smokes, and historical observations
are navigation aids. Current assembly/xrefs/literals,
direct retail comparison, and governed compiler/linker output outrank them.

The canonical executable-reproduction runbook is
`docs/reconstruction/retail_executable_reproduction.md`.

## 1. One Scheduler, Six Live Stages

With no explicit target, `python tools/recoil.py progress next` is the only
Recoil.exe scheduler. Owner, work-item, functional, final-data, final-image,
`messages.dll`, and documentation views are never peer queues.

The scheduler has these stages:

1. **`authored-function-order`** — traverse retail `.text` from `0x401000`.
   Every selected row is `authored`, `authored-lifecycle`, `non-authored`, or
   `unresolved`. Only `authored-body` and `authored-lifecycle-body` roles gate
   this phase. Each expected gating identity must resolve exactly once in natural
   retail relative order. Compiler-generated lifecycle variants stay
   inventoried for full order but do not gate authored order or authored bytes.
2. **`authored-call-contract`** — verify the current source's static invocation
   contracts for the live reviewed census derived from accepted authored-order
   classification of unique physical authored and authored-lifecycle gating
   bodies. The 3,380-body value was the reviewed
   one-time migration census; the permanent stage derives its current gating
   count from current reviewed classification. Traverse deterministic
   retail-contiguous slices capped at 160 bodies. Candidate-independent retail
   Binary Ninja evidence supplies expected
   call count/order, call versus tail-call form, direct versus indirect dispatch,
   authored/self/provider/IAT identity, virtual/interface slot, callback storage,
   and known caller cleanup. Unresolved target, storage, import, provider, ICF or
   logical-alias identity, or indirect provenance fails closed. Acceptance
   changes only the `call_contract` dimension and its evidence.
3. **`authored-byte-match`** — an independent retail-monotonic lane beginning
   at the first authored gating group. Require exact object body bytes outside
   relocation fields, exact relocation type/target/addend semantics, linked
   presence and symbolic identity, and relocation-normalized linked body bytes.
   Candidate RVA and resolved operands are diagnostic here. Compatible authored
   and authored-object byte work remains available while call-contract slices
   are pending; a physical-block resource conflict blocks only the conflicting
   packet.
4. **`full-function-order`** — restart at `0x401000` immediately after every
   authored call contract is current; do not wait for authored bytes. Require
   the exact selected linked address groups, identities, RVAs, order, providers,
   and padding; the seams remain exact.
5. **`linked-byte-match`** — begin only after both full order and authored bytes
   complete. Require exact linked RVA, resolved operands, targets, and raw
   linked-image bytes for every selected row.
6. **`final-validation`** — require complete live typed coverage of headers,
   sections, functions, variables, providers, resources, directories, padding,
   zero-fill, relocations, and overlay. Only the linker COFF timestamp and raw
   whole-file delta are diagnostic.

An unresolved row or unclassified selected extra blocks its applicable lane.
One lane never accepts or revokes another lane, owner, model, provider, tier,
storage contribution, output section, or final-image fact.

At raw object/TU order gates, inventory unlisted raw definitions but treat raw
extras as a mechanically non-blocking diagnostic: `/OPT:REF` and `/OPT:ICF` may discard or fold them. Full
linked order still requires the exact selected population and seams.

An unresolved row inside a registered order target's covered interval blocks
creation and acceptance of the complete physical-block packet. A raw-object
diagnostic over only the already resolved subset may still be useful, but its
PASS cannot make that partial subset launchable or acceptable.

## 2. Coding-First Live Loops

### Order source loop

The normal order worker receives a compact `order-edit-v1` packet containing a
registered target, exact writable source/header paths, one validation command,
an objective, and a stop condition. Its loop is only:

```powershell
python tools/recoil.py verify vc5-order <target> --build-root <packet-root>
```

Edit the assigned source/header closure, compile, inspect the first divergence
and neighbors, and repeat until PASS or a concrete out-of-scope source/block/
header contradiction. This loop requires no Binary Ninja session, byte work,
tracker mutation, artifact import, evidence packaging, hash qualification, or
ChatGPT Pro call. A worker PASS is feedback only.

Order validation scope, physical acceptance scope, source-owner scope, and
write/concurrency scope are distinct:

- an order target may cover one translation unit and several explicit,
  contiguous physical block slices;
- exact target PASS may accept those complete covered slices;
- order work does not accept an owner, tier, byte dimension, or source model;
- byte repair and owner/tier acceptance remain complete-owner operations.

### Call-contract source loop

The normal call-contract worker receives a compact `call-contract-edit-v1`
packet containing one deterministic slice, exact writable source/header paths,
one non-mutating validation command, an objective, and a stop condition. Its
loop is only:

```powershell
python tools/recoil.py verify call-contract --slice <slice-id> --progress .agent/RECONSTRUCTION_PROGRESS.sqlite3 --build-root <packet-root> --json
```

Edit only the assigned source/header closure and repeat until PASS or a
concrete out-of-scope source/block/header contradiction. A worker PASS is
feedback only; it accepts no call-contract, order, byte, owner, provider, gate,
tier, storage, or final-image state. The parent independently runs the fresh
`advance-live-call-contract` acceptance command below.

### Parent live acceptance

The parent independently rebuilds current source and accepts from the same
in-memory semantic result through:

```powershell
python tools/recoil.py progress advance-live-order --target <tracker-target-id> --build-root <fresh-root> --expected-revision <revision> --apply --json
python tools/recoil.py progress advance-live-call-contract --slice <slice-id> --build-root <fresh-root> --expected-revision <revision> --apply --json
python tools/recoil.py progress advance-live-byte --lane <object|authored|linked> --build-root <fresh-root> --expected-revision <revision> --apply --json
```

For these three self-validating live commands, direct `--apply` is the normal
route. Optional `--dry-run` is diagnostic only; do not routinely rebuild once
for dry-run and again for apply. The command must validate current source,
derive the exact safe blocks/groups from that invocation, and CAS-mutate the
tracker against the supplied revision. Revision drift, source/build conflict,
build failure, or semantic divergence produces no unsafe mutation.

Manual semantic mutations—owner, block, provider, classification, catalog
exception, tier, and similar reviewed changes—remain dry-run-first and are
repeated with `--apply` only after review.
Owner topology or membership replacement uses `progress owner replace-batch`;
conservative gate/tier decreases use `progress owner downgrade`; verification
target registration metadata uses `progress verification-target sync`.
Unsupported positive owner metadata, gate, or tier mutations are workspace
tooling requests, not ad hoc tracker edits.

Workers never run parent acceptance commands. The parent never accepts a
worker build, saved candidate, or prior dry-run result.

Call-contract acceptance stores no body proof that can substitute for fresh
verification. Currency is maintained by governed source/tool/manifest mutation
and conservative explicit invalidation. The reviewed schema coordinates are
`CALL_CONTRACT_VERIFIER_GENERATION = 10`,
`NORMALIZER_REGISTRY_GENERATION = 10`, and
`EXPECTED_FACT_SCHEMA_VERSION = 10`; a governed component edit increments its
coordinate and invalidates all affected evidence. Phase transition still
requires one fresh complete no-reuse zero-divergence scan.

### Byte feedback

Object validation proves only object body equality outside relocation fields.
Authored validation additionally requires live retail-derived relocation
expectations, exact candidate relocation rows/addends/target identity, linked
presence, and normalized linked body bytes. Linked validation additionally
requires exact RVA, resolved operands, targets, and raw linked bytes.

Expected relocation facts are derived from immutable retail and accepted typed
identity/provider/alias facts, never from the candidate. Deterministic rows and
explicit empty sets need no manually copied catalog. Genuine ambiguity blocks
before an expensive build and is resolved only through a narrow reviewed
exception command:

```powershell
python tools/recoil.py progress relocation-exception set --source-symbol-id <physical-symbol-id> --source-address 0xNNNNNN --payload-json '<json-object>' --expected-revision <revision> --dry-run --json
```

Review the dry-run, then repeat with `--apply`. This is a manual semantic
mutation, not part of the normal compile/edit loop.

Missing expected target identity is not a relocation ambiguity. When immutable
retail gives the operand but the tracker lacks its reviewed existing or exact
known-extent target identity, use the separate dry-run-first route:

```powershell
python tools/recoil.py progress relocation-target bind --source-symbol-id <physical-symbol-id> --source-address 0xNNNNNN --payload-json '<reviewed-binding>' --expected-revision <revision> --dry-run --json
```

Review and repeat with `--apply`. `relocation-target bind` registers the typed
target identity needed by live expectation derivation; `relocation-exception
set` records only a genuine, reviewed ambiguity. Neither command uses candidate
output as expected truth.

## 3. Work Packets And Concurrency

The parent owns scheduling, integration, acceptance, tracker mutations,
workspace issues, and final claims. Use subagents by default for bounded source
implementation and nontrivial tool maintenance.

Before concurrent launch, inspect:

```powershell
python tools/recoil.py progress work leases --json
```

With no explicit target, including a bare `Start` request, the root parent acts
autonomously. It runs `progress next --json` and the lease check, computes the
remaining child slots from the effective runtime-reported capacity, and—without
waiting for another user confirmation—claims every compatible current lane in
one revision-atomic operation:

```powershell
python tools/recoil.py progress work claim-current --lane all --max-packets <available-child-slots> --expected-revision <revision> --apply --json
```

The fixed claim priority is primary order, full authored byte, then subordinate
authored-object byte; here the primary packet category means the current
primary lane, order or call-contract. A blocked primary does not suppress
compatible byte work;
full authored byte wins over a newly overlapping object packet. Actual resource
conflicts and capacity skips are tool-owned. For every returned packet, render
the exact reservation and launch the named compatible worker:

```powershell
python tools/recoil.py progress handoff --packet-id <packet-id> --json
```

Individual `--lane <primary|authored|object>` claims remain available for a
focused retry or explicit assignment. Parent live acceptance remains a fresh
`advance-live-*` invocation after worker return.

The parent claims a current task atomically through the registered work command
and gives the worker only the compact mode-specific packet. The tool derives
normalized resource claims and the active lease behind `packet_id`; agents do
not repeat the lease listing or conflict matrix in prose.

Compatibility remains symmetric: read/read may overlap; read/write and
write/write may not. Shared headers, manifests, provider inputs, generated
outputs, canonical build windows, live BN databases, the Pro browser profile,
and ledgers conflict regardless of different owner/block ids.

Git is the authored workspace change-control mechanism. A governed packet starts
from a clean reviewed branch whose HEAD is the workspace baseline or a
descendant. Reservation stores only that opaque commit, the branch, packet id,
and exact writable closure. Closeout uses porcelain-v2 status and
commit-relative name-status/diff; every modified, added, deleted, type-changed,
unmerged, or rename endpoint must be writable. An ordinary copy whose source is
unchanged writes only its destination. Git never supplies retail expected truth
or binary acceptance, and Git object ids remain opaque implementation details.

An allocated output root is owned by its Windows volume identity and stable
file ID/index together with packet id, allocation operation id, tracker
identity, canonical path, and exact marker fields. Reopen it reparse-safely and
require the same physical identity before handoff, compiler or BN entry,
worker return, or deletion. A same-path replacement fails even if marker text
was copied.

`progress handoff --packet-id <packet-id> --json` renders only a real active
reservation. It must fail for no reservation, absent lease, empty write claims,
or a mutating worker command. It never fabricates a work item or exposes a
parent `--apply` command as worker validation.

For source work, the worker edits only packet paths and returns packet id,
outcome, changed paths, exact validation result, first divergence, and any
concrete scope contradiction. Claims, arbitration, and acceptance remain
machine/parent responsibilities.

Treat each live Binary Ninja database as one shared reader/writer resource.
Stable saved-view readers may overlap only when no writer lease exists. Exactly
one parent-assigned `recoil_bn_reconstructor` may hold the writer lease through
reanalysis, propagation checks, save, and return.

The parent/tool orchestrator owns packet branch creation, linked-worktree and
external-build-root allocation, exact-worktree handoff/closeout, integration,
retirement, and strict branch hygiene. Each active workspace-issue packet has
one `packet/` branch, one linked worktree, one physically authenticated sibling
build root, and one central reservation. The worker edits only packet paths,
may stage only the exact handed-off writable closure, and may create exactly one
nonaccepting packet commit whose message contains the packet id. The worker may
not create, switch, merge, rebase, integrate, or delete branches/worktrees, may
not modify `master`, and may not delete its build root.

The parent validates a packet commit, merges it first in a temporary integration
worktree, completes every fallible compiler, test, audit, and doctor check there,
and only then fast-forwards canonical `master`. After the fast-forward, perform
only deterministic Git, topology, tag, and physical-identity assertions; do not
run another fallible semantic validation command. After terminal closeout, retire
the packet worktree, merged packet branch, and authenticated build root; inactive
packet branches are not retained as history. `workspace worktree hygiene
--strict` rejects stale lifecycle state. Absolute checkout paths and external
build-root prefixes are diagnostic provenance, not retail expected truth or
acceptance identities. The progress reconstruction-packet worktree adapter is
`contained-disabled`: progress packets do not yet record a native-Git baseline.
Linked validation executes tracked source, tools, tests, policies, target
manifests, and `.agent/REFERENCE_EXECUTABLE.json` from the executing worktree.
It resolves machine-local `support/Recoil.exe` and the live progress/issue
SQLite databases from the validated canonical control root; it never copies or
links those inputs into the linked checkout.
Commit existence is never reconstruction acceptance. Destructive primary-worktree Git
operations remain prohibited. No agent clears `.devspace` or makes
durable facts depend on a concrete `.devspace` path.

## 4. Source Owners And Source Shape

Function addresses are evidence keys, not default source units. Expand an
address to the proven higher-order owner: class/interface, source-file cluster,
record/callback/table/global-object/static-member group, provider boundary,
subsystem, or strongly connected dependency group.

Class/interface recovery has priority when constructors/destructors, offset-0
table writes, `this` use, inherited cleanup, or dispatch xrefs support it.
Hand-authored `VTable`, `Vtbl`, `FTable`, slot arrays, dispatch views, provider
shims, raw offsets, and ABI call-shape scaffolds are not source-faithful models.

Before inventing or renaming a production-source identifier, class, module
folder, or filename, consult
`docs/reconstruction/source_naming_conventions.md` and search
`support/engine_terminology/`. These are evidence-ranked prospective defaults:
an exact Recoil spelling or path wins, and a naming pattern alone never proves
class ownership, source placement, function order, provider status, or tier.

Ordinary global/literal/constant/storage collections are auxiliary data packets
linked to a primary owner unless evidence proves the original source contained
that exact authored construct. Source owner/data gates, physical storage,
output sections, and final-image coverage are distinct. Unknown extents omit
size/end; never invent one-byte ranges.

The current production `src/` tree is implementation state, not
original-source authority. Passing smokes, current ABI shape, or a convenient
file layout is not source-shape proof.
When stronger BN, source-literal, neighboring-order, or VC5 evidence contradicts
it, reshape `.h`/`.cpp` files within the assigned scope. Recover declaration,
type, and body header layering plus include timing that naturally emits retail
order. Do not use new `.inl` files, pragmas, linker-order tricks, wrong-file
helpers, duplicate definitions, or post-link patching.

Source-level C/C++ `goto` statements are forbidden in production `src/` code.
A shared retail tail, folded branch, jump table, or other compiler-generated
control-flow shape is evidence about VC5 output, not evidence that the original
source used `goto`. Recover structured source with conditionals, switches,
loops, early returns, or an existing source-faithful state value, and let VC5
rediscover any equivalent folding naturally. Do not invent a helper merely to
hide a `goto`. The governed `guard source-goto` migration catalog is
monotonic: an exact reviewed legacy row may only be retired, every new,
relocated, changed, or reintroduced row fails, and cleanup packets must update
only the matching per-source retirement shard. The end state is strict zero.

Treat source-path literal and neighboring-order evidence as proof of
translation-unit contribution blocks when it is strong enough. Each block must
naturally reproduce generated VC5 COFF function order against retail Binary
Ninja address order.

`agent_source_path` is the worker path for physical-block units. It resolves
from `original_source_path`, then `provisional_original_path`, then legacy
`source_path`. Header/COMDAT placement exceptions require direct evidence and
must compile through the correct owning TU.

Source-to-binary traceability uses tracker-owned artifact relationships with
validated source-comment mirrors. A comment never accepts an owner, source
model, provider, tier, order, byte, storage, section, or final-image fact. Use
one repository-unique, source-stable anchor and one row per emitted artifact:

```cpp
/**
 * @recoil-anchor recoil:anchor:<stable-source-construct-id>
 * @recoil-artifact defines .text recoil:function:0xNNNNNN: Primary authored body.
 * @recoil-artifact emits .text recoil:function:0xNNNNNN: VC5-generated helper.
 * @recoil-artifact emits .rdata recoil:data:0xNNNNNN: VC5-generated table.
 * Purpose: Explains the source-level role of this construct.
 */
```

`defines` means the attached source construct is the direct authored definition;
`emits` means compiling that legitimate source construct causes an additional
function or data artifact. The section token is the exact final output section.
Attach the block immediately to a complete function definition, data definition,
complete type, recognized source-generation region, or explicit instantiation.
Detached registries, stacked address blocks, pure declarations, pipe-separated
address lists, and the legacy `Reimplements`/`Emits` address syntax are invalid.
Never add a fake wrapper, global, table, type, or other source construct merely
to provide an anchor.

Deliberately authored standalone construct symbols, including friendly Binary
Ninja labels, are permitted as local navigation labels with or without terminal
punctuation. They grant no evidence, artifact identity, source edge,
attachment, provenance, purpose, data-section fact, owner/tier state, or
acceptance, and never substitute for a required canonical `@recoil-artifact`,
`Purpose:`, or `Evidence:` row. Standalone source-path labels,
symbol-plus-path rows, repeated routing placeholders, and
lifecycle-contribution rows remain non-documentation. Do not repeat an exact
semantic prose row inside one comment block. Preserve substantive sentences and
wrapped continuations. Construct-only deletion is reserved for explicit
reviewed legacy `Reimplements` migration.

When several source constructs share one physical address, record distinct
reviewed logical artifacts and their ICF/pooling relationship instead of
duplicating the physical artifact id. Provider/import/external artifacts are
`not-applicable` and receive no production-source edge. Ambiguous ownership,
placement, section, extent, logical alias, or emission cause is `unresolved`
tracker debt and receives no source claim until reviewed evidence resolves it.
Update source mirrors and tracker relationships together through the governed
source-trace commands; never infer correctness from an address-bearing comment.

The no-literal shelf `[0x4b2960,0x4c0d20)` remains a known exception band:
current `.cpp` labels are provisional physical placements, not proof of
original translation-unit identity or global alphabetical order.

Use `recoil-source-model-recovery` for owner/source-shape recovery and
`recoil-provider-boundary` for CRT/MFC/DirectX/COM/compiler/runtime/provider
classification. The tracker remains the owner/gate/tier authority.

## 5. ChatGPT Pro: Escalation Only

The source-discovery ChatGPT Pro policy is ambiguity-driven. ChatGPT Pro is
advisory and never proves an owner, block, placement, catalog,
source model, tier, order, byte, or final-image result.

A parent-brokered Pro pass is mandatory only when:

- direct BN, retail, source, and VC5 evidence still leaves at least two
  competing source-owner/block/order models;
- a materially disputed tracker correction crosses a translation-unit, owner,
  provider, or physical-block boundary;
- raw inline assembly is proposed after credible source-faithful VC5SP3 C/C++
  variants fail;
- the user explicitly asks for an external critique.

Exempt routine registered order execution and interpretation, mechanical first
divergence, current-source rechecks, deterministic retail catalog derivation,
exact identity/provider lookup, tool-contract defects, and editing source to
satisfy an already-reviewed target model.

When triggered, the canonical prompt/attachment procedure is owned by
`recoil-source-model-recovery`; hard-byte/raw-assembly escalation is owned by
`recoil-tier-verification`. Workers return a scoped request packet and never
upload or invoke the Pro line themselves. The parent validates attachments,
serializes the call, and resumes the matching request id.

## 6. Binary Ninja

Before BN-backed work, check the bridge with `get_bridge_info` and `list_tools`,
then run `python tools/recoil.py doctor --quick --binja`. If unavailable, ask
the user to open the correct database; never load, switch, or patch binaries.

Read-only inspection uses `recoil-binary-ninja-workflow`. Mutation requires a
parent-assigned `recoil_bn_reconstructor` and
`recoil-binary-ninja-reconstruction`. That role may update assigned names,
types, prototypes, calling conventions, variables, globals, comments, function
properties, reanalysis, and saved state. It may not decide source owner, block,
provider, order acceptance, or tier.

Assembly, xrefs, layouts, and literals are primary. HLIL is useful but may hide
register/FPU/cleanup details. Correct current BN state when direct evidence
proves it wrong; do not work around wrong analysis labels in production source.

## 7. Tiers, Providers, And Raw Assembly

Owner promotion remains `C` behavior/source coverage, then accepted boundary,
source, data, and linkage gates for `B`, reviewed near-byte evidence for `A`,
and exact owner-scoped byte/provider ABI evidence for `S`. The owner tier is the
floor of its primary entries and gates. `Model:` records source-shape metadata;
`Reimplemented [S]` is the binary verification marker.

Passing behavior, ABI shape, one function's bytes, an object-order result, or a
data packet never silently accepts a complete owner. Provider-boundary owners
do not receive authored tiers. Tiny provider-looking bodies may still be
authored overrides when class, symbol, vtable/message-map, and physical-order
evidence prove it.

Use `recoil-tier-verification` before promotion and
`recoil-source-owner-scrutiny` for positive owner/data/linkage/tier-B+ or
source-faithful acceptance.

Raw assembly remains exception-only. Credible source-faithful C/C++ variants
must fail first; a triggered Pro pass must confirm the need; the inline block
must be minimal and supported by exact BN/VC5 opcode/register/FPU evidence, an
immediate/enclosing `Purpose:` docblock, and an address-scoped
`.agent/RAW_ASSEMBLY_ALLOWLIST.txt` entry. Naked functions, `_emit`, `.asm`
files, whole-function assembly, raw stack shells, provider shims, and order
tricks remain forbidden outside already documented CPU-probe exception classes.

## 8. Final Typed Coverage

`audit final-image-catalog` is a live coverage audit, not validation of a
manually populated candidate blob. It derives mechanical PE facts directly
from verified retail and joins accepted tracker facts for functions, variables,
providers, resources, storage, directories, padding, and overlay.

The audit builds exact file-backed and loaded-RVA interval partitions. Every
byte/range must be covered exactly once. Gaps, overlaps, unknown extents,
ambiguous padding, missing providers, or unresolved entities block. Persist
only narrow reviewed ambiguity annotations; deterministic retail facts are
rederived live and candidate output never supplies expected facts.

Final validation requires:

```powershell
python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil.py audit final-image-catalog --json
python tools/recoil.py verify final-image --json
```

The verifier fails before building when typed coverage is incomplete. When
complete, it performs one fresh unrestricted build and directly compares every
typed entity. Raw whole-file equality cannot replace typed comparison.

## 9. Tool, Role, Skill, And Instruction Maintenance

Documentation-only and infrastructure tasks do not select an address or require
Binary Ninja. Use:

```powershell
python tools/recoil.py doctor --infrastructure-only
python tools/recoil.py audit agent-surface --strict
python tools/recoil.py audit workflow-contracts --strict
python tools/recoil.py audit pipeline-reachability --strict
```

The root `README.md` contains one bounded marker-managed public snapshot
generated from the unified tracker. Every successful authoritative progress
mutation synchronizes it automatically. Use `python tools/recoil.py docs
readme-progress` for an explicit update and add `--check` for a non-mutating
freshness gate. The generated block is a public projection only; the tracker
remains the sole progress authority, and agents must not hand-edit or copy its
live values elsewhere.

`agent-surface` is static syntax/reference alignment. `workflow-contracts` and
`pipeline-reachability` prove that command transitions, handoffs, and required
expected-fact producers are executable. A static pass alone is not complete
workflow health.

Allowed agent-surface evolution paths are `.codex/skills/recoil-*`,
`.codex/agents/*.toml`, `.claude/skills/recoil-*`, `.claude/agents/*.md`,
`.claude/settings.json`, `CLAUDE.md`, `tools/recoil.py`, `tools/_recoil`,
`tests/tools`, and focused docs. Use `recoil_tool_maintainer` by default for
nontrivial changes.
Do not change production source, BN state, provider/owner/tier criteria, or
`.agent` ledgers unless separately assigned.

Two agent harnesses read this workspace, and `.codex` is the single canonical
side. Every canonical procedure body stays in `.codex/skills/recoil-*/SKILL.md`
and every role contract stays in `.codex/agents/*.toml`. The `.claude` surface
and `CLAUDE.md` are pointer mirrors: they carry harness routing metadata, the
verbatim canonical description, and the canonical path to read, never a second
copy of a procedure, contract, gate, tier, or acceptance rule. `audit
agent-surface` fails on a missing, orphaned, renamed, drifted, or thickened
pointer, so add or rename a canonical skill or role and its mirror together.

Report reproducible tool/rule defects instead of hiding them behind a
source-faithfulness or acceptance workaround. Workspace issues are for broken
tools, validation paths, environment/setup, or rules—not ordinary
reconstruction backlog.

## 10. Validation And No-Introduced-Debt Closeout

Choose the narrowest relevant checks through `recoil-validation`. For tool and
workflow changes, run focused unit tests, the full tools suite, static and
operational audits, infrastructure doctor, tracker audits, and issue audit.
For source work, run the packet's live verifier plus relevant source guards and
registered behavior tests.

Before reporting completion:

- ensure no worker or reservation remains active;
- reconcile the exact current tracker revision and scheduler command;
- remove or report generated source/IDE artifacts outside approved build roots;
- preserve unrelated user changes;
- put local facts in source comments, cross-file durable patterns in focused
  reconstruction docs, structured progress in the tracker, and routine
  validation output nowhere durable;
- do not write broad progress narratives, duplicate live tracker state, or make
  source checkpoints through git.

Final reports lead with pass/fail and the first actionable divergence. State
changed paths, exact commands/results, remaining blockers, and the narrow next
action. Never claim more than the direct comparison established.
