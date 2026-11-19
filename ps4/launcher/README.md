# PS5-style Homebrew Launcher (separate application)

Independent PS4 homebrew application. This project is intentionally isolated from the MINEPLUS game sources and package identity.

## Current implementation
- SDL2-based 1080p interface prototype.
- DualShock 4 D-pad and left-analog navigation, including controller hot-plug handling.
- Animated particle backdrop and selected-tile focus animation.
- Bottom control center with Games, Media, Settings and Profile sections.
- Centered five-slot library carousel for up to 40 discovered games.
- Loads each installed title's real `sce_sys/icon0.png` when accessible, with a procedural fallback.
- Caches at most five icon textures while browsing to keep GPU memory bounded.
- Read-only discovery attempt under `/user/app/*/sce_sys/param.sfo`.
- Bounded PARAM.SFO parser for `TITLE` and `TITLE_ID`, using bounded C stdio reads to avoid unsupported iostream locale code in the OpenOrbis libc++ headers.
- Honest empty/denied library states; no fake game entries are presented as installed games.

## Not implemented / not verified
- Access to installed retail application directories is not guaranteed from a normal homebrew sandbox. The scan must be tested on the target console and may report access denied or no readable games.
- Launching retail games is not implemented. No guessed SCE launch API is called.
- Game cover downloads, artwork cache, analog navigation and settings actions are not implemented.
- The launcher has not been runtime-tested on a PS4.

## Safety and platform limits
The scanner only attempts to read application metadata. It does not modify `app.db`, system databases, installed games or firmware. Do not treat a successful PKG build as proof of firmware compatibility or runtime functionality.

## Build
The isolated GitHub Actions workflow uses the OpenOrbis toolchain and SDL2 port. A successful package build validates compilation and package structure only; physical-console installation and runtime behavior must still be tested.
