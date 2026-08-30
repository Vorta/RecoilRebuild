---
name: recoil-source-owner-scrutiny
description: Challenge proposed Recoil source-owner, source-shape, data, linkage, gate, and tier-B-or-higher acceptance before applying it to the tracker.
---

# Recoil Source Owner Scrutiny

Use this for a positive owner/source/data/linkage gate, `Model: source-faithful`, or tier B/A/S proposal. Review the proposed mutation and its evidence independently of implementation convenience.

Require:

- a complete and non-overlapping owner boundary;
- evidence for every primary member and any logical alias or ICF relationship;
- natural source placement and higher-order source shape;
- explicit treatment of globals, tables, callbacks, helpers, lifecycle variants, and provider artifacts;
- source-to-artifact anchors attached to real constructs where required;
- no raw-offset, vtable-array, ABI-scaffold, wrong-file-helper, order-trick, or post-link workaround;
- distinct evidence for source, data, linkage, order, byte, provider, and tier dimensions.

Passing behavior or one matching body is not enough. Unknown extent, ownership, placement, emission cause, provider identity, or aliasing stays unresolved and blocks the positive gate it affects.

Return `ALLOW` only for the exact proposed scope and dimension, with the evidence that closes each gate. Otherwise return `BLOCK` with the first concrete missing or contradictory fact. Apply tracker mutations only through the governed dry-run-first command after scrutiny.
