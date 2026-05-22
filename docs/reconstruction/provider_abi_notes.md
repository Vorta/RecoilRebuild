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

- First-pass binary-safe compiler evidence is VC5SP3 `cl` 11.00.7022 with
  32-bit x86 code generation unless Binary Ninja/original bytes justify another
  profile.
- VC6 `cl` 12.00.8168 profiles are fallbacks for functions whose original
  bytes, imports, MFC/CRT behavior, or a reviewed VC5SP3 attempt indicate
  VS98-era code generation is more plausible.
- VC5SP3 `cl` 11.00.7022 rejects an explicit `__thiscall` keyword as C4234, so
  `RECOIL_THISCALL` intentionally remains empty for `_MSC_VER < 1300`. For VC5
  function-pointer vtable slots, such as `0x462660 zFMV_Script::Reset`, a raw
  free-function pointer can still compile as cdecl while Binary Ninja shows
  member-call dispatch (`ecx = self`, pushed user arguments only). Do not
  broaden the shared macro to force `__thiscall`; solve these as local ABI
  modeling or manifest/toolchain issues.
- Current executable provenance indicates VS97 SP3 `cvtres` for 1 object,
  VS97 SP3 `link` 5.10.7303 for 293 objects, and VS98-era tooling for 10
  objects.
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
