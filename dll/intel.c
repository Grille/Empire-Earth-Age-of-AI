// INTEL: what an AI knows about its enemies. Included by eeaimod.c after landing.c.
//
// vis P [target] (test): what the per-unit visibility value (unit +0x14 -> +0xC, uint16 per player; seen_by tests
// its low byte) means for moving units - "seen now" or "seen once". For each enemy unit of AI P: the value, and the
// distance to the nearest unit or building of P (its eyes); a histogram by distance and kind.
struct vis_scan {
    void *pl, *target;
    int allies;                          // its allies' units are eyes too (shared vision)
    int ne, ex[4000], ey[4000];          // P's own units and buildings
    int nu; uint8_t *u[4000];            // the enemies
};
static void vis_unit(uint8_t *u, void *ctx) {
    struct vis_scan *s = ctx;
    void *o = FIELD(u, 0x18, void *);
    if (FIELD(u, 0x3C, int) <= 0 || !o || FIELD(u, 0x1C, int) < 0) return;
    if (o == s->pl || (s->allies && FIELD(o, PL_INDEX, int) > 0 && !is_enemy(s->pl, o))) { if (s->ne < 4000) { s->ex[s->ne] = FIELD(u, 0x1C, int); s->ey[s->ne++] = FIELD(u, 0x20, int); } }
    else if ((s->target ? o == s->target : is_enemy(s->pl, o)) && FIELD(o, PL_INDEX, int) > 0 && s->nu < 4000) s->u[s->nu++] = u;
}
static void *g_vis_find[16];
static void vis_owner(uint8_t *u, void *ctx) {   // player objects by index, also of humans (g_players holds the AIs)
    (void)ctx;
    void *o = FIELD(u, 0x18, void *);
    int i = o ? FIELD(o, PL_INDEX, int) : -1;
    if (i > 0 && i < 16) g_vis_find[i] = o;
}
static unsigned vis_raw(uint8_t *u, unsigned pi) {
    uint8_t *v = FIELD(u, 0x14, uint8_t *);
    uint16_t *vis = v ? FIELD(v, 0xC, uint16_t *) : NULL;
    return vis ? vis[pi] : 0xFFFFu;
}
static void cmd_vis(char *out, const char *args) {
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
        uint8_t *u = s.u[k], *td = FIELD(u, 0x2C, uint8_t *);
        int x = FIELD(u, 0x1C, int), y = FIELD(u, 0x20, int), fam = FIELD(td, 0x4B4, int);
        int kind = fam == 2 || fam == 29 ? 0 : (is_soldier(td) || is_siege(FIELD(td, 0x4B8, int))) ? 1 : fam == 4 ? 2 : (fam == 3 || fam == 7 || fam == 17) ? 3 : 4;
        float bd = 1e9f;
        for (int e = 0; e < s.ne; e++) {
            float dx = (float)(s.ex[e] - x), dy = (float)(s.ey[e] - y), d2 = dx * dx + dy * dy;
            if (d2 < bd) bd = d2;
        }
        float d = sqrtf(bd);
        int bin = d <= 8 ? 0 : d <= 15 ? 1 : d <= 25 ? 2 : 3;
        unsigned v = vis_raw(u, (unsigned)p);
        h[kind][bin][(v & 0xFF) != 0]++;
        hi[kind][bin] += (v >> 8) != 0;
        if (shown < 30 && kind != 0 && (strstr(args, "all") || ((v & 0xFF) != 0) != (d <= 12))) {
            shown++;
            REPLY("  P%d type %d (%s) at %d,%d: value %04X, nearest eye of P%d %.1f tiles\n", FIELD(FIELD(u, 0x18, void *), PL_INDEX, int),
                  FIELD(td, 0x4B8, int), kinds[kind], x, y, v, p, d);
        }
    }
    if (strstr(args, "far")) {   // the land tiles farthest from every unit (P's side and its enemies): where nobody looks
        int bx[5] = { 0 }, by[5] = { 0 }; float bd[5] = { 0 };
        for (int x = 2; x < 148; x += 3)
            for (int y = 2; y < 148; y += 3) {
                if (!area_id(x, y, 2) || area_id(x, y, 1)) continue;
                float m = 1e9f;
                for (int e = 0; e < s.ne && m > 0; e++) { float dx = (float)(s.ex[e] - x), dy = (float)(s.ey[e] - y), d2 = dx * dx + dy * dy; if (d2 < m) m = d2; }
                for (int k = 0; k < s.nu && m > 0; k++) {
                    float dx = (float)(FIELD(s.u[k], 0x1C, int) - x), dy = (float)(FIELD(s.u[k], 0x20, int) - y), d2 = dx * dx + dy * dy;
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
    for (int q = 1; q < 16; q++) if (g_vis_find[q]) REPLY(" P%d %d (vision slot +0x45C: %d)", q, FIELD(g_vis_find[q], 0x465, uint8_t), FIELD(g_vis_find[q], 0x45C, int));
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
static uint8_t *g_vt_u[32]; static int g_vt_n;
static void cmd_vistrack(char *out, const char *args) {
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
            int dx = FIELD(s.u[k], 0x1C, int) - x, dy = FIELD(s.u[k], 0x20, int) - y;
            if (dx * dx + dy * dy <= r * r) g_vt_u[g_vt_n++] = s.u[k];
        }
        REPLY("tracking %d units\n", g_vt_n);
    }
    for (int k = 0; k < g_vt_n; k++) {
        uint8_t *u = g_vt_u[k];
        int alive = 0;
        for (int j = 0; j < s.nu && !alive; j++) alive = s.u[j] == u;
        if (!alive) { REPLY("  #%d gone\n", k); continue; }
        int ux = FIELD(u, 0x1C, int), uy = FIELD(u, 0x20, int);
        float bd = 1e9f;
        for (int e = 0; e < s.ne; e++) {
            float dx = (float)(s.ex[e] - ux), dy = (float)(s.ey[e] - uy), d2 = dx * dx + dy * dy;
            if (d2 < bd) bd = d2;
        }
        REPLY("  #%d type %d at %d,%d hp %d: value %04X, nearest eye of P%d or allies %.1f\n", k, FIELD(FIELD(u, 0x2C, uint8_t *), 0x4B8, int),
              ux, uy, FIELD(u, 0x3C, int), vis_raw(u, (unsigned)p), p, sqrtf(bd));
        uint8_t *v = FIELD(u, 0x14, uint8_t *);
        uint16_t *vv = v ? FIELD(v, 0xC, uint16_t *) : NULL;
        if (vv) REPLY("      slots 0-7: %04X %04X %04X %04X %04X %04X %04X %04X\n", vv[0], vv[1], vv[2], vv[3], vv[4], vv[5], vv[6], vv[7]);
    }
}
