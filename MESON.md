# Meson/Ninja Build

This repository now includes a Meson build that mirrors the legacy GNU Make targets for `SP/` and `MP/`.

## Quick Start

```sh
meson setup build-meson-debug --buildtype=debug --layout=mirror --prefix="$PWD/install" --bindir=. --libdir=. --reconfigure
meson install -C build-meson-debug --skip-subprojects
```

Successful builds stage runtime distributables into `install/`:

- `install/`: client/dedicated executables and renderer DLLs
- `install/main/` (or your `-Dbasegame=` value): game modules (`cgame*`, `qagame*`, `ui*`)

## Common Target Controls

```sh
# Build only SP
meson setup build-sp -Dbuild_mp=false

# Build only dedicated servers
meson setup build-ded -Dbuild_client=false -Dbuild_server=true

# Disable game shared modules
meson setup build-nogame -Dbuild_game_so=false

# Disable rend2 renderer
meson setup build-gl1 -Dbuild_renderer_rend2=false
```

## Dependency Fallbacks

Meson resolves dependencies through `subprojects/*.wrap` fallbacks when system packages are not present:

- `zlib`
- `libjpeg-turbo` (`libjpeg`)
- `freetype2`
- `sdl3`
- `openal-soft` (`openal`)
- `curl` (`libcurl`)

Additional wrappers are included for codec-related libs (`ogg`, `vorbis`, `opus`) to support future external codec switching.

## Notes

- The Meson source lists are generated from `SP/Makefile` and `MP/Makefile` via `scripts/meson_source_list.py`.
- On Windows, use a MinGW-style toolchain (and `windres`) for full parity with the legacy build.
- For MSVC-ABI x86_64 toolchains on Windows (for example `clang` targeting MSVC), compiled VM JIT is disabled automatically to avoid unresolved `qvmcall64` linkage.
