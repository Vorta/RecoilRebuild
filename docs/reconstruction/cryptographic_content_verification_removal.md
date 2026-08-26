# Historical removal of cryptographic content verification

> Historical migration note — not an active verification procedure, command
> reference, scheduler, acceptance rule, or source of current tracker state.

Workspace issue `WSI-20260826-001`, linked to `WSI-20260825-002`, removed the
unactivated cryptographic-content currentness design before it became live
authority. The retired design included certificate envelopes and body leaves,
SHA-256 fields, `hashlib` calls, `digest`/`hexdigest` operations, Merkle roots,
content/tree hashes, call fingerprints, normalizer/verifier content identities,
and scheduler output-cache keys. Historical diagnostic output does not become
current evidence and is not compatibility input.

The active replacement is deliberately direct:

- a parent call-contract acceptance invocation authenticates one active packet
  and physical output root, performs one fresh build, reads expected facts
  directly from retail and the governed Binary Ninja saved view, and accepts
  only bodies that directly pass in that invocation;
- call-contract currency uses governed mutation, explicit conservative
  invalidation, and the reviewed integer coordinates
  `CALL_CONTRACT_VERIFIER_GENERATION = 5`,
  `NORMALIZER_REGISTRY_GENERATION = 5`, and
  `EXPECTED_FACT_SCHEMA_VERSION = 5`;
- phase transition requires a fresh complete no-reuse zero-divergence scan;
- tracked workspace change control uses a clean reviewed branch, an opaque
  baseline commit, and native Git status/diff state;
- database currentness and no-mutation evidence uses SQLite revisions, CAS,
  schema/user version, exact row counts, transaction rollback checks, and
  `PRAGMA integrity_check`;
- retail, candidate, output-root, toolchain, and BN identity use stable physical
  identities, explicit versions/generations, and direct structured or byte
  comparison. The four-byte PE/COFF `TimeDateStamp` is nonsemantic and is not
  forced to its historical value.

## Exact retained terminology allowances

These are enumerated exceptions, not a pattern-based suppression:

1. `tools/_recoil/commands/live_validation_surface_audit.py` contains the exact
   forbidden-token detector source (`hashlib`, SHA, MD5, BLAKE, `digest`,
   `hexdigest`, Merkle, content hash, tree hash, and fingerprint spellings).
   Those literals make the strict audit reject operational use; that module
   performs none of the forbidden operations.
2. `tools/_recoil/commands/live_final_verify.py` and
   `tools/_recoil/lib/pe.py` may use the PE data-directory name `certificate`
   for the Authenticode certificate directory. This is a standard PE structure
   name and is not the removed call-contract certificate architecture.
3. Active Git documentation may state negatively that Git's native object IDs
   are opaque repository state. Reconstruction tooling does not create,
   interpret, or compare them as retail truth or candidate acceptance.
4. This file alone may name the retired mechanisms for historical explanation
   and audit review. It supplies no compatibility behavior, expected fact,
   current evidence, or acceptance route.

No third-party/provider exception is currently required by project-owned
active tooling. Any future occurrence must be separately enumerated by exact
path and role; generic directory, suffix, or token suppression is forbidden.
