# Changelog

Reconstructed release history for Recoil, newest first. Changes are relative to the next older entry. Demo dates identify builds; full-game release dates are unknown. Regressions identify newly introduced defects; Known issues lists other unresolved problems. Fix references identify the first listed build containing the fix.

Changelog up until version 1.0.6.8 is a historic reconstruction, not an actual changelog for this codebase.

## 1.0.6.8 — Retail

### Added

- Added Westwood Online account setup, a Remember Password option, game browsing, and hosting and joining through the online service.
- Added an update-download window with connection status, download progress, and estimated time remaining.
- Added saved-game deletion with confirmation and an updated save list.
- Added a final-lap notification to multiplayer races.
- Added localization support for scrolling text.
- Added logging of synchronous or asynchronous TCP/IP sending, capability-query failures, and missing reliable TCP/IP support, with synchronous fallback when asynchronous sending is unavailable.
- Added logging of fatal multiplayer startup and exit errors before displaying their dialogs.
- Added debugger output identifying the source file and line when the message DLL fails to load.
- Added a Winsock 2 compatibility check before TCP/IP or Westwood Online connections, with a warning and a choice to continue or cancel.

### Changed

- Replaced the vehicle-form HUD icons’ text labels with square status indicators, including a bright indicator for the selected form.
- Moved the multiplayer briefing’s mission label and progress bar, and the results screen’s buttons, upward by 15 pixels.
- Reduced the sonic burst’s screen-distortion radius and duration, with additional distortion stages controlled by distance.
- Moved a nano-canister pickup to a different location in Mission 3.
- Enabled positional playback for the looping ping sound and changed the High/Medium incoming-warning samples and High helicopter sample from stereo to mono.
- Narrowed the hover enemy’s ground-support footprint.
- Revised Mission 1’s shoreline surfaces and northern-base terrain, Mission 2’s bunker and gate geometry, and Mission 3’s volcanic terrain and lava-bubble placement.
- Extended lower geometry on Mission 4’s train sections and revised terrain in Mission 5.
- Revised Mission 6’s building roofs and finale-room surfaces, removed cutout transparency from one building texture, and added four fire elements to the finale scenery.
- Destroying Mission 1’s force-field generator now also destroys the truck and triggers the rebel van’s escape sequence.
- Race setup now requires at least two laps and disables time-limit controls. Selecting a combat world restores the combat limits and controls.
- Disabled player-count adjustments for modem sessions.
- Multiplayer races now end when the first player reaches the lap target, removing the grace period of up to 60 seconds.
- Reduced routine multiplayer update traffic. Player updates now include colors and refresh remote vehicles and scoreboard entries when colors change.
- Chat messages now use reliable delivery.
- Moved chat to a separate message area from kills and gameplay notifications.
- Restored the overhead-camera cheat and made it a requirement for the **G** shortcut.
- Removed **G** and **Shift+O** from the normal Controls list while retaining their default bindings and cheat unlocks.
- Updated the repair cheat to reinitialize movement, refresh the secondary weapon, and restore the applicable HOVER, AMPHIBIOUS, or SUBMARINE mode.
- Removed the AI Active/Alive line from the diagnostic HUD, retaining vehicle, camera, position, and yaw information.
- New settings now default to DirectSound instead of automatically selecting A3D when it is available.
- The finale movie now plays to completion without keyboard skipping.
- Disabled AI turret setup in multiplayer games.
- Multiplayer match timers now follow elapsed real time independently of simulation-time scaling and clamping.
- Offscreen animation update intervals now depend on the animation's configured priority.
- Projectile movement, lifetimes, and continuous beam damage now follow elapsed real time independently of simulation-time scaling and clamping.
- Remote guided-weapon launches now clear earlier guided projectiles and create a fresh projectile attachment.
- Disabled weapon updates while multiplayer vehicles are in the destroyed state.

### Fixes

- Prevented Mission 1’s truck destruction and rebel-van escape sequences from running more than once.
- Failed saves now remove the incomplete file and offer Retry or Cancel.
- Preserved transparency settings when loading textures with embedded palettes, and released their alpha masks correctly during cleanup.
- Fixed truncation of long player names and stray trailing characters.
- Helicopter sounds now stop in SUBMARINE mode and resume when returning to TRACKED, AMPHIBIOUS, or HOVER mode.
- Preserved temporary weapon-hit effects and their remaining duration across saves. Loading accepts the preceding player-record layout as well as the expanded layout.
- Multiplayer self-kills now subtract one kill, with a minimum score of zero. Death notifications also work when no weapon is associated with the event.
- Remote secondary shots now use the weapon identified by the firing message.
- Collecting ammunition for another weapon variant no longer overwrites the displayed variant's ammo counter.
- Reset both variants in every secondary-weapon bank when clearing weapon state, including visibility, position, and scale.
- Empty chat submissions are no longer displayed or sent.
- Hosting now trims and validates the player name, rejects an empty name, and returns focus to the name field.
- Protected the final-mission results sequence from turret updates and objective-review or panel-toggle commands once all five objectives are complete.
- Mouse capture now follows window focus and correctly handles an already-acquired or already-released mouse.
- Released cached visual effects during mission cleanup, preventing allocations from accumulating across mission changes and reloads.
- Prevented destroyed vehicles from collecting nearby pickups.
- Prevented weapon ammunition from falling below zero when firing consumes the remaining reserve.
- Secondary-weapon force feedback now plays only when a shot is successfully fired.
- Prevented the pending weapon-hit queue from overflowing during heavy combat; excess hits are processed immediately.
- Released beam and trail allocations when reloading secondary-weapon banks, removing vehicles, or clearing turrets.
- Cleared pending pickup respawns during mission cleanup.
- Restored minimized fullscreen windows when the game regains focus.
- Cleared water and lava contact state before respawning so that stale terrain checks cannot block restoration of TRACKED mode.
- Corrected secondary-weapon cleanup when switching weapons, clearing the stowed weapon's projectile without clearing the newly selected weapon's projectile.
- Empty secondary weapons no longer redeploy after their last guided projectile finishes.
- Applied save/load availability checks to requests to open either dialog.
- Dismissed stale objective displays when restoring a saved mission timer.
- Protected locally collected pickups from remote removal until their collection animation finishes.
- Restored the weapon-movement sound when retracting a secondary weapon.

### Regressions

- Attempting to enter the guided-weapon camera from the overhead view can hide the HUD without switching cameras. The HUD is not restored automatically when that projectile finishes.
- Replacing a remote guided projectile can also remove other vehicles' active guided projectiles.

### Known issues

- Destroyed enemies can continue to trigger proximity mines in single-player, even after their visible models disappear. Present since [Demo 1998-07-21](#demo-1998-07-21).
- Mission 3 turret `tur_202` uses the ordinary pulse-turret model but fires enhanced red pulse ammunition. Present since [1.0.1.23](#10123--full-game).
- High frame rates can cause uneven or accelerated vehicle movement. Vehicle physics advances even on frames with little or no measured elapsed time, and timing precision degrades after long Windows uptimes. Present since [Demo 1998-07-21](#demo-1998-07-21).
- Enemy nanite and most ammunition drops stop appearing when that pickup type's instance counter reaches 100. Collecting pickups does not reset the counter, and failed nanite drops also suppress the alternative ammunition reward. Present since [Demo 1998-07-21](#demo-1998-07-21).
- Easy and Hard vehicle parameters and pickup layouts fall back to Normal when the matching difficulty files exist only inside ZBD archives. The file check searches only for loose files.
- Paletted textures with uniform transparency can display incorrect colors in RGB565 software rendering.
- Some asset and save warnings are silent; other diagnostic output remains available.
- With CD Audio enabled, entering or resuming a mission can crash if the CD drive reports exactly two tracks. Regression introduced in [Demo 1998-09-28](#demo-1998-09-28).
- HOVER-to-TRACKED transformations incorrectly succeed over water. Regression introduced in [1.0.1.23](#10123--full-game).
- Disabling an object's texture scrolling through `Object3DSetScroll` or `Object3DSetScrollAlways` can crash the game. Regression introduced in [Demo 1998-09-28](#demo-1998-09-28).


## 1.0.1.23 — Full game

### Added

- Expanded the single-player demo into a six-mission campaign, with new objectives, briefings, enemy placements, secrets, and scripted encounters.
- Added campaign progression through the AMPHIBIOUS, HOVER, and SUBMARINE upgrades, with missions built around flooded, volcanic, industrial, and fortified environments.
- Expanded enemy encounters with amphibious, hover, submarine, freon, laser, laser-designator, sonic, and arc-sabre vehicles.
- Added distinct engine loops for ten enemy vehicle types, with individual volume and pitch settings.
- Added propeller-bubble trails in the campaign’s submarine missions at High effects detail.
- Added a Reset button to restore default controls and refresh the bindings list.
- Added Previous and Next buttons to the save/load interface.
- Added a finale sequence followed by credits and a return to the main menu.
- Added immediately written mission-loading and shutdown logs for resource loading, stopping sounds, unloading objectives, and leaving gameplay/networking.
- Added output-log messages for an unregistered A3D component, COM creation or aggregation failures, and unclassified creation failures.
- Displayed the selected primary weapon's name when switching variants.

### Changed

- Halved the laser designator’s charge time from 2.5 to 1.25 seconds.
- Made Mission 1’s helicopter guns effectively indestructible independently of the helicopter.
- Expanded the sound library with campaign dialogue and ambient effects, and supplied separate High and Low sound banks alongside Medium.
- Retuned sound falloff so that many shots, explosions, skids, and splashes remain at full volume over a larger nearby area, while helicopters, transports, and burning wrecks have shorter audible ranges.
- Enabled positional playback for Mission 1’s introduction, hints, and missile-site announcements, and shortened both pulse-gun firing samples.
- Revised multiplayer world previews, Mission 1’s secret-beach posters, and selected hardware texture resolutions.
- Removed generic destroyed-object fragments after four seconds instead of leaving them visible.
- Added a proximity check beneath Mission 1’s extraction transport that destroys the transport if the BFT enters that area before boarding.
- Saved games are now sorted by modification time, newest first.
- Replaced the demo introduction with the full-game introduction. Media detection now also looks for the introduction movie.
- Added modem-specific multiplayer connection controls and button behavior.
- Localized mission statistics for objectives, accuracy, defeated enemies, collected weapons, and elapsed time, and adjusted their layout and spacing.
- Disabled save/load shortcuts in multiplayer and checked loading availability before opening the load interface.
- Restored **Ctrl+X** cheat entry for single-player.
- Restored the diagnostic-display cheat and **Shift+O** shortcut, now requiring the cheat unlock. **G** continues to open the overhead camera directly.
- Expanded the diagnostic HUD to show separate active and living AI counts.
- Changed the repair cheat to restore missing secondary-weapon attachments through their normal deployment sequence.
- Localized the repair confirmation message.
- Localized multiplayer index-assignment errors and added audio, network, and video shutdown before exiting.
- New settings now prefer hardware acceleration when an accelerator is available.
- New audio settings now select A3D when its component is available, otherwise DirectSound.
- Localized the multiplayer results-screen timeout warning and added audio, network, and video shutdown before exiting.
- Removed selection of separate Easy and Hard AI vehicle placement lists (`aiv_easy.zrd` and `aiv_hard.zrd`). All difficulties now use `aiv.zrd`, even when the alternative files are present.
- Changed A3D sound positioning and listener orientation to use the game's world axes.
- Localized the deep-water and lava warnings.
- Hosted games now default to the player name without adding "'s game".
- Removed the accelerator-compatibility confirmation shown before launching the demos.
- Destroyed turrets no longer contribute to the Enemies Killed statistic.

### Fixes

- Stopped Mission 1’s transport sounds, airdrops, drop-off, and extraction sequences when the transport is destroyed; allowed its destruction animation to finish before cleanup.
- Reset transport wreckage positions, rotations, and scales before reusing the destruction effect.
- Prevented Mission 1’s seagull departure animations from being triggered repeatedly.
- Excluded transient missile-launch, muzzle-flash, napalm, and smoke-trail animations from saved animation replay.
- Incompatible saved player records are now skipped without applying them or displaying an incompatibility warning.
- Preserved difficulty in saved games and restored completed objective markers when loading.
- Refreshed weapon icons, variants, and ammo counters after loading, selecting an owned variant with ammunition when necessary.
- Fixed objective review when the selection reaches the end of the objective list.
- Fixed setup and polling for joysticks with fewer axes.
- Repeated joystick enable/disable requests no longer disturb acquisition state.
- Restored the active secondary-weapon attachment's position, scale, and door-animation state after damage or destruction.
- Stopped the receive loop after multiplayer session loss and rejected further receives from the lost session.
- Acceleration, renderer API, and Fullscreen selections are now saved and restored between launches.
- Audio options now reflect the active backend after A3D initialization falls back to DirectSound.
- Returning from menus now resumes only the sound instances that were playing before the menu opened.
- A failed automatic HOVER transformation no longer bypasses lava damage.
- Prevented weapon effects from intercepting their own ground-impact checks.
- Corrected transparency when rectangular textures are resized for accelerators that require square textures.
- Other players' failed secondary shots no longer play the local empty-weapon warning.
- Kept the mission timer running and multiplayer chat entry visible while the main HUD is disabled.

### Regressions

- HOVER-to-TRACKED transformations incorrectly succeed over water because the water restriction was removed.

### Known issues

- Destroyed enemies can continue to trigger proximity mines in single-player, even after their visible models disappear. Present since [Demo 1998-07-21](#demo-1998-07-21).
- Mission 3 turret `tur_202` uses the ordinary pulse-turret model but fires enhanced red pulse ammunition.
- High frame rates can cause uneven or accelerated vehicle movement. Vehicle physics advances even on frames with little or no measured elapsed time, and timing precision degrades after long Windows uptimes. Present since [Demo 1998-07-21](#demo-1998-07-21).
- Enemy nanite and most ammunition drops stop appearing when that pickup type's instance counter reaches 100. Collecting pickups does not reset the counter, and failed nanite drops also suppress the alternative ammunition reward. Present since [Demo 1998-07-21](#demo-1998-07-21).
- Easy and Hard vehicle parameters and pickup layouts fall back to Normal when the matching difficulty files exist only inside ZBD archives. The file check searches only for loose files.
- Failed saves can leave incomplete files without a retry prompt. Fixed in [1.0.6.8](#1068--retail).
- Loading textures with embedded palettes clears their transparency settings and can leak an accompanying alpha mask. Regression introduced in [Demo 1998-09-28](#demo-1998-09-28); fixed in [1.0.6.8](#1068--retail).
- Player names can be truncated incorrectly, and hosting accepts an empty name. Fixed in [1.0.6.8](#1068--retail).
- Temporary weapon-hit effects are lost across saves. Fixed in [1.0.6.8](#1068--retail).
- Ammunition received for another variant can overwrite the displayed counter, and weapon-state cleanup resets only the active attachment. Fixed in [1.0.6.8](#1068--retail).
- Multiplayer self-kills can award a kill. Fixed in [1.0.6.8](#1068--retail).
- Remote secondary shots can use the currently selected weapon instead of the weapon that fired. Fixed in [1.0.6.8](#1068--retail).
- Helicopter sounds remain enabled in SUBMARINE mode. Fixed in [1.0.6.8](#1068--retail).
- Empty chat messages can be displayed and sent. Fixed in [1.0.6.8](#1068--retail).
- Objective controls can interrupt the final-mission results sequence. Fixed in [1.0.6.8](#1068--retail).
- Mouse capture can become inconsistent after focus changes or repeated acquisition requests. Fixed in [1.0.6.8](#1068--retail).
- Cached visual effects leak during mission cleanup. Fixed in [1.0.6.8](#1068--retail).
- Destroyed vehicles can collect nearby pickups. Fixed in [1.0.6.8](#1068--retail).
- Weapon ammunition can fall below zero when firing consumes the remaining reserve. Fixed in [1.0.6.8](#1068--retail).
- Secondary-weapon force feedback can play even when firing fails. Fixed in [1.0.6.8](#1068--retail).
- Heavy combat can overflow the pending weapon-hit queue. Fixed in [1.0.6.8](#1068--retail).
- Beam and trail allocations leak when secondary-weapon banks are reloaded, vehicles are removed, or turrets are cleared. Fixed in [1.0.6.8](#1068--retail).
- Pending pickup respawns are not cleared during mission cleanup. Fixed in [1.0.6.8](#1068--retail).
- Regaining focus does not restore minimized fullscreen windows. Fixed in [1.0.6.8](#1068--retail).
- Terrain contact state from the destruction location can prevent a respawning vehicle from returning to TRACKED mode. Fixed in [1.0.6.8](#1068--retail).
- Switching secondary weapons can clear the new weapon's projectile reference instead of the stowed weapon's reference. Fixed in [1.0.6.8](#1068--retail).
- Empty secondary weapons can redeploy after their last guided projectile finishes. Fixed in [1.0.6.8](#1068--retail).
- Remote pickup-removal messages can interrupt a local collection animation. Fixed in [1.0.6.8](#1068--retail).
- Loading a saved mission timer can leave an earlier objective display open. Fixed in [1.0.6.8](#1068--retail).
- Paletted textures with uniform transparency can display incorrect colors in RGB565 software rendering.
- Some asset and save warnings are silent; other diagnostic output remains available.
- With CD Audio enabled, entering or resuming a mission can crash if the CD drive reports exactly two tracks. Regression introduced in [Demo 1998-09-28](#demo-1998-09-28).
- Disabling an object's texture scrolling through `Object3DSetScroll` or `Object3DSetScrollAlways` can crash the game. Regression introduced in [Demo 1998-09-28](#demo-1998-09-28).
- Secondary weapons retract without their movement sound. Fixed in [1.0.6.8](#1068--retail).

## Demo 1998-09-28

### Added

- Added separate ground and water impact effects for mortars and missiles, including impacts in quicksand.
- Added selectable A3D audio alongside DirectSound, with DirectSound fallback if A3D initialization fails.
- Added separate HUD preferences for software and hardware rendering.
- Added on-screen warnings when the BFT is not equipped for deep water or hot lava, alongside the existing audio alerts.
- Added support for combinations of Ctrl, Alt, and Shift in control bindings.
- Added a throttle-mode shortcut with a confirmation message.
- Added optional movie-audio streaming selected by movie scripts; full-track preloading remains available.
- Added error dialogs for failed multiplayer host-index assignment and client index-assignment timeout.
- Added fatal-error dialogs for out-of-memory, file-access, and other application exceptions, with specific explanations for file errors.
- Added weapon-definition settings for relative launch speed and a delay before projectile steering begins.
- Expanded engine scripting with separate near and far fog-distance controls and fog-range and backface-tolerance queries.
- Added the `NodeSetOverwrite` script command to draw designated objects over the rest of the scene, including a separate Direct3D drawing pass.
- Added detailed session-opening errors in the Windows session browser, including password, timeout, authentication, and security failures.
- Added automatic playback of mission briefings for objectives marked `AUTOPLAY`.

### Changed

- Increased small homing-missile damage from 10 to 15. Retuned enemy vehicles with faster missile fire and a higher ride height for the Hover Tank, and slower attacks for the Laser Designator Tank.
- Made skids, splashes, turret movement, transformations, weapon-lock sounds, ricochets, and several weapon effects positional. Retuned their audible ranges, including shorter ranges for fire crackling and transformations.
- Made automatic graphics defaults more conservative on slower processors, scaling effects, object detail, global lighting, perspective correction, and HUD style by CPU speed. Medium sound quality now depends on available memory instead of CPU speed.
- Expanded effect-detail scaling across weapon explosions, dirt and smoke bursts, parachutes, and vehicle destruction. Boat wakes now require High effects detail; primary-gun muzzle lighting requires at least Medium.
- Suppressed flying crew-body effects at Low effects detail while retaining their screams, and added extra fire and debris to helicopter destruction at High detail.
- Repositioned flames on burning vehicles and added distance-dependent screen flashes to orange flares.
- Limited colored explosion flashes to nearby blasts, shortened their expansion and fade, and removed the extra large light flash from generic destruction effects.
- Shortened smoke, small-explosion fragments, and SAM wreckage lifetimes; reduced the range of small-explosion lighting.
- Restored higher-resolution pickup textures in the 2 MB hardware texture set and selected force-field, generator, and fuel textures in the 4 MB set. Updated lightning and crater graphics.
- Revised the demo introduction, exit, and credits artwork, and expanded the Graphics panel to accommodate Full HUD.
- Revised the multiplayer BFT’s weapon and shadow models, Mission 1’s underwater geometry, and the demo multiplayer arena’s vents and structures.
- Based Mission 1’s extraction trigger on the exit gate lowering rather than on destruction of one particular gate panel.
- Spawned the hidden nuclear-weapon and nanite rewards from Mission 1’s van and crane crate through the pickup system.
- Control rebinding now identifies whether a keyboard key, mouse button, or joystick button is expected.
- The Controls dialog now suspends normal keyboard commands and clears held-key state when closed.
- Mission CD music now adapts to the disc's track count instead of cycling through three fixed tracks.
- Skid sounds now stop when a collision is resolved in HOVER mode.
- Primary fire now pauses for one second after changing vehicle form.
- Vehicle movement speed is no longer reduced according to remaining health.
- Model fog now accounts for distance to the side of the camera as well as forward distance.
- Scripted screen fades now use the selected accelerator when hardware rendering is active.
- Multiplayer setup now labels the match goal as kills or laps according to the selected world.
- Expanded accelerator-selection guidance to cover non-3Dfx hardware and the software-rendering fallback.
- Adjusted collision clearance around the moving vehicle.
- Disabled automatic weapon airdrops while the selected secondary weapon has infinite ammunition.
- Increased corrective forces on steep slopes and applied them during vertical-motion updates.
- Disabled normal enemy pickup drops while the selected secondary weapon has infinite ammunition, and nanite repair drops while invincibility is active.
- Multiplayer weapon airdrops now select from the level's available pickup types instead of the local player's collected weapons.
- Airdropped pickups now inherit the visibility area of the terrain beneath them.
- Reapplied joystick, cursor, camera, throttle, and steering preferences after loading a saved game.
- Removed match-timer, diagnostic-display, and overhead-camera commands from the retained cheat command set. Ammunition, repair, and invincibility remain; normal cheat entry is still unavailable.
- Made the overhead camera available through **G** without a cheat.
- Removed the **Shift+O** diagnostic-display shortcut, toggle action, and cheat unlock.
- Removed the repair cheat's numeric health-adjustment form and added restoration of the applicable secondary-weapon attachment. Normal cheat entry remains unavailable.
- Configuration loading now uses compiled data exclusively. Removed loose-source fallback, source-timestamp checks, and the source parser's oversized-token message.
- Removed the shutdown effect-usage report from `recoil.out`, including memory usage and effects created, instanced, and reused.
- Increased the multiplayer results-screen timeout from 60 seconds to 10 minutes.
- Added argument-count and node-type validation to fog-range, fog-state, backface-tolerance, and selected window script commands.
- Boolean script arguments now accept `true` as well as `on` and require a whole-token match.
- Reduced memory use for scene objects and switched world ZBD files from format 13 to format 15. Earlier world files require conversion.
- Kept the fallback image bank (`image.zbd` or `rimage.zbd`) open between image lookups instead of reopening its directory for each lookup.
- Reduced system-memory use by releasing redundant texture data after hardware upload.
- Reworked multiplayer race starts around a host-triggered countdown and a shared starting gate.
- Multiplayer player-color assignments now use reliable delivery.
- Sensor line-of-sight checks for vehicles and turrets now use the camera's target point instead of the BFT's position.
- Disabled primary-gun recoil animation in SUBMARINE mode.
- Heat and freezing effects now wear off faster. Reduced the hold before recovery from 3 to 2.5 seconds for freezing and from 5 to 2.5 seconds for laser heating, and accelerated the subsequent decay.
- Homing projectiles now turn toward their targets in bounded angular steps instead of easing toward the target direction.
- Projectile and beam collision checks now use the firing vehicle's visibility area.

### Fixes

- Kept the hum at Mission 1’s northern-base building playing until the building is disabled instead of stopping it when the first power bay is destroyed.
- Corrected primary-gun positioning during multiplayer SUBMARINE transformations.
- Corrected track references on minelayer and missile-tank models used by their destruction animations.
- Reset impact-fragment positions before reusing effects, attached a SAM debris trail to the correct fragment, and stopped parachute flares and sway during cleanup.
- Prevented helicopter rotor animations from restarting after the helicopter is destroyed.
- Added a usable texture-size limit when a graphics driver reports a zero maximum width or height.
- Incompatible saved vehicle records are now skipped without applying them or displaying an incompatibility warning.
- Restored animation-controlled lights and sounds when loading, including their position, attachment, and active state.
- Adjusted the SUBMARINE chase camera to avoid obstructed views.
- Replaced incompatible secondary weapons when entering SUBMARINE mode. Automatic selection now checks mode compatibility, ownership, and ammunition for each variant.
- Added distinct multiplayer join errors for player-capacity, invalid-parameter, missing-connection, and lost-connection failures.
- Added a visible error dialog when the multiplayer session is lost.
- Weapon pickups now preserve cheat-granted infinite ammunition. Normal cheat entry remains unavailable in this demo.
- Synchronized removal of respawning multiplayer pickups when their collection animation finishes, preventing other players from keeping those pickups active during the respawn delay.
- Fixed power-of-two texture-size validation before upload.
- Fixed damage marks not updating hardware-rendered textures.
- Prevented raycastable projectile effects from blocking their own blast damage.
- Closing objective review now stops overlapping instances of its audio as well as the main instance.
- Released pending sound requests during audio shutdown.
- Rejected archives too short to contain an index trailer before attempting to read their contents.
- Allowed indexed resource archives to load from read-only files and media, and allowed other readers to keep them open while the game runs.
- Skipped missing or empty sound-bank entries during sample loading.
- Capped the displayed Enemies Killed count at the mission's enemy total.
- Removed the misleading “3Dfx” label from the Windows title when another renderer is selected.
- Corrected scrolling-text positions and visibility bounds when the containing panel is offset from the screen origin.

### Regressions

- With CD Audio enabled, entering or resuming a mission can crash if the CD drive reports exactly two tracks.
- Loading textures with embedded palettes clears their transparency settings and can leak an accompanying alpha mask. Fixed in [1.0.6.8](#1068--retail).
- Disabling an object's texture scrolling through `Object3DSetScroll` or `Object3DSetScrollAlways` can crash the game.

### Known issues

- Destroyed enemies can continue to trigger proximity mines in single-player, even after their visible models disappear. Present since [Demo 1998-07-21](#demo-1998-07-21).
- High frame rates can cause uneven or accelerated vehicle movement. Vehicle physics advances even on frames with little or no measured elapsed time, and timing precision degrades after long Windows uptimes. Present since [Demo 1998-07-21](#demo-1998-07-21).
- Enemy nanite and most ammunition drops stop appearing when that pickup type's instance counter reaches 100. Collecting pickups does not reset the counter, and failed nanite drops also suppress the alternative ammunition reward. Present since [Demo 1998-07-21](#demo-1998-07-21).
- Easy and Hard vehicle parameters, pickup layouts, and AI vehicle placements fall back to Normal when the matching difficulty files exist only inside ZBD archives. The file check searches only for loose files.
- Incompatible saved player records are applied without a layout check. Fixed in [1.0.1.23](#10123--full-game).
- Saved games omit difficulty, and loading does not fully restore completed objective markers or weapon displays and selection. Fixed in [1.0.1.23](#10123--full-game).
- Objective review can pass the end of the list. Fixed in [1.0.1.23](#10123--full-game).
- Disabling the main HUD also stops its mission timer and hides multiplayer chat entry. Fixed in [1.0.1.23](#10123--full-game).
- Joysticks with fewer axes and repeated enable/disable requests are mishandled. Fixed in [1.0.1.23](#10123--full-game).
- The active secondary-weapon attachment is not fully reset after damage or destruction. Fixed in [1.0.1.23](#10123--full-game); resetting both variants in every bank is added in [1.0.6.8](#1068--retail).
- Network receiving can continue after the lost-session alert. Fixed in [1.0.1.23](#10123--full-game).
- Mouse capture can become inconsistent after focus changes or repeated acquisition requests. Fixed in [1.0.6.8](#1068--retail).
- Cached visual effects leak during mission cleanup. Fixed in [1.0.6.8](#1068--retail).
- Paletted textures with uniform transparency can display incorrect colors in RGB565 software rendering.
- Some asset and save warnings are silent; other diagnostic output remains available.
- Acceleration, renderer API, and Fullscreen selections are not retained between launches. Fixed in [1.0.1.23](#10123--full-game).
- Audio options can continue to show A3D after initialization falls back to DirectSound. Fixed in [1.0.1.23](#10123--full-game).
- Returning from a menu can restart sound instances that were not playing when the menu opened. Regression introduced in [Demo 1998-08-12](#demo-1998-08-12); fixed in [1.0.1.23](#10123--full-game).
- Rectangular textures can have misaligned transparency on accelerators that require square textures. Regression introduced in [Demo 1998-08-12](#demo-1998-08-12); fixed in [1.0.1.23](#10123--full-game).
- A failed automatic HOVER transformation can bypass lava damage. Fixed in [1.0.1.23](#10123--full-game).
- Weapon effects can intercept their own ground-impact checks. Fixed in [1.0.1.23](#10123--full-game).
- Secondary weapons retract without their movement sound. Fixed in [1.0.6.8](#1068--retail).

## Demo 1998-08-12

### Added

- Added a repeating low-shield alarm alongside the spoken warning.
- Added access to the cave behind Mission 1’s destructible shack.
- Expanded Mission 1’s placed supplies with additional enhanced pulse-gun ammunition, mortar and napalm pickups, proximity mines, and nanite repairs.
- Added power-up audio to the first mission's startup sequence, stopping it when the opening objective briefing begins.
- Added support for non-power-of-two textures on accelerators that support those dimensions.
- Added a delayed return to the menu after the player's destruction in single-player.
- Added a Name Tags option for hosted multiplayer games.
- Added mission-specific startup actions after loading a saved game.
- Added multiplayer match restarts within the current session, with revised settings synchronized to connected players.
- Added a steering-mode shortcut that resets and recenters mouse control.
- Added a 60-second multiplayer results-screen timeout that displays a warning and exits the game.
- Added per-entry playback limits within repeating sound groups.
- Added engine script commands to query camera and object positions and adjust the near and far clipping planes separately.

### Changed

- Retuned TRACKED acceleration and damping, AMPHIBIOUS speed, buoyancy, and turning, and SUBMARINE drag. Reshaped the HOVER, AMPHIBIOUS, and SUBMARINE collision bounds.
- Retuned movement through quicksand and lava for ground vehicles, and reduced minelayer health from 86 to 60.
- Reduced Mission 1’s helicopter detection ranges again, to 350 for the first two groups and 250 for the third. Replaced the third group’s napalm cannon with a pulse gun, adjusted firing bursts, and removed line-of-sight gating for these turrets.
- Rebalanced Mission 1’s mine supplies: three existing proximity-mine pickups now provide two mines instead of five; one mortar pickup now provides two rounds instead of one.
- Revised Mission 1’s Hard layout with additional RUMV and Trencher encounters and patrol routes, replacing two duplicate drone entries. The difficulty-loading issue prevents this layout from taking effect in an unmodified installation.
- Changed the demo multiplayer arena’s high-explosive mortar pickup from a weapon pickup to ammunition.
- Shortened the first objective’s spoken briefing from about 30 to 19 seconds and shortened the second demo introduction slide’s display time.
- Removed the preset multiplayer player name, leaving the name field empty initially.
- Expanded Mission 1’s secret-beach scenery with signs and posters, revised its underwater terrain and destructible entrance, and updated the cave and northern-base geometry.
- Updated Mission 1’s animated surf and water textures and reduced selected pickup and force-field texture resolutions in the smaller hardware texture sets.
- Reworked burning-vehicle flames, smoke, and crackling audio, including effect-detail-dependent flame sizes and revised wreck-fire duration.
- Shortened crushed-crate debris and lighthouse wreckage animations and cleaned up their fragments sooner.
- Increased the force-field warning’s trigger distance and reduced the delay before the post-generator warning from 30 to 2 seconds.
- Connected SAM destruction to the turret’s damage state, replacing the separate destruction trigger.
- Separated the missile-pad opening sequence from the repeated launch sequence in Mission 1.
- Restored saving and loading through the menus and shortcuts, subject to the normal gameplay restrictions.
- Updated compiled effect animations (`anim.zbd`) from format 27 to format 28; format 27 files are no longer accepted.
- Loading a saved game during a mission now restarts the loading fade and temporarily mutes audio.
- Connected the effects-volume slider to the game's sound-volume setting.
- Revised HOVER movement with speed-dependent rocking and steering bank, and expanded SUBMARINE bobbing to both pitch and roll.
- Blocked HOVER-to-TRACKED transformations over lava.
- Adjusted collision clearance around the moving vehicle.
- Changed mine bounces to retain 25% of their speed on every impact, replacing frame-rate-dependent damping and the stronger water damping.
- Localized session-browser text and departing-player notifications.
- Expanded localization to menu labels, selection controls, weapon names, and weapon kill messages.
- Reorganized the Windows Options menu into graphics, resolution, and other settings, with an explicit software-rendering choice.
- Allowed secondary weapons to aim at closer targets before falling back to forward aim.
- Restored the multiplayer session browser launched from the Windows menu.
- Sound nodes without an attachment or an explicit position now use non-positional playback.
- Nested effect animations now inherit their parent animation's starting velocity.
- Increased the minimum multiplayer time limit from one to five minutes.
- Added obstruction checks for blast damage against the local multiplayer vehicle.
- Removed normal cheat-dialog access, including the **Ctrl+X** binding. The dialog and cheat commands remain implemented.
- Replaced literal cheat commands, including `ammo`, `health`, `timer`, and `framerate`, with localized, case-insensitive commands that can match anywhere in the entered text. Normal cheat entry remains unavailable.
- Updated the repair cheat to stop an active hit animation before restoring the vehicle. Normal cheat entry remains unavailable.
- Information displayed when collecting a new pickup now closes automatically after a short delay.
- Repeated hits now restart the vehicle's hit animation.
- Forced player destruction now clears the displayed nanite count along with the remaining reserve.
- Expanded the starting multiplayer loadout with an arena-specific secondary weapon.
- Extended infinite-ammunition handling to continuously firing secondary weapons. Normal cheat entry remains unavailable.

### Fixes

- Prevented a supply crate from running both its collision and weapon-hit destruction sequences.
- Kept the core weapon-kill explosion visible at long range while limiting additional rings and lighting by distance.
- Prevented destroyed northern-base sirens, power-bay beams, and the northern-base force field from being reactivated by their proximity triggers.
- Improved multiplayer name-tag placement and suppressed labels that would be clipped at the top of the screen.
- Fixed truncation of long sound-group names during configuration loading.
- Fixed Direct3D loading software texture archives (`texture*.zbd`) instead of hardware texture archives (`rtexture*.zbd`) on devices reporting more than 8 MB of texture memory.
- Preserved running mission animations, associated objects, and active commands across saves.
- Fixed cleanup of a departing multiplayer participant whose vehicle has not spawned.
- Added support for rectangular textures on accelerators that require square textures.
- Kept very quiet sounds within DirectSound's supported volume range.
- Fixed display restoration when a surface is absent. Mode changes also restore existing surfaces before rebuilding them.
- Restored normal sound levels and playback when closing objective review.
- Recentered mouse steering when returning from the Controls menu.
- Accuracy statistics now count only the local player's shots and hits.
- Preserved weapon-hit counts and accuracy across saves.
- Ignored late firing, kill, and projectile-removal messages involving players who have already left, preventing multiplayer crashes.
- Closing objective review now stops the main playback instance of its audio.
- Stopped the main warning-sound instances when the player's vehicle is destroyed.
- Inactive turrets no longer appear as active sensor targets.
- Handled missing numeric and Boolean script arguments without crashing.

### Regressions

- Unsupported texture dimensions can pass validation on accelerators requiring power-of-two sizes and fail during upload. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Returning from a menu can restart sound instances that were not playing when the menu opened. Fixed in [1.0.1.23](#10123--full-game).
- Raycastable projectile effects can obstruct their own blast-damage checks. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Rectangular textures can have misaligned transparency on accelerators that require square textures. Fixed in [1.0.1.23](#10123--full-game).
- The Windows title always displays “RECOIL (3Dfx),” regardless of the selected renderer. Fixed in [Demo 1998-09-28](#demo-1998-09-28).

### Known issues

- Destroyed enemies can continue to trigger proximity mines in single-player, even after their visible models disappear. Present since [Demo 1998-07-21](#demo-1998-07-21).
- High frame rates can cause uneven or accelerated vehicle movement. Vehicle physics advances even on frames with little or no measured elapsed time, and timing precision degrades after long Windows uptimes. Present since [Demo 1998-07-21](#demo-1998-07-21).
- Enemy nanite and most ammunition drops stop appearing when that pickup type's instance counter reaches 100. Collecting pickups does not reset the counter, and failed nanite drops also suppress the alternative ammunition reward. Present since [Demo 1998-07-21](#demo-1998-07-21).
- Indexed resource archives require write access even when only being read, preventing loading from read-only files or media. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Easy and Hard vehicle parameters, pickup layouts, and AI vehicle placements fall back to Normal when the matching difficulty files exist only inside ZBD archives. The file check searches only for loose files.
- Overlapping copies of objective and warning sounds can keep playing after a stop request. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Scrolling text ignores the containing panel's position when placing text and determining which lines are visible. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- The displayed Enemies Killed count can exceed the mission's enemy total. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Weapon pickups can reset a variant's infinite ammunition to its normal maximum if the effect is active. Normal cheat entry is unavailable. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Graphics drivers reporting a zero maximum texture size are not accommodated. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Saved running animations omit the state of their attached lights and sounds. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Incompatible saved vehicle records are applied without a layout check. Fixed in [Demo 1998-09-28](#demo-1998-09-28); player-record validation is added in [1.0.1.23](#10123--full-game).
- The SUBMARINE chase camera can be obstructed, and automatic secondary-weapon selection can choose an incompatible weapon. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Some multiplayer join failures and session loss lack useful error messages. Improved in [Demo 1998-09-28](#demo-1998-09-28).
- Collected respawning pickups can remain active for other players during the respawn delay. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Mouse capture can become inconsistent after focus changes or repeated acquisition requests. Fixed in [1.0.6.8](#1068--retail).
- Cached visual effects leak during mission cleanup. Fixed in [1.0.6.8](#1068--retail).
- Paletted textures with uniform transparency can display incorrect colors in RGB565 software rendering.
- Some asset and save warnings are silent; other diagnostic output remains available.
- Acceleration, renderer API, and Fullscreen selections are not retained between launches. Fixed in [1.0.1.23](#10123--full-game).
- Secondary weapons retract without their movement sound. Fixed in [1.0.6.8](#1068--retail).

## Demo 1998-07-23

### Added

- Added Mission 1’s secret beach, unlocked by destroying all four northern-base sirens, with a destructible inner entrance.

### Changed

- Halved the firing rate of Mission 1’s pulse-gun helicopter turrets and reduced their detection range from 500 to 400. Reduced the napalm helicopter turret’s detection range from 400 to 300.
- Reduced minelayer texture resolution in the 2 MB hardware texture set and one dock texture in the 4 MB set.
- Revised the nanite and nuclear-weapon pickup models and their collision bounds, along with the truck and trailer models.
- Disabled saving and loading for the demo, with explanatory messages on the corresponding shortcuts.
- Updated compiled effect animations (`anim.zbd`) from format 26 to format 27; format 26 files are no longer accepted.
- Changed the runtime version label from 0.9 to 0.7.
- Reduced updates for visibility-dependent animations while their objects are outside the rendered scene.
- Adjusted vertical-motion smoothing when the vehicle has only partial ground contact.
- Removed the multiplayer session-browser command from the Windows menu.
- Explosion damage now preserves the projectile's original impact direction for vehicle hit reactions.
- Reworked software texture mapping for polygons with vertices near or behind the camera.

### Fixes

- Sensor-map files now load from `.\maps` instead of the development path `..\run\maps`.

### Regressions

None recorded.

### Known issues

- Destroyed enemies can continue to trigger proximity mines in single-player, even after their visible models disappear. Present since [Demo 1998-07-21](#demo-1998-07-21).
- High frame rates can cause uneven or accelerated vehicle movement. Vehicle physics advances even on frames with little or no measured elapsed time, and timing precision degrades after long Windows uptimes. Present since [Demo 1998-07-21](#demo-1998-07-21).
- Enemy nanite and most ammunition drops stop appearing when that pickup type's instance counter reaches 100. Collecting pickups does not reset the counter, and failed nanite drops also suppress the alternative ammunition reward. Present since [Demo 1998-07-21](#demo-1998-07-21).
- Indexed resource archives require write access even when only being read, preventing loading from read-only files or media. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Easy and Hard vehicle parameters, pickup layouts, and AI vehicle placements fall back to Normal when the matching difficulty files exist only inside ZBD archives. The file check searches only for loose files.
- Missing numeric or Boolean script arguments can crash the game. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Objective audio can continue after closing objective review. Main playback is fixed in [Demo 1998-08-12](#demo-1998-08-12); overlapping playback is fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Accuracy statistics can include shots and hits from other vehicles. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Late firing, kill, and projectile-removal messages involving players who have left can crash the game. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Long sound-group names are truncated during configuration loading and cannot be found by their full names. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Weapon pickups can reset the collected variant's infinite ammunition to its normal maximum after the `ammo` cheat. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Direct3D can load software texture archives (`texture*.zbd`) instead of hardware texture archives (`rtexture*.zbd`) on devices reporting more than 8 MB of texture memory. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Rectangular textures are not adapted for square-only accelerators. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Very quiet nonzero sounds can request an invalid volume. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Display restoration can access a missing surface. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Cleanup can crash when a departing multiplayer participant has no spawned vehicle. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Multiplayer name tags can be poorly positioned or clipped at the top of the screen. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Collected respawning pickups can remain active for other players during the respawn delay. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Mouse capture can become inconsistent after focus changes or repeated acquisition requests. Fixed in [1.0.6.8](#1068--retail).
- Cached visual effects leak during mission cleanup. Fixed in [1.0.6.8](#1068--retail).
- Paletted textures with uniform transparency can display incorrect colors in RGB565 software rendering.
- Some asset and save warnings are silent; other diagnostic output remains available.
- Acceleration, renderer API, and Fullscreen selections are not retained between launches. Fixed in [1.0.1.23](#10123--full-game).
- Secondary weapons retract without their movement sound. Fixed in [1.0.6.8](#1068--retail).

## Demo 1998-07-22

### Added

- Added four destructible sirens and rotating red warning lights to Mission 1’s northern base.
- Added a spoken warning when approaching Mission 1’s force field.
- Added a checked menu option for enabling or disabling joystick input.
- Added engine script control over lighting for a node and its descendants.

### Changed

- Revised Mission 1’s terrain, northern-base retaining walls, helicopter model, and truck wreckage.
- Reworked the demo multiplayer arena’s buildings and spillways.
- Retimed the destruction effects for Mission 1’s northern-base building and removed duplicate debris bursts.
- Reduced saved and networked animation history by excluding short-lived splashes, wakes, exhaust, regeneration flashes, and multiplayer vehicle effects.
- Reworked player-name, save-name, cheat-code, and multiplayer text entry, with filtering for numeric fields.
- Bouncing mines now update their position, lose speed, and play bounce sounds at each successive impact within the same frame.
- Limited the menu's blur transition to opening the menu during a mission.
- Adjusted software-rendered model lighting with limits on individual light contributions and revised distance falloff.

### Fixes

- Restored the correct track, wing, door, and shadow visibility when resetting the multiplayer BFT’s form.
- Made each missile-site progress announcement in Mission 1 play only once.
- Fixed double-counting of newly joined multiplayer participants.
- Fixed blending of translucent Direct3D polygons.
- Corrected lighting and color on untextured Direct3D polygons, including polygons clipped at the edge of the view.
- Fixed TRACKED shadows updating one frame late when leaving the ground or landing.
- Prevented ground-plane alignment from overriding airborne movement.
- Escape during multiplayer now opens the multiplayer exit panel instead of the single-player menu.

### Regressions

None recorded.

### Known issues

- Destroyed enemies can continue to trigger proximity mines in single-player, even after their visible models disappear. Present since [Demo 1998-07-21](#demo-1998-07-21).
- High frame rates can cause uneven or accelerated vehicle movement. Vehicle physics advances even on frames with little or no measured elapsed time, and timing precision degrades after long Windows uptimes. Present since [Demo 1998-07-21](#demo-1998-07-21).
- Enemy nanite and most ammunition drops stop appearing when that pickup type's instance counter reaches 100. Collecting pickups does not reset the counter, and failed nanite drops also suppress the alternative ammunition reward. Present since [Demo 1998-07-21](#demo-1998-07-21).
- Indexed resource archives require write access even when only being read, preventing loading from read-only files or media. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Easy and Hard vehicle parameters, pickup layouts, and AI vehicle placements fall back to Normal when the matching difficulty files exist only inside ZBD archives. The file check searches only for loose files.
- Missing numeric or Boolean script arguments can crash the game. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Objective audio can continue after closing objective review. Main playback is fixed in [Demo 1998-08-12](#demo-1998-08-12); overlapping playback is fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Accuracy statistics can include shots and hits from other vehicles. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Late firing, kill, and projectile-removal messages involving players who have left can crash the game. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Loading a save loses weapon-hit counts and changes the accuracy statistic. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Long sound-group names are truncated during configuration loading and cannot be found by their full names. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Weapon pickups can reset the collected variant's infinite ammunition to its normal maximum after the `ammo` cheat. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Direct3D can load software texture archives (`texture*.zbd`) instead of hardware texture archives (`rtexture*.zbd`) on devices reporting more than 8 MB of texture memory. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Rectangular textures are not adapted for square-only accelerators. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Very quiet nonzero sounds can request an invalid volume. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Display restoration can access a missing surface. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Collected respawning pickups can remain active for other players during the respawn delay. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Mouse capture can become inconsistent after focus changes or repeated acquisition requests. Fixed in [1.0.6.8](#1068--retail).
- Cached visual effects leak during mission cleanup. Fixed in [1.0.6.8](#1068--retail).
- Paletted textures with uniform transparency can display incorrect colors in RGB565 software rendering.
- Some asset and save warnings are silent; other diagnostic output remains available.
- Acceleration, renderer API, and Fullscreen selections are not retained between launches. Fixed in [1.0.1.23](#10123--full-game).
- Sensor maps are not found when installed under `.\maps`; the loader expects `..\run\maps`. Fixed in [Demo 1998-07-23](#demo-1998-07-23).
- Cleanup can crash when a departing multiplayer participant has no spawned vehicle. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Secondary weapons retract without their movement sound. Fixed in [1.0.6.8](#1068--retail).

## Demo 1998-07-21

Earliest available build and comparison baseline.

### Known issues

- Destroyed enemies can continue to trigger proximity mines in single-player, even after their visible models disappear.
- High frame rates can cause uneven or accelerated vehicle movement. Vehicle physics advances even on frames with little or no measured elapsed time, and timing precision degrades after long Windows uptimes.
- Enemy nanite and most ammunition drops stop appearing when that pickup type's instance counter reaches 100. Collecting pickups does not reset the counter, and failed nanite drops also suppress the alternative ammunition reward.
- Indexed resource archives require write access even when only being read, preventing loading from read-only files or media. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Easy and Hard vehicle parameters, pickup layouts, and AI vehicle placements fall back to Normal when the matching difficulty files exist only inside ZBD archives. The file check searches only for loose files.
- Missing numeric or Boolean script arguments can crash the game. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Objective audio can continue after closing objective review. Main playback is fixed in [Demo 1998-08-12](#demo-1998-08-12); overlapping playback is fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Ground-plane alignment can override airborne movement. Fixed in [Demo 1998-07-22](#demo-1998-07-22).
- Untextured Direct3D polygons can use incorrect lighting and color, including at clipped edges. Fixed in [Demo 1998-07-22](#demo-1998-07-22).
- Accuracy statistics can include shots and hits from other vehicles. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Late firing, kill, and projectile-removal messages involving players who have left can crash the game. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Loading a save loses weapon-hit counts and changes the accuracy statistic. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Long sound-group names are truncated during configuration loading and cannot be found by their full names. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Newly joined multiplayer participants can be counted twice. Fixed in [Demo 1998-07-22](#demo-1998-07-22).
- TRACKED shadows update one frame late when leaving the ground or landing. Fixed in [Demo 1998-07-22](#demo-1998-07-22).
- Escape during multiplayer opens the single-player menu. Fixed in [Demo 1998-07-22](#demo-1998-07-22).
- Some translucent Direct3D polygons use incorrect blending. Fixed in [Demo 1998-07-22](#demo-1998-07-22).
- Weapon pickups can reset the collected variant's infinite ammunition to its normal maximum after the `ammo` cheat. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Direct3D can load software texture archives (`texture*.zbd`) instead of hardware texture archives (`rtexture*.zbd`) on devices reporting more than 8 MB of texture memory. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Rectangular textures are not adapted for square-only accelerators. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Very quiet nonzero sounds can request an invalid volume. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Display restoration can access a missing surface. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Collected respawning pickups can remain active for other players during the respawn delay. Fixed in [Demo 1998-09-28](#demo-1998-09-28).
- Mouse capture can become inconsistent after focus changes or repeated acquisition requests. Fixed in [1.0.6.8](#1068--retail).
- Cached visual effects leak during mission cleanup. Fixed in [1.0.6.8](#1068--retail).
- Paletted textures with uniform transparency can display incorrect colors in RGB565 software rendering.
- Some asset and save warnings are silent; other diagnostic output remains available.
- Acceleration, renderer API, and Fullscreen selections are not retained between launches. Fixed in [1.0.1.23](#10123--full-game).
- Sensor maps are not found when installed under `.\maps`; the loader expects `..\run\maps`. Fixed in [Demo 1998-07-23](#demo-1998-07-23).
- Cleanup can crash when a departing multiplayer participant has no spawned vehicle. Fixed in [Demo 1998-08-12](#demo-1998-08-12).
- Secondary weapons retract without their movement sound. Fixed in [1.0.6.8](#1068--retail).
