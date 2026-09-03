# Recoil Engine Reconstruction

This repository is a source-faithful native C/C++ reconstruction of the
Windows x86 engine for Zipper Interactive's 1999 game **Recoil**. It is a
reconstruction effort, not a modern port.

Success has one exact definition: an unrestricted VC5SP3 compile and link of
the reconstructed source must reproduce every typed, acceptance-relevant
semantic fact in the immutable retail reference `support/Recoil.exe`. This
includes complete function population and order, object and linked bytes,
relocations and targets, data, resources, imports, providers, PE layout, and
all catalogued padding. The linker-written COFF timestamp and the resulting raw
whole-file difference are diagnostics, not acceptance gates. Behavioral
similarity, post-link patching, or a matching subset of the executable do not
satisfy the goal.

The unified tracker coordinates one strictly serial six-stage traversal:

1. `authored-function-order` recovers the natural VC5SP3 order of
   source-authored and authored-lifecycle bodies across retail `.text`.
2. `authored-call-contract` checks the static invocation contract of every
   currently reviewed physical authored gating body in deterministic
   retail-contiguous slices.
3. `authored-byte-match` proves each gating authored contribution at the
   object, relocation, symbolic-target, and relocation-normalized linked-body
   level.
4. `full-function-order` begins only after every authored call contract, its
   fresh closeout, and every authored byte group are current. It restarts at
   the beginning and recovers the complete linked contribution set, retail
   addresses, order, and seams, including compiler, runtime, framework, and
   provider rows.
5. `linked-byte-match` then proves every resolved linked address, relocation
   operand, reference target, and linked byte sequence.
6. `final-validation` derives complete typed final-image coverage live from
   retail plus accepted tracker facts, then checks one fresh unrestricted
   build. No unmodelled byte range may pass. Raw file
   differences and the COFF timestamp are reported only to aid diagnosis.

The source remains deliberately compatible with the recovered late-1990s
Windows compiler, ABI, SDK, MFC, and DirectX boundaries. Modern-language or
modern-library helpers may be used in host-side tools and tests; they are never
substitutes for source-faithful production reconstruction under `src/`.

## Repository Layout

- `src/Battlesport` — reconstructed game shell and gameplay-facing source.
- `src/GameZRecoil` — reconstructed engine subsystems.
- `tools/_recoil/compat/include` — compiler, calling-convention, and ABI
  compatibility bridges used by governed builds.
- `tests/tools` — the compact proof-kernel suite for tracker transactions,
  retail comparison, compiler/linker validation, and policy enforcement.
- `tools` — local reconstruction and verification infrastructure.
- `docs/reconstruction` — durable evidence and reconstruction runbooks.
- `export` — analysis exports used as supporting recovery evidence.

The canonical reproduction runbook is
[`docs/reconstruction/retail_executable_reproduction.md`](docs/reconstruction/retail_executable_reproduction.md).
Physical source-block and semantic-span mapping are internal reconstruction
state; durable placement evidence is documented in
[`source_naming_conventions.md`](docs/reconstruction/source_naming_conventions.md).
The tool-managed unified tracker is the only progress authority.

The public repository is source-only. It does not redistribute the original
game executable, game data, extracted assets, decompiler databases, legacy SDK
payloads, redistributable DLLs, or compiler toolchains.

## Status

Current reconstruction state is read directly from the sole progress authority:

```powershell
python tools/recoil.py progress next --json
```

`.agent/RECONSTRUCTION_PROGRESS.sqlite3` is intentionally machine-local. The
README does not cache or project its changing state.
## License

The reconstructed source, tests, and documentation in this repository are
released under the MIT License; see `LICENSE`.

That license does not grant rights to the original *Recoil* game, retail
executable, game data, extracted resources, third-party SDKs, redistributable
DLLs, decompiler databases, or compiler/toolchain payloads.
