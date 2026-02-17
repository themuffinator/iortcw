#!/usr/bin/env python3
import glob
import os
import shutil
import sys
from pathlib import Path


def find_sdl_runtime(build_root: Path) -> Path | None:
    patterns = [
        build_root / "subprojects" / "SDL3-*" / "SDL3.dll",
        build_root / "subprojects" / "SDL3*" / "SDL3.dll",
        build_root / "subprojects" / "sdl3-*" / "SDL3.dll",
        build_root / "subprojects" / "sdl3*" / "SDL3.dll",
    ]
    candidates: list[Path] = []

    for pattern in patterns:
        candidates.extend(Path(p) for p in glob.glob(str(pattern)))

    if not candidates:
        return None

    candidates.sort(key=lambda p: p.stat().st_mtime, reverse=True)
    return candidates[0]


def copy_file(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)
    print(f"install_sdl_runtime: copied {src} -> {dst}")


def main() -> int:
    build_root = os.environ.get("MESON_BUILD_ROOT")
    install_root = os.environ.get("MESON_INSTALL_DESTDIR_PREFIX")

    if not build_root or not install_root:
        print(
            "install_sdl_runtime: missing MESON_BUILD_ROOT or MESON_INSTALL_DESTDIR_PREFIX",
            file=sys.stderr,
        )
        return 1

    build_path = Path(build_root)
    install_path = Path(install_root)
    sdl_dll = find_sdl_runtime(build_path)

    if sdl_dll is None:
        print("install_sdl_runtime: SDL3.dll not found in subprojects output", file=sys.stderr)
        return 1

    copy_file(sdl_dll, install_path / sdl_dll.name)

    sdl_pdb = sdl_dll.with_suffix(".pdb")
    if sdl_pdb.exists():
        copy_file(sdl_pdb, install_path / sdl_pdb.name)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
