"""Python bridge to the real game rules (build/libgame.so).

The editor never re-implements the rules: every playtest step, vision
cone and solver run goes through src/game.c and tools/solver.c.
"""
import base64
import ctypes as C
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))
import gen_levels   # noqa: E402
import tileset as ts   # noqa: E402
LIB_PATH = ROOT / "build" / "libgame.so"


def _parse_defines(path):
    """Integer #defines from a C header, resolving simple aliases."""
    raw = dict(re.findall(r"^#define\s+([A-Z_][A-Z0-9_]*)\s+([A-Za-z0-9_]+)",
                          Path(path).read_text(), re.M))
    out = {}

    def resolve(name, depth=0):
        v = raw[name]
        if re.fullmatch(r"0x[0-9A-Fa-f]+|\d+", v):
            return int(v, 0)
        if v in raw and depth < 8:
            return resolve(v, depth + 1)
        return None

    for k in raw:
        v = resolve(k)
        if v is not None:
            out[k] = v
    return out


G = _parse_defines(ROOT / "src" / "game.h")     # game constants
G.update(_parse_defines(ROOT / "tools" / "solver.h"))  # SOLVE_USE_*
G.update(_parse_defines(ROOT / "src" / "trans.h"))     # TRANS_*
T = _parse_defines(ROOT / "src" / "tiles.h")    # tile/sprite indices

lib = C.CDLL(str(LIB_PATH))
lib.editor_info.restype = C.c_int
lib.editor_info.argtypes = [C.c_int]

MAP_MAX_W, MAP_MAX_H = lib.editor_info(4), lib.editor_info(5)
MAP_MAX_CELLS = lib.editor_info(13)
VIEW_W, VIEW_H = lib.editor_info(14), lib.editor_info(15)
MAX_PARTS = lib.editor_info(17)
MAX_GUARDS, MAX_WAYPOINTS = lib.editor_info(6), lib.editor_info(7)
ITEM_TYPES, DISGUISE_MAX = lib.editor_info(8), lib.editor_info(9)
MAX_SWITCHES = lib.editor_info(21)
LOOK_FRAMES = ts.LOOK_FRAMES


class Point(C.Structure):
    _fields_ = [("x", C.c_uint8), ("y", C.c_uint8)]


class GuardDef(C.Structure):
    _fields_ = [("facing", C.c_uint8), ("wp_count", C.c_uint8),
                ("wp", Point * MAX_WAYPOINTS), ("uniform", C.c_uint8),
                ("enforcer", C.c_uint8), ("flags", C.c_uint8), ("look", C.c_uint8),
                ("verb", C.c_char_p), ("script", C.POINTER(C.c_uint8))]


class SwitchDef(C.Structure):
    _fields_ = [(n, C.c_uint8) for n in "x y flag off on on_art".split()]


class LevelDef(C.Structure):
    _fields_ = [("name", C.c_char_p), ("width", C.c_uint8), ("height", C.c_uint8),
                ("map", C.c_char_p),
                ("guard_count", C.c_uint8), ("guards", GuardDef * MAX_GUARDS),
                ("items", C.c_uint8 * ITEM_TYPES),
                ("part", C.c_uint8), ("link", C.c_uint8 * 4), ("link_offset", C.c_int8 * 4),
                ("transition", C.c_uint8),
                ("switch_count", C.c_uint8), ("switches", SwitchDef * MAX_SWITCHES),
                ("zones", C.POINTER(C.c_uint8)), ("zone_rules", C.POINTER(C.c_uint8))]


class Guard(C.Structure):
    _fields_ = [(n, C.c_uint8) for n in
                "x y facing next_wp mode tx ty hx hy stripped".split()]


class GameState(C.Structure):
    _fields_ = [("px", C.c_uint8), ("py", C.c_uint8), ("status", C.c_uint8),
                ("guard_count", C.c_uint8), ("turn", C.c_uint16),
                ("items", C.c_uint8 * ITEM_TYPES),
                ("takedown_left", C.c_uint8), ("takedown_guard", C.c_uint8),
                ("disguise", C.c_uint8), ("flags", C.c_uint8),
                ("guards", Guard * MAX_GUARDS)]


class World(C.Structure):
    _fields_ = [("cur", C.c_uint8), ("part_count", C.c_uint8),
                ("part_def", C.c_uint8 * MAX_PARTS), ("part", GameState * MAX_PARTS)]


class ItemDef(C.Structure):
    _fields_ = [("name", C.c_char_p), ("range", C.c_uint8), ("noise", C.c_uint8)]


class SolveStep(C.Structure):
    _fields_ = [("action", C.c_uint8), ("tx", C.c_uint8), ("ty", C.c_uint8)]


for idx, struct in [(0, GameState), (1, LevelDef), (2, GuardDef), (3, Guard), (16, World),
                    (19, SwitchDef)]:
    if lib.editor_info(idx) != C.sizeof(struct):
        raise RuntimeError(
            f"{struct.__name__} layout in tools/editor/game_lib.py no longer "
            f"matches src/game.h ({C.sizeof(struct)} vs {lib.editor_info(idx)} "
            "bytes). Update the ctypes mirror, then run: make build/libgame.so")
for k, idx in [("ACT_COUNT", 10), ("TAKEDOWN_TURNS", 11)]:
    if G[k] != lib.editor_info(idx):
        raise RuntimeError(f"{k} mismatch: rebuild with make build/libgame.so")

S = C.POINTER(GameState)
L = C.POINTER(LevelDef)
Wp = C.POINTER(World)
_sig = {
    "game_load": (None, [L, S]),
    "game_step": (C.c_uint8, [S, C.c_uint8]),
    "game_throw": (C.c_uint8, [S, C.c_uint8, C.c_uint8, C.c_uint8]),
    "game_can_throw": (C.c_uint8, [S, C.c_uint8, C.c_uint8, C.c_uint8]),
    "game_cell": (C.c_uint8, [C.c_uint8, C.c_uint8]),
    "game_compute_vision": (None, [S, C.c_uint8, C.c_uint8, C.c_uint8, C.c_uint8,
                                   C.POINTER(C.c_uint8)]),
    "game_takedown_target": (C.c_uint8, [S]),
    "game_talk_target": (C.c_uint8, [S]),
    "game_said_count": (C.c_uint8, []),
    "game_said": (C.c_uint8, [C.c_uint8]),
    "game_uniform_here": (C.c_uint8, [S]),
    "game_body_adjacent": (C.c_uint8, [S]),
    "game_regard": (C.c_uint8, [S, C.c_uint8]),
    "game_access": (C.c_uint8, [S, C.c_uint8, C.c_uint8]),
    "world_use": (None, [L]),
    "world_load": (None, [L, C.c_uint8, Wp]),
    "world_activate": (None, [Wp]),
    "world_step": (C.c_uint8, [Wp, C.c_uint8]),
    "world_throw": (C.c_uint8, [Wp, C.c_uint8, C.c_uint8, C.c_uint8]),
    "solver_run": (C.c_int, [L, C.c_uint8, C.c_int, C.POINTER(SolveStep), C.c_int,
                             C.POINTER(C.c_int)]),
    "solver_format": (None, [C.POINTER(SolveStep), C.c_int, C.c_char_p, C.c_int]),
    "trans_steps": (C.c_uint8, [C.c_uint8] * 3),
    "trans_phase": (C.c_uint8, [C.c_uint8] * 7),
}
for name, (res, args) in _sig.items():
    f = getattr(lib, name)
    f.restype, f.argtypes = res, args

# Map character, CELL_* number and tile graphic for each kind of cell.
CELL_NAMES = ["FLOOR", "WALL", "EXIT", "DOOR", "WINDOW", "RAILING", "SECURITY",
              "CURTAIN", "MESH", "VENT"]
CELL_CHARS = gen_levels.CELL_OF
assert all(G["CELL_" + n] == i for i, n in enumerate(CELL_NAMES)) and \
    G["CELL_TYPES"] == len(CELL_NAMES), "CELL_* in game.h changed: update game_lib.py"
CELL_TILES = [T["TILE_" + n] for n in CELL_NAMES]
CELL_BLOCKS = list((C.c_uint8 * len(CELL_NAMES)).in_dll(lib, "cell_blocks"))

ITEM_DEFS = (ItemDef * ITEM_TYPES).in_dll(lib, "item_defs")
ITEMS = [{"name": d.name.decode(), "range": d.range, "noise": d.noise} for d in ITEM_DEFS]
DIRS = gen_levels.DIRS            # DIR_* order
UNIFORMS = gen_levels.UNIFORMS    # the disguises, in order (the suit first)
TRANSITIONS = gen_levels.TRANSITIONS
assert [G["TRANS_" + n.upper()] for n in TRANSITIONS] == list(range(G["TRANS_COUNT"])), \
    "TRANS_* in src/trans.h changed: update TRANSITIONS in tools/gen_levels.py"


def transition(kind, cx, cy):
    """A transition's pattern, from the same code the ROM plays: the step
    at which each screen tile goes black ("out") and comes back ("in"),
    rows of TRANS_W. steps = 0 means it isn't tile-based (cut, fade)."""
    k = TRANSITIONS.index(kind)
    w, h = G["TRANS_W"], G["TRANS_H"]
    cx, cy = min(max(cx, 0), w - 1), min(max(cy, 0), h - 1)
    n = lib.trans_steps(k, cx, cy)
    grid = [[[lib.trans_phase(k, into, n, x, y, cx, cy) for x in range(w)] for y in range(h)]
            for into in (0, 1)]
    return {"kind": kind, "steps": n, "out": grid[0], "in": grid[1]}


class Level:
    """A level's maps as a little LevelDef table (keeps its strings alive).
    Map 0 is where the player starts; links point within the table."""

    def __init__(self, data):
        data = gen_levels.upgrade(data)    # old one-map levels -> "maps"
        self.data = data
        self._keep = []
        maps = data["maps"]
        self.maps = maps
        self.table = (LevelDef * len(maps))()
        links = gen_levels.link_table(data)
        name = self._b(data["name"])
        # scripts as the ROM gets them; a SAY's number indexes self.texts
        self.flags = gen_levels.level_flags(data)
        rules = gen_levels.zone_rule_bytes(data)
        self.zone_rules = (C.c_uint8 * len(rules))(*rules)
        self.texts = []
        # painted art per map: grid of tileset tiles (None = built-in), and
        # how many variants each tile has
        self.art, self.art_kinds = [], []
        for m in maps:
            usage = gen_levels.art_usage(m)
            self.art.append(ts.parse_art(m["art"]) if m.get("art") else None)
            self.art_kinds.append({v: ts.slots_for(c) for v, c in usage.items()})
        for k, m in enumerate(maps):
            d = self.table[k]
            d.name = name
            rows = m["map"]
            d.width, d.height = len(rows[0]), len(rows)
            d.map = self._b("".join(gen_levels.cell_rows(m)))
            local = sorted(gen_levels.art_usage(m))
            switches = m.get("switches", [])[:MAX_SWITCHES]
            d.switch_count = len(switches)
            for i, sw in enumerate(switches):
                sd = d.switches[i]
                sd.x, sd.y = sw["x"], sw["y"]
                sd.flag = 1 << self.flags.index(sw["flag"])
                sd.off = G["CELL_" + CELL_CHARS[rows[sw["y"]][sw["x"]]]]
                sd.on = G["CELL_" + CELL_CHARS[sw["on"]]]
                sd.on_art = local.index(sw["on_art"]) if sw.get("on_art") is not None and local else 255
            guards = m.get("guards", [])[:MAX_GUARDS]
            d.guard_count = len(guards)
            for i, g in enumerate(guards):
                gd = d.guards[i]
                gd.facing = DIRS.index(g["facing"])
                route = g["route"][:MAX_WAYPOINTS]
                gd.wp_count = len(route)
                for w, (x, y) in enumerate(route):
                    gd.wp[w].x, gd.wp[w].y = x, y
                u = g.get("uniform", gen_levels.NO_UNIFORM)
                gd.uniform = 0 if u == gen_levels.NO_UNIFORM else UNIFORMS.index(u)
                gd.enforcer = sum(1 << UNIFORMS.index(u) for u in set(g.get("enforcer", [])))
                names = gen_levels.looks()["looks"]
                look = gen_levels.guard_look(g)
                gd.look = names.index(look) if look in names else 0
                talk = gen_levels.can_talk(g)
                gd.flags = (G["CHAR_NO_TAKEDOWN"] if talk or g.get("takedown") is False else 0) | \
                    (G["CHAR_NO_SIGHT"] if g.get("sees") is False else 0)
                if talk:
                    gd.verb = self._b(g.get("verb") or "TALK")
                    code = gen_levels.compile_script(g.get("script", []), self.flags, self.texts)
                    buf = (C.c_uint8 * len(code))(*code)
                    self._keep.append(buf)
                    gd.script = C.cast(buf, C.POINTER(C.c_uint8))
            if k == 0:
                items = data.get("items", {})
                for i, it in enumerate(ITEMS):
                    d.items[i] = items.get(it["name"].lower(), 0)
            if not gen_levels.zones_default(m):         # which zone each cell is in
                rle = gen_levels.zone_rle(m)
                buf = (C.c_uint8 * len(rle))(*rle)
                self._keep.append(buf)
                d.zones = C.cast(buf, C.POINTER(C.c_uint8))
            if data.get("zones"):                       # the level's rules, shared
                d.zone_rules = C.cast(self.zone_rules, C.POINTER(C.c_uint8))
            d.part = k
            d.transition = TRANSITIONS.index(data["transition"])
            for di, dname in enumerate(DIRS):
                if (k, dname) in links:
                    to, off = links[(k, dname)]
                    d.link[di] = to + 1
                    d.link_offset[di] = off

    def _b(self, s):
        b = s.encode()
        self._keep.append(b)
        return b

    def size(self, k):
        return self.table[k].width, self.table[k].height


def encode(world):
    return base64.b64encode(bytes(world)).decode()


def decode(token):
    return World.from_buffer_copy(base64.b64decode(token))


def new_game(level, disguise=None, map_index=0):
    """A fresh world. map_index > 0 shows another map (for the editor's
    cone preview; the player isn't in it)."""
    w = World()
    lib.world_load(level.table, 0, C.byref(w))
    if map_index:
        w.cur = map_index
    if disguise is not None:
        w.part[w.cur].disguise = disguise
    return w


def step(level, world, code):
    lib.world_use(level.table)    # the world came from an earlier request
    return bool(lib.world_step(C.byref(world), code))


def throw(level, world, item, x, y):
    lib.world_use(level.table)
    return bool(lib.world_throw(C.byref(world), item, x, y))


def said(level):
    """What the action just taken said: each message as its lines."""
    return [level.texts[lib.game_said(i)].split("\n") for i in range(lib.game_said_count())]


def view(level, world, aim_item=None):
    """Everything the screen needs, computed the same way as src/main.c."""
    lib.world_use(level.table)
    lib.world_activate(C.byref(world))
    state = world.part[world.cur]
    cdef = level.table[world.cur]
    w, h = cdef.width, cdef.height
    vis = (C.c_uint8 * (w * h))()
    lib.game_compute_vision(C.byref(state), 0, 0, w, h, vis)
    tiles, aim = [], []
    art, kinds = level.art[world.cur], level.art_kinds[world.cur]
    # switch cells whose flag is set show their other look (as draw_tiles)
    switched = {(sw["x"], sw["y"]): sw.get("on_art")
                for sw in level.maps[world.cur].get("switches", [])
                if state.flags & (1 << level.flags.index(sw["flag"]))}
    talk = lib.game_talk_target(C.byref(state))
    for y in range(h):
        row, arow = [], []
        for x in range(w):
            cell, seen = lib.game_cell(x, y), vis[y * w + x]
            can = aim_item is not None and lib.game_can_throw(C.byref(state), aim_item, x, y)
            arow.append(1 if can else 0)
            # same choices as draw_tiles() in src/main.c; painted tiles are
            # sent as -(1 + tile * 5 + variant)
            painted = art[y][x] if art else None
            if art and (x, y) in switched:
                painted = switched[(x, y)]
            if painted is not None:
                k = kinds[painted]
                if k >= 5 and can and cell == G["CELL_FLOOR"]:
                    v = ts.AIM_SEEN if seen == G["VIS_SEEN"] else ts.AIM
                elif k >= 3 and seen:
                    v = ts.SEEN if seen == G["VIS_SEEN"] else ts.WATCH
                else:
                    v = ts.BASE
                row.append(-(1 + painted * 5 + v))
                continue
            if cell == G["CELL_FLOOR"]:
                if can:
                    t = T["TILE_AIM_SEEN"] if seen == G["VIS_SEEN"] else T["TILE_AIM"]
                elif seen == G["VIS_SEEN"]:
                    t = T["TILE_SEEN"]
                elif seen:
                    t = T["TILE_WATCH"]
                else:
                    t = T["TILE_FLOOR"]
            elif cell == G["CELL_EXIT"]:
                t = T["TILE_EXIT_SEEN"] if seen == G["VIS_SEEN"] else T["TILE_EXIT"]
            elif cell == G["CELL_MESH"]:
                t = T["TILE_MESH_SEEN"] if seen == G["VIS_SEEN"] else \
                    T["TILE_MESH_WATCH"] if seen else T["TILE_MESH"]
            else:
                t = CELL_TILES[cell]
            row.append(t)
        tiles.append(row)
        aim.append(arow)

    sprites = [{"t": player_sprite(state.disguise), "x": state.px, "y": state.py}]
    noise, guards = [], []
    for i in range(state.guard_count):
        g = state.guards[i]
        base = T["SPRITE_TILE_COUNT"] + cdef.guards[i].look * LOOK_FRAMES
        if g.mode == G["GUARD_BODY"]:
            t = T["SPR_BODY_STRIPPED"] if g.stripped else base + 5
        elif g.mode == G["GUARD_SUBDUED"]:
            t = base + 4
        else:
            t = base + g.facing
        sprites.append({"t": t, "x": g.x, "y": g.y, "guard": i, "base": base})
        if g.mode == G["GUARD_INVESTIGATE"]:
            noise.append({"t": T["SPR_NOISE"], "x": g.tx, "y": g.ty})
        guards.append({"x": g.x, "y": g.y, "facing": g.facing, "mode": g.mode,
                       "regard": lib.game_regard(C.byref(state), i)})
    return {
        "map": world.cur, "w": w, "h": h,
        "tiles": tiles,
        "sprites": sprites + noise,
        "aim": aim if aim_item is not None else None,
        "status": state.status,
        "turn": state.turn,
        "items": list(state.items),
        "disguise": state.disguise,
        "player": [state.px, state.py],
        "takedown_left": state.takedown_left,
        "takedown_guard": state.takedown_guard,
        "prompt": {
            "takedown": lib.game_takedown_target(C.byref(state)) != 0xFF,
            "uniform": lib.game_uniform_here(C.byref(state)) != 0xFF,
            "body_adjacent": bool(lib.game_body_adjacent(C.byref(state))),
            "talk": cdef.guards[talk].verb.decode() if talk != 0xFF else None,
        },
        "flags": [f for i, f in enumerate(level.flags) if state.flags & (1 << i)],
        "guards": guards,
    }


def player_sprite(disguise):
    """Sprite tile of the player in a disguise: its look's down frame, as
    main.c draws it."""
    names = gen_levels.looks()["looks"]
    ds = gen_levels.disguises()
    look = ds[disguise]["look"] if disguise < len(ds) else ds[0]["look"]
    return T["SPRITE_TILE_COUNT"] + names.index(look) * LOOK_FRAMES + G["DIR_DOWN"]


MAX_STEPS = 1024


def solve(level, use):
    steps = (SolveStep * MAX_STEPS)()
    states = C.c_int(0)
    n = lib.solver_run(level.table, 0, use, steps, MAX_STEPS, C.byref(states))
    out = {"length": n, "states": states.value, "steps": [], "text": ""}
    if n > 0:
        k = min(n, MAX_STEPS)
        buf = C.create_string_buffer(8192)
        lib.solver_format(steps, k, buf, len(buf))
        out["text"] = buf.value.decode().strip()
        out["steps"] = [{"action": s.action, "tx": s.tx, "ty": s.ty} for s in steps[:k]]
    return out
