// THE PLAYERS IN THE TEXT LANGUAGE (script/host.h value, method, object - eeaimod.c x_host_install): what the
// diplomacy and the AI know, as the texts' values. Players: me (the speaker), them (the one it speaks to or
// about; in a system line the reader, the local player), enemy (a third: the enemy of an alliance offer, the ally or
// lord in some lines) and lord (the speaker's lord, whatever the line). A player is a value: its name in a text, equal
// only to itself (lord == them).
// Its members: me.soldiers, them.epoch, them.lord (a player again: them.lord.name). Its relations name the other
// player: me.opinion(them) is what the speaker thinks of them, them.opinion(me) what they think of it,
// enemy.opinion(them) ... Shortcuts: the speaker's own without "me." - epoch, traits, a relation (opinion(enemy) =
// me.opinion(enemy)) - and opinion, at_war, allied alone: the speaker's with them (me.opinion(them)). An opinion is
// a number and words (mod/system.eeai opinion1..5): "hostile" in a text. Nothing where a value doesn't apply (a relation
// with oneself, a war that isn't, a player who isn't there).
// A script's player also does things (XM_DO): p.win(), p.war(q) - and its stock may be set: p.gold = 500 (SCRIPTS' EFFECTS
// below); a text changes nothing.
// The members, the text's own names and the kinds' tables are declared in lang/ (player.cs, names.cs, game.cs,
// slot.cs - C#, the language's objects): names, types, arguments, docs - scripts/build/gen_lang.py makes lang.h of
// them, gen_eeai_readme.py the README's tables. What each one is worked out as is here.
// THE KINDS OF OBJECTS (script/host.h): a player, the game, a slot, a unit (game_script/units.c)
enum { XK_PLAYER = 1, XK_GAME, XK_SLOT, XK_UNIT };
#include "game_script/lang.h"
internal int g_llm_equal, g_llm_constrained;   // llm.c: [llm] equal, constrained
internal void *x_player_named(const char *n) {
    for (int j = 1; n && *n && j < nplayers(); j++) if (player_at(j) && !strcmp(player_name(player_at(j)), n)) return player_at(j);
    return NULL;
}
internal void x_players(struct xctx *c) {   // who is who: the context's players - them, when not given, the local player
    if (c->found) return;
    c->found = 1;
    unsigned h = local_player_index();
    if (!c->other && h < 16 && player_at((int)h) != c->pl) c->other = player_at((int)h);
}
internal int x_alive(int j) { return player_at(j) && !is_out(player_at(j)); }
internal void *x_lord(void *p) { return p && g_lord[pidx(p)] ? player_at(g_lord[pidx(p)]) : NULL; }
internal void x_player(struct xval *v, void *p, const char *who) {   // a player as a value: its name, itself
    if (!p) { xnothing(v); lstrcpynA(v->miss, who, sizeof v->miss); return; }   // who isn't there: to be checked
    xstr(v, *player_name(p) && *player_name(p) != '?' ? player_name(p) : "the player");   // a test's idle human has no name
    v->obj = pidx(p) + 1;
    v->okind = XK_PLAYER;
}
internal void x_game(struct xval *v) { xstr(v, "the game"); v->obj = 1; v->okind = XK_GAME; }
internal int x_game_path(struct xctx *c, const char *name, const struct xval *a, int na, int call, struct xval *v);
internal int x_player_n(int n, struct xval *v) {   // expr.c: player(n) in a script - player n, 0: none there
    void *p = n >= 1 && n < nplayers() ? player_at(n) : NULL;
    if (!p) return 0;
    x_player(v, p, "");
    return 1;
}
internal void *x_player_of(const struct xval *v) {   // the player a value is - or names (a placeholder: {ally})
    if (!v->w) return NULL;
    if (v->obj) return v->okind == XK_PLAYER ? player_at(v->obj - 1) : NULL;
    return v->has_s ? x_player_named(v->s) : NULL;
}
// member m of player px
internal void x_member(void *px, const struct xmember *m, struct xval *v) {
    xnothing(v);
    if (!px) return;
    int x = pidx(px);
    const char *n = m->name;
    double r = 0;
    static const char *const res[5] = { "food", "wood", "stone", "gold", "iron" };
    for (int k = 0; k < 5; k++) {
        if (!strcmp(n, "stock")) r += player_get_stock(px, k + 2);
        else if (!strcmp(n, res[k])) { xnum(v, player_get_stock(px, k + 2)); return; }
    }
    if (!strcmp(n, "stock")) { xnum(v, r); return; }
    if (!strcmp(n, "name")) { xstr(v, player_name(px)); return; }
    if (!strcmp(n, "epoch")) { char ep[96]; epoch_text(px, ep, sizeof ep); xdual(v, current_epoch(px) + 1, ep); return; }
    if (!strcmp(n, "color")) { int c = player_color(px); if (c >= 0) xdual(v, c + 1, color_name(c)); return; }
    if (!strcmp(n, "civilization") || !strcmp(n, "civilization_en")) { const char *s = player_civ_of(px, n[12] == '_'); if (s) xstr(v, s); return; }
    if (!strcmp(n, "soldiers")) r = g_ws.soldiers[x];
    else if (!strcmp(n, "citizens")) r = g_ws.citizens[x];
    else if (!strcmp(n, "population")) r = g_ws.soldiers[x] + g_ws.citizens[x];
    else if (!strcmp(n, "buildings")) r = g_ws.buildings[x];
    else if (!strcmp(n, "towers")) r = g_ws.towers[x];
    else if (!strcmp(n, "walls")) r = g_ws.walls[x];
    else if (!strcmp(n, "centers")) r = g_ws.centers[x];
    else if (!strcmp(n, "strength")) r = (double)(long)g_ws.power[x];
    else if (!strcmp(n, "rank")) { r = 1; for (int j = 1; j < nplayers(); j++) r += j != x && x_alive(j) && g_ws.power[j] > g_ws.power[x]; }
    else if (!strcmp(n, "alive")) r = !is_out(px);
    else if (!strcmp(n, "human")) r = (unsigned)x == local_player_index();
    else if (!strcmp(n, "vassals")) for (int j = 1; j < nplayers(); j++) r += j != x && x_alive(j) && g_lord[j] == x;
    else if (!strcmp(n, "allies") || !strcmp(n, "enemies"))
        for (int j = 1; j < nplayers(); j++) r += j != x && x_alive(j) && (n[0] == 'a' ? allied_with(x, j) : stance_of(px, j) != 0);
    else for (int t = 0; t < T_N; t++)
        if (!strcmp(n, k_trait_keys[t])) { xnum(v, trait(px, (enum trait)t) / 100.0); return; }   // a share: 60% is 0.6
    xnum(v, r);
}
// relation m of player pa with player pb: pa.<m>(pb), in the last win minutes (0: the whole game) where it counts
// events; kills of the types in sel (NULL: all)
internal void x_relation(void *pa, void *pb, const struct xmember *m, double win, const unsigned char *sel, struct xval *v) {
    xnothing(v);
    if (!pa || !pb || pa == pb) return;
    int a = pidx(pa), b = pidx(pb);
    const char *n = m->name;
    uint32_t now = game_time();
    int war = stance_of(pa, b) && stance_of(pb, a), ally = !stance_of(pa, b) && !stance_of(pb, a);
    uint32_t since = win > 0 && win * 60000.0 < now ? now - (uint32_t)(win * 60000.0) : 0;
    int whole = !(win > 0);
    double r = 0;
    if (!strcmp(n, "opinion")) {   // an AI's opinion - a human's of an AI: the guess (g_hum) - and its words
        if (g_players[a]) r = g_opinion[a][b];
        else if (hum_pair(a, b)) r = g_hum.op[a][b];
        else return;
        char key[12];
        wsprintfA(key, "opinion%d", r < -50 ? 1 : r < -15 ? 2 : r <= 15 ? 3 : r <= 50 ? 4 : 5);
        xdual(v, r / 100.0, sys_text(key));   // -100%..100% (the code's -100..100), its words in a text
        return;
    }
    else if (!strcmp(n, "at_war")) r = war;
    else if (!strcmp(n, "allied")) r = ally;
    else if (!strcmp(n, "hostile")) r = stance_of(pa, b) != 0;
    else if (!strcmp(n, "war_minutes")) {
        if (!g_hist.war[a][b]) return;
        r = (double)((now - g_hist.war[a][b]) / 60000u);
    }
    else if (!strcmp(n, "alliance_minutes")) {
        if (!ally) return;
        r = (double)((now - g_hist.since[a][b]) / 60000u);
    }
    else if (!strcmp(n, "betrayals")) r = whole ? g_hist.betrayals[a][b] : hlog_sum(HE_BETRAYAL, a, b, since);
    else if (!strcmp(n, "refusals")) r = whole ? g_hist.refused[a][b] : hlog_sum(HE_REFUSED, a, b, since);
    else if (!strcmp(n, "ignored")) r = g_offer_ignored[b][a];
    else if (!strcmp(n, "tribute")) r = whole ? g_hist.tribute[a][b] : hlog_sum(HE_TRIBUTE, a, b, since);
    if (g_tell.kind && g_tell.a == a && g_tell.b == b
        && ((g_tell.kind == HE_BETRAYAL && !strcmp(n, "betrayals")) || (g_tell.kind == HE_TRIBUTE && !strcmp(n, "tribute")))) {
        r -= hlog_sum(g_tell.kind, a, b, g_tell.t > since ? g_tell.t : since);   // the event the line is about
        if (r < 0) r = 0;
    }
    else if (!strcmp(n, "near")) { if (!g_players[b]) return; r = g_ws.close_by[b][a]; }
    else if (!strcmp(n, "in_town")) { if (!g_players[b]) return; r = g_pressure[b][a] >= 10; }
    else if (!strcmp(n, "attack_minutes")) {
        if (!g_attack_sent[a][b]) return;
        r = (double)((now - g_attack_sent[a][b]) / 60000u);
    }
    else if (!strcmp(n, "kills")) r = kl_count(a, b, sel, whole ? 0 : win);

    xnum(v, r);
}
internal const struct xmember *x_find_member(const char *n, int len) {
    for (int i = 0; i < N_XMEMBERS; i++) if ((int)strlen(k_xmembers[i].name) == len && !strncmp(k_xmembers[i].name, n, len)) return &k_xmembers[i];
    return NULL;
}
internal int x_mine(const char *name) {   // a member that alone is mine: epoch, allies, enemies, the traits ([mine])
    const struct xmember *m = x_find_member(name, (int)strlen(name));
    return m && m->mine;
}
// a path of players: me, them, enemy, lord, and .lord after them (them.lord.lord); *ok 0: not one. NULL: who (24
// bytes) the part of it that isn't there
internal void *x_path(struct xctx *c, const char *s, int len, int *ok, char *who) {
    const char *s0 = s, *dot = memchr(s, '.', (size_t)len);
    int n = dot ? (int)(dot - s) : len;
    void *p;
    *ok = 1;
    who[0] = 0;
    x_players(c);
    if (n == 2 && !strncmp(s, "me", 2)) p = c->pl;
    else if (n == 4 && !strncmp(s, "them", 4)) p = c->other;
    else if (n == 5 && !strncmp(s, "enemy", 5)) p = c->third;
    else if (n == 4 && !strncmp(s, "lord", 4)) p = x_lord(c->pl);
    else {   // a function's parameter that is a player: by.kills(of)
        int i = 0;
        while (i < c->fnp && ((int)strlen(c->fpn[i]) != n || strncmp(c->fpn[i], s, (size_t)n))) i++;
        if (i < c->fnp) {
            p = x_player_of(&c->fpv[i]);
            if (!p && !c->fpv[i].miss[0]) { *ok = 0; return NULL; }   // no player
        } else if (x_check_par_has(s, n)) p = NULL;   // its definition read: the syntax only
        else { *ok = 0; return NULL; }
    }
    if (!p) lstrcpynA(who, s0, n + 1 < 24 ? n + 1 : 24);
    while (dot) {
        s = dot + 1;
        len -= n + 1;
        dot = memchr(s, '.', (size_t)len);
        n = dot ? (int)(dot - s) : len;
        const struct xmember *m = x_find_member(s, n);
        if (!m || m->kind != XM_LINK) { *ok = 0; return NULL; }
        if (p && !(p = x_lord(p))) lstrcpynA(who, s0, (int)(s + n - s0) + 1 < 24 ? (int)(s + n - s0) + 1 : 24);
    }
    return p;
}
internal int x_builtin(struct xctx *c, const char *name, struct xval *v) {
    const char *dot = strrchr(name, '.');
    int ok;
    char who[24];
    if (name[0] == 'g' && x_game_path(c, name, NULL, 0, 0, v)) return 1;   // game, game.time ...
    if (dot) {   // <player>.<member>
        void *p = x_path(c, name, (int)(dot - name), &ok, who);
        const struct xmember *m = ok ? x_find_member(dot + 1, (int)strlen(dot + 1)) : NULL;
        if (!m || m->kind == XM_REL || m->kind == XM_FILTER || m->kind == XM_DO) return 0;   // a relation needs its other player: them.opinion(me)
        if (!p) { x_missing(c, who); xnothing(v); return 1; }   // lord.soldiers without a lord
        if (m->kind == XM_LINK) { x_player(v, x_lord(p), name); return 1; }   // them.lord: a player, to be checked
        x_member(p, m, v);
        return 1;
    }
    void *p = x_path(c, name, (int)strlen(name), &ok, who);
    if (ok) { x_player(v, p, who); return 1; }   // a player
    int g = 0;
    while (g < N_XGLOBALS && strcmp(k_xglobals[g].name, name)) g++;
    if (g < N_XGLOBALS && k_xglobals[g].kind == XM_REL) {   // opinion, at_war, allied: mine with them
        if (!c->pl || !c->other) { x_missing(c, c->pl ? "them" : "me"); xnothing(v); return 1; }
        x_relation(c->pl, c->other, x_find_member(name, (int)strlen(name)), 0, NULL, v);
        return 1;
    }
    if (g < N_XGLOBALS) {
        double r = 0;
        if (!strcmp(name, "minutes")) r = (double)(game_time() / 60000u);
        else if (!strcmp(name, "language")) { xstr(v, g_lang); return 1; }
        else if (!strcmp(name, "offer_minutes")) r = g_offer_min;
        else if (!strcmp(name, "llm_equal")) r = g_llm_equal;
        else if (!strcmp(name, "llm_constrained")) r = g_llm_constrained;
        else if (!strcmp(name, "population_limit")) r = pop_limit_setting();   // the game setting (econ.c)
        else if (!strcmp(name, "max_population")) {
            for (int j = 1; j < nplayers(); j++) if (x_alive(j) && g_ws.soldiers[j] + g_ws.citizens[j] > r) r = g_ws.soldiers[j] + g_ws.citizens[j];
        }
        else for (int j = 1; j < nplayers(); j++) r += x_alive(j);   // players
        xnum(v, r);
        return 1;
    }
    const struct xmember *m = x_find_member(name, (int)strlen(name));   // without "me.": my epoch, traits, allies, enemies
    if (!x_mine(name)) return 0;
    x_member(c->pl, m, v);
    return 1;
}
// a name that is a value of its own - a player, a game value, one of mine without "me." - and no placeholder may
// have (texts.c tsubv_vars)
internal int x_is_value(const char *name) {
    if (!strcmp(name, "me") || !strcmp(name, "them") || !strcmp(name, "enemy") || !strcmp(name, "lord") || x_mine(name)) return 1;
    for (int g = 0; g < N_XGLOBALS; g++) if (!strcmp(k_xglobals[g].name, name)) return 1;
    return 0;
}

// expr.c x_name_check: a name x_builtin takes - a player, a game value, one of mine, <player>[.lord ...].<member> -
// when a file is read (no game needed)
internal int x_name_valid(const char *name) {
    if (x_is_value(name) || x_game_path(NULL, name, NULL, 0, 0, NULL)) return 1;
    const char *dot = strrchr(name, '.');
    if (!dot) return 0;
    int n = (int)strcspn(name, ".");
    if (!((n == 2 && !strncmp(name, "me", 2)) || (n == 4 && !strncmp(name, "them", 4)) || (n == 5 && !strncmp(name, "enemy", 5)) ||
          (n == 4 && !strncmp(name, "lord", 4)) || x_check_par_has(name, n))) return 0;
    for (const char *s = name + n; s < dot; ) {   // .lord between
        s++;
        int k = (int)strcspn(s, ".");
        const struct xmember *m = x_find_member(s, k);
        if (!m || m->kind != XM_LINK) return 0;
        s += k;
    }
    const struct xmember *m = x_find_member(dot + 1, (int)strlen(dot + 1));
    return m && m->kind != XM_REL && m->kind != XM_DO;
}
// a filter of unit types: unit.<identifier>, kind.<kind> (each type of that family) or a list of them, nested too -
// any of them: into sel (set, not cleared); 0: something else in it (a mistake, told)
internal int x_type_filter(const char *name, const struct xval *f, unsigned char sel[G_TYPES_N]) {
    if (!f->w && !f->ln) { x_error("%s(...): no filter - nothing", name); return 0; }
    for (int i = 0; i < (f->ln ? f->ln : 1); i++) {
        const struct xval *it = xl_at(f, i);
        int t = it->has_n ? (int)it->n : -1;
        if (it->ln) { if (!x_type_filter(name, it, sel)) return 0; }
        else if (it->has_s && !strncmp(it->s, "kind.", 5) && t >= 0 && t < UNIT_KINDS) {
            for (int j = g_kind_first[t]; j < g_kind_first[t + 1]; j++) sel[g_kind_types[j]] = 1;   // its types (unitnames.c)
        } else if (it->has_s && t > 0 && t < G_TYPES_N && (it->s == g_unit_ident[t] || !strcmp(it->s, g_unit_ident[t]))) sel[t] = 1;
        else {
            char x[48];
            xto_text(it, x, sizeof x);
            x_error("%s(%s): a filter is unit.<identifier>, kind.<kind> or a list of them (docs/units.md)", name, x);
            return 0;
        }
    }
    return 1;
}
// <player>.count(filter), or count(filter) - mine: its units and buildings of those types, at the last world scan
internal int x_count_of(struct xctx *c, void *p, const char *who, const char *name, const struct xval *f, struct xval *v);
internal int x_count(struct xctx *c, const char *name, const char *dot, const struct xval *f, struct xval *v) {
    int ok = 1;
    char who[24] = "me";
    void *p = dot ? x_path(c, name, (int)(dot - name), &ok, who) : (x_players(c), c->pl);
    if (!ok) return 0;
    if (!v) return 1;
    return x_count_of(c, p, who, name, f, v);
}
// player p's count (who: as written, p NULL - none there)
internal int x_count_of(struct xctx *c, void *p, const char *who, const char *name, const struct xval *f, struct xval *v) {
    if (!p) { x_missing(c, who); xnothing(v); return 1; }   // lord.count(...) without a lord
    static unsigned char sel[G_TYPES_N];
    memset(sel, 0, sizeof sel);
    if (!x_type_filter(name, f, sel)) { xnothing(v); return 1; }
    int i = pidx(p), r = 0;
    for (int t = 0; t < G_TYPES_N; t++) if (sel[t]) r += g_ws_types[i][t];
    xnum(v, r);
    return 1;
}
// a relation: <player>.<relation>(<player>[, minutes]), or <relation>(<player>) - the speaker's. v NULL: only whether
// name is one
internal int x_count(struct xctx *c, const char *name, const char *dot, const struct xval *f, struct xval *v);
internal int x_method_of(struct xctx *c, void *pa, const char *who, const char *name, const struct xmember *m,
                         const struct xval *a, int na, struct xval *v);
internal int x_method(struct xctx *c, const char *name, const struct xval *a, int na, struct xval *v) {
    if (!strncmp(name, "game.", 5)) return x_game_path(c, name, a, na, 1, v);   // game.player(3)
    const char *dot = strrchr(name, '.');
    const char *rel = dot ? dot + 1 : name;
    const struct xmember *m = x_find_member(rel, (int)strlen(rel));
    if (m && m->kind == XM_FILTER && strcmp(rel, "units")) return na == 1 && x_count(c, name, dot, a, v);   // (units: a script's)
    // (B) or, counting events, (B, minutes) - kills also with a filter last: (B, kind.tank), (B, 10, kind.tank) - lang/player.cs
    if (!m || m->kind != XM_REL || na < m->amin || na > m->amax) return 0;
    int ok = 1;
    char who[24] = "me";
    void *pa = dot ? x_path(c, name, (int)(dot - name), &ok, who) : (x_players(c), c->pl);
    if (!ok) return 0;
    if (!v) return 1;
    return x_method_of(c, pa, who, name, m, a, na, v);
}
// relation m of player pa (who: as written, pa NULL - none there) with the arguments a - the count checked before
internal int x_method_of(struct xctx *c, void *pa, const char *who, const char *name, const struct xmember *m,
                         const struct xval *a, int na, struct xval *v) {
    int kr = !strcmp(m->name, "kills");
    void *pb = x_player_of(&a[0]);
    if (!pa || a[0].miss[0]) {   // lord.opinion(them), opinion(lord) without a lord: a mistake (x_missing)
        x_missing(c, pa ? a[0].miss : who);
        xnothing(v);
        return 1;
    }
    if (a[0].w && !pb) {   // anything else that is no player is a mistake
        char t[48];
        xto_text(&a[0], t, sizeof t);
        x_error("%s(%s): not a player", name, t);
        xnothing(v);
        return 1;
    }
    int fl = kr && na > 1 && (a[na - 1].ln || a[na - 1].has_s), nw = na - fl;   // anything but a number last: the filter
    if (nw > 1 && (a[1].ln || a[1].has_s)) {   // tribute(B, kind.tank), kills(B, "army", 5): no minutes
        char t[48];
        if (a[1].ln) lstrcpyA(t, "a list");   // (its text would be one item, drawn)
        else xto_text(&a[1], t, sizeof t);
        x_error("%s(...): %s - the minutes are a number%s", name, t, kr ? " (the filter comes last)" : " (only kills has a filter)");
        xnothing(v);
        return 1;
    }
    if (nw > 2) {   // kills(B, 5, 10)
        x_error("%s(B, minutes, filter): the last argument is the filter - unit.<identifier>, kind.<kind> or a list", name);
        xnothing(v);
        return 1;
    }
    if (nw > 1 && xto_num(&a[1]) < 0) {   // not "older than": that is the whole game minus the window
        x_error("%s(..., %g): the minutes are a window back from now, 0 the whole game - older events: %s(B) - %s(B, %g)",
                name, xto_num(&a[1]), name, name, -xto_num(&a[1]));
        xnothing(v);
        return 1;
    }
    static unsigned char sel[G_TYPES_N];
    if (fl) {
        memset(sel, 0, sizeof sel);
        if (!x_type_filter(name, &a[na - 1], sel)) { xnothing(v); return 1; }
    }
    x_relation(pa, pb, m, nw > 1 ? xto_num(&a[1]) : 0, fl ? sel : NULL, v);
    return 1;
}

// THE TYPES (script/host.h) of a player's members, of the text's own names (lang/: the tables') and of the constants
internal int x_member_type(const char *n) {   // X_ANY: none, or an effect (win, war: no value)
    const struct xmember *m = x_find_member(n, (int)strlen(n));
    return m ? m->type : X_ANY;
}
internal int x_name_type(const char *name) {   // a name of its own: the game's values, mine without me.
    for (int g = 0; g < N_XGLOBALS; g++) if (!strcmp(k_xglobals[g].name, name)) return k_xglobals[g].type;
    return x_mine(name) ? x_member_type(name) : X_ANY;
}
internal int x_const_type(const char *name) {   // persona.x, color.x, resource.x, kind.x, unit.x, epoch.x
    return !_strnicmp(name, "persona.", 8) ? XT_TEXT : !_strnicmp(name, "color.", 6) ? XT_COLOR : !_strnicmp(name, "resource.", 9) ? XT_INT :
           !_strnicmp(name, "kind.", 5) || !_strnicmp(name, "unit.", 5) ? XT_UTYPE : !_strnicmp(name, "epoch.", 6) ? XT_EPOCH : X_ANY;
}
internal int x_stock_index(const char *name) {   // food .. iron: the stock's index (2..6), -1 none
    static const char *const res[5] = { "food", "wood", "stone", "gold", "iron" };
    for (int k = 0; k < 5; k++) if (!strcmp(name, res[k])) return k + 2;
    return -1;
}

// THE KINDS (script/host.h: the check reads them, x_kmember): a player, the game, a slot - their members lang.h's
// k_xk_player / game / slot (lang/: a value, a player, a method with its arguments; the type of what each gives)
internal struct x_kind g_xkinds[4] = {
    { "player", XK_NUMBERED(XK_PLAYER), k_xk_player, (int)(sizeof k_xk_player / sizeof *k_xk_player) },
    { "game", XK_NUMBERED(XK_GAME), k_xk_game, (int)(sizeof k_xk_game / sizeof *k_xk_game) },
    { "slot", XK_NUMBERED(XK_SLOT), k_xk_slot, (int)(sizeof k_xk_slot / sizeof *k_xk_slot) },
    { "unit", XK_NUMBERED(XK_UNIT), k_xk_unit, (int)(sizeof k_xk_unit / sizeof *k_xk_unit) },
};
internal const struct x_kind *x_kind_of(int k) { return k >= XK_PLAYER && k <= XK_UNIT ? &g_xkinds[k - 1] : NULL; }
internal int x_root_kind(const char *name) {   // a text's players (me, them, enemy; lord one or none: a player?), the game
    int t = x_name_type(name);
    if (X_TBASE(t) == XK_PLAYER && !(t & X_LIST_OF)) return t;
    return !strcmp(name, "game") ? XK_GAME : 0;
}
internal int x_fn_kind(const char *name) { return !strcmp(name, "player") ? XK_PLAYER : X_ANY; }
// a mistake of the game's side, told: what is worked out fails (as one who isn't there does - expr.c x_obj_member)
internal PRINTF_LIKE(2, 3) void x_host_fail(struct xctx *c, const char *fmt, ...) {
    char t[200];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(t, sizeof t, fmt, ap);
    va_end(ap);
    x_error("%s: %.100s", t, c->top ? c->top : "");
    lstrcpynA(g_x_last_fail, t, sizeof g_x_last_fail);   // (a script's "stopped - ..." line)
    c->failed++;
    c->missed = 1;
}
// SETUP: a script's game.setup(() => ...) blocks run at the game's start before the players are made (test/scripts.c
// scripts_new_game, a new game only) - g_x_setup the game start's setup meanwhile, NULL else. There game.slots are the
// players to be (by number: the human and computer slots in order) and only they: a slot's persona and colour are set
// for persona_new_game, which plans with them (voice/persona.c g_setup_persona, g_setup_color)
internal ee_setup *g_x_setup;
internal int x_slot_index(int number) {   // the setup slot of player number, -1 none
    for (int s = 0, n = 0; g_x_setup && s < 16; s++) {
        int t = setup_slot_get_type(setup_slot_at(g_x_setup, s));
        if ((t == 0 || t == 1) && ++n == number) return s;
    }
    return -1;
}
internal void x_slot(struct xval *v, int number) {
    char t[16];
    wsprintfA(t, "slot %d", number);
    xstr(v, t);
    v->obj = number + 1;
    v->okind = XK_SLOT;
}
internal int x_slot_member(struct xctx *c, const struct xval *o, const char *name, struct xval *v) {
    int number = o->obj - 1, s = x_slot_index(number);
    if (s < 0) { x_host_fail(c, "slot %d: only in game.setup (before the game starts)", number); return 1; }
    int human = setup_slot_get_type(setup_slot_at(g_x_setup, s)) == 1;
    if (!strcmp(name, "number")) xnum(v, number);
    else if (!strcmp(name, "human")) xnum(v, human);
    else if (!strcmp(name, "persona")) { if (g_setup_persona[number] >= 0) xstr(v, g_personas[g_setup_persona[number]].file); }
    else if (!strcmp(name, "color")) {
        const struct pcolor *pc = &g_setup_color[number];
        if (pc->idx == -1) { double d[3] = { pc->rgb[0], pc->rgb[1], pc->rgb[2] }; xvec(v, d, 3); }
        else {
            int k = pc->idx >= 0 ? pc->idx : setup_slot_get_color(setup_slot_at(g_x_setup, s));
            if (k >= 0 && k < 17) xdual(v, k + 1, color_name(k));
        }
    } else return 0;
    return 1;
}
// SCRIPTS' EFFECTS: what a script's player does - p.win(), p.lose(), p.war(q), p.ally(q) - and its stock set (p.gold =
// 500). Each is told first (g_x_hooks.did: the log; a dry run's only told). A stance and a stock change at once - the
// stock written as the scenario's Set-Attribute does, without the AI's demand noticing; a win or a defeat waits for the
// next point between the simulation's steps (x_effects_flush, test/scripts.c script_triggers), where the game's own
// resign runs: as the scenario's Victory / Defeat (docs/triggers.md "2 Player")
internal int g_x_outcome[16];   // [player]: 1 wins, 2 loses - to be carried out
internal int x_did(struct xctx *c, const char *what) {   // 1: only told (or not a script's: a mistake, told)
    if (!c->script) { x_host_fail(c, "%s: a script's - a text changes nothing", what); return 1; }
    return g_x_hooks.did && g_x_hooks.did(what);
}
internal void x_player_do(struct xctx *c, int i, const char *name, const struct xval *a, int na, struct xval *v) {
    void *p = player_at(i);
    char t[80];
    xnothing(v);
    if (!p || is_out(p)) { x_host_fail(c, "player %d.%s(): not in the game", i, name); return; }
    if (!na) {   // win, lose
        wsprintfA(t, "player %d.%s()", i, name);
        if (!x_did(c, t)) g_x_outcome[i] = name[0] == 'w' ? 1 : 2;
        return;
    }
    void *q = x_player_of(&a[0]);
    if (!q || q == p) { x_host_fail(c, "player %d.%s(%.40s): %s", i, name, xtext_of(&a[0]), q ? "itself" : "not a player"); return; }
    int j = pidx(q), s = name[0] == 'w';   // 1 enemy, 0 allied
    wsprintfA(t, "player %d.%s(player %d)", i, name, j);
    if (!x_did(c, t) && stance_of(p, j) != s) set_stance(i, j, s);
}
internal void x_effects_flush(void) {
    for (int i = 1; i < 16; i++) {
        int w = g_x_outcome[i];
        g_x_outcome[i] = 0;
        void *p = w ? player_at(i) : NULL;
        if (!p || is_out(p)) continue;
        player_resign(p, w);         // its result (1 victorious, 2 defeated), the game's chat line
        game_player_out(w == 1, i);  // the map revealed, the game's end checked
    }
}
internal int x_player_set(struct xctx *c, const struct xval *o, const char *name, const struct xval *v) {
    int i = o->obj - 1, r = x_stock_index(name);
    void *p = player_at(i);
    char t[80];
    if (r < 0) return 0;
    if (!p || is_out(p)) { x_host_fail(c, "player %d.%s = ...: not in the game", i, name); return 0; }
    if (!v->has_n || v->ln || v->kind || !(v->n >= 0 && v->n <= 1e9) || v->n != (double)(long)v->n) {
        x_host_fail(c, "player %d.%s = %.40s: a stock is a whole number, 0 or more - round(...)", i, name, xtext_of(v));
        return 0;
    }
    wsprintfA(t, "player %d.%s = %ld", i, name, (long)v->n);
    if (!x_did(c, t)) player_stock_at(p)[r] = (int)v->n;
    return c->script;
}

// slot.persona = persona.fuchs, slot.color = color.red / rgb(40, 200, 120): for this game's planning (a computer's
// personality and colour; a human's colour one of the game's)
internal int x_obj_set(struct xctx *c, const struct xval *o, const char *name, const struct xval *v) {
    if (o->okind == XK_PLAYER) return x_player_set(c, o, name, v);
    if (o->okind == XK_UNIT) { x_host_fail(c, "a unit's %s can't be set", name); return 0; }   // (lang/unit.cs: none is [set] yet)
    int number = o->obj - 1, s = o->okind == XK_SLOT ? x_slot_index(number) : -1;
    if (s < 0) { x_host_fail(c, "%s.%s = ...: only in game.setup (before the game starts)", o->okind == XK_SLOT ? "slot" : "it", name); return 0; }
    int human = setup_slot_get_type(setup_slot_at(g_x_setup, s)) == 1;
    if (setup_get_mode(g_x_setup) != 0) { x_host_fail(c, "slot %d.%s: personalities and their colours are planned in random-map games only", number, name); return 0; }
    if (!human && is_vanilla_idx(number)) { x_host_fail(c, "slot %d.%s: player %d plays the game's own AI (vanilla) - its name and colour the game's", number, name, number); return 0; }
    if (!strcmp(name, "persona")) {
        int i = v->has_s && !v->has_n ? persona_find(v->s) : -1;
        if (human) { x_host_fail(c, "slot %d.persona: a human's - a personality is a computer's", number); return 0; }
        if (i < 0) { x_host_fail(c, "slot %d.persona = %.40s: no personality - persona.<its file> (aimod_ai)", number, xtext_of(v)); return 0; }
        g_setup_persona[number] = i;
        return 1;
    }
    if (!strcmp(name, "color")) {
        if (v->kind == XV_VEC && v->vn == 3 && !human) {
            struct pcolor pc = { -1, { (float)v->vc[0], (float)v->vc[1], (float)v->vc[2] } };
            for (int k = 0; k < 3; k++) if (!(v->vc[k] >= 0 && v->vc[k] <= 1)) { x_host_fail(c, "slot %d.color: rgb(r, g, b) of 0 to 255 each", number); return 0; }
            g_setup_color[number] = pc;
            return 1;
        }
        int k = v->has_n && !v->kind && !v->ln ? (int)v->n - 1 : -1;
        if (k < 0 || k >= 17 || (double)(k + 1) != v->n) {
            x_host_fail(c, "slot %d.color = %.40s: color.<name>%s", number, xtext_of(v), human ? " (a human's: one of the game's)" : " or rgb(r, g, b)");
            return 0;
        }
        if (human) setup_slot_set_color(setup_slot_at(g_x_setup, s), k);   // the game gives a human its slot's
        else { g_setup_color[number].idx = k; }
        return 1;
    }
    return 0;
}
internal int x_unit_member_of(struct xctx *c, const struct xval *o, const char *name, struct xval *v);   // units.c
internal int x_units_of(struct xctx *c, int pl, const struct xval *a, int na, struct xval *v);
internal void x_unit_by_id(struct xctx *c, const struct xval *id, struct xval *v);
// o.name, o.name(a ...) (expr.c x_obj_member: a member of its kind, its arguments counted)
internal int x_obj_member_of(struct xctx *c, const struct xval *o, const char *name, const struct xval *a, int na, struct xval *v) {
    if (o->okind == XK_SLOT) return x_slot_member(c, o, name, v);
    if (o->okind == XK_UNIT) return x_unit_member_of(c, o, name, v);
    if (o->okind == XK_GAME && g_x_setup && strcmp(name, "slots") && strcmp(name, "slot")) {
        x_host_fail(c, "game.%s: not before the game starts - game.setup has game.slots, game.slot(n)", name);
        return 1;
    }
    if (o->okind == XK_GAME && !g_x_setup && (!strcmp(name, "slots") || !strcmp(name, "slot"))) {
        x_host_fail(c, "game.%s: only in game.setup (before the game starts) - game.players after", name);
        return 1;
    }
    if (o->okind == XK_PLAYER) {
        void *p = player_at(o->obj - 1);
        const struct xmember *m = x_find_member(name, (int)strlen(name));
        if (!m) return 0;
        if (!strcmp(name, "units")) x_units_of(c, o->obj - 1, a, na, v);
        else if (m->kind == XM_PLAYER) x_member(p, m, v);
        else if (m->kind == XM_LINK) x_player(v, x_lord(p), "lord");
        else if (m->kind == XM_FILTER) x_count_of(c, p, "player", name, &a[0], v);
        else if (m->kind == XM_DO) x_player_do(c, o->obj - 1, name, a, na, v);
        else x_method_of(c, p, "player", name, m, a, na, v);
        return 1;
    }
    if (o->okind != XK_GAME) return 0;
    double now = g_x_hooks.now ? g_x_hooks.now() : game_time() / 1000.0;
    if (!strcmp(name, "time")) xnum(v, now);
    else if (!strcmp(name, "minutes")) xnum(v, (double)(long long)(now / 60));
    else if (!strcmp(name, "player")) {
        double n = xto_num(&a[0]);
        if (!a[0].has_n || !x_player_n((int)n, v) || (double)(int)n != n) x_host_fail(c, "game.player(%.40s): no such player", xtext_of(&a[0]));
    } else if (!strcmp(name, "timer")) {
        const char *why = "game.timer(step, f): a time - 20s";
        if (!c->script) x_host_fail(c, "game.timer: only in a script");
        else if (!a[0].w || !a[0].has_n || a[0].ln || !x_timer_make(c, a[0].n, &a[1], v, &why)) x_host_fail(c, "%s", why);
    } else if (!strcmp(name, "setup")) {   // (its blocks ran at the game's start - here, in their place, nothing)
    } else if (!strcmp(name, "unit")) x_unit_by_id(c, &a[0], v);
    else if (!strcmp(name, "slots")) {
        int n = 0;
        while (x_slot_index(n + 1) >= 0) n++;
        struct xval *it = n > 0 ? x_alloc((size_t)n * sizeof *it) : NULL;
        if (!it) return 1;
        for (int j = 1; j <= n; j++) x_slot(&it[j - 1], j);
        v->it = it; v->ln = v->w = n;
    } else if (!strcmp(name, "slot")) {
        double n = xto_num(&a[0]);
        if (!a[0].has_n || (double)(int)n != n || x_slot_index((int)n) < 0) x_host_fail(c, "game.slot(%.40s): no such player", xtext_of(&a[0]));
        else x_slot(v, (int)n);
    } else if (!strcmp(name, "players")) {
        int n = nplayers() - 1;
        struct xval *it = n > 0 ? x_alloc((size_t)n * sizeof *it) : NULL;
        if (!it) return 1;
        for (int j = 1; j <= n; j++) x_player(&it[j - 1], player_at(j), "player");
        v->it = it; v->ln = v->w = n;
    } else return 0;
    return 1;
}
// game.<member>[.<member> ...] by name (x_builtin, x_method): the game's own, through its kind's members
internal int x_game_path(struct xctx *c, const char *name, const struct xval *a, int na, int call, struct xval *v) {
    if (strncmp(name, "game", 4) || (name[4] && name[4] != '.')) return 0;
    struct xval o;
    x_game(&o);
    const char *s = name[4] ? name + 5 : NULL;
    while (s) {
        char m[40];
        int n = (int)strcspn(s, ".");
        if (n >= (int)sizeof m || !o.obj) return 0;
        lstrcpynA(m, s, n + 1);
        const struct x_kmember *km = x_kind_member(o.okind, m);
        int last = !s[n];
        if (!km || (km->amin >= 0) != (call && last)) return 0;
        if (call && last && (na < km->amin || na > km->amax)) return 0;
        if (!v) { if (last) return 1; if (!(km->result > 0 && km->result < X_LIST_OF)) return 0; o.okind = km->result; o.obj = 1; s += n + 1; continue; }
        struct xval r;
        xnothing(&r);
        if (!x_obj_member_of(c, &o, m, last ? a : NULL, last ? na : 0, &r)) return 0;
        o = r;
        s = last ? NULL : s + n + 1;
    }
    if (v) *v = o;
    return 1;
}
