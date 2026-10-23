// THE TABLES BEHIND GA, GF, GL AND THE FIELDS' NAMES - game/api/'s own: the rest of the code uses the accessors, wrappers and
// installers of the module's object files (unit.c, player.c ...), and FIELD (layout.h; scripts/build/layers.py). Every address the mod uses in
// Empire Earth.exe and every field offset it reads, by name, with its value in each exe build the mod knows - one
// column per k_exe_builds entry (builds.h), in that order. game_build (exe.c) finds the running build, addr_select and
// fields_select take its column when the DLL loads: GA(NAME) is the address, GF(NAME) the function there, typed by its
// row, GL(NAME) a list of addresses (its count: sizeof GL(NAME) / sizeof *GL(NAME)), a field's name (UNIT_HP) its
// offset. Another build - Art of Conquest, another release, a patched exe - is one more hash and one more value per
// row (scripts/re/port_addrs.py, port_fields.py).
// The address names: FN_ a function, CALL_ a call (patched), SITE_/CODE_ an instruction or operand, G_ data, VT_ a
// vtable, VTS_ a vtable slot's address, SLOT_ a vtable slot's index (a class may have gained a virtual function),
// SIZE_ an object's size, IAT_ an import slot, RVA_ a place in an engine DLL (0: the build hasn't got it). Tables of
// their own for one build: editor_layout.h (generated for the 2017 update: hooks.c apply_editor_layout only there).

// The address rows by who uses them (scripts/build/addr_sort.py keeps them there, the build checks it): functions.h what
// the module calls, hooks.h what it patches (the files' HOOKS, hooks.c), data.h the game's data it reads; one table of the
// three. F(name, return type, calling convention, (parameters), value per build): a function; D(name, value per
// build): an address or a number; L(name, count, (values of build 1), (values of build 2) ...): a list.
#include "game/api/raw/functions.h"   // the rows of the functions the mod calls
#include "game/api/raw/hooks.h"        // the rows of what it patches to hook the game
#include "game/api/raw/data.h"         // the rows of the game's data
#include "game/api/raw/fields.h"       // the rows of the objects' fields

#define ADDRS(F, D) ADDRS_FUNCTIONS(F, D) ADDRS_HOOKS(F, D) ADDRS_DATA(F, D)

// numbers that aren't addresses: SLOT_ a virtual function's index in its class's vtable (patch_vtable, a call through
// the vtable; slot 0, the deleting destructor in both builds, stays a plain 0), SIZE_ the size of a game object the
// mod makes itself (FN_NEW, then its constructor), RVA_ a place in an engine DLL relative to its base - each build
// ships its own DLLs (scripts/re/port_dll.py); what the DLLs export is taken by name instead. A patch there checks the
// bytes first: a DLL from elsewhere is left alone.
#define ADDR_NUMBERS(D) ADDR_NUMBERS_FUNCTIONS(D) ADDR_NUMBERS_HOOKS(D)

#define ADDR_LISTS(L) ADDR_LISTS_FUNCTIONS(L) ADDR_LISTS_HOOKS(L)

#define ADDR_ENUM_F(n, r, cc, a, ...) A_##n,
#define ADDR_ENUM_D(n, ...) A_##n,
enum addr_name { ADDRS(ADDR_ENUM_F, ADDR_ENUM_D) ADDR_NUMBERS(ADDR_ENUM_D) A_N };
#define ADDR_TYPE_F(n, r, cc, a, ...) typedef r (cc *n##_fn) a;
#define ADDR_TYPE_D(n, ...)
ADDRS(ADDR_TYPE_F, ADDR_TYPE_D)
#define ADDR_ROW_F(n, r, cc, a, ...) { __VA_ARGS__ },
#define ADDR_ROW_D(n, ...) { __VA_ARGS__ },
internal const uint32_t k_addr[A_N][ADDR_BUILDS] = { ADDRS(ADDR_ROW_F, ADDR_ROW_D) ADDR_NUMBERS(ADDR_ROW_D) };
#define ADDR_NAME_F(n, r, cc, a, ...) #n,
#define ADDR_NAME_D(n, ...) #n,
internal const char *const k_addr_name[A_N] = { ADDRS(ADDR_NAME_F, ADDR_NAME_D) ADDR_NUMBERS(ADDR_NAME_D) };
#define ADDR_UNPAREN(...) { __VA_ARGS__ }
#define ADDR_LIST_COL(v) ADDR_UNPAREN v,
#define ADDR_LIST(n, k, ...) static const uint32_t k_addr_list_##n[ADDR_BUILDS][k] = { ADDR_LIST_EACH(__VA_ARGS__) };
#define ADDR_LIST_EACH(v1, v2) ADDR_LIST_COL(v1) ADDR_LIST_COL(v2)   // one per build
ADDR_LISTS(ADDR_LIST)

internal uintptr_t g_addr[A_N];
#define GA(n) (g_addr[A_##n])
#define GF(n) ((n##_fn)g_addr[A_##n])
#define GL(n) (k_addr_list_##n[g_exe_build])
internal int g_addr_missing;               // names the running build hasn't got (logged at the start: addr_log)

// the running exe's column (g_exe_build: game_build) into g_addr; the names it hasn't got (0) are counted - logged at
// the start (addr_log)
internal int addr_select(void) {
    int missing = 0;
    for (int i = 0; i < A_N; i++) missing += !(g_addr[i] = k_addr[i][g_exe_build]);
    image_ranges();
    return missing;
}
internal void addr_log(FILE *log) {   // the names the running build hasn't got: what uses them doesn't work there
    if (!log || !g_addr_missing) return;
    fprintf(log, "# addresses: build %d hasn't got %d of %d:", g_exe_build, g_addr_missing, A_N);
    for (int i = 0; i < A_N; i++) if (!g_addr[i]) fprintf(log, " %s", k_addr_name[i]);
    fputc(10, log);
}

// A field's name is the running build's offset itself, a variable per row: FIELD(t, TECH_AVAILABLE, uint8_t),
// FIELD(rm, RM_DEMAND + r * 4, int).
#define OF_VAR(n, ...) internal uint32_t n;
FIELDS(OF_VAR)
#define OF_ENUM(n, ...) OF_##n,
enum field_name { FIELDS(OF_ENUM) OF_N };
#define OF_ROW(n, ...) { __VA_ARGS__ },
internal const uint32_t k_of[OF_N][ADDR_BUILDS] = { FIELDS(OF_ROW) };
#define OF_NAME(n, ...) #n,
internal const char *const k_of_name[OF_N] = { FIELDS(OF_NAME) };
#define OF_PTR(n, ...) &n,
internal uint32_t *const k_of_var[OF_N] = { FIELDS(OF_PTR) };
internal int g_of_missing;   // fields the running build hasn't got (NA): logged at the start (fields_log)
internal int fields_select(void) {   // the running exe's column (g_exe_build) into the fields (with addr_select)
    int missing = 0;
    for (int i = 0; i < OF_N; i++) missing += (*k_of_var[i] = k_of[i][g_exe_build]) == NA;
    return missing;
}
internal void fields_log(FILE *log) {
    if (!log || !g_of_missing) return;
    fprintf(log, "# fields: build %d hasn't got %d of %d:", g_exe_build, g_of_missing, OF_N);
    for (int i = 0; i < OF_N; i++) if (*k_of_var[i] == NA) fprintf(log, " %s", k_of_name[i]);
    fputc(10, log);
}
