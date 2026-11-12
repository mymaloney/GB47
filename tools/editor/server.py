#!/usr/bin/env python3
"""Level editor server.

    make editor            (builds build/libgame.so, then runs this)

Serves the editor page on http://127.0.0.1:8047 and a small JSON API.
Playtesting and solving call the real game rules in build/libgame.so.
Saving writes levels/levels.json and regenerates src/levels.c/.h.
Only listens on localhost.
"""
import json
import os
import re
import subprocess
import sys
import threading
import webbrowser
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(ROOT / "tools"))

try:
    import game_lib as g
except OSError:
    sys.exit("build/libgame.so is missing. Start the editor with: make editor")
import gen_levels
import tileset as ts
import autopaint

PORT = int(os.environ.get("EDITOR_PORT", "8047"))
STATIC = {
    "/": ("index.html", "text/html; charset=utf-8"),
    "/editor.js": ("editor.js", "text/javascript; charset=utf-8"),
    "/editor.css": ("editor.css", "text/css; charset=utf-8"),
}
LOCK = threading.Lock()   # game.c keeps global state: one request at a time
ORIGINS = {f"http://{h}:{PORT}" for h in ("127.0.0.1", "localhost")}


def allowed(headers):
    """Only the editor's own page may talk to the server. Host must be this
    machine by name: another site's name pointed at 127.0.0.1 (DNS
    rebinding) is refused. A cross-site page's Origin is refused too."""
    if f"http://{headers.get('Host', '')}" not in ORIGINS:
        return False
    origin = headers.get("Origin")
    return origin is None or origin in ORIGINS


def assets():
    """Tile graphics (decoded 2bpp) and constants for the browser."""
    src = (ROOT / "src" / "tiles.c").read_text()

    def tiles(name):
        body = src.split(f"const uint8_t {name}[] = {{")[1].split("};")[0]
        b = [int(h, 16) for h in re.findall(r"0x([0-9A-F]{2})", body)]
        out = []
        for t in range(0, len(b), 16):
            px = []
            for y in range(8):
                lo, hi = b[t + 2 * y], b[t + 2 * y + 1]
                px += [((lo >> (7 - x)) & 1) | (((hi >> (7 - x)) & 1) << 1) for x in range(8)]
            out.append(px)
        return out

    return {
        "bkg": tiles("bkg_tile_data"),
        # built-in sprites, then the character looks, as in sprite VRAM
        "spr": tiles("sprite_tile_data"),
        "looks": looks_assets(),
        "T": g.T,
        "G": g.G,
        "items": g.ITEMS,
        "dirs": g.DIRS,
        "uniforms": g.UNIFORMS,
        "transitions": g.TRANSITIONS,
        "max_switches": g.MAX_SWITCHES,
        "max_flags": gen_levels.MAX_FLAGS, "verb_max": gen_levels.VERB_MAX,
        "access": gen_levels.ACCESS, "access_names": gen_levels.ACCESS_NAMES,
        "default_rules": {u: gen_levels.default_rule(u) for u in gen_levels.UNIFORMS},
        "max_zones": gen_levels.MAX_ZONES, "zone_digits": gen_levels.ZONE_DIGITS,
        "text_cols": gen_levels.TEXT_COLS, "text_rows": gen_levels.TEXT_ROWS,
        "view_w": g.VIEW_W, "view_h": g.VIEW_H,
        "map_max_w": g.MAP_MAX_W, "map_max_h": g.MAP_MAX_H,
        "map_max_cells": g.MAP_MAX_CELLS,
        "max_guards": g.MAX_GUARDS, "max_waypoints": g.MAX_WAYPOINTS,
        "max_maps": g.MAX_PARTS,
        "name_max": gen_levels.NAME_MAX,
        "font_chars": "".join(sorted(gen_levels.FONT_CHARS)),
        # building blocks: map char, name, background tile, what it blocks
        # the tileset for painting: every tile's 5 variants as shades
        "tileset": tileset_assets(),
        "art_slots": gen_levels.art_slots_available(),
        "tiles": [{"char": ch, "name": name,
                   "tile": g.CELL_TILES[g.G["CELL_" + g.CELL_CHARS[ch]]],
                   "blocks": {"player": bp, "guards": bg, "sight": bs, "throws": bt}}
                  for ch, (name, bp, bg, bs, bt) in gen_levels.TILES.items()],
    }


def tileset_assets():
    t = ts.load()
    return {
        "columns": t["columns"],
        "license": t.get("license", ""),
        "tiles": [[ts.variant(px, k) for k in range(5)] for px in t["tiles"]],
        "behaviour": {str(k): v for k, v in t["behaviour"].items()},
        "autopaint": t.get("autopaint", {}),
    }


def looks_assets():
    """Character looks: names, and each look's frames as 64 shades."""
    t = ts.load_looks()
    return {"names": t["looks"], "frame_names": t.get("frames", []),
            "frames": t["frames_of"], "frame_count": ts.LOOK_FRAMES,
            "first_tile": g.T["SPRITE_TILE_COUNT"],
            "max": gen_levels.sprite_slots_available() // ts.LOOK_FRAMES}


def regenerate():
    """The ROM's copy of tileset tiles and looks lives in src/levels.c:
    regenerate it from the saved levels (if they're valid; Save does it
    otherwise). Returns whether it did."""
    gen_levels.forget_tilesets()
    saved = gen_levels.load()
    ok = not gen_levels.validate_all(saved)
    if ok:
        gen_levels.write_c(saved)
    return ok


def analyze(level):
    """Shortest solution, plus which mechanics the level really needs."""
    G = g.G
    full = g.solve(level, G["SOLVE_USE_ALL"])
    result = {"solve": full, "needs": {}}
    if full["length"] <= 0:
        return result
    guards = [gd for m in level.maps for gd in m.get("guards", [])]
    checks = []
    if any(level.data.get("items", {}).values()):
        checks.append(("items", G["SOLVE_USE_ITEMS"]))
    if guards:
        checks.append(("takedown", G["SOLVE_USE_TAKEDOWN"]))
        checks.append(("drag", G["SOLVE_USE_DRAG"]))
    if any(gen_levels.can_talk(gd) for gd in guards):
        checks.append(("talk", G["SOLVE_USE_TALK"]))
    if any(gd.get("uniform", "none") != "none" for gd in guards):
        checks.append(("uniform", G["SOLVE_USE_UNIFORM"]))
    for name, bit in checks:
        r = g.solve(level, G["SOLVE_USE_ALL"] & ~bit)
        result["needs"][name] = ("unknown" if r["length"] < 0 else
                                 "yes" if r["length"] == 0 else "no")
    return result


class Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        pass   # keep the terminal quiet

    def send_json(self, obj, code=200):
        body = json.dumps(obj).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        path = self.path.split("?")[0]
        if not allowed(self.headers):
            self.send_json({"error": "forbidden"}, 403)
        elif path in STATIC:
            name, ctype = STATIC[path]
            body = (HERE / name).read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", ctype)
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        elif path == "/api/assets":
            self.send_json(assets())
        elif path == "/api/levels":
            self.send_json({"levels": gen_levels.load()})
        else:
            self.send_json({"error": "not found"}, 404)

    def do_POST(self):
        if not allowed(self.headers):
            self.send_json({"error": "forbidden"}, 403)
            return
        try:
            n = int(self.headers.get("Content-Length", 0))
            req = json.loads(self.rfile.read(n) or b"{}")
            with LOCK:
                out = self.route(self.path, req)
            self.send_json(out)
        except Exception as e:   # report to the page instead of dying
            self.send_json({"error": f"{type(e).__name__}: {e}"}, 500)

    def route(self, path, req):
        if path == "/api/validate":
            return {"errors": gen_levels.validate_level(req["level"])}
        if path == "/api/save":
            errs = gen_levels.save(req["levels"])
            if not errs and "behaviour" in req:   # tileset tile behaviours
                ts.save_behaviour({int(k): v for k, v in req["behaviour"].items()
                                   if v in gen_levels.TILES})
            if not errs and "taught" in req:      # shapes taught to auto-tile
                ts.save_taught(req["taught"])
            return {"ok": not errs, "errors": errs}
        if path in ("/api/tileset/tile", "/api/tileset/add_row"):
            if path == "/api/tileset/tile":
                ts.set_tile(int(req["index"]), [int(v) for v in req["pixels"]])
            else:
                ts.add_row()
            return {"tileset": tileset_assets(), "regenerated": regenerate()}
        if path in ("/api/looks/tile", "/api/looks/add"):
            if path == "/api/looks/tile":
                look, frame = int(req["look"]), int(req["frame"])
                if not (0 <= frame < ts.LOOK_FRAMES and
                        0 <= look < len(ts.load_looks()["looks"])):
                    return {"error": "no such look or frame"}
                ts.set_tile(look * ts.LOOK_FRAMES + frame, [int(v) for v in req["pixels"]],
                            path=ts.CHARACTERS)
            else:
                name = str(req.get("name", "")).strip().lower()
                if not re.fullmatch(r"[a-z][a-z0-9_]{0,15}", name):
                    return {"error": "A look's name is 1-16 letters, digits or _, "
                                     "starting with a letter."}
                names = ts.load_looks()["looks"]
                if len(names) >= gen_levels.sprite_slots_available() // ts.LOOK_FRAMES:
                    return {"error": "There's no room for more looks in sprite memory."}
                copy = req.get("copy_from")
                try:
                    ts.add_look(name, int(copy) if copy is not None else None)
                except ValueError as e:
                    return {"error": str(e)[0].upper() + str(e)[1:] + "."}
            return {"looks": looks_assets(), "regenerated": regenerate()}
        if path in ("/api/autopaint", "/api/autotile"):
            # learn the painted style from the levels open in the editor
            cfg = dict(ts.load()["autopaint"])
            if "taught" in req:                   # the editor's, maybe unsaved
                cfg["taught"] = req["taught"]
            # don't learn from the cells being repainted: their art is stale
            li, mi = req.get("level", -1), req.get("map_index", -1)
            skip = {(li, mi, x + dx, y + dy) for x, y in req.get("changed", [])
                    for dx in (-2, -1, 0, 1, 2) for dy in (-2, -1, 0, 1, 2)}
            learned = autopaint.learn(req.get("levels", []), cfg, skip)
            # switch cells are painted by hand: leave their art alone
            keep = [tuple(c) for c in req.get("keep", [])]
            if path == "/api/autopaint":
                art = ts.format_art(autopaint.autopaint(req["map"], cfg, learned))
            else:
                art = autopaint.autotile(req["map"], req.get("art"),
                                         [tuple(c) for c in req["changed"]], cfg, learned)
            return {"art": autopaint.keep_art(art, req.get("art"), keep)}
        if path == "/api/wrap":       # a message as the Game Boy's box shows it
            try:
                return {"lines": gen_levels.wrap_text(req.get("text", ""))}
            except ValueError as e:
                return {"problem": str(e)}
        if path == "/api/transition":
            return g.transition(req["kind"], int(req.get("cx", 10)), int(req.get("cy", 8)))
        if path == "/api/build":
            p = subprocess.run(["make"], cwd=ROOT, capture_output=True, text=True,
                               timeout=600)
            return {"ok": p.returncode == 0, "output": (p.stdout + p.stderr)[-6000:]}

        # Everything below needs a valid level.
        errs = gen_levels.validate_level(req["level"])
        if errs:
            return {"error": "Fix the level first: " + errs[0], "errors": errs}
        level = g.Level(req["level"])
        aim = req.get("aim_item")

        if path == "/api/start":
            k = req.get("map", 0)
            if not 0 <= k < len(level.maps):
                return {"error": "no such map"}
            s = g.new_game(level, req.get("disguise"), k)
            return {"state": g.encode(s), "view": g.view(level, s, aim)}
        if path == "/api/view":
            s = g.decode(req["state"])
            return {"view": g.view(level, s, aim)}
        if path == "/api/act":
            s = g.decode(req["state"])
            a = req["action"]
            if a["type"] == "throw":
                ok = g.throw(level, s, a["item"], a["x"], a["y"])
            else:
                ok = g.step(level, s, a["code"])
            told = g.said(level) if ok else []
            return {"ok": ok, "state": g.encode(s), "view": g.view(level, s, aim), "said": told}
        if path == "/api/solve":
            return analyze(level)
        return {"error": "unknown endpoint"}


def main():
    server = HTTPServer(("127.0.0.1", PORT), Handler)
    url = f"http://127.0.0.1:{PORT}/"
    print(f"Level editor running at {url}  (Ctrl+C to stop)")
    if not os.environ.get("EDITOR_NO_BROWSER"):
        threading.Timer(0.5, lambda: webbrowser.open(url)).start()
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print()


if __name__ == "__main__":
    main()
