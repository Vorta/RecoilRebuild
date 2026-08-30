---
name: recoil-functional-targets
description: Create, inspect, register, and run Recoil functional verification targets and interpret what their behavioral evidence can justify.
---

# Recoil Functional Targets

Functional targets live under `tools/functional_verify_targets/` and run through `tools/recoil.py`. Inspect an existing nearby manifest before adding a target, choose a stable target id, and keep the target bounded to behavior that can be reproduced deterministically.

Common commands:

```powershell
python tools/recoil.py verify functional --list
python tools/recoil.py verify functional <target-id>
python tools/recoil.py progress verification-target sync --target <target-id> --expected-revision <revision> --dry-run --json
```

Review a registration dry-run before repeating that exact targeted command with `--apply`. Run the exact functional target after changing its source, manifest, registration, or smoke. If `recoil_native_smoke` is missing, build it through the repository's governed native build path rather than treating absence as a behavioral result.

A passing target can support behavior/source coverage for tier C. It does not establish source ownership, source shape, data or linkage gates, provider classification, function order, call contracts, object or linked bytes, or tier B/A/S. Promotion still requires the corresponding owner and tier reviews.

Do not add a smoke solely to manufacture an acceptance marker. The target should state the observable contract, inputs, expected outputs, and relevant original-source helper/docblock requirements.
