// The textures whose art is wrong, by name: <the settings' folder>\\texfix.eeai (game_script/texfix.c reads it, engine/render.c
// repairs them as they are loaded).
#define TF_NAMES 64
internal char g_tf_names[TF_NAMES][64];
internal int g_tf_n;
internal void texfix_add(const char *name) { if (g_tf_n < TF_NAMES) lstrcpynA(g_tf_names[g_tf_n++], name, sizeof *g_tf_names); }
// name: as the engine gives it, textures\bld_temple_04T
internal int texfix_listed(const char *name) {
    const char *slash = name ? strrchr(name, '\\') : NULL;
    if (slash) name = slash + 1;
    for (int i = 0; name && i < g_tf_n; i++) if (!lstrcmpiA(name, g_tf_names[i])) return 1;
    return 0;
}
