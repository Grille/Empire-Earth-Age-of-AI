// Unit type (object) data 0x9418A0[type]: 0x300 slots in the 2017 update (the game's own loops end at 0x9424A0; type
// ids 0..723 are used, the slots after them are empty), 0x367 in Art of Conquest (its new types from 768 on - the
// space units among them: until 2026-11-23 the mod read 0x300 there and knew none of them); past the table other data
// (AoC's ends at 0x964498, a float of the display's). Every read goes through here: NULL outside the build's table
// (type_table_count) and for an empty slot. G_TYPES_N: the arrays by type, room for the larger of the two.
#define G_TYPES_N 0x380
// Tech ids (the tech tree's lookup, 0x4019A1): the 2017 update's end at 13xxx, Art of Conquest's at 15038 (its Space
// Age's 15001-15038 - until 2026-11-24 the mod's loops stopped at 15000 and its AIs researched none of them: the
// player's game, "all the space age technologies arent getting researched"). G_TECH_IDS: every loop and array by id.
#define G_TECH_IDS 16384
internal inline ee_unit_type *type_data(int t) { return t >= 0 && t < type_table_count() ? type_table()[t] : NULL; }
// MAY IT BUILD ONE (the player 2026-11-19, a campaign: "the ai places some buildings even if its not allowed to"): the
// game's own question as the player's build click asks it (player_can_have of the type's tech, as can_build does) but
// for the cost - a scenario's disabled building, an epoch not reached, no unit of its that builds it, a type's
// population limit. Cost aside: a foundation is paid as it is laid, and the mod lays some when the money is there
internal inline int may_build(ee_player *pl, int type) {
    ee_unit_type *td = pl && type > 0 ? type_data(type) : NULL;
    int tech = td ? unit_type_get_tech(td) : 0;
    return td && tech > 0 && tech != 100000 && player_can_have(pl, tech, 0x20, 1, 0, 3, 0);
}
// MAY THE AI LAY ONE: may_build, and the scenario lets this computer player build - its AI toggle Build/Repair (the
// editor's player dialog, the trigger Set AI Toggle; the player 2026-11-19: an AI meant to build nothing built in a
// campaign - the toggle was asked by build.c, creep.c and expand.c only, docks, harbour towers, defences, walls,
// bunkers, landings, granaries and the build queue laid theirs regardless)
internal inline int ai_may_lay(ee_player *pl, int type) { return pl && player_ai_toggle(pl, SM_AIT_BUILD_REPAIR) && may_build(pl, type); }
// told of a foundation the mod lays itself (walls.c walls_laid: what the game's own placings get from its hook - a
// leaning type stood onto the ground, a tower's tiles noted for the walls' joins). Without it the mod's AI's AA missile
// towers stood level on a hill, half in it (the player, 2026-11-20: "la rocket towers arent fittet to terrain like the 88
// guns are")
internal void (*g_foundation_laid)(ee_unit *b, int type);
// a foundation of type laid out at x, y for pl when its AI may lay one (ai_may_lay) - every building the mod's AI lays
// itself goes through here; NULL: none. An Airport only where its whole strip fits (game/api/player.c airport_fits:
// the game's own placing tests its hangar alone, the player's preview every runway piece too - the player
// 2026-11-24: "placing that foundation should be a hard refusal like the player gets")
internal inline ee_unit *lay_foundation(ee_player *pl, int type, int x, int y) {
    if (is_airport_type(type) && !airport_fits(pl, x, y)) return NULL;
    ee_unit *b = ai_may_lay(pl, type) ? place_foundation(pl, type, x, y) : NULL;
    if (b && g_foundation_laid) g_foundation_laid(b, type);
    return b;
}
// a building of type laid out at x, y (0, 0: a spot of its own) by pl's build manager when its AI may lay one
// (ai_may_lay) - every placement of the build manager's the mod asks for or lets through (econ.c hook_place) goes
// through here; 0: refused. An Airport at a spot only where its whole strip fits (lay_foundation); at a spot of the
// game's own (0, 0) its computer player's search tests the whole strip itself (0x466xxx)
internal inline char lay_queued(ee_build_mgr *bm, int type, int x, int y) {
    ee_player *pl = build_mgr_get_player(bm);
    if ((x || y) && is_airport_type(type) && !airport_fits(pl, x, y)) return 0;
    return ai_may_lay(pl, type) ? build_mgr_place(bm, type, x, y) : 0;
}
// a wall of type from a to b ordered from pl's wall manager when its AI may lay one (ai_may_lay) - every wall the mod's
// AI orders goes through here; 0: refused. THE ONE WAY IN: lay_foundation, lay_queued, lay_wall are the mod's AI's
// only ways to the game's laying (scripts/build/style.py refuses place_foundation, build_mgr_place, build_wall in ai/)
internal inline int lay_wall(ee_player *pl, void *wall_mgr, int *a, int *b, int type) {
    if (!ai_may_lay(pl, type)) return 0;
    build_wall(wall_mgr, a, b, type);
    return 1;
}

internal uint32_t game_time(void) {
    ee_physics *clock = game_physics();
    return clock ? physics_get_time(clock) : 0;
}

// WHAT A UNIT TAKES ON BY ITSELF: the auto-target search's candidate test (unit_functions.target_ok - a unit's idle scan,
// an attack-move's look) with rules that strike a candidate out, from parts that can't call each other (ai/spoils.c:
// a resigned player's units take no building; engine/groupmove.c: an attack-move formation on its march takes no
// building that can't attack). One hook over the test's call sites, the rules asked in turn
typedef int (*target_skip_fn)(ee_unit *searcher, ee_player *pl, ee_unit *cand);
#define TARGET_SKIPS 4
internal target_skip_fn g_target_skips[TARGET_SKIPS];
internal int g_target_skip_n, g_target_hooked, g_target_sites;
internal char __thiscall hook_target_ok(ee_target_search *s, ee_unit *u) {
    char ok = target_ok(s, u);
    if (!ok) return ok;
    ee_unit *me = target_search_get_unit(s);
    ee_player *pl = target_search_get_player(s);
    for (int i = 0; i < g_target_skip_n; i++)
        if (g_target_skips[i](me, pl, u)) return 0;
    return ok;
}
// a rule added (at install: once per process); the hook put in with the first. Its call sites patched, *of of them
internal int target_skip_add(target_skip_fn f, int *of) {
    int known = 0;
    for (int i = 0; i < g_target_skip_n; i++) known |= g_target_skips[i] == f;
    if (!known && g_target_skip_n < TARGET_SKIPS) g_target_skips[g_target_skip_n++] = f;
    if (!g_target_sites) g_target_hooked = patch_target_ok(hook_target_ok, &g_target_sites);
    if (of) *of = g_target_sites;
    return g_target_hooked;
}
// games begun (eeaimod.c's new game counts them): what the render thread keeps of one game's map - the ground's heights
// (engine/collect_ground.c), the bent objects (render_units.c) - made anew after it
internal volatile LONG g_game_starts;

internal ee_player *resource_mgr_player(ee_resource_mgr *rm) {
    ee_civ_mgr *owner = resource_mgr_get_owner(rm);
    return owner ? civ_mgr_get_player(owner) : NULL;
}

// RTTI class name of a polymorphic object (MSVC: vtable[-1] -> complete object locator, +0xC type
// descriptor, name at +8 like ".?AVEEUGGather@@").
internal const char *rtti_name(void *obj) {
    if (!obj) return "none";
    uint8_t *col = ((uint8_t **)*(void **)obj)[-1];
    const char *n = *(const char **)(col + 0xC) + 8;
    return n[0] == '.' && n[1] == '?' && n[2] == 'A' ? n + 4 : n;
}

// An MSVC std::deque (the runtime library's, the same in both builds): +4 its map of blocks, +8 the map's size, +0xC
// the front element's index, +0x10 the count - its elements' size sets how many a block holds.
internal inline unsigned deque_map_size(const void *dq) { return FIELD(dq, 8, unsigned); }
internal inline unsigned deque_front(const void *dq) { return FIELD(dq, 0xC, unsigned); }
internal inline unsigned deque_count(const void *dq) { return FIELD(dq, 0x10, unsigned); }

// Element i of an MSVC std::deque<T*> (block of 1024 pointers; container +4 map, +8 map size), as the
// game's iterator deref 0x5F1BC6 computes it.
internal void *deque_at(uint8_t *dq, unsigned off) {
    unsigned blk = off >> 10, mapsz = deque_map_size(dq);
    if (blk >= mapsz) blk -= mapsz;
    return FIELD(dq, 4, void ***)[blk][off & 1023];
}
