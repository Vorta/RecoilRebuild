# zSound CD Verification Notes

- `0x4a2490 zSndCd::ResetTrackState` matches the original bytes when the CD playback position globals are modeled as three contiguous `zSndCdTrackState` triplets (`playFrom`, `current`, `playTo`) and reset by aggregate assignment.
- The matched source signature returns `int`; the original leaves `EAX=1` at return and callers observed so far ignore the value.
- Verification target `tools/vc6_verify_targets/zsnd_cd_reset_track_state.json` uses VC5SP3 `cl` 11.00.7022 with `/G5 /O2 /Ob1 /GX /Zp4 /FAcs` and compares COFF object bytes to Binary Ninja with relocation masking.
- `0x4a3ea0 zSnd::ReportMciError` requires the VC5/VC6 path to call through imported CRT provider pointers for `_iob` and `fprintf`; the modern smoke-build path keeps ordinary `fprintf(stderr, ...)`.
- Verification target `tools/vc6_verify_targets/zsnd_report_mci_error.json` uses the same VC5SP3 `/G5 /O2 /Ob1 /GX /Zp4 /FAcs` profile and passed with zero unmasked byte mismatches.
- `0x4a2750 zSndCd::PlayTrack` matches when `MCI_SEEK_PARMS` is left uninitialized except for `dwTo`, and the source-file path is a `const char[]` so call sites pass its direct address.
- Verification target `tools/vc6_verify_targets/zsnd_cd_play_track.json` uses the same VC5SP3 profile and passed with zero unmasked byte mismatches.
- `0x4a2600 zSndCd::ApplyPlaybackMode` now has coverage in `tools/vc6_verify_targets/zsnd_cd_apply_playback_mode.json`, but still fails object-byte comparison. Current drift is register allocation around `currentTrack`/`playToTrack` and MCI play-parameter setup; no marker has been updated for this address.
