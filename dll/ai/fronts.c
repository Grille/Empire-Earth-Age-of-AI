// FRONTS: where an AI's land meets land the enemy can walk over, and how much each stretch matters - the ground for
// standing front armies (the player 2026-10-31: "1 or 2 front armies, they hold the front, push in, retreat into
// fortified positions when losing; smaller defence forces in the backland" - and "it depends a lot on the map: in
// middle ocean two armies for both ends of my reach, on flatland more to spread over my whole open border, with
// concentrations only where I want to attack or fear a concentrated enemy push"). No count of armies is set: the
// border found here decides - water, cliffs and the map's edge are no border, so the terrain makes the fronts.
// Every 10 s per AI, the map in FR_CELL-tile cells (land: most of 5 tiles of it land, inside the map's diamond - the
// citizen may stand on its terrain kind, as the route planner tests; the AI areas' ids mark a coastal area both sea and land):
//   its territory - land cells within FR_TERR cells of its buildings, along land (not over water);
//   the enemy's - the same round the enemy buildings it knows (intel, honest);
//   border - a cell of its territory beside a land cell that isn't, on land where an enemy building it knows stands
//            (the enemy can walk there: area class 2), or beside the enemy's territory;
//   fronts - border cells within 2 cells of each other, joined; each one's width (cells), middle, threat (the enemy
//            buildings and soldiers it knows within FR_REACH tiles beyond, by strength), value (its own buildings
//            within FR_REACH tiles behind) and weight = width x (1 + threat) x (1 + value) / 4, its share of the
//            weights its share of the army to be.
// Nothing moves yet: logged ("# fronts:", every 5 minutes in tests) and drawn on the overlay's "fronts" layer (its
// territory faint, the border strong, each front's middle a circle by its weight). ai_base fronts_log (tests).
#define FR_CELL 8
#define FR_SIDE 128              // a 1024-tile map; a bigger one folds its far cells onto the edge
#define FR_TERR 3                // territory: 3 cells (24 tiles) round its buildings
#define FR_REACH 48
#define FR_MAX 32
struct front_st { ivec2 c; int width, threat, value; float weight, share; };
internal struct fronts_st { uint32_t t, log_t; int n; struct front_st f[FR_MAX]; int border, terr; } g_fronts[16];
internal uint8_t g_fr_land[FR_SIDE * FR_SIDE], g_fr_own[FR_SIDE * FR_SIDE], g_fr_foe[FR_SIDE * FR_SIDE];   // per look (sim thread)
internal short g_fr_q[FR_SIDE * FR_SIDE * 2];
internal unsigned g_fr_area[FR_SIDE * FR_SIDE];
internal int g_fr_lab[FR_SIDE * FR_SIDE];
internal int fr_tile_land(ee_map *map, int size, int x, int y) {
    if (x < 0 || y < 0 || x >= size || y >= size) return 0;
    if (x + y <= map_get_edge_sum(map) || x + y >= map_get_edge_sum_2(map) || y - x >= map_get_edge_diff(map) || x - y >= map_get_edge_diff(map)) return 0;
    ee_tile *tile = map_get_tiles(map)[y * size + x];
    static ee_unit_type *walker;   // a citizen: may it stand there (its type's terrain table, as the route planner asks)
    if (!walker) walker = type_data(UT(CITIZEN));
    return tile && walker && unit_type_terrain_ok(walker, tile_get_kind(tile));
}
internal int fr_building(ee_unit_type *td) {
    int fam = unit_type_get_family(td);
    return fam == CLS_BUILDING || fam == CLS_FORTRESS || is_tower_type(unit_type_get_id(td));
}
// territory: distance in cells from the seeds marked 1 in m, along land, up to FR_TERR (m: 0 none, else 1 + distance)
internal void fr_spread(uint8_t *m, int side) {
    int h = 0, t = 0;
    for (int i = 0; i < side * side; i++) if (m[i] == 1) { g_fr_q[t++] = (short)(i % side); g_fr_q[t++] = (short)(i / side); }
    while (h < t) {
        int x = g_fr_q[h++], y = g_fr_q[h++], d = m[y * side + x];
        if (d > FR_TERR) continue;
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                int nx = x + dx, ny = y + dy;
                if (nx < 0 || ny < 0 || nx >= side || ny >= side) continue;
                int j = ny * side + nx;
                if (m[j] || !g_fr_land[j] || t + 2 > (int)(sizeof g_fr_q / sizeof *g_fr_q)) continue;
                m[j] = (uint8_t)(d + 1);
                g_fr_q[t++] = (short)nx; g_fr_q[t++] = (short)ny;
            }
    }
}
internal void fronts_look(ee_player *pl, unsigned idx, uint32_t now) {
    ee_map *map = game_map();
    int size = map ? (int)map_get_size(map) : 0, side = (size + FR_CELL - 1) / FR_CELL;
    if (side <= 0) return;
    if (side > FR_SIDE) side = FR_SIDE;
    struct fronts_st *s = &g_fronts[idx];
    int n = side * side;
    for (int i = 0; i < n; i++) {
        int x0 = (i % side) * FR_CELL, y0 = (i / side) * FR_CELL, h = FR_CELL / 2, land = 0;
        land += fr_tile_land(map, size, x0 + h, y0 + h) + fr_tile_land(map, size, x0 + 1, y0 + 1) + fr_tile_land(map, size, x0 + FR_CELL - 2, y0 + 1) +
                fr_tile_land(map, size, x0 + 1, y0 + FR_CELL - 2) + fr_tile_land(map, size, x0 + FR_CELL - 2, y0 + FR_CELL - 2);
        g_fr_land[i] = land >= 3;
        g_fr_area[i] = g_fr_land[i] ? area_id(iv2(x0 + h, y0 + h), 2) : 0;
    }
    memset(g_fr_own, 0, (size_t)n); memset(g_fr_foe, 0, (size_t)n);
    #define FR_IDX(p) ((((p).y / FR_CELL) < side ? (p).y / FR_CELL : side - 1) * side + (((p).x / FR_CELL) < side ? (p).x / FR_CELL : side - 1))
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {   // its buildings
        ee_unit *u = it.u;
        if (unit_get_owner(u) != pl || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !fr_building(it.td)) continue;
        int i = FR_IDX(unit_get_tile(u));
        if (g_fr_land[i]) g_fr_own[i] = 1;
    }
    static unsigned foe_area[64];
    int nfa = 0;
    struct intel *in = &g_intel[idx];
    for (int k = 0; k < in->n; k++) {   // the enemy buildings it knows
        struct intel_rec *r = &in->r[k];
        ee_player *o = r->owner > 0 && r->owner < 16 ? player_at(r->owner) : NULL;
        if (r->kind != 0 || !o || !is_enemy(pl, o) || r->x < 0 || r->y < 0) continue;
        int i = FR_IDX(iv2(r->x, r->y));
        if (g_fr_land[i]) g_fr_foe[i] = 1;
        unsigned a = area_id(iv2(r->x, r->y), 2);
        int seen = 0;
        for (int j = 0; j < nfa; j++) seen |= foe_area[j] == a;
        if (!seen && a && nfa < 64) foe_area[nfa++] = a;
    }
    fr_spread(g_fr_own, side);
    fr_spread(g_fr_foe, side);
    // border cells: labelled 0 not, -1 border (to be joined)
    int border = 0, terr = 0;
    for (int i = 0; i < n; i++) {
        g_fr_lab[i] = 0;
        if (!g_fr_own[i]) continue;
        terr++;
        int x = i % side, y = i / side, b = 0;
        for (int dy = -1; dy <= 1 && !b; dy++)
            for (int dx = -1; dx <= 1 && !b; dx++) {
                int nx = x + dx, ny = y + dy;
                if (nx < 0 || ny < 0 || nx >= side || ny >= side) continue;
                int j = ny * side + nx;
                if (!g_fr_land[j]) continue;
                if (g_fr_foe[j]) { b = 1; break; }
                if (g_fr_own[j]) continue;
                for (int a = 0; a < nfa && !b; a++) b = g_fr_area[j] == foe_area[a];
            }
        if (b) { g_fr_lab[i] = -1; border++; }
    }
    // fronts: border cells within 2 cells of each other
    s->n = 0;
    struct front_st f[FR_MAX];
    int nf = 0;
    for (int i = 0; i < n; i++) {
        if (g_fr_lab[i] != -1) continue;
        int lab = nf < FR_MAX ? nf + 1 : FR_MAX, h = 0, t = 0, cnt = 0, sx = 0, sy = 0;
        g_fr_lab[i] = lab;
        g_fr_q[t++] = (short)(i % side); g_fr_q[t++] = (short)(i / side);
        while (h < t) {
            int x = g_fr_q[h++], y = g_fr_q[h++];
            cnt++; sx += x; sy += y;
            for (int dy = -2; dy <= 2; dy++)
                for (int dx = -2; dx <= 2; dx++) {
                    int nx = x + dx, ny = y + dy;
                    if (nx < 0 || ny < 0 || nx >= side || ny >= side) continue;
                    int j = ny * side + nx;
                    if (g_fr_lab[j] != -1 || t + 2 > (int)(sizeof g_fr_q / sizeof *g_fr_q)) continue;
                    g_fr_lab[j] = lab;
                    g_fr_q[t++] = (short)nx; g_fr_q[t++] = (short)ny;
                }
        }
        if (nf >= FR_MAX) continue;   // the rest folded into nothing: logged as border cells only
        struct front_st *fr = &f[nf++];
        memset(fr, 0, sizeof *fr);
        fr->width = cnt;
        fr->c = iv2(sx * FR_CELL / cnt + FR_CELL / 2, sy * FR_CELL / cnt + FR_CELL / 2);
    }
    // threat beyond and value behind each front
    for (int k = 0; k < in->n; k++) {
        struct intel_rec *r = &in->r[k];
        ee_player *o = r->owner > 0 && r->owner < 16 ? player_at(r->owner) : NULL;
        if (!o || !is_enemy(pl, o) || (r->kind != 0 && r->kind != 1)) continue;
        for (int j = 0; j < nf; j++)
            if (ivec2_dist2(f[j].c, iv2(r->x, r->y)) <= FR_REACH * FR_REACH) f[j].threat += r->kind == 1 ? 2 : 1;   // a soldier counts twice a building
    }
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it) && nf; ) {
        ee_unit *u = it.u;
        if (unit_get_owner(u) != pl || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !fr_building(it.td)) continue;
        for (int j = 0; j < nf; j++) if (ivec2_dist2(f[j].c, unit_get_tile(u)) <= FR_REACH * FR_REACH) f[j].value++;
    }
    float total = 0;
    for (int j = 0; j < nf; j++) {
        f[j].weight = (float)f[j].width * (1.0f + (float)f[j].threat) * (1.0f + (float)f[j].value) / 4.0f;
        total += f[j].weight;
    }
    for (int j = 0; j < nf; j++) f[j].share = total > 0 ? f[j].weight / total : 0;
    memcpy(s->f, f, (size_t)nf * sizeof *f);
    s->n = nf; s->border = border; s->terr = terr;
    // the overlay
    static int layer = -2;
    if (layer == -2) layer = overlay_layer("fronts");
    if (overlay_on(layer)) {
        overlay_begin(layer, idx, now, 30000);
        for (int i = 0; i < n; i++) {
            if (!g_fr_own[i]) continue;
            int x0 = (i % side) * FR_CELL, y0 = (i / side) * FR_CELL;
            for (int y = 0; y < FR_CELL; y += g_fr_lab[i] ? 1 : FR_CELL / 2)
                overlay_tiles(iv2(x0, y0 + y), FR_CELL, overlay_player_argb((int)idx, g_fr_lab[i] ? 120 : 16));
        }
        for (int j = 0; j < nf; j++)
            overlay_circle(v2((float)f[j].c.x, (float)f[j].c.y), 4.0f + 20.0f * f[j].share, overlay_player_argb((int)idx, 220), 0.5f);
        overlay_end();
    }
    if (g_log && g_set.fronts_log && (!s->log_t || now < s->log_t || now - s->log_t >= 300000)) {
        s->log_t = now ? now : 1;
        EnterCriticalSection(&g_lock);
        int nl = 0, ns = 0;
        for (int i = 0; i < n; i++) { nl += g_fr_land[i]; ns += g_fr_own[i] == 1; }
        fprintf(g_log, "# fronts: P%u at %.1f min: map %d cells (%d land), its buildings in %d, territory %d cells, border %d, %d fronts", idx, now / 60000.0, n, nl, ns, terr, border, nf);
        for (int j = 0; j < nf; j++)
            fprintf(g_log, " | %d,%d width %d threat %d value %d share %.0f%%", f[j].c.x, f[j].c.y, f[j].width, f[j].threat, f[j].value, 100 * f[j].share);
        fputc('\n', g_log);
        LeaveCriticalSection(&g_lock);
    }
    #undef FR_IDX
}
internal void fronts_tick(ee_player *pl, unsigned idx, uint32_t now) {
    struct fronts_st *s = &g_fronts[idx];
    if (now < s->t) memset(s, 0, sizeof *s);   // a new game
    if (s->t && now - s->t < 10000) return;
    s->t = now ? now : 1;
    fronts_look(pl, idx, now);
}
internal void fronts_new_game(void) { memset(g_fronts, 0, sizeof g_fronts); }
