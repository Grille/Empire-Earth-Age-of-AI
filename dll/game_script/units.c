// UNITS IN THE LANGUAGE (lang/unit.cs: the kind's members; relations.c x_obj_member_of hands them here): a unit as a
// value is its id (xval obj: the id + 1, okind XK_UNIT) - a handle, not a pointer: it stays a value when the unit is
// destroyed, and the unit is looked up again (unit_by_id) whenever it is read, so a script keeps no pointer into the
// game across a wait. Alive: on the map with hit points left; a gone unit's members are a mistake but for id and alive.
// A unit p.spawn returned before the game made it is a handle to come (obj -(its slot + 1): SPAWNING below).

// SPAWNING (p.spawn(type, at, count)): the units are made as the scenario's Create effect makes them (game/api/unit.c
// unit_create): in the simulation on every PC alike, no message - at the next point between the simulation's steps
// (x_spawn_flush from relations.c x_effects_flush, where the scripts' wins are carried out and the game's own triggers
// run). p.spawn returns handles to come at once and the script goes on: spawns one after another are made together.
// It holds - waits for that point - only when it uses one of them (a member, ==) or asks the map what is on it
// (p.units(), game.unit(id)). A trigger runs at that point already: its spawns are made at once. A dry run makes no
// units: its handles can't be read.
#define X_SPAWN_SLOTS 8192
enum { XS_COMING, XS_THERE, XS_NEVER, XS_DRY };
internal struct { int x, y, type, pl; unsigned id; unsigned char state; } g_xslot[X_SPAWN_SLOTS];
internal int g_xslot_n, g_xsp_open;   // slots used; slots still coming
internal int g_x_safe;                // 1 while the triggers run (at that point: test/scripts.c script_triggers)
internal int x_id_order(const void *a, const void *b) { unsigned x = *(const unsigned *)a, y = *(const unsigned *)b; return x < y ? -1 : x > y; }
internal void x_spawn_make(int k) {   // slot k's unit made - or never (its owner out, the type made none)
    ee_unit_type *t = type_data(g_xslot[k].type);
    void *p = player_at(g_xslot[k].pl);
    ee_unit *u = t && p && !is_out(p) ? unit_create(t, g_xslot[k].x, g_xslot[k].y, g_xslot[k].pl) : NULL;
    g_xslot[k].state = u ? XS_THERE : XS_NEVER;
    if (u) g_xslot[k].id = (unsigned)unit_get_serial(u);
}
internal void x_spawn_flush(void) {   // the spawns to come made, in the order they were asked for
    if (g_xsp_open <= 0) return;
    for (int k = 0; k < g_xslot_n; k++)
        if (g_xslot[k].state == XS_COMING) x_spawn_make(k);
    g_xsp_open = 0;
}
// the spawns to come made: a script's turn waits for the safe point (the game's next step), a trigger can't wait
internal void x_spawn_settle(int wait) {
    while (g_xsp_open > 0 && wait && g_x_hooks.wait) g_x_hooks.wait((g_x_hooks.now ? g_x_hooks.now() : game_time() / 1000.0) + 0.001);
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
    *why = st == XS_COMING ? "not on the map yet - a spawned unit is made at the next step" :
           st == XS_NEVER ? "the spawned unit was never made (its owner out of the game?)" :
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
    x_spawn_settle(c->script);   // the spawns asked for among them
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
// p.spawn(type, at, count): the units asked for, the handles to come returned (a trigger's units made at once)
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
    ee_map *map = game_map();
    if (!map || !type_data(type)) { x_host_fail(c, "player %d.spawn: no game, or no type %d", pl, type); return; }
    if (g_xslot_n + n > X_SPAWN_SLOTS) { x_host_fail(c, "player %d.spawn: more than %d spawned units in this game", pl, X_SPAWN_SLOTS); return; }
    int side = 1, first = g_xslot_n;
    while (side * side < n) side++;
    for (int i = 0; i < n; i++) {   // as the spawn command lays them out: a square, 2 tiles apart
        int x = (int)a[1].vc[0] + (i % side) * 2, y = (int)a[1].vc[1] + (i / side) * 2;
        float xy[2] = { (float)x + 0.5f, (float)y + 0.5f };
        if (!spawn_on_map(map, xy)) { x_host_fail(c, "player %d.spawn: %d,%d is off the map's land diamond", pl, x, y); return; }
        g_xslot[first + i].x = x, g_xslot[first + i].y = y, g_xslot[first + i].type = type, g_xslot[first + i].pl = pl;
    }
    snprintf(t, sizeof t, "player %d.spawn(unit.%s, vec(%g, %g), %d)", pl, g_unit_ident[type], a[1].vc[0], a[1].vc[1], n);
    int dry = x_did(c, t);
    struct xval *it = x_alloc((size_t)n * sizeof *it);
    if (!it) return;
    g_xslot_n += n;
    for (int i = 0; i < n; i++) {
        g_xslot[first + i].state = dry ? XS_DRY : XS_COMING;
        if (!dry && g_x_safe) x_spawn_make(first + i);
        else if (!dry) g_xsp_open++;
        xstr(&it[i], g_unit_ident[type]);
        it[i].obj = -(first + i + 1);
        it[i].okind = XK_UNIT;
    }
    v->it = it;
    v->ln = v->w = n;
}
