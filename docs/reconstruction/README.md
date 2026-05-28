# Reconstruction Documentation

Use this directory for durable facts that save future reconstruction time across
source files, subsystems, providers, or verification targets. Keep implementation
and verification moving; documentation is support work, not a separate deliverable.

Prefer source comments for facts that are local to one function, class, layout, or
call site. Put the note beside the code that depends on it, with the original
address and evidence source when that fact affects ABI or code generation.

When finishing reimplemented code, class/source-cluster work, or verification
evidence, make a documentation decision before handoff: add a compact durable
note here, add a local source comment, or state that no durable new
documentation was needed. Document facts that prevent rediscovery, not routine
progress.

Add or update a reconstruction document when a recovered fact is likely to be
rediscovered by multiple agents, for example:

- compiler, linker, runtime, SDK, MFC, DirectX, or import-provider contracts
- shared class layouts, vtables, message maps, globals, ownership, or cleanup
  patterns
- repeated inlined helper or class-method bodies that should be restored in
  source even though no standalone executable function exists
- file formats, resource formats, serialized structures, or asset naming
  conventions
- repeated Binary Ninja limitations, bridge/toolchain limits, or accepted
  verification boundaries
- source-file mapping evidence that affects where future functions should live

Keep entries compact:

- name original addresses, symbols, and files when known
- state the evidence source, such as Binary Ninja, `support/Recoil.exe`, VC
  output, tests, or a repo-local SDK/MFC source mirror
- separate recovered facts from open limits
- avoid broad progress notes, speculative narrative, and duplicated plan state

Useful section pattern:

```text
## Topic

Evidence:
- ...

Recovered contract:
- ...

Verification notes:
- ...

Open limits:
- ...
```

Current durable reconstruction notes:

- `knowledge_index.md` is the entry point for current durable ledgers.
- `agent_launch_checklist.md` is the compact preflight and handoff checklist
  for starting reconstruction agents.
- `compiler_linker_provenance.md` records the compiler/linker/provider evidence
  policy and the audit command that guards it.
- `inlined_helpers.md` records likely original helpers and methods that were
  fully inlined by the retail compiler and must be verified through callers or
  source/class clusters.
- `original_class_candidates.md` is a generated candidate map for likely
  original class-like systems, records, vtables/function tables, and namespace
  boundaries.
- `provider_abi_notes.md` summarizes workspace-level provider and ABI
  assumptions.
- `source_file_map.md` maps recovered original source paths to current repo
  files from provenance comments.
- `visual_studio_mcp_workflow.md` records the preferred Visual Studio MCP
  development workflow for generated `vs-x86` projects.
