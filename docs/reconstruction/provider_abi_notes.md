# Provider And ABI Notes

These notes summarize workspace-level provider assumptions for source-faithful
Recoil reconstruction. They do not override Binary Ninja,
the unified reconstruction tracker, or per-target VC verification evidence.

## Evidence

- `support/Recoil.exe` is the private immutable retail reference for PE,
  imports, resources, and final candidate comparison.
- `support/sdk/DirectX6` supplies repo-local DirectDraw, Direct3D Immediate
  Mode, DirectSound, DirectInput, DirectPlay, and `dxguid` headers and x86
  libraries from the original-era DirectX 6 SDK.
- Official project/build paths select only
  `D:/Recoil Project/Compiler/VC5SP3/VC/MFC/INCLUDE/AFXWIN.H`. The
  `support/sdk/MFC42` and `D:/Recoil Project/Visual C++ 5.0` header trees are
  evidence only. The support tree still supplies provider ABI/source/library
  evidence for MFC shell work.
- A matched `MFC42.LIB`/`MFCS42.LIB` pair from the Visual C++ 5.0 RTM tree may
  be used only as an explicit diagnostic library profile. It must not mix with
  the canonical pair, switch the header root, or establish provider acceptance.
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
  `Reimplemented [X]` until the owning source model is recovered.
- Current executable provenance indicates VS97 SP3 `cvtres` for 1 object,
  VS97 SP3 `link` 5.10.7303 for 293 objects, and VS97-era tooling for 10
  objects.
- MFC42, DirectX, CRT, Win32, and imported runtime behavior should be modeled as
  providers, not fake production stand-ins or local ABI shims. Use repo-local
  provider headers where available; otherwise leave the provider detail as a
  blocker.
- Use `python tools/recoil.py progress show`, `python tools/recoil.py progress
  find`, and `python tools/recoil.py progress owner relationships` for focused
  owner lookup. Add a missing BN-visible
  provider boundary only when current BN/provider evidence proves no authored
  body exists, using
  `python tools/recoil.py progress owner add --id <owner-id> --kind provider-boundary --name <name> --evidence-id sha256:<evidence-hash> --expected-revision <revision> --dry-run`
  before repeating the reviewed command with `--apply` against the same
  revision. Correct stale owner/provider classification through the
  focused owner link, remove, gate, and entry-tier commands.
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

- Provider notes record workspace assumptions only. Verification is owner-
  scoped: each authored primary function/data entry uses the unified
  `Reimplemented [X/C/B/A/S]` tiers, and owner tier `S` requires the complete
  source-shaped owner's primary-owned, referenced, touched, linked, and
  dependency data to satisfy its owner byte gate.
- Legacy checkmark markers are not acceptance state and must not substitute for
  unified owner gates, per-primary-entry tiers, or `Reimplemented [X/C/B/A/S]`.
