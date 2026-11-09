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

PORT = int(os.environ.get("EDITOR_PORT", "8047"))
STATIC = {
    "/": ("index.html", "text/html; charset=utf-8"),
    "/editor.js": ("editor.js", "text/javascript; charset=utf-8"),
    "/editor.css": ("editor.css", "text/css; charset=utf-8"),
}
LOCK = threading.Lock()   # game.c keeps global state: one request at a time


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
        "spr": tiles("sprite_tile_data"),
        "T": g.T,
        "G": g.G,
        "items": g.ITEMS,
        "dirs": g.DIRS,
        "uniforms": g.UNIFORMS,
        "view_w": g.VIEW_W, "view_h": g.VIEW_H,
        "map_max_w": g.MAP_MAX_W, "map_max_h": g.MAP_MAX_H,
        "map_max_cells": g.MAP_MAX_CELLS,
        "max_guards": g.MAX_GUARDS, "max_waypoints": g.MAX_WAYPOINTS,
        "name_max": gen_levels.NAME_MAX,
        "font_chars": "".join(sorted(gen_levels.FONT_CHARS)),
    }


def analyze(level):
    """Shortest solution, plus which mechanics the level really needs."""
    G = g.G
    full = g.solve(level, G["SOLVE_USE_ALL"])
    result = {"solve": full, "needs": {}}
    if full["length"] <= 0:
        return result
    data = level.data
    checks = []
    if any(data.get("items", {}).values()):
        checks.append(("items", G["SOLVE_USE_ITEMS"]))
    if data.get("guards"):
        checks.append(("takedown", G["SOLVE_USE_TAKEDOWN"]))
        checks.append(("drag", G["SOLVE_USE_DRAG"]))
    if any(gd.get("uniform", "none") != "none" for gd in data.get("guards", [])):
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
        if path in STATIC:
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
        # Reject cross-site requests: only the editor page may call the API.
        origin = self.headers.get("Origin")
        if origin and not re.fullmatch(r"http://(127\.0\.0\.1|localhost):%d" % PORT, origin):
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
            return {"ok": not errs, "errors": errs}
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
            s = g.new_game(level, req.get("disguise"))
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
            return {"ok": ok, "state": g.encode(s), "view": g.view(level, s, aim)}
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
