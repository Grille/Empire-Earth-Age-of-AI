// CLEARING GROUND ([ai_base] clear_ground; the player 2026-11-24: "the ai should be able to mark buildings disposable.
// woodcuter settlments after the forest is gone, no longer used porduction buildings. when they ai needs space it
// should be able to destroy them to make new space, in very extrem cases it should even be able to destroy still usable
// buildings (it will never get an airport on small silands elsewise)").
// DISPOSABLE (worth 0): a Settlement with nothing left to gather round it (no tree, mine, bush or animal with food
// within CG_SITE_R tiles, no field farmed within CG_FIELD_R) - a production building that has trained nothing for
// CG_IDLE while another of its kind stands, with another building of its holding its territory - a field nobody has
// farmed for CG_IDLE (the player 2026-11-24: "in the erly game the ai builds a lot to boom its economy, in later stages
// many fields stay unused and are redundant") - a granary whose fields are all such, with no food gathered round it,
// while another granary of its stands. EXPENDABLE (worth 1, only in the extreme): a House, a production building idle
// that long (the last of its kind, or its territory's only holder), one in use with another of its kind, its last
// granary gone unused. A DEFENCE (worth 2, an Airport's extreme only): a tower, an AA gun, a wall piece or gate. Never:
// towns, fields farmed, docks, fortresses, Airports, the research buildings, the space buildings, one with units
// inside, anything going up.
// A WANT (build.c cg_want): a building a step of build.c found no spot for, by the step's centre and rings - a clock a
// type (CG_WANTS), so two steps failing in the same look each keep theirs. Unmet for CG_PATIENCE it may clear the
// disposable; unmet for CG_EXTREME (an Airport, a research or production building only) the expendable too, an Airport
// the defences as well. A clearing is the anchor in the want's rings whose footprint (a granary's with its 8 fields'
// places round it, an Airport's with its runway pieces) holds nothing but open land, units and buildings it may take
// down - the least worth (each 1 + its worth) first, then the nearest; those buildings are taken down (shown dying, as
// the game's AI deletes). Of the wants due, the one waiting longest, one look a want every CG_EVERY; a want that was
// looked for and still finds no spot (nothing to clear, or the game's slope test after the clearing) waits CG_COOL.
// "# clearing:" lines: each clearing, and each look that found nothing, with why its anchors were refused.
#define CG_SITE_R 10
#define CG_FIELD_R 6
#define CG_IDLE 600000
#define CG_PATIENCE 120000
#define CG_EXTREME 600000
#define CG_EVERY 60000
#define CG_COOL 600000
#define CG_SET 4            // buildings a clearing takes down at most
#define CG_SET_AIR 16       // ... for an Airport in the extreme (wall pieces are a tile each)
#define CG_TRACK 1024       // production buildings whose last training, fields whose last farmer it keeps
#define CG_GRAN_R 4         // a granary's fields: their corners this near its own
#define CG_WANTS 8          // wants kept at once, one a type (a granary and an Airport failing in the same look each keep their own clock)
#define CG_TRIED 4          // a want's anchors cleared that still took no building: not cleared again (a tower rebuilt there meanwhile)
struct cg_want { int type, r0, r1, margin, extreme; ivec2 at; uint32_t first, last, done_t, cool_t; ivec2 tried[CG_TRIED]; int ntried; };
internal struct cg_st {
    struct { uint32_t serial, busy; } tr[CG_TRACK]; int ntr;   // a production building's or field's serial, when it last trained or was farmed
    int ngran;                // its granaries (cg_clear)
    uint32_t sum_t;           // the log's last "disposable now" line
    struct cg_want w[CG_WANTS];
    unsigned cleared, taken, extreme_n;   // so far: clearings, buildings taken down, clearings in the extreme
} g_cg[16];
internal int pr_kind(int type);   // produce.c
// build.c, each look: its production buildings' training, its fields' farmers (CG_IDLE)
internal void cg_note(unsigned idx, ee_unit *u, int busy, uint32_t now) {
    struct cg_st *c = &g_cg[idx];
    uint32_t sn = (uint32_t)unit_get_serial(u);
    for (int k = 0; k < c->ntr; k++)
        if (c->tr[k].serial == sn) { if (busy) c->tr[k].busy = now ? now : 1; return; }
    if (c->ntr < CG_TRACK) { c->tr[c->ntr].serial = sn; c->tr[c->ntr++].busy = now ? now : 1; }   // new: as if it trained now
    else { int o = 0; for (int k = 1; k < CG_TRACK; k++) if (c->tr[k].busy < c->tr[o].busy) o = k; c->tr[o].serial = sn; c->tr[o].busy = now ? now : 1; }
}
internal void cg_track(unsigned idx, ee_unit *u, uint32_t now) {   // a production building: training or something queued
    int *qb = unit_get_queue_begin(u), *qe = unit_get_queue_end(u);
    cg_note(idx, u, unit_get_producing(u) || (qb && qe > qb), now);
}
internal void cg_track_field(unsigned idx, ee_unit *u, uint32_t now) { cg_note(idx, u, site_gatherers(u) > 0, now); }   // a field: a farmer on it
internal int cg_idle(unsigned idx, ee_unit *u, uint32_t now) {
    struct cg_st *c = &g_cg[idx];
    uint32_t sn = (uint32_t)unit_get_serial(u);
    for (int k = 0; k < c->ntr; k++) if (c->tr[k].serial == sn) return now >= c->tr[k].busy && now - c->tr[k].busy >= CG_IDLE;
    return 0;
}
// a Settlement with nothing left to gather round it - a granary with no food (fields farmed, bushes, animals): fields
// nobody has farmed for CG_IDLE are none
internal int cg_spent(unsigned idx, ee_unit *s, int fr, uint32_t now) {
    ivec2 p = unit_get_tile(s);
    for (struct grid_it it = grid_it_box(p.x + 1, p.y + 1, CG_SITE_R, 1); grid_it_next(&it); ) {
        int d = abs(unit_get_x(it.u) - p.x) > abs(unit_get_y(it.u) - p.y) ? abs(unit_get_x(it.u) - p.x) : abs(unit_get_y(it.u) - p.y);
        if (unit_get_hp(it.u) <= 0 && unit_type_get_family(it.td) != KIND_ANIMAL) continue;
        if (is_field_type(unit_type_get_id(it.td)) && d <= fr && !cg_idle(idx, it.u, now)) return 0;
        if (is_field_type(unit_type_get_id(it.td))) continue;
        if (unit_type_get_class_id(it.td) == CLS_RESOURCE && unit_get_amount(it.u) > 0 && d <= CG_SITE_R && (fr == CG_FIELD_R || unit_get_resource(it.u) == 2)) return 0;   // a granary: food only
        if (unit_type_get_family(it.td) == KIND_ANIMAL && unit_get_resource(it.u) == 2 && unit_get_amount(it.u) > 0 && d <= CG_SITE_R) return 0;
    }
    return 1;
}
// what a building of its is worth to keep: -1 never taken down, 0 disposable, 1 expendable (the extreme only), 2 a
// defence - a tower, an AA gun, a wall piece or gate (an Airport's extreme only: on a small island its strip finds no
// ground otherwise - the player 2026-11-24; 2026-11-24 Small Islands from World War II: towns, granaries, concrete
// towers and walls, 88mm guns stood on every anchor its want was looked for at)
internal int cg_worth(ee_player *pl, unsigned idx, ee_unit *u, const int *kinds, uint32_t now) {
    ee_unit_type *td = unit_get_type_data(u);
    ee_unit *st = unit_get_physics(u);
    if (!td || unit_get_owner(u) != pl || unit_get_hp(u) <= 0 || !st || !unit_get_finished(st)) return -1;
    int t = unit_type_get_id(td), k = pr_kind(t), p = player_get_index(pl);
    if (unit_get_garrisoned(u) > 0) return -1;   // someone inside
    if (t == UT(B_SETTLEMENT)) return cg_spent(idx, u, CG_FIELD_R, now) ? 0 : -1;
    // FIELDS AND GRANARIES (the player 2026-11-24: "in the erly game the ai builds a lot to boom its economy, in later
    // stages many fields stay unused and are redundant"): a field nobody has farmed for CG_IDLE, a granary whose fields
    // are all such and no food is gathered round it - its last granary in the extreme only
    if (is_field_type(t)) return cg_idle(idx, u, now) ? 0 : -1;
    if (t == UT(B_GRANARY) || t == UT(B_GRANARY_IMPROVED)) return !cg_spent(idx, u, CG_GRAN_R, now) ? -1 : g_cg[idx].ngran >= 2 ? 0 : 1;
    if (t == UT(B_HOUSE)) return 1;
    if (is_tower_type(t) || is_wall(t) || unit_type_get_class_id(td) == CLS_GATE || unit_type_get_family(td) == KIND_AA_BUILDING) return 2;
    if (k < 0) return -1;
    unsigned r = ter_near(unit_get_x(u), unit_get_y(u));
    int held = r < (unsigned)g_ter.n && p > 0 && p < 16 ? g_ter.t[r].hold[p] : 0, more = kinds[k] >= 2;
    if (cg_idle(idx, u, now)) return more && held >= 2 ? 0 : 1;
    return more ? 1 : -1;
}
// build.c: a step found no spot for type, round at (rings r0 .. r1; margin: tiles round the footprint it needs free too)
internal void cg_want(unsigned idx, int type, ivec2 at, int r0, int r1, int margin, int extreme, uint32_t now) {
    struct cg_want *c = NULL, *w = g_cg[idx].w;
    for (int k = 0; k < CG_WANTS && !c; k++) if (w[k].type == type) c = &w[k];
    for (int k = 0; k < CG_WANTS && !c; k++) if (!w[k].last || now < w[k].last || now - w[k].last > 30000) { c = &w[k]; c->type = 0; }   // a free or stale one
    if (!c) return;
    if (c->type != type || !c->last || now < c->last || now - c->last > 30000) { memset(c, 0, sizeof *c); c->type = type; c->first = now ? now : 1; }   // met meanwhile: a new want
    c->at = at; c->r0 = r0; c->r1 = r1; c->margin = margin; c->extreme = extreme; c->last = now ? now : 1;
}
// the tiles a footprint at q needs: type's square grown by margin, an Airport's runway pieces too
internal int cg_tiles(int type, ivec2 q, int margin, int n, ivec2 *lo, ivec2 *hi) {
    ee_unit_type *td = type_data(type);
    int s = td && unit_type_get_size(td) > 0 ? unit_type_get_size(td) : 1, k = 0;
    lo[k] = iv2(q.x - margin, q.y - margin); hi[k++] = iv2(q.x + s - 1 + margin, q.y + s - 1 + margin);
    if (type == UT(B_AIRPORT)) {
        ivec2 pc[6];
        int rt = ut_has(UT(B_AIRPORT_RUNWAY)) ? UT(B_AIRPORT_RUNWAY) : type, m = ap_strip(q, n, pc);
        int rs = type_data(rt) && unit_type_get_size(type_data(rt)) > 0 ? unit_type_get_size(type_data(rt)) : s;
        for (int j = 0; j < m; j++) { lo[k] = pc[j]; hi[k++] = iv2(pc[j].x + rs - 1, pc[j].y + rs - 1); }
    }
    return k;
}
// why the anchors of the last look were refused (the log's no ground line): off the land, a tree or mine, a field, a
// building kept, one worth more than the look may take, too many, the last of a kind, the game's test where nothing is
// in the way, a spot cleared before that still took nothing, nothing in the way
enum { CGW_LAND, CGW_RES, CGW_FIELD, CGW_KEPT, CGW_WORTH, CGW_MANY, CGW_LAST, CGW_GAME, CGW_TRIED, CGW_FREE, CGW_N };
internal int g_cgw[CGW_N], g_cg_kept[G_TYPES_N];
// the buildings in the way of a footprint at q (into set, at most CG_SET): -1 something there it may not take down
// (or water, a tree, a mine, a field under the building itself), else the sum of their worth + 1 each, 0 nothing
internal int cg_fits(ee_player *pl, unsigned idx, int type, ivec2 q, int margin, int limit, const int *kinds, ee_unit **set, int *nset, uint32_t now) {
    ee_map *map = game_map();
    int n = map ? (int)map_get_size(map) : 0, cost = 0;
    ivec2 lo[8], hi[8];
    int nb = cg_tiles(type, q, margin, n, lo, hi), took[PR_NK] = { 0 }, in_way[8] = { 0 }, took_gran = 0;
    *nset = 0;
    if (!n || !g_ter.cls || g_ter.size != n) return -1;
    for (int b = 0; b < nb; b++)
        for (int y = lo[b].y; y <= hi[b].y; y++)
            for (int x = lo[b].x; x <= hi[b].x; x++) {
                if (x < 2 || y < 2 || x >= n - 2 || y >= n - 2 || (g_ter.cls[y * n + x] & 3) != TER_LAND) { g_cgw[CGW_LAND]++; return -1; }
                int inner = b || (x >= q.x && y >= q.y && x <= hi[0].x - margin && y <= hi[0].y - margin);   // not a granary's fields' places
                ee_tile *tile = map_get_tiles(map)[y * n + x];
                for (void *node = tile ? tile_get_objects(tile) : NULL; node; node = tilenode_get_next(node)) {
                    ee_unit *o = tilenode_get_object(node);
                    ee_unit_type *otd = o ? unit_get_type_data(o) : NULL;
                    int cls = otd ? unit_type_get_class_id(otd) : -1, ot = otd ? unit_type_get_id(otd) : -1;
                    if (!otd || cls == CLS_AMBIENT || cls == CLS_PROJECTILE) continue;
                    if (is_field_type(ot) && !inner) continue;   // a field round a granary is one of its own
                    if (cls == CLS_RESOURCE) { g_cgw[CGW_RES]++; return -1; }
                    if (cls != CLS_BUILDING && cls != CLS_FORTRESS && cls != CLS_GATE && cls != CLS_NAVAL_BUILDING && cls != CLS_SPACE_BUILDING) continue;   // units walk off
                    int j = 0;
                    in_way[b] = 1;
                    while (j < *nset && set[j] != o) j++;
                    if (j < *nset) continue;
                    int w = cg_worth(pl, idx, o, kinds, now), k = pr_kind(ot);
                    if (w < 0 && is_field_type(ot)) { g_cgw[CGW_FIELD]++; return -1; }   // a field farmed
                    if (w < 0) { g_cgw[CGW_KEPT]++; if (ot >= 0 && ot < G_TYPES_N) g_cg_kept[ot]++; return -1; }
                    if (w > limit) { g_cgw[CGW_WORTH]++; return -1; }
                    if (*nset == (limit >= 2 ? CG_SET_AIR : CG_SET)) { g_cgw[CGW_MANY]++; return -1; }
                    if (k >= 0 && ++took[k] >= kinds[k] && limit < 1) { g_cgw[CGW_LAST]++; return -1; }   // the last of a kind: the extreme's only
                    if ((ot == UT(B_GRANARY) || ot == UT(B_GRANARY_IMPROVED)) && ++took_gran >= g_cg[idx].ngran && limit < 1) { g_cgw[CGW_LAST]++; return -1; }   // ... its last granary too
                    set[(*nset)++] = o; cost += w + 1;
                }
            }
    // THE GAME'S OWN TEST where nothing is in the way: a piece of the footprint (the building's square, a runway piece)
    // with no building of its on it must take its piece now - its slope, its ground (2026-11-24: Airports' strips
    // cleared that the game still refused, a tower rebuilt and taken down again at the same spot)
    for (int b = 0; b < nb && *nset; b++) {
        if (in_way[b]) continue;
        int bt = b ? (ut_has(UT(B_AIRPORT_RUNWAY)) ? UT(B_AIRPORT_RUNWAY) : type) : type, x0 = b ? lo[b].x : q.x, y0 = b ? lo[b].y : q.y;
        int bs = b ? hi[b].x - lo[b].x + 1 : hi[0].x - lo[0].x + 1 - 2 * margin;
        if (!can_place_area(pl, bt, x0, y0, bs)) { g_cgw[CGW_GAME]++; return -1; }
    }
    return cost;
}
// build.c, each look after its steps: a clearing for the want, when it is due
internal void cg_clear(ee_player *pl, unsigned idx, uint32_t now) {
    struct cg_st *g = &g_cg[idx];
    struct cg_want *c = NULL;
    if (!g_set_ai[1].clear_ground) return;
    if (!g->sum_t || now < g->sum_t || now - g->sum_t >= 300000) {   // what is disposable now, every 5 minutes (the log)
        int kinds[PR_NK] = { 0 }, count = 0, nd[4] = { 0 }, nu[4] = { 0 };   // fields, granaries, Settlements, production: disposable, all
        g->sum_t = now ? now : 1; g->ngran = 0;
        for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {
            ee_unit_type *td = unit_get_type_data(u);
            int t = td && unit_get_hp(u) > 0 ? unit_type_get_id(td) : -1, k = t >= 0 ? pr_kind(t) : -1;
            if (k >= 0) kinds[k]++;
            g->ngran += t >= 0 && (t == UT(B_GRANARY) || t == UT(B_GRANARY_IMPROVED));
        }
        count = 0;
        for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {
            ee_unit_type *td = unit_get_type_data(u);
            int t = td && unit_get_hp(u) > 0 ? unit_type_get_id(td) : -1;
            int c4 = t < 0 ? -1 : is_field_type(t) ? 0 : t == UT(B_GRANARY) || t == UT(B_GRANARY_IMPROVED) ? 1 : t == UT(B_SETTLEMENT) ? 2 : pr_kind(t) >= 0 ? 3 : -1;
            if (c4 < 0) continue;
            nu[c4]++; nd[c4] += cg_worth(pl, idx, u, kinds, now) == 0;
        }
        log_locked("# clearing: P%u at %.1f min: disposable now %d of %d fields, %d of %d granaries, %d of %d Settlements, %d of %d production buildings | so far %u clearings (%u in the extreme), %u buildings taken down\n",
                   idx, now / 60000.0, nd[0], nu[0], nd[1], nu[1], nd[2], nu[2], nd[3], nu[3], g->cleared, g->extreme_n, g->taken);
    }
    for (int k = 0; k < CG_WANTS; k++) {   // of the wants unmet this look, the one waiting longest
        struct cg_want *w = &g->w[k];
        if (!w->last || w->last != now || now - w->first < CG_PATIENCE || (w->cool_t && now < w->cool_t)) continue;
        if (w->done_t && now >= w->done_t && now - w->done_t < CG_EVERY) continue;
        if (w->done_t) { w->cool_t = now + CG_COOL; w->done_t = 0; continue; }   // looked for it and still no spot: the slope or the rules - wait (its spot in tried)
        if (!c || w->first < c->first) c = w;
    }
    if (!c) return;
    int big = c->type == UT(B_AIRPORT) || c->type == UT(B_UNIVERSITY) || c->type == UT(B_HOSPITAL) || c->type == UT(B_TEMPLE) || pr_kind(c->type) >= 0;
    int limit = c->extreme && big && now - c->first >= CG_EXTREME ? c->type == UT(B_AIRPORT) ? 2 : 1 : 0;
    int kinds[PR_NK] = { 0 }, count = 0;
    g->ngran = 0;
    for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {
        ee_unit_type *td = unit_get_type_data(u);
        int k = td && unit_get_hp(u) > 0 ? pr_kind(unit_type_get_id(td)) : -1;
        if (k >= 0) kinds[k]++;
        g->ngran += td && unit_get_hp(u) > 0 && (unit_type_get_id(td) == UT(B_GRANARY) || unit_type_get_id(td) == UT(B_GRANARY_IMPROVED));
    }
    static ee_unit *set[CG_SET_AIR], *best[CG_SET_AIR];
    int nset = 0, nbest = 0, bcost = 0x7FFFFFFF;
    ivec2 bq = iv2(0, 0);
    memset(g_cgw, 0, sizeof g_cgw); memset(g_cg_kept, 0, sizeof g_cg_kept);
    for (int r = c->r0; r <= c->r1; r++)
        for (int ax = -r; ax <= r; ax++)
            for (int ay = -r; ay <= r; ay++) {
                if (abs(ax) != r && abs(ay) != r) continue;
                ivec2 q = iv2(c->at.x + ax, c->at.y + ay);
                if (tv_unsafe(idx, q.x, q.y) || wk_danger(idx, q)) continue;
                int was = 0;
                for (int k = 0; k < c->ntried && !was; k++) was = abs(c->tried[k].x - q.x) <= 2 && abs(c->tried[k].y - q.y) <= 2;
                if (was) { g_cgw[CGW_TRIED]++; continue; }
                int cost = cg_fits(pl, idx, c->type, q, c->margin, limit, kinds, set, &nset, now);
                g_cgw[CGW_FREE] += !cost;
                if (cost <= 0 || cost >= bcost) continue;   // 0: free already - no spot there for another reason
                bcost = cost; bq = q; nbest = nset; memcpy(best, set, (size_t)nset * sizeof set[0]);
            }
    c->done_t = now ? now : 1;
    if (!nbest) {   // nothing it may take down stands where it would fit (once a want in CG_COOL)
        int k1 = 0, k2 = 0;   // the kept buildings most in the way
        for (int t = 1; t < G_TYPES_N; t++) { if (g_cg_kept[t] > g_cg_kept[k1]) { k2 = k1; k1 = t; } else if (g_cg_kept[t] > g_cg_kept[k2]) k2 = t; }
        log_locked("# clearing: P%u at %.1f min: no spot for a %d for %.1f min - no ground to clear%s (anchors refused: off the land %d, a tree or mine %d, a field %d, a building kept %d - most a %d %d, a %d %d -, worth more %d, too many %d, the last of a kind %d, the game's test %d, cleared before %d; free %d)\n",
                   idx, now / 60000.0, c->type, (now - c->first) / 60000.0, limit ? " (in the extreme)" : "", g_cgw[CGW_LAND], g_cgw[CGW_RES], g_cgw[CGW_FIELD], g_cgw[CGW_KEPT],
                   k1, g_cg_kept[k1], k2, g_cg_kept[k2], g_cgw[CGW_WORTH], g_cgw[CGW_MANY], g_cgw[CGW_LAST], g_cgw[CGW_GAME], g_cgw[CGW_TRIED], g_cgw[CGW_FREE]);
        return;
    }
    g->cleared++; g->taken += (unsigned)nbest; g->extreme_n += limit > 0;
    c->tried[c->ntried < CG_TRIED ? c->ntried++ : (int)(g->cleared % CG_TRIED)] = bq;   // met there, the want ends: tried goes with it
    char took[160];
    int o = 0;
    took[0] = 0;
    for (int k = 0; k < nbest && o < 120; k++) o += snprintf(took + o, sizeof took - (size_t)o, " %d at %d,%d", unit_type_get_id(unit_get_type_data(best[k])), unit_get_x(best[k]), unit_get_y(best[k]));
    log_locked("# clearing: P%u at %.1f min: no spot for a %d for %.1f min - ground at %d,%d made by taking down%s%s | so far %u clearings (%u in the extreme), %u buildings taken down\n",
               idx, now / 60000.0, c->type, (now - c->first) / 60000.0, bq.x, bq.y, took, limit ? " (in the extreme: usable ones too)" : "", g->cleared, g->extreme_n, g->taken);
    for (int k = 0; k < nbest; k++) unit_destroy(best[k], 1, 0, 0);
}
