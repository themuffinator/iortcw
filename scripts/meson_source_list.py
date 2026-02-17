#!/usr/bin/env python3
"""
Extract and map iortcw Makefile object lists to concrete source files.

This keeps Meson source lists aligned with SP/Makefile and MP/Makefile
without duplicating huge static file arrays.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys
from typing import Dict, Iterable, List


TRACKED_GROUPS = {
    "Q3OBJ",
    "Q3ROBJ",
    "Q3R2OBJ",
    "Q3R2STRINGOBJ",
    "JPGOBJ",
    "FTOBJ",
    "Q3DOBJ",
    "Q3CGOBJ_",
    "Q3GOBJ_",
    "Q3UIOBJ_",
    "Q3CGOBJ",
    "Q3GOBJ",
    "Q3UIOBJ",
}


def _bool01(value: str) -> str:
    return "1" if value and value not in ("0", "false", "False", "off") else "0"


def _join_line_continuations(lines: Iterable[str]) -> List[str]:
    out: List[str] = []
    buf = ""
    for line in lines:
        if line.rstrip().endswith("\\"):
            buf += line.rstrip()[:-1] + " "
            continue
        buf += line
        out.append(buf)
        buf = ""
    if buf:
        out.append(buf)
    return out


def _expand_make(value: str, vars_map: Dict[str, str]) -> str:
    nested_findstring_rx = re.compile(r"\$\(findstring\s+\$\(([A-Za-z0-9_]+)\),([^)]+)\)")
    findstring_rx = re.compile(r"\$\(findstring\s+([^,]+),([^)]+)\)")
    var_rx = re.compile(r"\$\(([A-Za-z0-9_]+)\)")

    text = value
    for _ in range(32):
        prev = text

        def nested_findstring_repl(match: re.Match[str]) -> str:
            needle = vars_map.get(match.group(1).strip(), "")
            haystack = _expand_make(match.group(2).strip(), vars_map)
            if not needle:
                return ""
            if needle in haystack.split() or needle in haystack:
                return needle
            return ""

        def findstring_repl(match: re.Match[str]) -> str:
            needle = _expand_make(match.group(1).strip(), vars_map)
            haystack = _expand_make(match.group(2).strip(), vars_map)
            if not needle:
                return ""
            if needle in haystack.split() or needle in haystack:
                return needle
            return ""

        text = nested_findstring_rx.sub(nested_findstring_repl, text)
        text = findstring_rx.sub(findstring_repl, text)
        text = var_rx.sub(lambda m: vars_map.get(m.group(1).strip(), ""), text)
        if text == prev:
            break

    return text.strip().strip('"')


def _eval_cond(raw: str, vars_map: Dict[str, str]) -> bool:
    neg = raw.startswith("ifneq")
    inside = raw[5:].strip()
    if inside.startswith("(") and inside.endswith(")"):
        inside = inside[1:-1]
    lhs = inside
    rhs = ""
    depth = 0
    split_at = -1
    for idx, ch in enumerate(inside):
        if ch == "(":
            depth += 1
        elif ch == ")":
            if depth > 0:
                depth -= 1
        elif ch == "," and depth == 0:
            split_at = idx
            break
    if split_at >= 0:
        lhs = inside[:split_at]
        rhs = inside[split_at + 1 :]
    lhs_val = _expand_make(lhs.strip(), vars_map)
    rhs_val = _expand_make(rhs.strip(), vars_map)
    cond = lhs_val == rhs_val
    return (not cond) if neg else cond


def _extract_group_tokens(
    root: pathlib.Path, game: str, vars_map: Dict[str, str], group: str
) -> List[str]:
    makefile = root / game / "Makefile"
    text_lines = makefile.read_text(encoding="utf-8", errors="ignore").splitlines()

    # Object lists start at Q3OBJ; skipping earlier setup avoids supporting
    # full GNU make expression semantics not needed for source extraction.
    start_idx = next((i for i, line in enumerate(text_lines) if line.startswith("Q3OBJ =")), None)
    if start_idx is None:
        raise RuntimeError(f"Unable to find Q3OBJ in {makefile}")

    lines = _join_line_continuations(text_lines[start_idx:])
    local_vars = dict(vars_map)
    active_stack = [True]

    assign_rx = re.compile(r"^([A-Za-z0-9_]+)\s*(\+?=)\s*(.*)$")

    for line in lines:
        raw = line.strip()
        if not raw or raw.startswith("#"):
            continue

        if raw.startswith("ifeq") or raw.startswith("ifneq"):
            active_stack.append(active_stack[-1] and _eval_cond(raw, local_vars))
            continue

        if raw.startswith("ifdef "):
            name = raw.split(None, 1)[1].strip()
            active_stack.append(active_stack[-1] and bool(local_vars.get(name, "")))
            continue

        if raw.startswith("ifndef "):
            name = raw.split(None, 1)[1].strip()
            active_stack.append(active_stack[-1] and not bool(local_vars.get(name, "")))
            continue

        if raw == "else":
            prev = active_stack.pop()
            parent = active_stack[-1] if active_stack else True
            active_stack.append(parent and (not prev))
            continue

        if raw == "endif":
            if len(active_stack) > 1:
                active_stack.pop()
            continue

        if not active_stack[-1]:
            continue

        match = assign_rx.match(raw)
        if not match:
            continue

        var, op, value = match.groups()
        value = value.strip()

        if op == "+=":
            local_vars[var] = (local_vars.get(var, "") + " " + value).strip()
        else:
            local_vars[var] = value

    if group not in TRACKED_GROUPS:
        raise RuntimeError(f"Unsupported group: {group}")

    expanded = _expand_make(local_vars.get(group, ""), local_vars)
    return [token for token in expanded.split() if token.endswith(".o")]


def _rel(root: pathlib.Path, path: pathlib.Path) -> str:
    return path.relative_to(root).as_posix()


def _map_object_to_source(root: pathlib.Path, game: str, obj: str) -> str | None:
    code = root / game / "code"
    token = obj.replace("\\", "/")

    if "/rend2/glsl/" in token:
        stem = token.split("/rend2/glsl/")[-1][:-2]
        src = code / "rend2" / "glsl" / f"{stem}.glsl"
        return _rel(root, src) if src.exists() else None

    if "/splines/" in token:
        stem = token.split("/splines/")[-1][:-2]
        for ext in (".cpp", ".c"):
            src = code / "splines" / f"{stem}{ext}"
            if src.exists():
                return _rel(root, src)
        return None

    if "/renderer/" in token:
        stem = token.split("/renderer/")[-1][:-2]
        candidates = [
            code / "qcommon" / f"{stem}.c",
            code / "sdl" / f"{stem}.c",
            code / "jpeg-8c" / f"{stem}.c",
            code / "renderer" / f"{stem}.c",
        ]
        ft_src = code / "freetype-2.9" / "src"
        for sub in (
            "autofit",
            "base",
            "bdf",
            "bzip2",
            "cache",
            "cff",
            "cid",
            "gxvalid",
            "gzip",
            "lzw",
            "otvalid",
            "pcf",
            "pfr",
            "psaux",
            "pshinter",
            "psnames",
            "raster",
            "sfnt",
            "smooth",
            "tools",
            "truetype",
            "type1",
            "type42",
            "winfonts",
        ):
            candidates.append(ft_src / sub / f"{stem}.c")

        for src in candidates:
            if src.exists():
                return _rel(root, src)
        return None

    if "/rend2/" in token:
        stem = token.split("/rend2/")[-1][:-2]
        src = code / "rend2" / f"{stem}.c"
        return _rel(root, src) if src.exists() else None

    if "/client/" in token:
        stem = token.split("/client/")[-1][:-2]

        if stem.startswith("vorbis/"):
            name = stem.split("/", 1)[1]
            src = code / "libvorbis-1.3.6" / "lib" / f"{name}.c"
            return _rel(root, src) if src.exists() else None

        if stem.startswith("opus/"):
            name = stem.split("/", 1)[1]
            for sub in ("src", "celt", "silk", "silk/float", "silk/x86"):
                src = code / "opus-1.2.1" / sub / f"{name}.c"
                if src.exists():
                    return _rel(root, src)
            return None

        candidates = [
            code / "asm" / f"{stem}.c",
            code / "asm" / f"{stem}.s",
            code / "asm" / f"{stem}.asm",
            code / "client" / f"{stem}.c",
            code / "server" / f"{stem}.c",
            code / "qcommon" / f"{stem}.c",
            code / "botlib" / f"{stem}.c",
            code / "libogg-1.3.3" / "src" / f"{stem}.c",
            code / "opusfile-0.9" / "src" / f"{stem}.c",
            code / "minizip" / f"{stem}.c",
            code / "zlib-1.2.11" / f"{stem}.c",
            code / "sdl" / f"{stem}.c",
            code / "sys" / f"{stem}.c",
            code / "sys" / f"{stem}.m",
            code / "sys" / f"{stem}.rc",
        ]
        for src in candidates:
            if src.exists():
                return _rel(root, src)
        return None

    if "/ded/" in token:
        stem = token.split("/ded/")[-1][:-2]
        candidates = [
            code / "asm" / f"{stem}.c",
            code / "asm" / f"{stem}.s",
            code / "asm" / f"{stem}.asm",
            code / "server" / f"{stem}.c",
            code / "qcommon" / f"{stem}.c",
            code / "minizip" / f"{stem}.c",
            code / "zlib-1.2.11" / f"{stem}.c",
            code / "botlib" / f"{stem}.c",
            code / "sys" / f"{stem}.c",
            code / "sys" / f"{stem}.m",
            code / "sys" / f"{stem}.rc",
            code / "null" / f"{stem}.c",
        ]
        for src in candidates:
            if src.exists():
                return _rel(root, src)
        return None

    if "/cgame/" in token:
        stem = token.split("/cgame/")[-1][:-2]
        if stem.startswith("bg_"):
            src = code / "game" / f"{stem}.c"
            if src.exists():
                return _rel(root, src)
        src = code / "cgame" / f"{stem}.c"
        return _rel(root, src) if src.exists() else None

    if "/game/" in token:
        stem = token.split("/game/")[-1][:-2]
        src = code / "game" / f"{stem}.c"
        return _rel(root, src) if src.exists() else None

    if "/ui/" in token:
        stem = token.split("/ui/")[-1][:-2]
        if stem.startswith("bg_"):
            src = code / "game" / f"{stem}.c"
            if src.exists():
                return _rel(root, src)
        src = code / "ui" / f"{stem}.c"
        return _rel(root, src) if src.exists() else None

    if "/qcommon/" in token:
        stem = token.split("/qcommon/")[-1][:-2]
        src = code / "qcommon" / f"{stem}.c"
        return _rel(root, src) if src.exists() else None

    return None


def _parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--game", required=True, choices=("SP", "MP"))
    parser.add_argument("--group", required=True)
    parser.add_argument("--arch", default="x86_64")
    parser.add_argument("--platform", default="linux")
    parser.add_argument("--mingw", default="0")
    parser.add_argument("--basegame", default="main")

    parser.add_argument("--use-bloom", default="1")
    parser.add_argument("--use-renderer-dlopen", default="1")
    parser.add_argument("--use-internal-jpeg", default="0")
    parser.add_argument("--use-freetype", default="1")
    parser.add_argument("--use-internal-freetype", default="0")
    parser.add_argument("--use-internal-opus", default="1")
    parser.add_argument("--use-internal-ogg", default="1")
    parser.add_argument("--use-internal-vorbis", default="1")
    parser.add_argument("--use-internal-zlib", default="0")
    parser.add_argument("--use-codec-vorbis", default="1")
    parser.add_argument("--use-codec-opus", default="1")
    parser.add_argument("--use-voip", default="1")
    parser.add_argument("--use-mumble", default="1")
    parser.add_argument("--use-antiwallhack", default="0")
    parser.add_argument("--have-vm-compiled", default="1")
    return parser.parse_args()


def main() -> int:
    args = _parse_args()
    repo_root = pathlib.Path(__file__).resolve().parents[1]

    vars_map = {
        "ARCH": args.arch,
        "PLATFORM": args.platform,
        "BASEGAME": args.basegame,
        "MINGW": "1" if _bool01(args.mingw) == "1" else "",
        "USE_BLOOM": _bool01(args.use_bloom),
        "USE_RENDERER_DLOPEN": _bool01(args.use_renderer_dlopen),
        "USE_INTERNAL_JPEG": _bool01(args.use_internal_jpeg),
        "USE_FREETYPE": _bool01(args.use_freetype),
        "USE_INTERNAL_FREETYPE": _bool01(args.use_internal_freetype),
        "USE_INTERNAL_OPUS": _bool01(args.use_internal_opus),
        "USE_INTERNAL_OGG": _bool01(args.use_internal_ogg),
        "USE_INTERNAL_VORBIS": _bool01(args.use_internal_vorbis),
        "USE_INTERNAL_ZLIB": _bool01(args.use_internal_zlib),
        "USE_CODEC_VORBIS": _bool01(args.use_codec_vorbis),
        "USE_CODEC_OPUS": _bool01(args.use_codec_opus),
        "USE_VOIP": _bool01(args.use_voip),
        "USE_MUMBLE": _bool01(args.use_mumble),
        "USE_ANTIWALLHACK": _bool01(args.use_antiwallhack),
        "HAVE_VM_COMPILED": "true" if _bool01(args.have_vm_compiled) == "1" else "false",
    }

    need_opus = (
        vars_map["USE_VOIP"] == "1" or vars_map["USE_CODEC_OPUS"] == "1"
    )
    need_ogg = need_opus or vars_map["USE_CODEC_VORBIS"] == "1"
    vars_map["NEED_OPUS"] = "1" if need_opus else "0"
    vars_map["NEED_OGG"] = "1" if need_ogg else "0"

    object_tokens = _extract_group_tokens(repo_root, args.game, vars_map, args.group)

    out_sources: List[str] = []
    seen = set()

    for obj in object_tokens:
        source = _map_object_to_source(repo_root, args.game, obj)
        if source is None:
            print(
                f"error: failed to map object '{obj}' in group {args.group} ({args.game})",
                file=sys.stderr,
            )
            return 2
        if source not in seen:
            out_sources.append(source)
            seen.add(source)

    sys.stdout.write("\n".join(out_sources))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
