# PS5-style Homebrew Launcher (separate application)

Independent PS4 homebrew application. This project is intentionally isolated from the MINEPLUS game sources and package identity.

## First milestone
- SDL2-based 1080p interface prototype.
- DualShock 4 D-pad navigation.
- Animated particle backdrop and selected-tile focus animation.
- Bottom action hub.
- Mock catalog only: system game discovery and title launching are not implemented yet.

## Important platform limits
A normal homebrew process is sandboxed. Reading protected application databases/directories and launching installed retail titles cannot be assumed available from SDL2 or OpenOrbis alone. Those features require firmware-specific, documented APIs and permissions, validated on the target console. This prototype deliberately does not issue guessed SCE calls or scan protected paths.

## Build
The isolated GitHub Actions workflow uses the OpenOrbis toolchain and SDL2 port. A successful package build validates compilation and package structure only; physical-console installation and runtime behavior must still be tested.
