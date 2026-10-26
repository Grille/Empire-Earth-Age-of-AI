// DANGER MAP (`danger_routes`, [ai_base], default 1): per player two layers of tiles. DANGER_THREATS: the tiles within
// reach of the enemies it knows of - each tile how many threats cover it (0..DANGER_MAX). The AI fills its own
// (ai/attack.c danger_refresh: the enemy defences it has seen, enemy soldiers and ships it sees at their reach, where
// its citizens were hurt lately), only what it knows; the route search (engine/shiproute.c) makes those tiles dearer
// for its units that can't fight (citizens, fishing boats, transports), so they go round where there is a way round
// and through only where there is none. DANGER_LOST: where its fight is lost - the reach of enemy forces it sees that
// its own nearby can't beat (ai/cover.c cv_zones) -, for its soldiers on their way somewhere (a move, not an attack):
// they go round, and a march of its attack subgroups that would end in one stops at its edge (ai/landing.c) - they
// wait there till enough have come to win, then go in together. A layer not refreshed for DANGER_STALE ms counts as
// none (the AI gone, the setting off).

#define DANGER_MAX 3
#define DANGER_STALE 15000
#define DANGER_THREATS 0
#define DANGER_LOST 1

internal struct { uint8_t *lv; int size; uint32_t at; } g_danger[16][2];
internal unsigned g_dr_routes, g_dr_tiles;   // the route search: searches with a danger map, danger tiles left on their routes
internal unsigned g_dr_lost_routes, g_dr_legs_held;   // soldiers' routes by the lost layer; marches stopped at its edge

// layer k of player pi's map as of now, every tile 0 (refreshed now)
internal int danger_begin(unsigned pi, int k, int size, uint32_t now) {
    pi &= 15; k &= 1;
    if (size <= 0) return 0;
    if (g_danger[pi][k].size != size) {
        free(g_danger[pi][k].lv);
        g_danger[pi][k].lv = calloc((size_t)size * (size_t)size, 1);
        g_danger[pi][k].size = g_danger[pi][k].lv ? size : 0;
    }
    if (!g_danger[pi][k].lv) return 0;
    memset(g_danger[pi][k].lv, 0, (size_t)size * (size_t)size);
    g_danger[pi][k].at = now ? now : 1;
    return 1;
}

// one threat more on layer k's tiles within r of cx, cy
internal void danger_disc(unsigned pi, int k, float cx, float cy, float r) {
    pi &= 15; k &= 1;
    int size = g_danger[pi][k].size;
    if (!g_danger[pi][k].lv || r <= 0) return;
    int x0 = (int)floorf(cx - r), x1 = (int)floorf(cx + r), y0 = (int)floorf(cy - r), y1 = (int)floorf(cy + r);
    for (int y = y0 < 0 ? 0 : y0; y <= y1 && y < size; y++)
        for (int x = x0 < 0 ? 0 : x0; x <= x1 && x < size; x++) {
            float dx = (float)x + 0.5f - cx, dy = (float)y + 0.5f - cy;
            uint8_t *t = &g_danger[pi][k].lv[y * size + x];
            if (dx * dx + dy * dy <= r * r && *t < DANGER_MAX) (*t)++;
        }
}

// layer k of player pi's map, if fresh (NULL: none)
internal const uint8_t *danger_map(unsigned pi, int k, int size) {
    pi &= 15; k &= 1;
    uint32_t now = game_time();
    if (!g_danger[pi][k].lv || g_danger[pi][k].size != size || now < g_danger[pi][k].at || now - g_danger[pi][k].at > DANGER_STALE) return NULL;
    return g_danger[pi][k].lv;
}

// layer k's tiles in danger (the logs)
internal int danger_count(unsigned pi, int k) {
    pi &= 15; k &= 1;
    int n = 0, size = g_danger[pi][k].size;
    for (int i = 0; g_danger[pi][k].lv && i < size * size; i++) n += g_danger[pi][k].lv[i] != 0;
    return n;
}

internal void danger_new_game(void) {
    for (int i = 0; i < 16; i++) g_danger[i][0].at = g_danger[i][1].at = 0;
    g_dr_routes = g_dr_tiles = g_dr_lost_routes = g_dr_legs_held = 0;
}
