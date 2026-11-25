// MAP SIZES PAST GIGANTIC ([general] map_sizes; the player 2026-11-24: "would it be possible to add 300 400 and 500?").
// The game's map size is a scale, 0 Tiny .. 5 Gigantic (setup.map_scale), that setup.map_dims turns into the new map's
// sides: 42, 58, 75, 105, 160, 220 - the map a diamond in a square of twice that (Gigantic 440 tiles a side). Three
// scales more, 6 Enormous 300, 7 Colossal 400, 8 Titanic 500 (1000 tiles a side): map_dims's three calls answered for
// them here, the game's lists of sizes made longer (the setup screen's and the lobby's, the editor's map panel - their
// names the language file's strings from map_scale_text on, the new ones the mod's words: game_strings.c) and the setup
// copy letting them through (it made any scale past Gigantic Small). Two places of the game take their numbers from the
// map's width and had none past Gigantic: the map's script (IsGiganticMap - none at all, an assert) and the pathing
// sectors (Large's 15 tiles): both give a wider map Gigantic's (patch_map_scales). The mod's own map types
// (mapgen.c) are made for any size. The game's own play Gigantic's script, its numbers in tiles and counts scaled to
// the map (hook_random_map_generate, the player 2026-11-25: "extrapolating the gigantic map size script"): most of a
// script's values are shares already - of the land, of the map's radius - and grow with the map by themselves; what
// is in tiles grows with the side (the water along the edge, the starts' push from the middle and from the edge, the
// water between the islands), by the area the neutral lands (their land is a share, split among them: as many as
// Gigantic's to an area its size) and what is laid on them (ResourcePerNeutralLand is how many in all, not each -
// else 5 times the land with Gigantic's few mines on it), and the land growth's rounds (their square). What is round
// a player's start stays (its resources' rings, forests, herds): a start is a start on any map. The neutral lands'
// herds (RandomHerdPerNeutral, 2 in six scripts) stay as they are: a list of herds, not a number.
// Nothing else of the game reads the scale: a map, a save, the network know the map by its sides. The mod's own
// whole-map tables are made for maps up to 1024 tiles a side (the player's planets).
#define MS_FIRST 6   // the first of the mod's scales: setup.map_scales of the game
internal const struct { int side; const char *word; } k_ms[] = {
    { 300, "map_size_enormous" }, { 400, "map_size_colossal" }, { 500, "map_size_titanic" },
};
#define MS_N (int)(sizeof k_ms / sizeof *k_ms)

internal void __thiscall hook_map_dims(ee_setup *setup, int *w, int *h) {
    unsigned i = (unsigned)setup_get_map_scale(setup) - setup_map_scales();
    if (i < (unsigned)MS_N) {
        *w = *h = k_ms[i].side;
        return;
    }
    setup_map_dims(setup, w, h);
}

// a scale's map: its tiles a side, 0 none - map_dims (the game's and the mod's) asked with a setup of that scale
// alone: the game's reads nothing else of it (0x524E1F)
internal int map_scale_tiles(int scale) {
    static uint8_t setup[0x1000];   // room for a setup up to its map_scale (+0x3DC in both builds)
    int w = 0, h = 0;
    setup_set_map_scale((ee_setup *)setup, scale);
    hook_map_dims((ee_setup *)setup, &w, &h);
    return w + h;
}

// the script's numbers of a map wider than Gigantic scaled to its side before the map is made from them (each try
// of random_map_create reads the script anew)
#define MS_GIGANTIC 440   // Gigantic's side as the plug-in sees it (random_map_get_side)
internal int ms_scale(int v, double f) { return v > 0 ? (int)(v * f + 0.5) : v; }
internal int __thiscall hook_random_map_generate(ee_random_map *rm) {
    int side = random_map_get_side(rm);
    if (side > MS_GIGANTIC) {
        double s = (double)side / MS_GIGANTIC, a = s * s;
        int wb = random_map_get_water_border(rm), op = random_map_get_outer_push(rm), in = random_map_get_inner_neutrals(rm),
            on = random_map_get_outer_neutrals(rm);
        random_map_set_water_border(rm, ms_scale(wb, s));
        random_map_set_outer_push(rm, ms_scale(op, s));
        random_map_set_inner_push(rm, ms_scale(random_map_get_inner_push(rm), s));
        random_map_set_start_to_edge(rm, ms_scale(random_map_get_start_to_edge(rm), s));
        random_map_set_river_width(rm, ms_scale(random_map_get_river_width(rm), s));
        random_map_set_inner_neutrals(rm, ms_scale(in, a));
        random_map_set_outer_neutrals(rm, ms_scale(on, a));
        random_map_set_clump_growth(rm, ms_scale(random_map_get_clump_growth(rm), s));
        int gold = random_map_get_neutral_resources(rm, 1);
        for (int r = 0; r < 7; r++) random_map_set_neutral_resources(rm, r, ms_scale(random_map_get_neutral_resources(rm, r), a));
        if (g_log) fprintf(g_log, "# map sizes: side %d, the script's numbers x%.2f (area x%.2f) - water border %d -> %d, push %d -> %d,"
                           " neutral lands %d+%d -> %d+%d (their gold %d -> %d), river %d\n", side, s, a, wb, random_map_get_water_border(rm),
                           op, random_map_get_outer_push(rm), in, on, random_map_get_inner_neutrals(rm), random_map_get_outer_neutrals(rm),
                           gold, random_map_get_neutral_resources(rm, 1), random_map_get_river_width(rm));
    }
    return random_map_generate(rm);
}

internal void map_sizes_install(void) {
    if (!g_set.map_sizes) {
        if (g_log) fprintf(g_log, "# map sizes: map_sizes=0 - the game's six\n");
        return;
    }
    if (setup_map_scales() != MS_FIRST) {
        if (g_log) fprintf(g_log, "# map sizes: the game has %u sizes, not %d - left alone\n", setup_map_scales(), MS_FIRST);
        return;
    }
    int of = 0, d = patch_map_dims(hook_map_dims, &of), p = patch_map_scales(MS_FIRST + MS_N), t = 0,
        g = patch_random_map_generate(hook_random_map_generate);
    for (int i = 0; i < MS_N; i++) t += game_string_add(setup_map_scale_text() + MS_FIRST + i, k_ms[i].word);
    if (g_log) fprintf(g_log, "# map sizes: %d more (to %d) - map_dims's calls %d/%d, %d/%d patches, %d/%d names, the scripts' scaling %s%s\n", MS_N,
                       k_ms[MS_N - 1].side, d, of, p, MAP_SCALES_PATCHES, t, MS_N, g ? "in" : "NOT in",
                       d == of && of && p == MAP_SCALES_PATCHES && t == MS_N && g ? "" : " (FAILED)");
}
