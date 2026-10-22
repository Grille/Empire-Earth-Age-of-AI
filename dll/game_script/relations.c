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
// The tables also make the documentation: scripts/gen_eeai_readme.py writes them into aimod_ai\README.md.
enum { XM_PLAYER, XM_LINK, XM_REL, XM_FILTER };   // a player's own, a player (them.lord), A.<relation>(B), A.count(filter)
struct xmember { const char *name; int kind; const char *doc; };
internal const struct xmember k_xmembers[] = {
    { "name", XM_PLAYER, "its name - the same as the player itself in a text: {them} = {them.name}" },
    { "soldiers", XM_PLAYER, "its soldiers (all numbers of units are counted every 10 seconds)" },
    { "citizens", XM_PLAYER, "its citizens" },
    { "population", XM_PLAYER, "its units: soldiers and citizens - to scale a number to the game's stage: population_limit, max_population" },
    { "buildings", XM_PLAYER, "its buildings" },
    { "towers", XM_PLAYER, "its towers" },
    { "walls", XM_PLAYER, "its wall pieces" },
    { "count", XM_FILTER, "its units and buildings of a filter: unit.<identifier> (one type), kind.<kind> (the game's family, every epoch's: kind.towers, kind.citizen), or a list of them - any of them: them.count(kind.towers), me.count([kind.tank, kind.tank_strong]) (docs/units.md: the identifiers and kinds)" },
    { "centers", XM_PLAYER, "its Town Centers and Capitols" },
    { "strength", XM_PLAYER, "its fighting strength (its soldiers' hit points, 30 a building) - to compare: them.strength > me.strength * 2" },
    { "rank", XM_PLAYER, "its place by strength among the players still in, 1 = the strongest" },
    { "epoch", XM_PLAYER, "its epoch: the name in a text, 1 (Prehistoric) .. 15 (Space) as a number - compare it with epoch.middle etc." },
    { "color", XM_PLAYER, "its colour: the name in a text (in the game's language), 1 (blue) .. 17 as a number - compare it with color.red etc." },
    { "civilization", XM_PLAYER, "its civilization's name, as the game shows it (Karthago in a German game; an AI's personality may give it another: civilization=) - nothing when none is chosen" },
    { "civilization_en", XM_PLAYER, "its civilization in the game's database, in English in every language (Carthage) - for conditions: them.civilization_en == \"Carthage\"" },
    { "alive", XM_PLAYER, "1 while it is still in the game" },
    { "human", XM_PLAYER, "1 for the human player at this PC" },
    { "vassals", XM_PLAYER, "how many have capitulated to it" },
    { "allies", XM_PLAYER, "how many are allied with it (both ways), still in - allies alone: mine" },
    { "enemies", XM_PLAYER, "how many it treats as enemies, still in - enemies alone: mine" },
    { "aggression", XM_PLAYER, "its traits, 0-100 (50 without a personality)" },
    { "loyalty", XM_PLAYER, "" }, { "pride", XM_PLAYER, "" }, { "greed", XM_PLAYER, "" }, { "forgiveness", XM_PLAYER, "" },
    { "caution", XM_PLAYER, "" }, { "talk", XM_PLAYER, "" },
    { "lord", XM_LINK, "its lord, the player it capitulated to - a player: them.lord.name, lord == them; nothing when it has none" },
    { "food", XM_PLAYER, "its stock - another's too (the AI's spies): them.gold" },
    { "wood", XM_PLAYER, "" }, { "stone", XM_PLAYER, "" }, { "gold", XM_PLAYER, "" }, { "iron", XM_PLAYER, "" },
    { "stock", XM_PLAYER, "the five resources together - to scale amounts: them.tribute(me) > me.stock / 2" },
    { "opinion", XM_REL, "-100..100: what A thinks of B - in a text in words: hostile, unfriendly, indifferent, friendly, trusting (round(opinion): the number). An AI's opinion; a human's of an AI is a guess from what the AI did to them (war, a betrayal, peace, tribute, a refusal, their losses), nothing of another human" },
    { "at_war", XM_REL, "1 when A and B are enemies, both of them" },
    { "allied", XM_REL, "1 when A and B are allied, both of them" },
    { "hostile", XM_REL, "1 when A treats B as an enemy (whatever B does)" },
    { "war_minutes", XM_REL, "how long the war of A and B has lasted: from both enemies until both are allied again (a peace offer doesn't end it; in peace_accept: the war just ended) - nothing without a war" },
    { "alliance_minutes", XM_REL, "how long A and B have been allied - nothing when they are not" },
    { "betrayals", XM_REL, "how often A turned on B while they were allied (an alliance of 2 minutes or more: a peace broken at once is war, not betrayal) - (B, minutes): in the last minutes; in betrayed the betrayals before the one it is about" },
    { "refusals", XM_REL, "how often A refused B's offers of peace or alliance - (B, minutes): in the last minutes" },
    { "ignored", XM_REL, "B's alliance offers A let run out, in a row - in offer_withdrawn those before the one withdrawn" },
    { "tribute", XM_REL, "resources A has paid B in this game - (B, minutes): in the last minutes, them.tribute(me, 15); in tribute_thanks / tribute_small what was paid before this tribute" },
    { "near", XM_REL, "A's soldiers within 20 tiles of B's base now, while B treats A as an enemy (an AI's base)" },
    { "in_town", XM_REL, "1 when A's army is in B's town: its soldiers have stayed near B's base for a while (an AI's base)" },
    { "attack_minutes", XM_REL, "minutes since A's last attack wave set out against B - nothing if none (an AI's waves)" },
    { "kills", XM_REL, "B's units and buildings A has destroyed - (B, minutes): in the last minutes (whole minutes, the current one counts); a filter last picks which, as count's: unit.<identifier>, kind.<kind>, a list of them - or one of mod/units.eeai's lists (units, army, airforce, marine, military, civilians, buildings): them.kills(me, units), me.kills(them, 10, kind.tank), them.kills(me, 20, buildings)" },
};
#define N_XMEMBERS (int)(sizeof k_xmembers / sizeof *k_xmembers)
internal const struct xmember k_xglobals[] = {
    { "me", XM_PLAYER, "the AI that speaks - its name in a text; me.soldiers ..." },
    { "them", XM_PLAYER, "the player it speaks to or about; in a system line you, the local player" },
    { "enemy", XM_PLAYER, "a third player: the enemy in an alliance offer, the ally or lord in some lines" },
    { "lord", XM_PLAYER, "my lord (me.lord), the player I capitulated to - in every line; nothing when I have none" },
    { "opinion", XM_REL, "me.opinion(them): what I think of them" },
    { "at_war", XM_REL, "me.at_war(them)" },
    { "allied", XM_REL, "me.allied(them)" },
    { "minutes", XM_PLAYER, "the game time in minutes" },
    { "players", XM_PLAYER, "the players still in, me among them" },
    { "population_limit", XM_PLAYER, "the game's unit limit per player (its setting)" },
    { "max_population", XM_PLAYER, "the largest population of a player still in - kills > max_population / 2: many for this stage of the game" },
    { "language", XM_PLAYER, "the game's language: de, en, fr, ..." },
    { "offer_minutes", XM_PLAYER, "how long an offer stands, in minutes ([all] offer_minutes in aimod.ini)" },
    { "llm_equal", XM_PLAYER, "1 when the language model is told nothing of who plays the computer ([llm] equal)" },
    { "llm_constrained", XM_PLAYER, "1 when the language model decides within the rules' limits ([llm] constrained)" },
};
internal int g_llm_equal, g_llm_constrained;   // llm.c: [llm] equal, constrained
#define N_XGLOBALS (int)(sizeof k_xglobals / sizeof *k_xglobals)
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
}
internal int x_player_n(int n, struct xval *v) {   // expr.c: player(n) in a script - player n, 0: none there
    void *p = n >= 1 && n < nplayers() ? player_at(n) : NULL;
    if (!p) return 0;
    x_player(v, p, "");
    return 1;
}
internal void *x_player_of(const struct xval *v) {   // the player a value is - or names (a placeholder: {ally})
    if (!v->w) return NULL;
    if (v->obj) return player_at(v->obj - 1);
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
        if (!strcmp(n, "stock")) r += FIELD(px, PL_STOCK + (k + 2) * 4, int);
        else if (!strcmp(n, res[k])) { xnum(v, FIELD(px, PL_STOCK + (k + 2) * 4, int)); return; }
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
// the relations that count events, with a time window in minutes as a second argument: them.tribute(me, 15)
internal int x_windowed(const char *n) {
    return !strcmp(n, "tribute") || !strcmp(n, "refusals") || !strcmp(n, "betrayals") || !strcmp(n, "kills");
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
internal int x_mine(const char *name) {   // a member that alone is mine: epoch, allies, enemies, the traits
    int mine = !strcmp(name, "epoch") || !strcmp(name, "allies") || !strcmp(name, "enemies");
    for (int t = 0; t < T_N; t++) mine |= !strcmp(name, k_trait_keys[t]);
    return mine;
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
    if (dot) {   // <player>.<member>
        void *p = x_path(c, name, (int)(dot - name), &ok, who);
        const struct xmember *m = ok ? x_find_member(dot + 1, (int)strlen(dot + 1)) : NULL;
        if (!m || m->kind == XM_REL || m->kind == XM_FILTER) return 0;   // a relation needs its other player: them.opinion(me)
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
    if (x_is_value(name)) return 1;
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
    return m && m->kind != XM_REL;
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
internal int x_count(struct xctx *c, const char *name, const char *dot, const struct xval *f, struct xval *v) {
    int ok = 1;
    char who[24] = "me";
    void *p = dot ? x_path(c, name, (int)(dot - name), &ok, who) : (x_players(c), c->pl);
    if (!ok) return 0;
    if (!v) return 1;
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
internal int x_method(struct xctx *c, const char *name, const struct xval *a, int na, struct xval *v) {
    const char *dot = strrchr(name, '.');
    const char *rel = dot ? dot + 1 : name;
    const struct xmember *m = x_find_member(rel, (int)strlen(rel));
    if (m && m->kind == XM_FILTER) return na == 1 && x_count(c, name, dot, a, v);
    // (B) or, counting events, (B, minutes) - kills also with a filter last: (B, kind.tank), (B, 10, kind.tank)
    int kr = !strcmp(rel, "kills");
    if (!m || m->kind != XM_REL || na < 1 || na > (kr ? 3 : x_windowed(rel) ? 2 : 1)) return 0;
    int ok = 1;
    char who[24] = "me";
    void *pa = dot ? x_path(c, name, (int)(dot - name), &ok, who) : (x_players(c), c->pl);
    if (!ok) return 0;
    if (!v) return 1;
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
