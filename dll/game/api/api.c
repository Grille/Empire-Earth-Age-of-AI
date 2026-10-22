// THE GAME'S API: everything the mod knows of Empire Earth's code and objects, one module - eeaimod.c includes this
// file only, the order inside is the module's own. What the rest of the code uses: types.h (the objects typed),
// objects.c (their fields by name), functions.c (the game's functions as wrappers, its data as accessors), hooks.c (the
// hooks' types and installers, the code patches) and the fields' names (UNIT_0x13C, FIELD, layout.h). raw/ is the module's
// inside: the addresses and offsets per exe build (builds.h, functions.h, hooks.h, data.h, fields.h, editor_layout.h)
// and tables.c, which makes them tables and GA, GF, GL - nothing raw/ defines is used outside game/api/, and game/api/
// uses only base/ (scripts/layers.py). exe.c finds the running build (game_build, build_is) and takes its column;
// api_log tells the start's log what is missing.
#include "game/api/types.h"        // the game's objects typed: ee_player *, ee_unit * ...
#include "game/api/raw/builds.h"   // the exe builds the mod knows: the tables' columns
#include "game/api/raw/tables.c"   // the game's addresses and objects' fields per exe build: GA(G_WORLD_CLOCK), PL_0x9D4
#include "game/api/exe.c"          // which build runs: game_build
#include "game/api/raw/editor_layout.h"   // the scenario editor's layout sites (generated, the 2017 update)
#include "game/api/layout.h"       // FIELD: a read at a field's name or a plain number
#include "game/api/objects.c"      // the objects' fields by name: unit_get_x(u), unit_get_hp(u) ...
#include "game/api/functions.c"    // the game's functions over them, its data: explored_by(site, pl)
#include "game/api/hooks.c"        // the game's functions the mod hooks: their hook types and installers
