// texfix.eeai, beside ageofai.ini: fixalpha = ["bld_temple_04t", ...] - the game's textures painted a little see-through all
// over by mistake, by their names (the files' in textures\ of Data\data.ssa, without .sst). They are made solid as they are
// loaded (engine/render.c); the game's files stay as they are. No file, no repairs.
internal void texfix_name(const char *name, void *arg) { (void)arg; texfix_add(name); }
internal void texfix_key(void *arg, const char *sec, const char *key, struct tval *v, struct tlex *l) {
    (void)arg;
    if (*sec || lstrcmpiA(key, "fixalpha")) { tl_error(l, "this file has fixalpha = [\"a texture\", ...]"); return; }
    if (!x_plain_strings(v->src, texfix_name, NULL)) tl_error(l, "fixalpha = [\"a texture\", ...]: the textures' names, in quotes");
}
internal void texfix_read_file(const char *path, const char *shown) {
    char *text = read_text_acp(path);
    if (!text) return;
    tfile_parse(shown, text, texfix_key, NULL);
    free(text);
}
internal void texfix_read(void) {
    char path[MAX_PATH];
    lstrcpynA(path, g_aimod_ini, sizeof path - 16);
    char *slash = strrchr(path, '\\');
    lstrcpyA(slash ? slash + 1 : path, "texfix.eeai");
    texfix_read_file(path, "texfix.eeai");
}
