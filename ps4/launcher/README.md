# PS5-style Homebrew Launcher (separate application)

Independent PS4 homebrew application. This project is isolated from the MINEPLUS game sources and package identity.

## Current implementation
- SDL2 interface prototype rendered at 1920x1080.
- Dark blue/black gradient background with restrained blue ambient lighting and optional floating particles. No medieval scenery or MINEPLUS game artwork.
- DualShock 4 D-pad and left-analog navigation, controller hot-plug handling, and PlayStation-style button prompts.
- Five-slot game carousel with selected-tile focus animation and L1/R1 page skips.
- Bounded read-only discovery attempt under `/user/app/*/sce_sys/param.sfo`, parsing `TITLE` and `TITLE_ID` with strict PS4 title-ID validation and self-entry filtering.
- Up to 40 discovered games shown, sorted alphabetically while limiting traversal to 1,024 directory entries and retaining at most 256 readable games.
- Loads real `sce_sys/icon0.png` artwork when accessible, with procedural fallback artwork.
- Caches at most five visible icon textures to bound GPU memory use.
- Triangle rescans the installed-game library without restarting.
- Settings page offers ambient particles and title-ID visibility toggles. The app attempts to persist these values in `/data/launcher_settings.cfg`; actual path permissions and persistence must be verified on-console.
- Launch action calls `sceLncUtilLaunchApp` after attempting System Service/User Service initialization and foreground-user lookup.
- Clear status and error feedback; no fake installed games are presented.

## Not implemented / not verified
- Access to installed retail application directories is not guaranteed from a normal homebrew sandbox. The scan may report access denied or no readable games.
- The Media and Profile sections are navigation placeholders, not functional applications.
- The launch request is wired but has not been tested on a physical PS4. Successful compilation does not establish that the LNC imports, service initialization, user lookup, or app launch work on firmware 14.02 / GoldHEN 2.4b18.14.
- Settings persistence is implemented in source but its writable path and behavior are not confirmed on hardware.
- The interface has not been runtime-tested on a PS4, and controller mapping/display scaling still require device validation.

## Safety and platform limits
The scanner only attempts to read application metadata. It does not modify `app.db`, system databases, installed games, or firmware. A successful PKG build validates compilation and package structure only; it does not prove installation, firmware compatibility, permissions, or runtime behavior.

## Build
The isolated GitHub Actions workflow uses the OpenOrbis toolchain and SDL2 port. The workflow produces a `PS5-LAUNCHER-PKG` artifact when successful. Physical-console installation and runtime behavior remain to be tested.
