/*
 * main.c - Game Boy side: input, drawing, and the screen flow.
 * All the rules live in game.c; this file only reads GameState.
 *
 * Controls (playing):
 *   D-pad   move one tile (spends a turn)
 *   A       wait one turn            (after escaping: next level)
 *   B       undo
 *   SELECT  open inventory
 *   START   restart level
 *
 * Inventory:  UP/DOWN choose, A aim, B/SELECT close
 * Aiming:     D-pad move cursor, A throw (spends a turn), B back, SELECT cancel
 */
#include <gb/gb.h>
#include <stdint.h>

#include "game.h"
#include "levels.h"
#include "tiles.h"

#define HUD_Y 16   /* HUD occupies rows 16 and 17 */

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

static GameState   state;
static GameState   before;   /* snapshot taken before each turn, for undo */
static uint8_t     level_idx;
static uint8_t     vision[MAP_H][MAP_W];
static uint8_t     row_buf[MAP_W];

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
    while (*str && x < MAP_W) row_buf[x++] = char_tile(*str++);
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
    for (x = 0; x < MAP_W; x++) row_buf[x] = TILE_BLANK;
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
    /* Row 1: "1 FIRST STEPS   T012" or "THROW STONE     T012" */
    clear_row_buf();
    if (ui_mode == UI_AIM) {
        put_text(put_text(0, "THROW ") , item_defs[inv_item].name);
    } else {
        row_buf[0] = FONT_BASE + level_idx + 1;
        put_text(2, levels[level_idx].name);
    }
    row_buf[16] = char_tile('T');
    put_number3(17, state.turn);
    set_bkg_tiles(0, HUD_Y, MAP_W, 1, row_buf);

    /* Row 2: what the player can do right now */
    clear_row_buf();
    if (hud_msg)                         put_text(0, hud_msg);
    else if (state.status == STATUS_CAUGHT) put_text(0, "SPOTTED!   B:UNDO");
    else if (state.status == STATUS_WON)    put_text(0, "ESCAPED!   A:NEXT");
    else if (ui_mode == UI_AIM)          put_text(0, "A:THROW B:BACK");
    else if (has_items())                put_text(0, "B:UNDO SELECT:ITEMS");
    else                                 put_text(0, "A:WAIT B:UNDO");
    set_bkg_tiles(0, HUD_Y + 1, MAP_W, 1, row_buf);
    hud_msg = 0;   /* messages last until the next HUD redraw */
}

/* ------------------------------------------------------------------ */
/* Map + sprites                                                       */
/* ------------------------------------------------------------------ */

static void draw_map(void)
{
    uint8_t x, y, cell, seen, aim;

    game_compute_vision(&state, vision);

    for (y = 0; y < MAP_H; y++) {
        for (x = 0; x < MAP_W; x++) {
            cell = game_cell(x, y);
            seen = vision[y][x];
            if (cell == CELL_WALL) {
                row_buf[x] = TILE_WALL;
            } else if (cell == CELL_EXIT) {
                row_buf[x] = seen ? TILE_EXIT_SEEN : TILE_EXIT;
            } else {
                aim = (ui_mode == UI_AIM) && game_can_throw(&state, inv_item, x, y);
                if (aim) row_buf[x] = seen ? TILE_AIM_SEEN : TILE_AIM;
                else     row_buf[x] = seen ? TILE_SEEN : TILE_FLOOR;
            }
        }
        set_bkg_tiles(0, y, MAP_W, 1, row_buf);
    }
}

/* Hardware sprite coordinates are offset by (8, 16). */
#define SPR_X(tx) ((uint8_t)((tx) * 8 + 8))
#define SPR_Y(ty) ((uint8_t)((ty) * 8 + 16))

static void draw_cursor(void)
{
    if (ui_mode != UI_AIM) {
        move_sprite(SPRITE_CURSOR, 0, 0);
        return;
    }
    set_sprite_tile(SPRITE_CURSOR,
        game_can_throw(&state, inv_item, aim_x, aim_y) ? SPR_CURSOR : SPR_CURSOR_BAD);
    move_sprite(SPRITE_CURSOR, SPR_X(aim_x), SPR_Y(aim_y));
}

static void draw_sprites(void)
{
    uint8_t i;

    set_sprite_tile(SPRITE_PLAYER, SPR_PLAYER);
    move_sprite(SPRITE_PLAYER, SPR_X(state.px), SPR_Y(state.py));

    for (i = 0; i < MAX_GUARDS; i++) {
        uint8_t spr   = SPRITE_GUARD + i;
        uint8_t noise = SPRITE_NOISE + i;
        if (i < state.guard_count) {
            const Guard *g = &state.guards[i];
            set_sprite_tile(spr, SPR_GUARD_UP + g->facing);
            move_sprite(spr, SPR_X(g->x), SPR_Y(g->y));
            /* mark the spot this guard is heading to investigate */
            if (g->mode == GUARD_INVESTIGATE) {
                set_sprite_tile(noise, SPR_NOISE);
                move_sprite(noise, SPR_X(g->tx), SPR_Y(g->ty));
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
    draw_map();
    draw_sprites();
    draw_hud();
}

/* ------------------------------------------------------------------ */
/* Inventory window (window layer, drawn over the bottom of the map)   */
/* ------------------------------------------------------------------ */

static void draw_inventory(void)
{
    uint8_t i, row = 2;

    for (i = 0; i < MAP_W; i++) row_buf[i] = TILE_WIN_EDGE;
    set_win_tiles(0, 0, MAP_W, 1, row_buf);

    clear_row_buf();
    put_text(1, "THROW WHAT?");
    set_win_tiles(0, 1, MAP_W, 1, row_buf);

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
        set_win_tiles(0, row++, MAP_W, 1, row_buf);
    }
    clear_row_buf();
    while (row < WIN_ROWS - 1) set_win_tiles(0, row++, MAP_W, 1, row_buf);

    put_text(1, "A:AIM  B:BACK");
    set_win_tiles(0, WIN_ROWS - 1, MAP_W, 1, row_buf);
}

static void open_inventory(void)
{
    if (!state.items[inv_item]) inv_item = next_item(inv_item, 1);
    ui_mode = UI_INVENTORY;
    draw_inventory();
    move_win(7, WIN_Y);
    HIDE_SPRITES;     /* sprites would draw on top of the window */
    SHOW_WIN;
}

static void close_inventory(void)
{
    HIDE_WIN;
    SHOW_SPRITES;
}

static void back_to_play(void)
{
    if (ui_mode == UI_INVENTORY) close_inventory();
    ui_mode = UI_PLAY;
    redraw();
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
    game_load(&levels[idx], &state);
    undo_clear();
    redraw();
}

/* ------------------------------------------------------------------ */
/* Input per mode                                                      */
/* ------------------------------------------------------------------ */

static void input_play(uint8_t pressed)
{
    uint8_t action;

    if (pressed & J_B) {
        if (undo_pop(&state)) redraw();
        return;
    }

    if (state.status == STATUS_WON) {
        if (pressed & J_A) load_level((level_idx + 1) % LEVEL_COUNT);
        return;
    }
    if (state.status != STATUS_PLAYING) return;

    if (pressed & J_SELECT) {
        if (has_items()) {
            open_inventory();
        } else {
            hud_msg = "NOTHING TO THROW";
            draw_hud();
        }
        return;
    }

    if      (pressed & J_UP)    action = ACT_UP;
    else if (pressed & J_DOWN)  action = ACT_DOWN;
    else if (pressed & J_LEFT)  action = ACT_LEFT;
    else if (pressed & J_RIGHT) action = ACT_RIGHT;
    else if (pressed & J_A)     action = ACT_WAIT;
    else return;

    /* SDCC can't initialise a struct from another struct in its
     * declaration ("needs curly braces"), so assign separately. */
    before = state;
    if (game_step(&state, action)) {
        undo_push(&before);
        redraw();
    }
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
        before = state;
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
    if (nx >= MAP_W || ny >= MAP_H) return;
    if (game_distance(state.px, state.py, nx, ny) > item_defs[inv_item].range) return;
    aim_x = nx;
    aim_y = ny;
    draw_cursor();
}

/* ------------------------------------------------------------------ */
/* Main loop                                                           */
/* ------------------------------------------------------------------ */

void main(void)
{
    uint8_t keys, prev = 0, pressed;

    DISPLAY_OFF;
    BGP_REG  = 0xE4;   /* 11 10 01 00: index 0 = white ... 3 = black */
    OBP0_REG = 0xE4;
    set_bkg_data(0, BKG_TILE_COUNT, bkg_tile_data);
    set_sprite_data(0, SPRITE_TILE_COUNT, sprite_tile_data);
    HIDE_WIN;
    load_level(0);
    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;

    for (;;) {
        vsync();
        keys    = joypad();
        pressed = keys & ~prev;    /* only react to new presses */
        prev    = keys;
        if (!pressed) continue;

        if (pressed & J_START) {
            load_level(level_idx);
            continue;
        }

        if      (ui_mode == UI_INVENTORY) input_inventory(pressed);
        else if (ui_mode == UI_AIM)       input_aim(pressed);
        else                              input_play(pressed);
    }
}
