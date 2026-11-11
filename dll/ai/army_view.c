// ARMY VIEW (the player 2026-11-11: "maybe on a high level it should only see armies as one entity, how strong is it,
// what is it good against, has it siege etc. the ai than can command reinforce or split these armies as it needs
// without ever looking at what is in it"; on how many: "if its smart enough it should be allowed as much as it needs
// ... we should get away from hard limits"): what an army is, summed from its members, for the deciding parts to read
// instead of the members. Step 1: the force's summary (armies.c), written to the log each minute; nothing decides on
// it yet. Included by eeaimod.c after stats.c (st_shot_t, st_range ...) and before armies.c.
//
// What it is good against: its damage a second at each family (the game's "Family ID", kind.*: the armour and bonus
// class its hit table reads as the target's row), from the game's own numbers (stats.c st_shot_t: attack x the damage
// table x the hit factor, the time of a shot with ST_SHOT_OVERHEAD as the fight estimate takes it) at one type of that
// family, no armour - armour is the target's, a summary of the army can't know it. Siege and rams are the roles the
// game's data gives (unit_ids.h UTR_SIEGE, UTR_RAM). Members of a type share their owner's stats of it, so the work is
// once per type, not per member.

struct av_sum {
    int n, aboard, siege, rams, ranged;   // members alive; of them aboard a transport; of role siege, of role ram; shooting from 2 tiles on
    float hp, hp_full;            // hit points now and at full health
    float reach, slowest, speed;  // the longest range; the slowest mover's and the mean speed (tiles/s)
    ivec2 mid; float spread;      // the members' middle and their distance from it (root mean square)
    float vs[UNIT_KINDS];         // damage a second at each family
    double sx, sy, sxx, syy, ssp;
    unsigned short cnt[G_TYPES_N];   // members by type (the per-type work in av_done)
    ee_unit *rep[G_TYPES_N];
};
internal ee_unit_type *av_kind_type(int k) {   // a type of family k, NULL: none in the game's data
    unit_types_load();
    return k >= 0 && k < UNIT_KINDS && g_kind_first[k] < g_kind_first[k + 1] ? type_data(g_kind_types[g_kind_first[k]]) : NULL;
}
internal void av_clear(struct av_sum *s) { memset(s, 0, sizeof *s); }
internal void av_add(struct av_sum *s, ee_unit *u) {
    ee_unit_type *td = u ? unit_get_type_data(u) : NULL;
    int t = td ? unit_type_get_id(td) : -1, hp = u ? unit_get_hp(u) : 0;
    if (t < 0 || t >= G_TYPES_N || hp <= 0) return;
    ee_unit_stats *st = unit_get_stats(u);
    ivec2 p = unit_get_tile(u);
    float rg = st_range(u), sp = st_speed(u);
    s->n++;
    if (unit_get_x(u) < 0) s->aboard++;   // aboard a transport: no place on the map (the middle and spread are those ashore's)
    else { s->sx += p.x; s->sy += p.y; s->sxx += (double)p.x * p.x; s->syy += (double)p.y * p.y; }
    s->hp += (float)hp; s->hp_full += (float)st_maxhp(td, st);
    if (rg > s->reach) s->reach = rg;
    if (rg >= 2.0f) s->ranged++;
    if (sp > 0 && (s->slowest <= 0 || sp < s->slowest)) s->slowest = sp;
    s->ssp += sp;
    s->siege += ut_role(t, UTR_SIEGE) != 0; s->rams += ut_role(t, UTR_RAM) != 0;
    if (s->cnt[t] < 0xFFFF) s->cnt[t]++;
    if (!s->rep[t]) s->rep[t] = u;
}
internal void av_done(struct av_sum *s) {
    if (!s->n) return;
    int ashore = s->n - s->aboard;
    double mx = ashore ? s->sx / ashore : -1, my = ashore ? s->sy / ashore : -1, v = ashore ? s->sxx / ashore - mx * mx + s->syy / ashore - my * my : 0;
    s->mid = iv2((int)mx, (int)my);
    s->spread = v > 0 ? (float)sqrt(v) : 0.0f;
    s->speed = (float)(s->ssp / s->n);
    for (int t = 0; t < G_TYPES_N; t++) {
        ee_unit *u = s->rep[t];
        if (!s->cnt[t] || !u) continue;
        ee_unit_type *td = unit_get_type_data(u);
        ee_unit_stats *st = unit_get_stats(u);
        int pi = st_pidx(u), own = unit_get_reload(u);
        float shot = (own > 0 ? own / 1000.0f : st_reload_t(td, st)) + ST_SHOT_OVERHEAD;
        for (int k = 0; k < UNIT_KINDS; k++) {
            ee_unit_type *tt = av_kind_type(k);
            if (tt) s->vs[k] += (float)s->cnt[t] * (float)st_shot_t(td, st, pi, tt, NULL, pi) / shot;
        }
    }
}
// the families of what pl sees of its enemies now - their fighters and buildings (a bit each; UNIT_KINDS fits in 64)
internal uint64_t av_enemy_kinds(ee_player *pl) {
    uint64_t m = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_player *o = unit_get_owner(it.u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !cv_alive(it.u) || !(cv_mil(it.td) || fr_building(it.td)) || !seen_by(it.u, pl)) continue;
        int f = unit_type_get_family(it.td);
        if (f >= 0 && f < UNIT_KINDS && f < 64) m |= 1ull << f;
    }
    return m;
}
// the summary as a line: what it is, then its damage a second at each family of the enemy's it sees, the most first
internal void av_line(const struct av_sum *s, uint64_t kinds, char *o, int cap) {
    int w = snprintf(o, (size_t)cap, "%d alive (%d aboard), hp %.0f of %.0f, at %d,%d spread %.1f, reach %.1f, slowest %.2f (mean %.2f), siege %d, rams %d, ranged %d; damage a second at the enemy's:",
                     s->n, s->aboard, s->hp, s->hp_full, s->mid.x, s->mid.y, s->spread, s->reach, s->slowest, s->speed, s->siege, s->rams, s->ranged);
    uint64_t left = kinds;
    while (left && w > 0 && w < cap - 32) {
        int b = -1;
        for (int k = 0; k < UNIT_KINDS && k < 64; k++) if (left >> k & 1 && (b < 0 || s->vs[k] > s->vs[b])) b = k;
        if (b < 0) break;
        left &= ~(1ull << b);
        w += snprintf(o + w, (size_t)(cap - w), " %s %.0f", k_kind_ident[b], s->vs[b]);
    }
    if (!kinds && w > 0 && w < cap - 8) snprintf(o + w, (size_t)(cap - w), " none seen");
}

// --- the armies: a list a player, as many as it needs ---------------------------------------------------------------
// Step 2: an army other than the force (armies.c, army 0: its members' own state stays there) is a record here - its
// kind, what it is for, its members by pointer and serial (a pointer alone could be another unit after a death) - made,
// filled, emptied and ended by the parts that use it: the force's soldiers the landing has are one (armies.c
// armies_lend: a split from the force, ar_lent_back: a merge back). No count limit: the list and the member lists
// grow as they need. Not saved, as the lists they replace: a load's first tick takes the held back into the force.
enum { AV_FORCE = 0, AV_LANDING = 1, AV_RAID = 2 };
internal const char *const k_av_kind[] = { "the force", "the landing's", "a raid party" };
struct av_member { ee_unit *u; int id; };
struct av_army {
    int id, kind; struct av_member *m; int n, cap; uint32_t made_t;
    ivec2 dest; int sent, clear; uint32_t look_t; float sent_hp;   // a mission's: where it goes, members and hit points it set out with, looks with nothing there, its last look
};
internal struct { struct av_army *a; int n, cap, next; } g_av[16];
internal struct av_army *av_of(unsigned idx, int kind) {   // the player's army of that kind (the first), NULL: none
    for (int k = 0; k < g_av[idx & 15].n; k++) if (g_av[idx & 15].a[k].kind == kind) return &g_av[idx & 15].a[k];
    return NULL;
}
internal struct av_army *av_new(unsigned idx, int kind, uint32_t now) {   // NULL only without memory
    idx &= 15;
    if (g_av[idx].n == g_av[idx].cap) {
        int cap = g_av[idx].cap ? 2 * g_av[idx].cap : 8;
        struct av_army *na = realloc(g_av[idx].a, (size_t)cap * sizeof *na);
        if (!na) return NULL;
        g_av[idx].a = na; g_av[idx].cap = cap;
    }
    struct av_army *a = &g_av[idx].a[g_av[idx].n++];
    memset(a, 0, sizeof *a);
    a->id = ++g_av[idx].next; a->kind = kind; a->made_t = now ? now : 1;
    return a;
}
internal int av_put(struct av_army *a, ee_unit *u) {   // a member at the end; 0 only without memory
    if (a->n == a->cap) {
        int cap = a->cap ? 2 * a->cap : 32;
        struct av_member *nm = realloc(a->m, (size_t)cap * sizeof *nm);
        if (!nm) return 0;
        a->m = nm; a->cap = cap;
    }
    a->m[a->n].u = u; a->m[a->n++].id = unit_get_serial(u);
    return 1;
}
internal int av_live(const struct av_member *m, ee_player *pl) {   // still the unit it was, pl's and alive
    return unit_get_serial(m->u) == m->id && unit_get_owner(m->u) == pl && unit_get_hp(m->u) > 0;
}
internal void av_end(unsigned idx, struct av_army *a) {   // the army gone (its members already given elsewhere or dead)
    idx &= 15;
    int k = (int)(a - g_av[idx].a);
    if (k < 0 || k >= g_av[idx].n) return;
    free(a->m);
    g_av[idx].a[k] = g_av[idx].a[--g_av[idx].n];
}
internal void av_clear_player(unsigned idx) {
    idx &= 15;
    for (int k = 0; k < g_av[idx].n; k++) free(g_av[idx].a[k].m);
    free(g_av[idx].a);
    memset(&g_av[idx], 0, sizeof g_av[idx]);
}
internal void av_new_game(void) { for (unsigned i = 0; i < 16; i++) av_clear_player(i); }
