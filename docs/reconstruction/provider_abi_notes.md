# Provider And ABI Notes

These notes summarize workspace-level provider assumptions for source-faithful
Recoil reconstruction. They do not override Binary Ninja, the plan, or
per-target VC verification evidence.

## Evidence

- `support/Recoil.exe` is the private immutable retail reference for PE,
  imports, resources, and final candidate comparison.
- `support/sdk/DirectX_Aug2007` supplies repo-local DirectDraw, Direct3D
  Immediate Mode, DirectSound, DirectInput, DirectPlay, and `dxguid` headers and
  x86 libraries.
- `support/sdk/MFC42` supplies repo-local MFC42 headers, libraries, runtime DLL,
  and selected source evidence for MFC shell ABI work.
- `tools/compiler_linker_profiles.json` records accepted compiler flag profiles;
  `tools/recoil_provenance_audit.py --strict` guards VC verification manifests
  against those profiles.

## Recovered Contract

- Default binary-safe compiler evidence is VC6 `cl` 12.00.8168 with 32-bit x86
  code generation unless Binary Ninja/original bytes justify a narrower profile.
- VC5SP3 profiles are explicit exceptions for functions whose original bytes
  currently match VC5-era code generation better than the default VC6 profile.
- MFC42, DirectX, CRT, Win32, and imported runtime behavior should be modeled as
  providers, not fake production stand-ins.
- Production source must preserve 32-bit pointer, alignment, calling convention,
  message-map, vtable, import, and cleanup behavior when those affect generated
  code or ABI.

## Verification Notes

- Native CMake builds and CTest are smoke and guard checks, not binary-safe
  acceptance.
- VC verification manifests should compile production source through
  `source_from`.
- Passing binary-safe verification requires relocation-masked COFF object bytes
  to match Binary Ninja/original bytes, or an explicitly accepted legacy text
  target with documented limits.
- Final executable comparison uses `tools/recoil_vc6_build.py` and
  `tools/recoil_pe_reference.py`, but whole-binary acceptance is expected to
  become useful only when the reconstructed source is substantially complete.

## Open Limits

- Provider notes record workspace assumptions only; each authored function still
  needs address-local Binary Ninja inspection and per-function or group
  verification.
- Some implemented functions currently use limited `☑️` reconstruction markers
  where Binary Ninja/provider/toolchain limits remain.
