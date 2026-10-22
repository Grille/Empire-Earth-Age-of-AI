// THE GAME'S ADDRESSES: every address the mod uses in Empire Earth.exe, by name, with its value in each exe build the
// mod knows - one column per k_exe_builds entry (game/exe.c), in that order (0: the build hasn't got it). game_build
// finds the running build, addr_select takes its column when the DLL loads: GA(NAME) is the address, GF(NAME) the
// function there, typed by its row, GL(NAME) a list of addresses (its count: sizeof GL(NAME) / sizeof *GL(NAME)).
// Another build - Art of Conquest, another release, a patched exe - is one more hash and one more value per row. The names: FN_ a function,
// CALL_ a call (patched), SITE_/CODE_ an instruction or operand, G_ data, VT_ a vtable, VTS_ a vtable slot's address,
// SLOT_ a vtable slot's index (a class may have gained a virtual function), SIZE_ an object's size, IAT_ an import
// slot, RVA_ a place in an engine DLL. Tables of their own for one build: editor_layout.h (generated for the 2017
// update: apply_editor_layout only there). The objects' fields: fields.h, the same columns.
// Art of Conquest's column: scripts/port_addrs.py (the two exes' code paired; its HAND table the few found by hand,
// each with its reason).

// The rows by who uses them (scripts/addr_sort.py keeps them there, the build checks it): functions_addr.h what
// functions.h calls, hooks_addr.h what hooks.h patches, data_addr.h the game's data functions.h reads. One table: the
// three composed here. F(name, return type, calling convention, (parameters), value per build): a function;
// D(name, value per build). Only game/api/ uses GA, GF and GL (scripts/layers.py).
#include "game/api/functions_addr.h"   // the rows of the functions the mod calls
#include "game/api/hooks_addr.h"       // the rows of what it patches to hook the game
#include "game/api/data_addr.h"        // the rows of the game's data

#define ADDRS(F, D) ADDRS_FUNCTIONS(F, D) ADDRS_HOOKS(F, D) ADDRS_DATA(F, D)

// numbers that aren't addresses: SLOT_ a virtual function's index in its class's vtable (patch_vtable, a call through
// the vtable; slot 0, the deleting destructor in both builds, stays a plain 0), SIZE_ the size of a game object the
// mod makes itself (FN_NEW, then its constructor), RVA_ a place in an engine DLL relative to its base - each build
// ships its own DLLs (scripts/port_dll.py); what the DLLs export is taken by name instead. A patch there checks the
// bytes first: a DLL from elsewhere is left alone.
#define ADDR_NUMBERS(D) ADDR_NUMBERS_FUNCTIONS(D) ADDR_NUMBERS_HOOKS(D)

// lists: L(name, count, (values of build 1), (values of build 2) ...)
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
