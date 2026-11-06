/*
 * GB47 level editor (browser side).
 *
 * The page never decides game rules itself: playtest turns, vision cones
 * and solving all go to the server, which runs src/game.c. This file only
 * mirrors src/main.c's screen layout and button handling.
 */
"use strict";

const SCALE = 3;                 // 160x144 Game Boy screen -> 480x432
const TILE = 8 * SCALE;
const PALETTE = ["#e0f8d0", "#88c070", "#346856", "#081820"];
const GUARD_COLORS = ["#e4572e", "#2e86ab", "#c23bd4", "#f2b705"];
const TOOL_HINTS = {
  wall: "Click or drag to paint walls.",
  floor: "Click or drag to paint floor.",
  exit: "Click or drag to place exits. Reaching any exit wins.",
  start: "Click to place the player's start.",
  guard: "Click an empty tile to add a guard (max 4), or click a guard to select it. Drag to move it.",
  route: "Click to add waypoints to the selected guard (corners are added for you). Drag a point to move it; right-click to remove it.",
};

let A = null;                    // assets from /api/assets
let levels = [];
let cur = 0;
let dirty = false;
let mode = "edit";
let tool = "wall";
let selGuard = -1;
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

function drawBkg(t, tx, ty) { ctx.drawImage(bkgTiles[t], tx * TILE, ty * TILE, TILE, TILE); }
function drawSpr(t, tx, ty) { ctx.drawImage(sprTiles[t], tx * TILE, ty * TILE, TILE, TILE); }

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
  else if (v.prompt.uniform) msg = "B:TAKE UNIFORM";
  else if (v.prompt.body_adjacent) msg = "HOLD B:DRAG";
  else if (v.items.some((c) => c > 0)) msg = "SEL:ITEMS START:UNDO";
  else msg = "A:WAIT START:UNDO";
  return [row1, textRow([[0, msg]])];
}

// =================================================================== EDIT

function localTiles(lv) {
  const T = A.T;
  return lv.map.map((r) => [...r].map((c) =>
    c === "#" ? T.TILE_WALL : c === "E" ? T.TILE_EXIT : T.TILE_FLOOR));
}

function guardBase(g) {
  return g.uniform === "officer" ? A.T.SPR_OFFICER_UP : A.T.SPR_GUARD_UP;
}

function renderEdit() {
  const lv = level();
  const fresh = preview && preview.key === keyOf(lv) && previewDisguise >= 0;
  const tiles = fresh ? preview.view.tiles : localTiles(lv);
  tiles.forEach((row, y) => row.forEach((t, x) => drawBkg(t, x, y)));

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

  const v = fresh ? preview.view : { turn: 0, status: 0, items: [0, 0],
    prompt: { takedown: false, uniform: false, body_adjacent: false } };
  const [r1, r2] = hudRows(lv, cur, v, { mode: "play" });
  drawRow(r1, 16);
  drawRow(r2, 17);
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

function snapshot() { return JSON.stringify({ levels, cur }); }

function pushUndo() {
  undoStack.push(snapshot());
  if (undoStack.length > 300) undoStack.shift();
  redoStack = [];
}

function restore(s) {
  const o = JSON.parse(s);
  levels = o.levels;
  cur = Math.min(o.cur, levels.length - 1);
  if (selGuard >= level().guards.length) selGuard = -1;
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
  const x = Math.floor((e.clientX - r.left) / (r.width / 20));
  const y = Math.floor((e.clientY - r.top) / (r.height / 18));
  return [x, y];
}

function setCell(x, y, c) {
  const lv = level();
  if (lv.map[y][x] === c) return false;
  if (c === "P") lv.map = lv.map.map((r) => r.replace("P", "."));
  const row = lv.map[y];
  lv.map[y] = row.slice(0, x) + c + row.slice(x + 1);
  return true;
}

function guardAt(x, y) {
  return level().guards.findIndex((g) => g.route[0][0] === x && g.route[0][1] === y);
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
  const lv = level();
  const paint = { wall: "#", floor: ".", exit: "E" }[tool];
  if (paint) return setCell(x, y, paint);
  if (tool === "start") return first ? setCell(x, y, "P") : false;

  if (tool === "guard") {
    if (first) {
      const i = guardAt(x, y);
      if (i >= 0) { selGuard = i; stroke.drag = { guard: i, point: 0 }; renderGuards(); return false; }
      if (lv.map[y][x] === "#") { hint("Guards can't stand in walls."); return false; }
      if (lv.guards.length >= A.max_guards) { hint(`At most ${A.max_guards} guards per level.`); return false; }
      lv.guards.push({ facing: "down", route: [[x, y]], uniform: "none", attitude: null });
      selGuard = lv.guards.length - 1;
      stroke.drag = { guard: selGuard, point: 0 };
      return true;
    }
    return moveDragged(x, y);
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
  const g = level().guards[stroke.drag.guard];
  const p = g.route[stroke.drag.point];
  if (p[0] === x && p[1] === y) return false;
  g.route[stroke.drag.point] = [x, y];
  return true;
}

canvas.addEventListener("contextmenu", (e) => { if (mode === "edit") e.preventDefault(); });

canvas.addEventListener("mousedown", (e) => {
  if (mode !== "edit") return;
  const [x, y] = tileAt(e);
  if (x < 0 || x >= 20 || y < 0 || y >= 16) return;
  stroke = { before: snapshot(), changed: false, drag: null };
  if (applyTool(x, y, e, true)) { stroke.changed = true; renderAll(); }
  e.preventDefault();
});

canvas.addEventListener("mousemove", (e) => {
  if (!stroke) return;
  const [x, y] = tileAt(e);
  if (x < 0 || x >= 20 || y < 0 || y >= 16) return;
  if (applyTool(x, y, e, false)) { stroke.changed = true; renderAll(); }
});

window.addEventListener("mouseup", () => {
  if (!stroke) return;
  if (stroke.changed) {
    undoStack.push(stroke.before);
    redoStack = [];
    changed();
  }
  stroke = null;
});

function hint(msg) { $("tool-hint").textContent = msg; }

function setTool(t) {
  tool = t;
  document.querySelectorAll("#toolbar button").forEach((b) =>
    b.classList.toggle("active", b.dataset.tool === t));
  hint(TOOL_HINTS[t]);
}

// ------------------------------------------------------------- panels

function renderAll() {
  renderLevelList();
  renderProps();
  renderGuards();
  if (mode === "edit") renderEdit();
}

function renderLevelList() {
  const ol = $("level-list");
  ol.innerHTML = "";
  levels.forEach((lv, i) => {
    const li = document.createElement("li");
    li.textContent = lv.name || "(unnamed)";
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
  A.items.forEach((it) => {
    const key = it.name.toLowerCase();
    $("item-" + key).value = (lv.items || {})[key] || 0;
  });
}

function renderGuards() {
  const lv = level();
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

function selectLevel(i) {
  cur = i;
  selGuard = -1;
  preview = null;
  if (mode === "play") enterEdit();
  changed(false);
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
      const p = await api("/api/start", { level: lv, disguise: previewDisguise });
      if (!p.error && keyOf(level()) === k) preview = { key: k, view: p.view };
    }
  }
  renderLevelList();
  if (mode === "edit") renderEdit();
}, 150);

// ------------------------------------------------------------- solving

const NEED_NAMES = { items: "items", takedown: "a takedown", drag: "moving a body", uniform: "a disguise" };

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

// =================================================================== PLAY

let P = null;   // playtest session

function newPlayUI() { return { mode: "play", item: 0, ax: 0, ay: 0, msg: null }; }

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
        busy: false, replay: null, keys: new Set(), struggle: null };
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

function renderPlay() {
  const v = P.view;
  const ui = P.ui;
  v.tiles.forEach((row, y) => row.forEach((t, x) => drawBkg(t, x, y)));
  const [r1, r2] = hudRows(P.lv, P.idx, v, ui);
  drawRow(r1, 16);
  drawRow(r2, 17);

  if (ui.mode === "inventory") { drawInventory(); return; }   // sprites hidden

  // lower sprite index = on top (player is sprite 0), so draw in reverse
  [...v.sprites].reverse().forEach((s) => {
    let t = s.t;
    if (P.struggle && s.guard === v.takedown_guard && P.struggle.on)
      t = s.base + 4;
    else if (P.struggle && s.guard === v.takedown_guard)
      t = s.base + v.guards[s.guard].facing;
    drawSpr(t, s.x, s.y);
  });
  if (ui.mode === "aim") {
    const ok = v.aim && v.aim[ui.ay][ui.ax];
    drawSpr(ok ? A.T.SPR_CURSOR : A.T.SPR_CURSOR_BAD, ui.ax, ui.ay);
  }
}

function drawInventory() {
  const T = A.T;
  const rows = 3 + A.items.length;
  const top = 18 - rows;
  drawRow(new Array(20).fill(T.TILE_WIN_EDGE), top);
  drawRow(textRow([[1, "THROW WHAT?"]]), top + 1);
  let r = top + 2;
  A.items.forEach((it, i) => {
    const n = P.view.items[i];
    if (!n) return;
    const parts = [[3, it.name], [15, "X" + n]];
    if (i === P.ui.item) parts.push([1, ">"]);
    drawRow(textRow(parts), r++);
  });
  while (r < top + rows - 1) drawRow(textRow([]), r++);
  drawRow(textRow([[1, "A:AIM  B:BACK"]]), top + rows - 1);
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
  renderPlay();
}

async function undoPlay() {
  if (P.ui.mode !== "play" || !P.undo.length) return;
  P.state = P.undo.pop();
  await refreshView();
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
    if (btn === "SELECT") { ui.mode = "play"; await refreshView(); return; }
    if (btn === "B") { ui.mode = "inventory"; await refreshView(); return; }
    if (btn === "A") {
      if (await act({ type: "throw", item: ui.item, x: ui.ax, y: ui.ay })) ui.mode = "play";
      else ui.msg = "NO CLEAR SHOT";
      return;
    }
    if (dir === undefined) return;
    const nx = ui.ax + [0, 1, 0, -1][dir];
    const ny = ui.ay + [-1, 0, 1, 0][dir];
    if (nx < 0 || ny < 0 || nx >= 20 || ny >= 16) return;
    const d = Math.abs(nx - v.player[0]) + Math.abs(ny - v.player[1]);
    if (d > A.items[ui.item].range) return;
    ui.ax = nx; ui.ay = ny;
    return;
  }

  // play mode
  if (v.status === G.STATUS_WON) { if (btn === "A") await restartPlay(); return; }
  if (v.status !== G.STATUS_PLAYING) return;
  if (btn === "B") {
    if (v.prompt.takedown) await doTakedown();
    else if (v.prompt.uniform) await act({ type: "step", code: G.ACT_UNIFORM });
    return;
  }
  if (btn === "SELECT") {
    if (hasItems()) { if (!v.items[ui.item]) ui.item = nextItem(ui.item, 1); ui.mode = "inventory"; }
    else ui.msg = "NOTHING TO THROW";
    return;
  }
  if (btn === "A") { await act({ type: "step", code: G.ACT_WAIT }); return; }
  if (dir === undefined) return;
  const drag = P.keys.has("B") && v.prompt.body_adjacent;
  await act({ type: "step", code: drag ? G.ACT_DRAG + dir : dir });
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
  if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "s") { e.preventDefault(); save(); return; }
  if (typing(e)) return;

  if (mode === "edit") {
    if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "z") {
      e.preventDefault();
      e.shiftKey ? redoEdit() : undoEdit();
      return;
    }
    if ((e.ctrlKey || e.metaKey) && e.key.toLowerCase() === "y") { e.preventDefault(); redoEdit(); return; }
    const tools = { 1: "wall", 2: "floor", 3: "exit", 4: "start", 5: "guard", 6: "route" };
    if (tools[e.key]) { setTool(tools[e.key]); return; }
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
});

// ============================================================== actions

function newLevel() {
  const map = [];
  for (let y = 0; y < 16; y++) {
    let r = "";
    for (let x = 0; x < 20; x++) r += (x === 0 || y === 0 || x === 19 || y === 15) ? "#" : ".";
    map.push(r);
  }
  map[1] = "#P" + map[1].slice(2);
  map[14] = map[14].slice(0, 18) + "E#";
  return { name: "NEW LEVEL", notes: "", map, items: { stone: 0, bottle: 0 }, guards: [] };
}

function deleteGuard() {
  pushUndo();
  level().guards.splice(selGuard, 1);
  selGuard = -1;
  changed();
}

async function save() {
  const res = await api("/api/save", { levels });
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
  document.querySelectorAll("#toolbar button").forEach((b) =>
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
  $("preview-disguise").addEventListener("change", (e) => {
    previewDisguise = parseInt(e.target.value, 10);
    preview = null;
    renderEdit();
    checkLevel();
  });

  $("lv-new").addEventListener("click", () => {
    pushUndo(); levels.splice(cur + 1, 0, newLevel()); cur++; selGuard = -1; changed();
  });
  $("lv-dup").addEventListener("click", () => {
    pushUndo(); levels.splice(cur + 1, 0, clone(level())); cur++; changed();
  });
  $("lv-del").addEventListener("click", () => {
    if (levels.length <= 1) return;
    if (!confirm(`Delete level ${cur + 1} "${level().name}"?`)) return;
    pushUndo(); levels.splice(cur, 1); cur = Math.min(cur, levels.length - 1); selGuard = -1; changed();
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

  A.dirs.forEach((d) => $("g-facing").add(new Option(d, d)));
  A.uniforms.forEach((u) => $("g-uniform").add(new Option(u === "none" ? "none" : u.toUpperCase(), u)));
  $("g-facing").addEventListener("change", (e) => { pushUndo(); level().guards[selGuard].facing = e.target.value; changed(); });
  $("g-uniform").addEventListener("change", (e) => { pushUndo(); level().guards[selGuard].uniform = e.target.value; changed(); });
  $("g-custom").addEventListener("change", (e) => {
    pushUndo();
    level().guards[selGuard].attitude = e.target.checked ? "EEE" : null;
    changed();
  });
  $("g-route-clear").addEventListener("click", () => {
    pushUndo(); const g = level().guards[selGuard]; g.route = [g.route[0]]; changed();
  });
  $("g-delete").addEventListener("click", deleteGuard);
  $("g-route-close").addEventListener("click", () => {
    const g = level().guards[selGuard];
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
  levels.forEach((lv) => { lv.items = lv.items || {}; lv.guards = lv.guards || []; });
  wire();
  setTool("wall");
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
