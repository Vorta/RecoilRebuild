# Reconstruction status and verification

The project reconstructs the 1999 Windows x86 program using native C/C++ and the Visual C++ 5.0 SP3 compiler context. Its long-term target is a complete typed comparison with the retail executable, including code, data, dependencies, resources, and layout.

## Different kinds of progress

| Dimension | What it establishes |
| --- | --- |
| Source recovery | A source representation and an evidence-backed interpretation of its types and ownership. |
| Authored census | The expected authored identities are represented in their source/compile context. |
| Call contracts | Calls use the expected targets, arguments, receiver relationships, and calling conventions. |
| Authored byte matching | A scoped compiled function or group agrees with the relevant retail bytes and typed relocation/target facts. |
| Linked order and bytes | The corresponding bodies have the required relationships and representation in the linked program. |
| Final-image verification | The complete executable satisfies the required typed code, data, provider, resource, and layout comparisons. |

These dimensions are related but are not interchangeable. A successful compile or link does not prove an executable match. A matching function does not prove that all of its containing subsystem is complete. Physical function groups, logical bodies, and source blocks are different counting units and must not be combined into an overall percentage.

## Published snapshots

The README carries a dated summary of the maintained reconstruction tracker. That summary is informational; the canonical tracker and fresh governed comparisons remain the operational authorities. Snapshots should identify their units and any unresolved validation instead of treating old source annotations as current proof.

The 22 September 2026 snapshot records tracker transaction revision **8371** (semantic and evidence-generation revisions **8368**). Its selected census task covers the HUD block beginning at `0x404CA0`. The accompanying fresh main-executable build compiled 92 translation units and completed resource compilation and linking, with compiler warnings. It did not perform linked-order, byte-match, final-image, or gameplay acceptance.

The annotation audit found 915 retained historical function-match annotations and no annotation-consistency findings. All 915 required fresh verification in the current input context. A freshness requirement is not an observed byte mismatch.

The repository does not currently claim complete retail-executable equivalence or validated end-to-end playability. The source is published so that the reconstruction can be studied while work continues.
