# Recoil Engine Reconstruction

This repository is a source-faithful native C/C++ reconstruction of the
Windows x86 engine for Zipper Interactive's 1999 game **Recoil**. It is a
reconstruction effort, not a modern port.

Success has one exact definition: an unrestricted VC5SP3 compile and link of
the reconstructed source must produce a `Recoil.exe` whose complete bytes and
whole-file SHA-256 are identical to the immutable retail reference
`support/Recoil.exe`. Behavioral similarity, selected function matches,
normalized PE comparisons, post-link patching, or a matching subset of the
executable do not satisfy that goal.

The work proceeds globally and sequentially:

1. Recover the exact retail `.text` function and contribution set and natural
   VC5SP3 order, from the first retail function to the last, stopping at the
   first divergence.
2. Restart at the first function and match each object contribution,
   relocation, linked address, reference target, and linked byte sequence.
3. Validate one unrestricted final build, including sections, data, resources,
   imports, provider selections, addresses, and the whole-file SHA-256.

The source remains deliberately compatible with the recovered late-1990s
Windows compiler, ABI, SDK, MFC, and DirectX boundaries. Modern-language or
modern-library helpers may be used in host-side tools and tests; they are never
substitutes for source-faithful production reconstruction under `src/`.

## Repository Layout

- `src/Battlesport` — reconstructed game shell and gameplay-facing source.
- `src/GameZRecoil` — reconstructed engine subsystems.
- `src/native` — native target wiring and include bridges.
- `tests/native` and `tests/tools` — focused behavior, ABI, guard, and tooling
  verification.
- `tools` — local reconstruction and verification infrastructure.
- `docs/reconstruction` — durable evidence and reconstruction runbooks.
- `export` — analysis snapshots used as supporting recovery evidence.

The canonical reproduction runbook is
[`docs/reconstruction/retail_executable_reproduction.md`](docs/reconstruction/retail_executable_reproduction.md).
Physical source-block and semantic-span mapping are internal reconstruction
state; durable placement evidence is documented in
[`source_file_layout_audit.md`](docs/reconstruction/source_file_layout_audit.md).
The tool-managed unified tracker is the only progress authority.

The public repository is source-only. It does not redistribute the original
game executable, game data, extracted assets, decompiler databases, legacy SDK
payloads, redistributable DLLs, or compiler toolchains.

## Status

Reconstruction is ongoing. Some subsystems have source-level implementations
and focused tests, while others retain unresolved source-shape, provider,
ordering, linkage, or byte-identity work. Live counts, the global cursor,
blocker, and next command are intentionally not copied into this visitor-facing
README because they would quickly become stale; operational reports are
generated on demand from `.agent/RECONSTRUCTION_PROGRESS.json`.

## License

The reconstructed source, tests, and documentation in this repository are
released under the MIT License; see `LICENSE`.

That license does not grant rights to the original *Recoil* game, retail
executable, game data, extracted resources, third-party SDKs, redistributable
DLLs, decompiler databases, or compiler/toolchain payloads.
