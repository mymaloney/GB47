/*
 * editor_lib.c - Extra entry points for the level editor, which loads
 * src/game.c + tools/solver.c as a shared library (build/libgame.so) so
 * playtesting uses exactly the same rules as the ROM.
 *
 * The editor mirrors GameState/LevelDef with Python ctypes; editor_info()
 * lets it check that the layouts and constants still match game.h.
 */
#include "../src/game.h"
#include "solver.h"
#include "../src/world.h"

int editor_info(int what)
{
    switch (what) {
    case 0:  return sizeof(GameState);
    case 1:  return sizeof(LevelDef);
    case 2:  return sizeof(GuardDef);
    case 3:  return sizeof(Guard);
    case 4:  return MAP_MAX_W;
    case 5:  return MAP_MAX_H;
    case 6:  return MAX_GUARDS;
    case 7:  return MAX_WAYPOINTS;
    case 8:  return ITEM_TYPES;
    case 9:  return DISGUISE_MAX;
    case 10: return ACT_COUNT;
    case 11: return TAKEDOWN_TURNS;
    case 12: return UNDO_DEPTH;
    case 13: return MAP_MAX_CELLS;
    case 14: return VIEW_W;
    case 15: return VIEW_H;
    case 16: return sizeof(World);
    case 17: return MAX_PARTS;
    case 19: return sizeof(SwitchDef);
    case 21: return MAX_SWITCHES;
    case 22: return MAX_SAID;
    case 23: return sizeof(PickupDef);
    case 24: return sizeof(PortalDef);
    case 25: return MAX_PICKUPS;
    case 26: return MAX_PORTALS;
    default: return -1;
    }
}
