// THE GAME'S API: everything the mod knows of Empire Earth's code and objects, one module - eeaimod.c includes this
// file only, the order inside is the module's own. What the rest of the code uses: functions.c (the objects typed, the
// game's functions as wrappers, its data as accessors), hooks.c (the hooks' types and installers, the code patches)
// and the fields' names (OF, FIELD, layout.h). raw/ is the module's inside: the addresses and offsets per exe build
// (builds.h, functions.h, hooks.h, data.h, fields.h, editor_layout.h) and tables.c, which makes them tables and GA,
// GF, GL - nothing raw/ defines is used outside game/api/, and game/api/ uses only base/ (scripts/layers.py). exe.c
// finds the running build (game_build, build_is) and takes its column; api_log tells the start's log what is missing.
#include "game/api/raw/builds.h"   // the exe builds the mod knows: the tables' columns
#include "game/api/raw/tables.c"   // the game's addresses and objects' fields per exe build: GA(G_WORLD_CLOCK), OF(PL, 0x9D4)
#include "game/api/exe.c"          // which build runs: game_build
#include "game/api/raw/editor_layout.h"   // the scenario editor's layout sites (generated, the 2017 update)
#include "game/api/layout.h"       // names for some fields: PL_INDEX, RM_DEMAND ...
#include "game/api/functions.c"    // the objects typed and the functions over them: explored_by(site, pl)
#include "game/api/hooks.c"        // the game's functions the mod hooks: their hook types and installers
