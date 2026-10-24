// THE GAME'S API: everything the mod knows of Empire Earth's code and objects, one module - eeaimod.c includes this
// file only, the order inside is the module's own. One file per object of the game (unit.c, player.c, attack.c ...):
// what the rest of the code uses of it - its fields, the game's functions on it (by their first argument, the C++
// object of a thiscall), its hooks; game.c the game's global state and its getters (game_map(), game_players() ...),
// hooks.c the patches of the game's code that are about no object. types.h names the objects' types (one place: they
// point at each other). raw/ is the module's inside: the addresses and offsets per exe build (builds.h, functions.h,
// hooks.h, data.h, fields.h, editor_layout.h) and tables.c, which makes them tables, GA, GF, GL and the fields' names -
// nothing raw/ defines is used outside game/api/, and game/api/ uses only base/ (scripts/build/layers.py). exe.c finds the
// running build (game_build, build_is) and takes its column; api_log tells the start's log what is missing.
//
// FIELDS: kind_get_x(obj) reads a field, kind_set_x(obj, v) writes one (only those the mod writes), kind_x_at(obj) its
// address - the offsets per build from raw/fields.h, rows named for what they are (the 2017 update's offset beside
// each, as engine-notes.md and the decompile name them). Inlined: the machine code is FIELD's. Where a field's meaning
// isn't known, its comment says how the code uses it.
// FUNCTIONS: the game's functions as plain C functions over types.h's objects - explored_by(site, pl) instead of
// GF(FN_EXPLORED)(FIELD(site, UNIT_VIS, void *), pl) - and its data as accessors; raw/functions.h has their
// addresses and raw signatures. Each wrapper is the call itself: inlined, the machine code is the same as GF's
// (scripts/build/code_same.py).
// HOOKS: for each hooked function the type of its hook and the installer. A hook stands in for the game's function at
// its calls (patch_call: the call instruction redirected, only while it still calls that function) or in its class's
// vtable (patch_vtable: only while the slot still holds it); it calls the original through the function's wrapper
// when it wants the game's own work done. The hook types are the game's real arguments - a hook declared with other
// parameters doesn't build (a hook of a method is __thiscall too: this in ECX, as the game calls it). An installer returns how many
// places it patched; of (may be NULL) gets the number there are. Kinds not typed yet are void *. Assembler stubs (they
// read registers) are void (*)(void).
#include "game/api/types.h"        // the game's objects typed: ee_player *, ee_unit * ...
#include "game/api/raw/builds.h"   // the exe builds the mod knows: the tables' columns
#include "game/api/raw/tables.c"   // the game's addresses and objects' fields per exe build: GA(G_WORLD_CLOCK), UNIT_HP
#include "game/api/exe.c"          // which build runs: game_build
#include "game/api/raw/editor_layout.h"   // the scenario editor's layout sites (generated, the 2017 update)
#include "game/api/layout.h"       // FIELD: a read at a field's offset or a plain number
#include "game/api/patching.c"     // the installers' helpers: patch_calls, PATCH_LIST, site_index
#include "game/api/game.c"         // the game's globals, memory, strings, actions, simulation, run
#include "game/api/unit.c"
#include "game/api/unit_type.c"
#include "game/api/player.c"
#include "game/api/goal.c"
#include "game/api/tech.c"
#include "game/api/zone.c"
#include "game/api/ai_managers.c"
#include "game/api/attack.c"
#include "game/api/map.c"
#include "game/api/path.c"
#include "game/api/setup.c"
#include "game/api/messages.c"
#include "game/api/scenario.c"
#include "game/api/ui.c"
#include "game/api/engine_display.c"
#include "game/api/engine_sound.c"
#include "game/api/hooks.c"        // the game's code patched where no object is concerned
