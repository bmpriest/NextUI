# NextUI port: Miyoo A30 (`my282`)

## Objective

Port current NextUI to the Miyoo A30 without regressing the common frontend.
The first playable release must provide:

- reliable boot and update behavior;
- the NextUI launcher and MinArch;
- all built-in controls, including the UART analog stick;
- correct 640x480 output on the portrait-mounted panel;
- audio, saves, suspend/resume, shutdown, brightness, volume, battery, CPU
  control, and rumble;
- the stock systems currently supported by the MinUI `my282` target.

Wi-Fi, RetroAchievements, advanced shaders, display calibration, and additional
cores are beta milestones rather than alpha blockers.

## Current progress

Status as of 2026-08-03, based on live testing on a Miyoo A30 under both
Spruce and a standalone NextUI card:

### Completed and hardware-validated

- Reproducible Cortex-A7 hard-float toolchain using the stock glibc 2.23 ABI.
- Diagnostic and settings builds against stock Spruce SDL2, EGL/GLES2, ALSA,
  and Mali libraries.
- Physical display characterization: 480x640 portrait scanout, logical
  640x480 UI, Mali-400 MP, and OpenGL ES 2.0.
- A correctly rotated, accelerated, full-screen, and responsive NextUI menu.
  The implementation is isolated under `workspace/my282`; the shared
  `workspace/all/common/generic_video.c` is unchanged.
- Six-row 640x480 menu layout with no overlap against the bottom input hints.
- Spruce ROM-folder discovery and NextUI directory navigation.
- Direct GPIO input for D-pad, face buttons, shoulders, and volume controls.
- UART analog-stick decoding directly through the physical `/dev/ttyS2`,
  retaining `/dev/ttyS0` only as a compatibility fallback.
- GBA hardware validation of L/R and analog-to-D-pad gameplay input.
- Successful MinArch save-state and load-state round trip during GBA gameplay.
- Native A30 builds of the initial six-core set: FCEUmm, Gambatte, gpSP,
  PicoDrive, Snes9x 2005, and PCSX-ReARMed. All six are reproducible
  32-bit ARMv7 EABI5 hard-float outputs. gpSP and PCSX-ReARMed include their
  ARM dynarecs; gpSP, PicoDrive, and PCSX-ReARMed include NEON paths.
- Native gpSP hardware validation with playable Mario Kart video, audio, and
  controls.
- Native FCEUmm hardware validation with 1942: smooth gameplay, working
  buttons and analog-to-D-pad input, and no observed functional issues. Audio
  fidelity remains unjudged because the tester was unfamiliar with the game.
- Native Gambatte hardware validation with Tetris: gameplay, presentation,
  audio, and exercised controls were all reported good.
- Native PicoDrive validation with Sonic the Hedgehog 2: performance and all
  exercised controls were good. A narrow corrupted strip appeared at the
  physical bottom edge. The save-state preview contains the same strip at the
  left edge of the logical frame (before A30 rotation), ruling out the A30
  presentation shim. The identical artifact occurs under Spruce and on a
  TrimUI Brick, confirming upstream game/core border behavior rather than a
  my282 port defect.
- Native Snes9x 2005 hardware validation with Super Mario World. The initial
  forced-Blargg-APU build was laggy and showed visible tearing. Rebuilding with
  the core's default fast APU made gameplay smooth and responsive. A follow-up
  run at the normal Auto CPU policy was indistinguishable from the fixed
  Performance run, so SFC does not require a forced high clock.
- Brightness and software-volume access through `libmsettings`.
- Battery/charging, CPU frequency, temperature, firmware, rumble, and basic
  platform telemetry implementations.
- Recovery-wrapped test launch that suspends Spruce frontend/input services and
  restores them after NextUI exits.
- Recovery-wrapped launch with all private SDL2 libraries removed from
  `.system/my282/lib`. The firmware runtime still created the Mali window,
  selected `opengles2`, and displayed the frontend correctly.
- Cold boot from a freshly prepared SD card into standalone NextUI, using the
  firmware's `/usr/miyoo/lib` Mali SDL runtime without Spruce's bind mount.
- Standalone gameplay validation of FC, GB/GBC, GBA, MD, and SFC. The tester
  reported that these systems all launched and worked from the release tree.
- A reproducible 30 MB standalone card tree containing the stock A30 update
  hook and fallback entry points, production frontend supervisor, native A30
  key monitor, shared UI resources, Settings pak, seven native emulator paks,
  and six native cores. It contains no Spruce or RetroArch dependency.
- Standalone `keymon.elf` ownership of Volume +/- and Menu+Volume brightness.
  Live testing confirmed that the physical buttons update the UI indicators
  and the codec's writable `headphone volume` control during gameplay. This
  avoids the unavailable/contended `Soft Volume Master` control.
- Platform-local framebuffer-vsync waits and EGL swap interval 1. Hardware
  testing confirmed that they remove the fixed vertical tear boundary seen in
  PS1, GBA, and GBC without a performance regression.
- Native PCSX-ReARMed hardware validation with an extracted BIN/CUE image at
  60 FPS using its ARMv7 NEON renderer and dynarec. Standard controller is the
  A30 default so its analog stick mirrors the D-pad; DualShock remains a
  per-game option. ZIP input works by full temporary extraction but provides
  no progress indicator and is impractically slow compared with BIN/CUE, CHD,
  or PBP.
- NextUI Wi-Fi configuration and reconnection on the standalone card, including
  RTL8188FU initialization, `wpa_supplicant`, BusyBox DHCP, saved credentials,
  signal/status reporting, and time synchronization through the existing
  network lifecycle.
- A packaged Dropbear 2025.88 SSH toggle, reachable over Wi-Fi with the A30's
  firmware-valid `/bin/ash` account shell. Live SSH was used to validate mixer,
  process, network, and display state.
- Settings launches reliably, identifies the Miyoo A30, and displays About
  without crashing. Its Wi-Fi/Bluetooth workers are explicitly owned and
  joined, and the power monitor now shuts down cooperatively instead of using
  `pthread_cancel`; logs confirm complete clean teardown.
- Correct MinArch exit animation and process handoff. The A30 keeps the panel
  blank during Mali teardown, restores brightness only after the next process
  presents a correctly oriented frame, and uses a logical-FBO screenshot path
  so the normal fade is not vertically inverted. The shared
  `generic_video.c` remains unchanged.
- Correct separation between standard image capture and the A30 exit fade.
  Screenshots, the open-menu game background, and newly saved state previews
  use the generic top-left image orientation, while only the final exit fade
  uses the unflipped logical-FBO capture required by the portrait presentation
  path. Hardware testing confirmed upright menu imagery and state previews
  without regressing the corrected exit transition.
- Hardware-validated safe shutdown. The `/tmp` second stage releases SSH's
  passwd bind, stops SD-backed processes, synchronizes writes, and verifies a
  read-only FAT remount before detaching the card. The A30 keeps an unexplained
  kernel reference after all visible process holders are gone, so normal
  unmount reports `EBUSY`; matching Spruce's production ordering, NextUI uses
  lazy detach only after the read-only state is independently verified. Four
  instrumented shutdown/reboot cycles mounted the card writable afterward with
  no FAT dirty/error messages. The final trace showed `/mnt/SDCARD` absent
  before `poweroff`.
- Hardware-validated hybrid sleep and suspend-to-RAM. After a five-second test
  timeout with external power disconnected, the A30 entered the kernel `mem`
  state, remained asleep until a second Power press, and resumed the game,
  display, controls, audio, saved mixer level, and Wi-Fi connection. SDL audio
  is reopened after wake; restoring the saved volume after that reopen avoids
  the codec's loud default gain. The BSP's `Suspended for 0.000 seconds` message
  is not a valid residence-time measurement because its monotonic clock stops
  in suspend; wall-clock timestamps and physical observation confirmed the
  sustained sleep.

### Implemented but not yet release-validated

- CPU performance/powersave policy changes.
- Remaining direct input combinations, long-press behavior, and power-button
  ownership while Spruce services are not running.
- Read-only-safe launchers fall back to `/tmp` logs rather than allowing shell
  redirection failure to prevent MinArch, Settings, or NextUI from executing.
  The boot remount now supplies both the block device discovered in
  `/proc/mounts` and the mountpoint, as required by the A30 BusyBox build.
- A cross-compiled MinArch executable with ALSA, saves, compressed states,
  rewind, and inert RetroAchievements stubs for the non-networked alpha.
- Screenshot behavior beyond the MinArch exit-transition capture path.

### In progress

- Phase 2 validation depth. All six initial native cores have now been played
  successfully from the standalone package, with representative controls,
  audio, and video validated. Per-core 30-minute sessions, save/state coverage,
  relaunch persistence, rewind constraints, and PS1 long-duration/state tests
  remain before the alpha gate is closed.
- Phase 3 lifecycle work. Wi-Fi, SSH, shutdown, suspend/wake, post-wake audio,
  and Wi-Fi reconnection are hardware-validated; updater integration remains.

### Not started

- Update/install flow beyond deploying the pre-expanded standalone card tree.
- RetroAchievements integration and validation.
- Full core test matrix, long-duration play tests, and release documentation.

### Deferred polish

- The upstream Settings Wi-Fi worker scans every two seconds whenever Settings
  is open and Wi-Fi is enabled, even when the Wi-Fi submenu is not visible.
  Exiting during an active `wpa_cli` scan can briefly freeze the final Settings
  frame while its worker joins. Teardown is clean and scanning stops when the
  Settings process exits. Later, gate scans on submenu visibility or make the
  backend scan operation interruptible; this is not an A30 release blocker.
- Add a visible progress/cancel affordance for full-path core extraction of
  large ZIP archives, especially PS1 images.
- A suspend/resume cycle can produce one or two recoverable ALSA underruns and
  a brief isolated gameplay stutter a few seconds after wake. Audio and
  performance then remain normal. Revisit buffer priming only if longer tests
  show repeated underruns or sustained disruption.

## Sources of truth

Use each source for the area it has exercised in production:

1. `spruceOS` for A30 firmware behavior, input mappings, runtime mounts, power,
   audio, Wi-Fi, display controls, and firmware requirements.
2. MinUI `my282` for the original NextUI/MinUI platform boundary, direct evdev
   input, UART analog decoding, rotation, scaling, and A30-compatible cores.
3. The NextUI H700 port for the current platform API, Allwinner BSP packaging,
   patched SDL Mali-fbdev integration, raw evdev patterns, and diagnostics.
4. The NextUI `my355` port for current new-target organization and lifecycle.
5. Allium for the steward-fu A30 toolchain and its glibc 2.23, EGL/GLES2, SDL2,
   and library packaging details.

Do not copy hardware constants between devices without checking SpruceOS or the
running A30.

## Architectural decisions

### Platform name and ABI

- Platform name: `my282`
- Device name: Miyoo A30
- CPU: Cortex-A7, ARMv7 hard-float, NEON/VFPv4
- Userspace ABI: glibc 2.23
- Logical display: 640x480
- Physical framebuffer: normally 480x640, rotated 270 degrees
- GPU ceiling: OpenGL ES 2.0

The Phase 0 hardware probe confirmed that direct drawable output appears
rotated 90 degrees clockwise to the user. Present the logical 640x480 render
target with a 90-degree counter-clockwise texture transform.

### Input

NextUI will read the hardware directly:

- `/dev/input/event0` for power/volume events where exposed;
- `/dev/input/event3` for GPIO controls;
- `/dev/ttyS2` for the analog stick. This is the physical UART and remains
  present without another frontend. Spruce temporarily creates `/dev/ttyS0`
  as an alias while PyUI runs, so NextUI accepts it only as a fallback.

The Spruce closed-source `joypad` daemon and Spruce's frontend lifecycle are
not part of the initial design; Spruce is used only as a temporary hardware
test harness.
This avoids duplicate input and preserves L2/R2, which are missing from its
virtual controller. MinUI's UART decoder is the starting point, corrected with
Spruce mappings and calibration behavior.

### Graphics

The common UI compositor is retained without changes. The A30 platform wraps
the SDL entry points used by that compositor in a local presentation shim. The
shim:

1. Validate the steward-fu SDL2 Mali-fbdev driver and stock EGL/GLES libraries
   with `workspace/my282/diagnostics`.
2. Request an ES 2.0 context before window creation.
3. Create the SDL window at the physical 480x640 scanout dimensions.
4. Compose the frontend into an accelerated 640x480 SDL render target.
5. Rotate that texture counter-clockwise into the physical window with
   `SDL_RenderGeometry`, then present it with SDL.

This path was validated on hardware as full-screen, correctly oriented,
unclipped, and responsive. It avoids the deformation caused by creating a
640x480 window against the 480x640 drawable, and avoids the severe stalls of a
temporary GPU-to-CPU-to-GPU readback bridge.

The A30 uses six visible menu rows. Seven rows overlap the footer because each
row is 60 pixels at scale 2: the seventh row ends at y=430 while the footer
begins at y=410.

MinArch uses an A30-local GLES2 path that removes ES3-only program-binary and
VAO requirements and rotates a zero-copy logical 640x480 framebuffer into the
portrait scanout. Its exit screenshot reads that logical framebuffer without
the generic landscape row flip, preserving the normal fade orientation.
Additional shaders still require an explicit GLES2-compatible allowlist.

No ES2 accommodation should silently alter the renderer used by existing
platforms.

### Power and audio

Suspend is real `mem` sleep. Before sleep, pause emulation and synchronize
saves. On wake, clear the RTC alarm, restore the display state, and reopen
audio; Spruce explicitly requires audio reinitialization after A30 resume.

Shutdown must synchronize data and follow the A30's SD-card handling behavior.

## Delivery phases and gates

### Phase 0: reproducible toolchain and probe

Status: **complete**.

- Build an ARMv7 diagnostic with the A30 glibc 2.23 sysroot.
- On hardware, capture SDL, EGL/GLES, framebuffer, input, battery, and
  brightness data.

Gate: the probe creates a visible, correctly oriented frame and reports a
working ES2 context or a working accelerated SDL renderer.

### Phase 1: frontend bring-up

Status: **complete on a standalone cold-boot card**. Menu navigation, hardware
adjustments, repeated game/frontend transitions, and instrumented safe
shutdown are hardware-validated.

- Add build/package skeleton.
- Implement basic settings and hardware controls.
- Implement direct buttons and analog input.
- Bring up NextUI using the simplest validated video path.

Gate: cold boot reaches the menu; navigation, brightness, volume, shutdown,
and repeated launches work.

### Phase 2: playable MinArch alpha

Status: **in progress; native core build and first-pass hardware matrix
complete**.

All six native cores launch from the standalone package. Gameplay video,
sound, D-pad, face buttons, GBA L/R, analog-to-D-pad, menu pause/resume, exit
fade/handoff, GBA SRAM creation, and a manual save-state/load-state round trip
are hardware-validated. PCSX-ReARMed runs extracted BIN/CUE at 60 FPS with its
dynarec and NEON renderer. Relaunch persistence, broader per-core save/state
coverage, rewind constraints, and long-duration sessions remain.

- Bring up audio and the initial six systems.
- Validate frame pacing, scaling, overlays, saves, states, rewind constraints,
  and menu transitions.

Gate: a 30-minute play session on each stock core has correct input/audio/video
and survives save/load plus clean shutdown.

### Phase 3: lifecycle beta

Status: **in progress; Wi-Fi and diagnostic SSH hardware-validated**.

- Implement suspend/wake and audio recovery.
- Add Wi-Fi and RetroAchievements.
- Add battery monitoring, time synchronization, screenshots, and updater
  integration.

Gate: repeated sleep/wake, low-battery shutdown, Wi-Fi reconnect, and updater
  tests do not corrupt the SD card or lose saves.

### Phase 4: renderer and release polish

Status: **not started**.

- Expand the ES2 shader allowlist.
- Profile GPU/CPU/memory behavior.
- Test firmware variants and document the minimum supported firmware.
- Complete installation, recovery, and upgrade documentation.

## Hardware test record

Record probe output and test results under `docs/my282-tests/`. Each report
should include firmware version, SDL version, EGL/GLES strings, framebuffer
geometry, library hashes, and the NextUI commit.
