// UNITS IN THE LANGUAGE (lang/unit.cs: the kind's members; relations.c x_obj_member_of hands them here): a unit as a
// value is its id (xval obj: the id + 1, okind XK_UNIT) - a handle, not a pointer: it stays a value when the unit is
// destroyed, and the unit is looked up again (unit_by_id) whenever it is read, so a script keeps no pointer into the
// game across a wait. Alive: on the map with hit points left; a gone unit's members are a mistake but for id and alive.
// A unit p.spawn returned before the game made it is a handle to come (obj -(its slot + 1): SPAWNING below).

// SPAWNING (p.spawn(type, at, count)): the game makes a unit from a message (the new-unit cheat's, as the spawn command
// sends) at one of its next steps, so the units aren't there when p.spawn returns - it returns handles to come, and
// the messages go out at once: spawns one after another are made together, nothing waits for another. The script
// holds - waits the game's next steps - only when it uses one of them (a member, ==) or asks the map what is on it
// (p.units(), game.unit(id)): then every spawn sent is matched with the units it made - of its owner and type, not
// there when it was sent (its list of those that were), at one of its places - for at most X_SPAWN_HOLD game seconds;
// one that didn't come is gone. A trigger can't wait: one to come there is a mistake. A dry run makes no units: its
// handles can't be read.
#define X_SPAWN_SLOTS 8192
#define X_SPAWN_HOLD 5.0
struct x_spawn { void *owner; int type, first, n; double sent; unsigned *had; int nhad; };   // its slots first..first+n-1
internal struct x_spawn g_xsp[256];
internal int g_xsp_n;
enum { XS_COMING, XS_THERE, XS_NEVER, XS_DRY };
internal struct { float xy[2]; unsigned id; unsigned char state; } g_xslot[X_SPAWN_SLOTS];
internal int g_xslot_n, g_xsp_open;   // slots used; slots still coming
internal int x_id_order(const void *a, const void *b) { unsigned x = *(const unsigned *)a, y = *(const unsigned *)b; return x < y ? -1 : x > y; }
internal void x_spawn_seen(ee_unit *u, void *ctx) {   // one unit on the map: one a spawn made?
    (void)ctx;
    void *owner = unit_get_owner(u);
    int type = unit_type_get_id(unit_get_type_data(u));
    unsigned id = (unsigned)unit_get_serial(u);
    if (unit_get_hp(u) <= 0) return;
    vec3 p = unit_get_pos(u);
    for (int i = 0; i < g_xsp_n; i++) {
        struct x_spawn *s = &g_xsp[i];
        if (s->owner != owner || s->type != type || bsearch(&id, s->had, (size_t)s->nhad, sizeof id, x_id_order)) continue;
        int best = -1;
        for (int k = s->first; k < s->first + s->n; k++) {
            if (g_xslot[k].state == XS_THERE && g_xslot[k].id == id) return;   // matched already
            float dx = p.x - g_xslot[k].xy[0], dy = p.y - g_xslot[k].xy[1];
            if (g_xslot[k].state == XS_COMING && dx * dx + dy * dy <= 1.0f && best < 0) best = k;
        }
        if (best >= 0) { g_xslot[best].state = XS_THERE; g_xslot[best].id = id; g_xsp_open--; return; }
    }
}
// the spawns sent matched with what the map holds - waiting the game's steps while one is to come (wait: a script's
// turn may; a trigger's may not), at most X_SPAWN_HOLD game seconds after it was sent
internal void x_spawn_settle(int wait) {
    while (g_xsp_open > 0) {
        grid_each(x_spawn_seen, NULL);
        double now = g_x_hooks.now ? g_x_hooks.now() : game_time() / 1000.0;
        for (int i = 0; i < g_xsp_n; i++)
            for (int k = g_xsp[i].first; k < g_xsp[i].first + g_xsp[i].n; k++)
                if (g_xslot[k].state == XS_COMING && now - g_xsp[i].sent > X_SPAWN_HOLD) { g_xslot[k].state = XS_NEVER; g_xsp_open--; }
        if (!g_xsp_open || !wait || !g_x_hooks.wait) break;
        g_x_hooks.wait(now + 0.001);   // (the game's next step)
    }
    if (g_xsp_open) return;
    for (int i = 0; i < g_xsp_n; i++) free(g_xsp[i].had);   // all settled: the spawns' lists done with
    g_xsp_n = 0;
}
// a handle's state: there (*id), or why it is none - a handle to come waits for it (where waiting may be)
internal int x_unit_state(const struct xval *o, unsigned *id) {
    if (o->obj > 0) { *id = (unsigned)(o->obj - 1); return XS_THERE; }
    int k = -o->obj - 1;
    if (g_xslot[k].state == XS_COMING) x_spawn_settle(1);
    *id = g_xslot[k].id;
    return g_xslot[k].state;
}
internal int x_unit_key(const struct xval *v) {   // == of units: the id of one that came (expr.c xcmp, host.h obj_key)
    unsigned id;
    if (v->okind != XK_UNIT) return v->obj;
    return x_unit_state(v, &id) == XS_THERE ? (int)id + 1 : v->obj;
}

// a unit as a value: its type's identifier in a text, its id
internal void x_unit(struct xval *v, ee_unit *u) {
    xstr(v, unit_ident(unit_type_get_id(unit_get_type_data(u))));
    v->obj = unit_get_serial(u) + 1;
    v->okind = XK_UNIT;
}
// the unit a value holds while it is on the map, NULL gone (*why: what is told then)
internal ee_unit *x_unit_of(const struct xval *o, const char **why) {
    unsigned id;
    int st = x_unit_state(o, &id);
    ee_unit *u = st == XS_THERE ? unit_by_id(id) : NULL;
    *why = st == XS_COMING ? "not on the map yet - a trigger can't wait for a spawned unit: use it in the script" :
           st == XS_NEVER ? "the spawned unit never came (its place taken or off the land?)" :
           st == XS_DRY ? "a dry run makes no units - a spawned one can't be read" : "the unit is gone (destroyed) - check u.alive first";
    return u && unit_get_type_data(u) && unit_get_hp(u) > 0 ? u : NULL;
}
// u.<member> (a member lang/unit.cs declares - expr.c checked the name)
internal int x_unit_member_of(struct xctx *c, const struct xval *o, const char *name, struct xval *v) {
    const char *why;
    ee_unit *u = x_unit_of(o, &why);
    unsigned id;
    if (!strcmp(name, "alive")) { xnum(v, u != NULL); return 1; }
    if (!strcmp(name, "id") && x_unit_state(o, &id) == XS_THERE) { xnum(v, id); return 1; }
    if (!u) { x_host_fail(c, "%s.%s: %s", xtext_of(o), name, why); return 1; }
    int type = unit_type_get_id(unit_get_type_data(u));
    if (!strcmp(name, "type")) xdual(v, type, (unsigned)type < G_TYPES_N ? g_unit_ident[type] : "?");   // as unit.<identifier>
    else if (!strcmp(name, "owner")) x_player(v, unit_get_owner(u), "owner");
    else if (!strcmp(name, "pos")) { vec3 p = unit_get_pos(u); double xy[2] = { p.x, p.y }; xvec(v, xy, 2); }
    else if (!strcmp(name, "hp")) xnum(v, unit_get_hp(u));
    else if (!strcmp(name, "max_hp")) { ee_unit_stats *st = unit_get_stats(u); xnum(v, st ? unit_stats_get_hp(st) : 0); }
    else return 0;
    return 1;
}
// player pl's units on the map now (p.units(filter) - all without one), by id: a list the same on every PC
struct x_units_scan { void *owner; const unsigned char *sel; unsigned *ids; int n, cap; };
internal void x_units_add(ee_unit *u, void *ctx) {
    struct x_units_scan *s = ctx;
    if (unit_get_owner(u) != s->owner || unit_get_hp(u) <= 0) return;
    int t = unit_type_get_id(unit_get_type_data(u));
    if (s->sel && ((unsigned)t >= G_TYPES_N || !s->sel[t])) return;
    if (s->n == s->cap) {
        unsigned *m = realloc(s->ids, (size_t)(s->cap = s->cap ? s->cap * 2 : 256) * sizeof *m);
        if (!m) return;
        s->ids = m;
    }
    s->ids[s->n++] = (unsigned)unit_get_serial(u);
}
internal int x_units_of(struct xctx *c, int pl, const struct xval *a, int na, struct xval *v) {
    static unsigned char sel[G_TYPES_N];
    struct x_units_scan s = { player_at(pl), NULL, NULL, 0, 0 };
    if (na) {
        memset(sel, 0, sizeof sel);
        if (!x_type_filter("units", &a[0], sel)) { c->failed++; c->missed = 1; xnothing(v); return 1; }
        s.sel = sel;
    }
    x_spawn_settle(c->script);   // the spawns sent among them
    grid_each(x_units_add, &s);
    qsort(s.ids, (size_t)s.n, sizeof *s.ids, x_id_order);
    struct xval *it = s.n ? x_alloc((size_t)s.n * sizeof *it) : NULL;
    xnothing(v);
    for (int i = 0; it && i < s.n; i++) x_unit(&it[i], unit_by_id(s.ids[i]));
    if (it) { v->it = it; v->ln = v->w = s.n; }
    free(s.ids);
    return 1;
}
// game.unit(id): the unit while it is on the map, else nothing (null)
internal void x_unit_by_id(struct xctx *c, const struct xval *id, struct xval *v) {
    double n = xto_num(id);
    const char *why;
    xnothing(v);
    if (!id->has_n || n != (double)(unsigned)n) { x_host_fail(c, "game.unit(%.40s): an id - u.id", xtext_of(id)); return; }
    x_spawn_settle(c->script);
    struct xval t = { 0 };
    t.obj = (int)((unsigned)n + 1);
    ee_unit *u = x_unit_of(&t, &why);
    if (u) x_unit(v, u);
}
internal void x_had_add(ee_unit *u, void *ctx) {   // a spawn's list of the units of its owner and type there before it
    struct x_spawn *s = ctx;
    if (unit_get_owner(u) != s->owner || unit_type_get_id(unit_get_type_data(u)) != s->type) return;
    unsigned *m = (s->nhad & 63) ? s->had : realloc(s->had, (size_t)(s->nhad + 64) * sizeof *m);
    if (!m) return;
    s->had = m;
    s->had[s->nhad++] = (unsigned)unit_get_serial(u);
}
// p.spawn(type, at, count): the new-unit messages sent, the handles to come returned
internal void x_units_spawn(struct xctx *c, int pl, const struct xval *a, int na, struct xval *v) {
    int type = a[0].has_n ? (int)a[0].n : -1, n = na > 2 ? (int)xto_num(&a[2]) : 1;
    char t[120];
    xnothing(v);
    if (g_x_setup) { x_host_fail(c, "player %d.spawn: not before the game starts", pl); return; }
    if (type <= 0 || type >= G_TYPES_N || !a[0].has_s || strcmp(a[0].s, g_unit_ident[type]) || a[0].ln) {
        x_host_fail(c, "player %d.spawn(%.40s, ...): one type - unit.<identifier>", pl, xtext_of(&a[0]));
        return;
    }
    if (a[1].kind != XV_VEC || a[1].vn != 2) { x_host_fail(c, "player %d.spawn: its place a vec2 - vec(x, y) in tiles", pl); return; }
    if (na > 2 && (!a[2].has_n || a[2].n != (double)n || n < 1 || n > 100)) { x_host_fail(c, "player %d.spawn: 1 to 100 units", pl); return; }
    ee_server *srv = game_server();
    ee_map *map = game_map();
    if (!srv || !map || !type_data(type)) { x_host_fail(c, "player %d.spawn: no game, or no type %d", pl, type); return; }
    if (g_xsp_n == 256 || g_xslot_n + n > X_SPAWN_SLOTS) { x_host_fail(c, "player %d.spawn: more than %d spawned units in this game", pl, X_SPAWN_SLOTS); return; }
    int side = 1, first = g_xslot_n;
    while (side * side < n) side++;
    for (int i = 0; i < n; i++) {   // as the spawn command lays them out: a square, 2 tiles apart
        float *xy = g_xslot[first + i].xy;
        xy[0] = (float)((int)a[1].vc[0] + (i % side) * 2) + 0.5f;
        xy[1] = (float)((int)a[1].vc[1] + (i / side) * 2) + 0.5f;
        if (!spawn_on_map(map, xy)) { x_host_fail(c, "player %d.spawn: %.0f,%.0f is off the map's land diamond", pl, xy[0], xy[1]); return; }
    }
    snprintf(t, sizeof t, "player %d.spawn(unit.%s, vec(%g, %g), %d)", pl, g_unit_ident[type], a[1].vc[0], a[1].vc[1], n);
    int dry = x_did(c, t);
    struct xval *it = x_alloc((size_t)n * sizeof *it);
    if (!it) return;
    g_xslot_n += n;
    if (!dry) {
        struct x_spawn *s = &g_xsp[g_xsp_n++];
        *s = (struct x_spawn){ player_at(pl), type, first, n, g_x_hooks.now ? g_x_hooks.now() : game_time() / 1000.0, NULL, 0 };
        grid_each(x_had_add, s);
        qsort(s->had, (size_t)s->nhad, sizeof *s->had, x_id_order);
        for (int i = 0; i < n; i++) {
            void *msg = new_unit_msg_alloc();
            if (!msg) { g_xslot[first + i].state = XS_NEVER; continue; }
            new_unit_msg_init(msg, &type, g_xslot[first + i].xy, (char)pl);
            server_send_all(srv, msg);
            g_xslot[first + i].state = XS_COMING;
            g_xsp_open++;
        }
    }
    for (int i = 0; i < n; i++) {
        if (dry) g_xslot[first + i].state = XS_DRY;
        xstr(&it[i], g_unit_ident[type]);
        it[i].obj = -(first + i + 1);
        it[i].okind = XK_UNIT;
    }
    v->it = it;
    v->ln = v->w = n;
}
