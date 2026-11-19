# PS5-style Homebrew Launcher (separate application)

Independent PS4 homebrew application. This project is intentionally isolated from the MINEPLUS game sources and package identity.

## Current implementation
- SDL2-based 1080p interface prototype.
- DualShock 4 D-pad and left-analog navigation, including controller hot-plug handling.
- PlayStation-native prompts for D-pad/stick browsing and Triangle library refresh.
- Triangle/Y rescans the installed-game library without restarting.
- Animated particle backdrop and selected-tile focus animation.
- Bottom control center with Games, Media, Settings and Profile sections.
- Centered five-slot library carousel for up to 40 discovered games, sorted alphabetically from a bounded scan of up to 256 readable entries.
- Loads each installed title's real `sce_sys/icon0.png` when accessible, with a procedural fallback.
- Caches at most five icon textures while browsing to keep GPU memory bounded.
- Read-only discovery attempt under `/user/app/*/sce_sys/param.sfo`.
- Bounded PARAM.SFO parser for `TITLE` and `TITLE_ID`, with strict PS4 title-ID validation and self-entry filtering, using bounded C stdio reads to avoid unsupported iostream locale code in the OpenOrbis libc++ headers.
- Lowercase-aware bitmap text with basic punctuation for readable title IDs and names.
- Visible feedback for rescans and for menu/game actions that are not yet implemented.
- Honest empty/denied library states; no fake game entries are presented as installed games.

## Not implemented / not verified
- Access to installed retail application directories is not guaranteed from a normal homebrew sandbox. The scan must be tested on the target console and may report access denied or no readable games.
- Game launch is wired through `sceLncUtilInitialize` and `sceLncUtilLaunchApp`, with the System Service and User Service internal modules loaded first and the foreground user passed to `LncAppParam`. The OpenOrbis link includes both `-lSceLncUtil` and `-lSceSystemService`, preserving the LncUtil/SystemService import pairing used by PS4 homebrew references. The PKG has compiled with these imports, but launch behavior is not yet verified on a physical console or on firmware 14.02 / GoldHEN 2.4b18.14. A successful build is not proof that the firmware exposes the same runtime behavior.
- Game cover downloads, artwork cache, functional Media/Settings/Profile pages and game launching are not implemented.
- The launcher has not been runtime-tested on a PS4.

## Safety and platform limits
The scanner only attempts to read application metadata. It does not modify `app.db`, system databases, installed games or firmware. Do not treat a successful PKG build as proof of firmware compatibility or runtime functionality.

## Build
The isolated GitHub Actions workflow uses the OpenOrbis toolchain and SDL2 port. A successful package build validates compilation and package structure only; physical-console installation and runtime behavior must still be tested.
