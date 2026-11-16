// WATCHING ITS OWN GROUND ([ai_base] fog_patrols, watch_towers; [autotest] v_fog_patrols, v_watch_towers). The player,
// 2026-11-16: "the ai is leaving large parts of its territory in the fog of war, thas dangerus, it should regulary patrol
// these alrger unknows patches with single units, and build towers in the long term so it knows whats going on in its
// own borders, right now its to easy to sneak in and build a whole base witout it ever noticing". The scouts (stations.c
// SCOUTS) stand in every territory once and are done; the border towers (territories.c) cover only the outside border.
// Every WT_EVERY per AI, the map in cells of WT_CELL x WT_CELL tiles: a cell is in its sight when the tile in its middle
// is (the game's sight count for the player, 2 and more - engine/sight.c; its own sight, not its allies'), and keeps
// when it last was (last) and how much of the time (share: 255 always, an eighth of the way towards in or out a look).
//   ITS GROUND - the open land (no tree at the territories' build) of the land territories it holds (territories.c's
//               role) and of the land territories beside them that nobody of its side holds and it knows no enemy
//               building in: where a base sneaked in behind its lines or next door would stand.
//   DARK       - a cell of its ground not in its sight for WT_AGE; a PATCH WT_PATCH or more dark cells side by side
//               (8 ways: a few dark tiles behind a wood are nothing to walk to), largest first, its middle the dark cell
//               nearest the mean of its cells. Not in a territory with enemy soldiers in it: that fight is known.
//   PATROLS    - (stations.c PATROLS) single soldiers of the main force sent at the patches.
//   DIM        - a cell of a land territory it holds that it had in sight less than WT_DIM of 255 of the time, after
//               WT_WARM of watching; WT_TOWER_PATCH of them side by side are a place for a WATCH TOWER (build.c), largest
//               first.
// "# watch:" a minute.
#define WT_CELL 4
#define WT_EVERY 10000
#define WT_AGE 240000             // DARK: out of its sight this long
#define WT_PATCH 4                // ... cells side by side at least
#define WT_PATCHES 64
#define WT_DIM 64                 // DIM: in its sight less than a quarter of the time
#define WT_TOWER_PATCH 6          // ... cells side by side at least
#define WT_WARM 600000            // ... watched this long first (the share starts at 0)
#define WT_TAKEN 12               // a patrol takes no patch within this many tiles of another's
#define WT_PATROLS 4              // PATROLS at most
#define WT_PATROL_ARMY 30         // ... one more for every this many soldiers of the main force
#define WT_PATROL_FORCE 3         // ... none below this many (as the scouts)
#define WT_GIVE_UP 150000         // a patch not seen this long after a patrol took it: given up (WT_AGE before it is dark again)
struct wt_patch { ivec2 at; short ter, cells; };
internal struct wt_st {
    uint32_t t, start_t, log_t;
    int cn;                                  // cells across (0: none yet)
    uint32_t *last; uint8_t *share;          // per cell
    int np, ntp; struct wt_patch p[WT_PATCHES], tp[WT_PATCHES];   // the dark patches, the dim ones (tower places), largest first
    int ground, dark, dim;                   // the last look's cells
    unsigned taken, looked, gave_up, towers; // patrols taken, patches seen by them, given up; watch towers laid
} g_wt[16];
internal int g_fog_patrols = 0, g_v_fog_patrols = -1, g_watch_towers = 0, g_v_watch_towers = -1;
#define FOG_PATROLS(idx) PV(idx, g_v_fog_patrols, g_fog_patrols)
#define WATCH_TOWERS(idx) PV(idx, g_v_watch_towers, g_watch_towers)
// the tile p in pl's sight now
internal int wt_in_sight(ee_player *pl, ivec2 p) {
    ee_map *map = game_map();
    int size = map ? (int)map_get_size(map) : 0;
    if (!pl || p.x < 0 || p.y < 0 || p.x >= size || p.y >= size) return 0;
    ee_tile *t = map_get_tiles(map)[p.y * size + p.x];
    return t && sight_count(tile_get_sight(t)[player_get_index(pl)]) >= 2;
}
internal int wt_patch_cmp(const void *a, const void *b) { return ((const struct wt_patch *)b)->cells - ((const struct wt_patch *)a)->cells; }
// the patches of the cells marked 1 in mark (8 ways, min cells at least) into out, largest first (cap at most)
internal int wt_patches(struct wt_st *w, uint8_t *mark, int *q, int min, struct wt_patch *out, int cap) {
    static struct wt_patch all[1024];
    int na = 0, cn = w->cn;
    for (int c0 = 0; c0 < cn * cn; c0++) {
        if (mark[c0] != 1) continue;
        int qh = 0, qt = 0;
        long sx = 0, sy = 0;
        q[qt++] = c0; mark[c0] = 2;
        while (qh < qt) {
            int c = q[qh++], cx = c % cn, cy = c / cn;
            sx += cx; sy += cy;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++) {
                    int x = cx + dx, y = cy + dy;
                    if (x < 0 || y < 0 || x >= cn || y >= cn || mark[y * cn + x] != 1) continue;
                    mark[y * cn + x] = 2; q[qt++] = y * cn + x;
                }
        }
        if (qt < min || na >= 1024) continue;
        int mx = (int)(sx / qt), my = (int)(sy / qt), b = q[0], bd = 0x7FFFFFFF;
        for (int k = 0; k < qt; k++) {   // its middle: the cell of it nearest the mean (a patch round a lake has its mean in the water)
            int d = (q[k] % cn - mx) * (q[k] % cn - mx) + (q[k] / cn - my) * (q[k] / cn - my);
            if (d < bd) { bd = d; b = q[k]; }
        }
        ivec2 at = iv2((b % cn) * WT_CELL + WT_CELL / 2, (b / cn) * WT_CELL + WT_CELL / 2);
        all[na].at = at; all[na].cells = (short)(qt < 32767 ? qt : 32767); all[na++].ter = (short)ter_near(at.x, at.y);
    }
    qsort(all, (size_t)na, sizeof *all, wt_patch_cmp);
    if (na > cap) na = cap;
    memcpy(out, all, (size_t)na * sizeof *out);
    return na;
}
internal void watch_tick(ee_player *pl, unsigned idx, uint32_t now) {
    static uint8_t *mark;
    static int *q, qcap;
    struct wt_st *w = &g_wt[idx & 15];
    struct tv_st *s = &g_tv[idx & 15];
    if (now < w->t) { w->t = w->start_t = w->log_t = 0; w->np = w->ntp = 0; if (w->cn) { memset(w->last, 0, (size_t)w->cn * w->cn * sizeof *w->last); memset(w->share, 0, (size_t)w->cn * w->cn); } }
    if (w->t && now - w->t < WT_EVERY) return;
    w->t = now ? now : 1;
    ee_map *map = game_map();
    int size = map ? (int)map_get_size(map) : 0, cn = (size + WT_CELL - 1) / WT_CELL;
    if (!size || !s->n || s->version != g_ter.version || g_ter.size != size || !g_ter.lab || !g_ter.cls) return;
    if (w->cn != cn) {
        free(w->last); free(w->share);
        w->last = calloc((size_t)cn * cn, sizeof *w->last); w->share = calloc((size_t)cn * cn, 1);
        w->cn = w->last && w->share ? cn : 0;
        w->start_t = now ? now : 1;
        if (!w->cn) return;
    }
    if (!w->start_t) w->start_t = now ? now : 1;
    if (qcap < cn * cn) {
        free(mark); free(q);
        mark = malloc((size_t)cn * cn); q = malloc((size_t)cn * cn * sizeof *q);
        qcap = mark && q ? cn * cn : 0;
        if (!qcap) return;
    }
    // its ground: what it holds, and the land beside that nobody of its side holds and it knows no enemy building in
    static uint8_t gr[PR_T];
    int n = s->n < PR_T ? s->n : PR_T;
    for (int r = 0; r < n; r++) gr[r] = !g_ter.t[r].water && s->t[r].role != TVR_NONE ? 2 : 0;
    for (int r = 0; r < n; r++) {
        if (gr[r] || g_ter.t[r].water || s->t[r].friendly || s->t[r].ebld) continue;
        for (int k = 0; k < g_ter.t[r].nbn && !gr[r]; k++) { unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to; if (o < (unsigned)n && gr[o] == 2) gr[r] = 1; }
    }
    ee_tile **tiles = map_get_tiles(map);
    unsigned pi = (unsigned)player_get_index(pl);
    int warm = now - w->start_t >= WT_WARM;
    w->ground = w->dark = w->dim = 0;
    memset(mark, 0, (size_t)cn * cn);   // the dark cells 1 (wt_patches)
    for (int cy = 0; cy < cn; cy++)
        for (int cx = 0; cx < cn; cx++) {
            int c = cy * cn + cx, x = cx * WT_CELL + WT_CELL / 2, y = cy * WT_CELL + WT_CELL / 2;
            if (x >= size || y >= size) continue;
            unsigned r = g_ter.lab[y * size + x];
            ee_tile *t = tiles[y * size + x];
            if (!t || r >= (unsigned)n || (g_ter.cls[y * size + x] & (TER_LAND | TER_TREE)) != TER_LAND) continue;
            int in = sight_count(tile_get_sight(t)[pi]) >= 2;
            if (in) w->last[c] = now ? now : 1;
            w->share[c] = (uint8_t)(w->share[c] - w->share[c] / 8 + (in ? 31 : 0));
            if (!gr[r]) continue;
            w->ground++;
            if ((!w->last[c] || now - w->last[c] >= WT_AGE) && !s->t[r].foe) { mark[c] = 1; w->dark++; }
            w->dim += warm && gr[r] == 2 && w->share[c] < WT_DIM;
        }
    w->np = wt_patches(w, mark, q, WT_PATCH, w->p, WT_PATCHES);
    w->ntp = 0;
    if (warm && WATCH_TOWERS(idx)) {   // the dim ones, marked anew (the dark ones' mark is spent)
        uint8_t *dim = mark;
        memset(dim, 0, (size_t)cn * cn);
        for (int cy = 0; cy < cn; cy++)
            for (int cx = 0; cx < cn; cx++) {
                int c = cy * cn + cx, x = cx * WT_CELL + WT_CELL / 2, y = cy * WT_CELL + WT_CELL / 2;
                if (x >= size || y >= size) continue;
                unsigned r = g_ter.lab[y * size + x];
                if (r < (unsigned)n && gr[r] == 2 && tiles[y * size + x] && (g_ter.cls[y * size + x] & (TER_LAND | TER_TREE)) == TER_LAND && w->share[c] < WT_DIM) dim[c] = 1;
            }
        w->ntp = wt_patches(w, dim, q, WT_TOWER_PATCH, w->tp, WT_PATCHES);
    }
    if (g_log && (!w->log_t || now - w->log_t >= 60000)) {
        w->log_t = now ? now : 1;
        log_locked("# watch: P%u at %.1f min: its ground %d cells of %dx%d tiles, dark (out of sight %d s) %d in %d patches of %d+ (the largest %d at %d,%d), "
                   "seen under %d%% of the time in what it holds %d in %d places for a tower | so far %u patrols taken, %u patches seen, %u given up, %u watch towers laid\n",
                   idx, now / 60000.0, w->ground, WT_CELL, WT_CELL, WT_AGE / 1000, w->dark, w->np, WT_PATCH, w->np ? w->p[0].cells : 0, w->np ? w->p[0].at.x : 0, w->np ? w->p[0].at.y : 0,
                   WT_DIM * 100 / 255, w->dim, w->ntp, w->taken, w->looked, w->gave_up, w->towers);
    }
}
// the patrols it wants: by the main force's size, no more than there are patches (0 with too few soldiers)
internal int watch_patrols_wanted(unsigned idx, int army) {
    struct wt_st *w = &g_wt[idx & 15];
    if (!FOG_PATROLS(idx) || army < WT_PATROL_FORCE) return 0;
    int want = 1 + army / WT_PATROL_ARMY;
    if (want > WT_PATROLS) want = WT_PATROLS;
    return want < w->np ? want : w->np;
}
// the patch for a patrol at `from` on land `land` (0 any): the nearest, none within WT_TAKEN tiles of another's (taken)
internal int watch_patrol_target(unsigned idx, ivec2 from, unsigned land, const ivec2 *taken, int ntaken, ivec2 *at) {
    struct wt_st *w = &g_wt[idx & 15];
    int b = -1, bd = 0;
    for (int k = 0; k < w->np; k++) {
        int near_ = 0, d = ivec2_dist2(w->p[k].at, from);
        for (int j = 0; j < ntaken && !near_; j++) near_ = ivec2_dist2(taken[j], w->p[k].at) <= WT_TAKEN * WT_TAKEN;
        if (near_ || (land && area_id(w->p[k].at, 2) != land)) continue;
        if (b < 0 || d < bd) { b = k; bd = d; }
    }
    if (b < 0) return 0;
    *at = w->p[b].at;
    return 1;
}
// a patch a patrol could not get a look at: its cells within 2 of p count as seen now (dark again after WT_AGE)
internal void watch_skip(unsigned idx, ivec2 p, uint32_t now) {
    struct wt_st *w = &g_wt[idx & 15];
    int cn = w->cn, cx = p.x / WT_CELL, cy = p.y / WT_CELL;
    for (int y = cy - 2; y <= cy + 2; y++)
        for (int x = cx - 2; x <= cx + 2; x++) if (cn && x >= 0 && y >= 0 && x < cn && y < cn) w->last[y * cn + x] = now ? now : 1;
    for (int k = 0; k < w->np; k++) if (ivec2_dist2(w->p[k].at, p) <= 2 * WT_CELL * 2 * WT_CELL) w->p[k--] = w->p[--w->np];
}
// the k-th place for a watch tower (largest dim patch first): 0 none
internal int watch_tower_spot(unsigned idx, int k, ivec2 *at) {
    struct wt_st *w = &g_wt[idx & 15];
    if (!WATCH_TOWERS(idx) || k < 0 || k >= w->ntp) return 0;
    *at = w->tp[k].at;
    return 1;
}
internal void watch_count(unsigned idx, int what) {   // 0 a patrol taken, 1 a patch seen, 2 given up, 3 a watch tower laid
    struct wt_st *w = &g_wt[idx & 15];
    if (what == 0) w->taken++; else if (what == 1) w->looked++; else if (what == 2) w->gave_up++; else w->towers++;
}
// fogwatch P: AI P's ground as the last look saw it - the dark patches (largest first: the middle, cells, territory), the
// places for a watch tower, the counts so far. fogwatch P x / fogwatch P y: the largest dark patch's middle in a territory P
// holds (-1 none); fogwatch P seen x y: 1 the tile is in P's sight now, else 0 (tests/probes/sneak_base.eeai)
internal void cmd_fogwatch(char *out, const char *args) {
    int p = -1, x = -1, y = -1;
    char what[16] = "";
    sscanf(args, "%d %15s %d %d", &p, what, &x, &y);
    if (p < 1 || p > 15) { REPLY("error usage: fogwatch P [x | y | seen x y]\n"); return; }
    struct wt_st *w = &g_wt[p];
    struct tv_st *s = &g_tv[p];
    if (!strcmp(what, "seen")) { ee_player **pls = game_players(); REPLY("%d\n", p < (int)game_player_count() && wt_in_sight(pls[p], iv2(x, y))); return; }
    if (!strcmp(what, "x") || !strcmp(what, "y")) {
        for (int k = 0; k < w->np; k++)
            if (w->p[k].ter >= 0 && w->p[k].ter < s->n && s->t[w->p[k].ter].role != TVR_NONE) { REPLY("%d\n", what[0] == 'x' ? w->p[k].at.x : w->p[k].at.y); return; }
        REPLY("-1\n");
        return;
    }
    REPLY("P%d: ground %d cells, dark %d in %d patches, dim %d in %d tower places | patrols taken %u, patches seen %u, given up %u, watch towers laid %u\n",
          p, w->ground, w->dark, w->np, w->dim, w->ntp, w->taken, w->looked, w->gave_up, w->towers);
    for (int k = 0; k < w->np; k++) REPLY("dark %d %d %d cells, territory %d\n", w->p[k].at.x, w->p[k].at.y, w->p[k].cells, w->p[k].ter);
    for (int k = 0; k < w->ntp; k++) REPLY("dim %d %d %d cells, territory %d\n", w->tp[k].at.x, w->tp[k].at.y, w->tp[k].cells, w->tp[k].ter);
}
internal void watch_install(const char *autotest_ini) {
    g_fog_patrols = g_set_ai[1].fog_patrols;
    g_watch_towers = g_set_ai[1].watch_towers;
    if (autotest_ini) g_v_fog_patrols = ini_int("autotest", "v_fog_patrols", -1, autotest_ini);
    if (autotest_ini) g_v_watch_towers = ini_int("autotest", "v_watch_towers", -1, autotest_ini);
}
internal void watch_new_game(void) {
    for (int i = 0; i < 16; i++) { free(g_wt[i].last); free(g_wt[i].share); memset(&g_wt[i], 0, sizeof g_wt[i]); }
}
