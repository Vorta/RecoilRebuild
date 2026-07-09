# Recoil Engine Reconstruction

This repository is a source-faithful native C/C++ reconstruction of the Windows
x86 engine for Zipper Interactive's 1999 game **Recoil**.

The project aims to recover readable, maintainable source that preserves the
behavior, ABI-sensitive structure, and late-1990s Windows engine boundaries of
the original game. It is a reconstruction effort, not a modern port.

## What's Included

- `src/Battlesport` - recovered game shell and gameplay-facing source.
- `src/GameZRecoil` - recovered engine subsystems for rendering, video, sound,
  FMV, HUD, geometry, input, networking, assets, and utility code.
- `src/native` - native target wiring and include bridges for the reconstructed
  source.
- `tests/native` - focused native behavior and ABI checks.
- `docs/reconstruction` - compact durable notes about recovered engine facts.
- `export` - text snapshots from executable analysis used to guide recovery.

The public repository is source-only. It does not redistribute the original
game executable, game data, extracted assets, decompiler databases, legacy SDK
payloads, redistributable DLLs, or compiler toolchains.

## Engine Overview

The recovered source is organized around the original engine boundaries:

- `Battlesport` contains the MFC application shell, frame windows, game-state
  transitions, mission startup, player and network gameplay glue, pickups, and
  HUD integration.
- `GameZRecoil/RecoilApp` contains the reconstructed state host and state
  objects used by menus, FMV playback, gameplay, and transitions.
- `zVideo`, `zRndr`, `zImage`, and `zModel` cover DirectDraw/Direct3D-era video
  setup, software and hardware rendering paths, image and texture handling,
  model display, lighting, and material behavior.
- `zSound` and `zFMV` cover DirectSound/A3D-era audio, CD audio, sample sets,
  streamed and grouped sound playback, and Video for Windows FMV scripting.
- `zHud`, `zInput`, `zNetwork`, `zGeometry`, `zMath`, `zReader`, `zClass`, and
  `zUtil` cover UI widgets, input mapping, DirectPlay networking, clipping and
  geometry helpers, math, asset loading, object/class runtime data, and save or
  resource utilities.

The codebase intentionally keeps late-1990s Windows C/C++ idioms where they
matter for layout, calling conventions, provider behavior, and generated code
shape. Modern helper code is limited to places where it makes reconstruction,
testing, or review practical without changing the recovered engine contract.

## Status

<!-- RECOIL_PROGRESS:START -->
Generated from `.agent/SOURCE_OWNERS.json` (schema version 3) and source-owner projection rows.

### Source-Owner Overview

| Binary | Owners | Authored owners | Provider boundaries |
| --- | ---: | ---: | ---: |
| recoil | 1244 | 1199 | 45 |
| messages | 1 | 1 | 0 |
| Total | 1245 | 1200 | 45 |

### Source-Owner Gates

| Gate | accepted | blocked | deferred | none | pending | Total |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| boundary | 1234 | 4 | 0 | 1 | 6 | 1245 |
| source | 1226 | 7 | 0 | 1 | 11 | 1245 |
| data | 967 | 3 | 0 | 215 | 60 | 1245 |
| functional | 807 | 0 | 0 | 325 | 113 | 1245 |
| linkage | 1028 | 141 | 0 | 67 | 9 | 1245 |
| byte | 23 | 466 | 725 | 0 | 31 | 1245 |

### Owner Reimplementation Tiers

| Tier | Count | Percent of authored owners |
| --- | ---: | ---: |
| X | 117 | 9.8% |
| C | 222 | 18.5% |
| B | 846 | 70.5% |
| A | 1 | 0.1% |
| S | 14 | 1.2% |

### Authored Entry Matching Tiers

Counts source-owner projection rows for authored function/data entries. This is byte-matching progress per entry, not source-owner scope tier acceptance.

| Tier | recoil | messages | Total | Percent of authored entries |
| --- | ---: | ---: | ---: | ---: |
| X | 833 | 0 | 833 | 12.2% |
| F | 0 | 0 | 0 | 0.0% |
| C | 1653 | 0 | 1653 | 24.3% |
| B | 4277 | 0 | 4277 | 62.8% |
| A | 4 | 0 | 4 | 0.1% |
| S | 39 | 2 | 41 | 0.6% |

### Owner Kinds

| Kind | Count | Percent of owners |
| --- | ---: | ---: |
| callback-table | 1 | 0.1% |
| class | 163 | 13.1% |
| data-owner | 430 | 34.5% |
| provider-boundary | 45 | 3.6% |
| record | 42 | 3.4% |
| source-file | 167 | 13.4% |
| standalone | 58 | 4.7% |
| subsystem | 339 | 27.2% |
<!-- RECOIL_PROGRESS:END -->

<!-- RECOIL_SOURCE_BLOCKS:START -->
Generated from `tools/_recoil/config/source_file_blocks.json`.

Ranges are half-open retail `.text` ranges. `partial` rows are header contributors physically emitted inside the owning `.cpp` block. `semantic` rows identify BN-proven semantic islands whose original source/header and physical host are still unresolved.

| Range | Source path | Status / detail |
| --- | --- | --- |
| `0x401000..0x401060` | `src/Battlesport/about.cpp` | mapped-no-literal-about-prelude-order-confirmed-contiguous; 4 semantic subranges |
| `0x401060..0x402f60` | `src/Battlesport/ai_net.h` | partial, included in `src/Battlesport/ai_net.cpp` |
| `0x402f60..0x402fd0` | `src/GameZRecoil/zMath/zmth.h` | partial, included in `src/Battlesport/ai_net.cpp` |
| `0x402fd0..0x4038a0` | `src/Battlesport/ai_net.cpp` | mapped; 6 semantic subranges |
| `0x4038a0..0x404ca0` | `src/Battlesport/Briefing.cpp` | mapped; 11 semantic subranges |
| `0x404ca0..0x415ab0` | `src/Battlesport/hud.cpp` | mapped-refined-order-diagnostic-clean; 22 semantic subranges |
| `0x415ab0..0x417350` | `src/Battlesport/map.cpp` | mapped-refined; 8 semantic subranges |
| `0x417350..0x41cc10` | `src/Battlesport/mission.cpp` | mapped-refined; 14 semantic subranges |
| `0x41cc10..0x41ea90` | `src/Battlesport/pickup.cpp` | mapped; 10 semantic subranges |
| `0x41ea90..0x42de10` | `src/Battlesport/player.cpp` | mapped; 19 semantic subranges |
| `0x42de10..0x436630` | `src/Battlesport/RecoilApp.cpp` | mapped physical; CZRecoilFrame semantic layer retained inside block; 9 semantic subranges |
| `0x436630..0x437e60` | `src/Battlesport/turret.cpp` | mapped; 7 semantic subranges |
| `0x437e60..0x438980` | `src/Battlesport/util.cpp` | mapped-no-literal-mixed; 7 semantic subranges |
| `0x438980..0x438990` | `src/Battlesport/version.cpp` | mapped-no-literal |
| `0x438990..0x43ce80` | `src/Battlesport/weapon.cpp` | mapped-no-literal-mixed; 11 semantic subranges |
| `0x43ce80..0x43cf90` | `src/Battlesport/WinSock.cpp` | mapped-no-literal-mixed; 6 semantic subranges |
| `0x43cf90..0x442890` | `src/Battlesport/WOL.cpp` | mapped-no-literal-mixed; 13 semantic subranges |
| `0x442890..0x4428b0` | `provider:mfc/message-map-getters` | provider, provider-boundary |
| `0x4428b0..0x443730` | `semantic:late-recoilapp-appframe-cluster` | semantic source unresolved; Late RecoilApp/app-frame runtime and state-queue authored cluster physically emitted before the CZGameFrame class block; exact original source/header and physical host unresolved.; candidates: `src/CZGameFrame/AppFrame.cpp`, `src/CZGameFrame/AppFrame.h`, `src/Battlesport/recoil_app.h` |
| `0x443730..0x443b70` | `src/CZGameFrame/CZGameFrame.cpp` | mapped-no-literal-czgameframe-cluster; 8 semantic subranges |
| `0x443b70..0x443c50` | `provider:mfc/gdi-bitmap-destructors` | provider, provider-boundary |
| `0x443c50..0x4478c0` | `src/GameZRecoil/zClass/cls_di.c` | mapped; 6 semantic subranges |
| `0x4478c0..0x449ba0` | `src/GameZRecoil/zClass/Class.c` | mapped-refined; 24 semantic subranges |
| `0x449ba0..0x44d990` | `src/GameZRecoil/zClass/Camera.c` | mapped-refined-with-semantic-exceptions; 4 semantic subranges |
| `0x44d990..0x44e630` | `src/GameZRecoil/zClass/Object3d.c` | mapped-refined; 14 semantic subranges |
| `0x44e630..0x44f7a0` | `src/GameZRecoil/zClass/List.c` | mapped; 3 semantic subranges |
| `0x44f7a0..0x44fdd0` | `src/GameZRecoil/zClass/Window.c` | mapped; 3 semantic subranges |
| `0x44fdd0..0x450030` | `src/GameZRecoil/zClass/Display.c` | mapped-refined; 3 semantic subranges |
| `0x450030..0x4518b0` | `src/GameZRecoil/zClass/cls_world.c` | mapped-refined; 16 semantic subranges |
| `0x4518b0..0x452920` | `src/GameZRecoil/zClass/cls_util.c` | mapped-refined; 6 semantic subranges |
| `0x452920..0x4529c0` | `src/GameZRecoil/zClass/Switch.c` | mapped-refined; 2 semantic subranges |
| `0x4529c0..0x452fd0` | `src/GameZRecoil/zClass/Sound.c` | mapped-refined; 5 semantic subranges |
| `0x452fd0..0x453b10` | `src/GameZRecoil/zClass/Light.c` | mapped-refined; 3 semantic subranges |
| `0x453b10..0x453ee0` | `src/GameZRecoil/zClass/Animate.c` | mapped-refined; 4 semantic subranges |
| `0x453ee0..0x454360` | `src/GameZRecoil/zClass/Seq.c` | mapped-refined; 2 semantic subranges |
| `0x454360..0x4558f0` | `src/GameZRecoil/zClass/cls_zbd.c` | mapped-refined; 9 semantic subranges |
| `0x4558f0..0x455ea0` | `src/GameZRecoil/zDEClient/zdec_init.cpp` | mapped; 3 semantic subranges |
| `0x455ea0..0x456ad0` | `src/GameZRecoil/zDEClient/zdec_qsand.cpp` | mapped; 7 semantic subranges |
| `0x456ad0..0x458af0` | `src/GameZRecoil/zDEClient/zdec_crater.cpp` | mapped-with-semantic-exceptions; 7 semantic subranges |
| `0x458af0..0x45e100` | `src/GameZRecoil/zEffect/zeff_anim_run.c` | mapped; 7 semantic subranges |
| `0x45e100..0x460020` | `src/GameZRecoil/zEffect/zeff_anim_init.c` | mapped; 6 semantic subranges |
| `0x460020..0x4603d0` | `src/GameZRecoil/zEffect/zeff_init.c` | mapped; 4 semantic subranges |
| `0x4603d0..0x4622f0` | `src/GameZRecoil/zEffect/zeff_anim_save.c` | mapped; 4 semantic subranges |
| `0x4622f0..0x462330` | `src/GameZRecoil/zError/zerr_old.c` | mapped; 2 semantic subranges |
| `0x462330..0x4625e0` | `src/GameZRecoil/zFMV/fmv_main.cpp` | mapped; 3 semantic subranges |
| `0x4625e0..0x463d50` | `src/GameZRecoil/zFMV/fmv_script.cpp` | mapped; 8 semantic subranges |
| `0x463d50..0x464670` | `src/GameZRecoil/zFMV/fmv_stream.cpp` | mapped; 5 semantic subranges |
| `0x464670..0x46a690` | `src/GameZRecoil/zGeometry/zgeo_weiler.cpp` | mapped-with-semantic-conflicts; 5 semantic subranges |
| `0x46a690..0x46bd50` | `src/GameZRecoil/zGeometry/zgeo_model.cpp` | mapped-with-semantic-conflicts; 7 semantic subranges |
| `0x46bd50..0x46d310` | `src/GameZRecoil/zGeometry/zgeo_convexify.cpp` | mapped-with-semantic-exceptions; 4 semantic subranges |
| `0x46d310..0x46efc0` | `src/GameZRecoil/zImage/zimg_texture.cpp` | mapped-with-semantic-conflicts; 5 semantic subranges |
| `0x46efc0..0x46f300` | `src/GameZRecoil/zImage/zimg_fonts.cpp` | mapped; 5 semantic subranges |
| `0x46f300..0x470020` | `src/GameZRecoil/zInput/zin_kbd.cpp` | mapped; 4 semantic subranges |
| `0x470020..0x4706c0` | `src/GameZRecoil/zInput/zin_mouse.cpp` | mapped-no-literal-bracketed; 19 semantic subranges |
| `0x4706c0..0x4719e0` | `src/GameZRecoil/zInput/zinput.cpp` | mapped-no-literal-bracketed; 58 semantic subranges |
| `0x4719e0..0x471e40` | `src/GameZRecoil/zInput/zin_init.cpp` | mapped-refined; 7 semantic subranges |
| `0x471e40..0x472670` | `src/GameZRecoil/zInput/zin_joystick.cpp` | mapped-no-literal-bracketed-with-semantic-exceptions; 5 semantic subranges |
| `0x472670..0x475c40` | `src/GameZRecoil/zMath/zmth_main.c` | mapped-refined-literal-backed; 10 semantic subranges |
| `0x475c40..0x4805b0` | `src/GameZRecoil/zModel/gmod_init.c` | mapped-with-semantic-conflicts; 22 semantic subranges |
| `0x4805b0..0x481530` | `src/GameZRecoil/zModel/gmod_matl.c` | mapped-with-semantic-conflicts; 9 semantic subranges |
| `0x481530..0x487a30` | `src/GameZRecoil/zModel/gmod_const.c` | mapped-with-semantic-conflicts; 19 semantic subranges |
| `0x487a30..0x489d00` | `src/GameZRecoil/zModel/gmod_light.c` | mapped; 2 semantic subranges |
| `0x489d00..0x48c7d0` | `src/GameZRecoil/zNetwork/znet_dplay.cpp` | mapped; 11 semantic subranges |
| `0x48c7d0..0x48d340` | `src/GameZRecoil/zReader/zreader.cpp` | mapped; 5 semantic subranges |
| `0x48d340..0x49f614` | `src/GameZRecoil/zRender/zrndr_draw.c` | mapped-with-semantic-conflicts; 15 semantic subranges |
| `0x49f614..0x4a10e0` | `src/GameZRecoil/zSound/zsnd_play.cpp` | mapped-with-semantic-slices; 7 semantic subranges |
| `0x4a10e0..0x4a12c0` | `src/GameZRecoil/zSound/zsnd_parm.cpp` | mapped; 3 semantic subranges |
| `0x4a12c0..0x4a2010` | `src/GameZRecoil/zSound/zsnd_init.cpp` | mapped; 3 semantic subranges |
| `0x4a2010..0x4a2950` | `src/GameZRecoil/zSound/zsnd_cd.cpp` | mapped; 2 semantic subranges |
| `0x4a2950..0x4a2ea0` | `src/GameZRecoil/zSound/zsnd_3d.cpp` | mapped-effective-literal; 3 semantic subranges |
| `0x4a2ea0..0x4a3930` | `src/GameZRecoil/zSound/zsnd_create.cpp` | mapped; 3 semantic subranges |
| `0x4a3930..0x4a3ea0` | `src/GameZRecoil/zSound/zsnd_fade.cpp` | mapped-no-literal-bracketed; 10 semantic subranges |
| `0x4a3ea0..0x4a44c0` | `src/GameZRecoil/zSound/zsnd_error.cpp` | mapped-no-literal-reporter-shelf; 8 semantic subranges |
| `0x4a44c0..0x4a53f0` | `src/GameZRecoil/zSound/zsnd_grp.cpp` | mapped; 5 semantic subranges |
| `0x4a53f0..0x4a5670` | `src/GameZRecoil/zSound/zsnd.cpp` | mapped-no-literal-bracketed; 3 semantic subranges |
| `0x4a5670..0x4a59d0` | `src/GameZRecoil/zSys/zsys.cpp` | mapped-no-literal-candidate; 4 semantic subranges |
| `0x4a59d0..0x4a59e0` | `provider:kernel32-gettickcount-import-thunk` | provider, provider-boundary |
| `0x4a59e0..0x4a5c20` | `src/GameZRecoil/zSys/zsys.cpp` | mapped-no-literal-candidate; 3 semantic subranges |
| `0x4a5c20..0x4a66e0` | `src/GameZRecoil/zUtil/zutl_zar.cpp` | mapped; 4 semantic subranges |
| `0x4a66e0..0x4a69c0` | `src/GameZRecoil/zVideo/zvid_main.c` | mapped-no-literal-mixed; 14 semantic subranges |
| `0x4a69c0..0x4a6b40` | `src/GameZRecoil/zVideo/zvid_buff.c` | mapped-after-unassigned-prelude; 2 semantic subranges |
| `0x4a6b40..0x4a7b40` | `src/GameZRecoil/zVideo/zvid_init.c` | mapped-with-semantic-exceptions; 12 semantic subranges |
| `0x4a7b40..0x4a9ac0` | `src/GameZRecoil/zVideo/zvid_dd.c` | mapped; 6 semantic subranges |
| `0x4a9ac0..0x4ae380` | `src/GameZRecoil/zVideo/zvid_ddd3d.c` | mapped-with-semantic-tail; 8 semantic subranges |
| `0x4ae380..0x4b2960` | `src/GameZRecoil/zWeapon/zwep_init.c` | mapped-broad-physical-multi-owner; 7 semantic subranges |
| `0x4b2960..0x4b33f0` | `src/GameZRecoil/zGame/zgame_opt.c` | mapped-no-literal-mixed; 11 semantic subranges |
| `0x4b33f0..0x4b3ce0` | `src/GameZRecoil/zSys/zsys_cpu.cpp` | mapped-no-literal; 2 semantic subranges |
| `0x4b3ce0..0x4bffe0` | `src/GameZRecoil/zUI/zui.cpp` | mapped-no-literal-mixed; 64 semantic subranges |
| `0x4bffe0..0x4c0d20` | `src/GameZRecoil/zUtil/zutl_zbd.cpp` | mapped-no-literal-mixed; 11 semantic subranges |
| `0x4c0d20..0x4c5a50` | `src/GameZRecoil/zInterp/zinterp_parse.cpp` | mapped; 4 semantic subranges |
| `0x4c5a50..0x4c5eb8` | `provider:mfc42-tail-import-thunks` | provider, provider-boundary |
| `0x4c5eb8..0x4c5ec0` | `padding:linker-before-msvc-eh-tail` | padding, padding |
| `0x4c5ec0..0x4c60a0` | `provider:msvc5-eh-array-helpers` | provider, provider-boundary |
| `0x4c60a0..0x4c60b0` | `provider:msvc-crt-import-thunks` | provider, provider-boundary |
| `0x4c60b0..0x4c637c` | `provider:vc5-crt-startup-runtime` | provider, provider-boundary |
| `0x4c637c..0x4c63f0` | `provider:platform-directx-tail-import-thunks` | provider, provider-boundary |
| `0x4c63f0..0x4c7408` | `provider:directinput-c_dfDIKeyboard-data` | provider, provider-data |
| `0x4c7408..0x4c7410` | `padding:linker-between-directinput-c_dfDIKeyboard-and-c_dfDIMouse` | padding, padding |
| `0x4c7410..0x4c7498` | `provider:directinput-c_dfDIMouse-data` | provider, provider-data |
| `0x4c7498..0x4c74a0` | `provider:directinput-import-thunk` | provider, provider-boundary |
| `0x4c74a0..0x4c7ef8` | `provider:directinput-c_dfDIJoystick-data` | provider, provider-data |
| `0x4c7ef8..0x4c7f00` | `padding:linker-before-tail-authored-island` | padding, padding |
| `0x4c7f00..0x4c7fd0` | `semantic:late-zimage-font-blit-before-winmain` | semantic source unresolved; zImage font blit authored body physically emitted immediately before WinMain tail; exact original source/header and physical host unresolved; candidates: `src/GameZRecoil/zImage/zimg_fonts.cpp`, `src/GameZRecoil/zImage/zimg_fonts.h` |
| `0x4c7fd0..0x4c81c0` | `semantic:late-zvideo-palette-before-winmain` | semantic source unresolved; zVideo palette authored bodies physically emitted immediately before WinMain; exact original source/header and physical host unresolved; candidates: `src/GameZRecoil/zVideo/zVideo.cpp`, `src/GameZRecoil/zVideo/zvid.h` |
| `0x4c81c0..0x4c81d8` | `src/WinMain.cpp` | mapped-no-literal-entry-thunk-candidate |
| `0x4c81d8..0x4c8230` | `provider:mfc42-crt-module-state-tail` | provider, provider-boundary |
| `0x4c8230..0x4cb9e8` | `provider:msvc-cxx-eh-funclet-tail` | provider, provider-boundary |
<!-- RECOIL_SOURCE_BLOCKS:END -->



Reconstruction is ongoing. Some subsystems have source-level implementations
and focused tests, while others remain partial or documented as recovered facts.
The project favors evidence-backed source recovery over broad rewrites, so code
may retain original-era naming, structure, and implementation style where that
helps preserve behavior.

## License

The reconstructed source, tests, and documentation in this repository are
released under the MIT License; see `LICENSE`.

That license does not grant rights to the original *Recoil* game, original
retail executable, game data, extracted resources, third-party SDKs,
redistributable DLLs, decompiler databases, or compiler/toolchain payloads.
