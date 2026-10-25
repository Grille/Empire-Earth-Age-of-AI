// UNITS IN THE LANGUAGE (lang/unit.cs: the kind's members; relations.c x_obj_member_of hands them here): a unit as a
// value is its id (xval obj: the id + 1, okind XK_UNIT) - a handle, not a pointer: it stays a value when the unit is
// destroyed, and the unit is looked up again (unit_by_id) whenever it is read, so a script keeps no pointer into the
// game across a wait. Alive: on the map with hit points left; a gone unit's members are a mistake but for id and alive.

// a unit as a value: its type's identifier in a text, its id
internal void x_unit(struct xval *v, ee_unit *u) {
    xstr(v, unit_ident(unit_type_get_id(unit_get_type_data(u))));
    v->obj = unit_get_serial(u) + 1;
    v->okind = XK_UNIT;
}
// the unit a value holds while it is on the map, NULL gone
internal ee_unit *x_unit_of(const struct xval *o) {
    ee_unit *u = unit_by_id((unsigned)(o->obj - 1));
    return u && unit_get_type_data(u) && unit_get_hp(u) > 0 ? u : NULL;
}
// u.<member> (a member lang/unit.cs declares - expr.c checked the name)
internal int x_unit_member_of(struct xctx *c, const struct xval *o, const char *name, struct xval *v) {
    ee_unit *u = x_unit_of(o);
    if (!strcmp(name, "id")) { xnum(v, o->obj - 1); return 1; }
    if (!strcmp(name, "alive")) { xnum(v, u != NULL); return 1; }
    if (!u) { x_host_fail(c, "%s.%s: the unit is gone (destroyed) - check u.alive first", xtext_of(o), name); return 1; }
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
internal int x_id_order(const void *a, const void *b) { unsigned x = *(const unsigned *)a, y = *(const unsigned *)b; return x < y ? -1 : x > y; }
internal int x_units_of(struct xctx *c, int pl, const struct xval *a, int na, struct xval *v) {
    static unsigned char sel[G_TYPES_N];
    struct x_units_scan s = { player_at(pl), NULL, NULL, 0, 0 };
    if (na) {
        memset(sel, 0, sizeof sel);
        if (!x_type_filter("units", &a[0], sel)) { c->failed++; c->missed = 1; xnothing(v); return 1; }
        s.sel = sel;
    }
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
    xnothing(v);
    if (!id->has_n || n != (double)(unsigned)n) { x_host_fail(c, "game.unit(%.40s): an id - u.id", xtext_of(id)); return; }
    struct xval t = { 0 };
    t.obj = (int)((unsigned)n + 1);
    ee_unit *u = x_unit_of(&t);
    if (u) x_unit(v, u);
}
