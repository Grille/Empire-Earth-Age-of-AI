// INTEL: what an AI knows about its enemies. Included by eeaimod.c after landing.c.
//
// [all] honest_vision (default 1): the AIs see every unit of the human everywhere (a floor of 1 in their slots of
// the visibility count - engine-notes.md). 0: the mod uses that as the game has it. 1: an AI knows what a human in
// its place would - the user: "planning ahead, assuming the enemy has towers and a defence army before I ever saw his
// base, knowing that the huge army I saw 5 minutes ago didn't just disappear because I can't see it now":
// - seen now: the count above the floor every unit of that owner carries (least count over its units, per viewer -
//   measured every 3 s, so no assumption about where the floor comes from);
// - memory: every enemy unit seen, with tile, hit points, attack and time; buildings stay known until their spot is
//   looked at again and they are gone; soldiers, ships and aircraft count for 10 minutes at the last place seen
//   (moved on when that place is looked at and they are not there - still in the army estimate);
// - looked at: per 6x6-tile cell the last time a unit of the AI or an ally was within 8 tiles;
// - assumptions: a place not looked at for 3 minutes is assumed held by a quarter of the army it remembers.
// The systems ask through seen_by (attack.c), intel_soldiers, intel_army, intel_looked_ago - with 0 those give the
// game's view (nothing remembered, everything looked at now), so every system works under both.
#define INTEL_REC 1024
#define INTEL_CELL 6
#define INTEL_NC (600 / INTEL_CELL + 1)   // the map up to 600 tiles (150 until 2026-10-20: on Large / Huge maps, up to ~330 tiles, everything beyond 150 counted as never looked at - landing.c assumed every beach there held)
struct intel_rec { ee_unit *u; int owner, type, kind, x, y, hp; float att; uint32_t t; };   // kind 0 building, 1 soldier, 2 ship, 3 aircraft, 4 other
internal struct intel {
    struct intel_rec r[INTEL_REC]; int n;
    ee_unit *known[4096];                // buildings remembered (open addressing by pointer)
    uint32_t looked[INTEL_NC][INTEL_NC];
    uint32_t now, last;
    unsigned seen_now, dropped, moved;
} g_intel[16];

internal int g_intel_seed;   // a saved game without the AIs' memories: they start remembering the buildings of their enemies

internal void intel_new_game(int from_save) { memset(g_intel, 0, sizeof g_intel); g_intel_seed = from_save; }
internal void intel_loaded(void) {   // the memories came with the save: their unit pointers are of the old session
    for (int p = 0; p < 16; p++) {
        struct intel *in = &g_intel[p];
        if (in->n) g_intel_seed = 0;
        for (int k = 0; k < in->n; k++) in->r[k].u = NULL;
        memset(in->known, 0, sizeof in->known);
    }
}

internal unsigned intel_hash(ee_unit *u) { return (unsigned)(((uintptr_t)u >> 4) * 2654435761u) & 4095; }
internal int intel_remembers(ee_player *pl, ee_unit *u) {
    struct intel *in = &g_intel[(unsigned)player_get_index(pl) & 15];
    for (unsigned h = intel_hash(u), k = 0; k < 4096 && in->known[h]; h = (h + 1) & 4095, k++)
        if (in->known[h] == u) return 1;
    return 0;
}
internal int intel_kind(ee_unit_type *td) {
    int fam = type_get_family(td);
    if (fam == 2 || fam == 29) return 0;
    if (is_soldier(td) || is_siege(type_get_id(td))) return 1;
    if (fam == 4) return 2;
    if (fam == 3 || fam == 7 || fam == 17) return 3;
    return 4;
}
// ms since a unit of pl or an ally was near (x, y); 0 with honest_vision off (the game's view: everything at once)
internal uint32_t intel_looked_ago(ee_player *pl, int x, int y) {
    struct intel *in = &g_intel[(unsigned)player_get_index(pl) & 15];
    if (!g_honest_vision) return 0;
    int cx = x / INTEL_CELL, cy = y / INTEL_CELL;
    if (cx < 0 || cy < 0 || cx >= INTEL_NC || cy >= INTEL_NC) return 0xFFFFFFFFu;
    return in->looked[cx][cy] ? in->now - in->looked[cx][cy] : 0xFFFFFFFFu;
}
// soldiers (and siege) remembered within r tiles of (x, y) and not seen now, seen within the last 5 minutes: hit
// points and attack summed, how many. Nothing with honest_vision off (all of them are seen)
internal void intel_soldiers(ee_player *pl, int x, int y, int r, float *hp, float *att, float *pw, int *n) {
    struct intel *in = &g_intel[(unsigned)player_get_index(pl) & 15];
    if (!g_honest_vision) return;
    for (int k = 0; k < in->n; k++) {
        struct intel_rec *e = &in->r[k];
        if (e->kind != 1 || e->x < 0 || e->t == in->last || in->now - e->t > 300000) continue;
        if (ivec2_dist2(iv2(e->x, e->y), iv2(x, y)) > r * r) continue;
        *hp += (float)e->hp; *att += e->att; (*n)++;
        if (pw) *pw += (float)e->hp * e->att;
    }
}
// the enemy army it knows of: soldiers seen within 10 minutes (seen now included), summed
internal void intel_army(ee_player *pl, float *hp, float *att, float *pw, int *n) {
    struct intel *in = &g_intel[(unsigned)player_get_index(pl) & 15];
    for (int k = 0; k < in->n; k++)
        if (in->r[k].kind == 1 && in->now - in->r[k].t <= 600000) {
            *hp += (float)in->r[k].hp; *att += in->r[k].att; (*n)++;
            if (pw) *pw += (float)in->r[k].hp * in->r[k].att;
        }
}

struct intel_scan {
    ee_player *pl; unsigned pi;
    // every unit, in the grid's order (the simulation's: the same in every run of a game), and the same sorted by address
    // for intel_live (identity only - a remembered pointer is read only when it is in there). The records are made in
    // the grid's order: in the address order (until 2026-10-22) the memory's order, so the float sums over it and which
    // record a full memory drops, followed the heap - which differs between two runs of the same game, more so under load
    int nlive; ee_unit *live[6000], *by_addr[6000];
    uint8_t floor[16]; int fseen[16];
    int ne; ivec2 eye[3000];             // eyes: its and its allies' units
};
internal void intel_unit(ee_unit *u, void *ctx) {
    struct intel_scan *s = ctx;
    ee_player *o = unit_get_owner(u);
    if (!o || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) return;
    int oi = player_get_index(o);
    if (oi <= 0 || oi >= 16) return;
    if (s->nlive < 6000) s->live[s->nlive++] = u;
    if (!is_enemy(s->pl, o)) { if (s->ne < 3000) s->eye[s->ne++] = unit_get_tile(u); return; }
    ee_unit_vis *v = unit_get_vis(u);
    uint16_t *vis = v ? siteown_get_seen(v) : NULL;
    unsigned c = vis ? vis[s->pi] & 0xFF : 0;
    if (!s->fseen[oi] || c < s->floor[oi]) { s->floor[oi] = (uint8_t)c; s->fseen[oi] = 1; }
}
internal int intel_ptr_cmp(const void *a, const void *b) {
    uintptr_t x = *(const uintptr_t *)a, y = *(const uintptr_t *)b;
    return x < y ? -1 : x > y;
}
internal int intel_live(struct intel_scan *s, ee_unit *u) {
    return bsearch(&u, s->by_addr, (size_t)s->nlive, sizeof *s->by_addr, intel_ptr_cmp) != NULL;
}
internal void intel_tick(ee_player *pl, uint32_t now) {
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    struct intel *in = &g_intel[pi];
    if (now < in->now) memset(in, 0, sizeof *in);   // a new game
    if (!g_honest_vision || (in->now && now - in->now < 3000)) return;
    static struct intel_scan s;
    s.pl = pl; s.pi = pi; s.nlive = 0; s.ne = 0;
    memset(s.floor, 0, sizeof s.floor); memset(s.fseen, 0, sizeof s.fseen);
    grid_each(intel_unit, &s);
    for (int o = 0; o < 16; o++) g_vis_floor[pi][o] = s.fseen[o] ? s.floor[o] : 0;
    memcpy(s.by_addr, s.live, (size_t)s.nlive * sizeof *s.live);
    qsort(s.by_addr, (size_t)s.nlive, sizeof *s.by_addr, intel_ptr_cmp);
    in->last = in->now = now;
    // looked at now
    for (int e = 0; e < s.ne; e++)
        for (int cx = (s.eye[e].x - 8) / INTEL_CELL; cx <= (s.eye[e].x + 8) / INTEL_CELL; cx++)
            for (int cy = (s.eye[e].y - 8) / INTEL_CELL; cy <= (s.eye[e].y + 8) / INTEL_CELL; cy++) {
                if (cx < 0 || cy < 0 || cx >= INTEL_NC || cy >= INTEL_NC) continue;
                ivec2 mid = iv2(cx * INTEL_CELL + INTEL_CELL / 2, cy * INTEL_CELL + INTEL_CELL / 2);   // the cell's
                if (ivec2_dist2(mid, s.eye[e]) <= 64) in->looked[cx][cy] = now;
            }
    // a saved game without memories (made before honest_vision, or without the mod): the buildings of its enemies, as
    // a human loading the game sees them on the explored map. Records without a unit (restored from a save): buildings
    // matched to the one standing at that spot
    int seed = g_intel_seed && now > 60000 && !in->n;
    for (int k = 0; k < s.nlive; k++) {
        ee_unit *u = s.live[k];
        ee_unit_type *td = unit_get_type_data(u);
        ee_player *o = unit_get_owner(u);
        if (!is_enemy(pl, o) || intel_kind(td) != 0) continue;
        int x = unit_get_x(u), y = unit_get_y(u), t = type_get_id(td), oi = player_get_index(o) & 15;
        if (seed && in->n < INTEL_REC) {
            struct intel_rec *e = &in->r[in->n++];
            float r, a = 0;
            defence_stats(u, &r, &a);
            e->u = u; e->owner = oi; e->type = t; e->kind = 0; e->x = x; e->y = y; e->hp = unit_get_hp(u); e->att = a; e->t = now;
            continue;
        }
        for (int j = 0; j < in->n; j++)
            if (!in->r[j].u && in->r[j].kind == 0 && in->r[j].x == x && in->r[j].y == y && in->r[j].type == t && in->r[j].owner == oi) {
                in->r[j].u = u;
                break;
            }
    }
    if (seed && g_log) fprintf(g_log, "# intel: P%u at %.1f min: a save without memories - remembers %d buildings of its enemies\n", pi, now / 60000.0, in->n);
    // what it sees now: into the memory
    in->seen_now = 0;
    for (int k = 0; k < s.nlive; k++) {
        ee_unit *u = s.live[k];
        ee_player *o = unit_get_owner(u);
        if (!is_enemy(pl, o)) continue;
        ee_unit_vis *v = unit_get_vis(u);
        uint16_t *vis = v ? siteown_get_seen(v) : NULL;
        int oi = player_get_index(o) & 15;
        if (!vis || (vis[pi] & 0xFF) <= g_vis_floor[pi][oi]) continue;
        in->seen_now++;
        int j = 0;
        while (j < in->n && in->r[j].u != u) j++;
        if (j == in->n) {
            if (in->n == INTEL_REC) {   // full: the oldest goes
                j = 0;
                for (int q = 1; q < in->n; q++) if (in->r[q].t < in->r[j].t) j = q;
            } else in->n++;
        }
        struct intel_rec *e = &in->r[j];
        ee_unit_type *td = unit_get_type_data(u);
        e->u = u; e->owner = oi; e->type = type_get_id(td); e->kind = intel_kind(td);
        e->x = unit_get_x(u); e->y = unit_get_y(u); e->hp = unit_get_hp(u);
        float a = 0, r;
        if (e->kind == 0) defence_stats(u, &r, &a);
        else a = e->hp > 0 ? unit_power(u) / (float)e->hp : 0;
        e->att = a; e->t = now;
    }
    // what it looked at and did not find: buildings gone; soldiers moved on (or dead - seen dying when they were seen
    // the last time); old sightings forgotten
    int n = 0;
    for (int k = 0; k < in->n; k++) {
        struct intel_rec *e = &in->r[k];
        int keep = 1;
        if (e->t != now) {
            int looked = 0, live = intel_live(&s, e->u);   // an eye within 6 tiles of where it was
            for (int q = 0; q < s.ne && e->x >= 0 && !looked; q++)
                looked = ivec2_dist2(s.eye[q], iv2(e->x, e->y)) <= 36;
            if (e->kind == 0) {
                keep = !(looked || (!live && now - e->t <= 6000));
                // a remembered building no longer on the map (destroyed out of sight) keeps its spot but not its pointer:
                // the memory is freed, and a new building allocated there counted as remembered (seen_by). Without a
                // unit it is matched again like a record from a save - to a building of that owner and type at its spot
                if (!live) e->u = NULL;
            }
            else if (!live && now - e->t <= 6000) keep = 0;   // seen until the last look and gone now: died in sight
            else {
                if (looked) { e->x = -1; in->moved++; }   // not there any more: somewhere
                if (now - e->t > 600000) keep = 0;
            }
        }
        if (keep) in->r[n++] = *e; else in->dropped++;
    }
    in->n = n;
    memset(in->known, 0, sizeof in->known);
    for (int k = 0; k < in->n; k++) {
        if (in->r[k].kind != 0) continue;
        unsigned h = intel_hash(in->r[k].u);
        while (in->known[h]) h = (h + 1) & 4095;
        in->known[h] = in->r[k].u;
    }
}

// intel P (test): what AI P knows - mode, floors, what it sees now and remembers per owner and kind, looked cells,
// the army it knows of
internal void cmd_intel(char *out, const char *args) {
    int p = atoi(args);
    if (p <= 0 || p >= 16 || !g_players[p]) { REPLY("error: intel P\n"); return; }
    struct intel *in = &g_intel[p];
    REPLY("P%d: honest_vision %d, %d records, %u seen at the last look (%.1f min), %u dropped, %u moved on; floors:", p, g_honest_vision, in->n,
          in->seen_now, in->now / 60000.0, in->dropped, in->moved);
    for (int o = 1; o < 16; o++) if (g_vis_floor[p][o]) REPLY(" P%d %d", o, g_vis_floor[p][o]);
    REPLY("\n");
    static const char *kinds[] = { "buildings", "soldiers", "ships", "aircraft", "other" };
    for (int o = 1; o < 16; o++)
        for (int k = 0; k < 5; k++) {
            int now_n = 0, rem = 0, lost = 0;
            for (int j = 0; j < in->n; j++) {
                struct intel_rec *e = &in->r[j];
                if (e->owner != o || e->kind != k) continue;
                if (e->t == in->last) now_n++; else if (e->x < 0) lost++; else rem++;
            }
            if (now_n + rem + lost) REPLY("  P%d %-9s: %d seen now, %d remembered where they were, %d somewhere\n", o, kinds[k], now_n, rem, lost);
        }
    int cells = 0, recent = 0;
    for (int x = 0; x < INTEL_NC; x++)
        for (int y = 0; y < INTEL_NC; y++) {
            if (!in->looked[x][y]) continue;
            cells++;
            recent += in->now - in->looked[x][y] <= 180000;
        }
    float hp = 0, att = 0; int n = 0;
    intel_army(g_players[p], &hp, &att, NULL, &n);
    REPLY("  cells looked at: %d ever, %d within 3 minutes (of %d); the army it knows of: %d soldiers, strength %.0f\n", cells, recent,
          INTEL_NC * INTEL_NC, n, sqrtf(hp * att));
}

// vis P [target] (test): what the per-unit visibility value (unit +0x14 -> +0xC, uint16 per player; seen_by tests
// its low byte) means for moving units - "seen now" or "seen once". For each enemy unit of AI P: the value, and the
// distance to the nearest unit or building of P (its eyes); a histogram by distance and kind.
struct vis_scan {
    ee_player *pl, *target;
    int allies;                          // its allies' units are eyes too (shared vision)
    int ne; ivec2 eye[4000];             // P's own units and buildings
    int nu; ee_unit *u[4000];            // the enemies
};
internal void vis_unit(ee_unit *u, void *ctx) {
    struct vis_scan *s = ctx;
    ee_player *o = unit_get_owner(u);
    if (unit_get_hp(u) <= 0 || !o || unit_get_x(u) < 0) return;
    if (o == s->pl || (s->allies && player_get_index(o) > 0 && !is_enemy(s->pl, o))) { if (s->ne < 4000) s->eye[s->ne++] = unit_get_tile(u); }
    else if ((s->target ? o == s->target : is_enemy(s->pl, o)) && player_get_index(o) > 0 && s->nu < 4000) s->u[s->nu++] = u;
}
internal void *g_vis_find[16];
internal void vis_owner(ee_unit *u, void *ctx) {   // player objects by index, also of humans (g_players holds the AIs)
    (void)ctx;
    ee_player *o = unit_get_owner(u);
    int i = o ? player_get_index(o) : -1;
    if (i > 0 && i < 16) g_vis_find[i] = o;
}
internal unsigned vis_raw(ee_unit *u, unsigned pi) {
    ee_unit_vis *v = unit_get_vis(u);
    uint16_t *vis = v ? siteown_get_seen(v) : NULL;
    return vis ? vis[pi] : 0xFFFFu;
}
internal void cmd_vis(char *out, const char *args) {
    int p = 0, t = 0;
    sscanf(args, "%d %d", &p, &t);
    memset(g_vis_find, 0, sizeof g_vis_find);
    grid_each(vis_owner, NULL);
    if (p <= 0 || p >= 16 || !g_vis_find[p]) { REPLY("error: vis P [target]\n"); return; }
    static struct vis_scan s;
    memset(&s, 0, sizeof s);
    s.pl = g_vis_find[p];
    s.target = t > 0 && t < 16 ? g_vis_find[t] : NULL;
    s.allies = strstr(args, "allies") != NULL;
    grid_each(vis_unit, &s);
    // [kind: 0 building, 1 soldier/siege, 2 ship, 3 aircraft, 4 other][distance bin: <=8, <=15, <=25, >25][low byte 0 / !=0]
    static const char *kinds[] = { "building", "soldier", "ship", "aircraft", "other" };
    int h[5][4][2] = { 0 }, hi[5][4] = { 0 }, shown = 0;
    for (int k = 0; k < s.nu; k++) {
        ee_unit *u = s.u[k];
        ee_unit_type *td = unit_get_type_data(u);
        int x = unit_get_x(u), y = unit_get_y(u), fam = type_get_family(td);
        int kind = fam == 2 || fam == 29 ? 0 : (is_soldier(td) || is_siege(type_get_id(td))) ? 1 : fam == 4 ? 2 : (fam == 3 || fam == 7 || fam == 17) ? 3 : 4;
        float bd = 1e9f;
        for (int e = 0; e < s.ne; e++) {
            float d2 = (float)ivec2_dist2(s.eye[e], iv2(x, y));
            if (d2 < bd) bd = d2;
        }
        float d = sqrtf(bd);
        int bin = d <= 8 ? 0 : d <= 15 ? 1 : d <= 25 ? 2 : 3;
        unsigned v = vis_raw(u, (unsigned)p);
        h[kind][bin][(v & 0xFF) != 0]++;
        hi[kind][bin] += (v >> 8) != 0;
        if (shown < 30 && kind != 0 && (strstr(args, "all") || ((v & 0xFF) != 0) != (d <= 12))) {
            shown++;
            REPLY("  P%d type %d (%s) at %d,%d: value %04X, nearest eye of P%d %.1f tiles\n", player_get_index(unit_get_owner(u)),
                  type_get_id(td), kinds[kind], x, y, v, p, d);
        }
    }
    if (strstr(args, "far")) {   // the land tiles farthest from every unit (P's side and its enemies): where nobody looks
        int bx[5] = { 0 }, by[5] = { 0 }; float bd[5] = { 0 };
        for (int x = 2; x < 148; x += 3)
            for (int y = 2; y < 148; y += 3) {
                if (!area_id(x, y, 2) || area_id(x, y, 1)) continue;
                float m = 1e9f;
                for (int e = 0; e < s.ne && m > 0; e++) { float d2 = (float)ivec2_dist2(s.eye[e], iv2(x, y)); if (d2 < m) m = d2; }
                for (int k = 0; k < s.nu && m > 0; k++) {
                    float d2 = (float)ivec2_dist2(unit_get_tile(s.u[k]), iv2(x, y));
                    if (d2 < m) m = d2;
                }
                for (int j = 0; j < 5; j++)
                    if (m > bd[j]) { for (int i = 4; i > j; i--) { bd[i] = bd[i - 1]; bx[i] = bx[i - 1]; by[i] = by[i - 1]; } bd[j] = m; bx[j] = x; by[j] = y; break; }
            }
        REPLY("farthest land from every unit:");
        for (int j = 0; j < 5; j++) REPLY(" %d,%d (%.0f)", bx[j], by[j], sqrtf(bd[j]));
        REPLY("\n");
    }
    REPLY("explore all (player +0x465):");
    for (int q = 1; q < 16; q++) if (g_vis_find[q]) REPLY(" P%d %d (vision slot +0x45C: %d)", q, player_get_explore_all(g_vis_find[q]), player_get_index(g_vis_find[q]));
    REPLY("\n");
    REPLY("P%d: %d own units/buildings, %d enemy units; per kind and distance to the nearest eye (<=8, <=15, <=25, >25): seen (low byte) / all, high byte set\n",
          p, s.ne, s.nu);
    for (int k = 0; k < 5; k++) {
        REPLY("  %-9s", kinds[k]);
        for (int b = 0; b < 4; b++) REPLY("  %4d/%-4d hi %-4d", h[k][b][1], h[k][b][0] + h[k][b][1], hi[k][b]);
        REPLY("\n");
    }
}

// vistrack P x y r (test): remember (add) the units within r tiles of x, y; vistrack P: those units now - tile, hit points,
// their visibility value for P, the distance to the nearest unit of P or its allies
internal ee_unit *g_vt_u[32]; internal int g_vt_n;
internal void cmd_vistrack(char *out, const char *args) {
    int p = 0, x = -1, y = -1, r = 0;
    int na = sscanf(args, "%d %d %d %d", &p, &x, &y, &r);
    memset(g_vis_find, 0, sizeof g_vis_find);
    grid_each(vis_owner, NULL);
    if (p <= 0 || p >= 16 || !g_vis_find[p]) { REPLY("error: vistrack P [x y r]\n"); return; }
    static struct vis_scan s;
    memset(&s, 0, sizeof s);
    s.pl = g_vis_find[p]; s.allies = 1;
    grid_each(vis_unit, &s);
    if (na == 4) {
        // appended: several groups can be tracked
        for (int k = 0; k < s.nu && g_vt_n < 32; k++) {
            if (ivec2_dist2(unit_get_tile(s.u[k]), iv2(x, y)) <= r * r) g_vt_u[g_vt_n++] = s.u[k];
        }
        REPLY("tracking %d units\n", g_vt_n);
    }
    for (int k = 0; k < g_vt_n; k++) {
        ee_unit *u = g_vt_u[k];
        int alive = 0;
        for (int j = 0; j < s.nu && !alive; j++) alive = s.u[j] == u;
        if (!alive) { REPLY("  #%d gone\n", k); continue; }
        int ux = unit_get_x(u), uy = unit_get_y(u);
        float bd = 1e9f;
        for (int e = 0; e < s.ne; e++) {
            float d2 = (float)ivec2_dist2(s.eye[e], iv2(ux, uy));
            if (d2 < bd) bd = d2;
        }
        REPLY("  #%d type %d at %d,%d hp %d: value %04X, nearest eye of P%d or allies %.1f\n", k, type_get_id(unit_get_type_data(u)),
              ux, uy, unit_get_hp(u), vis_raw(u, (unsigned)p), p, sqrtf(bd));
        ee_unit_vis *v = unit_get_vis(u);
        uint16_t *vv = v ? siteown_get_seen(v) : NULL;
        if (vv) REPLY("      slots 0-7: %04X %04X %04X %04X %04X %04X %04X %04X\n", vv[0], vv[1], vv[2], vv[3], vv[4], vv[5], vv[6], vv[7]);
    }
}

internal void intel_install(const char *aimod_ini, const char *autotest_ini) {
    g_honest_vision = cfg_int(aimod_ini, autotest_ini, 1, "honest_vision", 1);
    if (g_log) fprintf(g_log, "# intel: honest_vision %d (%s)\n", g_honest_vision,
                       g_honest_vision ? "what it sees, remembers and assumes" : "the game's view - every unit of the human");
}

