// DANGER MAP (`danger_routes`, [all], default 1): per player the tiles within reach of the enemies it knows of - each tile how many
// threats cover it (0..DANGER_MAX). The AI fills its own (ai/attack.c danger_refresh: the enemy defences it has seen,
// enemy soldiers and ships it sees at their reach, where its citizens were hurt lately), only what it knows; the
// route search (engine/shiproute.c) makes those tiles dearer for its units that can't fight (citizens, fishing boats,
// transports), so they go round where there is a way round and through only where there is none. A map not refreshed
// for DANGER_STALE ms counts as none (the AI gone, the setting off).

#define DANGER_MAX 3
#define DANGER_STALE 15000

internal struct { uint8_t *lv; int size; uint32_t at; } g_danger[16];
internal unsigned g_dr_routes, g_dr_tiles;   // the route search: searches with a danger map, danger tiles left on their routes

// the map of player pi as of now, every tile 0 (refreshed now)
internal int danger_begin(unsigned pi, int size, uint32_t now) {
    pi &= 15;
    if (size <= 0) return 0;
    if (g_danger[pi].size != size) {
        free(g_danger[pi].lv);
        g_danger[pi].lv = calloc((size_t)size * (size_t)size, 1);
        g_danger[pi].size = g_danger[pi].lv ? size : 0;
    }
    if (!g_danger[pi].lv) return 0;
    memset(g_danger[pi].lv, 0, (size_t)size * (size_t)size);
    g_danger[pi].at = now ? now : 1;
    return 1;
}

// one threat more on the tiles within r of cx, cy
internal void danger_disc(unsigned pi, float cx, float cy, float r) {
    pi &= 15;
    int size = g_danger[pi].size;
    if (!g_danger[pi].lv || r <= 0) return;
    int x0 = (int)floorf(cx - r), x1 = (int)floorf(cx + r), y0 = (int)floorf(cy - r), y1 = (int)floorf(cy + r);
    for (int y = y0 < 0 ? 0 : y0; y <= y1 && y < size; y++)
        for (int x = x0 < 0 ? 0 : x0; x <= x1 && x < size; x++) {
            float dx = (float)x + 0.5f - cx, dy = (float)y + 0.5f - cy;
            uint8_t *t = &g_danger[pi].lv[y * size + x];
            if (dx * dx + dy * dy <= r * r && *t < DANGER_MAX) (*t)++;
        }
}

// player pi's map, if fresh (NULL: none)
internal const uint8_t *danger_map(unsigned pi, int size) {
    pi &= 15;
    uint32_t now = game_time();
    if (!g_danger[pi].lv || g_danger[pi].size != size || now < g_danger[pi].at || now - g_danger[pi].at > DANGER_STALE) return NULL;
    return g_danger[pi].lv;
}

internal void danger_new_game(void) {
    for (int i = 0; i < 16; i++) g_danger[i].at = 0;
    g_dr_routes = g_dr_tiles = 0;
}
