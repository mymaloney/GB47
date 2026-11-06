#!/usr/bin/env python3
"""Level data: levels/levels.json -> src/levels.c and src/levels.h.

    python3 tools/gen_levels.py          (or: make levels)

levels/levels.json is the source of truth; edit it with the level editor
(make editor) or by hand. The generated C files are committed so the ROM
builds without Python, but don't edit them directly.

JSON format, one object per level:

    {
      "name":  "FIRST STEPS",              max 12 chars, HUD font only
      "notes": "Shown as a comment in levels.c",
      "map":   [16 strings of 20 chars],   '#' wall '.' floor 'E' exit
                                           'P' player start
      "items": {"stone": 0, "bottle": 0},
      "guards": [{
          "facing":   "right",             up/right/down/left
          "route":    [[1, 5], [18, 5]],   patrol loop, starts at the first
                                           point; one point = sentry
          "uniform":  "none",              none/guard/officer (from body)
          "attitude": null                 or e.g. "HBE": hostile/enforcer/
                                           blind to own clothes, GUARD,
                                           OFFICER; null = enforcer to all
      }]
    }
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
JSON_PATH = ROOT / "levels" / "levels.json"

# Must match src/game.h
MAP_W, MAP_H = 20, 16
MAX_GUARDS, MAX_WAYPOINTS = 4, 6
NAME_MAX = 12
DIRS = ["up", "right", "down", "left"]
UNIFORMS = ["none", "guard", "officer"]
ITEMS = ["stone", "bottle"]
FONT_CHARS = set("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 :!-.>?")


def validate_level(lv):
    """Return a list of human-readable problems (empty = OK)."""
    errs = []
    name = lv.get("name", "")
    if not name.strip():
        errs.append("Level needs a name.")
    if len(name) > NAME_MAX:
        errs.append(f"Name is {len(name)} characters; the HUD fits {NAME_MAX}.")
    bad = sorted(set(name) - FONT_CHARS)
    if bad:
        errs.append("Name can only use A-Z, 0-9, space and : ! - . > ? "
                    f"(not {' '.join(bad)}).")

    rows = lv.get("map", [])
    if len(rows) != MAP_H or any(len(r) != MAP_W for r in rows):
        errs.append(f"Map must be {MAP_H} rows of {MAP_W} characters.")
        return errs
    if any(c not in "#.EP" for r in rows for c in r):
        errs.append("Map may only contain # . E P.")
    starts = [(x, y) for y, r in enumerate(rows) for x, c in enumerate(r) if c == "P"]
    if len(starts) != 1:
        errs.append(f"Map needs exactly one player start (has {len(starts)}).")
    if not any("E" in r for r in rows):
        errs.append("Map needs at least one exit.")

    def wall(x, y):
        return not (0 <= x < MAP_W and 0 <= y < MAP_H) or rows[y][x] == "#"

    items = lv.get("items", {})
    for k in ITEMS:
        n = items.get(k, 0)
        if not isinstance(n, int) or not 0 <= n <= 99:
            errs.append(f"Item count for {k} must be 0-99.")

    guards = lv.get("guards", [])
    if len(guards) > MAX_GUARDS:
        errs.append(f"At most {MAX_GUARDS} guards (has {len(guards)}).")
    seen_starts = {}
    for i, g in enumerate(guards):
        tag = f"Guard {i + 1}"
        route = g.get("route", [])
        if not 1 <= len(route) <= MAX_WAYPOINTS:
            errs.append(f"{tag}: route needs 1-{MAX_WAYPOINTS} points (has {len(route)}).")
            continue
        if g.get("facing") not in DIRS:
            errs.append(f"{tag}: facing must be one of {', '.join(DIRS)}.")
        if g.get("uniform", "none") not in UNIFORMS:
            errs.append(f"{tag}: uniform must be one of {', '.join(UNIFORMS)}.")
        att = g.get("attitude")
        if att is not None and (len(att) != len(UNIFORMS) or set(att) - set("HEB")):
            errs.append(f"{tag}: attitude must be {len(UNIFORMS)} letters of H, E, B.")
        for w, (x, y) in enumerate(route):
            if wall(x, y):
                errs.append(f"{tag}: point {w + 1} ({x},{y}) is in a wall.")
        start = tuple(route[0])
        if start in seen_starts:
            errs.append(f"{tag} starts on the same tile as guard {seen_starts[start] + 1}.")
        seen_starts[start] = i
        if starts and start == starts[0]:
            errs.append(f"{tag} starts on the player.")
        if len(route) > 1:
            for w in range(len(route)):
                (ax, ay), (bx, by) = route[w], route[(w + 1) % len(route)]
                leg = f"{tag}: leg {w + 1}->{(w + 1) % len(route) + 1}"
                if (ax, ay) == (bx, by):
                    errs.append(f"{leg} goes nowhere (repeated point).")
                elif ax != bx and ay != by:
                    errs.append(f"{leg} isn't a straight line; add a corner point.")
                else:
                    sx = (bx > ax) - (bx < ax)
                    sy = (by > ay) - (by < ay)
                    x, y = ax, ay
                    while (x, y) != (bx, by):
                        x, y = x + sx, y + sy
                        if wall(x, y):
                            errs.append(f"{leg} walks through a wall at ({x},{y}).")
                            break
    return errs


def validate_all(levels):
    errs = []
    if not levels:
        errs.append("There must be at least one level.")
    if len(levels) > 99:
        errs.append("At most 99 levels.")
    for n, lv in enumerate(levels):
        errs += [f"Level {n + 1} ({lv.get('name', '?')}): {e}" for e in validate_level(lv)]
    return errs


def c_string(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def generate_c(levels):
    out = [
        "/* Generated by tools/gen_levels.py from levels/levels.json.",
        " * Don't edit by hand: use the level editor (make editor) or the JSON. */",
        '#include "levels.h"',
        "",
        "const LevelDef levels[LEVEL_COUNT] = {",
    ]
    for n, lv in enumerate(levels):
        notes = lv.get("notes", "").strip()
        out.append(f"    /* {n + 1}: {lv['name']}" + (f" - {notes}" if notes else "")
                   .replace("*/", "* /") + " */")
        out.append("    {")
        out.append(f"        {c_string(lv['name'])},")
        out.append("        {")
        for r in lv["map"]:
            out.append(f"            {c_string(r)},")
        out.append("        },")
        guards = lv.get("guards", [])
        out.append(f"        {len(guards)},")
        out.append("        {")
        for g in guards:
            pts = ", ".join(f"{{{x}, {y}}}" for x, y in g["route"])
            att = c_string(g["attitude"]) if g.get("attitude") else "0"
            out.append(f"            {{ DIR_{g['facing'].upper()}, {len(g['route'])}, "
                       f"{{ {pts} }}, DISGUISE_{g.get('uniform', 'none').upper()}, {att} }},")
        out.append("        },")
        items = lv.get("items", {})
        out.append("        { " + ", ".join(str(items.get(k, 0)) for k in ITEMS) + " },")
        out.append("    },")
    out.append("};")
    out.append("")
    return "\n".join(out)


def generate_h(levels):
    return "\n".join([
        "/* Generated by tools/gen_levels.py - do not edit by hand. */",
        "#ifndef LEVELS_H",
        "#define LEVELS_H",
        "",
        '#include "game.h"',
        "",
        f"#define LEVEL_COUNT {len(levels)}",
        "extern const LevelDef levels[LEVEL_COUNT];",
        "",
        "#endif",
        "",
    ])


def dumps(levels):
    """JSON with one map row per line, so diffs and hand edits stay readable."""
    parts = []
    for lv in levels:
        lines = ["  {"]
        lines.append(f'    "name": {json.dumps(lv["name"])},')
        lines.append(f'    "notes": {json.dumps(lv.get("notes", ""))},')
        lines.append('    "map": [')
        lines.append(",\n".join(f"      {json.dumps(r)}" for r in lv["map"]))
        lines.append("    ],")
        lines.append(f'    "items": {json.dumps({k: lv.get("items", {}).get(k, 0) for k in ITEMS})},')
        gl = []
        for g in lv.get("guards", []):
            gl.append("      " + json.dumps({
                "facing": g["facing"], "route": g["route"],
                "uniform": g.get("uniform", "none"), "attitude": g.get("attitude")}))
        lines.append('    "guards": [' + ("\n" + ",\n".join(gl) + "\n    ]" if gl else "]"))
        lines.append("  }")
        parts.append("\n".join(lines))
    return "[\n" + ",\n".join(parts) + "\n]\n"


def load(path=JSON_PATH):
    return json.loads(Path(path).read_text())


def save(levels, path=JSON_PATH):
    """Validate, then write the JSON and the generated C. Returns errors."""
    errs = validate_all(levels)
    if errs:
        return errs
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    Path(path).write_text(dumps(levels))
    write_c(levels)
    return []


def write_c(levels):
    (ROOT / "src" / "levels.c").write_text(generate_c(levels))
    (ROOT / "src" / "levels.h").write_text(generate_h(levels))


def main():
    levels = load()
    errs = validate_all(levels)
    if errs:
        print("\n".join(errs), file=sys.stderr)
        sys.exit(1)
    write_c(levels)
    print(f"wrote {len(levels)} levels to src/levels.c")


if __name__ == "__main__":
    main()
