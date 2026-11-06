/*
 * main.c - Game Boy side: input, drawing, and the screen flow.
 * All the rules live in game.c; this file only reads GameState.
 *
 * Controls:
 *   D-pad   move one tile (spends a turn)
 *   A       wait one turn
 *   B       undo
 *   START   restart level
 *   A       (after escaping) next level
 */
#include <gb/gb.h>
#include <stdint.h>

#include "game.h"
#include "levels.h"
#include "tiles.h"

#define HUD_Y 16   /* HUD occupies rows 16 and 17 */

static GameState state;
static GameState before;   /* snapshot taken before each turn, for undo */
static uint8_t   level_idx;
static uint8_t   vision[MAP_H][MAP_W];
static uint8_t   row_buf[MAP_W];

/* ------------------------------------------------------------------ */
/* HUD text                                                            */
/* ------------------------------------------------------------------ */

static uint8_t char_tile(char c)
{
    if (c >= '0' && c <= '9') return FONT_BASE + (c - '0');
    if (c >= 'A' && c <= 'Z') return FONT_BASE + 10 + (c - 'A');
    if (c == ':') return FONT_COLON;
    if (c == '!') return FONT_BANG;
    if (c == '-') return FONT_DASH;
    if (c == '.') return FONT_PERIOD;
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

static void draw_hud(void)
{
    /* Row 1: "1 FIRST STEPS   T012" */
    clear_row_buf();
    row_buf[0] = FONT_BASE + level_idx + 1;
    put_text(2, levels[level_idx].name);
    row_buf[16] = char_tile('T');
    put_number3(17, state.turn);
    set_bkg_tiles(0, HUD_Y, MAP_W, 1, row_buf);

    /* Row 2: what the player can do right now */
    clear_row_buf();
    if (state.status == STATUS_CAUGHT)
        put_text(0, "SPOTTED!   B:UNDO");
    else if (state.status == STATUS_WON)
        put_text(0, "ESCAPED!   A:NEXT");
    else
        put_text(0, "A:WAIT B:UNDO");
    set_bkg_tiles(0, HUD_Y + 1, MAP_W, 1, row_buf);
}

/* ------------------------------------------------------------------ */
/* Map + sprites                                                       */
/* ------------------------------------------------------------------ */

static void draw_map(void)
{
    uint8_t x, y, cell, seen;

    game_compute_vision(&state, vision);

    for (y = 0; y < MAP_H; y++) {
        for (x = 0; x < MAP_W; x++) {
            cell = game_cell(x, y);
            seen = vision[y][x];
            if (cell == CELL_WALL)      row_buf[x] = TILE_WALL;
            else if (cell == CELL_EXIT) row_buf[x] = seen ? TILE_EXIT_SEEN : TILE_EXIT;
            else                        row_buf[x] = seen ? TILE_SEEN : TILE_FLOOR;
        }
        set_bkg_tiles(0, y, MAP_W, 1, row_buf);
    }
}

/* Hardware sprite coordinates are offset by (8, 16). */
#define SPR_X(tx) ((uint8_t)((tx) * 8 + 8))
#define SPR_Y(ty) ((uint8_t)((ty) * 8 + 16))

static void draw_sprites(void)
{
    uint8_t i;

    set_sprite_tile(0, SPR_PLAYER);
    move_sprite(0, SPR_X(state.px), SPR_Y(state.py));

    for (i = 0; i < MAX_GUARDS; i++) {
        uint8_t spr = i + 1;
        if (i < state.guard_count) {
            const Guard *g = &state.guards[i];
            set_sprite_tile(spr, SPR_GUARD_UP + g->facing);
            move_sprite(spr, SPR_X(g->x), SPR_Y(g->y));
        } else {
            move_sprite(spr, 0, 0);   /* off screen */
        }
    }
}

static void redraw(void)
{
    draw_map();
    draw_sprites();
    draw_hud();
}

static void load_level(uint8_t idx)
{
    level_idx = idx;
    game_load(&levels[idx], &state);
    undo_clear();
    redraw();
}

/* ------------------------------------------------------------------ */
/* Main loop                                                           */
/* ------------------------------------------------------------------ */

void main(void)
{
    uint8_t keys, prev = 0, pressed, action;

    DISPLAY_OFF;
    BGP_REG  = 0xE4;   /* 11 10 01 00: index 0 = white ... 3 = black */
    OBP0_REG = 0xE4;
    set_bkg_data(0, BKG_TILE_COUNT, bkg_tile_data);
    set_sprite_data(0, SPRITE_TILE_COUNT, sprite_tile_data);
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

        if (pressed & J_B) {
            if (undo_pop(&state)) redraw();
            continue;
        }

        if (state.status == STATUS_WON) {
            if (pressed & J_A)
                load_level((level_idx + 1) % LEVEL_COUNT);
            continue;
        }

        if      (pressed & J_UP)    action = ACT_UP;
        else if (pressed & J_DOWN)  action = ACT_DOWN;
        else if (pressed & J_LEFT)  action = ACT_LEFT;
        else if (pressed & J_RIGHT) action = ACT_RIGHT;
        else if (pressed & J_A)     action = ACT_WAIT;
        else continue;

        /* SDCC can't initialise a struct from another struct in its
         * declaration ("needs curly braces"), so assign separately. */
        before = state;
        if (game_step(&state, action)) {
            undo_push(&before);
            redraw();
        }
    }
}
