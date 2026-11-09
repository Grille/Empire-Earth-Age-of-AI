// THE EDITOR'S UNIT SEARCH (`editor_unit_search`, [editor]): a search box above the list of unit types in the
// scenario editor's units panel (EESEUnit, the editor's main button 3). The panel's refill (0x58C2F5) builds the list
// when its category, sort or units (the scenario's or all) have changed - every type of the category added by
// list.add; a type whose names don't hold every word typed is left out there (call site se_unit_list_add). The box is
// an edit box made as the trigger screen makes its own, into the panel's group of the form, in the panel's left
// column right above the list (the sort switches stand right of that column). It is of the mod's own kind:
// UIEditBox's vtable copied, its UpdateKeyboard (called for the box while it has the keyboard) the mod's - when the
// text has changed, the panel's shown category is forgotten and refill builds the list anew, at once and in the
// game's own UI pass. A name is the type's as the list shows it (name_text in the language file) or its internal one
// from the game data (the same in every language); case is ignored, accented letters' too (Latin-1).
// The panel's hooks are in place without the setting too: editor_place.c's box comes with the panel.
#define ES_ID 0x3F8          // the box's control id: the units panel's own are 0x3E9..0x3F7
#define ES_HEIGHT 18.0f
#define ES_WIDTH 140.0f      // the left column (world player / player, the player's box)
#define ES_GAP 3.0f          // between the box and the list
#define ES_WORDS 8
internal int g_es_on;
internal ee_se_unit *g_es_panel;       // the editor's units panel, while it is
internal ee_ui_control *g_es_box;
internal void *g_es_vt[40];            // the box's vtable (+1), its RTTI pointer first
internal ui_control_update_keyboard_fn g_es_keyboard;   // UIEditBox's
internal char g_es_text[64];           // the box's text the list was last built for
internal char g_es_word[ES_WORDS][32];
internal int g_es_words;
internal unsigned g_es_refills, g_es_left_out;

internal unsigned char es_lower(unsigned char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 0xC0 && c <= 0xDE && c != 0xD7) ? (unsigned char)(c + 32) : c;
}
// s holds w (w lower case already)
internal int es_has(const char *s, const char *w) {
    size_t n = strlen(w);
    for (; *s; s++) {
        size_t i = 0;
        while (i < n && s[i] && es_lower((unsigned char)s[i]) == (unsigned char)w[i]) i++;
        if (i == n) return 1;
    }
    return 0;
}
internal void es_set_text(const char *t) {
    lstrcpynA(g_es_text, t, sizeof g_es_text);
    g_es_words = 0;
    for (const char *p = g_es_text; *p && g_es_words < ES_WORDS;) {
        while (*p == ' ') p++;
        int n = 0;
        for (; *p && *p != ' '; p++) if (n < 31) g_es_word[g_es_words][n++] = (char)es_lower((unsigned char)*p);
        if (n) g_es_word[g_es_words++][n] = 0;
    }
}
// every word typed in one of the type's names
internal int es_match(int type) {
    ee_unit_type *td = type_data(type);
    if (!td) return 1;
    const char *in = unit_type_get_name(td);
    char shown[128] = "";
    void *res = string_resources();
    if (res) {
        __attribute__((aligned(8))) uint8_t w[32];
        wstring_init(w);
        resource_string(res, unit_type_get_name_text(td), w);
        const char *s = FIELD(w, 4, const char *);
        if (s) lstrcpynA(shown, s, sizeof shown);
        wstring_free(w);
    }
    for (int i = 0; i < g_es_words; i++)
        if (!es_has(shown, g_es_word[i]) && !(in && es_has(in, g_es_word[i]))) return 0;
    return 1;
}

internal void __thiscall hook_es_add(ee_se_unit_list *list, int type) {
    if (g_es_words && !es_match(type)) { g_es_left_out++; return; }
    se_unit_list_add(list, type);
}

internal int __thiscall hook_es_keyboard(ee_ui_control *box) {
    int r = g_es_keyboard(box);
    const char *t = box == g_es_box && g_es_panel ? edit_text(box) : NULL;
    char now[sizeof g_es_text];
    lstrcpynA(now, t ? t : "", sizeof now);
    if (t && strcmp(now, g_es_text)) {
        es_set_text(now);
        unsigned before = g_es_left_out;
        se_unit_set_shown_category(g_es_panel, -1);
        se_unit_refill(g_es_panel);
        g_es_refills++;
        if (g_log) log_locked("# editor: unit search \"%s\": %u types left out\n", g_es_text, g_es_left_out - before);
    }
    return r;
}

internal void *__thiscall hook_es_init(ee_se_unit *p, int *index, ee_ui_form *form, void *font, uint32_t arg) {
    void *r = se_unit_init(p, index, form, font, arg);
    ee_ui_control *list = (ee_ui_control *)se_unit_get_list(p);
    if (!list) return r;   // (a box of a panel before this one is its form's: a form deletes its panels with it)
    ep_panel_init(p, list, form, index);
    if (!g_es_on) return r;
    float top = ui_control_get_top(list), left = ui_control_get_left(list);
    float rect[4] = { top - ES_GAP, left, left + ES_WIDTH - 1.0f, top - ES_GAP - ES_HEIGHT };   // bottom, left, right, top
    ee_ui_control *box = edit_box_new(ES_ID, rect, form, index);
    if (!box) return r;
    *(void ***)box = g_es_vt + 1;
    ui_control_set_bg_opacity(box, 0.4f);   // as the panel's own controls
    editor_units_add_control(form, box);
    g_es_panel = p;
    g_es_box = box;
    es_set_text("");
    if (g_log) log_locked("# editor: unit search box at %.0f,%.0f (the list %.0f,%.0f - %.0f,%.0f)\n", left, rect[3],
                          left, top, ui_control_get_right(list), ui_control_get_bottom(list));
    return r;
}

// the panel deleted (the editor closed): the box first - the panel deletes its own controls only, the moment they go
// (a control's destructor doesn't take it out of its form: the form is going too)
internal void *__thiscall hook_es_delete(ee_se_unit *p, uint8_t flags) {
    ep_panel_delete(p);
    if (p == g_es_panel) {
        if (g_es_box) ((void (__thiscall *)(void *, int))(*(void ***)g_es_box)[0])(g_es_box, 1);
        g_es_box = NULL;
        g_es_panel = NULL;
        if (g_log) log_locked("# editor: unit search box deleted with the panel (%u searches)\n", g_es_refills);
    }
    return eese_unit_destroy(p, flags);
}

internal void editor_search_install(void) {
    g_es_on = g_set.editor_unit_search;
    int n = 0, of = 0;
    if (g_es_on) {
        void **kb = edit_box_vtable_copy(g_es_vt, (int)(sizeof g_es_vt / sizeof *g_es_vt));
        if (kb) {
            g_es_keyboard = (ui_control_update_keyboard_fn)*kb;
            *kb = (void *)hook_es_keyboard;
            n = 1 + patch_se_unit_list_add(hook_es_add);
        }
    }
    // the panel's two: the sanity check's box is there whatever the search does (editor_place.c)
    int panel = patch_se_unit_init(hook_es_init) + patch_se_unit_delete(hook_es_delete) + patch_se_unit_update(hook_ep_update);
    int preview = patch_se_ghost_preview(hook_ep_preview, &of);
    if (g_log) fprintf(g_log, "# editor: editor_unit_search=%d%s, sanity check box %s\n", g_es_on, !g_es_on ? "" : n == 2 ? " (hooked)" : " (FAILED)",
                       panel == 3 && preview == of ? "hooked" : "FAILED");
}
