// THE DEBUG OVERLAY's shapes ([display] overlay; docs/renderer.md): lines, circles and tiles on the map, in tiles, that
// the own renderer draws into the 3D view - on the ground, under the interface (engine/render_overlay.c). Whoever
// knows a plan adds it to a layer ("routes", "waves" ...) under a key of its own (the subgroup, the player): overlay_begin
// drops what that key had and starts it anew, the shapes up to overlay_end are its; they stay until the key is drawn
// again or, ttl game milliseconds after it was (0: never), drop out. A layer off (overlay_on) costs its producer a test.
// The shapes are kept here, not in the renderer: the AI may use base/ and game/ only.

#define OV_LAYERS 16
#define OV_SHAPES 16384
enum { OV_LINE, OV_CIRCLE, OV_TILE };
typedef struct {
    uint8_t kind, layer;
    uint32_t argb;
    uintptr_t key;
    uint32_t until;   // game time it drops out (0: kept)
    float a[4];       // line x0 y0 x1 y1; circle x y radius; tiles x y and their count along the row
    float width;      // a line's or circle's width in tiles
} ov_shape;
internal struct {
    CRITICAL_SECTION cs;
    int ready;
    char names[OV_LAYERS][16];
    int n_layers;
    uint32_t on;      // the layers shown (bit per layer)
    char want[256];   // the layers asked for by name (the setting, the pipe), before they all registered
    ov_shape s[OV_SHAPES];
    int n;
    // the open overlay_begin
    int open_layer;
    uintptr_t open_key;
    uint32_t open_until;
} g_ov;

internal void ov_init(void) {
    if (g_ov.ready) return;
    InitializeCriticalSection(&g_ov.cs);
    g_ov.ready = 1;
}
internal int ov_wanted(const char *name) {   // name in the comma list g_ov.want ("all": every one)
    size_t n = strlen(name);
    for (const char *p = g_ov.want; *p; ) {
        while (*p == ',' || *p == ' ') p++;
        const char *e = p;
        while (*e && *e != ',' && *e != ' ') e++;
        if (((size_t)(e - p) == n && !strncmp(p, name, n)) || (e - p == 3 && !strncmp(p, "all", 3))) return 1;
        p = e;
    }
    return 0;
}
// the layer's number (made on its first use), -1 when all 16 are taken
internal int overlay_layer(const char *name) {
    ov_init();
    EnterCriticalSection(&g_ov.cs);
    int i = 0;
    while (i < g_ov.n_layers && strcmp(g_ov.names[i], name)) i++;
    if (i == g_ov.n_layers) {
        if (i == OV_LAYERS) i = -1;
        else {
            lstrcpynA(g_ov.names[i], name, sizeof g_ov.names[i]);
            g_ov.n_layers++;
            if (ov_wanted(name)) g_ov.on |= 1u << i;
        }
    }
    LeaveCriticalSection(&g_ov.cs);
    return i;
}
// the layers to show, a comma list of names ("routes,waves", "all", "" none)
internal void overlay_show(const char *names) {
    ov_init();
    EnterCriticalSection(&g_ov.cs);
    lstrcpynA(g_ov.want, names ? names : "", sizeof g_ov.want);
    g_ov.on = 0;
    for (int i = 0; i < g_ov.n_layers; i++) if (ov_wanted(g_ov.names[i])) g_ov.on |= 1u << i;
    LeaveCriticalSection(&g_ov.cs);
}
internal int overlay_on(int layer) { return layer >= 0 && (g_ov.on >> layer & 1); }

// key's shapes of layer anew from here to overlay_end; now: the game time, ttl: how long they stay (0: until replaced)
internal void overlay_begin(int layer, uintptr_t key, uint32_t now, uint32_t ttl) {
    ov_init();
    EnterCriticalSection(&g_ov.cs);
    int k = 0;
    for (int i = 0; i < g_ov.n; i++)
        if (g_ov.s[i].layer != layer || g_ov.s[i].key != key) g_ov.s[k++] = g_ov.s[i];
    g_ov.n = k;
    g_ov.open_layer = layer;
    g_ov.open_key = key;
    g_ov.open_until = ttl ? now + ttl : 0;
}
internal void overlay_end(void) { LeaveCriticalSection(&g_ov.cs); }
internal ov_shape *ov_add(int kind, uint32_t argb, float width) {
    if (g_ov.n == OV_SHAPES) return NULL;   // full: the rest of this plan isn't shown
    ov_shape *s = &g_ov.s[g_ov.n++];
    s->kind = (uint8_t)kind;
    s->layer = (uint8_t)g_ov.open_layer;
    s->key = g_ov.open_key;
    s->until = g_ov.open_until;
    s->argb = argb;
    s->width = width;
    return s;
}
// between overlay_begin and overlay_end: a line on the ground, a circle, a tile filled (argb: its colour and opacity)
internal void overlay_line(vec2 a, vec2 b, uint32_t argb, float width) {
    ov_shape *s = ov_add(OV_LINE, argb, width);
    if (s) { s->a[0] = a.x; s->a[1] = a.y; s->a[2] = b.x; s->a[3] = b.y; }
}
internal void overlay_circle(vec2 c, float r, uint32_t argb, float width) {
    ov_shape *s = ov_add(OV_CIRCLE, argb, width);
    if (s) { s->a[0] = c.x; s->a[1] = c.y; s->a[2] = r; }
}
internal void overlay_tiles(ivec2 t, int n, uint32_t argb) {   // n tiles along the row from t
    ov_shape *s = ov_add(OV_TILE, argb, 0);
    if (s) { s->a[0] = (float)t.x; s->a[1] = (float)t.y; s->a[2] = (float)n; }
}
// a player's colour for its shapes (index 1..8 as the game's slots; the game's own colours roughly)
internal uint32_t overlay_player_argb(int player, unsigned alpha) {
    static const uint32_t c[9] = { 0xFFFFFF, 0x3060FF, 0xFF3030, 0x30D040, 0xFFE040, 0x30E0E0, 0xE040E0, 0xFF9020, 0xC0C0C0 };
    return (uint32_t)(alpha & 255) << 24 | c[player >= 1 && player <= 8 ? player : 0];
}
// the shapes of the layers shown and still due at now, copied out for drawing (the renderer's thread); their count
internal int overlay_copy(ov_shape *out, int cap, uint32_t now) {
    if (!g_ov.ready || !g_ov.on) return 0;
    EnterCriticalSection(&g_ov.cs);
    int n = 0, k = 0;
    for (int i = 0; i < g_ov.n; i++) {
        ov_shape *s = &g_ov.s[i];
        if (s->until && (int32_t)(now - s->until) >= 0) continue;   // dropped out
        g_ov.s[k++] = *s;
        if (n < cap && overlay_on(s->layer)) out[n++] = *s;
    }
    g_ov.n = k;
    LeaveCriticalSection(&g_ov.cs);
    return n;
}
