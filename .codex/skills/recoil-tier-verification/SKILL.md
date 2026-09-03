---
name: recoil-tier-verification
description: Verify Recoil authored-owner and provider-boundary evidence for behavior, source, data, linkage, near-byte, exact-byte, helper, ABI, and tier eligibility.
---

# Recoil Tier Verification

Evaluate the complete owner, not a convenient subset. Provider-boundary owners do not receive authored tiers.

Authored tiers are cumulative:

- `C`: reviewed behavior and source coverage on every primary entry.
- `B`: accepted owner boundary plus source, data, and linkage gates.
- `A`: reviewed near-byte evidence for the complete owner.
- `S`: exact owner-scoped byte and provider-ABI evidence.

The owner tier is the floor of all primary entries and required gates. Passing behavior, correct ABI shape, one exact function, order validation, or a data result never silently promotes the complete owner.

Inspect owner membership, source model, traceability docblocks, touched globals and extents, table/callback/helper provenance, provider boundaries, raw object evidence, relocations, linked presence, exact RVAs and operands where applicable, and VC5-generated lifecycle or helper artifacts. Tier C is derived from primary-entry coverage; it has no separate test-manifest gate.

Raw assembly is exception-only. First exhaust credible source-faithful VC5SP3 C/C++ variants. Then use ChatGPT Pro directly for the required advisory critique. Any allowed inline block must be minimal, backed by exact opcode/register/FPU evidence, documented by an immediate `Purpose:` block, and covered by the address-scoped raw-assembly allowlist. Naked functions, `_emit`, `.asm` files, whole-function assembly, stack shells, and order tricks remain forbidden outside documented CPU-probe exceptions.

For a promotion, state every satisfied gate and any floor imposed by a weaker member. Use source-owner scrutiny for positive B/A/S acceptance. Apply the exact reviewed tracker mutation dry-run first; use the conservative downgrade route when evidence regresses.
