---
name: recoil-durable-notes
description: Capture durable RecoilRebuild reconstruction notes. Use when you need to decide whether evidence should be recorded, choose between source comments, docs/reconstruction notes, unified progress-tracker evidence, Binary Ninja comments, tests, or structured work items, update durable knowledge notes, avoid README/progress-note clutter, or state the documentation decision before handoff.
---

# Recoil Durable Notes

## Global Text Pipeline

Durable handoff records the current `progress next` phase/primary cursor,
`parallel_authored_byte_cursor` only when returned, and subordinate
`parallel_authored_object_byte_cursor` only when returned, plus the phase-
specific accepted prefix, first unresolved item, exact failed/next command,
direct evidence/transcript paths with role/scope, and limits. Object-byte work is
evidence preparation only, not a phase or peer scheduler. After every object-
only packet closes, recompute `python tools/recoil.py progress next`; do not
carry or automatically prepare the next object row.
The primary cursor may be order or call-contract work; claim priority is the
active primary lane, full authored byte, then subordinate object byte. The six
stages remain `authored-function-order`, `authored-call-contract`,
`authored-byte-match`, `full-function-order`, `linked-byte-match`, and
`final-validation`.
Never leave material order/byte conclusions only in chat or `.devspace`; a
truthful recorded blocker is backlog, while stale/conflicting frontier state is
session-introduced debt.

Start from root `AGENTS.md`; this skill only chooses where durable evidence belongs.

Use this skill when finishing or handing off RecoilRebuild reconstruction, source-model recovery, provider/classification work, verification evidence, or structured work-item cleanup. The goal is to preserve facts that save future reconstruction time without turning docs into progress logs.

## Note Threshold

Record a durable note when a stable fact would prevent rediscovery or protect a source/ABI decision. Good candidates include:

- Layout, offset, cleanup-order, vtable/function-table, message-map, or provider-boundary reasoning.
- Magic constants, file-format facts, initialized global data, source-file placement evidence, or original-era source analogies.
- Binary Ninja limitations, toolchain quirks, VC5SP3 byte-diff limits, provider ABI assumptions, or verification-sensitive source shape.
- Repeated patterns across callers, classes, subsystems, or verification targets.
- Evidence that a non-address helper is recovered original inline/static/member source, including observed caller addresses, why no standalone retail function exists, and the repeated pattern/source-cluster basis.

Do not record routine progress, obvious narration, duplicate callsite commentary, speculative conclusions, stale temporary state, or owner gate/tier state that belongs in `.agent/RECONSTRUCTION_PROGRESS.sqlite3`.

## Choose The Destination

Prefer the narrowest durable location that future agents will naturally check:

- Source comments: facts local to one function, class, layout, call site, cleanup order, constant, provider assumption, table identity, Binary Ninja limitation, or verification-sensitive code shape. For new or touched address-backed definitions, use the required immediate `/** ... */` canonical `@recoil-anchor`/`@recoil-artifact` mirror and `Purpose:` docblock. Do not add a source artifact row for an unresolved or provider-owned mapping. A deliberate standalone construct/friendly-BN symbol title is permitted, punctuated or unpunctuated, but grants no evidence, artifact identity, source edge, durable fact, or acceptance. Standalone source paths, symbol-plus-path rows, routing placeholders, repeated prose rows, and lifecycle-contribution labels remain non-notes: identity belongs in the canonical artifact row, while prose explains purpose or material evidence. Construct-only deletion is reserved for explicit reviewed legacy `Reimplements` migration.
- `docs/reconstruction/`: cross-file, subsystem, provider/ABI, file-format, compiler/linker, repeated pattern, class-boundary, source-placement, or inlined-helper facts.
- Existing reconstruction notes first: check `docs/reconstruction/knowledge_index.md`, then update the narrow existing document when one fits.
- `.agent/RECONSTRUCTION_PROGRESS.sqlite3`: durable higher-order owner ids, member/data
  relationships, gates, entry tiers, target/model/data metadata, blockers, and
  evidence state only. Use `python tools/recoil.py progress` through
  `recoil-progress-tracker`; never hand-edit, add narrative notes, or duplicate
  these facts in shadow trackers.
- Structured tracker work items: temporary active dependency closures only.
  Move durable facts elsewhere before closing completed work.
- Binary Ninja comments: current database reconstruction facts that improve decompilation or future binary inspection.
- Tests: executable expectations when a behavior fact should be guarded rather than described.

Never put agent workflow details, ignored-path references, local tooling notes,
hand-maintained progress tables, or reconstruction evidence ledgers in the
root `README.md`. The one exception is the bounded marker-managed public
snapshot maintained by `python tools/recoil.py docs readme-progress`; it is a
deterministic projection of the unified tracker, not an evidence destination
or a second progress authority.

Repository-local `.devspace` is temporary session scratch, never a durable
destination. Before parent-only cleanup, promote material ChatGPT Pro evidence
to the narrow durable surface: conclusion, limits, date, scope/address,
advisory disposition, session-scoped request id, transcript path, and per-file
upload results. For byte-match work record each attachment's role/path, common
function/owner scope, VC5SP3 profile, and triplet completeness. Do not archive
routine full transcripts, prompts, or uploads. Reopened inquiries regenerate
current artifacts and reattach the synchronized triplet. Durable
notes may describe `.devspace` policy with placeholders, but must not depend on
a concrete session artifact remaining present.
Subagents never clear `.devspace`; they return material semantic conclusions
and direct evidence/transcript paths with their role and scope to the parent.

## Writing Durable Notes

Keep data symbols/owner gates, physical storage contributions, PE output
sections, and final-image state distinct in tracker evidence. Unknown extents
omit size/end. Final-data reports are diagnostic observed
evidence, never durable work units, schedulers, owner-action batches, or
implicit acceptance.

Keep notes compact and evidence-oriented:

- Include original addresses and symbols when known.
- State the evidence source: Binary Ninja assembly/xrefs/types, source comments, VC verification, functional target, provider docs, PE/reference facts, or test output.
- For inline/static helpers without a standalone address, state observed caller addresses, the original-helper evidence basis, and whether the fact belongs in `docs/reconstruction/inlined_helpers.md` because it recurs across callers or a class/source cluster.
- Separate recovered facts from open limits.
- Prefer bullets over narrative.
- Link or name the verification target or command only when it helps reproduce the evidence.

Avoid broad summaries that duplicate the unified tracker. If a detail is already obvious from nearby code or owner metadata, do not repeat it.

## Handoff Decision

Before a final handoff, make one documentation decision and report it:

- Durable facts were captured in source comments.
- Durable facts were captured under `docs/reconstruction/`.
- Structured work-item notes were pruned or left active with a reason.
- No durable new documentation was needed.

Documentation should not block implementation or verification unless missing durable context would cause likely rework or unsafe owner gate/tier updates.

