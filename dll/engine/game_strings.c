// THE MOD'S STRINGS IN THE GAME'S LANGUAGE LOOKUP (UResourceFile::GetString, an import of the exe the game asks for
// every text of its language file by number): a number the file doesn't have answered with one of the mod's words
// (language.eeai, base/words.c) - the editor's rock cliffs (terrain_cliffs.c), the map sizes past Gigantic
// (map_sizes.c). One hook for all of them: the import slot takes only one.
#define GS_MAX 16
internal struct { int id; const char *word; } g_gs[GS_MAX];
internal int g_gs_n, g_gs_hooked;   // g_gs_hooked: 0 not yet, 1 in, -1 failed
internal resource_string_fn g_gs_orig;

internal int __thiscall hook_game_string(void *res, int id, void *s) {
    for (int i = 0; i < g_gs_n; i++)
        if (g_gs[i].id == id) {
            __attribute__((aligned(8))) uint8_t w[32];
            wstring_init_text(w, word(g_gs[i].word));
            wstring_copy(s, w);
            wstring_free(w);
            return 0;
        }
    return g_gs_orig(res, id, s);
}

// the language's string `id` the mod's word `word` from now on: 1 done, 0 not (the hook didn't go in, no room)
internal int game_string_add(int id, const char *word_key) {
    if (!g_gs_hooked) g_gs_hooked = patch_resource_string(hook_game_string, &g_gs_orig) ? 1 : -1;
    if (g_gs_hooked < 0 || g_gs_n >= GS_MAX) return 0;
    for (int i = 0; i < g_gs_n; i++) if (g_gs[i].id == id) { g_gs[i].word = word_key; return 1; }
    g_gs[g_gs_n].id = id;
    g_gs[g_gs_n++].word = word_key;
    return 1;
}
