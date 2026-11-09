/*
 * main.c - Game Boy side: input, drawing, and the screen flow.
 * All the rules live in game.c; this file only reads GameState.
 *
 * Controls (playing):
 *   D-pad        move one tile (spends a turn); you can walk over bodies
 *   A            wait one turn            (after escaping: next level)
 *   B            take down an adjacent guard, or take the uniform of
 *                the body you stand on (when prompted)
 *   B + D-pad    hold B to drag a body behind you, or push one ahead
 *   SELECT       tap: open inventory
 *   SELECT+D-pad hold SELECT to look around a big level; let go to return
 *   START        tap: undo    hold: restart level
 *
 * Inventory:  UP/DOWN choose, A aim, B/SELECT close
 * Aiming:     D-pad move cursor, A throw (spends a turn), B back, SELECT cancel
 *
 * Levels bigger than the screen scroll. The camera follows the player
 * (or the aiming cursor), keeping it CAM_MARGIN tiles from the edges, and
 * slides one tile at a time. The background map is 32x32 tiles and wraps,
 * so the visible part of a level of any size is drawn into it at world
 * position modulo 32. The HUD lives in the window layer so it stays still.
 */
#include <gb/gb.h>
#include <stdint.h>

#include "game.h"
#include "levels.h"
#include "tiles.h"

#define HUD_WIN_Y  (VIEW_H * 8)   /* window top when showing the 2-row HUD */
#define CAM_MARGIN_X 5            /* keep the player this far from the sides */
#define CAM_MARGIN_Y 4            /* ... and from the top/bottom */
#define SCROLL_STEP  2            /* pixels per frame when sliding */

/* Hardware sprite slots */
#define SPRITE_PLAYER 0
#define SPRITE_GUARD  1   /* 1..4 */
#define SPRITE_CURSOR 5
#define SPRITE_NOISE  6   /* 6..9, one per guard */

/* Input modes */
#define UI_PLAY      0
#define UI_INVENTORY 1
#define UI_AIM       2

/* Inventory window: edge, title, one row per item type, hint line */
#define WIN_ROWS (3 + ITEM_TYPES)
#define WIN_Y    (144 - WIN_ROWS * 8)

#define RESTART_HOLD   45   /* frames START must be held to restart */
#define TAKEDOWN_PAUSE 24   /* frames shown per automatic takedown turn */

static GameState   state;
static GameState   before;   /* snapshot taken before each turn, for undo */
static uint8_t     level_idx;
static uint8_t     vision[(VIEW_W + 1) * (VIEW_H + 1)];  /* view + 1 for slides */
static uint8_t     row_buf[32];

static uint8_t     cam_x, cam_y;            /* camera (top-left), in tiles */
static uint16_t    scroll_x, scroll_y;      /* camera, in pixels */
static uint8_t     select_armed;            /* SELECT pressed while playing */
static uint8_t     looking;                 /* SELECT held and panned */
static uint8_t     look_x, look_y;          /* camera before looking */

static uint8_t     ui_mode;
static uint8_t     inv_item;     /* highlighted / chosen item type */
static uint8_t     aim_x, aim_y; /* aiming cursor */
static const char *hud_msg;      /* one-off message for the HUD's 2nd row */

/* ------------------------------------------------------------------ */
/* Text                                                                */
/* ------------------------------------------------------------------ */

static uint8_t char_tile(char c)
{
    if (c >= '0' && c <= '9') return FONT_BASE + (c - '0');
    if (c >= 'A' && c <= 'Z') return FONT_BASE + 10 + (c - 'A');
    if (c == ':') return FONT_COLON;
    if (c == '!') return FONT_BANG;
    if (c == '-') return FONT_DASH;
    if (c == '.') return FONT_PERIOD;
    if (c == '>') return FONT_GT;
    if (c == '?') return FONT_QMARK;
    return TILE_BLANK;
}

/* Writes str into row_buf starting at x; returns the next x. */
static uint8_t put_text(uint8_t x, const char *str)
{
    while (*str && x < VIEW_W) row_buf[x++] = char_tile(*str++);
    return x;
}

static void put_number3(uint8_t x, uint16_t n)
{
    if (n > 999) n = 999;
    row_buf[x]     = FONT_BASE + (uint8_t)(n / 100);
    row_buf[x + 1] = FONT_BASE + (uint8_t)((n / 10) % 10);
    row_buf[x + 2] = FONT_BASE + (uint8_t)(n % 10);
}

static void clear_row_buf(void)
{
    uint8_t x;
    for (x = 0; x < VIEW_W; x++) row_buf[x] = TILE_BLANK;
}

/* ------------------------------------------------------------------ */
/* Inventory helpers                                                   */
/* ------------------------------------------------------------------ */

static uint8_t has_items(void)
{
    uint8_t i;
    for (i = 0; i < ITEM_TYPES; i++)
        if (state.items[i]) return 1;
    return 0;
}

/* Next item type with a non-zero count, searching from 'from' in
 * direction step (+1 / ITEM_TYPES-1 for -1). Returns 'from' if none. */
static uint8_t next_item(uint8_t from, uint8_t step)
{
    uint8_t i = from, n;
    for (n = 0; n < ITEM_TYPES; n++) {
        i = (uint8_t)((i + step) % ITEM_TYPES);
        if (state.items[i]) return i;
    }
    return from;
}

/* ------------------------------------------------------------------ */
/* HUD                                                                 */
/* ------------------------------------------------------------------ */

static void draw_hud(void)
{
    if (ui_mode == UI_INVENTORY) return;   /* the window shows the inventory */

    /* Row 1: "1 FIRST STEPS   T012" or "THROW STONE     T012" */
    clear_row_buf();
    if (ui_mode == UI_AIM) {
        put_text(put_text(0, "THROW ") , item_defs[inv_item].name);
    } else {
        /* level number (1-99), a space, then the name (max 12 chars) */
        uint8_t n = level_idx + 1, x = 0;
        if (n >= 10) row_buf[x++] = FONT_BASE + n / 10;
        row_buf[x++] = FONT_BASE + n % 10;
        put_text(x + 1, levels[level_idx].name);
    }
    row_buf[16] = char_tile('T');
    put_number3(17, state.turn);
    set_win_tiles(0, 0, VIEW_W, 1, row_buf);

    /* Row 2: what the player can do right now */
    clear_row_buf();
    if (hud_msg)                         put_text(0, hud_msg);
    else if (state.status == STATUS_CAUGHT) put_text(0, "SPOTTED! START:UNDO");
    else if (state.status == STATUS_ALARM)  put_text(0, "BODY SEEN START:UNDO");
    else if (state.status == STATUS_WON)    put_text(0, "ESCAPED!   A:NEXT");
    else if (ui_mode == UI_AIM)          put_text(0, "A:THROW B:BACK");
    else if (game_takedown_target(&state) != 0xFF) put_text(0, "B:TAKEDOWN");
    else if (game_uniform_here(&state) != 0xFF)    put_text(0, "B:TAKE UNIFORM");
    else if (game_body_adjacent(&state)) put_text(0, "HOLD B:DRAG");
    else if (looking)                    put_text(0, "LOOKING AROUND");
    else if (has_items())                put_text(0, "SEL:ITEMS START:UNDO");
    else                                 put_text(0, "A:WAIT START:UNDO");
    set_win_tiles(0, 1, VIEW_W, 1, row_buf);
    hud_msg = 0;   /* messages last until the next HUD redraw */
}

/* ------------------------------------------------------------------ */
/* Map + sprites                                                       */
/* ------------------------------------------------------------------ */

/* Draw the w x h block of the level at (x0, y0) into the background map
 * (wrapping at 32 tiles), with vision cones and the aiming overlay. */
static void draw_tiles(uint8_t x0, uint8_t y0, uint8_t w, uint8_t h)
{
    uint8_t  x, y, wx, wy, cell, seen, aim, bx, first;
    uint8_t *v = vision;

    game_compute_vision(&state, x0, y0, w, h, vision);

    for (y = 0; y < h; y++) {
        wy = y0 + y;
        for (x = 0; x < w; x++, v++) {
            wx   = x0 + x;
            cell = game_cell(wx, wy);
            seen = *v;
            if (cell == CELL_WALL) {
                row_buf[x] = TILE_WALL;
            } else if (cell == CELL_EXIT) {
                row_buf[x] = seen == VIS_SEEN ? TILE_EXIT_SEEN : TILE_EXIT;
            } else {
                aim = (ui_mode == UI_AIM) && game_can_throw(&state, inv_item, wx, wy);
                if (aim)                    row_buf[x] = seen == VIS_SEEN ? TILE_AIM_SEEN : TILE_AIM;
                else if (seen == VIS_SEEN)  row_buf[x] = TILE_SEEN;
                else if (seen)              row_buf[x] = TILE_WATCH;  /* blind guard */
                else                        row_buf[x] = TILE_FLOOR;
            }
        }
        /* the row may wrap past the right edge of the 32-tile map */
        bx    = x0 & 31;
        first = 32 - bx;
        if (first > w) first = w;
        set_bkg_tiles(bx, wy & 31, first, 1, row_buf);
        if (first < w) set_bkg_tiles(0, wy & 31, w - first, 1, row_buf + first);
    }
}

static void set_scroll(uint16_t px, uint16_t py)
{
    scroll_x = px;
    scroll_y = py;
    move_bkg((uint8_t)px, (uint8_t)py);   /* the map wraps every 256 px */
}

/* Camera position (one axis) that keeps tile t at least margin tiles inside
 * the view, moving as little as possible, and never past the level edge. */
static uint8_t follow(uint8_t cam, uint8_t t, uint8_t margin, uint8_t view, uint8_t size)
{
    int16_t c = cam;
    if ((int16_t)t < c + margin) c = (int16_t)t - margin;
    if ((int16_t)t > c + view - 1 - margin) c = (int16_t)t - (view - 1 - margin);
    if (c > (int16_t)size - view) c = (int16_t)size - view;
    if (c < 0) c = 0;
    return (uint8_t)c;
}

/* Where the camera should be: on the aiming cursor while aiming, wherever
 * the player panned it while looking, otherwise on the player. */
static void camera_target(uint8_t *nx, uint8_t *ny)
{
    if (looking) {
        *nx = cam_x;
        *ny = cam_y;
    } else if (ui_mode == UI_AIM) {
        *nx = follow(cam_x, aim_x, 1, VIEW_W, game_width());
        *ny = follow(cam_y, aim_y, 1, VIEW_H, game_height());
    } else {
        *nx = follow(cam_x, state.px, CAM_MARGIN_X, VIEW_W, game_width());
        *ny = follow(cam_y, state.py, CAM_MARGIN_Y, VIEW_H, game_height());
    }
}

static void draw_sprites(void);

/* Move the camera, redrawing the visible tiles. A one-tile move slides;
 * anything bigger (restart, undo) jumps. */
static void move_camera(uint8_t nx, uint8_t ny)
{
    int8_t  dx = (int8_t)(nx - cam_x), dy = (int8_t)(ny - cam_y);
    uint8_t f;

    if (dx < -1 || dx > 1 || dy < -1 || dy > 1 || (dx == 0 && dy == 0)) {
        cam_x = nx;
        cam_y = ny;
        draw_tiles(cam_x, cam_y, VIEW_W, VIEW_H);
        set_scroll((uint16_t)cam_x * 8, (uint16_t)cam_y * 8);
        return;
    }
    /* draw old view + the new strip, then slide across */
    draw_tiles(dx < 0 ? nx : cam_x, dy < 0 ? ny : cam_y,
               VIEW_W + (dx != 0), VIEW_H + (dy != 0));
    for (f = SCROLL_STEP; f <= 8; f += SCROLL_STEP) {
        vsync();
        set_scroll((uint16_t)cam_x * 8 + dx * f, (uint16_t)cam_y * 8 + dy * f);
        draw_sprites();
    }
    cam_x = nx;
    cam_y = ny;
}

/* Put a sprite on a level tile, hiding it if that tile is off screen or
 * would overlap the HUD. Hardware coordinates are offset by (8, 16). */
static void place_sprite(uint8_t n, uint8_t tx, uint8_t ty)
{
    int16_t sx = (int16_t)tx * 8 - (int16_t)scroll_x;
    int16_t sy = (int16_t)ty * 8 - (int16_t)scroll_y;
    if (sx <= -8 || sx >= VIEW_W * 8 || sy <= -8 || sy + 8 > VIEW_H * 8) {
        move_sprite(n, 0, 0);
        return;
    }
    move_sprite(n, (uint8_t)(sx + 8), (uint8_t)(sy + 16));
}

static void draw_cursor(void)
{
    if (ui_mode != UI_AIM) {
        move_sprite(SPRITE_CURSOR, 0, 0);
        return;
    }
    set_sprite_tile(SPRITE_CURSOR,
        game_can_throw(&state, inv_item, aim_x, aim_y) ? SPR_CURSOR : SPR_CURSOR_BAD);
    place_sprite(SPRITE_CURSOR, aim_x, aim_y);
}

/* First sprite tile of a guard's uniform style (see SPR_GUARD_STYLE_TILES). */
static uint8_t guard_tiles(uint8_t i)
{
    if (levels[level_idx].guards[i].uniform == DISGUISE_OFFICER) return SPR_OFFICER_UP;
    return SPR_GUARD_UP;
}

static void draw_sprites(void)
{
    uint8_t i;

    set_sprite_tile(SPRITE_PLAYER, SPR_PLAYER + state.disguise);
    place_sprite(SPRITE_PLAYER, state.px, state.py);

    for (i = 0; i < MAX_GUARDS; i++) {
        uint8_t spr   = SPRITE_GUARD + i;
        uint8_t noise = SPRITE_NOISE + i;
        if (i < state.guard_count) {
            const Guard *g = &state.guards[i];
            uint8_t base = guard_tiles(i);
            if (g->mode == GUARD_BODY)
                set_sprite_tile(spr, g->stripped ? SPR_BODY_STRIPPED : base + 5);
            else if (g->mode == GUARD_SUBDUED) set_sprite_tile(spr, base + 4);
            else                               set_sprite_tile(spr, base + g->facing);
            place_sprite(spr, g->x, g->y);
            /* mark the spot this guard is heading to investigate */
            if (g->mode == GUARD_INVESTIGATE) {
                set_sprite_tile(noise, SPR_NOISE);
                place_sprite(noise, g->tx, g->ty);
            } else {
                move_sprite(noise, 0, 0);
            }
        } else {
            move_sprite(spr, 0, 0);     /* off screen */
            move_sprite(noise, 0, 0);
        }
    }
    draw_cursor();
}

static void redraw(void)
{
    uint8_t nx, ny;
    camera_target(&nx, &ny);
    if (nx == cam_x && ny == cam_y) draw_tiles(cam_x, cam_y, VIEW_W, VIEW_H);
    else move_camera(nx, ny);
    draw_sprites();
    draw_hud();
}

/* ------------------------------------------------------------------ */
/* Inventory window (window layer, drawn over the bottom of the map)   */
/* ------------------------------------------------------------------ */

static void draw_inventory(void)
{
    uint8_t i, row = 2;

    for (i = 0; i < VIEW_W; i++) row_buf[i] = TILE_WIN_EDGE;
    set_win_tiles(0, 0, VIEW_W, 1, row_buf);

    clear_row_buf();
    put_text(1, "THROW WHAT?");
    set_win_tiles(0, 1, VIEW_W, 1, row_buf);

    for (i = 0; i < ITEM_TYPES; i++) {
        if (!state.items[i]) continue;
        clear_row_buf();
        if (i == inv_item) row_buf[1] = FONT_GT;
        put_text(3, item_defs[i].name);
        row_buf[15] = char_tile('X');
        if (state.items[i] >= 10) {
            row_buf[16] = FONT_BASE + state.items[i] / 10;
            row_buf[17] = FONT_BASE + state.items[i] % 10;
        } else {
            row_buf[16] = FONT_BASE + state.items[i];
        }
        set_win_tiles(0, row++, VIEW_W, 1, row_buf);
    }
    clear_row_buf();
    while (row < WIN_ROWS - 1) set_win_tiles(0, row++, VIEW_W, 1, row_buf);

    put_text(1, "A:AIM  B:BACK");
    set_win_tiles(0, WIN_ROWS - 1, VIEW_W, 1, row_buf);
}

static void open_inventory(void)
{
    if (!state.items[inv_item]) inv_item = next_item(inv_item, 1);
    ui_mode = UI_INVENTORY;
    draw_inventory();
    move_win(7, WIN_Y);   /* grow the window up over the map */
    HIDE_SPRITES;         /* sprites would draw on top of the window */
}

static void close_inventory(void)
{
    move_win(7, HUD_WIN_Y);   /* back to just the HUD */
    SHOW_SPRITES;
}

static void back_to_play(void)
{
    if (ui_mode == UI_INVENTORY) close_inventory();
    ui_mode = UI_PLAY;
    redraw();
}

/* SELECT was let go: a tap opens the inventory, a look returns to the
 * player. */
static void select_released(void)
{
    if (!select_armed) return;
    select_armed = 0;
    if (looking) {             /* put the camera back where it was */
        looking = 0;
        move_camera(look_x, look_y);
        redraw();
        return;
    }
    if (ui_mode != UI_PLAY || state.status != STATUS_PLAYING) return;
    if (has_items()) {
        open_inventory();
    } else {
        hud_msg = "NOTHING TO THROW";
        draw_hud();
    }
}

/* ------------------------------------------------------------------ */
/* Level flow                                                          */
/* ------------------------------------------------------------------ */

static void load_level(uint8_t idx)
{
    if (ui_mode == UI_INVENTORY) close_inventory();
    ui_mode   = UI_PLAY;
    level_idx = idx;
    inv_item  = 0;
    looking   = 0;
    select_armed = 0;
    game_load(&levels[idx], &state);
    undo_clear();
    /* start with the player in the middle of the screen */
    cam_x = follow(0, state.px, VIEW_W / 2 - 1, VIEW_W, game_width());
    cam_y = follow(0, state.py, VIEW_H / 2 - 1, VIEW_H, game_height());
    draw_tiles(cam_x, cam_y, VIEW_W, VIEW_H);
    set_scroll((uint16_t)cam_x * 8, (uint16_t)cam_y * 8);
    redraw();
}

/* ------------------------------------------------------------------ */
/* Input per mode                                                      */
/* ------------------------------------------------------------------ */

/* Run one turn; push undo and redraw if it happened. */
static uint8_t do_action(uint8_t action)
{
    /* SDCC can't initialise a struct from another struct in its
     * declaration ("needs curly braces"), so assign separately. */
    before = state;
    if (!game_step(&state, action)) return 0;
    undo_push(&before);
    redraw();
    return 1;
}

/* Wait a number of frames, making a guard mid-takedown struggle. */
static void struggle_frames(uint8_t frames)
{
    uint8_t f, spr = SPRITE_GUARD + state.takedown_guard;
    uint8_t facing = state.guards[state.takedown_guard].facing;
    uint8_t base   = guard_tiles(state.takedown_guard);
    for (f = 0; f < frames; f++) {
        vsync();
        set_sprite_tile(spr, (f & 4) ? base + 4 : base + facing);
    }
}

/* Start a takedown, then play out its locked turns automatically so the
 * player can watch the other guards move. Undo rewinds the whole thing. */
static void do_takedown(void)
{
    hud_msg = "TAKEDOWN...";
    if (!do_action(ACT_TAKEDOWN)) {
        hud_msg = 0;
        return;
    }
    while (state.takedown_left && state.status == STATUS_PLAYING) {
        struggle_frames(TAKEDOWN_PAUSE);
        game_step(&state, ACT_WAIT);
        if (state.takedown_left) hud_msg = "TAKEDOWN...";
        redraw();
    }
}

static void input_play(uint8_t pressed, uint8_t keys)
{
    uint8_t dir;

    if (state.status == STATUS_WON) {
        if (pressed & J_A) load_level((level_idx + 1) % LEVEL_COUNT);
        return;
    }
    if (state.status != STATUS_PLAYING) return;   /* START to undo */

    if (pressed & J_B) {
        if (game_takedown_target(&state) != 0xFF) do_takedown();
        else if (game_uniform_here(&state) != 0xFF) do_action(ACT_UNIFORM);
        return;   /* otherwise B only matters while held (dragging) */
    }

    if (pressed & J_SELECT) {   /* acted on when let go: see select_released */
        select_armed = 1;
        return;
    }

    /* holding SELECT: the D-pad pans the camera to look around */
    if (keys & J_SELECT) {
        uint8_t nx = cam_x, ny = cam_y;
        if      ((pressed & J_UP) && ny > 0)                         ny--;
        else if ((pressed & J_DOWN) && ny < game_height() - VIEW_H)  ny++;
        else if ((pressed & J_LEFT) && nx > 0)                       nx--;
        else if ((pressed & J_RIGHT) && nx < game_width() - VIEW_W)  nx++;
        if (pressed & (J_UP | J_DOWN | J_LEFT | J_RIGHT)) {
            if (!looking) {
                look_x = cam_x;
                look_y = cam_y;
            }
            looking = 1;
            move_camera(nx, ny);
            draw_sprites();
            draw_hud();
        }
        return;
    }

    if (pressed & J_A) {
        do_action(ACT_WAIT);
        return;
    }

    if      (pressed & J_UP)    dir = DIR_UP;
    else if (pressed & J_DOWN)  dir = DIR_DOWN;
    else if (pressed & J_LEFT)  dir = DIR_LEFT;
    else if (pressed & J_RIGHT) dir = DIR_RIGHT;
    else return;

    /* Holding B next to a body drags/pushes it; otherwise a normal step. */
    if ((keys & J_B) && game_body_adjacent(&state)) do_action(ACT_DRAG + dir);
    else                                             do_action(dir);
}

static void input_inventory(uint8_t pressed)
{
    if (pressed & (J_B | J_SELECT)) {
        back_to_play();
    } else if (pressed & J_UP) {
        inv_item = next_item(inv_item, ITEM_TYPES - 1);
        draw_inventory();
    } else if (pressed & J_DOWN) {
        inv_item = next_item(inv_item, 1);
        draw_inventory();
    } else if (pressed & J_A) {
        close_inventory();
        ui_mode = UI_AIM;
        aim_x   = state.px;
        aim_y   = state.py;
        redraw();     /* shows the throwable tiles */
    }
}

static void input_aim(uint8_t pressed)
{
    uint8_t nx = aim_x, ny = aim_y;

    if (pressed & J_SELECT) {
        back_to_play();
        return;
    }
    if (pressed & J_B) {
        ui_mode = UI_PLAY;
        redraw();          /* clear the aim overlay */
        open_inventory();
        return;
    }
    if (pressed & J_A) {
        before = state;   /* separate assignment: see do_action() */
        if (game_throw(&state, inv_item, aim_x, aim_y)) {
            undo_push(&before);
            ui_mode = UI_PLAY;
            redraw();
        } else {
            hud_msg = "NO CLEAR SHOT";
            draw_hud();
        }
        return;
    }

    if      (pressed & J_UP)    ny--;
    else if (pressed & J_DOWN)  ny++;
    else if (pressed & J_LEFT)  nx--;
    else if (pressed & J_RIGHT) nx++;
    else return;

    /* keep the cursor on the map and within throwing range */
    if (nx >= game_width() || ny >= game_height()) return;
    if (game_distance(state.px, state.py, nx, ny) > item_defs[inv_item].range) return;
    aim_x = nx;
    aim_y = ny;
    camera_target(&nx, &ny);          /* scroll if the cursor nears an edge */
    if (nx != cam_x || ny != cam_y) {
        move_camera(nx, ny);
        draw_sprites();
    } else {
        draw_cursor();
    }
}

/* ------------------------------------------------------------------ */
/* Main loop                                                           */
/* ------------------------------------------------------------------ */

/* START: tap to undo (while playing), hold to restart the level. */
static void handle_start(uint8_t keys, uint8_t released)
{
    static uint8_t held;

    if (keys & J_START) {
        if (held < 255) held++;
        if (held == 15) {
            hud_msg = "HOLD TO RESTART";
            draw_hud();
        }
        if (held == RESTART_HOLD) load_level(level_idx);
        return;
    }
    if (released & J_START) {
        if (held < 15) {                    /* a tap: undo */
            if (ui_mode == UI_PLAY && undo_pop(&state)) redraw();
        } else if (held < RESTART_HOLD) {   /* let go early: cancel */
            draw_hud();
        }
    }
    held = 0;
}

void main(void)
{
    uint8_t keys, prev = 0, pressed, released;

    DISPLAY_OFF;
    BGP_REG  = 0xE4;   /* 11 10 01 00: index 0 = white ... 3 = black */
    OBP0_REG = 0xE4;
    set_bkg_data(0, BKG_TILE_COUNT, bkg_tile_data);
    set_sprite_data(0, SPRITE_TILE_COUNT, sprite_tile_data);
    move_win(7, HUD_WIN_Y);   /* the HUD: bottom two rows, never scrolls */
    load_level(0);
    SHOW_BKG;
    SHOW_WIN;
    SHOW_SPRITES;
    DISPLAY_ON;

    for (;;) {
        vsync();
        keys     = joypad();
        pressed  = keys & ~prev;    /* only react to new presses */
        released = prev & ~keys;
        prev     = keys;

        handle_start(keys, released);
        if (released & J_SELECT) select_released();
        pressed &= ~J_START;
        if (!pressed) continue;

        if      (ui_mode == UI_INVENTORY) input_inventory(pressed);
        else if (ui_mode == UI_AIM)       input_aim(pressed);
        else                              input_play(pressed, keys);
    }
}
