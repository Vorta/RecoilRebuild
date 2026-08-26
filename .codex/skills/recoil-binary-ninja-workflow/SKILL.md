---
name: recoil-binary-ninja-workflow
description: Inspect the already-open Recoil Binary Ninja database safely and read-only. Use when you need current names, types, layouts, globals, xrefs, assembly, function order, HLIL caveats, BN-backed status checks, or a fact packet without changing Binary Ninja state.
---

# Recoil Binary Ninja Read-Only Workflow

Root `AGENTS.md` is authoritative; this skill adds only read-only Binary Ninja
evidence and safety rules.

This skill owns read-only evidence and safety only. All names/types/signatures/
variables/comments/properties edits, reanalysis, and database saves belong
exclusively to a parent-assigned `recoil-binary-ninja-reconstruction` workflow
executed by `recoil_bn_reconstructor`.

Binary Ninja is a maintained analysis artifact, not an immutable input. New
active-scope evidence is enough for the parent to assign a bounded correction
without separate user approval or a tracker mutation. Editing, reanalysis, and
saving the already-open database belong exclusively to the reconstructor; its
read-only filesystem sandbox does not make assigned BN MCP state immutable. It may not
load/switch/patch binaries or decide owner/block/order/provider/tier acceptance.

With no explicit target, `python tools/recoil.py progress next` selects the
Recoil.exe cursor. Inspect only that cursor/required dependency or the user's
explicit target; ordinary `messages.dll` work is deferred.
The six stages are `authored-function-order`, `authored-call-contract`,
`authored-byte-match`, `full-function-order`, `linked-byte-match`, and
`final-validation`. Full order begins when all call-contract slices are current
without waiting for authored bytes; linked bytes require both full order and
authored bytes.

## Safety

- Acquire and report a read reservation/lease id for the named database before
  inspection. Read leases may overlap only against the same stable saved view
  and only when no writer lease is active. Once a writer is reserved, no reader
  may start or continue until the exclusive writer completes reanalysis,
  propagation checks, saves the database, returns its packet, and releases the
  writer lease.
- Work only in the already-open Binary Ninja 5.2 database exposed by MCP.
- Check bridge identity and tool availability before fact gathering.
- Never select/switch/load a binary, patch bytes, invoke an external
  disassembler, or use shell-driven reverse-engineering tools.
- Never edit, reanalyze, or save BN state under this skill.
- Assembly and xrefs are primary. HLIL/decompilation is derivative and must be
  labeled with type/signature caveats.

Use target-qualified preflight when needed:

```powershell
python tools/recoil.py binja preflight --binary recoil --strict
python tools/recoil.py binja preflight --binary messages --strict
```

## Evidence Packet

For an authored-call-contract `call-contract-retail-fact-v1` handoff, use only
the exact descriptor-derived target, caller symbols/addresses, physical blocks,
and blocker messages in `retail_fact_scope`. The packet contains at most eight
retail callers and must hold an active packet-specific lane reservation plus a
`binary-ninja-db:Recoil.bndb` read claim. Run its target-qualified preflight,
then gather complete address-labeled assembly, xrefs, indirect register/storage
provenance, direct/import/provider navigation facts, and analysis caveats for
every listed blocker. Return an explicit result or unresolved/truncated status
for every row and direct evidence/transcript paths. Do not broaden to a source
repair, dependent owner/header route, unlisted caller, or inferred identity.
The packet is read-only and nonaccepting; candidate output never supplies its
expected truth.

For the assigned target/phase/cursor/range/frontier relation, inspect exact
function boundaries and order, assembly, prototypes/calling conventions,
callers/callees, imports/provider references, globals/data extents, tables,
xrefs, source-path literals, neighboring functions, and analysis caveats.
Treat BN names/comments as provisional navigation labels.

For byte-match evidence, this workflow may produce only the complete
address-labeled retail-assembly member of the synchronized triplet, with binary,
exact body extent, completeness, and extraction caveats. The source worker owns
exact-current-source and compiled-assembly production plus local validation;
the parent checks direct attachment roles and scope, owns upload, and runs the
session-global single-flight Pro call; the verifier validates the complete
triplet and parent results. Refer to `recoil-tier-verification` for the
acceptance policy.

Raw fact packets do not decide source owner, source block, provider/data class,
function-order acceptance, source model, tier, or final acceptance. If facts
show BN state needs correction, return an exact reconstruction request for the
parent to assign through `recoil-binary-ninja-reconstruction`.

Subagents never clear or durably depend on `.devspace`; return material
semantic conclusions and direct evidence/transcript paths with their role and
scope. Do not run git commands or mutate source,
tools, docs, ledgers, manifests, or build state.
