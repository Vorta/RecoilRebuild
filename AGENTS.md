# AGENTS.md

Mandatory workflow for AI- or human-assisted Recoil reconstruction.

## 0. Mission And Authority

Reconstruct a source-faithful native C/C++ codebase for the 1999 Windows x86
*Recoil* engine. An unrestricted VC5SP3 build must reproduce every catalogued
typed semantic fact in `support/Recoil.exe`.

Acceptance is live and semantic. The linker-written COFF timestamp and the raw
whole-file difference are diagnostic only. No source, object, executable,
receipt, snapshot, or artifact hash qualifies a candidate. Every acceptance
operation rebuilds current production source and compares typed code, data,
targets, providers, layout, resources, directories, padding, and PE facts
directly with retail.

Repository-local authority, subject to system/developer/host constraints:

1. the explicit user task;
2. this file;
3. `.agent/RECONSTRUCTION_PROGRESS.sqlite3` through `tools/recoil.py`;
4. the canonical skill or runbook that owns a specialized procedure.

Never hand-edit either SQLite authority. Progress uses semantic schema 6 and
SQLite user-version 3. Workspace issues use SQLite user-version 2. The
authoritative progress concurrency coordinates are transaction, semantic, and
evidence-generation revision. There is no scheduler revision, work-item
collection, packet/reservation schema, current-metadata cache, JSON backend,
mirror, or export.

Primary evidence:

- the already-open `Recoil.bndb` for current behavior, ABI, layout, globals,
  xrefs, imports, tables, function boundaries/order, and assembly;
- `support/Recoil.exe` as the immutable retail executable;
- the already-open `messages.bndb` and `support/messages.dll` only for an
  explicitly selected companion-DLL task;
- current source and fresh governed VC5SP3 output for candidate behavior.

BN names/comments are provisional navigation labels. Current assembly, xrefs,
literals, direct retail comparison, and governed compiler/linker output outrank
old notes, comments, filenames, smokes, and historical observations.

The canonical executable runbook is
`docs/reconstruction/retail_executable_reproduction.md`.

## 1. One Agent, One Serial Scheduler

`python tools/recoil.py progress next --json` is the only no-target scheduler.
It returns one `recoil-current-task-v2` object with one stage, one task, one
advisory scope, an optional nonmutating check command, exactly one direct
acceptance command when ready, a blocker when not ready, and the three retained
revision coordinates.

All work is performed directly in the canonical checkout by the current agent.
The workspace has no Recoil role registry, worker handoff, task allocator,
packet, reservation, lease, resource-arbitration, linked-worktree, integration,
or multi-lane workflow. Do not recreate those mechanisms to perform ordinary
work.

The six stages are strictly serial:

1. **`authored-function-order`** — traverse retail `.text` from
   `0x401000`. Every selected row is `authored`,
   `authored-lifecycle`, `non-authored`, or `unresolved`. Only
   `authored-body` and `authored-lifecycle-body` gate this stage.
2. **`authored-call-contract`** — verify the accepted authored-order census
   in deterministic retail-contiguous slices capped at 160 bodies. Require exact
   call count/order, call versus tail form, direct versus indirect dispatch,
   target/provider/IAT identity, slot/callback storage, and known cleanup.
3. **`authored-byte-match`** — require object body equality outside
   relocation fields plus exact relocation type/target/addend semantics, linked
   presence, target identity, and relocation-normalized linked body bytes.
4. **`full-function-order`** — begin only after every authored call contract,
   its fresh closeout, and every authored byte group are current. Require exact
   selected linked groups, identities, RVAs, order, providers, padding, and
   seams.
5. **`linked-byte-match`** — require exact linked RVA, resolved operands,
   targets, and raw linked-image bytes for every selected row.
6. **`final-validation`** — require complete live typed coverage of headers,
   sections, functions, variables, providers, resources, directories, padding,
   zero-fill, relocations, and overlay.

An unresolved row or unclassified selected extra blocks its stage. One stage
never silently accepts or revokes an owner, model, provider, tier, storage
contribution, output section, or final-image fact.

At raw object/TU order gates, inventory unlisted raw definitions but treat them
as a nonblocking diagnostic because `/OPT:REF` and `/OPT:ICF` may discard or
fold them. Full linked order still requires the exact selected population.

## 2. Direct Coding And Acceptance

For order source work, run:

```powershell
python tools/recoil.py verify vc5-order <target> --build-root <fresh-root>
```

Edit the current target’s source/header closure, inspect the first divergence
and its neighbors, and repeat until PASS or a concrete scope contradiction.
Then perform fresh live acceptance:

```powershell
python tools/recoil.py progress advance-live-order --target <target-id> --build-root <fresh-root> --expected-revision <revision> --apply --json
```

For call-contract source work:

```powershell
python tools/recoil.py verify call-contract --slice <slice-id> --build-root <fresh-root> --json
python tools/recoil.py progress advance-live-call-contract --slice <slice-id> --build-root <fresh-root> --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --apply --json
```

Only bodies passing the direct comparison in that invocation may advance. No
stored result or saved candidate substitutes. After every body is current,
record the required fresh, complete, no-reuse, zero-divergence scan:

```powershell
python tools/recoil.py progress call-contract close-live --build-root <fresh-root> --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --apply --json
```

The reviewed implementation coordinates are
`CALL_CONTRACT_VERIFIER_GENERATION = 18`,
`NORMALIZER_REGISTRY_GENERATION = 15`, and
`EXPECTED_FACT_SCHEMA_VERSION = 16`. A governed component edit increments its
coordinate and conservatively invalidates affected evidence.

For bytes:

```powershell
python tools/recoil.py progress advance-live-authored-byte --build-root <fresh-root> --expected-revision <revision> --apply --json
python tools/recoil.py progress advance-live-linked-byte --build-root <fresh-root> --expected-revision <revision> --apply --json
```

Direct `--apply` is normal for self-validating live commands. Optional
`--dry-run` is diagnostic. Manual semantic mutations—owner, block, provider,
classification, catalog exception, tier, and similar reviewed changes—remain
dry-run-first and are repeated with `--apply` only after review.

Relocation expectations come from immutable retail plus accepted typed
identity/provider/alias facts, never the candidate. Explicit empty sets are
valid. Missing deterministic target identity uses dry-run-first
`progress relocation-target bind`; genuine ambiguity alone uses
`progress relocation-exception set`.

## 3. Canonical Checkout And Build Roots

Edit the canonical checkout directly. Preserve unrelated user changes and
inspect the current working tree before broad edits. Git records authored
workspace history but never supplies retail expected truth or reconstruction
acceptance.

Live validation roots must be fresh, absent paths below
`build/live-validation`. Keep machine-local retail and SQLite authorities in
their canonical locations. Do not copy them into build roots. Reopen and
authenticate stable retail/BN inputs where the owning verifier requires it.

Do not clear `.devspace`, depend durably on a concrete scratch path, or perform
destructive primary-checkout Git operations.

## 4. Source Owners And Source Shape

Function addresses are evidence keys, not default source units. Expand an
address to the proven higher-order owner: class/interface, source-file cluster,
record/callback/table/global-object/static-member group, provider boundary,
subsystem, or strongly connected dependency group.

Class/interface recovery has priority when constructors/destructors, offset-0
table writes, `this` use, inherited cleanup, or dispatch xrefs support it.
Hand-authored vtables, slot arrays, dispatch views, provider shims, raw offsets,
and ABI call-shape scaffolds are not source-faithful models.

Before inventing or renaming a production identifier, class, folder, or
filename, consult `docs/reconstruction/source_naming_conventions.md` and
search `support/engine_terminology/`.

The current `src/` tree is implementation state, not original-source
authority. Reshape `.h`/`.cpp` layering when stronger BN, literal,
neighboring-order, or VC5 evidence contradicts it. Do not use new `.inl`
files, pragmas, linker-order tricks, wrong-file helpers, duplicate definitions,
or post-link patching.

Source-level C/C++ `goto` is forbidden in production `src/`. Recover
structured source. Raw assembly remains exception-only under the tier skill.

Use canonical attached source anchors:

```cpp
/**
 * @recoil-anchor recoil:anchor:<stable-source-construct-id>
 * @recoil-artifact defines .text recoil:function:0xNNNNNN: Primary authored body.
 * @recoil-artifact emits .rdata recoil:data:0xNNNNNN: VC5-generated table.
 * Purpose: Explains the source-level role of this construct.
 */
```

Comments mirror tracker relationships; they accept nothing by themselves.
Ambiguous ownership, placement, section, extent, alias, or emission cause stays
`unresolved`.

The no-literal shelf `[0x4b2960,0x4c0d20)` remains a known exception band.
Use `recoil-source-model-recovery` for source shape and
`recoil-provider-boundary` for provider/runtime classification.

## 5. ChatGPT Pro: Ambiguity-Driven Advisory

ChatGPT Pro is advisory and never proves an owner, block, placement, catalog,
model, tier, order, byte, or final-image result. The current agent invokes the
Pro line directly when:

- direct BN, retail, source, and VC5 evidence still leaves at least two
  competing source-owner/block/order models;
- a materially disputed correction crosses a TU, owner, provider, or physical
  block boundary;
- raw inline assembly is proposed after credible source-faithful VC5 C/C++
  variants fail;
- the user explicitly requests an external critique.

Routine registered order execution, first-divergence interpretation,
current-source rechecks, deterministic retail derivation, identity lookup, tool
defects, and editing an already reviewed target model do not require Pro.

The source-model skill owns source-discovery prompt/attachment procedure; the
tier skill owns hard-byte/raw-assembly escalation. Validate attachments,
serialize calls through the persistent Pro line, and treat the answer only as
advice.

## 6. Binary Ninja

Once after Binary Ninja is opened, reopened, or switched for the current
working session, authenticate the intended database with:

```powershell
python tools/recoil.py binja preflight --binary recoil --strict
```

Reuse that result while the database and connection remain unchanged.
Call-contract slices and their closeout query the target-qualified database
directly and do not repeat database preflight per slice.

If unavailable, ask the user to open the correct database; never load, switch,
or patch binaries. Read-only inspection uses
`recoil-binary-ninja-workflow`. Mutation uses
`recoil-binary-ninja-reconstruction`, completes reanalysis/propagation
checks, and saves before returning to source work. Do not decide source owner,
provider, order acceptance, or tier from a BN edit.

Assembly, xrefs, layouts, and literals are primary. HLIL may hide register,
FPU, and cleanup details.

## 7. Tiers, Providers, And Raw Assembly

Owner promotion is C behavior/source coverage, then accepted boundary/source/
data/linkage gates for B, reviewed near-byte evidence for A, and exact
owner-scoped byte/provider ABI evidence for S. The owner tier is the floor of
its primary entries and gates. Provider-boundary owners do not receive authored
tiers.

Passing behavior, one function’s bytes, an order result, or a data result never
silently accepts a complete owner. Use `recoil-tier-verification` before
promotion and `recoil-source-owner-scrutiny` for positive owner/data/linkage/
tier-B+ acceptance.

Raw assembly requires failed credible C/C++ variants, a triggered Pro pass,
exact BN/VC5 opcode/register/FPU evidence, a nearby `Purpose:` docblock, and
an address-scoped `.agent/RAW_ASSEMBLY_ALLOWLIST.txt` entry. Naked functions,
`_emit`, `.asm` files, whole-function assembly, and order tricks remain
forbidden outside documented CPU-probe exceptions.

## 8. Final Typed Coverage

`audit final-image-catalog` derives live coverage from verified retail and
accepted tracker facts. Every file-backed and loaded-RVA byte/range must be
covered exactly once. Gaps, overlaps, unknown extents, ambiguous padding,
missing providers, or unresolved entities block.

Final validation requires:

```powershell
python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil.py audit final-image-catalog --json
python tools/recoil.py verify final-image --json
```

The verifier fails before building when typed coverage is incomplete. Raw
whole-file equality cannot replace typed comparison.

## 9. Tool, Skill, Instruction, And Validation Maintenance

Documentation/infrastructure work does not select an address or require BN.
Canonical procedures live in `.codex/skills/recoil-*/SKILL.md`. There are no
repository-local skill mirrors or role registries. The README contains only a
static pointer to `progress next --json`; the tracker is the sole current-state
record.

Use `recoil-validation` as the single validation matrix for tool, skill,
instruction, tracker, build, and final-image changes. Other skills and runbooks
must refer to it rather than copying maintenance command lists.

Report reproducible tool/rule defects through `issue report`. Workspace issues
track broken tools, validation paths, environment/setup, or rules—not ordinary
reconstruction backlog. Keep validation proportional to changed scope, lead
with the first actionable divergence, and never claim more than the direct
comparison established.
