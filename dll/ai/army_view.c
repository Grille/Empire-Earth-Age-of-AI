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
    ee_unit *rep[G_TYPES_N];         // a live member of the type (its own reload), NULL: remembered ones only
    ee_unit_stats *rst[G_TYPES_N];   // the owner's stats of the type and the owner, of its first member
    signed char rpi[G_TYPES_N];
};
internal ee_unit_type *av_kind_type(int k) {   // a type of family k, NULL: none in the game's data
    unit_types_load();
    return k >= 0 && k < UNIT_KINDS && g_kind_first[k] < g_kind_first[k + 1] ? type_data(g_kind_types[g_kind_first[k]]) : NULL;
}
internal void av_clear(struct av_sum *s) { memset(s, 0, sizeof *s); }
// a member of type td, of owner pi with its stats st, hp hit points, at p (x < 0: aboard a transport, or where it is not
// known); u the unit when it is a live one (NULL: a remembered one - the enemy's armies)
internal void av_add_one(struct av_sum *s, ee_unit *u, ee_unit_type *td, ee_unit_stats *st, int pi, int hp, ivec2 p) {
    int t = td ? unit_type_get_id(td) : -1;
    if (t < 0 || t >= G_TYPES_N || hp <= 0) return;
    float rg = st ? unit_stats_get_range(st) : unit_type_get_range(td), sp = st ? unit_stats_get_speed(st) : 0.0f;
    s->n++;
    if (p.x < 0) s->aboard++;   // aboard a transport: no place on the map (the middle and spread are those ashore's)
    else { s->sx += p.x; s->sy += p.y; s->sxx += (double)p.x * p.x; s->syy += (double)p.y * p.y; }
    s->hp += (float)hp; s->hp_full += (float)st_maxhp(td, st);
    if (rg > s->reach) s->reach = rg;
    if (rg >= 2.0f) s->ranged++;
    if (sp > 0 && (s->slowest <= 0 || sp < s->slowest)) s->slowest = sp;
    s->ssp += sp;
    s->siege += ut_role(t, UTR_SIEGE) != 0; s->rams += ut_role(t, UTR_RAM) != 0;
    if (!s->cnt[t]) { s->rst[t] = st; s->rpi[t] = (signed char)pi; }
    if (s->cnt[t] < 0xFFFF) s->cnt[t]++;
    if (!s->rep[t]) s->rep[t] = u;
}
internal void av_add(struct av_sum *s, ee_unit *u) {
    ee_unit_type *td = u ? unit_get_type_data(u) : NULL;
    if (td) av_add_one(s, u, td, unit_get_stats(u), st_pidx(u), unit_get_hp(u), unit_get_x(u) < 0 ? iv2(-1, -1) : unit_get_tile(u));
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
        ee_unit_type *td = !s->cnt[t] ? NULL : u ? unit_get_type_data(u) : type_data(t);
        if (!td) continue;
        ee_unit_stats *st = u ? unit_get_stats(u) : s->rst[t];
        int pi = u ? st_pidx(u) : s->rpi[t], own = u ? unit_get_reload(u) : 0;
        float shot = (own > 0 ? own / 1000.0f : st_reload_t(td, st)) + ST_SHOT_OVERHEAD;
        for (int k = 0; k < UNIT_KINDS; k++) {
            ee_unit_type *tt = av_kind_type(k);
            if (tt) s->vs[k] += (float)s->cnt[t] * (float)st_shot_t(td, st, pi, tt, NULL, pi) / shot;
        }
    }
}
// FIELDS OF OPERATION (the player 2026-11-11: "ships/fleets under the same system, just another field of operation ...
// space on top as a layer"): land, sea, air and space, a unit's from its class in the game's data (class_id: a data mod's
// new ship is a ship), an army's from its kind. What an army is good against is listed at the enemy it can get to: a land
// army what stands on its land (its middle's continent) and what flies; a sea army what sails on its sea (its middle's
// ocean), what flies, and what stands within its reach of that sea; air and space everything.
enum { AVF_LAND = 1, AVF_SEA = 2, AVF_AIR = 4, AVF_SPACE = 8 };
internal int av_field_of(ee_unit_type *td) {
    switch (unit_type_get_class_id(td)) {
    case CLS_NAVAL: case CLS_SUBMERGED_NAVAL: case CLS_NAVAL_TRANSPORT: case CLS_NAVAL_BUILDING: case CLS_NAVAL_CITIZEN: case CLS_SEA_ANIMAL: return AVF_SEA;
    case CLS_AIRPLANE: case CLS_HELICOPTER: case CLS_HELICOPTER_TRANSPORT: case CLS_HELICOPTER_PRIEST: case CLS_AIR_ANIMAL: return AVF_AIR;
    case CLS_SPACESHIP: case CLS_SPACE_BUILDING: case CLS_SPACE_TRANSPORT: case CLS_PLANET_SPACESHIP: return AVF_SPACE;
    default: return AVF_LAND;
    }
}
// what pl sees of its enemies now - their fighters and buildings: family, field, land and sea (map_area 2 and 1) - once
// for all of its armies' lines (a list that grows as it needs)
struct av_foe { int kind, field; unsigned land, sea; ivec2 p; };
internal struct { struct av_foe *f; int n, cap; } g_avf;
internal void av_foes_scan(ee_player *pl) {
    g_avf.n = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_player *o = unit_get_owner(it.u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !cv_alive(it.u) || !(cv_mil(it.td) || fr_building(it.td)) || !seen_by(it.u, pl)) continue;
        int k = unit_type_get_family(it.td);
        if (k < 0 || k >= UNIT_KINDS || k >= 64) continue;
        if (g_avf.n == g_avf.cap) {
            int cap = g_avf.cap ? 2 * g_avf.cap : 256;
            struct av_foe *nf = realloc(g_avf.f, (size_t)cap * sizeof *nf);
            if (!nf) return;
            g_avf.f = nf; g_avf.cap = cap;
        }
        ivec2 p = unit_get_tile(it.u);
        struct av_foe *e = &g_avf.f[g_avf.n++];
        e->kind = k; e->field = av_field_of(it.td); e->p = p; e->land = map_area(p, 2); e->sea = map_area(p, 1);
    }
}
internal int av_near_sea(ivec2 p, unsigned sea, int r) {   // a tile of that sea within r of p (8 ways, at 1, r/2 and r)
    static const int dx[8] = { 1, 1, 0, -1, -1, -1, 0, 1 }, dy[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
    int ds[3] = { 1, r / 2 > 1 ? r / 2 : 1, r > 1 ? r : 1 };
    for (int j = 0; j < 3; j++)
        for (int w = 0; w < 8; w++) if (map_area(iv2(p.x + dx[w] * ds[j], p.y + dy[w] * ds[j]), 1) == sea) return 1;
    return 0;
}
// the families of the enemy (av_foes_scan) an army of that field, summed in s, can get to (a bit each)
internal uint64_t av_kinds_for(const struct av_sum *s, int field) {
    int ashore = s->n - s->aboard;
    unsigned land = ashore ? map_area(s->mid, 2) : 0, sea = ashore ? map_area(s->mid, 1) : 0;   // all aboard, or nowhere known: every land, every sea
    uint64_t m = 0;
    for (int i = 0; i < g_avf.n; i++) {
        const struct av_foe *e = &g_avf.f[i];
        int ok = (field & (AVF_AIR | AVF_SPACE)) || e->field == AVF_AIR;
        if (!ok && (field & AVF_LAND)) ok = e->field == AVF_LAND && (!land || e->land == land);
        if (!ok && (field & AVF_SEA)) ok = e->field == AVF_SEA ? !sea || e->sea == sea : e->field == AVF_LAND && (!sea || av_near_sea(e->p, sea, (int)s->reach + 1));
        if (ok) m |= 1ull << e->kind;
    }
    return m;
}

// THE ENEMY'S ARMIES (army view step 5, the log only so far): what an AI knows of its enemies' forces, as armies like its
// own. Its memory (intel.c: every enemy soldier, ship and aircraft seen within the last 10 minutes - type, hit points, the
// place it was seen; those seen now among them) grouped by owner and field: members within AVE_LINK tiles of another on
// the same land (ships: the same sea; aircraft anywhere) are one army - linked one by one, so a column on the march stays
// one; members whose place is no longer known (looked at there, gone) are an army without a place of their owner and
// field. Each with its summary (av_sum, by its owner's stats: what the AI would see of such a unit), when it was last
// seen (its newest member) and where. Built on asking, at most every 10 s; nothing of it is saved (the memory is).
#define AVE_LINK 10
internal const char *const k_av_field_name[] = { "", "land", "sea", "", "air", "", "", "", "space" };
struct av_erec { int owner, type, field, x, y, hp, army; unsigned area; uint32_t t; };
struct av_enemy { int owner, field, first, n; unsigned area; uint32_t seen_t; ivec2 mid; float hp; };
internal struct { struct av_erec *r; int nr, rcap; struct av_enemy *a; int n, cap; int *up; uint32_t built_t; } g_ave[16];
internal void avx_update(ee_player *pl, unsigned idx, uint32_t now);   // below: WHAT THE ENEMY FIELDS
internal int ave_root(int *up, int i) { while (up[i] != i) { up[i] = up[up[i]]; i = up[i]; } return i; }
internal int ave_cmp(const void *x, const void *y) {
    const struct av_erec *a = x, *b = y;
    return (a->army > b->army) - (a->army < b->army);   // unique: the army's lowest record, then the memory's order
}
internal void av_enemy_build(ee_player *pl, uint32_t now) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct intel *in = &g_intel[idx];
    __typeof__(g_ave[0]) *e = &g_ave[idx];
    if (e->built_t && now >= e->built_t && now - e->built_t < 10000) return;
    e->built_t = now ? now : 1;
    e->nr = e->n = 0;
    for (int k = 0; k < in->n; k++) {
        const struct intel_rec *r = &in->r[k];
        ee_unit_type *td = r->kind >= 1 && r->kind <= 3 && in->now - r->t <= 600000 ? type_data(r->type) : NULL;
        ee_player *o = td ? player_at(r->owner) : NULL;
        if (!o || !is_enemy(pl, o) || r->hp <= 0) continue;
        if (e->nr == e->rcap) {
            int cap = e->rcap ? 2 * e->rcap : 256;
            struct av_erec *nr = realloc(e->r, (size_t)cap * sizeof *nr);
            int *nu = realloc(e->up, (size_t)cap * sizeof *nu);
            if (nu) e->up = nu;
            if (!nr || !nu) return;
            e->r = nr; e->rcap = cap;
        }
        struct av_erec *q = &e->r[e->nr];
        q->owner = r->owner; q->type = r->type; q->field = av_field_of(td); q->x = r->x; q->y = r->y; q->hp = r->hp; q->t = r->t;
        q->area = r->x < 0 || q->field == AVF_AIR || q->field == AVF_SPACE ? 0 : map_area(iv2(r->x, r->y), q->field == AVF_SEA ? 1 : 2);
        e->up[e->nr] = e->nr;
        e->nr++;
    }
    for (int i = 0; i < e->nr; i++)   // linked: same owner and field, both placed and within AVE_LINK on the same area, or both nowhere known
        for (int j = i + 1; j < e->nr; j++) {
            const struct av_erec *a = &e->r[i], *b = &e->r[j];
            if (a->owner != b->owner || a->field != b->field || (a->x < 0) != (b->x < 0) || a->area != b->area) continue;
            if (a->x >= 0 && ivec2_dist2(iv2(a->x, a->y), iv2(b->x, b->y)) > AVE_LINK * AVE_LINK) continue;
            int ra = ave_root(e->up, i), rb = ave_root(e->up, j);
            if (ra != rb) e->up[ra > rb ? ra : rb] = ra < rb ? ra : rb;
        }
    avx_update(pl, idx, now);   // what it expects each enemy to field, what each has seen of its own (below)
    for (int i = 0; i < e->nr; i++) e->r[i].army = ave_root(e->up, i) * 4096 + i;   // the army's lowest record, then the memory's order
    qsort(e->r, (size_t)e->nr, sizeof *e->r, ave_cmp);
    for (int i = 0; i < e->nr; ) {
        int j = i, root = e->r[i].army / 4096;
        while (j < e->nr && e->r[j].army / 4096 == root) j++;
        if (e->n == e->cap) {
            int cap = e->cap ? 2 * e->cap : 32;
            struct av_enemy *na = realloc(e->a, (size_t)cap * sizeof *na);
            if (!na) return;
            e->a = na; e->cap = cap;
        }
        struct av_enemy *a = &e->a[e->n++];
        a->owner = e->r[i].owner; a->field = e->r[i].field; a->area = e->r[i].area; a->first = i; a->n = j - i; a->seen_t = 0; a->hp = 0;
        long sx = 0, sy = 0;
        for (int k = i; k < j; k++) {
            if (e->r[k].t > a->seen_t) a->seen_t = e->r[k].t;
            a->hp += (float)e->r[k].hp; sx += e->r[k].x; sy += e->r[k].y;
        }
        a->mid = e->r[i].x < 0 ? iv2(-1, -1) : iv2((int)(sx / a->n), (int)(sy / a->n));
        i = j;
    }
}
internal void av_enemy_sum(unsigned idx, const struct av_enemy *a, struct av_sum *s) {
    av_clear(s);
    for (int k = a->first; k < a->first + a->n; k++) {
        const struct av_erec *r = &g_ave[idx & 15].r[k];
        ee_player *o = player_at(r->owner);
        av_add_one(s, NULL, type_data(r->type), o ? st_rec(o, r->type) : NULL, r->owner, r->hp, r->x < 0 ? iv2(-1, -1) : iv2(r->x, r->y));
    }
    av_done(s);
}
// the families of the members summed in s (a bit each): what an enemy army's line lists its damage at
internal uint64_t av_kinds_in(const struct av_sum *s) {
    uint64_t m = 0;
    for (int t = 0; t < G_TYPES_N; t++) {
        ee_unit_type *td = s->cnt[t] ? type_data(t) : NULL;
        int k = td ? unit_type_get_family(td) : -1;
        if (k >= 0 && k < 64) m |= 1ull << k;
    }
    return m;
}

// WHAT THE ENEMY FIELDS, WHAT IT HAS SEEN, ITS COUNTERS (army view step 6; the player 2026-11-11: training by need judged
// against the few enemies in sight - "remember what the enemy fields"; and "the ai should also consider what it knows the
// enemy saw of its units, and build counters to the counters the enemy might build"). Per AI and enemy, by type:
// - fields: the most units of the type it knew of at once (its memory: seen within 10 minutes, its armies above),
//   fading with a half-life of AVX_HALF - what it expects the enemy to field, long after it lost sight of them;
// - seen: the most of its own military units of a type that stood within AVX_SEEN of an enemy unit it saw (the enemy
//   saw them: what a player knows of what his opponent got a look at), fading alike;
// - counters: what the enemy will train against what it saw - its own way, the game's counter vote (unit type
//   counter_1..3: 3, 2 and 1 for each own type it saw, by how many), each class as the type of it the enemy can field now
//   (the game offers it to that player: its civilization and epoch, both public), AVX_ANTICIPATE of the army it fields.
// Not saved: after a load it builds again from the memory (saved). Nothing decides on it but training by need (armies.c).
#define AVX_HALF 1200000.0f
#define AVX_SEEN 10
#define AVX_ANTICIPATE 0.5f
internal float g_avx[16][16][G_TYPES_N];   // [AI][enemy][type]: fields
internal float g_avs[16][16][G_TYPES_N];   // [AI][enemy][its own type]: seen
internal uint32_t g_avx_t[16];
internal int prio_buildable(ee_player *pl, int t);   // priority.c: the game offers it to that player now
internal void avx_update(ee_player *pl, unsigned idx, uint32_t now) {
    static uint16_t cur[16][G_TYPES_N];
    static struct { ivec2 p; int o; } eye[1024];
    struct intel *in = &g_intel[idx & 15];
    __typeof__(g_ave[0]) *e = &g_ave[idx & 15];
    if (now < g_avx_t[idx]) { memset(g_avx[idx], 0, sizeof g_avx[idx]); memset(g_avs[idx], 0, sizeof g_avs[idx]); g_avx_t[idx] = 0; }   // a new game
    float f = g_avx_t[idx] ? 1.0f / (1.0f + (float)(now - g_avx_t[idx]) * (0.6931f / AVX_HALF)) : 1.0f;   // the fading (arithmetic only: every PC the same)
    g_avx_t[idx] = now ? now : 1;
    memset(cur, 0, sizeof cur);
    for (int i = 0; i < e->nr; i++)
        if (e->r[i].owner > 0 && e->r[i].owner < 16 && (unsigned)e->r[i].type < G_TYPES_N && cur[e->r[i].owner][e->r[i].type] < 0xFFFF) cur[e->r[i].owner][e->r[i].type]++;
    for (int o = 1; o < 16; o++)
        for (int t = 0; t < G_TYPES_N; t++) { float v = g_avx[idx][o][t] * f; g_avx[idx][o][t] = v > (float)cur[o][t] ? v : (float)cur[o][t]; }
    int ne = 0;   // what it sees of the enemy now, buildings too (towers see): the eyes on its own
    for (int k = 0; k < in->n && ne < 1024; k++) {
        const struct intel_rec *r = &in->r[k];
        ee_player *o = r->t == in->last && r->x >= 0 ? player_at(r->owner) : NULL;
        if (o && is_enemy(pl, o)) { eye[ne].p = iv2(r->x, r->y); eye[ne++].o = r->owner; }
    }
    memset(cur, 0, sizeof cur);
    for (struct grid_it it = grid_it_all(1); ne && grid_it_next(&it); ) {
        if (unit_get_owner(it.u) != pl || !cv_alive(it.u) || !cv_mil(it.td)) continue;
        int t = unit_type_get_id(it.td);
        if ((unsigned)t >= G_TYPES_N) continue;
        ivec2 p = unit_get_tile(it.u);
        unsigned done = 0;   // once per enemy
        for (int k = 0; k < ne; k++)
            if (!(done >> eye[k].o & 1) && ivec2_dist2(p, eye[k].p) <= AVX_SEEN * AVX_SEEN) { done |= 1u << eye[k].o; if (cur[eye[k].o][t] < 0xFFFF) cur[eye[k].o][t]++; }
    }
    for (int o = 1; o < 16; o++)
        for (int t = 0; t < G_TYPES_N; t++) { float v = g_avs[idx][o][t] * f; g_avs[idx][o][t] = v > (float)cur[o][t] ? v : (float)cur[o][t]; }
}
// the type of RPS class rps that enemy o can field now, the latest of the game's offer (the strongest of those: hit points x
// attack); 0: none - land only (fl: AVF_LAND), the counters the land army meets
internal int avx_class_type(ee_player *o, int rps) {
    int best = 0, bep = -1; float bs = 0;
    ee_tech_tree *tt = o ? player_get_tech_tree(o) : NULL;
    for (int t = 0; tt && t < G_TYPES_N; t++) {
        ee_unit_type *td = type_data(t);
        if (!td || unit_type_get_id(td) != t || unit_type_get_rps_class(td) != rps || av_field_of(td) != AVF_LAND || unit_type_get_attack(td) <= 0) continue;
        if (ut_role(t, UTR_CITIZEN) || !prio_buildable(o, t)) continue;
        ee_tech *x = tech_tree_lookup(tt, unit_type_get_tech(td));
        int ep = x ? tech_get_epoch(x) : 0;
        float sc = (float)unit_type_get_hp(td) * (float)unit_type_get_attack(td);
        if (ep > bep || (ep == bep && sc > bs)) { best = t; bep = ep; bs = sc; }
    }
    return best;
}
// what enemy o will train against what it saw of the AI's land army: n[type], the land army it fields times
// AVX_ANTICIPATE, by the counter vote over what it saw; returns how many in all
internal float avx_counters(unsigned idx, int o, float *n) {
    static float w[256];
    memset(w, 0, sizeof w);
    float fields = 0, sw = 0;
    for (int t = 0; t < G_TYPES_N; t++) {
        ee_unit_type *td = type_data(t);
        if (!td || av_field_of(td) != AVF_LAND) continue;
        fields += g_avx[idx][o][t];
        float s = g_avs[idx][o][t];
        if (s <= 0) continue;
        int c[3] = { unit_type_get_counter_1(td), unit_type_get_counter_2(td), unit_type_get_counter_3(td) };
        for (int j = 0; j < 3; j++) if (c[j] > 0 && c[j] < 256) { w[c[j]] += s * (float)(3 - j); sw += s * (float)(3 - j); }
    }
    float all = fields * AVX_ANTICIPATE, got = 0;
    ee_player *op = player_at(o);
    for (int k = 1; k < 256 && sw > 0 && all > 0; k++) {
        if (w[k] <= 0) continue;
        int t = avx_class_type(op, k);
        if (t) { n[t] += all * w[k] / sw; got += all * w[k] / sw; }
    }
    return got;
}
// the most of a [type] list as "N name, ..." (at most 6)
internal void avx_list(const float *v, char *o, int cap) {
    int w = 0; o[0] = 0;
    static unsigned char used[G_TYPES_N];
    memset(used, 0, sizeof used);
    for (int j = 0; j < 6 && w < cap - 48; j++) {
        int b = -1;
        for (int t = 0; t < G_TYPES_N; t++) if (!used[t] && v[t] >= 0.5f && (b < 0 || v[t] > v[b])) b = t;
        if (b < 0) break;
        used[b] = 1;
        w += snprintf(o + w, (size_t)(cap - w), "%s%.0f %s", j ? ", " : "", v[b], unit_ident(b));
    }
    if (!w) snprintf(o, (size_t)cap, "nothing");
}
// the minute line per enemy (armies.c, tests)
internal void avx_log(unsigned idx, uint32_t now, void (*log)(unsigned, uint32_t, const char *, ...)) {
    static char a[400], b[400], c[400];
    static float n[G_TYPES_N];
    for (int o = 1; o < 16; o++) {
        float fields = 0;
        for (int t = 0; t < G_TYPES_N; t++) fields += g_avx[idx][o][t];
        if (fields < 0.5f) continue;
        memset(n, 0, sizeof n);
        float got = avx_counters(idx, o, n);
        avx_list(g_avx[idx][o], a, (int)sizeof a); avx_list(g_avs[idx][o], b, (int)sizeof b); avx_list(n, c, (int)sizeof c);
        log(idx, now, "what P%d fields as it expects: %.0f (%s); has seen of its own: %s; its counters to expect, %.0f: %s", o, fields, a, b, got, c);
    }
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
enum { AV_FORCE = 0, AV_LANDING = 1, AV_RAID = 2, AV_FLEET = 3 };
internal const char *const k_av_kind[] = { "the force", "the landing's", "a raid party", "a fleet's force" };
internal const int k_av_field[] = { AVF_LAND, AVF_LAND, AVF_LAND, AVF_SEA };
struct av_member { ee_unit *u; int id; };
struct av_army {
    int id, kind, field, sub; struct av_member *m; int n, cap; uint32_t made_t;   // field: AVF_*; sub: which of its kind (a fleet's force: fleet.c TF_*)
    const char *what, *task;   // its name and its task now, as its part calls them (the log's; NULL: none)
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
    a->id = ++g_av[idx].next; a->kind = kind; a->field = k_av_field[kind]; a->made_t = now ? now : 1;
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
// THE FLEET (fleet.c, step 4 of the army view): its forces - TF_HOME .. TF_ESCORT with task forces, else the one fleet as
// sub 0 - are sea armies of this list, their members as the fleet has them at its tick (fleet.c still decides; here the
// deciding parts see them as armies). n 0 ends the force's army.
internal void av_fleet_sync(unsigned idx, int sub, ee_unit *const *u, int n, const char *what, const char *task, uint32_t now) {
    idx &= 15;
    struct av_army *a = NULL;
    for (int k = 0; k < g_av[idx].n && !a; k++) if (g_av[idx].a[k].kind == AV_FLEET && g_av[idx].a[k].sub == sub) a = &g_av[idx].a[k];
    if (!n) { if (a) av_end(idx, a); return; }
    if (!a && !(a = av_new(idx, AV_FLEET, now))) return;
    a->sub = sub; a->what = what; a->task = task; a->n = 0;
    for (int k = 0; k < n; k++) av_put(a, u[k]);
}
