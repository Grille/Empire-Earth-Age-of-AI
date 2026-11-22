// THE EDITOR'S TEXTS: the .eeai files an editor has open, their unsaved texts read in place of the files on disk - for
// ageofai_check's language server (check/lsp.c), whose checks see what is typed, not what was last saved. Every reader
// of the text files asks first (voice/persona.c read_text_acp: the personalities, mod\, language.eeai, texfix.eeai;
// test/scripts.c script_read: the scripts). Empty in the game: only the check's process sets them (check/lsp.c
// lsp_child_texts). By the file's full path, case ignored (as Windows names it).
#define EDITS_MAX 32
internal struct { char path[MAX_PATH]; char *text; size_t n; } g_edits[EDITS_MAX];   // the text malloc'd, 0 after it
internal int g_edits_n;
// path's text as the editor has it - a copy, malloc'd, its length in *n - or NULL: the file's own
internal char *edit_text_of(const char *path, size_t *n) {
    char full[MAX_PATH];
    if (!g_edits_n || !GetFullPathNameA(path, MAX_PATH, full, NULL)) return NULL;
    for (int i = 0; i < g_edits_n; i++)
        if (!lstrcmpiA(g_edits[i].path, full)) {
            char *t = malloc(g_edits[i].n + 1);
            if (t) { memcpy(t, g_edits[i].text, g_edits[i].n + 1); *n = g_edits[i].n; }
            return t;
        }
    return NULL;
}
