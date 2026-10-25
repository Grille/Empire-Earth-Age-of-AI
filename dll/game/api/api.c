// THE GAME'S API: everything the mod knows of Empire Earth's code and objects, one module - eeaimod.c includes this
// file only, the order inside is the module's own. One file per object of the game (unit.c, player.c, attack.c ...):
// what the rest of the code uses of it - its fields, the game's functions on it (by their first argument, the C++
// object of a thiscall), its hooks; game.c the game's global state and its getters (game_map(), game_players() ...),
// hooks.c the patches of the game's code that are about no object. types.h names the objects' types (one place: they
// point at each other). raw/ is the module's inside: editor_layout.h (the scenario editor's layout sites, generated) - nothing raw/
// defines is used outside game/api/, and game/api/ uses only base/ (scripts/build/layers.py). gen/ is the
// spec's C (spec/game/, made by scripts/build/gen_api.py, never edited): every object's
// fields, functions and hooks in plain declarations - unit_get_speed, unit_max_speed, patch_aim - and their values set
// per build by spec_select. exe.c finds the running build (game_build, build_is) and selects its values; api_log tells the
// start's log what is missing.
//
// FIELDS: kind_get_x(obj) reads a field, kind_set_x(obj, v) writes one, kind_x_at(obj) its address - gen/'s, from the
// spec's classes (spec/game/); a hand-written accessor here where a field is read otherwise (an index, two fields
// gathered), with the generated offset (FIELD(u, of_unit_queue + 0x4, int *)).
// FUNCTIONS: the game's functions as plain C functions over types.h's objects - gen/'s from the spec (its wrappers
// keep the names the C side gave them: unit_distance, explored_by's game function), hand-written here where a call
// does more than pass its arguments on (explored_by(site, pl): the site's visibility record first).
// HOOKS: for each hooked function the type of its hook and the installer. A hook stands in for the game's function at
// its calls (patch_call: the call instruction redirected, only while it still calls that function) or in its class's
// vtable (patch_vtable: only while the slot still holds it); it calls the original through the function's wrapper
// when it wants the game's own work done. The hook types are the game's real arguments - a hook declared with other
// parameters doesn't build (a hook of a method is __thiscall too: this in ECX, as the game calls it). An installer returns how many
// places it patched; of (may be NULL) gets the number there are. Kinds not typed yet are void *. Assembler stubs (they
// read registers) are void (*)(void).
#include "game/api/types.h"        // the game's objects typed: ee_player *, ee_unit * ...
#include "game/api/builds.h"       // the exe builds the mod knows: the spec's builds (ee, aoc)
#include "game/api/gen/values.c"   // the spec's values: offsets, addresses, slots (spec_select sets them)
#include "game/api/exe.c"          // which build runs: game_build
#include "game/api/raw/editor_layout.h"   // the scenario editor's layout sites (generated, the 2017 update)
#include "game/api/layout.h"       // FIELD: a read at a field's offset or a plain number
#include "game/api/patching.c"     // the installers' helpers: patch_calls, PATCH_LIST, site_index
#include "game/api/gen/enums.c"    // the game's enums (spec/game/scenario_lists.cs ...): the spec's C types fields by them
#include "game/api/scenario_lists.c"   // the scenario editor's lists: a player by number, the class filter by build
#include "game/api/gen/gen.c"      // the spec's C (spec/game/, scripts/build/gen_api.py)
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
#include "game/api/movement.c"
#include "game/api/formation.c"
#include "game/api/setup.c"
#include "game/api/messages.c"
#include "game/api/scenario.c"
#include "game/api/ui.c"
#include "game/api/engine_display.c"
#include "game/api/engine_sound.c"
#include "game/api/hooks.c"        // the game's code patched where no object is concerned
