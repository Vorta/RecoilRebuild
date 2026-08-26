---
name: recoil-source-owner-scrutiny
description: "Scrutinize RecoilRebuild source-owner acceptance. Use when you need to challenge or approve proposed owner source/data/linkage gates, Model: source-faithful, or tier B/A/S promotions before the unified tracker gates are accepted."
---

# Recoil Source Owner Scrutiny

## Global Text Pipeline

Scrutinize only the complete owner needed by the current `progress next` cursor.
An exact live order or byte result is candidate evidence, not automatic owner
acceptance. Do not approve later-owner work ahead of the global prefix and do
not turn deterministic verification into an owner-acceptance shortcut.
The six stages are `authored-function-order`, `authored-call-contract`,
`authored-byte-match`, `full-function-order`, `linked-byte-match`, and
`final-validation`; full order waits for every call-contract slice but not for
authored bytes.

## Core Rule

Start from root `AGENTS.md`; it remains authoritative for owner gate/tier criteria.
This skill is an adversarial review gate for owner acceptance, not a general
verification workflow and not a source implementation workflow.

Use it before accepting any positive source-shape gate:

```powershell
python tools/recoil.py progress show <owner-id-or-address> --json
python tools/recoil.py progress audit --scope owners --strict
python tools/recoil.py progress audit --scope owners --strict --json
```

Run focused `python tools/recoil.py progress show`, `python tools/recoil.py
progress owner relationships`, or `python tools/recoil.py progress owner audit
--strict --json` commands, plus current Binary Ninja caller/callee/xref
inspection, when evidence is needed to disprove the proposed owner.
Do not read the whole unified tracker and do not mutate ledgers in the scrutiny pass.

Follow the source-discovery trigger in `recoil-source-model-recovery`. Run a
ChatGPT Pro reasoning pass through the parent broker only when the scrutiny
must choose between credible competing source models, resolve a materially
disputed cross-owner/block/provider conclusion, escalate to raw assembly after
credible source experiments fail, or honor an explicit user request. A
registered live-order comparison, its first divergence, rechecking an accepted
fact, or reviewing an implementation against an already-reviewed owner model
does not trigger Pro. The scrutinizer returns a session-scoped request id, request
kind `source-discovery`, prompt inputs, the owner/address scope, direct
attachment roles and paths, and requested conclusions, then releases its slot.
It must never invoke `chatgpt-pro-line` or perform a live upload. The parent
checks every attachment role and path against the stated scope, uploads the
attachments, runs the session-global single-flight call, and resumes the
scrutinizer with transcript and per-file upload results matched to the request
id. `source-discovery` and `hard-byte-raw-assembly` must never share a prompt,
attachment set, or call.
Mechanical lookup of already-accepted durable facts and deterministic catalog
derivation are exempt, and raw BN fact packets are exempt only when they do not
recommend ownership or placement. The prompt must be self-contained: address/range, binary target,
current owner/block/order hypothesis, source-path literal xrefs, neighboring BN
function order, assembly/xrefs/calls/data facts, current catalog/docs rows,
proposed included/excluded functions/data, alternative hypotheses,
contradictions, stale-evidence risk, and requested ALLOW/BLOCK-style critique.
Ask Pro to challenge the conclusion and identify missing evidence. Retain the
session request id and ChatGPT Pro transcript, or state the specific exemption
reason.
ChatGPT Pro output is advisory evidence only; it does not prove source ownership,
physical-block tracker changes, owner gates, `Model: source-faithful`, or tier
`S`.
Use the complete prompt schema and exemptions in
`recoil-source-model-recovery`; this skill owns only the final `ALLOW`/`BLOCK`
scrutiny result.

An `ALLOW` semantic result does not imply that a mutation command exists.
Route a complete reviewed structural correction only to dry-run-first `owner
replace-batch`, and a conservative invalidation only to `owner downgrade`.
Positive metadata/gate/tier evidence with no registered mutation route is a
workspace-issue candidate even when the semantic proposal is otherwise
allowed; never name a retired or generic owner operation.

## Scrutiny Standard

Try to disprove the owner before allowing it. Return `BLOCK` unless the proposed
owner is the complete primary source-shaped unit proved by current evidence.
Primary owners are original source constructs: classes/interfaces, source-file
clusters, subsystems, authored callback/record/table/global objects/static
class-member groups, provider boundaries, or true standalone leaves. Ordinary
global/literal/constant/storage groupings are auxiliary data packets; they are
evidence/prerequisite scopes, not primary source-owner targets, unless current
evidence proves the original source had that exact authored data construct.

Reject these shortcuts:

- single-function ownership when class, record, interface, source-file,
  subsystem, lifecycle, callback/data, or recursive-group evidence exists
- source-file owners that are arbitrary slices rather than complete coherent
  source clusters
- source-owner claims that contradict proven physical source-file block order
  from source-path literal xrefs or neighboring BN function order, unless a
  header/provider/COMDAT exception is proven
- folded/shared implementations assigned to one concrete class without evidence
- lifecycle pairs, stack push/pop pairs, init/shutdown pairs, or coupled helpers
  split across owners without BN/source rationale
- accepted owner or data gates inferred only from smokes, ABI shape, byte
  matches, or passing tests
- accepted owner source gate where the promoted address lacks exactly one
  primary unified-tracker owner link to a primary source-shaped owner
- anchor-only or dependency-only relationships used as primary ownership
- auxiliary data packets used as primary tier `S` source-owner targets without
  exact original authored data-construct proof
- data-packet byte acceptance treated as parent/source-owner tier `S`
  completion
- an accepted owner data gate, or `data=none`, without evidence proving the
  complete touched authored data scope or the absence of authored data
- owner/data acceptance inferred from a storage contribution, output-section
  comparison, or final-data correlation; these are distinct evidence entities
- an unknown extent represented with a guessed size/end
- `Model: source-faithful` while production source still uses raw offsets,
  copied tables, VTable/FTable scaffolds, fake provider shims, unsupported
  helpers, or unresolved source-shape blockers

## Required Evidence Packet

Before allowing a promotion, require:

- proposed owner id, kind, state, gates, anchors, primary function/data links,
  source paths, and dependency owners
- affected owner addresses and intended gate/tier changes
- `python tools/recoil.py progress owner relationships` output, with primary ownership separated from anchors
  and dependencies
- `python tools/recoil.py progress owner audit --strict --json` result or a
  blocker explaining why current BN membership evidence is unavailable
- current BN/source facts proving included members/data and explaining excluded
  adjacent/coupled functions
- data-owner facts for `data=accepted` or no-data proof for `data=none`
- for tier `S`, proof that the target is the primary source-shaped owner and
  that all referenced/touched/linked auxiliary data packets are byte-ready, or
  proof that the data packet itself is the exact original authored construct

## Return Packet

Return only a Source Owner Scrutiny Packet:

- decision: `ALLOW` or `BLOCK`
- owner id and affected addresses/data rows
- exact accepted gate/tier changes if allowed
- exact blockers if blocked, including stale or contradictory evidence
- commands and evidence reviewed
- ChatGPT Pro session request id and transcript when a source-discovery trigger
  actually applied, or the exact exemption reason
- facts that would stale the decision

Do not edit source, `.agent/RECONSTRUCTION_PROGRESS.sqlite3`, owner gates/tiers, manifests, docs,
tools, Binary Ninja state, workspace issues, or version-control state.
Subagents never clear or durably depend on `.devspace`; return material
semantic conclusions and direct evidence/transcript paths with their role and
scope.
