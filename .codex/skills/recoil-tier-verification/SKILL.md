---
name: recoil-tier-verification
description: Verify RecoilRebuild authored/provider owner tier evidence. Use when you need to promote, downgrade, or audit source-owner gates, Reimplemented [X/C/B/A/S], Provider-boundary owners, functional targets, touched-global data, helper provenance, docblocks, VC5 byte/provider ABI evidence, or owner tier eligibility.
---

# Recoil Tier Verification

## Global Text Pipeline

Global traversal has a primary lane (`authored-function-order`, then
`authored-call-contract`, then `full-function-order`) and an independent
retail-monotonic authored-byte lane. Full order begins only after every
call-contract slice is current and does not wait for authored bytes.
`linked-byte-match` starts after full order and authored bytes are complete,
then `final-validation`.
`progress next` chooses which complete owner may be verified; evidence
registration never promotes owner tiers. Preserve all owner/data/linkage gates
and defer unrelated owner-local byte work except an explicitly returned
`parallel_authored_byte_cursor`. Authored-byte acceptance uses exact object
bytes outside relocations, relocation identities/addends, linked presence,
symbolic target identity, and relocation-normalized linked body bytes; exact
retail RVA and resolved relocation operands remain diagnostic until the later
full/link phases. This phase distinction does not weaken owner tier `S`.

## Core Rule

Use this skill for owner gate/tier eligibility, not general build selection. For general validation commands, use `recoil-validation`.

Start from root `AGENTS.md`, `recoil-progress-tracker`, and current evidence.
Use `progress show` and `progress find` for joined owner context; do not read
the entire tracker for normal tier verification. Structural owner replacement
uses parent-only dry-run-first `owner replace-batch`, and conservative
invalidation uses `owner downgrade`. Unsupported positive metadata/gate/tier
mutations are workspace issues, not generic owner operations. Owner tiers are
derived from primary entry tiers plus gates; an entry-level S live byte result
does not by itself accept its owner byte gate.

New accepted or limited-accepted owner gates/tiers require current-session
source/build/Binary Ninja/provider evidence. Evidence that would justify a
positive mutation is returned for scrutiny, but without a registered positive
command it remains blocked behind a workspace issue. Downgrade false or stale
gates/tiers with `owner downgrade` instead of preserving them.
Accepted source, data, linkage, and tier `B`/`A`/`S` promotions require a linked
tracker owner record. If `progress show 0xNNNNNN` has no owner or the required
gate is pending/blocked, keep the owner gate or tier failed even when an
isolated function body or smoke passes. When relationship meaning is ambiguous,
inspect the joined JSON view; primary function/data ownership, dependency links,
and anchor-only links are different evidence.
Before accepting positive owner/data/linkage/tier `B`+ gates or `Model: source-faithful`,
run the source-owner scrutiny gate with focused `progress audit --scope owners
--strict`; include current BN membership evidence when relevant.
Block promotions when the evidence proves only an address slice, an arbitrary
source-file subset, a folded/shared body assigned to one class, anchor-only
ownership, or test/ABI/byte evidence without a complete source owner.

Use the source-discovery Pro trigger in `recoil-source-model-recovery` only when
direct evidence leaves a genuine competing source model or a materially
disputed cross-boundary correction. Registered order-target execution,
first-divergence reporting, deterministic catalog derivation, and editing an
already-reviewed source model are exempt. When triggered, the worker returns
the scoped request packet and the parent brokers the call; Pro remains advisory
and proves no owner, block, gate, model, tier, order, or byte acceptance. Use the
complete prompt and transport procedure in `recoil-source-model-recovery`
instead of copying it here.

Treat an address as an evidence key, not the default verification unit. The primary source-shaped owner is the normal binary-lane work unit. When owner tier eligibility depends on a class/interface, source-file cluster, subsystem, authored callback/record/table/global object/static class-member group, or recursive group, verify that coherent owner unit. Ordinary initialized-global/literal/constant/storage groupings are auxiliary data packets that feed data gates and byte-readiness for a primary owner, unless evidence proves the original source had that exact authored data construct. Do not choose isolated function or data-packet verification because the owner pass looks large, slow, or multi-turn; expected effort is not a tier blocker. Class/interface verification has priority when current evidence supports that source model.

For normal work, verify only the owner selected by `python tools/recoil.py
progress next` or explicitly assigned by the user. Focused owner views never
select work. Global final-data and final-repro diagnostics are observed-
evidence producers, not
source-owner entries, work units, schedulers, or owner-action generators.
Do not start or recommend isolated implementation or an `S` attempt while the
surrounding owner/source model remains failed or unaudited. A code/function
source owner may proceed to `S` only when the complete primary source-shaped
owner plus primary-owned, referenced, touched, linked, and dependency data
packets are ready for that owner's byte gate, unless the user explicitly
directs a narrower diagnostic. Owner-tracked data
variables may advance to `S` when their own linked data-packet byte gate and
data-symbol evidence are accepted, but that means dependency byte-readiness,
not parent/source-owner tier `S` completion unless the data packet itself is
proven to be the exact original authored source construct.

Current repo rules also require original-source helper provenance and canonical
source-trace/Purpose docblocks for touched address-backed definitions before
accepting higher authored tiers. Treat unsupported reconstruction helpers as
tier blockers, not harmless implementation details. A helper with no
standalone retail address can be valid when it is recovered as likely original
inline/static/member source and verified through callers or the smallest
source/class cluster; caller addresses remain evidence prose, not artifact
claims for the helper.

## Owner Tier Order

Verify owner gates in order:

1. Boundary and dependency evidence for the complete owner.
2. `Reimplemented [C]` behavior/source evidence.
3. Accepted `source`, `data`, and `linkage` gates.
4. `Reimplemented [B]`.
5. `Reimplemented [A]`.
6. `Reimplemented [S]`.

Do not let stale out-of-order owner gates or tiers override blockers. A caller remains source-blocked when dependency gates are unknown or failed, even if later tiers were previously accepted.
In binary-lane triage, treat a failed owner source gate as a structural blocker ahead of
isolated implementation and tier `C` evidence, and treat `Source owner ❌` plus
failed data/linkage gates inside the same linked owner scope as blockers ahead of
verify-only code/function tier `S` debt for that owner.

## Authored Tier Gates

Use root `AGENTS.md` for the exact owner gate/tier criteria and status semantics. Keep
this skill focused on the audit sequence:

- Boundary and dependency gates: prove current BN names, types, callers/callees,
  layout, globals, provider/table dependencies, and owner boundaries before
  considering source tiers.
- `Reimplemented [C]`: require real compiled source, required canonical
  source-trace/Purpose docblocks, targeted behavior evidence, and no raw authored
  runtime offsets, unsupported helpers, source-shape scaffolds, provider shims,
  or unallowlisted/undocumented raw assembly.
- `source`, `data`, and `linkage` gates: require the linked
  `.agent/RECONSTRUCTION_PROGRESS.sqlite3` owner record, scrutiny gate, and complete owner
  or data-owner evidence; `data=none` is valid only for current no-authored-globals
  proof, and `linkage=accepted` is distinct from future whole-exe fully-linked evidence.
- `Reimplemented [B]`, `[A]`, and `[S]`: require tier `C` plus accepted
  owner/data gates for `B`, reviewed near-byte equivalence for `A`, and
  generated byte/provider ABI evidence for `S` at the complete primary
  source-shaped owner unit plus all referenced/touched/linked data packets.
  Isolated function `S` remains exception-only for true standalone owners.

Owner-tracked data entries use the same tier names with data-specific gates:
`X` no accepted source-level data implementation; `C` canonical compiled
definition/declaration plus linked owner `boundary/source` accepted; `B` linked
owner `data` accepted; `A` reviewed near-byte-equivalent data-symbol evidence;
`S` accepted data-symbol bytes and relocation identity plus linked data-packet
`byte` accepted, with no current final executable `.data` layout contradiction
for the row or owner byte gate. Data-entry `S` is byte-ready evidence for
parents that reference or touch it; it is only primary source-owner `S` when
the packet is itself proven to be the exact original authored data construct.

## Provider Boundary

Use `provider-boundary` owners for compiler/runtime/import/framework/provider
functions. They never carry authored `Reimplemented [X/C/B/A/S]` tiers. Do not
author fake stand-ins, local provider storage, or mixed authored/provider owner
states.

Read-only classifiers return the intended final owner kind and complete
relationship set; they do not mutate the ledger. The parent inspects the
current record before any correction:

```powershell
python tools/recoil.py progress show <owner-id-or-address>
python tools/recoil.py progress show <owner-id-or-address> --json
```

When a replacement provider or authored record is needed, the parent supplies
the complete exact current and replacement records to the registered
dry-run-first `owner replace-batch` route described by
`recoil-provider-boundary`. Finish with:

```powershell
python tools/recoil.py progress owner replace-batch --payload-json '<recoil-owner-replace-batch-v2-object>' --expected-revision <revision> --dry-run --json
python tools/recoil.py progress show <replacement-owner>
python tools/recoil.py progress show <replacement-owner> --json
python tools/recoil.py progress audit --scope owners --strict
```

The parent reviews the dry-run before repeating it with `--apply`. There is no
direct kind-changing shortcut, and unsupported positive gate/tier/metadata
changes remain workspace issues rather than implied commands.

## Verification Commands

- Address context: `python tools/recoil.py progress show 0xNNNNNN`
- Deferred owner context: `python tools/recoil.py progress show <owner-id-or-address>`
- Owner context: `python tools/recoil.py progress show <owner-id-or-address>`
- Owner relationship context: `python tools/recoil.py progress show <owner-id-or-address> --json`
- Owner entry: `python tools/recoil.py progress show 0xNNNNNN`
- Typed owner dependencies: `python tools/recoil.py progress owner relationships 0xNNNNNN`
- Functional evidence: `python tools/recoil.py verify functional 0xNNNNNN`
- Original-source helper guard: `python tools/recoil.py guard original-symbol --root src --max 50`
- Touched-source trace audit: `python tools/recoil.py audit source-trace --path <touched-source> --policy migrated --json`
- VC target coverage: `python tools/recoil.py verify vc5 --list`
- VC owner byte evidence: `python tools/recoil.py verify vc5 --owner <owner-id-or-address> --auto-chunk`
- VC target/address diagnostics or manifest development: `python tools/recoil.py verify vc5 <target-or-address>`
- Missing VC target guidance: `python tools/recoil.py verify vc5 --explain-missing 0xNNNNNN`
- Final executable validation: `python tools/recoil.py verify final-image --json`
- Final executable data drift: `python tools/recoil.py audit final-data --include-owners --strict --json-out build/vc5-final/final_data_diff.json`

Never use `--skip-bn-compare` for byte-sentinel or tier `S` evidence. A
non-order function/data target may use that flag only as a compile-only
diagnostic. An order-only manifest can support only the explicitly enabled
function-order or translation-unit-order comparison; it supplies no byte
acceptance.

For tier `S`, use VC5SP3-only named profiles from `tools/_recoil/config/compiler_linker_profiles.json`. Use `vc5_o2_ob0_facs` as the first-pass profile for plain non-EH code, and `vc5_o2_ob1_gx_facs` when EH, constructors, destructors, member cleanup, or inlining evidence matters. Use other VC5SP3 profiles only with local evidence, such as CRT import shape, MFC/DirectX provider requirements, or a focused profile sweep.

Direct hard-byte iteration does not require ChatGPT Pro merely because a body
is large, has loops/x87/EH, or still mismatches. Continue the registered live
compile/compare loop while the source model and next C/C++ experiment are
evidence-backed. Escalate to parent-brokered Pro only when two plausible
source-faithful models remain after direct evidence, the user asks for the
review, or raw inline assembly is proposed after credible VC5SP3 C/C++ variants
fail. Raw assembly always requires that triggered Pro confirmation in addition
to the existing opcode, docblock, allowlist, and byte evidence; Pro never
substitutes for those gates.
Whenever Pro is asked to reason about function byte matching, including every
follow-up or retry, the three highest-value artifacts are mandatory: complete
address-labeled retail assembly from the correct binary/current BN body, the
exact current C/C++ reimplementation with all codegen-relevant macros, inline
helpers, declarations, and types, and complete current compiled assembly from
that exact source under the stated VC5SP3 command/profile. They must have unique
basenames and prompt labels and cover the same address/function or complete
multi-function owner scope and current source/profile. Missing, partial, stale,
truncated, or mismatched artifacts block the request; HLIL, pseudocode,
snippets, facts, diffs, and mismatch counts do not substitute.

The source worker produces and locally validates the triplet, then returns a
session-scoped request id with request kind `hard-byte-raw-assembly`, direct
attachment paths, unique basenames and roles, common owner/address scope and
profile, prompt inputs, local completeness checks, and requested conclusions,
then releases its slot. Workers must never invoke `chatgpt-pro-line` and must
never perform live uploads. The parent broker checks the three attachment roles
and paths against the common scope/profile, uploads all three attachments, runs
the session-global single-flight call, and resumes subscribers with the
transcript and per-file upload results matched to the request id. Resumed
workers may echo those parent-supplied results. Produce a synchronized current
triplet and a new session request id for every follow-up. `source-discovery` and
`hard-byte-raw-assembly` are distinct request kinds and must never share a
prompt, attachment set, or call. Existing invocation
exemptions do not waive the triplet after Pro is invoked. Ask for source-
faithful VC5 C/C++ alternatives first; raw inline assembly or an assembly macro
is considered only if Pro says it is required. Historical conclusions remain
non-gating observations; reopened inquiries and new raw-assembly proposals
require a current triplet.
New compiler environment or flag tuples require updating the provenance ledger/config and passing `python tools/recoil.py audit provenance --strict`. Ad hoc flags are diagnostic/probe evidence only and cannot support accepted tier `S` until promoted into the provenance system.

Authored production VC manifests must compile production source through `source_from`; do not use manifest-local production copies or generated project-header shadows. Manifests may include `functions`, `data_symbols`, or both; `data_symbols` entries need BN address, VC5 COFF symbol, display name, optional BN name, and byte length. Owner-scoped `verify vc5 --owner` refuses to compile until every linked authored function/data row has exactly one manifest item; add or fix coverage before treating a VC5 run as owner byte-gate evidence. Run or trust `python tools/recoil.py guard vc5-manifest` when manifest shape is in question.

Report the compiler profile name, compiler version, flags, provenance audit
status, target architecture, source or manifest, generated symbol, current
`.obj` or linked candidate path, BN address, direct byte/relocation comparison,
first divergence, accepted typed differences, and relocation identity for
data-symbol comparisons. Validation acceptance comes from that current direct
comparison, not from a stored file identity.

Before accepting or preserving data `S` when a final candidate executable exists,
check whether final-build PE comparison or section diagnostics show `.data`
raw-size, virtual-size, zero-fill, or map-placement drift. If so, run
`audit final-data --include-owners`. A strict nonzero result from section deltas
is a final executable byte identity blocker and directly affected owner/data
byte-gate blocker, not a tool failure by itself. Direct owner correlation means
candidate address drift or a missing candidate map symbol; owner-expanded rows
are conservative byte-gate fallout and are not per-symbol content mismatch
proof.

## Metadata And Reporting

Data entry tier, owner data gate, physical storage contribution, PE output
section, and final-image acceptance remain distinct. Unknown extents omit
size/end. Historical observations are non-gating; storage/section acceptance is
explicit, and final acceptance requires a fresh build with complete typed live
comparison of every mandatory section and entity.

Do not infer nested `Model: source-faithful` from tier `S` byte match. `Model:`
records source-shape state, while the owner `source` gate records
standalone/member/parent-owner acceptance. Return independently reviewed
positive metadata/gate/tier evidence to the parent, but do not name or invent a
positive setter: when no registered command exists, the required mutation is a
workspace issue. Use `owner downgrade` only for conservative invalidation and
`owner replace-batch` only for a complete structural replacement. A flattened
single-function implementation is not an accepted owner source gate or
`Model: source-faithful` when current evidence proves an unrecovered
class/source-cluster/table/data owner.

Accept the owner `data` gate only after the owner `source` gate and the complete data-owner packet are accepted. A passing owner-scoped `verify vc5 --owner` data-symbol comparison supports byte and relocation-mask evidence, but the unified tracker and `data_owner_audit.md` still decide tier eligibility.

When many authored owner entries have evidence for a positive gate or tier
change, retain the exact scrutinized evidence packet and report the missing
registered mutation as a workspace issue. Do not substitute retired batch,
gate, tier, or address-metadata setters. `owner downgrade` only removes stale
acceptance; `owner replace-batch` changes complete reviewed structure and does
not manufacture positive evidence or relax owner/data gates.

Raw assembly, naked functions, `_emit`, `.asm` files, or production assembly
stubs invalidate the affected source cluster unless a documented,
address-scoped source-faithful exception satisfies **both** conditions: valid
source-faithful VC5SP3 C/C++ variants failed to byte-match, and every applicable
hard trigger received a compliant synchronized-triplet parent-brokered
`chatgpt-pro-line` pass
that confirms raw inline assembly or a macro is required. The snippet must be
minimal, have BN/VC5 register/FPU/opcode evidence, an immediate/enclosing
Purpose docblock explaining why C/C++ was insufficient, and an address-scoped
`.agent/RAW_ASSEMBLY_ALLOWLIST.txt` row using `source-faithful-inline-asm` or a
narrower existing tag. Pro approval is narrow and proves no owner/source gate,
`Model: source-faithful`, tier `S`, or byte readiness. Provider shims, naked
functions, `_emit`, `.asm`, whole-function assembly, raw stack shells, and order
tricks remain forbidden outside already documented CPU-probe exception classes.

Local `struct ...Dispatch { virtual ... }`, `struct ...Virtual { virtual ... }`,
production `VTable`/`FTable` structs, globals, or slot arrays,
`Make...Vtable`/`Make...Vtbl`/`Make...FTable` factories, and temporary
ABI/source-shape scaffold comments in production source are tier blockers. A
slot call that compiles, smokes, or byte-matches through such a scaffold is not
real reconstructed source. Keep temporary probes outside production source,
delete them before handoff, and verify the BN-proven class/interface first when
the evidence fits; otherwise verify the proven provider boundary,
callback/data system, namespace/source-file owner, subsystem, or record owner.

Subagents never clear or durably depend on `.devspace`; return material
semantic conclusions and direct evidence/transcript paths with their role and
scope. When reporting, name the owner/address/group,
current owner gate/tier state, requested change, evidence, blockers, and exact
commands. If criteria are not met, state the highest justified tier and why.

