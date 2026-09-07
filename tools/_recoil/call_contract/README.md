# Recoil call-contract proof ownership

This package implements the call-contract stage for this reconstruction on this
machine. The reviewed Recoil patterns are intentionally specific. Keeping the
proof boundary inspectable is the design constraint; reuse is not a goal.

`commands/call_contract_verify.py` is the public CLI entry point. It delegates
only parser construction and execution to `cli.py`. Other tools import their
actual evidence owner below. There is no monolithic compatibility namespace.

| Owner | Responsibility |
| --- | --- |
| `cli.py`, `reporting.py` | Public arguments, result envelope, slice projection, diagnostics. |
| `session.py`, `work.py`, `phase_*.py` | One invocation and one fresh per-caller construction record; nine explicit serial proof phases. |
| `records.py`, `catalog.py` | Immutable evidence records and reviewed Recoil selection/pattern data. The catalog has no proof functions. |
| `instructions.py`, `cfg.py`, `flow.py` | Exact x86 decoding and explicit/implicit effects, local control-flow validation, and shared finite must-reaching definitions. |
| `retail.py`, `retail_imports.py`, `identity.py` | Target-qualified current BN facts, immutable retail identities, accepted typed identity bindings. |
| `source.py`, `candidate.py`, `candidate_session.py`, `listing.py` | Current source discovery, governed fresh compilation, COD/COFF provenance, and complete instruction boundaries. |
| `iat.py`, `dispatch.py`, `candidate_imports.py` | Retail/candidate import identity and exact load-to-transfer register lineage. |
| `receiver_*.py`, `storage_identity.py` | Receiver, stack, field, callback, vptr, slot, constructor, and bounded complete-body equivalence premises. |
| `providers.py`, `abi.py`, `callable_identity.py`, `logical_identity.py` | Independent provider/ABI identities, native helper bodies, and accepted physical/logical identities. |
| `recoil_*.py`, `reviewed_dispatch.py`, `lifecycle.py`, `callbacks.py` | Specific Recoil proof producers. A profile selects a proof; current retail and candidate evidence must satisfy its premises. |
| `contributions.py`, `proofs.py`, `comparison.py` | Physical invocation accounting, immutable obligations/results, contradiction-preserving composition, and selected dependency comparisons. |

`lib/call_argument_bits.py` owns the selected argument bit algebra and consumes
the same exact instruction effects. `lib/implicit_copy_members.py` and the VC5
driver own the native implicit-copy source route; they are not call-contract
acceptance shortcuts.

## Evidence flow

1. Acquire the current census and accepted identities, authenticate the selected
   retail inputs, and compile the current source in a fresh governed root.
2. Recover retail storage and receiver premises independently. Selected
   constructor, provider argument, and return dependencies are derived from
   retail, never from a candidate value at the matching call ordinal.
3. Recover candidate IAT/ABI, provider, dispatch, storage, receiver, and lifecycle
   premises from its actual COD, COFF, relocations, and typed identities.
4. Freeze comparable claims as `Fact` values. Only retail facts may construct
   an `Obligation`. `prove` returns one immutable `ProofResult` with status
   `proven`, `not-applicable`, `unresolved`, or `conflict`.
5. Preserve physical invocation contributions through projections, compare all
   required obligations, and report the first divergence plus per-body details.
   Tracker mutation belongs to the public progress commands.

`CallerWork` is mutable orchestration state, never an acceptance fact. Every
caller gets a new instance. Evidence producers compose mappings with
`merge_into`/`checked_merge`: identical conclusions may agree, but a later
producer cannot overwrite a conflicting conclusion. A refinement needs its
own premises before publication.

The byte decoder is pinned in `tools/requirements.txt`. Text labels do not
override its instruction effects. CFG joins require the relevant definition
on every incoming path; unsupported or ambiguous paths remain unresolved.
The lightweight receiver interpreter's `stack` marker denotes the current
stack root. Proof of a specific entry-stack slot belongs to the CFG lineage
checks, which account for stack movement and known cleanup.

Provider equivalence requires independently established identity and complete
native body/relocation evidence. Identity equivalence cannot remove a physical
call, turn a call into a tail, or erase indirect dispatch. A complete-body
equivalence proof is bounded by exact instruction coverage and independently
bound relocation targets; candidate bytes cannot supply retail expectations.

## Maintenance and assurance

`lib/call_contract_generations.py` is the sole generation authority and lists
every Python module in this package. Adding an unregistered component fails
the live-validation surface audit. Governed edits advance the owning
coordinate and invalidate affected evidence conservatively.

Keep new Recoil selection data in the relevant catalog or producer, and put a
new inference in its evidence owner. The retained proof-kernel tests cover
infrastructure premises and rejection cases, not one test per retail function.
Use the canonical `recoil-validation` skill for validation selection.

A passing call contract covers its named invocation and selected dependency
dimensions. It does not establish general behavior, source ownership, all data
flow, complete constructor side effects, or the later byte/final-image gates.
Fresh diagnostic census outputs help review a tool change; they do not advance
or qualify reconstruction state.
