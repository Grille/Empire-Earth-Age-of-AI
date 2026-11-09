// The textures whose art is wrong, by name: <the settings' folder>\\texfix.eeai (game_script/texfix.c reads it, engine/render.c
// repairs them as they are loaded).
#define TF_NAMES 64
internal char g_tf_names[TF_NAMES][64];
internal int g_tf_n;
internal void texfix_add(const char *name) { if (g_tf_n < TF_NAMES) lstrcpynA(g_tf_names[g_tf_n++], name, sizeof *g_tf_names); }
// name: as the engine gives it, textures\bld_temple_04T - Art of Conquest's with its .sst
internal int texfix_listed(const char *name) {
    if (!name) return 0;
    const char *slash = strrchr(name, '\\');
    if (slash) name = slash + 1;
    const char *dot = strrchr(name, '.');
    size_t n = dot ? (size_t)(dot - name) : strlen(name);
    for (int i = 0; i < g_tf_n; i++) if (strlen(g_tf_names[i]) == n && !_strnicmp(name, g_tf_names[i], n)) return 1;
    return 0;
}
