// THE MOD'S OWN MAP TYPES (the player 2026-11-24: "i would circumvent the games own generator fully, put our entries
// directly in the list of available map types and if selected we take over fully"). The game's random maps are a
// script per type (Data\Random Map Scripts\*.rmv: some 110 numbers and placement lists) run by a generator plug-in
// (Default.dll: a fractal height map, land grown round each player's start and round neutral points, resources in
// rings) - a script can't say where land goes, so two continents shared by the players, climates by region or rivers
// are beyond it. The mod's types sit beside the game's in its list of types (random_map.types, what every list of
// the setup screens, the lobby and the editor shows) and a map of one of them is made here whole: the game's
// generator, its plug-in and the scripts are not run for it (hook_random_map_create: the game start's and the
// editor's call of the game's own). The game's types play as before.
// What the game needs of a random map (read from its own, random_map_create): the ground of the new map it hands over,
// each playing player's start units in it (made, built and paid for as its own are), where this PC's view begins - and
// 0 for "made". Nothing else of the start asks for the generator's own object.
// SO FAR: one type, the proof that the way in holds - the map as the game makes a new one (flat), each player a
// Capitol and its citizens on a circle round the middle. In multiplayer every PC needs the same types (the map is made
// on each from the setup and the seed).
#define MAPGEN_TEST "Age of AI Test"
internal int g_mapgen_added;

internal int mapgen_is_ours(const char *name) { return name && !lstrcmpiA(name, MAPGEN_TEST); }

// the mod's types pushed into the game's list, once, when the game has made its own
internal void mapgen_types_add(void) {
    if (g_mapgen_added || !map_types_count()) return;
    g_mapgen_added = 1;
    map_types_add(MAPGEN_TEST, MAPGEN_TEST, "Default");
    if (g_log) log_locked("# mapgen: \"%s\" added to the game's %u map types\n", MAPGEN_TEST, map_types_count() - 1);
}
internal int __cdecl hook_random_map_types_load(void *path) {
    int r = random_map_types_load(path);
    mapgen_types_add();
    return r;
}

// a start unit as the game's random map makes one: the type's unit at a point of the map, built, paid for
internal ee_unit *mapgen_unit(int type, ee_player *pl, float x, float y, ee_map *map) {
    ee_unit_type *t = ut_has(type) ? type_data(type) : NULL;
    ee_unit *u = t ? unit_type_create_at(t, pl, x, y, map, 0.0f) : NULL;
    ee_unit *ph = u ? unit_get_physics(u) : NULL;
    if (!ph) return u;
    unit_set_built(ph, 1, 0, 0);
    unit_set_paid_for(ph, 1);
    return u;
}

// THE TEST TYPE: the new map's ground left as the game made it, the players on a circle round the middle (a quarter
// of the map's side out: inside the diamond at every angle), each its Capitol on a tile corner and its citizens in
// two rows beside it
internal int mapgen_make(void *players, int human, ee_map *map, float *start) {
    ee_player **pb = FIELD(players, 4, ee_player **), **pe = FIELD(players, 8, ee_player **);
    int n = pb ? (int)(pe - pb) : 0, size = (int)map_get_size(map), made = 0;
    float mid = (float)(size / 2), r = (float)size * 0.25f;
    start[0] = start[1] = mid + 0.5f;
    for (int i = 1; i < n; i++) {
        float a = 6.2831853f * (float)(i - 1) / (float)(n - 1);
        float cx = floorf(mid + r * cosf(a)), cy = floorf(mid + r * sinf(a));
        made += mapgen_unit(UT(B_CAPITOL), pb[i], cx, cy, map) != NULL;
        int want = (int)player_get_start_citizens(pb[i]);
        for (int k = 0; k < want && k < 12; k++)
            made += mapgen_unit(k & 1 ? UT(FEMALE_CITIZEN) : UT(CITIZEN), pb[i], cx + 3.5f + (float)(k / 6), cy - 2.5f + (float)(k % 6), map) != NULL;
        if (i == human) { start[0] = cx + 0.5f; start[1] = cy + 0.5f; }
    }
    start[2] = ground_at(map, start[0], start[1]);
    if (g_log) log_locked("# mapgen: \"%s\" made - %d tiles a side, %d players, %d start units, the view from %.1f %.1f %.2f; the map's flags %u, default ground %d\n",
                          MAPGEN_TEST, size, n - 1, made, (double)start[0], (double)start[1], (double)start[2], map_get_flags(map), map_get_default_terrain(map));
    return 0;
}
internal int __cdecl hook_random_map_create(ee_setup *setup, void *players, int human, void *name, uint32_t seed, ee_map *map, float *start) {
    if (!mapgen_is_ours(name ? FIELD(name, 4, const char *) : NULL)) return random_map_create(setup, players, human, name, seed, map, start);
    return mapgen_make(players, human, map, start);
}

// the game's list is made at its start, which may be over when the mod's patches go in: then the types are added now
internal void mapgen_install(void) {
    int of = 0, n = patch_random_map_create(hook_random_map_create, &of), l = patch_random_map_types_load(hook_random_map_types_load);
    mapgen_types_add();
    if (g_log) fprintf(g_log, "# mapgen: the mod's map types - the game's map maker's calls %d/%d, its list's load %s%s\n", n, of, l ? "hooked" : "FAILED",
                       g_mapgen_added ? " (the list was there: added now)" : "");
}
