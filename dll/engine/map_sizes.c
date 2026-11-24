// MAP SIZES PAST GIGANTIC ([general] map_sizes; the player 2026-11-24: "would it be possible to add 300 400 and 500?").
// The game's map size is a scale, 0 Tiny .. 5 Gigantic (setup.map_scale), that setup.map_dims turns into the new map's
// sides: 42, 58, 75, 105, 160, 220 - the map a diamond in a square of twice that (Gigantic 440 tiles a side). Three
// scales more, 6 Enormous 300, 7 Colossal 400, 8 Titanic 500 (1000 tiles a side): map_dims's three calls answered for
// them here, the game's lists of sizes made longer (the setup screen's and the lobby's, the editor's map panel - their
// names the language file's strings from map_scale_text on, the new ones the mod's words: game_strings.c) and the setup
// copy letting them through (it made any scale past Gigantic Small). Two places of the game take their numbers from the
// map's width and had none past Gigantic: the map's script (IsGiganticMap - none at all, an assert) and the pathing
// sectors (Large's 15 tiles): both give a wider map Gigantic's (patch_map_scales). The mod's own map types
// (mapgen.c) are made for any size; the game's own play Gigantic's script for now, its numbers unscaled.
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

internal void map_sizes_install(void) {
    if (!g_set.map_sizes) {
        if (g_log) fprintf(g_log, "# map sizes: map_sizes=0 - the game's six\n");
        return;
    }
    if (setup_map_scales() != MS_FIRST) {
        if (g_log) fprintf(g_log, "# map sizes: the game has %u sizes, not %d - left alone\n", setup_map_scales(), MS_FIRST);
        return;
    }
    int of = 0, d = patch_map_dims(hook_map_dims, &of), p = patch_map_scales(MS_FIRST + MS_N), t = 0;
    for (int i = 0; i < MS_N; i++) t += game_string_add(setup_map_scale_text() + MS_FIRST + i, k_ms[i].word);
    if (g_log) fprintf(g_log, "# map sizes: %d more (to %d) - map_dims's calls %d/%d, %d/%d patches, %d/%d names%s\n", MS_N, k_ms[MS_N - 1].side,
                       d, of, p, MAP_SCALES_PATCHES, t, MS_N, d == of && of && p == MAP_SCALES_PATCHES && t == MS_N ? "" : " (FAILED)");
}
