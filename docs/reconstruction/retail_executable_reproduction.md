# Retail Executable Reproduction

This is the canonical operational runbook for rebuilding the 1999 Windows x86
Recoil executable from source with VC5SP3.

## Acceptance Model

The goal is source-faithful C/C++ whose unrestricted build reproduces every
catalogued typed semantic fact in `support/Recoil.exe`.

Acceptance always uses current source and fresh governed output. A candidate
hash, source hash, object hash, saved executable, receipt, snapshot, or raw
whole-file match cannot qualify work. The linker COFF timestamp and raw file
delta are diagnostic only.

Expected truth comes from:

1. immutable retail bytes and PE structure;
2. current assembly, xrefs, imports, layouts, literals, tables, and function
   boundaries in the already-open `Recoil.bndb`;
3. reviewed tracker identity/provider/alias facts;
4. current source and fresh VC5SP3 output only as the candidate.

## Current Task

With no explicit target:

```powershell
python tools/recoil.py progress next --json
```

The result is one serial task. Work it directly in the canonical checkout.
There is no secondary queue or allocation step.

The stages are:

1. authored function order;
2. authored call contracts;
3. authored bytes;
4. full linked function order;
5. linked bytes;
6. final typed validation.

Do not move to full order until authored bytes are complete.

## Stage 1: Authored Function Order

Retail traversal begins at `0x401000`. Each selected row must be reviewed as
`authored`, `authored-lifecycle`, `non-authored`, or `unresolved`.
Only physical `authored-body` and `authored-lifecycle-body` identities gate
the authored order.

Run the registered target:

```powershell
python tools/recoil.py verify vc5-order <target> --build-root <fresh-root>
```

The verifier reports the first missing, duplicated, unexpected, or reordered
identity and its neighbors. Edit only the source/header closure implicated by
the current target. Recover declaration order, source-file placement, header
layering, include timing, and structured source that naturally produces the
retail order.

Do not use pragmas, linker-order manipulation, duplicate definitions,
wrong-file helpers, post-link patching, or source-level `goto`.

One unresolved row inside the target interval blocks acceptance of the complete
covered physical slice. Raw definitions omitted from the target are diagnostics
because the linker may discard or fold them; full linked order later requires
the exact linked population.

The reviewed `recoil-logical-alias-group-v5` route represents shared-header
inline or implicit complete destructors. It separates the attached source
construct from its proven emitting translation units and retains one physical
authored gate. A target names one proved logical object witness; this is not an
assertion about the original linker winner. Every declared member emitted by
that TU must still exist in the fresh object as an independent ANY COMDAT with
identical complete bytes and relocation semantics. Only exact self-referencing
VC5 FPO associations are supported. Missing members, changed cleanup code,
unproved targets, duplicate source mirrors, and unknown physical artifacts fail
closed. Retail selector evidence and the paired same-object ICF/NOICF mechanism
proof remain mandatory; neither a selected witness nor a saved map accepts
linked placement or bytes.

For a read-only lifecycle diagnosis, `audit coff-lifecycle --object <obj>
--symbol <decorated-name> [--symbol ...] [--map <map>]` inventories complete
definitions, associations, inbound relocations, and map observations. Its output
is diagnostic only and does not establish freshness or accept tracker facts.

Fresh acceptance:

```powershell
python tools/recoil.py progress advance-live-order --target <target-id> --build-root <fresh-root> --expected-revision <revision> --apply --json
```

The same invocation builds, validates, derives safe complete slices, and
CAS-updates only order state.

## Stage 2: Authored Call Contracts

The live census is derived from reviewed authored-order classifications, not a
fixed historical body count. Slices are deterministic retail-contiguous windows
of at most 160 bodies.

The direct verifier compares:

- exact invocation count and order;
- call versus tail-call form;
- direct versus indirect dispatch;
- authored/self/provider/IAT target identity;
- virtual/interface slot or callback storage;
- selected retail-proven provider argument bits and their reaching-definition provenance;
- selected initializer dependencies on concrete constructor vptr writes and emitted dispatch targets;
- selected embedded-state dispatch dependencies on the concrete quit override and its live retail return semantics;
- selected background-thread dispatch dependencies on the complete object's constructor stamp, emitted update slot, and global receiver load;
- known caller cleanup;
- current direct Binary Ninja facts from the target-qualified loaded database.

```powershell
python tools/recoil.py verify call-contract --slice <slice-id> --build-root <fresh-root> --json --summary
```

Unresolved target, storage, provider, import, ICF/logical-alias identity, or
indirect provenance fails closed.

Argument/dependency selection rules do not contain saved expected flag values
or table contents. Those facts are derived from the live retail body and
compared with the freshly compiled candidate. Unsupported bit operations,
conflicting CFG predecessors, missing constructor bodies, unbound table slots,
or unresolved target identities block the selected proof. Constructor listing
coverage permits only bounded post-return VC5 alignment, never additional code.
The save/load initializer obligation checks the complete relocated table extent
and selects deleting, activation, entry, and deactivation targets. Other
inherited/folded cells do not gain logical-alias or full-table byte acceptance.
Weak dispatch targets require the exact fresh COFF weak-external/default chain;
decorated-name substitution is not proof. The embedded quit-return obligation
does not accept a new physical/logical ICF identity. This is not a claim
that call contracts establish every ordinary data-flow or rendering behavior;
the later byte and final-image stages remain necessary.

The briefing update obligation follows entry `this` through a bounded normal
constructor CFG, requires the same explicit vptr stamp on every return, and
compares the generated three-cell table's update target with live retail.
The background-thread obligation also binds its actual slot-zero invocation to the
relocated global receiver load. Missing overrides, bypassed receiver loads,
ambiguous table relocations, and unsupported constructor instructions block
verification. This is a selected dispatch dependency, not acceptance of all
table cells or of arbitrary side effects inside constructor callees.

Fresh acceptance:

```powershell
python tools/recoil.py progress advance-live-call-contract --slice <slice-id> --build-root <fresh-root> --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --apply --json
```

Only bodies passing in that invocation advance. An unrelated divergent body
stays pending and keeps the cursor on its first slice.

The normal authored-call-contract route is the serial replay command; it
replaces ad hoc shell loops across the complete original-slice census:

```powershell
python tools/recoil.py progress call-contract replay-live --dry-run --json
python tools/recoil.py progress call-contract replay-live --apply --json
```

Dry-run plans the complete original-slice census and fresh replay sibling root
without building, querying Binary Ninja, or mutating. Apply loads the serial
task once, creates an exclusive `-replay-NNN` sibling without consuming the
scheduler-selected direct root, and performs one invocation-local complete
proof. Repository/source discovery, each unique target build, each separate
definition-TU build, the COD index, and target-qualified immutable Binary Ninja
facts are shared across the census. The proof is projected back into the exact
original slices and committed serially through the same per-body evidence and
semantic/evidence CAS path as direct acceptance. Already-current predecessor
slices are revalidated. The first divergent current slice commits only its
passing non-current bodies and stops; later slices remain untouched. Interrupted
roots are inert, and a resumed invocation uses a new sibling. Replay neither
supplies expected truth nor performs the mandatory closeout. After the last
slice passes, it returns the scheduler-selected `close-live` command without
running it.

After all bodies are current:

```powershell
python tools/recoil.py progress call-contract close-live --build-root <fresh-root> --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --max-workers 8 --apply --json
```

The closeout runs the complete current census from fresh output, forbids reuse,
requires zero divergence, queries the target-qualified canonical BN database
directly without repeating database preflight per slice, records each slice's
exact JSON-native expected-fact transcript, and records the current verifier
and expected-fact generations. The isolated slice verifiers use bounded
subprocess concurrency: eight slots by default, configurable from one through
21. Completion order is diagnostic; validation and storage always follow
retail slice order. After every slice passes and before tracker mutation,
closeout runs exactly one fresh canonical whole-program compile, COFF-alias
assembly, resource build, and link below the closeout root. This linkability
diagnostic must produce the executable and map, suppresses linked-order
evaluation and playground deployment, and accepts no byte, linked-order, or
final-image fact. It is never repeated per slice. The final closeout CAS is one
serial mutation and remains the only route to stage completion.

## Playground Build Safeguard (Not An Acceptance Stage)

### External gameplay diagnosis

For a user-requested Windows x86 reproduction, the bounded external debugger
is available through the workspace toolset:

```powershell
python tools/recoil.py diagnose gameplay-start --exe playground/Recoil-rebuild.exe --map <matching-linker-map> --output-dir build/diagnostics/<fresh-root>
```

The user or available Computer Use connection performs the menu actions. The
command launches only the named executable with its own directory as working
directory, records exceptions and periodic thread snapshots, and stops only
that exact process at the five-minute deadline. It injects no code or
breakpoints and accepts no reconstruction facts. Output contains raw x86
contexts, stack memory with explicitly heuristic symbol candidates, loaded
modules, and the last loading log line. Three one-second samples are taken
after a loading-log line has stayed unchanged for twenty seconds; log silence
alone is not classified as a hang. Existing game logs are copied before launch
and the new logs retained at completion.

Optional exact map symbols can be sampled with repeated `--watch-symbol`
arguments. `--timeout` is bounded to 1-300 seconds and `--snapshot-interval` to
1-30 seconds. An ambiguous/missing watched symbol, unavailable debugger API,
or unsupported host/target architecture fails explicitly. The tool requires
64-bit Windows Python and an x86 target. A diagnostic map is not acceptance
evidence or proof of correspondence to current production source.

The Windows structures/event lifecycle follow Microsoft's
[debug-event API](https://learn.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-waitfordebugevent)
and [WOW64 context layout](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-wow64_context).

### Governed deployment

Before deployment, both canonical build modes also run the startup
regression guard against the same fresh object and linked image. Its present
scope includes the complete constant member-byte writes of the turret initializer
and selected unprojection, light-update, and model-rendering matrix-stack depth
on every reachable control-flow path, derived directly from immutable retail.
It detects missing/wrong initialization,
including the omitted active-trail flag, rejects unsupported initializer code,
and distinguishes branch-exclusive cleanup from sequential double pops despite
identical static call counts. The unprojection/light checks require reviewed
callee effects. Model-renderer checks cover local primitive invocations only,
not transitive callee effects. Unsupported control transfers, unresolved branches,
underflow, and object/linked byte or relocation target/addend disagreement block.
The call-contract verifier also enforces the selected path proof. It is not full
startup control-flow, whole-program call-contract, or runtime acceptance.
Failure reports `failure_stage: startup-contract` and leaves the playground exe
unchanged. No extra compile/link is performed for this guard during deployment.

The standalone non-deploying route is:

```powershell
python tools/recoil.py verify startup-contract --build-root build/live-validation/<fresh-root> --json
```

This performs one fresh canonical linkability build and then the same check.
The separate `--linkability-only` route itself still suppresses this guard and
all deployment, as required by call-contract closeout.

For broader read-only investigation of matrix-stack users in an existing
canonical build, use:

```powershell
python tools/recoil.py diagnose matrix-stack --build-dir <canonical-build> --output-dir build/diagnostics/<fresh-root>
```

The survey queries the open Recoil BN cross-reference census, authenticates its
instruction bytes against retail, and compares local primitive push/pop paths
with object/MAP-qualified candidate bodies. It reports matching local depths,
inspection candidates, and unresolved cases separately. Nonprimitive calls are
not analyzed transitively and branch predicates are unconstrained; a survey
flag is not itself a proven source defect, and a local match is not gameplay or
reconstruction acceptance. Bounded redisassembly uses `llvm-objdump` from PATH
when whole-section VC5 DUMPBIN loses synchronization after inline data.

For a requested playground test executable, use the explicit non-accepting mode:

```powershell
python tools/recoil.py verify final-build --playground-only --build-dir build/live-validation/playground/<fresh-root>
```

This mode requires an absent root, the canonical manifest/profile and tracker,
and a complete fresh compile, resource build, and link. It runs the same complete
authored linked-presence safeguard described below and deploys only if that
passes. A failed deployment is also a command failure. Partial builds, diagnostic
profiles, custom order targets, `--clean`, and `--linkability-only` cannot be
combined with it. It evaluates no linked order and accepts no order, byte, alias,
or final-image fact; those comparisons remain separate serial-stage work.
In particular, candidate ICF sharing is neither a retail alias proof nor a reason
to fabricate source solely to keep identical functions apart.

A normal canonical `verify final-build` checks required authored linked
presence across the complete accepted authored-order census before replacing
the playground executable. The current linked-order cursor does not limit that
presence check. Missing/stale target registrations, uncovered census members,
missing functions, and ambiguous matches block deployment and are reported as
`failure_stage: linked-presence`.

The accepted census owns membership: a compiler-lifecycle manifest role does
not remove a selected identity's presence obligation. Every census member must
still have required presence and an unambiguous linked selector.

The linked-presence guard checks selected symbolic identity presence only. It permits named
aliases to share an address and does not require retail RVAs, ordering, or body
bytes. It neither accepts a reconstruction stage nor proves runtime behavior.
The explicit `--linkability-only` diagnostic used by call-contract closeout
returns before this guard and before all playground deployment, preserving its
compile/resource/link-only contract.

## Stage 3: Authored Bytes

The public authored acceptance command is:

```powershell
python tools/recoil.py progress advance-live-authored-byte --build-root <fresh-root> --expected-revision <revision> --apply --json
```

Authored equality requires:

- exact object body bytes outside relocation fields;
- exact relocation type;
- immutable-retail-derived target/provider/alias identity;
- exact relocation addend semantics;
- linked presence and symbolic identity;
- relocation-normalized linked body bytes.

Expected relocation facts never come from candidate output:

```powershell
python tools/recoil.py audit relocation-expectations --at 0xNNNNNN --json
```

An explicit empty expectation is valid. A missing deterministic target identity
uses reviewed `progress relocation-target bind`. A genuine ambiguity alone
uses reviewed `progress relocation-exception set`. Both are dry-run-first.

## Stage 4: Full Function Order

Full order begins only after authored call-contract closeout and authored byte
completion. Restart at `0x401000`.

Require exact:

- selected linked physical address groups;
- identities and RVAs;
- relative order;
- provider/compiler-generated members;
- predecessor/successor seams;
- padding intervals.

Use the current task’s registered target and the same order validation/acceptance
commands as stage 1. Passing raw object order is feedback only; acceptance is
the linked full-order result.

## Stage 5: Linked Bytes

```powershell
python tools/recoil.py verify linked-byte --at 0xNNNNNN
python tools/recoil.py progress advance-live-linked-byte --build-root <fresh-root> --expected-revision <revision> --apply --json
```

Linked validation requires exact linked RVA, resolved operands, target identity,
and raw linked-image bytes. It may advance only explicitly matched physical
groups before the first typed divergence.

## Stage 6: Final Typed Validation

The live catalog partitions file-backed and loaded RVA intervals and joins
accepted tracker facts for functions, variables, providers, resources, storage,
directories, padding, zero-fill, relocations, and overlay.

Coverage includes headers and section tables, linker-produced seams and
directories, import/export/provider boundaries, resources, initialized data,
BSS and other loaded zero-fill, base relocations, file/alignment padding, and
overlay bytes. Each file-backed or loaded-RVA interval has exactly one typed
owner; neither an unmapped gap nor two overlapping explanations may pass.

```powershell
python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil.py audit final-image-catalog --json
python tools/recoil.py verify final-image --json
```

Every range must be covered exactly once. Gaps, overlaps, unknown extents,
ambiguous padding, missing providers, or unresolved entities block before the
unrestricted build.

## Source Shape

Addresses identify evidence, not source units. Expand work to the proven
source-level construct: class/interface, TU cluster, table/callback group,
record subsystem, global object/static-member group, provider boundary, or
strongly connected dependency group.

Prefer class recovery when construction/destruction, offset-0 table writes,
`this` use, inherited cleanup, or dispatch xrefs support it. Do not model
authored classes as raw offsets, slot arrays, synthetic vtables, or provider
shims.

Consult `docs/reconstruction/source_naming_conventions.md` and
`support/engine_terminology/` before inventing production names and paths.
The current `src/` layout is not evidence of original placement.

Attach canonical `@recoil-anchor` and `@recoil-artifact` rows to the actual
source construct. A comment mirrors traceability; it does not accept an owner,
model, provider, tier, order, byte, section, or final fact.

## Provider And Tier Separation

Provider/import/external artifacts are not authored source. Use
`recoil-provider-boundary` to classify CRT, MFC, DirectX, COM, compiler, and
runtime boundaries.

Authored owner tiers progress from C to B to A to S. The owner tier is the floor
of all primary entries and required gates. Passing behavior, one function’s
bytes, order, or data equality never promotes a complete owner implicitly.

Use `recoil-tier-verification` before promotion and
`recoil-source-owner-scrutiny` for positive owner/data/linkage/tier-B+
acceptance.

## Raw Assembly Exception

Raw assembly is considered only after credible source-faithful VC5 C/C++
variants fail and a triggered ChatGPT Pro pass advises that it is necessary.
Require exact BN/VC5 opcode/register/FPU evidence, a nearby purpose docblock,
and an address-scoped allowlist entry. Keep the block minimal.

Naked functions, `_emit`, standalone assembly files, whole-function assembly,
raw stack shells, provider shims, and order tricks are forbidden outside
documented CPU-probe exceptions.

## ChatGPT Pro

Invoke the persistent Pro line directly only when evidence leaves competing
source models, a disputed correction crosses an ownership/TU/provider/block
boundary, raw assembly is proposed after credible failures, or the user asks
for external critique.

Pro is advisory. Preserve the competing models and primary evidence in the
prompt, ask it to challenge the interpretation, and return to direct retail/BN/
VC5 evidence for every actual acceptance.

## Binary Ninja

Once after Binary Ninja is opened, reopened, or switched for the current
working session:

```powershell
python tools/recoil.py binja preflight --binary recoil --strict
```

If the bridge is unavailable, ask the user to open the correct database. Do not
load or switch binaries. Reuse the preflight result while the database and
connection remain unchanged; never repeat it per slice. Read-only inspection
never saves. Reconstruction edits only explicitly selected analysis facts,
runs propagation checks, and saves before returning to source work.

## Persistence And Recovery

The two live SQLite databases are the only runtime authorities. Never hand-edit
them. Semantic mutations use CAS and fresh evidence. Issue mutations use their
independent monotonic revision.

The README contains only a static pointer to `progress next --json`. It is not
a second current-state authority and no tracker transaction updates it.

## Validation

After tool, docs, skill, or tracker work, use the canonical matrix in
`recoil-validation` rather than maintaining another copy here.

Run focused unit tests first. Preserve unrelated changes. Report exact commands,
first divergences, changed paths, remaining gaps, and the narrow next action.
