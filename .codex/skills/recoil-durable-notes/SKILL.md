---
name: recoil-durable-notes
description: Record durable Recoil reconstruction evidence in the correct source, tracker, Binary Ninja, test, or focused documentation location without duplicating live progress.
---

# Recoil Durable Notes

Choose the smallest authoritative home for the fact:

- Use governed tracker mutations for owner, relationship, gate, tier, provider, classification, order, call-contract, byte, storage, and final-image state.
- Use production-source docblocks for source-to-artifact traceability and the source-level purpose of the attached construct.
- Use focused `docs/reconstruction/` notes for durable cross-cutting reasoning that is not represented by a tracker fact.
- Use tests or manifests for executable expectations.
- Use Binary Ninja comments only for analysis navigation; they are not acceptance evidence.
- Use `.agent/WORKSPACE_ISSUES.sqlite3` only for reproducible tool, environment, validation, or rule defects.

Do not create progress diaries, duplicate tracker snapshots, address ledgers, status scratchpads, packet notes, or handoff documents. The root README progress block is generated; never edit it by hand.

For source traceability, attach one stable anchor directly to a real source construct and use one artifact row per emitted object:

```cpp
/**
 * @recoil-anchor recoil:anchor:<stable-source-construct-id>
 * @recoil-artifact defines .text recoil:function:0xNNNNNN: Primary authored body.
 * Purpose: Explains the source-level role of this construct.
 */
```

Use governed source-trace commands to update source mirrors and tracker relationships together. If ownership, placement, section, extent, aliasing, or emission cause remains ambiguous, record no positive source claim.
