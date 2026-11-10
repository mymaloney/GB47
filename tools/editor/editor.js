/*
 * GB47 level editor (browser side).
 *
 * The page never decides game rules itself: playtest turns, vision cones
 * and solving all go to the server, which runs src/game.c. This file only
 * mirrors src/main.c's screen layout and button handling.
 */
"use strict";

const PLAY_SCALE = 3;            // playtest: 160x144 Game Boy screen -> 480x432
let TILE = 16;                   // current tile size on the canvas, in px
let zoomSetting = "fit";         // edit view zoom: "fit" or 1-3
const PALETTE = ["#e0f8d0", "#88c070", "#346856", "#081820"];
const GUARD_COLORS = ["#e4572e", "#2e86ab", "#c23bd4", "#f2b705"];
const TOOL_HINTS = {
  start: "Click to place the player's start.",
  guard: "Click an empty tile to add a guard (max 4), or click a guard to select it. Drag to move it.",
  route: "Click to add waypoints to the selected guard (corners are added for you). Drag a point to move it; right-click to remove it.",
  npc: "Click a tile to add something to talk to (paint its look with the tileset), or click one to select it. Drag to move it.",
  switch: "Click a tile to make it a switch cell: it changes (e.g. into the exit) when a script sets its flag. Click one to select it; drag to move it.",
};
const NPC_COLOR = "#00a6a6";
const SWITCH_COLOR = "#f2b705";

let A = null;                    // assets from /api/assets
let levels = [];
let cur = 0;
let curMap = 0;                  // which map of the level is being edited
let dirty = false;
let mode = "edit";
let tool = "tile:#";
let selGuard = -1;
let selNpc = -1;                 // selected NPC / switch cell of the map being edited
let selSwitch = -1;
let undoStack = [];
let redoStack = [];
let preview = null;              // {key, view} for the current level
let previewDisguise = 0;
let validity = [];               // per level: true/false/undefined
let lastSolve = null;            // {key, steps}

const $ = (id) => document.getElementById(id);
const canvas = $("screen");
const ctx = canvas.getContext("2d");
ctx.imageSmoothingEnabled = false;

// ------------------------------------------------------------------ util

async function api(path, body) {
  const res = await fetch(path, body === undefined ? {} : {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(body),
  });
  return res.json();
}

const clone = (o) => JSON.parse(JSON.stringify(o));
const level = () => levels[cur];
const cmap = () => levels[cur].maps[curMap];     // the map being edited
const keyOf = (lv) => JSON.stringify(lv);
const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

function debounce(fn, ms) {
  let t = null;
  return (...args) => { clearTimeout(t); t = setTimeout(() => fn(...args), ms); };
}

// --------------------------------------------------------------- tiles

let bkgTiles = [];
let sprTiles = [];

function makeTile(px, transparent) {
  const c = document.createElement("canvas");
  c.width = c.height = 8;
  const g = c.getContext("2d");
  const img = g.createImageData(8, 8);
  px.forEach((v, i) => {
    const hex = PALETTE[v];
    img.data[i * 4] = parseInt(hex.slice(1, 3), 16);
    img.data[i * 4 + 1] = parseInt(hex.slice(3, 5), 16);
    img.data[i * 4 + 2] = parseInt(hex.slice(5, 7), 16);
    img.data[i * 4 + 3] = transparent && v === 0 ? 0 : 255;
  });
  g.putImageData(img, 0, 0);
  return c;
}

let artImgs = [];      // [tileset tile][variant] canvases
let behaviour = {};    // tileset tile (string) -> map char painted with it
let artTile = 0;       // selected tileset tile
let taught = {};       // auto-tile: "<cell kind><open-neighbour bits>" -> tile you painted

/* The shape of a cell, as tools/autopaint.py pattern(): its kind plus
 * which of its 8 neighbours (N, NE, E, SE, S, SW, W, NW) are open. */
function shapeKey(m, x, y) {
  const open = (xx, yy) => yy >= 0 && yy < m.map.length && xx >= 0 && xx < m.map[0].length &&
                           m.map[yy][xx] !== "#";
  const dirs = [[0, -1], [1, -1], [1, 0], [1, 1], [0, 1], [-1, 1], [-1, 0], [-1, -1]];
  let bits = 0;
  dirs.forEach(([dx, dy], i) => { if (open(x + dx, y + dy)) bits |= 1 << i; });
  const c = m.map[y][x] === "P" ? "." : m.map[y][x];
  return c + bits;
}

/* Painting art by hand teaches auto-tile: this shape gets this tile. */
function teach(m, x, y, n) {
  const key = shapeKey(m, x, y);
  const southOpen = y + 1 < m.map.length && m.map[y + 1][x] !== "#";
  const face = A.tileset.autopaint.face || [];
  taught[key] = m.map[y][x] === "#" && southOpen && face.includes(n) ? "face" : n;
  updateTaughtInfo();
}

function updateTaughtInfo() {
  const n = Object.keys(taught).length;
  $("taught-info").textContent = n
    ? `Auto-tile has been taught ${n} wall/floor shape${n === 1 ? "" : "s"} by your hand-painting.`
    : "Paint a tile by hand to teach auto-tile what you want for that shape.";
  $("taught-forget").hidden = !n;
}

/* Background tile image: built-in ids are >= 0; painted art comes from the
 * server and the edit view as -(1 + tile * 5 + variant). */
const tileImg = (t) => t < 0 ? artImgs[Math.floor((-t - 1) / 5)][(-t - 1) % 5] : bkgTiles[t];

function drawBkg(t, tx, ty) { ctx.drawImage(tileImg(t), tx * TILE, ty * TILE, TILE, TILE); }
function drawSpr(t, tx, ty) { ctx.drawImage(sprTiles[t], tx * TILE, ty * TILE, TILE, TILE); }
/* draw at a pixel offset (playtest camera), in Game Boy pixels */
function drawBkgPx(t, gx, gy) {
  ctx.drawImage(tileImg(t), gx * PLAY_SCALE, gy * PLAY_SCALE, 8 * PLAY_SCALE, 8 * PLAY_SCALE);
}
function drawSprPx(t, gx, gy) {
  ctx.drawImage(sprTiles[t], gx * PLAY_SCALE, gy * PLAY_SCALE, 8 * PLAY_SCALE, 8 * PLAY_SCALE);
}

const mapW = (lv) => lv.map[0].length;
const mapH = (lv) => lv.map.length;

function sizeCanvas(wPx, hPx) {
  if (canvas.width !== wPx || canvas.height !== hPx) {
    canvas.width = wPx;
    canvas.height = hPx;
    ctx.imageSmoothingEnabled = false;
  }
}

function charTile(c) {
  const T = A.T;
  if (c >= "0" && c <= "9") return T.FONT_BASE + (c.charCodeAt(0) - 48);
  if (c >= "A" && c <= "Z") return T.FONT_BASE + 10 + (c.charCodeAt(0) - 65);
  return { ":": T.FONT_COLON, "!": T.FONT_BANG, "-": T.FONT_DASH, ".": T.FONT_PERIOD,
           ">": T.FONT_GT, "?": T.FONT_QMARK }[c] ?? T.TILE_BLANK;
}

/* A 20-tile text row, like row_buf in main.c. */
function textRow(parts) {
  const row = new Array(20).fill(A.T.TILE_BLANK);
  for (const [x, s] of parts)
    for (let i = 0; i < s.length && x + i < 20; i++) row[x + i] = charTile(s[i]);
  return row;
}

function drawRow(row, ty) { row.forEach((t, x) => drawBkg(t, x, ty)); }

const pad3 = (n) => String(Math.min(n, 999)).padStart(3, "0");

// ----------------------------------------------------------- HUD (main.c)

function hudRows(lv, idx, v, ui) {
  const n = idx + 1;
  const num = String(n);
  const row1 = ui.mode === "aim"
    ? textRow([[0, "THROW " + A.items[ui.item].name], [16, "T" + pad3(v.turn)]])
    : textRow([[0, num], [num.length + 1, lv.name], [16, "T" + pad3(v.turn)]]);
  const G = A.G;
  let msg;
  if (ui.msg) msg = ui.msg;
  else if (v.status === G.STATUS_CAUGHT) msg = "SPOTTED! START:UNDO";
  else if (v.status === G.STATUS_ALARM) msg = "BODY SEEN START:UNDO";
  else if (v.status === G.STATUS_WON) msg = "ESCAPED!   A:NEXT";
  else if (ui.mode === "aim") msg = "A:THROW B:BACK";
  else if (v.prompt.takedown) msg = "B:TAKEDOWN";
  else if (v.prompt.talk) msg = "B:" + v.prompt.talk;
  else if (v.prompt.uniform) msg = "B:TAKE UNIFORM";
  else if (v.prompt.body_adjacent) msg = "HOLD B:DRAG";
  else if (ui.looking) msg = "LOOKING AROUND";
  else if (v.items.some((c) => c > 0)) msg = "SEL:ITEMS START:UNDO";
  else msg = "A:WAIT START:UNDO";
  return [row1, textRow([[0, msg]])];
}

// =================================================================== EDIT

function localTiles(lv) {
  const byChar = Object.fromEntries(A.tiles.map((t) => [t.char, t.tile]));
  const art = lv.art;
  return lv.map.map((r, y) => [...r].map((c, x) => {
    const a = art && art[y].slice(2 * x, 2 * x + 2);
    if (a && a !== "..") return -(1 + parseInt(a, 16) * 5);
    return byChar[c] ?? A.T.TILE_WALL;
  }));
}

// ---- tile palette: built from the server's table (tools/gen_levels.py)

/* Paint keys: 1-3 as before, then each building block's map letter. */
const TILE_KEYS = { "#": "1", ".": "2", "E": "3" };
const tileKey = (ch) => TILE_KEYS[ch] || ch;

function tileHint(t) {
  const b = t.blocks;
  const what = { player: "the player (and bodies)", guards: "guards", sight: "sight", throws: "throws" };
  const blocked = Object.keys(what).filter((k) => b[k]).map((k) => what[k]);
  const passes = Object.keys(what).filter((k) => !b[k]).map((k) => what[k]);
  const cap = t.name[0].toUpperCase() + t.name.slice(1);
  if (!blocked.length) return `${cap}: nothing is blocked. Click or drag to paint.`;
  if (!passes.length) return `${cap}: blocks everything. Click or drag to paint.`;
  return `${cap}: blocks ${blocked.join(", ")}; lets ${passes.join(", ")} through.` +
    (b.sight && !b.player ? " Anything standing on it is hidden." : "") + " Click or drag to paint.";
}

function buildPalette() {
  const box = $("palette");
  A.tiles.filter((t) => t.char !== "P").forEach((t) => {
    const b = document.createElement("button");
    b.dataset.tool = "tile:" + t.char;
    b.title = tileHint(t) + ` (key ${tileKey(t.char)})`;
    const c = document.createElement("canvas");
    c.width = c.height = 8;
    c.getContext("2d").drawImage(bkgTiles[t.tile], 0, 0);
    const label = document.createElement("span");
    label.innerHTML = `<b>${tileKey(t.char)}</b> ${t.name[0].toUpperCase() + t.name.slice(1)}`;
    b.append(c, label);
    b.addEventListener("click", () => setTool(b.dataset.tool));
    box.appendChild(b);
  });
}

function guardBase(g) {
  return g.uniform === "officer" ? A.T.SPR_OFFICER_UP : A.T.SPR_GUARD_UP;
}

function renderEdit() {
  const lv = cmap();
  TILE = 8 * editZoom(lv);
  sizeCanvas(mapW(lv) * TILE, mapH(lv) * TILE);
  const fresh = preview && preview.key === previewKey() && previewDisguise >= 0;
  const tiles = fresh ? preview.view.tiles : localTiles(lv);
  tiles.forEach((row, y) => row.forEach((t, x) => drawBkg(t, x, y)));
  drawScreenGuides(lv);

  // routes under the sprites
  lv.guards.forEach((g, i) => drawRoute(g, i, i === selGuard));

  // player start and guards at their first point
  lv.map.forEach((r, y) => [...r].forEach((c, x) => {
    if (c === "P") drawSpr(A.T.SPR_PLAYER_NONE + (fresh ? preview.view.disguise : 0), x, y);
  }));
  lv.guards.forEach((g) => {
    const [x, y] = g.route[0];
    drawSpr(guardBase(g) + A.dirs.indexOf(g.facing), x, y);
  });
  if (selGuard >= 0 && lv.guards[selGuard]) {
    const [x, y] = lv.guards[selGuard].route[0];
    ctx.strokeStyle = GUARD_COLORS[selGuard];
    ctx.lineWidth = 3;
    ctx.strokeRect(x * TILE + 1.5, y * TILE + 1.5, TILE - 3, TILE - 3);
  }

  drawScripted(lv);
  drawLinkMarkers();
  if ($("show-behaviour").checked && lv.art) drawBehaviourLetters(lv);
  updateSizeInfo(lv);
  updateArtBudget(lv);
}

/* NPCs and switch cells. An NPC's look is whatever is painted on its cell,
 * so it gets an outline and its prompt word. A switch cell shows what it
 * turns into, faintly, inside a dashed outline: stairs that aren't there
 * yet are still visible to the designer. */
function drawScripted(lv) {
  ctx.save();
  ctx.font = `bold ${Math.max(8, TILE * 0.4)}px ui-monospace, monospace`;
  ctx.textAlign = "center";
  (lv.switches || []).forEach((sw, i) => {
    const def = A.tiles.find((t) => t.char === sw.on);
    const img = sw.on_art !== null && sw.on_art !== undefined && artImgs[sw.on_art]
      ? artImgs[sw.on_art][0] : def ? bkgTiles[def.tile] : null;
    if (img) {
      ctx.globalAlpha = 0.55;
      ctx.drawImage(img, sw.x * TILE, sw.y * TILE, TILE, TILE);
      ctx.globalAlpha = 1;
    }
    ctx.strokeStyle = SWITCH_COLOR;
    ctx.lineWidth = i === selSwitch ? 3 : 2;
    ctx.setLineDash([4, 3]);
    ctx.strokeRect(sw.x * TILE + 1.5, sw.y * TILE + 1.5, TILE - 3, TILE - 3);
    ctx.setLineDash([]);
  });
  (lv.npcs || []).forEach((n, i) => {
    ctx.strokeStyle = NPC_COLOR;
    ctx.lineWidth = i === selNpc ? 3 : 2;
    ctx.strokeRect(n.x * TILE + 1.5, n.y * TILE + 1.5, TILE - 3, TILE - 3);
    const w = ctx.measureText(n.verb).width + 4, h = Math.max(9, TILE * 0.45);
    ctx.fillStyle = NPC_COLOR;
    ctx.fillRect(n.x * TILE + TILE / 2 - w / 2, n.y * TILE - h, w, h);
    ctx.fillStyle = "#fff";
    ctx.textBaseline = "middle";
    ctx.fillText(n.verb, n.x * TILE + TILE / 2, n.y * TILE - h / 2 + 0.5);
  });
  ctx.restore();
}

/* Painted art can hide what a cell does: label painted cells whose
 * behaviour isn't what that tile normally means (e.g. a wall-looking tile
 * on a walkable cell). Plain floor under decoration isn't labelled. */
function drawBehaviourLetters(lv) {
  ctx.save();
  ctx.font = `bold ${Math.max(9, TILE * 0.55)}px ui-monospace, monospace`;
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  lv.map.forEach((r, y) => [...r].forEach((c, x) => {
    const code = lv.art[y].slice(2 * x, 2 * x + 2);
    if (code === "..") return;
    const usual = behaviour[String(parseInt(code, 16))];
    if (c === "P" || c === usual || (c === "." && !usual)) return;
    const cx = x * TILE + TILE / 2, cy = y * TILE + TILE / 2;
    ctx.fillStyle = "rgba(15, 20, 17, 0.6)";
    ctx.fillRect(cx - TILE * 0.32, cy - TILE * 0.32, TILE * 0.64, TILE * 0.64);
    ctx.fillStyle = "#f2b705";
    ctx.fillText(c, cx, cy + 0.5);
  }));
  ctx.restore();
}

/* Game Boy tile slots this map's art needs (as tools/tileset.py slots_for:
 * 5 for tiles on floor, 3 on exits/mesh, 1 elsewhere). */
function artSlots(lv) {
  if (!lv.art) return { tiles: 0, slots: 0 };
  const used = {};
  lv.map.forEach((r, y) => [...r].forEach((c, x) => {
    const a = lv.art[y].slice(2 * x, 2 * x + 2);
    if (a === "..") return;
    const need = ".P".includes(c) ? 5 : "EM".includes(c) ? 3 : 1;
    used[a] = Math.max(used[a] || 0, need);
  }));
  return { tiles: Object.keys(used).length, slots: Object.values(used).reduce((s, n) => s + n, 0) };
}

function updateArtBudget(lv) {
  const u = artSlots(lv);
  $("art-budget").textContent = u.tiles
    ? `This map paints ${u.tiles} different tiles, using ${u.slots} of ${A.art_slots} Game Boy tile slots.`
    : "Pick a tile and paint: unpainted cells use the built-in look.";
  $("art-budget").classList.toggle("errors", u.slots > A.art_slots);
}

/* Dashed lines on the screen boundaries: the Game Boy shows one screen at
 * a time and cuts to the next when you cross a line. */
function drawScreenGuides(lv) {
  const w = mapW(lv), h = mapH(lv);
  if (w <= A.view_w && h <= A.view_h) return;
  ctx.save();
  ctx.strokeStyle = "rgba(242, 183, 5, 0.85)";
  ctx.lineWidth = 2;
  ctx.setLineDash([6, 6]);
  for (let x = A.view_w; x < w; x += A.view_w) {
    ctx.beginPath(); ctx.moveTo(x * TILE, 0); ctx.lineTo(x * TILE, h * TILE); ctx.stroke();
  }
  for (let y = A.view_h; y < h; y += A.view_h) {
    ctx.beginPath(); ctx.moveTo(0, y * TILE); ctx.lineTo(w * TILE, y * TILE); ctx.stroke();
  }
  ctx.restore();
}

/* "Fit": the biggest zoom that keeps the map within about 720x600 px. */
function editZoom(lv) {
  if (zoomSetting !== "fit") return parseInt(zoomSetting, 10);
  for (let z = 3; z > 1; z--)
    if (mapW(lv) * 8 * z <= 720 && mapH(lv) * 8 * z <= 600) return z;
  return 1;
}

function updateSizeInfo(lv) {
  const w = mapW(lv), h = mapH(lv);
  const sx = w / A.view_w, sy = h / A.view_h;
  const fits = (x, y) => x * A.view_w <= A.map_max_w && y * A.view_h <= A.map_max_h &&
                         x * A.view_w * y * A.view_h <= A.map_max_cells;
  // only offer sizes that fit in RAM alongside the other dimension
  for (const [id, n, other, isX] of [["screens-x", sx, sy, true], ["screens-y", sy, sx, false]]) {
    const sel = $(id);
    sel.innerHTML = "";
    for (let k = 1; k <= 3; k++) {
      const o = new Option(String(k), String(k));
      o.disabled = !(isX ? fits(k, other) : fits(other, k));
      sel.add(o);
    }
    sel.value = Number.isInteger(n) ? String(n) : "";
  }
  $("size-info").textContent = `${w}×${h} tiles = ${w * h} of ${A.map_max_cells}. ` +
    (sx * sy > 1 ? "Dashed lines are the screen boundaries." : "");
}

/* Resize the map (in whole screens): new area is wall; cutting off part of
 * the map can strand the start, exits or guards, which the problems list
 * will point out. */
function resizeMap(w, h) {
  const lv = cmap();
  if (w * h > A.map_max_cells) return;
  if (w === mapW(lv) && h === mapH(lv)) return;
  pushUndo();
  const rows = [];
  for (let y = 0; y < h; y++) {
    const old = lv.map[y] || "";
    rows.push((old + "#".repeat(w)).slice(0, w));
  }
  lv.map = rows;
  if (lv.art) {
    const art = [];
    for (let y = 0; y < h; y++) art.push(((lv.art[y] || "") + "..".repeat(w)).slice(0, 2 * w));
    lv.art = art;
  }
  changed();
}

function drawRoute(g, i, selected) {
  const pts = g.route.map(([x, y]) => [x * TILE + TILE / 2, y * TILE + TILE / 2]);
  ctx.save();
  ctx.strokeStyle = GUARD_COLORS[i];
  ctx.fillStyle = GUARD_COLORS[i];
  ctx.globalAlpha = selected ? 1 : 0.6;
  ctx.lineWidth = selected ? 3 : 2;
  if (pts.length > 1) {
    ctx.beginPath();
    pts.forEach(([x, y], k) => (k ? ctx.lineTo(x, y) : ctx.moveTo(x, y)));
    ctx.stroke();
    ctx.setLineDash([5, 5]);            // the leg back to the start
    ctx.beginPath();
    ctx.moveTo(...pts[pts.length - 1]);
    ctx.lineTo(...pts[0]);
    ctx.stroke();
    ctx.setLineDash([]);
  }
  pts.forEach(([x, y], k) => {
    if (k === 0) return;
    ctx.beginPath();
    ctx.arc(x, y, 8, 0, Math.PI * 2);
    ctx.fill();
    ctx.fillStyle = "#fff";
    ctx.font = "bold 11px system-ui, sans-serif";
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText(String(k + 1), x, y + 0.5);
    ctx.fillStyle = GUARD_COLORS[i];
  });
  ctx.restore();
}

// ------------------------------------------------------- edit history

function snapshot() { return JSON.stringify({ levels, cur, behaviour, taught }); }

function pushUndo() {
  undoStack.push(snapshot());
  if (undoStack.length > 300) undoStack.shift();
  redoStack = [];
}

function restore(s) {
  const o = JSON.parse(s);
  levels = o.levels;
  if (o.behaviour) { behaviour = o.behaviour; drawTileset(); }
  if (o.taught) { taught = o.taught; updateTaughtInfo(); }
  cur = Math.min(o.cur, levels.length - 1);
  curMap = Math.min(curMap, level().maps.length - 1);
  if (selGuard >= cmap().guards.length) selGuard = -1;
  changed(false);
}

function undoEdit() {
  if (!undoStack.length) return;
  redoStack.push(snapshot());
  restore(undoStack.pop());
}

function redoEdit() {
  if (!redoStack.length) return;
  undoStack.push(snapshot());
  restore(redoStack.pop());
}

/* Call after any change to level data. */
function changed(markDirty = true) {
  if (markDirty) setDirty(true);
  lastSolve = null;
  $("btn-watch").disabled = true;
  renderAll();
  checkLevel();
}

function setDirty(d) {
  dirty = d;
  const el = $("save-state");
  el.textContent = d ? "Unsaved changes" : "All changes saved";
  el.classList.toggle("dirty", d);
}

// ------------------------------------------------------- mouse editing

let stroke = null;   // {before, changed, drag}

function tileAt(e) {
  const r = canvas.getBoundingClientRect();
  const lv = cmap();
  const x = Math.floor((e.clientX - r.left) / (r.width / mapW(lv)));
  const y = Math.floor((e.clientY - r.top) / (r.height / mapH(lv)));
  return [x, y];
}

const inMap = (x, y) => x >= 0 && y >= 0 && x < mapW(cmap()) && y < mapH(cmap());

function setCell(x, y, c) {
  const lv = cmap();
  if (lv.map[y][x] === c) return false;
  if (c === "P")   // one start per level, in whichever map you put it
    level().maps.forEach((m) => { m.map = m.map.map((r) => r.replace("P", ".")); });
  const row = lv.map[y];
  lv.map[y] = row.slice(0, x) + c + row.slice(x + 1);
  return true;
}

/* Paint (or with n = null, erase) art on one cell; unless "look only" is
 * on, also set the cell's behaviour to the tile's (keeping the start). */
function paintArt(x, y, n) {
  const m = cmap();
  if (!m.art) {
    if (n === null) return false;
    m.art = m.map.map((r) => "..".repeat(r.length));
  }
  const code = n === null ? ".." : n.toString(16).padStart(2, "0");
  let changed_ = false;
  if (m.art[y].slice(2 * x, 2 * x + 2) !== code) {
    m.art[y] = m.art[y].slice(0, 2 * x) + code + m.art[y].slice(2 * x + 2);
    changed_ = true;
  }
  const b = n !== null && behaviour[String(n)];
  if (b && !$("art-only").checked) {
    const keepStart = m.map[y][x] === "P" && !A.tiles.find((d) => d.char === b).blocks.player;
    if (!keepStart && m.map[y][x] !== b) {
      if (b === "P") return changed_;
      const row = m.map[y];
      m.map[y] = row.slice(0, x) + b + row.slice(x + 1);
      changed_ = true;
    }
  }
  if (m.art.every((r) => /^(\.\.)*$/.test(r))) delete m.art;   // nothing painted
  return changed_;
}

function guardAt(x, y) {
  return cmap().guards.findIndex((g) => g.route[0][0] === x && g.route[0][1] === y);
}

function waypointAt(g, x, y) {
  return g.route.findIndex(([px, py]) => px === x && py === y);
}

function addWaypoint(g, x, y) {
  const [lx, ly] = g.route[g.route.length - 1];
  if (lx === x && ly === y) return false;
  const add = (lx === x || ly === y) ? [[x, y]] : [[x, ly], [x, y]];  // horizontal first, like the guards
  if (g.route.length + add.length > A.max_waypoints) {
    hint(`A route can have at most ${A.max_waypoints} points.`);
    return false;
  }
  g.route.push(...add);
  return true;
}

function applyTool(x, y, e, first) {
  const lv = cmap();
  if (tool.startsWith("art:") || tool === "art-erase") {
    const n = tool === "art-erase" ? null : parseInt(tool.slice(4), 10);
    const did = paintArt(x, y, n);
    if (did) teach(lv, x, y, n);
    return did;
  }
  const paint = tool.startsWith("tile:") ? tool.slice(5) : null;
  if (paint) {
    const did = setCell(x, y, paint);
    if (did && stroke) (stroke.cells = stroke.cells || []).push([x, y]);
    return did;
  }
  if (tool === "start") return first ? setCell(x, y, "P") : false;

  if (tool === "guard") {
    if (first) {
      const i = guardAt(x, y);
      if (i >= 0) { selGuard = i; stroke.drag = { guard: i, point: 0 }; renderGuards(); return false; }
      const here = A.tiles.find((d) => d.char === lv.map[y][x]);
      if (here && here.blocks.guards) { hint(`Guards can't stand on a ${here.name}.`); return false; }
      if (lv.guards.length >= A.max_guards) { hint(`At most ${A.max_guards} guards per level.`); return false; }
      lv.guards.push({ facing: "down", route: [[x, y]], uniform: "none", attitude: null });
      selGuard = lv.guards.length - 1;
      stroke.drag = { guard: selGuard, point: 0 };
      return true;
    }
    return moveDragged(x, y);
  }

  if (tool === "npc" || tool === "switch") {
    const isNpc = tool === "npc";
    const key = isNpc ? "npcs" : "switches";
    if (!first) return moveDragged(x, y);
    const list = lv[key] || [];
    const i = list.findIndex((d) => d.x === x && d.y === y);
    if (i >= 0) {
      if (isNpc) selNpc = i; else selSwitch = i;
      stroke.drag = { thing: key, index: i };
      renderScripting();
      renderEdit();
      return false;
    }
    const max = isNpc ? A.max_npcs : A.max_switches;
    if (list.length >= max) { hint(`At most ${max} ${isNpc ? "NPCs" : "switch cells"} in a map.`); return false; }
    if (isNpc) {
      list.push({ x, y, verb: "TALK", blocks: "move", script: [{ say: "Hello." }] });
      selNpc = list.length - 1;
    } else {
      // stairs that appear are the usual case: an exit, with the tileset's
      // exit tile if this map is painted
      const exitArt = lv.art && A.tileset.autopaint ? A.tileset.autopaint.exit : null;
      list.push({ x, y, flag: levelFlags()[0] || "open", on: "E", on_art: exitArt ?? null });
      selSwitch = list.length - 1;
    }
    lv[key] = list;
    stroke.drag = { thing: key, index: list.length - 1 };
    return true;
  }

  if (tool === "route") {
    const g = lv.guards[selGuard];
    if (!g) { if (first) hint("Select a guard first (Guard tool, or click a guard chip)."); return false; }
    if (!first) return moveDragged(x, y);
    const w = waypointAt(g, x, y);
    if (e.button === 2) {
      if (w > 0) { g.route.splice(w, 1); return true; }
      return false;
    }
    if (w >= 0) { stroke.drag = { guard: selGuard, point: w }; return false; }
    return addWaypoint(g, x, y);
  }
  return false;
}

function moveDragged(x, y) {
  if (!stroke.drag) return false;
  if (stroke.drag.thing) {                 // an NPC or a switch cell
    const d = cmap()[stroke.drag.thing][stroke.drag.index];
    if (d.x === x && d.y === y) return false;
    d.x = x; d.y = y;
    return true;
  }
  const g = cmap().guards[stroke.drag.guard];
  const p = g.route[stroke.drag.point];
  if (p[0] === x && p[1] === y) return false;
  g.route[stroke.drag.point] = [x, y];
  return true;
}

canvas.addEventListener("contextmenu", (e) => { if (mode === "edit") e.preventDefault(); });

canvas.addEventListener("mousedown", (e) => {
  if (mode !== "edit") return;
  const [x, y] = tileAt(e);
  if (!inMap(x, y)) return;
  stroke = { before: snapshot(), changed: false, drag: null };
  if (applyTool(x, y, e, true)) { stroke.changed = true; renderAll(); }
  e.preventDefault();
});

canvas.addEventListener("mousemove", (e) => {
  if (!stroke) return;
  const [x, y] = tileAt(e);
  if (!inMap(x, y)) return;
  if (applyTool(x, y, e, false)) { stroke.changed = true; renderAll(); }
});

window.addEventListener("mouseup", () => {
  if (!stroke) return;
  if (stroke.changed) {
    undoStack.push(stroke.before);
    redoStack = [];
    changed();
    if (stroke.cells && $("autotile").checked) autoTile(stroke.cells);
  }
  stroke = null;
});

/* Repaint the art around cells whose behaviour just changed (part of the
 * same undo step as the stroke). A map with no art gets painted whole. */
async function autoTile(cells) {
  const m = cmap(), lvIdx = cur, mapIdx = curMap;
  const res = await api("/api/autotile", { levels, taught, level: lvIdx, map_index: mapIdx,
                                           map: m.map, art: m.art || null, changed: cells,
                                           keep: scriptedCells(m) });
  if (res.error || lvIdx !== cur || mapIdx !== curMap) return;
  m.art = res.art;
  changed();
}

function hint(msg) { $("tool-hint").textContent = msg; }

function setTool(t) {
  tool = t;
  document.querySelectorAll("#toolbar button, #palette button, .art-panel button[data-tool]").forEach((b) =>
    b.classList.toggle("active", b.dataset.tool === t));
  const tile = t.startsWith("tile:") && A.tiles.find((x) => x.char === t.slice(5));
  if (t.startsWith("art:")) {
    artTile = parseInt(t.slice(4), 10);
    drawTileset();
    const b = behaviour[String(artTile)];
    const d = b && A.tiles.find((x) => x.char === b);
    hint(`Painting tileset tile ${artTile}` + (d && !$("art-only").checked
      ? `: also makes the cell a ${d.name}.` : ": changes only the look."));
  } else if (t === "art-erase") {
    hint("Click or drag to remove painted art; the cell's behaviour stays.");
  } else {
    hint(tile ? tileHint(tile) : TOOL_HINTS[t]);
  }
}

// ------------------------------------------------------------- panels

function renderAll() {
  renderLevelList();
  renderMapTabs();
  renderLinks();
  renderProps();
  renderGuards();
  renderScripting();
  if (mode === "edit") renderEdit();
}

function renderLevelList() {
  const ol = $("level-list");
  ol.innerHTML = "";
  levels.forEach((lv, i) => {
    const li = document.createElement("li");
    li.textContent = (lv.name || "(unnamed)") + (lv.maps.length > 1 ? ` (${lv.maps.length} maps)` : "");
    li.classList.toggle("active", i === cur);
    li.classList.toggle("invalid", validity[i] === false);
    li.addEventListener("click", () => selectLevel(i));
    ol.appendChild(li);
  });
}

function renderProps() {
  const lv = level();
  if (document.activeElement !== $("lv-name")) $("lv-name").value = lv.name;
  if (document.activeElement !== $("lv-notes")) $("lv-notes").value = lv.notes || "";
  const box = $("lv-items");
  if (!box.children.length) {
    A.items.forEach((it) => {
      const key = it.name.toLowerCase();
      const l = document.createElement("label");
      l.textContent = it.name[0] + it.name.slice(1).toLowerCase() + "s";
      const inp = document.createElement("input");
      inp.type = "number"; inp.min = 0; inp.max = 99; inp.id = "item-" + key;
      inp.addEventListener("change", () => {
        pushUndo();
        level().items[key] = Math.max(0, Math.min(99, parseInt(inp.value || "0", 10)));
        changed();
      });
      l.appendChild(inp);
      box.appendChild(l);
    });
  }
  $("lv-transition").value = lv.transition || "fade";
  A.items.forEach((it) => {
    const key = it.name.toLowerCase();
    $("item-" + key).value = (lv.items || {})[key] || 0;
  });
}

function renderGuards() {
  const lv = cmap();
  $("guard-count").textContent = `${lv.guards.length}/${A.max_guards}`;
  const list = $("guard-list");
  list.innerHTML = "";
  lv.guards.forEach((g, i) => {
    const b = document.createElement("button");
    b.className = "guard-chip" + (i === selGuard ? " active" : "");
    b.innerHTML = `<span class="dot" style="background:${GUARD_COLORS[i]}"></span>Guard ${i + 1}`;
    b.title = g.route.length === 1 ? "Sentry" : `Patrol, ${g.route.length} points`;
    b.addEventListener("click", () => { selGuard = i; renderAll(); });
    list.appendChild(b);
  });
  if (!lv.guards.length) list.innerHTML = '<span class="muted">None yet: use the Guard tool (5).</span>';
  const g0 = lv.guards[selGuard];
  if (g0) {
    const [fx, fy] = g0.route[0], [lx, ly] = g0.route[g0.route.length - 1];
    $("g-route-close").disabled = g0.route.length < 2 || lx === fx || ly === fy;
  }

  const g = lv.guards[selGuard];
  $("guard-props").hidden = !g;
  if (!g) return;
  $("g-facing").value = g.facing;
  $("g-uniform").value = g.uniform || "none";
  $("g-custom").checked = !!g.attitude;
  const att = $("g-attitudes");
  att.innerHTML = "";
  ["own clothes", "GUARD", "OFFICER"].forEach((label, k) => {
    const s = document.createElement("span");
    s.textContent = "To " + label;
    const sel = document.createElement("select");
    [["H", "Hostile (big cone)"], ["E", "Enforcer (normal)"], ["B", "Blind (watches bodies)"]]
      .forEach(([v, t]) => sel.add(new Option(t, v)));
    sel.value = g.attitude ? g.attitude[k] : "E";
    sel.disabled = !g.attitude;
    sel.addEventListener("change", () => {
      pushUndo();
      const a = [...g.attitude];
      a[k] = sel.value;
      g.attitude = a.join("");
      changed();
    });
    att.append(s, sel);
  });
  const ol = $("g-route");
  ol.innerHTML = "";
  g.route.forEach(([x, y], k) => {
    const li = document.createElement("li");
    li.textContent = `(${x}, ${y})` + (k === 0 ? " start" : "");
    if (k > 0) {
      const del = document.createElement("button");
      del.textContent = "✕";
      del.title = "Remove point";
      del.addEventListener("click", () => { pushUndo(); g.route.splice(k, 1); changed(); });
      li.appendChild(del);
    }
    ol.appendChild(li);
  });
}

// ---- NPCs, their scripts, and switch cells

/* Cells whose art auto-paint must leave alone (painted by hand). */
const scriptedCells = (m) => [...(m.npcs || []), ...(m.switches || [])].map((d) => [d.x, d.y]);

/* Every step of a script, including those inside "if" steps. */
function* scriptSteps(script) {
  for (const st of script || []) {
    yield st;
    if ("if" in st) { yield* scriptSteps(st.then); yield* scriptSteps(st.else); }
  }
}

/* The level's flag names (tools/gen_levels.py level_flags). */
function levelFlags() {
  const names = [];
  level().maps.forEach((m) => (m.switches || []).forEach((sw) => names.push(sw.flag)));
  level().maps.forEach((m) => (m.npcs || []).forEach((n) => {
    for (const st of scriptSteps(n.script))
      for (const k of ["set", "clear", "if"]) if (k in st) names.push(st[k]);
  }));
  return [...new Set(names.filter((n) => typeof n === "string" && n))];
}

let scriptTyping = false;     // typing in one of these panels: don't rebuild it

/* A change typed into a text box of these panels: one undo step per visit
 * to the box, and the panel isn't rebuilt under the cursor (nor when the
 * box loses the focus: that would eat the click that took it). */
function typedField(el, apply) {
  let fresh = true;
  el.addEventListener("input", () => {
    if (fresh) { pushUndo(); fresh = false; }
    apply(el.value);
    scriptTyping = true;
    changed();
    scriptTyping = false;
  });
  el.addEventListener("blur", () => { fresh = true; });
}

const STEP_KINDS = [["say", "Say"], ["set", "Set flag"], ["clear", "Clear flag"], ["if", "If flag"]];
const stepKind = (st) => (STEP_KINDS.find(([k]) => k in st) || STEP_KINDS[0])[0];

/* How a message will look in the Game Boy's box (asks the server, which
 * wraps it exactly as the ROM's text is made). */
async function showBox(text, box) {
  const res = await api("/api/wrap", { text });
  box.classList.toggle("problem", !!res.problem);
  box.textContent = res.problem ? "Can't show this: " + res.problem + "." : res.lines.join("\n");
}

/* The editor for a list of steps (a script, or one branch of an "if"). */
function scriptEditor(steps, depth = 0) {
  const box = document.createElement("div");
  const structural = (fn) => { pushUndo(); fn(); changed(); };
  steps.forEach((st, i) => {
    const kind = stepKind(st);
    const row = document.createElement("div");
    row.className = "step";
    const sel = document.createElement("select");
    STEP_KINDS.forEach(([k, label]) => (depth < 3 || k !== "if") && sel.add(new Option(label, k)));
    sel.value = kind;
    sel.addEventListener("change", () => structural(() => {
      const flag = levelFlags()[0] || "open";
      steps[i] = { say: { say: "..." }, set: { set: flag }, clear: { clear: flag },
                   if: { if: flag, then: [], else: [] } }[sel.value];
    }));
    const arg = document.createElement("input");
    arg.spellcheck = false;
    arg.value = st[kind] ?? "";
    if (kind === "say") arg.placeholder = "What it says";
    else { arg.setAttribute("list", "flag-names"); arg.placeholder = "flag name"; }
    const del = document.createElement("button");
    del.textContent = "✕";
    del.title = "Remove this step";
    del.addEventListener("click", () => structural(() => steps.splice(i, 1)));
    row.append(sel, arg, del);
    if (kind === "say") {
      const preview = document.createElement("div");
      preview.className = "box";
      row.appendChild(preview);
      showBox(st.say, preview);
      const later = debounce(showBox, 150);          // one timer per message
      typedField(arg, (v) => { st.say = v; later(v, preview); });
    } else {
      typedField(arg, (v) => { st[kind] = v.trim(); });
    }
    box.appendChild(row);
    if (kind === "if") {
      for (const [key, label] of [["then", "then"], ["else", "otherwise"]]) {
        st[key] = st[key] || [];
        const br = document.createElement("div");
        br.className = "branch";
        br.innerHTML = `<div class="branch-label">${label}</div>`;
        br.appendChild(scriptEditor(st[key], depth + 1));
        box.appendChild(br);
      }
    }
  });
  const add = document.createElement("button");
  add.className = "add";
  add.textContent = "+ step";
  add.addEventListener("click", () => structural(() => steps.push({ say: "..." })));
  box.appendChild(add);
  return box;
}

function renderScripting() {
  const m = cmap();
  const npcs = m.npcs || [], switches = m.switches || [];
  if (selNpc >= npcs.length) selNpc = -1;
  if (selSwitch >= switches.length) selSwitch = -1;
  $("flag-names").innerHTML = "";
  levelFlags().forEach((f) => $("flag-names").appendChild(new Option(f)));

  const chips = (id, list, sel, label, pick, none) => {
    const box = $(id);
    box.innerHTML = "";
    list.forEach((d, i) => {
      const b = document.createElement("button");
      b.className = "guard-chip" + (i === sel ? " active" : "");
      b.textContent = label(d, i);
      b.addEventListener("click", () => { pick(i); renderAll(); });
      box.appendChild(b);
    });
    if (!list.length) box.innerHTML = `<span class="muted">${none}</span>`;
  };
  $("npc-count").textContent = `${npcs.length}/${A.max_npcs}`;
  chips("npc-list", npcs, selNpc, (n) => `${n.verb} (${n.x},${n.y})`, (i) => { selNpc = i; },
        "None yet: use the NPC tool (7).");
  $("sw-count").textContent = `${switches.length}/${A.max_switches}`;
  chips("sw-list", switches, selSwitch, (s) => `${s.flag} (${s.x},${s.y})`, (i) => { selSwitch = i; },
        "None yet: use the Switch cell tool (8).");

  if (scriptTyping) return;     // the lists above are enough while typing
  const n = npcs[selNpc];
  $("npc-props").hidden = !n;
  if (n) {
    if (document.activeElement !== $("n-verb")) $("n-verb").value = n.verb;
    $("n-blocks").value = n.blocks || "move";
    n.script = n.script || [];
    $("n-script").replaceChildren(scriptEditor(n.script));
  }
  const sw = switches[selSwitch];
  $("sw-props").hidden = !sw;
  if (sw) {
    if (document.activeElement !== $("s-flag")) $("s-flag").value = sw.flag;
    $("s-on").value = sw.on;
    $("s-art-info").textContent = sw.on_art === null || sw.on_art === undefined
      ? "the built-in graphic" : `tileset tile ${sw.on_art}`;
  }
}

function deleteScripted(key) {
  const m = cmap();
  const i = key === "npcs" ? selNpc : selSwitch;
  if (!m[key] || i < 0) return;
  pushUndo();
  m[key].splice(i, 1);
  if (!m[key].length) delete m[key];
  if (key === "npcs") selNpc = -1; else selSwitch = -1;
  changed();
}

function selectLevel(i) {
  cur = i;
  curMap = 0;
  selGuard = -1;
  selNpc = selSwitch = -1;
  preview = null;
  if (mode === "play") enterEdit();
  changed(false);
}

// ------------------------------------------------------ maps and links

const EDGES = ["up", "right", "down", "left"];
const OPP = { up: "down", down: "up", left: "right", right: "left" };
const previewKey = () => keyOf(level()) + "#" + curMap;

/* The link on edge `edge` of map k, seen from map k: {to, offset} or null.
 * Links are stored once (from -> to) and work both ways. */
function linkOf(k, edge) {
  for (const ln of level().links) {
    if (ln.from === k && ln.edge === edge) return { to: ln.to, offset: ln.offset || 0 };
    if (ln.to === k && OPP[ln.edge] === edge) return { to: ln.from, offset: -(ln.offset || 0) };
  }
  return null;
}

/* Join (or with to = null, unjoin) edge `edge` of map k. Anything already
 * on either edge is replaced. */
function setLink(k, edge, to, offset) {
  const lv = level();
  lv.links = lv.links.filter((ln) =>
    !((ln.from === k && ln.edge === edge) || (ln.to === k && OPP[ln.edge] === edge)) &&
    !(to !== null && ((ln.from === to && ln.edge === OPP[edge]) || (ln.to === to && ln.edge === edge))));
  if (to !== null) lv.links.push({ from: k, edge, to, offset });
}

function renderMapTabs() {
  const box = $("map-tabs");
  const lv = level();
  box.innerHTML = "";
  lv.maps.forEach((m, k) => {
    const b = document.createElement("button");
    b.className = "tab" + (k === curMap ? " active" : "");
    b.textContent = `Map ${k + 1}`;
    b.title = `${mapW(m) / A.view_w}×${mapH(m) / A.view_h} screens` + (k === 0 ? "; the level starts here" : "");
    b.addEventListener("click", () => { curMap = k; selGuard = selNpc = selSwitch = -1; preview = null; changed(false); });
    box.appendChild(b);
  });
  const add = document.createElement("button");
  add.className = "tab";
  add.textContent = "+ Map";
  add.title = `Add a map (max ${A.max_maps}); join it with the Links panel`;
  add.disabled = lv.maps.length >= A.max_maps;
  add.addEventListener("click", () => {
    pushUndo();
    lv.maps.push({ map: emptyRoom(), guards: [] });
    curMap = lv.maps.length - 1;
    selGuard = -1;
    changed();
  });
  box.appendChild(add);
}

function renderLinks() {
  const lv = level();
  const box = $("links");
  box.innerHTML = "";
  $("map-delete").disabled = curMap === 0;
  $("links-section").hidden = lv.maps.length < 2;
  if (lv.maps.length < 2) return;
  EDGES.forEach((edge) => {
    const ln = linkOf(curMap, edge);
    const label = document.createElement("span");
    label.textContent = edge[0].toUpperCase() + edge.slice(1) + " edge";
    const sel = document.createElement("select");
    sel.add(new Option("not linked", ""));
    lv.maps.forEach((m, k) => { if (k !== curMap) sel.add(new Option(`→ Map ${k + 1}`, String(k))); });
    sel.value = ln ? String(ln.to) : "";
    const off = document.createElement("input");
    off.type = "number";
    off.value = ln ? ln.offset : 0;
    off.disabled = !ln;
    off.title = edge === "left" || edge === "right"
      ? "Which row of this map the other map's top row lines up with"
      : "Which column of this map the other map's left column lines up with";
    const apply = () => {
      pushUndo();
      setLink(curMap, edge, sel.value === "" ? null : parseInt(sel.value, 10),
              parseInt(off.value || "0", 10));
      changed();
    };
    sel.addEventListener("change", apply);
    off.addEventListener("change", apply);
    box.append(label, sel, off);
  });
}

function deleteMap() {
  const lv = level();
  if (curMap === 0) return;
  if (!confirm(`Delete map ${curMap + 1} of this level?`)) return;
  pushUndo();
  const k = curMap;
  lv.maps.splice(k, 1);
  lv.links = lv.links.filter((ln) => ln.from !== k && ln.to !== k)
    .map((ln) => ({ ...ln, from: ln.from > k ? ln.from - 1 : ln.from, to: ln.to > k ? ln.to - 1 : ln.to }));
  curMap = k - 1;
  selGuard = -1;
  changed();
}

/* A strip along each linked edge, labelled with where it leads. */
function drawLinkMarkers() {
  const m = cmap();
  const w = mapW(m) * TILE, h = mapH(m) * TILE;
  ctx.save();
  ctx.font = "bold 12px system-ui, sans-serif";
  ctx.textBaseline = "middle";
  EDGES.forEach((edge) => {
    const ln = linkOf(curMap, edge);
    if (!ln) return;
    const t = 5;
    ctx.fillStyle = "rgba(242, 183, 5, 0.55)";
    if (edge === "up") ctx.fillRect(0, 0, w, t);
    if (edge === "down") ctx.fillRect(0, h - t, w, t);
    if (edge === "left") ctx.fillRect(0, 0, t, h);
    if (edge === "right") ctx.fillRect(w - t, 0, t, h);
    const text = `Map ${ln.to + 1}`;
    const tw = ctx.measureText(text).width + 10;
    const [bx, by] = edge === "up" ? [w / 2 - tw / 2, 4] : edge === "down" ? [w / 2 - tw / 2, h - 22]
      : edge === "left" ? [6, h / 2 - 9] : [w - tw - 6, h / 2 - 9];
    ctx.fillStyle = "rgba(15, 20, 17, 0.85)";
    ctx.fillRect(bx, by, tw, 18);
    ctx.fillStyle = "#f2b705";
    ctx.fillText(text, bx + 5, by + 9);
  });
  ctx.restore();
}

// ------------------------------------------------- validation & preview

const checkLevel = debounce(async () => {
  const idx = cur;
  const lv = clone(level());
  const k = keyOf(lv);
  const res = await api("/api/validate", { level: lv });
  if (idx !== cur || keyOf(level()) !== k) return;   // stale
  validity[idx] = res.errors.length === 0;
  const ul = $("errors");
  ul.innerHTML = "";
  if (res.errors.length) {
    res.errors.forEach((e) => { const li = document.createElement("li"); li.textContent = e; ul.appendChild(li); });
    preview = null;
  } else {
    ul.innerHTML = '<li class="ok">No problems found.</li>';
    if (previewDisguise >= 0) {
      const m = curMap;
      const p = await api("/api/start", { level: lv, disguise: previewDisguise, map: m });
      if (!p.error && keyOf(level()) === k && curMap === m) preview = { key: k + "#" + m, view: p.view };
    }
  }
  renderLevelList();
  if (mode === "edit") renderEdit();
}, 150);

// ------------------------------------------------------------- solving

const NEED_NAMES = { items: "items", takedown: "a takedown", drag: "moving a body", uniform: "a disguise",
                     talk: "talking to an NPC" };

async function solve() {
  const out = $("solve-result");
  const lv = clone(level());
  out.textContent = "Solving… (checks every possible move; big levels take a few seconds)";
  $("btn-solve").disabled = true;
  try {
    const res = await api("/api/solve", { level: lv });
    if (res.error) { out.textContent = res.error; return; }
    const s = res.solve;
    if (s.length === 0) {
      out.innerHTML = `<b>Can't be beaten.</b> The solver tried all ${s.states.toLocaleString()} reachable situations.`;
      return;
    }
    if (s.length < 0) {
      out.textContent = `Too many possibilities to finish (stopped after ${s.states.toLocaleString()} situations).`;
      return;
    }
    const needs = Object.entries(res.needs);
    out.innerHTML = `<b>Solvable in ${s.length} turns</b> <span class="muted">(${s.states.toLocaleString()} situations checked)</span>
      <code>${s.text}</code>` +
      (needs.length ? `<div class="needs">${needs.map(([k, v]) =>
        `<span class="need ${v}" title="${v === "yes" ? "Can't be beaten without it" : v === "no" ? "Optional" : "Search too big to tell"}">${
          v === "yes" ? "Needs " : v === "no" ? "Optional: " : "? "}${NEED_NAMES[k]}</span>`).join("")}</div>` : "");
    lastSolve = { key: keyOf(lv), steps: s.steps };
    $("btn-watch").disabled = false;
  } finally {
    $("btn-solve").disabled = false;
  }
}

// ------------------------------------------------------------- tileset

const TS_SCALE = 3;

function buildTileset() {
  const ts = A.tileset;
  behaviour = { ...ts.behaviour };
  taught = { ...((ts.autopaint || {}).taught || {}) };
  updateTaughtInfo();
  $("taught-forget").addEventListener("click", () => {
    pushUndo();
    taught = {};
    setDirty(true);
    updateTaughtInfo();
  });
  artImgs = ts.tiles.map((variants) => variants.map((px) => makeTile(px, false)));
  const c = $("tileset");
  const rows = Math.ceil(ts.tiles.length / ts.columns);
  c.width = ts.columns * 8 * TS_SCALE;
  c.height = rows * 8 * TS_SCALE;
  c.addEventListener("click", (e) => {
    const t = A.tileset;                       // may have grown since
    const r = c.getBoundingClientRect();
    const x = Math.floor((e.clientX - r.left) / (r.width / t.columns));
    const y = Math.floor((e.clientY - r.top) / (r.height / Math.ceil(t.tiles.length / t.columns)));
    const n = y * t.columns + x;
    if (n < t.tiles.length) setTool("art:" + n);
  });
  const sel = $("art-behaviour");
  sel.add(new Option("nothing (look only)", ""));
  A.tiles.filter((d) => d.char !== "P").forEach((d) => sel.add(new Option(`${d.name} (${d.char})`, d.char)));
  sel.addEventListener("change", () => {
    if (sel.value) behaviour[String(artTile)] = sel.value;
    else delete behaviour[String(artTile)];
    setDirty(true);
    drawTileset();
    setTool("art:" + artTile);
  });
  $("art-only").addEventListener("change", () => tool.startsWith("art:") && setTool(tool));
  $("show-behaviour").addEventListener("change", () => renderEdit());
  $("autopaint").addEventListener("click", async () => {
    const m = cmap();
    const res = await api("/api/autopaint", { levels, taught, map: m.map, art: m.art || null,
                                              keep: scriptedCells(m) });
    if (res.error) { hint(res.error); return; }
    pushUndo();
    m.art = res.art;
    changed();
    hint("Repainted from the map: room outlines, floor, exits and doors. Ctrl+Z to undo.");
  });
  drawTileset();
}

function drawTileset() {
  const ts = A.tileset;
  const c = $("tileset");
  const g = c.getContext("2d");
  const s = 8 * TS_SCALE;
  g.imageSmoothingEnabled = false;
  ts.tiles.forEach((_, n) => {
    const x = (n % ts.columns) * s, y = Math.floor(n / ts.columns) * s;
    g.drawImage(artImgs[n][0], x, y, s, s);
    const b = behaviour[String(n)];
    if (b) {                       // small corner badge with the map letter
      g.fillStyle = "rgba(15, 20, 17, 0.75)";
      g.fillRect(x + s - 10, y + s - 10, 10, 10);
      g.fillStyle = "#f2b705";
      g.font = "bold 9px ui-monospace, monospace";
      g.textAlign = "center";
      g.textBaseline = "middle";
      g.fillText(b, x + s - 5, y + s - 4.5);
    }
  });
  if (tool.startsWith("art:")) {
    g.strokeStyle = "#e4572e";
    g.lineWidth = 3;
    g.strokeRect((artTile % ts.columns) * s + 1.5, Math.floor(artTile / ts.columns) * s + 1.5, s - 3, s - 3);
  }
  $("art-tile-info").textContent = `Tile ${artTile}`;
  $("art-behaviour").value = behaviour[String(artTile)] || "";
}

// ---------------------------------------------------------- tile painter

const TP = { index: 0, px: [], shade: 3, tool: "pencil", undo: [], redo: [], clip: null,
             drawing: false, stroke: null };

function tpOpen(n) {
  TP.index = n;
  TP.px = [...A.tileset.tiles[n][0]];
  TP.undo = [];
  TP.redo = [];
  $("tile-title").textContent = `Edit tile ${n}`;
  $("tile-status").textContent = "";
  tpRender();
  $("tile-dialog").showModal();
}

function tpPush() {
  TP.undo.push([...TP.px]);
  if (TP.undo.length > 200) TP.undo.shift();
  TP.redo = [];
}

function tpRender() {
  const c = $("tile-canvas"), g = c.getContext("2d");
  const s = c.width / 8;
  TP.px.forEach((v, i) => {
    g.fillStyle = PALETTE[v];
    g.fillRect((i % 8) * s, Math.floor(i / 8) * s, s, s);
  });
  g.strokeStyle = "rgba(0, 0, 0, 0.18)";
  g.lineWidth = 1;
  for (let k = 1; k < 8; k++) {
    g.beginPath(); g.moveTo(k * s + 0.5, 0); g.lineTo(k * s + 0.5, c.height); g.stroke();
    g.beginPath(); g.moveTo(0, k * s + 0.5); g.lineTo(c.width, k * s + 0.5); g.stroke();
  }
  const tile = makeTile(TP.px, false);
  const seen = makeTile(TP.px.map((v) => (v === 0 ? 1 : v)), false);   // cone: lightest -> light grey
  const draw = (id, img, n) => {
    const cv = $(id), gg = cv.getContext("2d");
    gg.imageSmoothingEnabled = false;
    const t = cv.width / n;
    for (let y = 0; y < n; y++) for (let x = 0; x < n; x++) gg.drawImage(img, x * t, y * t, t, t);
  };
  draw("tile-prev1", tile, 1);
  draw("tile-prev3", tile, 3);
  draw("tile-prevseen", seen, 3);
  document.querySelectorAll("#shade-row button").forEach((b) =>
    b.classList.toggle("active", Number(b.dataset.shade) === TP.shade));
  document.querySelectorAll("[data-ptool]").forEach((b) =>
    b.classList.toggle("active", b.dataset.ptool === TP.tool));
}

function tpPixelAt(e) {
  const r = $("tile-canvas").getBoundingClientRect();
  const x = Math.floor((e.clientX - r.left) / (r.width / 8));
  const y = Math.floor((e.clientY - r.top) / (r.height / 8));
  return x >= 0 && y >= 0 && x < 8 && y < 8 ? y * 8 + x : -1;
}

function tpFill(i, shade) {
  const from = TP.px[i];
  if (from === shade) return;
  const todo = [i];
  while (todo.length) {
    const k = todo.pop();
    if (TP.px[k] !== from) continue;
    TP.px[k] = shade;
    const x = k % 8, y = Math.floor(k / 8);
    if (x > 0) todo.push(k - 1);
    if (x < 7) todo.push(k + 1);
    if (y > 0) todo.push(k - 8);
    if (y < 7) todo.push(k + 8);
  }
}

function tpOp(op) {
  const p = TP.px, at = (x, y) => p[y * 8 + x];
  const remap = (f) => { tpPush(); TP.px = [...Array(64)].map((_, i) => f(i % 8, Math.floor(i / 8))); };
  if (op === "fliph") remap((x, y) => at(7 - x, y));
  else if (op === "flipv") remap((x, y) => at(x, 7 - y));
  else if (op === "rot") remap((x, y) => at(y, 7 - x));
  else if (op === "left") remap((x, y) => at((x + 1) % 8, y));
  else if (op === "right") remap((x, y) => at((x + 7) % 8, y));
  else if (op === "up") remap((x, y) => at(x, (y + 1) % 8));
  else if (op === "down") remap((x, y) => at(x, (y + 7) % 8));
  else if (op === "clear") remap(() => 0);
  else if (op === "copy") { TP.clip = [...p]; $("tile-status").textContent = "Copied."; }
  else if (op === "paste" && TP.clip) { tpPush(); TP.px = [...TP.clip]; }
  else if (op === "undo" && TP.undo.length) { TP.redo.push([...p]); TP.px = TP.undo.pop(); }
  else if (op === "redo" && TP.redo.length) { TP.undo.push([...p]); TP.px = TP.redo.pop(); }
  tpRender();
}

/* Apply a new tileset from the server: tile images, panel, map views. */
function tilesetUpdated(tset) {
  A.tileset = tset;
  artImgs = tset.tiles.map((variants) => variants.map((px) => makeTile(px, false)));
  const c = $("tileset");
  c.height = Math.ceil(tset.tiles.length / tset.columns) * 8 * TS_SCALE;
  drawTileset();
  preview = null;
  if (mode === "edit") renderEdit();
  checkLevel();
}

async function tpSave() {
  $("tile-status").textContent = "Saving…";
  const res = await api("/api/tileset/tile", { index: TP.index, pixels: TP.px });
  if (res.error) { $("tile-status").textContent = res.error; return; }
  tilesetUpdated(res.tileset);
  $("tile-dialog").close();
  hint(`Saved tile ${TP.index} to the tileset image` +
       (res.regenerated ? " and updated src/levels.c." : ". Save the levels to update src/levels.c."));
}

function wireTilePainter() {
  const row = $("shade-row");
  [0, 1, 2, 3].forEach((s) => {
    const b = document.createElement("button");
    b.dataset.shade = s;
    b.style.background = PALETTE[s];
    b.style.color = s < 2 ? PALETTE[3] : PALETTE[0];
    b.textContent = String(s + 1);
    b.title = ["Lightest (shows the floor)", "Light", "Dark", "Darkest"][s] + ` (key ${s + 1})`;
    b.addEventListener("click", () => { TP.shade = s; tpRender(); });
    row.appendChild(b);
  });
  document.querySelectorAll("[data-ptool]").forEach((b) =>
    b.addEventListener("click", () => { TP.tool = b.dataset.ptool; tpRender(); }));
  document.querySelectorAll("#tile-dialog [data-op]").forEach((b) =>
    b.addEventListener("click", () => tpOp(b.dataset.op)));

  const c = $("tile-canvas");
  c.addEventListener("contextmenu", (e) => e.preventDefault());
  c.addEventListener("pointerdown", (e) => {
    const i = tpPixelAt(e);
    if (i < 0) return;
    const shade = e.button === 2 ? 0 : TP.shade;
    tpPush();
    if (TP.tool === "fill") { tpFill(i, shade); tpRender(); return; }
    TP.drawing = true;
    TP.stroke = shade;
    c.setPointerCapture(e.pointerId);
    TP.px[i] = shade;
    tpRender();
  });
  c.addEventListener("pointermove", (e) => {
    if (!TP.drawing) return;
    const i = tpPixelAt(e);
    if (i >= 0 && TP.px[i] !== TP.stroke) { TP.px[i] = TP.stroke; tpRender(); }
  });
  c.addEventListener("pointerup", () => { TP.drawing = false; });

  $("tile-dialog").addEventListener("keydown", (e) => {
    if (e.key >= "1" && e.key <= "4") { TP.shade = Number(e.key) - 1; tpRender(); }
    else if (e.key === "p" || e.key === "P") { TP.tool = "pencil"; tpRender(); }
    else if (e.key === "f" || e.key === "F") { TP.tool = "fill"; tpRender(); }
    else if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "z") tpOp(e.shiftKey ? "redo" : "undo");
    else if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "c") tpOp("copy");
    else if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "v") tpOp("paste");
    else return;
    e.preventDefault();
    e.stopPropagation();
  });
  $("tile-save").addEventListener("click", tpSave);
  $("tile-cancel").addEventListener("click", () => $("tile-dialog").close());
  $("tile-edit").addEventListener("click", () => tpOpen(artTile));
  $("tileset").addEventListener("dblclick", () => tpOpen(artTile));
  $("tile-add-row").addEventListener("click", async () => {
    const res = await api("/api/tileset/add_row", {});
    if (res.error) { hint(res.error); return; }
    tilesetUpdated(res.tileset);
    hint(`Added 16 blank tiles (${res.tileset.tiles.length - 16}–${res.tileset.tiles.length - 1}).`);
  });
}

// =================================================================== PLAY

let P = null;   // playtest session

function newPlayUI() { return { mode: "play", item: 0, ax: 0, ay: 0, msg: null, looking: false }; }

// ----- camera: same rules as screen_of()/camera_target() in main.c.
// One whole screen at a time; crossing an edge cuts to the next screen.

const screenOf = (t, view) => Math.floor(t / view) * view;

function cameraTarget() {
  if (P.ui.looking) return [P.cam.x, P.cam.y];
  if (P.ui.mode === "aim") return [screenOf(P.ui.ax, A.view_w), screenOf(P.ui.ay, A.view_h)];
  const [px, py] = P.view.player;
  return [screenOf(px, A.view_w), screenOf(py, A.view_h)];
}

function snapCamera(x, y) {
  P.cam = { x, y };
  P.scroll = { x: x * 8, y: y * 8 };
}

function centerCamera() {
  const [px, py] = P.view.player;
  snapCamera(screenOf(px, A.view_w), screenOf(py, A.view_h));
}

async function moveCamera(nx, ny) { snapCamera(nx, ny); }

async function followCamera() {
  const [nx, ny] = cameraTarget();
  await moveCamera(nx, ny);
}

async function enterPlay() {
  const lv = clone(level());
  const res = await api("/api/start", { level: lv });
  if (res.error) {
    hint(res.error);
    $("play-status").textContent = "";
    return false;
  }
  mode = "play";
  P = { lv, idx: cur, state: res.state, view: res.view, undo: [], ui: newPlayUI(),
        busy: false, replay: null, keys: new Set(), struggle: null,
        cam: { x: 0, y: 0 }, scroll: { x: 0, y: 0 }, selectArmed: false };
  centerCamera();
  $("tab-play").classList.add("active");
  $("tab-edit").classList.remove("active");
  $("edit-tools").hidden = true;
  $("play-tools").hidden = false;
  canvas.style.cursor = "default";
  canvas.focus();
  $("play-status").textContent = "Playing with the real game rules (src/game.c).";
  renderPlay();
  return true;
}

function enterEdit() {
  if (P) P.replay = null;
  mode = "edit";
  P = null;
  $("tab-edit").classList.add("active");
  $("tab-play").classList.remove("active");
  $("edit-tools").hidden = false;
  $("play-tools").hidden = true;
  canvas.style.cursor = "crosshair";
  renderEdit();
}

/* Draw what the Game Boy shows: the map scrolled to the camera, sprites,
 * then the HUD (the window layer) over the bottom two rows. */
function renderPlay() {
  const v = P.view;
  const ui = P.ui;
  const sx = P.scroll.x, sy = P.scroll.y;
  sizeCanvas(160 * PLAY_SCALE, 144 * PLAY_SCALE);

  const tx0 = Math.floor(sx / 8), ty0 = Math.floor(sy / 8);
  for (let ty = ty0; ty <= ty0 + A.view_h; ty++) {
    for (let tx = tx0; tx <= tx0 + A.view_w; tx++) {
      const row = v.tiles[ty];
      const t = row && row[tx] !== undefined ? row[tx] : A.T.TILE_WALL;
      drawBkgPx(t, tx * 8 - sx, ty * 8 - sy);
    }
  }

  const place = (t, x, y) => {
    const px = x * 8 - sx, py = y * 8 - sy;
    if (px <= -8 || px >= A.view_w * 8 || py <= -8 || py + 8 > A.view_h * 8) return;  // off screen / under HUD
    drawSprPx(t, px, py);
  };
  $("play-flags").textContent = v.flags && v.flags.length ? "Flags set: " + v.flags.join(", ") : "";
  if (ui.mode !== "inventory" && ui.mode !== "dialogue") {   // sprites are hidden while the window is up
    // lower sprite index = on top (player is sprite 0), so draw in reverse
    [...v.sprites].reverse().forEach((s) => {
      let t = s.t;
      if (P.struggle && s.guard === v.takedown_guard)
        t = P.struggle.on ? s.base + 4 : s.base + v.guards[s.guard].facing;
      place(t, s.x, s.y);
    });
    if (ui.mode === "aim") {
      const ok = v.aim && v.aim[ui.ay][ui.ax];
      place(ok ? A.T.SPR_CURSOR : A.T.SPR_CURSOR_BAD, ui.ax, ui.ay);
    }
  }

  if (ui.mode === "inventory") { drawInventory(); return; }
  if (ui.mode === "dialogue") { drawDialogue(); return; }
  const [r1, r2] = hudRows(P.lv, P.idx, v, ui);
  r1.forEach((t, x) => drawBkgPx(t, x * 8, A.view_h * 8));
  r2.forEach((t, x) => drawBkgPx(t, x * 8, A.view_h * 8 + 8));
}

function drawInventory() {
  const T = A.T;
  const rows = 3 + A.items.length;
  const top = 18 - rows;
  const row = (r, y) => r.forEach((t, x) => drawBkgPx(t, x * 8, y * 8));
  row(new Array(20).fill(T.TILE_WIN_EDGE), top);
  row(textRow([[1, "THROW WHAT?"]]), top + 1);
  let r = top + 2;
  A.items.forEach((it, i) => {
    const n = P.view.items[i];
    if (!n) return;
    const parts = [[3, it.name], [15, "X" + n]];
    if (i === P.ui.item) parts.push([1, ">"]);
    row(textRow(parts), r++);
  });
  while (r < top + rows - 1) row(textRow([]), r++);
  row(textRow([[1, "A:AIM  B:BACK"]]), top + rows - 1);
}

/* The message box (draw_dialogue in main.c): what an NPC said this turn. */
function drawDialogue() {
  const T = A.T;
  const top = 18 - 5;
  const row = (r, y) => r.forEach((t, x) => drawBkgPx(t, x * 8, y * 8));
  const lines = P.said[P.ui.sayAt] || [];
  row(new Array(20).fill(T.TILE_WIN_EDGE), top);
  for (let i = 0; i < A.text_rows; i++) row(textRow([[1, lines[i] || ""]]), top + 1 + i);
  row(textRow([[1, P.ui.sayAt + 1 < P.said.length ? "A:MORE" : "A:OK"]]), top + 4);
}

async function act(action, opts = {}) {
  const prev = P.state;
  const res = await api("/api/act", { level: P.lv, state: P.state, action,
                                       aim_item: opts.aim ?? null });
  if (res.error) { $("play-status").textContent = res.error; return false; }
  if (!res.ok) return false;
  if (opts.undo !== false) {
    P.undo.push(prev);
    if (P.undo.length > A.G.UNDO_DEPTH) P.undo.shift();   // same limit as the ROM
  }
  P.state = res.state;
  P.view = res.view;
  P.said = res.said || [];
  await followCamera();
  return true;
}

async function refreshView(aimItem) {
  const res = await api("/api/view", { level: P.lv, state: P.state, aim_item: aimItem ?? null });
  P.view = res.view;
}

async function restartPlay() {
  const res = await api("/api/start", { level: P.lv });
  P.state = res.state;
  P.view = res.view;
  P.undo = [];
  P.ui = newPlayUI();
  P.selectArmed = false;
  centerCamera();
  renderPlay();
}

// ----- level transitions: the dip to black between levels (main.c's
// transition()). The patterns come from src/trans.c through the server.

const FRAME_MS = 1000 / 60;
const TRANS_BATCH = 12;          // tiles changed per frame, as in main.c
const TRANSITION_NAMES = {
  cut: "Cut (no animation)", fade: "Fade", wipe: "Wipe", curtain: "Curtain",
  iris: "Iris (circle)", dissolve: "Dissolve (dither)", diagonal: "Diagonal wipe",
  blinds: "Blinds", interlace: "Interlace",
};
let transitionRunning = false;

function grabCanvas() {
  const c = document.createElement("canvas");
  c.width = canvas.width;
  c.height = canvas.height;
  c.getContext("2d").drawImage(canvas, 0, 0);
  return c;
}

function blackout() {
  ctx.fillStyle = PALETTE[3];
  ctx.fillRect(0, 0, canvas.width, canvas.height);
}

/* The picture with every Game Boy shade k steps darker (the palette fade). */
function darker(shot, k) {
  const c = grabCanvas();
  const g = c.getContext("2d");
  g.drawImage(shot, 0, 0);
  const img = g.getImageData(0, 0, c.width, c.height);
  const rgb = PALETTE.map((hex) => [1, 3, 5].map((i) => parseInt(hex.slice(i, i + 2), 16)));
  const d = img.data;
  for (let i = 0; i < d.length; i += 4) {
    const shade = d[i] > 0xb4 ? 0 : d[i] > 0x5e ? 1 : d[i] > 0x1e ? 2 : 3;   // by red
    const [r, gr, b] = rgb[Math.min(3, shade + k)];
    d[i] = r; d[i + 1] = gr; d[i + 2] = b;
  }
  g.putImageData(img, 0, 0);
  return c;
}

/* Play one half of a transition on the canvas. into = false: from `shot`
 * (what the canvas shows) to black; true: from black back to `shot`.
 * (cx, cy) is the player's screen tile, for the iris. Timing as the ROM. */
async function playTransition(kind, into, shot, cx, cy) {
  if (kind === "cut") {
    if (into) ctx.drawImage(shot, 0, 0); else blackout();
    return;
  }
  if (kind === "fade") {
    for (let s = 1; s <= 3; s++) {
      ctx.drawImage(darker(shot, into ? 3 - s : s), 0, 0);
      await sleep(5 * FRAME_MS);     // main.c: a shade every 5 frames
    }
    return;
  }
  const pat = await api("/api/transition", { kind, cx, cy });
  const grid = into ? pat.in : pat.out;
  // square cells across the picture's width: exactly the Game Boy's tiles
  // in a playtest; on the edit view (a whole map) the pattern repeats down
  const cell = canvas.width / grid[0].length;
  const rows = Math.ceil(canvas.height / cell);
  ctx.fillStyle = PALETTE[3];
  // like main.c: tiles change in step order (top to bottom within a step),
  // TRANS_BATCH of the screen's tiles per frame
  const order = [];
  for (let step = 0; step < pat.steps; step++)
    for (let y = 0; y < rows; y++)
      grid[y % grid.length].forEach((at, x) => { if (at === step) order.push([x, y]); });
  const perFrame = Math.ceil(TRANS_BATCH * rows / grid.length);
  for (let i = 0; i < order.length; i += perFrame) {
    for (const [x, y] of order.slice(i, i + perFrame)) {
      if (into) ctx.drawImage(shot, x * cell, y * cell, cell, cell, x * cell, y * cell, cell, cell);
      else ctx.fillRect(x * cell, y * cell, cell, cell);
    }
    await sleep(FRAME_MS);
  }
}

/* The iris closes on the player; anywhere else, on the middle. */
function transitionCentre() {
  if (mode === "play" && P) {
    const x = P.view.player[0] - P.cam.x, y = P.view.player[1] - P.cam.y;
    if (x >= 0 && x < A.view_w && y >= 0 && y < A.view_h) return [x, y];
  }
  return [A.view_w / 2, A.view_h / 2];
}

/* Playtest: the level is finished. Dip to black and start it again (the
 * ROM goes on to the next level the same way). */
async function finishPlay() {
  const kind = P.lv.transition || "fade";
  transitionRunning = true;
  try {
    await playTransition(kind, false, grabCanvas(), ...transitionCentre());
    blackout();
    await sleep(300);
    await restartPlay();
    const shot = grabCanvas();
    blackout();
    await playTransition(kind, true, shot, ...transitionCentre());
  } finally {
    transitionRunning = false;
  }
}

/* The Preview button: play the level's transition on whatever is shown. */
async function previewTransition() {
  if (transitionRunning || (P && P.busy)) return;
  transitionRunning = true;
  const kind = level().transition || "fade";
  const shot = grabCanvas();
  const centre = transitionCentre();
  try {
    await playTransition(kind, false, shot, ...centre);
    blackout();
    await sleep(300);
    await playTransition(kind, true, shot, ...centre);
  } finally {
    transitionRunning = false;
    if (mode === "play" && P) renderPlay(); else renderEdit();
  }
}

async function undoPlay() {
  if (P.ui.mode !== "play" || !P.undo.length) return;
  P.state = P.undo.pop();
  await refreshView();
  await followCamera();
  renderPlay();
}

const hasItems = () => P.view.items.some((n) => n > 0);

function nextItem(from, step) {
  const n = A.items.length;
  let i = from;
  for (let k = 0; k < n; k++) {
    i = (i + step + n) % n;
    if (P.view.items[i]) return i;
  }
  return from;
}

async function doTakedown() {
  P.ui.msg = "TAKEDOWN...";
  if (!(await act({ type: "step", code: A.G.ACT_TAKEDOWN }))) { P.ui.msg = null; return; }
  renderPlay();
  while (P.view.takedown_left && P.view.status === A.G.STATUS_PLAYING) {
    // struggle for 24 frames (0.4 s), flipping every 4 frames, like main.c
    P.struggle = { on: false };
    for (let f = 0; f < 24; f++) {
      P.struggle.on = (f & 4) !== 0;
      renderPlay();
      await sleep(1000 / 60);
    }
    P.struggle = null;
    await act({ type: "step", code: A.G.ACT_WAIT }, { undo: false });
    P.ui.msg = P.view.takedown_left ? "TAKEDOWN..." : null;
    renderPlay();
  }
  P.ui.msg = null;
}

const DIRKEYS = { ArrowUp: 0, ArrowRight: 1, ArrowDown: 2, ArrowLeft: 3 };

async function onPlayKey(btn, dir) {
  const G = A.G;
  const ui = P.ui;
  const v = P.view;
  ui.msg = null;

  if (ui.mode === "dialogue") {            // A or B: next message, or close
    if (btn === "A" || btn === "B") {
      ui.sayAt++;
      if (ui.sayAt >= P.said.length) ui.mode = "play";
    }
    return;
  }

  if (ui.mode === "inventory") {
    if (btn === "B" || btn === "SELECT") ui.mode = "play";
    else if (dir === 0) ui.item = nextItem(ui.item, -1);
    else if (dir === 2) ui.item = nextItem(ui.item, 1);
    else if (btn === "A") {
      ui.mode = "aim";
      [ui.ax, ui.ay] = v.player;
      await refreshView(ui.item);
    }
    return;
  }

  if (ui.mode === "aim") {
    if (btn === "SELECT") { ui.mode = "play"; await refreshView(); await followCamera(); return; }
    if (btn === "B") { ui.mode = "inventory"; await refreshView(); await followCamera(); return; }
    if (btn === "A") {
      if (await act({ type: "throw", item: ui.item, x: ui.ax, y: ui.ay })) {
        ui.mode = "play";
        await followCamera();
      } else ui.msg = "NO CLEAR SHOT";
      return;
    }
    if (dir === undefined) return;
    const nx = ui.ax + [0, 1, 0, -1][dir];
    const ny = ui.ay + [-1, 0, 1, 0][dir];
    if (nx < 0 || ny < 0 || nx >= P.view.w || ny >= P.view.h) return;
    const d = Math.abs(nx - v.player[0]) + Math.abs(ny - v.player[1]);
    if (d > A.items[ui.item].range) return;
    ui.ax = nx; ui.ay = ny;
    await followCamera();                  // cut to the cursor's screen
    return;
  }

  // play mode
  if (v.status === G.STATUS_WON) { if (btn === "A") await finishPlay(); return; }
  if (v.status !== G.STATUS_PLAYING) return;
  if (btn === "B") {
    if (v.prompt.takedown) await doTakedown();
    else if (v.prompt.talk) {
      if (await act({ type: "step", code: G.ACT_TALK }) && P.said.length) {
        ui.mode = "dialogue";
        ui.sayAt = 0;
      }
    }
    else if (v.prompt.uniform) await act({ type: "step", code: G.ACT_UNIFORM });
    return;
  }
  if (btn === "SELECT") { P.selectArmed = true; return; }   // acted on when let go
  if (P.keys.has("SELECT") && dir !== undefined) {           // look at other screens
    let { x, y } = P.cam;
    if (dir === 0 && y > 0) y -= A.view_h;
    else if (dir === 2 && y + A.view_h < P.view.h) y += A.view_h;
    else if (dir === 3 && x > 0) x -= A.view_w;
    else if (dir === 1 && x + A.view_w < P.view.w) x += A.view_w;
    if (!ui.looking) P.lookFrom = { ...P.cam };   // to return to on release
    ui.looking = true;
    await moveCamera(x, y);
    return;
  }
  if (btn === "A") { await act({ type: "step", code: G.ACT_WAIT }); return; }
  if (dir === undefined) return;
  const drag = P.keys.has("B") && v.prompt.body_adjacent;
  await act({ type: "step", code: drag ? G.ACT_DRAG + dir : dir });
}

/* SELECT let go: a tap opens the inventory, a look returns to the player. */
async function selectUp() {
  if (!P.selectArmed) return;
  P.selectArmed = false;
  const ui = P.ui;
  if (ui.looking) {
    ui.looking = false;
    await moveCamera(P.lookFrom.x, P.lookFrom.y);
  } else if (ui.mode === "play" && P.view.status === A.G.STATUS_PLAYING) {
    if (hasItems()) {
      if (!P.view.items[ui.item]) ui.item = nextItem(ui.item, 1);
      ui.mode = "inventory";
    } else ui.msg = "NOTHING TO THROW";
  }
  renderPlay();
}

// START: tap to undo, hold to restart (like handle_start in main.c)
let startTimer = null, startHint = null, startAt = 0;

function startDown() {
  startAt = performance.now();
  startHint = setTimeout(() => { P.ui.msg = "HOLD TO RESTART"; renderPlay(); }, 250);
  startTimer = setTimeout(async () => { startTimer = null; await restartPlay(); }, 750);
}

async function startUp() {
  clearTimeout(startHint);
  if (startTimer === null) return;          // already restarted
  clearTimeout(startTimer);
  startTimer = null;
  if (performance.now() - startAt < 250) await undoPlay();
  else { P.ui.msg = null; renderPlay(); }
}

function buttonFor(e) {
  if (e.key === "z" || e.key === "Z") return "A";
  if (e.key === "x" || e.key === "X") return "B";
  if (e.key === "Shift" || e.key === "Backspace") return "SELECT";
  if (e.key === "Enter") return "START";
  return null;
}

async function watchSolution() {
  if (!lastSolve || lastSolve.key !== keyOf(level())) return;
  if (mode !== "play" && !(await enterPlay())) return;
  await restartPlay();
  const steps = lastSolve.steps;
  const token = {};
  P.replay = token;
  for (let k = 0; k < steps.length; k++) {
    if (!P || P.replay !== token) return;   // cancelled
    const s = steps[k];
    $("play-status").textContent = `Replaying solution: turn ${k + 1} of ${steps.length}. Press any key to take over.`;
    const a = s.action & 0x80
      ? { type: "throw", item: s.action & 0x7f, x: s.tx, y: s.ty }
      : { type: "step", code: s.action };
    await act(a);
    renderPlay();
    await sleep(220);
  }
  if (P && P.replay === token) {
    $("play-status").textContent = "Solution finished.";
    P.replay = null;
  }
}

// ============================================================ keyboard

function typing(e) {
  const t = e.target;
  return t && (t.tagName === "INPUT" || t.tagName === "TEXTAREA" || t.tagName === "SELECT");
}

window.addEventListener("keydown", async (e) => {
  if (document.querySelector("dialog[open]")) return;   // e.g. the tile painter
  if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "s") { e.preventDefault(); save(); return; }
  if (typing(e)) return;

  if (mode === "edit") {
    if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "z") {
      e.preventDefault();
      e.shiftKey ? redoEdit() : undoEdit();
      return;
    }
    if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "y") { e.preventDefault(); redoEdit(); return; }
    const tools = { 4: "start", 5: "guard", 6: "route", 7: "npc", 8: "switch", 0: "art-erase" };
    if (tools[e.key]) { setTool(tools[e.key]); return; }
    const tile = !e.ctrlKey && !e.metaKey &&
      A.tiles.find((x) => x.char !== "P" && tileKey(x.char) === e.key.toUpperCase().replace(/^([123])$/, "$1"));
    if (tile) { setTool("tile:" + tile.char); return; }
    if ((e.key === "Delete" || e.key === "Backspace") && selGuard >= 0) {
      deleteGuard();
      e.preventDefault();
    }
    if (e.key === "p" || e.key === "P") enterPlay();
    return;
  }

  // play mode
  const btn = buttonFor(e);
  const dir = DIRKEYS[e.key];
  if (btn === null && dir === undefined) {
    if (e.key === "Escape") enterEdit();
    return;
  }
  e.preventDefault();
  if (P.replay) { P.replay = null; $("play-status").textContent = "Replay stopped."; }
  if (btn) P.keys.add(btn);
  if (e.repeat || P.busy) return;
  if (btn === "START") { startDown(); return; }
  P.busy = true;
  try {
    await onPlayKey(btn, dir);
    if (P) renderPlay();
  } finally {
    if (P) P.busy = false;
  }
});

window.addEventListener("keyup", async (e) => {
  if (mode !== "play" || !P) return;
  const btn = buttonFor(e);
  if (btn) P.keys.delete(btn);
  if (btn === "START") await startUp();
  if (btn === "SELECT" && !P.busy) await selectUp();
});

// ============================================================== actions

function emptyRoom() {
  const map = [];
  for (let y = 0; y < 16; y++) {
    let r = "";
    for (let x = 0; x < 20; x++) r += (x === 0 || y === 0 || x === 19 || y === 15) ? "#" : ".";
    map.push(r);
  }
  return map;
}

function newLevel() {
  const map = emptyRoom();
  map[1] = "#P" + map[1].slice(2);
  map[14] = map[14].slice(0, 18) + "E#";
  return { name: "NEW LEVEL", notes: "", items: { stone: 0, bottle: 0 }, transition: "fade",
           maps: [{ map, guards: [] }], links: [] };
}

function deleteGuard() {
  pushUndo();
  cmap().guards.splice(selGuard, 1);
  selGuard = -1;
  changed();
}

async function save() {
  const res = await api("/api/save", { levels, behaviour, taught });
  if (res.ok) {
    setDirty(false);
    $("save-state").textContent = "Saved to levels/levels.json and src/levels.c";
  } else {
    $("save-state").textContent = "Not saved: fix the problems first";
    $("save-state").classList.add("dirty");
    const bad = res.errors.map((e) => e.match(/^Level (\d+)/)).find(Boolean);
    if (bad) selectLevel(parseInt(bad[1], 10) - 1);
  }
}

async function build() {
  if (dirty && !confirm("Save first? The ROM is built from the saved levels.")) return;
  if (dirty) { await save(); if (dirty) return; }
  const dlg = $("build-dialog");
  $("build-output").textContent = "Running make…";
  dlg.showModal();
  const res = await api("/api/build", {});
  $("build-output").textContent = (res.ok ? "Build succeeded: build/stealth.gb\n\n" : "Build failed.\n\n") +
    (res.output || res.error || "");
}

function wire() {
  document.querySelectorAll("#toolbar button, .art-panel button[data-tool]").forEach((b) =>
    b.addEventListener("click", () => setTool(b.dataset.tool)));
  $("tab-edit").addEventListener("click", enterEdit);
  $("tab-play").addEventListener("click", () => mode !== "play" && enterPlay());
  $("play-exit").addEventListener("click", enterEdit);
  $("play-restart").addEventListener("click", async () => { await restartPlay(); canvas.focus(); });
  $("play-undo").addEventListener("click", async () => { await undoPlay(); canvas.focus(); });
  $("btn-undo-edit").addEventListener("click", undoEdit);
  $("btn-redo-edit").addEventListener("click", redoEdit);
  $("btn-save").addEventListener("click", save);
  $("btn-build").addEventListener("click", build);
  $("btn-solve").addEventListener("click", solve);
  $("btn-watch").addEventListener("click", watchSolution);
  $("zoom").addEventListener("change", (e) => { zoomSetting = e.target.value; renderEdit(); });
  $("screens-x").addEventListener("change", (e) =>
    resizeMap(parseInt(e.target.value, 10) * A.view_w, mapH(cmap())));
  $("screens-y").addEventListener("change", (e) =>
    resizeMap(mapW(cmap()), parseInt(e.target.value, 10) * A.view_h));
  $("preview-disguise").addEventListener("change", (e) => {
    previewDisguise = parseInt(e.target.value, 10);
    preview = null;
    renderEdit();
    checkLevel();
  });

  $("lv-new").addEventListener("click", () => {
    pushUndo(); levels.splice(cur + 1, 0, newLevel()); cur++; curMap = 0; selGuard = -1; changed();
  });
  $("lv-dup").addEventListener("click", () => {
    pushUndo(); levels.splice(cur + 1, 0, clone(level())); cur++; curMap = 0; selGuard = -1; changed();
  });
  $("lv-del").addEventListener("click", () => {
    if (levels.length <= 1) return;
    if (!confirm(`Delete level ${cur + 1} "${level().name}"?`)) return;
    pushUndo(); levels.splice(cur, 1); cur = Math.min(cur, levels.length - 1); curMap = 0; selGuard = -1; changed();
  });
  $("lv-up").addEventListener("click", () => {
    if (cur === 0) return;
    pushUndo(); [levels[cur - 1], levels[cur]] = [levels[cur], levels[cur - 1]]; cur--; changed();
  });
  $("lv-down").addEventListener("click", () => {
    if (cur >= levels.length - 1) return;
    pushUndo(); [levels[cur + 1], levels[cur]] = [levels[cur], levels[cur + 1]]; cur++; changed();
  });

  // text fields: one undo step per editing burst
  let typingUndo = false;
  const textField = (id, apply) => {
    const el = $(id);
    el.addEventListener("input", () => {
      if (!typingUndo) { pushUndo(); typingUndo = true; }
      apply(el);
      changed();
    });
    el.addEventListener("blur", () => { typingUndo = false; });
  };
  const allowed = new RegExp(`[^${A.font_chars.replace(/[\]\\^-]/g, "\\$&")}]`, "g");
  textField("lv-name", (el) => {
    const pos = el.selectionStart;
    el.value = el.value.toUpperCase().replace(allowed, "").slice(0, A.name_max);
    el.setSelectionRange(pos, pos);
    level().name = el.value;
  });
  textField("lv-notes", (el) => { level().notes = el.value; });

  A.transitions.forEach((t) => $("lv-transition").add(new Option(TRANSITION_NAMES[t] || t, t)));
  $("lv-transition").addEventListener("change", (e) => {
    pushUndo();
    level().transition = e.target.value;
    if (P) P.lv.transition = e.target.value;   // a playtest under way uses it too
    changed();
    previewTransition();
  });
  $("trans-preview").addEventListener("click", previewTransition);

  A.dirs.forEach((d) => $("g-facing").add(new Option(d, d)));
  A.uniforms.forEach((u) => $("g-uniform").add(new Option(u === "none" ? "none" : u.toUpperCase(), u)));
  $("g-facing").addEventListener("change", (e) => { pushUndo(); cmap().guards[selGuard].facing = e.target.value; changed(); });
  $("g-uniform").addEventListener("change", (e) => { pushUndo(); cmap().guards[selGuard].uniform = e.target.value; changed(); });
  $("g-custom").addEventListener("change", (e) => {
    pushUndo();
    cmap().guards[selGuard].attitude = e.target.checked ? "EEE" : null;
    changed();
  });
  $("g-route-clear").addEventListener("click", () => {
    pushUndo(); const g = cmap().guards[selGuard]; g.route = [g.route[0]]; changed();
  });
  $("g-delete").addEventListener("click", deleteGuard);

  // NPCs and switch cells
  A.npc_blocks.forEach((b) => $("n-blocks").add(new Option(
    { move: "walking (player and guards)", none: "nothing (walk over it)", all: "everything, like a wall" }[b] || b, b)));
  A.tiles.filter((t) => t.char !== "P").forEach((t) =>
    $("s-on").add(new Option(t.name[0].toUpperCase() + t.name.slice(1), t.char)));
  const npc = () => (cmap().npcs || [])[selNpc];
  const sw = () => (cmap().switches || [])[selSwitch];
  const fontOnly = new RegExp(`[^${A.font_chars.replace(/[\]\\^-]/g, "\\$&")}]`, "g");
  typedField($("n-verb"), (v) => {
    const el = $("n-verb");
    el.value = v.toUpperCase().replace(fontOnly, "").replace(/ /g, "").slice(0, A.verb_max);
    if (npc()) npc().verb = el.value;
  });
  typedField($("s-flag"), (v) => { if (sw()) sw().flag = v.trim(); });
  $("n-blocks").addEventListener("change", (e) => { pushUndo(); npc().blocks = e.target.value; changed(); });
  $("s-on").addEventListener("change", (e) => { pushUndo(); sw().on = e.target.value; changed(); });
  $("s-art-pick").addEventListener("click", () => { pushUndo(); sw().on_art = artTile; changed(); });
  $("s-art-none").addEventListener("click", () => { pushUndo(); sw().on_art = null; changed(); });
  $("n-delete").addEventListener("click", () => deleteScripted("npcs"));
  $("s-delete").addEventListener("click", () => deleteScripted("switches"));
  $("map-delete").addEventListener("click", deleteMap);
  $("g-route-close").addEventListener("click", () => {
    const g = cmap().guards[selGuard];
    const [fx, fy] = g.route[0];
    const [lx, ly] = g.route[g.route.length - 1];
    if (g.route.length < 2 || lx === fx || ly === fy) { hint("The loop already closes in a straight line."); return; }
    if (g.route.length >= A.max_waypoints) { hint(`No room: a route can have at most ${A.max_waypoints} points.`); return; }
    pushUndo();
    g.route.push([fx, ly]);   // walk horizontally, then up/down to the start
    changed();
  });

  window.addEventListener("beforeunload", (e) => { if (dirty) { e.preventDefault(); e.returnValue = ""; } });
}

async function init() {
  A = await api("/api/assets");
  bkgTiles = A.bkg.map((px) => makeTile(px, false));
  sprTiles = A.spr.map((px) => makeTile(px, true));
  levels = (await api("/api/levels")).levels;
  levels.forEach((lv) => {
    lv.items = lv.items || {};
    lv.links = lv.links || [];
    lv.maps.forEach((m) => { m.guards = m.guards || []; });
  });
  wire();
  buildPalette();
  buildTileset();
  wireTilePainter();
  setTool("tile:#");
  setDirty(false);
  renderAll();
  checkLevel();
  // mark any invalid levels in the list
  levels.forEach(async (lv, i) => {
    const r = await api("/api/validate", { level: lv });
    validity[i] = r.errors.length === 0;
    renderLevelList();
  });
}

init();
