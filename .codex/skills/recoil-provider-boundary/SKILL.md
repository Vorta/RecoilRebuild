---
name: recoil-provider-boundary
description: Classify Recoil functions, data, thunks, imports, runtime wrappers, compiler helpers, and framework artifacts as authored, lifecycle-authored, provider-supplied, or unresolved.
---

# Recoil Provider Boundary

Classify from retail evidence, not appearance. Inspect the exact body or data, inbound and outbound xrefs, imports, neighboring order, vtable or message-map membership, literals, calling convention, cleanup, and any compiler-generated relationships.

Use these outcomes:

- `authored`: a deliberate game/engine source construct.
- `authored-lifecycle`: an authored class lifecycle contribution, while compiler-generated variants remain separately inventoried.
- `non-authored` or provider boundary: CRT, MFC, DirectX, COM, compiler/runtime, import, library, or other supplied implementation.
- `unresolved`: evidence does not yet justify a boundary.

Tiny bodies, thunks, wrappers, EH helpers, and familiar runtime shapes are not automatically provider-owned. An override, callback, message handler, or lifecycle body may be authored even when it resembles framework glue. Conversely, source convenience wrappers do not convert provider code into authored ownership.

Record the exact identity and supporting evidence. Provider artifacts receive no authored tier or production-source edge. A boundary decision does not accept order, bytes, data, linkage, or final-image coverage. Apply reviewed tracker changes dry-run first, then with `--apply`.

For a reviewed source struct rename in an existing canonical-header provider,
`progress provider-function register` accepts optional `logical_type_renames`
pairs with exact `from` and `to` C++ identifiers. Retain the primary provider,
owner, header, recipe, extent and unknown physical winner. The requested logical
symbol list must equal the complete existing ordered list after those struct
spelling substitutions; additions, removals, chaining and primary replacement
are forbidden. Every resulting specialization must have one registered probe
and pass a fresh canonical VC5 compilation and direct retail comparison before
the revision-guarded mutation. Dry-run, review, then apply. This refresh changes
no authored ownership or stage acceptance. Use `recoil-validation` for tool
changes and resume the serial scheduler afterward.
