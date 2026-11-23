// THE SELECTION'S CARDS BY KIND ([general] selection_cards_by_kind, default 1 - the player, 2026-11-23: "right now every
// selected unit gets its own card in the bottom left, that gets unreadable quickly. how about instead each unit type gets
// a card with a number for the count? with the health bar being their average"). The game's unit cards
// (EEUICUnitIconBar) are a card a selected unit, squeezed together to fit the bar. Now a card a kind:
// - refresh's cards (an icon and a health a unit, in the UI's group's order) are folded right after it into the
//   first card of each kind: its icon, the kind's average health. Refresh's step is the bar's width over the cards'
//   number (A / (n - 1), at most 1, n at most 35), so the same width over k cards is step x (n - 1) / (k - 1).
// - the control's draw_text (UIControl's own, which draws nothing) draws the number of each kind of more than one onto
//   its card's corner: a GEText a card in the font of the main group's numbers, as the attributes draw theirs.
// - a left click keeps the card's kind alone selected, a right click takes it out of the selection (EEMSelectTheseUnits,
//   as the control-click sends for a kind; shift and control change nothing then: the game's shift-drag over the
//   cards would add the units at the cards' places in the selection, not theirs). The units inside one selected
//   transport or building (refresh's inside) stay a card each - a click lets that one out.
// The folded cards (g_cards) are the bar's on this PC - one bar, the main game screen's; the selection they were
// folded from is read again at a click, so a unit gone since is not sent.
#define CARDS_MAX 64       // kinds folded: a selection of more kinds keeps a card a unit
#define CARDS_DRAWN 35     // the bar draws at most this many cards (its draw_polygons)
internal struct {
    icon_bar_refresh_fn refresh;
    int n;                       // the cards as folded by the last refresh; 0 not folded (off, inside, nothing selected)
    int kind[CARDS_MAX];         // a card's kind (its unit type's id)
    int count[CARDS_MAX];        // its units
    ee_text *text[CARDS_MAX];    // its number, made when first drawn
    int shown[CARDS_MAX];        // the number in it
    ee_font *font;
    unsigned long font_h;
} g_cards;

// the local player's selection as the cards show it: the UI's copy (ui_group), which refresh reads - a new selection
// is there steps before it reaches selected_group, and the cards read from that stayed a card a unit until it did
// (the player, 2026-11-23: "it takes a second for the cards to collapse"); NULL none
internal void *cards_group(void) {
    ee_player *pl = game_players()[local_player_index()];
    return pl ? player_get_ui_group(pl) : NULL;
}
internal int cards_kind_of(ee_unit *u) { return u ? unit_type_get_id(unit_get_type_data(u)) : -1; }
internal int cards_find(int k, int kind) {
    int c = 0;
    while (c < k && g_cards.kind[c] != kind) c++;
    return c;
}

internal void __thiscall cards_refresh(ee_icon_bar *bar) {
    g_cards.refresh(bar);
    g_cards.n = 0;
    if (!g_set.selection_cards_by_kind || icon_bar_get_inside(bar)) return;
    void *grp = cards_group();
    void **icon = icon_bar_get_icons_begin(bar);
    float *hp = icon_bar_get_health_begin(bar);
    unsigned n = grp ? group_count(grp) : 0;
    if (!n || !icon || !hp || icon_bar_get_icons_end(bar) - icon != (ptrdiff_t)n || icon_bar_get_health_end(bar) - hp != (ptrdiff_t)n)
        return;
    int k = 0;   // the kinds first: more than CARDS_MAX leaves the cards as they are
    for (unsigned i = 0; i < n; i++) {
        int kind = cards_kind_of(group_get(grp, i)), c = cards_find(k, kind);
        if (c < k) continue;
        if (k == CARDS_MAX) return;
        g_cards.kind[k++] = kind;
    }
    float sum[CARDS_MAX] = { 0 };
    memset(g_cards.count, 0, sizeof g_cards.count);
    for (unsigned i = 0; i < n; i++) {   // a card's place is never after its first unit's: folded in place
        int c = cards_find(k, cards_kind_of(group_get(grp, i)));
        if (!g_cards.count[c]) icon[c] = icon[i];
        g_cards.count[c]++;
        sum[c] += hp[i];
    }
    for (int c = 0; c < k; c++) hp[c] = sum[c] / (float)g_cards.count[c];
    icon_bar_set_icons_end(bar, icon + k);
    icon_bar_set_health_end(bar, hp + k);
    float step = icon_bar_get_step(bar);
    int n0 = n < CARDS_DRAWN ? (int)n : CARDS_DRAWN, k0 = k < CARDS_DRAWN ? k : CARDS_DRAWN;
    if (step < 1.0f && n0 > 1) {
        step = k0 > 1 ? step * (float)(n0 - 1) / (float)(k0 - 1) : 1.0f;
        icon_bar_set_step(bar, step < 1.0f ? step : 1.0f);
    }
    g_cards.n = k;
}

internal int __thiscall cards_draw_text(ee_icon_bar *bar) {
    int r = icon_bar_draw_text(bar);
    int k = g_cards.n;
    void **drawn = icon_bar_get_drawn_begin(bar);
    ee_ui_form *form = icon_bar_get_form(bar);
    if (!k || !drawn || !form || icon_bar_get_drawn_end(bar) - drawn != k) return r;   // the cards drawn are copy_data's of the folded
    float iw = icon_bar_get_icon_right(bar) - icon_bar_get_icon_left(bar), ih = icon_bar_get_icon_bottom(bar) - icon_bar_get_icon_top(bar);
    float step = iw * icon_bar_get_drawn_step(bar) + 1.0f, left = icon_bar_get_screen_left(bar), top = icon_bar_get_screen_top(bar);
    unsigned long fh = (unsigned long)(ih * 0.4f);
    if (fh < 10) fh = 10;
    if (!g_cards.font || g_cards.font_h != fh) {
        ee_font *f = ui_font_numbers(fh);
        if (!f) return r;
        g_cards.font = f;
        g_cards.font_h = fh;
        for (int c = 0; c < CARDS_MAX; c++)
            if (g_cards.text[c]) { text_set_font(g_cards.text[c], f); g_cards.shown[c] = 0; }
    }
    for (int c = 0; c < k && c < CARDS_DRAWN; c++) {
        int n = g_cards.count[c];
        if (n < 2) continue;
        if (!g_cards.text[c] && !(g_cards.text[c] = text_new(g_cards.font))) return r;
        ee_text *t = g_cards.text[c];
        if (g_cards.shown[c] != n) { text_set_number(t, n); g_cards.shown[c] = n; }
        float x = left + (float)c * step, w = step - 1.0f < iw ? step - 1.0f : iw;
        const float rect[4] = { top + ih, x, x + w, top };
        for (int pass = 0; pass < 2; pass++) {   // a dark shadow a pixel down and right, the white number on it (GDI draws at once)
            float v = pass ? 1.0f : 0.0f, d = pass ? 0.0f : 1.0f;
            text_set_x(t, x + 2.0f + d);
            text_set_y(t, top + ih - (float)fh - 2.0f + d);
            text_set_red(t, v);
            text_set_green(t, v);
            text_set_blue(t, v);
            ui_form_draw_clipped_text(form, rect, t, bar);
        }
    }
    return r;
}

// the selected units of the kind (keep: alone; else: all but them - none when it is the only kind) sent as the
// selection; 0 not sent
internal int cards_select(int kind, int keep) {
    void *grp = cards_group();
    unsigned n = grp ? group_count(grp) : 0, m = 0;
    uint32_t *ids = n ? (uint32_t *)malloc(n * sizeof *ids) : NULL;
    if (!ids) return 0;
    for (unsigned i = 0; i < n; i++) {
        ee_unit *u = group_get(grp, i);
        if (u && (cards_kind_of(u) == kind) == keep) ids[m++] = (uint32_t)unit_get_serial(u);
    }
    int sent = (m || !keep) && select_these_send(ids, m);
    free(ids);
    return sent;
}
// a click on a folded card: its kind kept (left) or taken out (right); else the game's
internal int cards_click(ee_icon_bar *bar, int keep) {
    if (!g_cards.n || !g_set.selection_cards_by_kind || icon_bar_get_inside(bar) || icon_bar_get_shift_drag(bar)) return 0;
    unsigned c = icon_bar_card_at_mouse(bar);
    if (c >= (unsigned)g_cards.n) return 0;
    if (!cards_select(g_cards.kind[c], keep)) return 0;
    ui_control_release_captured_mouse((ee_ui_control *)bar);
    return 1;
}
internal int __thiscall cards_left_up(ee_icon_bar *bar) { return cards_click(bar, 1) ? 0 : icon_bar_mouse_l_button_up(bar); }
internal int __thiscall cards_right_up(ee_icon_bar *bar) { return cards_click(bar, 0) ? 0 : icon_bar_mouse_r_button_up(bar); }

internal void cards_install(void) {
    int ok = 0;
    if (patch_icon_bar_refresh(cards_refresh, &g_cards.refresh)) {
        ok = 1;
        ok += patch_icon_bar_draw_text(cards_draw_text);
        ok += patch_icon_bar_left_up(cards_left_up);
        ok += patch_icon_bar_right_up(cards_right_up);
    }
    if (g_log) fprintf(g_log, "# selection: the cards by kind (selection_cards_by_kind=%d): hooks %d/4\n", g_set.selection_cards_by_kind, ok);
}
