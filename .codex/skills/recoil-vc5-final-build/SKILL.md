---
name: recoil-vc5-final-build
description: Run and diagnose the governed VC5SP3 final candidate build, its compile/link/resource steps, logs, PE checks, and typed final-image validation.
---

# Recoil VC5 Final Build

Use the governed configuration at `tools/_recoil/config/vc5_final_build.json` and run the command from the canonical checkout:

```powershell
python tools/recoil.py verify final-build --help
python tools/recoil.py verify final-build --dry-run
python tools/recoil.py verify final-build
```

Use compile-only or other supported diagnostic modes only to isolate a failing step. Inspect logs under `build/vc5-final` and report the first compiler, linker, resource, or PE divergence with its exact command context.

Do not patch the output, reorder through linker tricks, copy a saved candidate into place, or qualify by hash. The governed unrestricted build and direct typed comparison against `support/Recoil.exe` are authoritative.

After a successful candidate build, run:

```powershell
python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil.py audit final-image-catalog --json
python tools/recoil.py verify final-image --json
```

The linker timestamp and raw whole-file delta remain diagnostic only.
