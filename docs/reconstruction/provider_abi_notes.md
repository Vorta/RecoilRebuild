# Provider And ABI Notes

These notes summarize workspace-level provider assumptions for source-faithful
Recoil reconstruction. They do not override Binary Ninja, the plan, or
per-target VC verification evidence.

## Evidence

- `support/Recoil.exe` is the private immutable retail reference for PE,
  imports, resources, and final candidate comparison.
- `support/sdk/DirectX6` supplies repo-local DirectDraw, Direct3D Immediate
  Mode, DirectSound, DirectInput, DirectPlay, and `dxguid` headers and x86
  libraries from the original-era DirectX 6 SDK.
- `support/sdk/MFC42` supplies repo-local MFC42 headers, libraries, runtime DLL,
  and selected source evidence for MFC shell ABI work.
- The A3D backend uses the Aureal A3D 2.0-era COM API, not the A3D 1.2 API:
  `support/Recoil.exe` contains `CLSID_A3dApi`
  `{92FA2C24-253C-11D2-90FB-006008A1F441}`, `IID_IA3d3`,
  `IID_IA3dGeom`, and `IID_IA3dListener`, and does not contain the A3D 1.x
  `CLSID_A3d` or `IID_IA3d`. The preserved A3D 1.2 SDK `Ia3d.h` lacks those
  2.0 symbols and lacks the `IA3d3` `NewSource`, `DuplicateSource`, and
  `SetCooperativeLevel` methods used by `zSndBackend_InitA3D`; the local
  `support/sdk/Aureal/A3D20/inc/ia3dapi.h` and `Ia3dutil.h` match the preserved
  A3D 2.0 SDK headers byte-for-byte.
- `tools/_recoil/config/compiler_linker_profiles.json` records accepted compiler flag profiles;
  `python tools/recoil.py audit provenance --strict` guards VC verification
  manifests against those profiles.

## Recovered Contract

- Tier `S` compiler evidence is VC5SP3 `cl` 11.00.7022 with 32-bit x86 code
  generation. Verification may vary VC5SP3 flags and provider libraries, but
  non-VC5 compiler profiles are retired and do not establish new tier `S`
  evidence.
- VC5SP3 `cl` 11.00.7022 rejects an explicit `__thiscall` keyword as C4234.
  Production reconstruction must not use forced thiscall wrappers or explicit
  `__thiscall`; member ABI must come from real C++ member syntax, virtual
  declarations for proven classes/interfaces, callback/data records only when
  class evidence does not fit, or real provider headers/types. If a
  free-function pointer with `self` in `ecx` is the only current model, the
  source is not reimplemented and the affected entry must stay
  `Reimplemented [X]`/`❌` until the owning source model is recovered.
- Current executable provenance indicates VS97 SP3 `cvtres` for 1 object,
  VS97 SP3 `link` 5.10.7303 for 293 objects, and VS97-era tooling for 10
  objects.
- MFC42, DirectX, CRT, Win32, and imported runtime behavior should be modeled as
  providers, not fake production stand-ins or local ABI shims. Use repo-local
  provider headers where available; otherwise leave the provider detail as a
  blocker.
- Use `plan reclassify` for existing authored/provider entries. Use
  `plan add-provider-boundary ... --dry-run` only after focused plan lookup
  proves a BN-visible provider boundary is absent and current BN/provider
  evidence proves no authored body exists.
- Production source must preserve 32-bit pointer, alignment, calling convention,
  message-map, vtable, import, and cleanup behavior when those affect generated
  code or ABI.

## Verification Notes

- Native CMake builds and CTest are smoke and guard checks, not tier `S`
  acceptance.
- VC verification manifests should compile production source through
  `source_from`.
- Passing tier `S` verification requires relocation-masked COFF object bytes to
  match Binary Ninja/original bytes, or an accepted provider ABI boundary when
  no authored compiler comparison applies.
- Final executable comparison uses `python tools/recoil.py verify final-build`
  and `python tools/recoil.py verify pe`, but whole-binary acceptance is
  expected to become useful only when the reconstructed source is substantially
  complete.

## Open Limits

- Provider notes record workspace assumptions only; each authored function still
  needs address-local Binary Ninja inspection and per-function or group
  verification.
- Some implemented functions currently use limited `☑️` reconstruction markers
  where Binary Ninja/provider/toolchain limits remain.
