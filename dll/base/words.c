// THE MOD'S WORDS: what it writes on the game's screens (the editor's boxes and dialogs, the diplomacy screen) in the
// game's language: language.eeai beside ageofai.ini, a record per word - vassal = { en = "Vassal", de = "Vasall" }. Read
// by voice/texts.c (the language's parser is above base/), kept here for the parts that may not use it (engine/). en is
// a must, any other language may be left out: English then. k_words: every word the mod asks for - a word of the file
// that isn't here, or one here that the file lacks, is a mistake, told when it is read.
internal const char *const k_words[] = {
    "vassal", "vassal_of",                                                 // the diplomacy screen (mod/system.eeai)
    "sanity_check", "rows", "columns",                                     // editor_place.c
    "formation", "form_rotate", "form_short_line", "form_long_line", "form_loose_line", "form_staggered_line",
    "form_deep_wedge", "form_shallow_wedge", "form_inward_circle", "form_outward_circle",
    "cliff_rock_1", "cliff_rock_2", "cliff_color", "water_color",          // terrain_cliffs.c
    "min_height", "max_height", "min_slope", "max_slope", "water_at",      // terrain_filter.c
    "cliffs", "cliffs_any", "cliffs_none", "cliffs_only",
    "ok", "cancel", "delete", "colour_new", "colour_edit", "colour_flat",  // terrain_dialog.c
    "colour_ramp", "colour_each", "blend", "blend_srgb", "blend_linear", "blend_oklab", "slots", "recent",
    "slots_taken", "slots_short", "hint_ramp", "hint_each", "hint_flat",
};
#define N_WORDS (int)(sizeof k_words / sizeof *k_words)
internal char *g_word[N_WORDS];
internal wchar_t *g_word_w[N_WORDS];

internal int word_index(const char *key) {
    for (int i = 0; i < N_WORDS; i++) if (!lstrcmpiA(k_words[i], key)) return i;
    return -1;
}
internal void word_set(int i, const char *text) {
    free(g_word[i]);
    g_word[i] = _strdup(text);
    free(g_word_w[i]);
    g_word_w[i] = NULL;
}
// a word in the game's language (its key when the file has none - told when it was read)
internal const char *word(const char *key) {
    int i = word_index(key);
    return i >= 0 && g_word[i] ? g_word[i] : key;
}
// the same for a Win32 control's text; made once, kept till the word changes
internal const wchar_t *word_w(const char *key) {
    int i = word_index(key);
    if (i < 0) return L"?";
    if (!g_word_w[i]) {
        const char *t = word(key);
        int n = MultiByteToWideChar(CP_ACP, 0, t, -1, NULL, 0);
        wchar_t *w = n > 0 ? malloc((size_t)n * sizeof *w) : NULL;
        if (w) MultiByteToWideChar(CP_ACP, 0, t, -1, w, n);
        g_word_w[i] = w;
    }
    return g_word_w[i] ? g_word_w[i] : L"";
}
// out (cap wide characters) = the word with {1} and {2} written as these numbers
internal void word_fill_w(wchar_t *out, int cap, const char *key, unsigned a, unsigned b) {
    int o = 0;
    for (const wchar_t *s = word_w(key); *s && o < cap - 12; s++) {
        if (s[0] == L'{' && (s[1] == L'1' || s[1] == L'2') && s[2] == L'}') {
            o += wsprintfW(out + o, L"%u", s[1] == L'1' ? a : b);
            s += 2;
        } else out[o++] = *s;
    }
    out[o] = 0;
}
