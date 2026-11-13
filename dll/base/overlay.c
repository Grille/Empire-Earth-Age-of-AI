// THE DEBUG OVERLAY's shapes ([display] overlay; docs/renderer.md): lines, circles and tiles on the map, in tiles, that
// the own renderer draws into the 3D view - on the ground, under the interface (engine/render_overlay.c). Whoever
// knows a plan adds it to a layer ("routes", "waves" ...) under a key of its own (the subgroup, the player): overlay_begin
// drops what that key had and starts it anew, the shapes up to overlay_end are its; they stay until the key is drawn
// again or, ttl game milliseconds after it was (0: never), drop out. A layer off (overlay_on) costs its producer a test.
// The shapes are kept here, not in the renderer: the AI may use base/ and game/ only.

// The screen's own (OV_MARK, OV_ARROW, OV_TEXT): at a place on the map, but their size in the screen's pixels and over
// everything of the map - read at any zoom (the AI's tactical map, ai/ai_overlay.c). Those drawn under an anchor
// (overlay_anchor: a unit's serial, an army's own number) move smoothly between their producer's updates: the renderer
// glides the anchor's mark from where it was drawn to its new place over the time the updates come apart, and moves the
// anchor's text and arrows' starts with it (render_overlay.c ro_glide).
#define OV_LAYERS 16
#define OV_SHAPES 65536   // all layers on a big map: the danger and fronts layers' runs of tiles, the AI's dots
#define OV_TEXT_MAX 47
enum { OV_LINE, OV_CIRCLE, OV_TILE, OV_RING, OV_MARK, OV_ARROW, OV_TEXT };
enum { OVM_SQUARE, OVM_DIAMOND, OVM_CIRCLE, OVM_TRIANGLE, OVM_HOLLOW = 0x80 };   // a mark's shape, | OVM_HOLLOW: its outline only
typedef struct {
    uint8_t kind, layer, sub;   // sub: a mark's shape (OVM_)
    uint32_t argb;
    uintptr_t key;
    uint32_t until;   // game time it drops out (0: kept)
    float a[4];       // line x0 y0 x1 y1; circle x y radius; tiles x y and their count along the row; mark x y size; arrow
                      // x0 y0 x1 y1; text x y row
    float width;      // a line's or circle's width in tiles, an arrow's in pixels
    char t[OV_TEXT_MAX + 1];   // a text's
    uint32_t id;      // its anchor (overlay_anchor; 0: none)
} ov_shape;
internal struct {
    CRITICAL_SECTION cs;
    int ready;
    char names[OV_LAYERS][16];
    int n_layers;
    uint32_t on;      // the layers shown (bit per layer)
    uint32_t kept;    // the layers shown whatever was asked for (overlay_keep: a view of the mod's own, not a debug one)
    char want[256];   // the layers asked for by name (the setting, the pipe), before they all registered
    ov_shape s[OV_SHAPES];
    int n;
    // the open overlay_begin
    int open_layer;
    uintptr_t open_key;
    uint32_t open_until;
    uint32_t open_id;
    unsigned dropped;   // shapes refused, the store full
} g_ov;

internal void ov_init(void) {
    if (g_ov.ready) return;
    InitializeCriticalSection(&g_ov.cs);
    g_ov.ready = 1;
}
// name in the comma list w ("all": every one; "-name": not that one whatever else the list says - overlay_toggle's)
internal int ov_wanted_in(const char *w, const char *name) {
    size_t n = strlen(name);
    int hit = 0;
    for (const char *p = w; *p; ) {
        while (*p == ',' || *p == ' ') p++;
        const char *e = p;
        while (*e && *e != ',' && *e != ' ') e++;
        if (*p == '-' && (size_t)(e - p - 1) == n && !strncmp(p + 1, name, n)) return 0;
        if (((size_t)(e - p) == n && !strncmp(p, name, n)) || (e - p == 3 && !strncmp(p, "all", 3))) hit = 1;
        p = e;
    }
    return hit;
}
internal int ov_wanted(const char *name) { return ov_wanted_in(g_ov.want, name); }
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
    g_ov.on = g_ov.kept;
    for (int i = 0; i < g_ov.n_layers; i++) if (ov_wanted(g_ov.names[i])) g_ov.on |= 1u << i;
    LeaveCriticalSection(&g_ov.cs);
}
// one layer on or off, the others as they are (the pipe's overlay <name> - the player, 2026-11-13: "overlay ai, should
// be a toggle for that flag only"); 1 it is on now. The list loses the name and its "-name", then gets the name (on and
// not asked for otherwise) or "-name" (off but still asked for by "all") - a layer that hasn't drawn yet stays in "all".
internal int overlay_toggle(const char *name) {
    ov_init();
    EnterCriticalSection(&g_ov.cs);
    int on = !ov_wanted(name);
    char w[sizeof g_ov.want] = "";
    size_t n = 0, len = strlen(name);
    for (const char *p = g_ov.want; *p; ) {
        while (*p == ',' || *p == ' ') p++;
        const char *e = p;
        while (*e && *e != ',' && *e != ' ') e++;
        const char *q = *p == '-' ? p + 1 : p;
        if (e > p && !((size_t)(e - q) == len && !strncmp(q, name, len)) && n < sizeof w)
            n += (size_t)snprintf(w + n, sizeof w - n, "%s%.*s", n ? "," : "", (int)(e - p), p);
        p = e;
    }
    if (n < sizeof w && on != ov_wanted_in(w, name))
        snprintf(w + n, sizeof w - n, "%s%s%s", n ? "," : "", on ? "" : "-", name);
    LeaveCriticalSection(&g_ov.cs);
    overlay_show(w);
    return on;
}
// a layer shown (or not) whatever the setting asks for: the scenario editor's orders (engine/editor_orders.c)
internal void overlay_keep(int layer, int on) {
    if (layer < 0) return;
    ov_init();
    EnterCriticalSection(&g_ov.cs);
    uint32_t bit = 1u << layer;
    if (on) g_ov.kept |= bit, g_ov.on |= bit;
    else { g_ov.kept &= ~bit; if (!ov_wanted(g_ov.names[layer])) g_ov.on &= ~bit; }
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
    g_ov.open_id = 0;
}
// the screen's shapes from here on are id's (its mark first: the place they glide by), 0 none - an id unique across
// layers and keys (a unit's serial; an army's own number with a high bit of its own)
internal void overlay_anchor(uint32_t id) { g_ov.open_id = id; }
internal void overlay_end(void) { LeaveCriticalSection(&g_ov.cs); }
internal ov_shape *ov_add(int kind, uint32_t argb, float width) {
    if (g_ov.n == OV_SHAPES) { g_ov.dropped++; return NULL; }   // full: the rest of this plan isn't shown (counted: the pipe's overlay)
    ov_shape *s = &g_ov.s[g_ov.n++];
    s->kind = (uint8_t)kind;
    s->sub = 0;
    s->t[0] = 0;
    s->id = g_ov.open_id;
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
// the game's command circle (its texture, tinted by argb - the renderer's plain circle where it has none), r its outer
// radius
internal void overlay_ring(vec2 c, float r, uint32_t argb) {
    ov_shape *s = ov_add(OV_RING, argb, 0);
    if (s) { s->a[0] = c.x; s->a[1] = c.y; s->a[2] = r; }
}
internal void overlay_tiles(ivec2 t, int n, uint32_t argb) {   // n tiles along the row from t
    ov_shape *s = ov_add(OV_TILE, argb, 0);
    if (s) { s->a[0] = (float)t.x; s->a[1] = (float)t.y; s->a[2] = (float)n; }
}
// the screen's: a mark of shape (OVM_) px pixels across at c, edged in black
internal void overlay_mark(vec2 c, int shape, float px, uint32_t argb) {
    ov_shape *s = ov_add(OV_MARK, argb, 0);
    if (s) { s->sub = (uint8_t)shape; s->a[0] = c.x; s->a[1] = c.y; s->a[2] = px; }
}
// ... an arrow from a to b, px pixels wide, its head at b
internal void overlay_arrow(vec2 a, vec2 b, float px, uint32_t argb) {
    ov_shape *s = ov_add(OV_ARROW, argb, px);
    if (s) { s->a[0] = a.x; s->a[1] = a.y; s->a[2] = b.x; s->a[3] = b.y; }
}
// ... a line of text centred on c, row lines below a mark there (negative: above it), at most OV_TEXT_MAX characters
internal PRINTF_LIKE(4, 5) void overlay_text(vec2 c, int row, uint32_t argb, const char *fmt, ...) {
    ov_shape *s = ov_add(OV_TEXT, argb, 0);
    if (!s) return;
    s->a[0] = c.x; s->a[1] = c.y; s->a[2] = (float)row;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(s->t, sizeof s->t, fmt, ap);
    va_end(ap);
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
