/*
 * game.h - Pure game rules. No Game Boy hardware access in here.
 *
 * Everything in game.c is deterministic and depends only on the level data
 * and the GameState passed in. That gives us:
 *   - cheap undo (a GameState is ~50 bytes; just snapshot it),
 *   - puzzles that always play out the same way,
 *   - the ability to compile and test the rules on a PC (see tests/).
 */
#ifndef GAME_H
#define GAME_H

#include <stdint.h>

/* The screen shows VIEW_W x VIEW_H map tiles (the bottom 2 of the 18 tile
 * rows are the HUD). Levels are 1-3 screens across and down, and the view
 * cuts from screen to screen. Their size is limited by RAM: pathfinding
 * needs one byte per tile. (The rules work for any size; the whole-screen
 * rule is checked by tools/gen_levels.py and main.c relies on it.) */
#define VIEW_W        20
#define VIEW_H        16
#define MAP_MAX_W     60    /* 3 screens wide */
#define MAP_MAX_H     48    /* 3 screens tall */
#define MAP_MAX_CELLS 2048  /* width * height: up to 3x2 or 2x3 screens */

#define MAX_GUARDS    4
#define MAX_WAYPOINTS 6
#define VISION_RANGE  4   /* how many tiles ahead a guard can see */
#define HOSTILE_RANGE 7   /* ... when hostile to your disguise */
#define HOSTILE_HALF  3   /* hostile cone half-width (7 tiles across) */
#define UNDO_DEPTH    32  /* turns of undo history kept in RAM */
#define TAKEDOWN_TURNS 3  /* turns the player and target are locked */

/* Directions. Order matters: sprite tiles are laid out in this order. */
#define DIR_UP    0
#define DIR_RIGHT 1
#define DIR_DOWN  2
#define DIR_LEFT  3

/* Player actions fed into game_step(). */
#define ACT_UP        DIR_UP
#define ACT_RIGHT     DIR_RIGHT
#define ACT_DOWN      DIR_DOWN
#define ACT_LEFT      DIR_LEFT
#define ACT_WAIT      4
#define ACT_DRAG      5   /* ACT_DRAG + dir: move while pushing/dragging */
#define ACT_TAKEDOWN  9   /* grab an adjacent guard */
#define ACT_UNIFORM   10  /* put on the uniform of the body you stand on */
#define ACT_TALK      11  /* talk to (pet, ...) the character next to you */
#define ACT_COUNT     12

/* Throwables. Stats live in item_defs[] (game.c), so items can differ
 * later (a louder bottle, a coin that rolls...). */
#define ITEM_TYPES  2
#define ITEM_STONE  0
#define ITEM_BOTTLE 1

typedef struct {
    const char *name;        /* max 8 chars for the inventory window */
    uint8_t     range;       /* max throw distance (grid steps) */
    uint8_t     noise;       /* guards within this many steps hear it */
} ItemDef;

extern const ItemDef item_defs[ITEM_TYPES];

/* Disguises. A guard's uniform is what you get from its body. */
#define DISGUISE_TYPES   3
#define DISGUISE_NONE    0   /* your own clothes */
#define DISGUISE_GUARD   1
#define DISGUISE_OFFICER 2

/* Access: where each outfit may be. Every cell of a map says, for each
 * disguise, whether someone wearing it is legal there, trespassing, or in
 * a hostile area. Guards don't have opinions of their own: they enforce
 * these rules for the cell the player is standing on.
 *
 *   legal        guards ignore you. Their normal cone still spots bodies,
 *                takedowns and suspicious actions.
 *   trespassing  a guard whose normal cone covers you catches you.
 *   hostile      as trespassing, but with the big cone (HOSTILE_RANGE).
 *
 * The one exception is an enforcer: a guard can be tagged as seeing
 * through particular disguises (GuardDef.enforcer), and treats someone
 * wearing one as trespassing even where that outfit is legal.
 *
 * One byte per cell: two bits per disguise, disguise d in bits 2d, 2d+1. */
#define ACCESS_LEGAL    0
#define ACCESS_TRESPASS 1
#define ACCESS_HOSTILE  2
/* A map without an access layer: your own clothes trespass everywhere,
 * uniforms are legal everywhere. */
#define ACCESS_DEFAULT  (ACCESS_TRESPASS << (2 * DISGUISE_NONE))

/* What a guard is doing. */
#define GUARD_PATROL      0  /* following its route (or standing post) */
#define GUARD_INVESTIGATE 1  /* walking to a noise */
#define GUARD_RETURN      2  /* walking back to where it left its route */
#define GUARD_SUBDUED     3  /* being taken down; frozen, blind, deaf */
#define GUARD_BODY        4  /* out cold; x,y is where the body lies */

/* Guards in these modes see, hear and move. */
#define GUARD_ACTIVE(g) ((g)->mode <= GUARD_RETURN)

/* Map cells. Each kind blocks some of four things (cell_blocks[]):
 *
 *   char  cell       player+bodies guards  sight  throws
 *   '#'   wall           x           x       x      x
 *   '.'   floor
 *   'E'   exit
 *   'D'   door                               x      x
 *   'W'   window         x           x              x
 *   'R'   railing        x           x
 *   'S'   security       x                   x      x   (guards only)
 *   'C'   curtain                            x
 *   'M'   mesh                                      x
 *   'V'   vent                       x       x      x   (player only)
 *
 * Anything standing on a sight-blocking tile can't be seen, so vents,
 * curtains and doorways hide the player and bodies. 'P' (player start) is
 * floor. */
#define CELL_FLOOR    0
#define CELL_WALL     1
#define CELL_EXIT     2
#define CELL_DOOR     3
#define CELL_WINDOW   4
#define CELL_RAILING  5
#define CELL_SECURITY 6
#define CELL_CURTAIN  7
#define CELL_MESH     8
#define CELL_VENT     9
#define CELL_TYPES    10

#define BLOCK_PLAYER 1   /* the player walking, and bodies being moved */
#define BLOCK_GUARD  2   /* guards walking (patrols and pathfinding) */
#define BLOCK_SIGHT  4   /* vision cones */
#define BLOCK_THROW  8   /* thrown items: their path and where they land */
#define BLOCK_ALL    15

extern const uint8_t cell_blocks[CELL_TYPES];

/* Game status. */
#define STATUS_PLAYING 0
#define STATUS_CAUGHT  1
#define STATUS_WON     2
#define STATUS_ALARM   3   /* a guard saw a body or a takedown */

typedef struct {
    uint8_t x, y;
} Point;

/* ---- Talking, scripts and switch cells ----
 *
 * Any character (GuardDef) can have a script: then it can be talked to
 * (ACT_TALK from a neighbouring cell). Talking runs its script and costs
 * a turn. A script is a few bytes of the ops below; it can show messages
 * and set or clear the level's flags (GameState.flags, eight per level,
 * shared by all its maps). A switch cell is one kind of cell while its
 * flag is clear and another once it is set: that is how a script changes
 * the map, e.g. makes the exit appear.
 *
 * In the map, a switch cell is the character '0' + its index in
 * LevelDef.switches. */
#define MAX_SWITCHES 8    /* per map */
#define MAX_FLAGS    8    /* per level */
#define MAX_SAID     4    /* messages one turn can show */

#define OP_END   0        /* stop */
#define OP_SAY   1        /* SAY n: show message n (the level table's text) */
#define OP_SET   2        /* SET mask: set those flags */
#define OP_CLEAR 3        /* CLEAR mask */
#define OP_IF    4        /* IF mask, n: skip the next n bytes unless a flag in mask is set */
#define OP_JUMP  5        /* JUMP n: skip the next n bytes */

typedef struct {
    uint8_t x, y;
    uint8_t flag;            /* mask in GameState.flags */
    uint8_t off, on;         /* CELL_* while the flag is clear / set */
    uint8_t on_art;          /* painted look once set: a local art tile
                                (art.h), 0xFF = built-in graphic. While
                                clear, the cell's own art is used. */
} SwitchDef;

/* ---- Static level data (lives in ROM) ---- */

/* A character: a guard, or anyone else in the map (a dog, a cleaner...).
 * They all patrol a route (or stand still), watch with a cone and enforce
 * the map's access rules (or don't watch at all), hear noises, block
 * walking while standing, and can be taken down and/or talked to. The
 * code calls them all guards. */
#define CHAR_NO_TAKEDOWN 1   /* GuardDef.flags: B can't take it down */
#define CHAR_NO_SIGHT    2   /* no cone at all: never catches you or raises
                                the alarm (a dog, a cleaner...). Still
                                hears noises. */

typedef struct {
    uint8_t facing;          /* initial facing; only matters if wp_count == 1 */
    uint8_t wp_count;        /* 1 = stationary sentry */
    Point   wp[MAX_WAYPOINTS]; /* patrol loop; guard starts on wp[0] */
    uint8_t uniform;         /* DISGUISE_* worn; NONE = nothing to take */
    /* Disguises it sees through: bit DISGUISE_x set = wearing x counts as
     * trespassing to this guard even where x is legal. 0 = just enforces
     * the map's access rules. */
    uint8_t     enforcer;
    uint8_t     flags;       /* CHAR_* */
    uint8_t     look;        /* which sprites draw it (levels.h LOOK_*);
                                only looks, the rules never read it */
    const char    *verb;     /* talking: HUD prompt, "PET" -> B:PET (max 8
                                chars); 0 = can't be talked to */
    const uint8_t *script;   /* OP_* bytes, when verb is set */
} GuardDef;

typedef struct {
    const char *name;        /* shown in the HUD, max 12 chars */
    uint8_t     width;       /* a multiple of VIEW_W, up to MAP_MAX_W */
    uint8_t     height;      /* a multiple of VIEW_H, up to MAP_MAX_H */
    /* width * height chars, row after row:
     * '#' wall, '.' floor, 'E' exit, 'P' player start.
     * On the Game Boy this is in ROM bank BANK_MAPS (far.h): only
     * game_use reads it. */
    const char *map;
    uint8_t     guard_count;
    GuardDef    guards[MAX_GUARDS];
    uint8_t     items[ITEM_TYPES]; /* starting inventory (first map only) */
    /* A level can be several of these maps joined at their edges (see
     * world.h). part = which map of its level this is (0 = where the level
     * starts). link[dir] = 1 + index in the level table of the map beyond
     * that edge, 0 = none. link_offset[dir] = where the neighbour's row 0
     * (for left/right) or column 0 (for up/down) lines up along this map's
     * edge. */
    uint8_t     part;
    uint8_t     link[4];
    int8_t      link_offset[4];
    /* TRANS_* (trans.h): how the screen dips to black when this level is
     * finished. Only looks, and only read from a level's first map. */
    uint8_t     transition;
    uint8_t     switch_count;
    SwitchDef   switches[MAX_SWITCHES];
    /* The access layer (see ACCESS_*): width * height bytes, run-length
     * encoded as (run, byte) pairs. 0 = ACCESS_DEFAULT everywhere. Like
     * the map, it is in ROM bank BANK_MAPS and only game_use reads it. */
    const uint8_t *access;
} LevelDef;

/* ---- Dynamic state (lives in RAM, snapshotted for undo) ---- */

typedef struct {
    uint8_t x, y;
    uint8_t facing;
    uint8_t next_wp;         /* index of the waypoint it is walking toward */
    uint8_t mode;            /* GUARD_PATROL / _INVESTIGATE / _RETURN */
    uint8_t tx, ty;          /* noise being investigated */
    uint8_t hx, hy;          /* where it left its route ("home") */
    uint8_t stripped;        /* body's uniform already taken */
} Guard;

typedef struct {
    uint8_t  px, py;
    uint8_t  status;
    uint8_t  guard_count;
    uint16_t turn;
    uint8_t  items[ITEM_TYPES];   /* in the state so undo restores them */
    uint8_t  takedown_left;       /* turns left in a takedown, 0 = none */
    uint8_t  takedown_guard;      /* who is being taken down */
    uint8_t  disguise;            /* DISGUISE_* the player is wearing */
    uint8_t  flags;               /* the level's script flags (carried
                                     between its maps like the items) */
    Guard    guards[MAX_GUARDS];
} GameState;

/* Make lvl the current map (rules read the map through it) and start a
 * fresh game on it in *s. */
void game_load(const LevelDef *lvl, GameState *s);

/* Make lvl the current map without touching any game state, e.g. to
 * resume a map the player left earlier. */
void game_use(const LevelDef *lvl);

/* Forget cached pathfinding. Call it when what guards can walk on may have
 * changed behind the current LevelDef pointer (game_load and a change of
 * map already do). */
void game_map_changed(void);

/* Forget everything cached about level data. game_load does this; call
 * it when the data behind a LevelDef pointer may have been replaced (the
 * editor builds levels at addresses it reuses). */
void game_data_changed(void);

/* The player steps into this map at (x, y) from a neighbouring one: counts
 * as a turn, wins on an exit, and guards who can see the spot catch them,
 * but guards don't move. Returns 1. */
uint8_t game_arrive(GameState *s, uint8_t x, uint8_t y);

/* Is a standing guard (not a body) on this tile? */
uint8_t game_guard_at(const GameState *s, uint8_t x, uint8_t y);

/* Apply one player action (ACT_*). Returns 1 if a turn was spent, 0 if the
 * action was illegal (e.g. walking into a wall) and nothing changed.
 *
 *   Moves walk over bodies freely.
 *   ACT_DRAG + dir: if a body lies in that direction, push it one tile
 *     ahead; otherwise drag an adjacent body into the tile you leave.
 *   ACT_TAKEDOWN: grab an adjacent standing guard. For TAKEDOWN_TURNS
 *     turns (this one included) the player and target are locked and only
 *     ACT_WAIT is legal; then the guard becomes a body.
 *
 *   ACT_UNIFORM: standing on a body, swap into its uniform.
 *
 *   ACT_TALK: next to a character with a script, run it (messages,
 *     flags). Costs a turn like anything else; it isn't suspicious.
 *     While someone next to you can be talked to, B talks: ACT_TAKEDOWN
 *     is illegal then, and nobody with a script can be taken down.
 *
 * A guard that sees a body (or a takedown in progress) raises the alarm.
 * Takedowns, dragging, throwing and changing clothes are suspicious: even
 * where your outfit is legal, guards catch you doing them in their normal
 * cone. */
uint8_t game_step(GameState *s, uint8_t action);

/* Index of the character ACT_TALK would talk to, or 0xFF. */
uint8_t game_talk_target(const GameState *s);

/* The messages the last action showed, in order: game_said(i) for
 * i < game_said_count() is a message number (see OP_SAY). */
uint8_t game_said_count(void);
uint8_t game_said(uint8_t i);

/* Switch cells are read with these flags. Every action and world_activate
 * set them from the state they are given; call this before asking about
 * the map (game_cell, game_blocks, vision...) for any other state. */
void game_set_flags(uint8_t flags);

/* Index of the guard a takedown would grab, or 0xFF if none. */
uint8_t game_takedown_target(const GameState *s);

/* 1 if ACT_DRAG + dir is legal right now. */
uint8_t game_can_drag(const GameState *s, uint8_t dir);

/* 1 if a body lies on a tile next to the player. */
uint8_t game_body_adjacent(const GameState *s);

/* Is (x, y) a legal target for throwing this item right now?
 * Needs the item in inventory, floor (not wall) at the target, within the
 * item's range in grid steps, not the player's own tile, and a clear line. */
uint8_t game_can_throw(const GameState *s, uint8_t item, uint8_t x, uint8_t y);

/* Throw an item (spends a turn). Every guard within the item's noise range
 * turns to look this turn; the one with the shortest walk goes to the spot,
 * then returns to its route. Returns 0 (nothing happens) if illegal. */
uint8_t game_throw(GameState *s, uint8_t item, uint8_t x, uint8_t y);

/* Grid-step (Manhattan) distance. */
uint8_t game_distance(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1);

/* Map queries for the current level. Outside the map counts as wall. */
uint8_t game_cell(uint8_t x, uint8_t y);
uint8_t game_width(void);
/* Does tile (x, y) block any of the BLOCK_* bits in what? */
uint8_t game_blocks(uint8_t x, uint8_t y, uint8_t what);
uint8_t game_height(void);

/* Can this guard see tile (x, y) with the normal cone (+ line of sight)? */
uint8_t game_guard_sees(const Guard *g, uint8_t x, uint8_t y);

/* ACCESS_* of cell (x, y) for the disguise the player is wearing.
 * Outside the map counts as trespassing. */
uint8_t game_access(const GameState *s, uint8_t x, uint8_t y);

/* How guard idx regards the player where they stand now: ACCESS_*, with
 * its enforcer tag applied. Always ACCESS_LEGAL for one that doesn't
 * watch (CHAR_NO_SIGHT). */
uint8_t game_regard(const GameState *s, uint8_t idx);

/* Vision for the w x h block of tiles starting at (x0, y0), written row by
 * row into vis (w * h bytes): VIS_SEEN where a guard would catch the player
 * in what they're wearing, VIS_WATCHED where a guard is looking but the
 * player is legal (bodies and suspicious acts are still noticed).
 * The Game Boy only asks for the part of the map on screen. */
#define VIS_NONE    0
#define VIS_WATCHED 1
#define VIS_SEEN    2
void game_compute_vision(const GameState *s, uint8_t x0, uint8_t y0,
                         uint8_t w, uint8_t h, uint8_t *vis);

/* Index of the body under the player whose uniform can be taken, or 0xFF. */
uint8_t game_uniform_here(const GameState *s);

/* Undo lives in world.h, since a turn may cross between maps. */

#endif
