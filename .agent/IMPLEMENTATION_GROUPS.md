# Implementation Groups

Use this tracked file for temporary dependency-group notes during
reconstruction. `.agent/SOURCE_OWNERS.json` is the durable owner-scope ledger;
this file lists only active multi-function, source-readiness, owner, or data
groups currently being coordinated. Pure tier `S` verification groups are active only after
`tier_s_priority_ready=true` or explicit user direction. Active groups are the
default no-address startup queue: new agents should resume actionable WIP here
before selecting new work with
`python tools/recoil.py plan next --lane binary`. Keep the header and template
available even when no groups are active.

## Rules

- Create or update a group before editing when a task touches more than one
  function or a shared type/global/vtable.
- Create or update the matching source-owner record with `python tools/recoil.py
  owner ...` before accepting `Source owner`, `Data reimplemented`, or tier
  `B`/`A`/`S` plan markers. This file is not source-owner evidence.
- When launching without a user-specified address or source group, inspect
  active groups first and resume the first actionable one. Start unrelated new
  work only when active groups are absent, stale, contradicted, completed, or
  explicitly deprioritized by the user.
- Keep groups scoped. Prefer one class, one source file cluster, one recursive
  cycle, or one call-chain frontier.
- Do not mark plan entries done from this file alone. Plan markers still
  require current source/build/Binary Ninja evidence.
- Keep notes concise and temporary. Move durable facts into source comments,
  Binary Ninja comments, tests, `docs/reconstruction/`, or narrow subsystem
  docs before pruning.
- Verification-only queues that no longer carry source, owner, or data blockers
  should not live in this active working file while global owner/data blockers
  remain. Use `.agent/RECOIL_PLAN.md`, `python tools/recoil.py status
  0xNNNNNN`, VC verification manifests, and `python tools/recoil.py audit
  backlog --lane binary --include-deferred-verify` for deferred verification
  state.
- Normal binary-lane planning prioritizes owner structure blockers before
  isolated implementation/behavior work and prioritizes global owner/data
  blockers before verify-only tier `S` work. Active verify-only groups should
  condense or move out of this file while any authored `Source owner ❌` or
  `Data reimplemented ❌` marker remains.
- Recompute verification scope with `python tools/recoil.py status 0xNNNNNN`
  or `python tools/recoil.py frontier 0xNNNNNN --depth 1` after source blockers
  clear.
- Use `python tools/recoil.py audit groups --summary --wip-limit 4` to check
  for stale, completed, or overgrown groups.
- Use `python tools/recoil.py audit handoff --path .agent/IMPLEMENTATION_GROUPS.md
  --strict` before launching workers from live handoff blocks.

## Active Group Template

```text
### Group: short descriptive name

- Anchor: 0xNNNNNN Name
- Owner id:
- Section:
- Queue: ready owner/data work / blocked pending evidence or policy / shared blocker / deferred verify-only debt
- Reason: dependency closure / class cluster / recursive cycle / shared ABI layout / source file cluster
- Source blockers:
  - 0xNNNNNN Name
- Next action:
  - python tools/recoil.py status 0xNNNNNN
```

## Source Worker Handoff Template

```text
### Parent batch card: short batch name

- Task kind: active WIP / address-led owner-data work / validation handoff
- Active group or address:
- Evidence packets required:
- Evidence packets received:
- Worker allocation:
- Validation scope:
- Exit criteria:

### Source-worker handoff: short scope name

- Section:
- Owner/source scope:
- Owner id:
- Anchor addresses/groups:
- Allowed write paths:
- Forbidden paths:
- Evidence inputs:
  - BN fact packet:
  - source-owner packet:
  - provider/data packet:
  - scaffold audit packet:
  - workspace/librarian packet:
- Expected source model:
- Validation commands:
- Return packet:
  - changed files
  - evidence used and caveats
  - commands run with pass/fail
  - blockers and overlap warnings
  - non-authoritative marker recommendations only
```

## Verifier Handoff Template

```text
### Verifier handoff: short scope name

- Section:
- Validation scope:
- Anchor addresses/groups:
- Exact commands:
- Evidence inputs:
  - source worker packet:
  - BN fact packet:
  - provider/data packet:
- Forbidden paths:
- Return packet:
  - exact command lines
  - pass/fail results
  - key output lines
  - failure category
  - next narrow verification command
```

## Active Groups

Active queue sections:

- Blocked pending evidence or policy: zVideo/zRndr renderer dispatch remains
  blocked on the ESP-pivot span-family source model unless new evidence or
  explicit policy direction appears.
- Shared blockers: the former zVideo adjust-surfaces cleanup is folded into
  the renderer dispatch/global owner audit because both route through 0x48ff70
  and the 0x42e330 caller/data path.
- Deferred verify-only debt: keep tier S-only zVideo/zRndr and HUD addresses
  in plan/VC manifests, not as active groups, while `tier_s_priority_ready=false`.

### Group: zVideo renderer dispatch/global owner audit

- Anchor: 0x4a77a0 zVideo::BindRendererDispatch
- Section: render_video
- Queue: blocked pending evidence or policy; shared blocker.
- Reason: renderer dispatch globals and DirectDraw hardware-device data shared
  by memory-query, surface, palette, mode-setting, restore, and teardown
  callers.
- Current blockers:
  - Work remains owner/data-led while `tier_s_priority_ready=false`. Do not use
    this group as a verify-only queue unless the user explicitly directs tier S
    work.
  - Source-owner blockers reported by `audit groups --summary`: 0x48ff80,
    0x49b7e0, 0x49e6c0, 0x49edc0, 0x49bbf0, 0x4997d0, 0x49f180,
    0x48d450, plus related zVideo/zRndr renderer-dispatch owners in the same
    source cluster.
  - Data blockers reported by `audit groups --summary`: 0x42e330, 0x48ff70,
    and 0x48d340. The 0x42e330 caller path currently routes through 0x48ff70,
    and 0x48ff70 remains data-blocked by downstream zRndr
    SelectSpanRoutines callback/global ownership.
  - Folded adjust-surfaces status: 0x4a6900 has accepted direct renderer
    adjust-helper source and VC5 byte evidence, and `verify functional`
    evidence for 0x42e330 and 0x48ff70 has been repaired. The remaining
    reason to keep that path active is this shared 0x48ff70 data gate, so it
    should not live as a separate group.
  - 2026-06-19 update: `render_video.zvideo_renderer_dispatch` now has
    accepted boundary/source/data/functional gates for 0x4a77a0 and 0x4a6b40.
    VC5SP3 data-symbol manifests
    `zvideo_renderer_dispatch_core_globals`,
    `zvideo_renderer_dispatch_surface_globals`,
    `zvideo_renderer_dispatch_image_globals`,
    `zvideo_renderer_dispatch_texture_globals`,
    `zvideo_renderer_dispatch_fog_poly_globals`, and
    `zvideo_renderer_dispatch_flush_globals` cover the complete renderer
    selection/fullscreen/dispatch callback global set, including
    `g_zVideo_pfnImageLazyCreateVideoMemorySurface`,
    `g_zVideo_pfnImageEnsureSurfaceForCurrentDevice`,
    `g_zVideo_pfnQueryDeviceVideoMemoryBytes`, and
    `g_zVideo_pfnQueryTextureMemoryBytes`. Plan entries 0x4a77a0 and 0x4a6b40
    are now tier B; tier S remains globally deferred.
  - 2026-06-19 update: `render_video.zvideo_fog_color_globals`,
    `render_video.zvideo_d3d_device_globals`, and
    `render_video.zvideo_dd3d_fog_state` now have accepted data evidence.
    Source-worker pass reordered the zVideo fog/color scalar declarations to
    match BN storage; VC5SP3 data-symbol manifests
    `zvideo_fog_color_pending_bias`, `zvideo_fog_color_target_applied`,
    `zvideo_d3d_fog_cache`, `zvideo_d3d_device_globals_a`, and
    `zvideo_d3d_device_globals_b` passed. Plan entries 0x4a7220, 0x4a7250,
    0x4a7300, 0x4a7330, 0x4a73a0, 0x4aa9e0, 0x4aaa30, 0x4aaa60, 0x4aaa90,
    and 0x4aab30 are now tier B; tier S remains globally deferred.
  - 2026-06-19 update: `render_video.zvideo_pixel_pack_state`,
    `render_video.zvideo_dd3d_submit_queue_storage`, and
    `render_video.zvideo_dd3d_submit_queue` now have accepted data evidence.
    VC5SP3 data-symbol manifests `zvideo_pixel_pack_state_global`,
    `zvideo_dd3d_submit_temp_vertices`, `zvideo_dd3d_submit_sorted_queue`,
    `zvideo_dd3d_submit_overwrite_queue`,
    `zvideo_dd3d_submit_queue_counts`, and
    `zvideo_d3d_render_state_cache_global` passed for the complete
    pixel-pack, temp-vertex, sorted/overwrite queue, queue-count, and
    render-state cache storage. Plan entries 0x4a6b90, 0x4a6bb0, 0x4a6bd0,
    0x4a6bf0, 0x4aab90, 0x4aaef0, 0x4ab320, 0x4ab6d0, 0x4abb20, 0x4ac370,
    0x4acbd0, and 0x4ace30 are now tier B; tier S remains globally deferred.
  - 2026-06-19 update: `render_video.zvideo_dd_hw_api_feature_flags`
    now has accepted data evidence for the complete
    `g_zVideo_HwApiDeviceTable` storage. VC5SP3 data-symbol manifest
    `zvideo_hw_api_device_table_global` passed for the four 0x6ec-byte
    hardware API device records, and 0x4a9920 is now tier B; tier S remains
    globally deferred.
  - 2026-06-19 update: `render_video.zvideo_dd_surface_state_globals`,
    `render_video.zvideo_selected_hw_api_device_record`, and
    `render_video.zvideo_dd_primary_sw_blit` now have accepted data evidence.
    Existing VC5SP3 target `zvideo_dd_present_display_mode_surface_data`
    covers the four DirectDraw surface-state globals, new target
    `zvideo_selected_hw_api_device_record_global` covers
    `g_zVideo_pSelectedHwApiDeviceRecord`, and plan entries 0x4a7d90,
    0x4a7dd0, and 0x4a7e10 are now tier B; tier S remains globally deferred.
  - Same-session BN/source-worker packets for the 0x49b7e0-led switch-vshift
    span family confirm the retail source shape intentionally pivots ESP
    through gRndr_SavedEspSlot and writes destination words with push/sub-esp.
    No safe VC5-era production C++ model was found under the current no raw
    assembly/scaffold rules, so this owner/data gate remains blocked until a
    policy-approved source model is identified or the raw-assembly prohibition
    is explicitly changed.
  - Accepted recent work is durable elsewhere: zRndr queue/lens/fog/palette
    slices, cached-client-rect mask helpers, DirectDraw present/clear/data
    passes, and circle helpers have source/plan/verification evidence. Keep
    this file focused on the remaining owner/data routing.
  - Deferred verify-only addresses include current tier B/S-ready zRndr/zVideo
    byte-comparison debt such as 0x499a20, 0x499c40, 0x49a2b0, 0x49aa90,
    0x49b020, 0x49b780, 0x46e720, and 0x4a8790; revisit them only after the
    global owner/data gate opens or explicit user direction.
- Next action:
  - Do not reassign the 0x49b7e0-led ESP-pivot span-family slice without new
    BN/source-model evidence or explicit user approval for a lower-level
    implementation strategy. Prefer another active owner/data WIP while that
    slice remains blocked by the current source rules.
  - If the caller/data path is resumed, start with
    `python tools/recoil.py status 0x42e330 --lane binary`, then route through
    the 0x48ff70 data blocker before assigning any source worker.
  - Re-run `python tools/recoil.py audit groups --summary --wip-limit 4` after
    each owner/data update and prune this group again when it becomes
    verify-only.
